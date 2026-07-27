import argparse
import os
import shutil
import subprocess
import sys


SECURE_BUILD_ARTIFACTS = ["rtthread.elf", "rtthread.hex", "rtthread.bin"]


def run_command(command, workdir, extra_env=None):
    env = os.environ.copy()
    if extra_env:
        env.update(extra_env)

    process = subprocess.Popen(
        command,
        cwd=workdir,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        bufsize=1,
        universal_newlines=True,
    )

    assert process.stdout is not None
    for line in process.stdout:
        sys.stdout.write(line)
        sys.stdout.flush()

    process.wait()
    if process.returncode != 0:
        raise SystemExit(process.returncode)


def which(program):
    path = os.environ.get("PATH", "")
    extensions = [""] + os.environ.get("PATHEXT", ".EXE;.BAT;.CMD").split(os.pathsep)
    for directory in path.split(os.pathsep):
        for extension in extensions:
            candidate = os.path.join(directory, program + extension)
            if os.path.isfile(candidate):
                return candidate
    return None


def resolve_scons_command():
    for candidate in ("scons.bat", "scons"):
        found = which(candidate)
        if found:
            return [found]
    return [sys.executable, "-m", "SCons"]


def require_file(path):
    if not os.path.exists(path):
        raise IOError("Required file not found: " + path)
    return path


def secure_build_targets(build_dir):
    return [os.path.join(build_dir, artifact) for artifact in SECURE_BUILD_ARTIFACTS]


def write_signing_config(template_path, output_path, secure_hex_path):
    import json

    with open(template_path, "r") as config_in:
        config = json.load(config_in)

    secure_hex_path = os.path.abspath(secure_hex_path)
    for item in config.get("content", []):
        if not item.get("enabled", True):
            continue

        for command in item.get("commands", []):
            if command.get("command") != "sign":
                continue

            for input_item in command.get("inputs", []):
                input_item["file"] = secure_hex_path

            for output_item in command.get("outputs", []):
                output_item["file"] = secure_hex_path

    output_dir = os.path.dirname(output_path)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)

    with open(output_path, "w") as config_out:
        json.dump(config, config_out, indent=4)


def parse_args():
    parser = argparse.ArgumentParser(description="Build the PSE84 CM33 Secure firmware.")
    parser.add_argument("--project-root", help="NS project root used for project-local libs.")
    parser.add_argument("--bsp-root", help="Root directory containing libs/TARGET_APP_KIT_PSE84_EVAL_EPC2.")
    parser.add_argument("--libraries-root", help="Root directory containing libraries/components.")
    parser.add_argument("--build-dir", help="Directory used by the Secure SCons build.")
    parser.add_argument("--output-dir", help="Directory to copy the final Secure build outputs into.")
    return parser.parse_args()


def resolve_tools_root(project_dir):
    current = os.path.abspath(project_dir)
    start_dir = current
    while True:
        edgeprotecttools = os.path.join(current, "tools", "edgeprotecttools", "bin", "edgeprotecttools.exe")
        if os.path.exists(edgeprotecttools):
            return current

        parent = os.path.dirname(current)
        if parent == current:
            return start_dir

        current = parent


def add_python_scripts_to_path():
    python_dir = os.path.dirname(os.path.abspath(sys.executable))
    scripts_dir = os.path.join(python_dir, "Scripts")
    path = os.environ.get("PATH", "")
    os.environ["PATH"] = scripts_dir + os.pathsep + python_dir + os.pathsep + path


def main():
    args = parse_args()
    add_python_scripts_to_path()

    package_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.abspath(args.project_root) if args.project_root else os.path.abspath(os.path.join(package_dir, "..", "..", ".."))
    bsp_root = os.path.abspath(args.bsp_root) if args.bsp_root else project_dir
    libraries_root = os.path.abspath(args.libraries_root) if args.libraries_root else project_dir
    tools_root = resolve_tools_root(project_dir)
    output_dir = os.path.abspath(args.output_dir) if args.output_dir else os.path.join(package_dir, "build")
    secure_build_dir = os.path.abspath(args.build_dir) if args.build_dir else os.path.join(package_dir, "secure_project", "build")
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    if not os.path.exists(secure_build_dir):
        os.makedirs(secure_build_dir)

    secure_project_dir = require_file(os.path.join(package_dir, "secure_project"))
    secure_boot_config_template = require_file(os.path.join(secure_project_dir, "config", "boot_with_extended_boot_scons.json"))
    secure_boot_config = os.path.join(secure_build_dir, "boot_with_extended_boot_scons.generated.json")
    edgeprotecttools = require_file(os.path.join(tools_root, "tools", "edgeprotecttools", "bin", "edgeprotecttools.exe"))

    env = {
        "NS_PROJECT_ROOT": project_dir,
        "NS_BSP_ROOT": bsp_root,
        "NS_LIBRARIES_ROOT": libraries_root,
        "SECURE_BUILD_DIR": secure_build_dir,
    }
    rtt_exec_path = os.environ.get("RTT_EXEC_PATH", "")
    if rtt_exec_path:
        env["RTT_EXEC_PATH"] = rtt_exec_path

    # The signing step below updates build/rtthread.hex in place. Remove it
    # before invoking SCons so the unsigned HEX is regenerated from the ELF
    # instead of signing an already-signed image again on the next build.
    signed_in_place_hex = os.path.join(secure_build_dir, "rtthread.hex")
    if os.path.exists(signed_in_place_hex):
        os.remove(signed_in_place_hex)

    sconsign_file = os.path.join(secure_project_dir, ".sconsign.dblite")
    if os.path.exists(sconsign_file):
        os.remove(sconsign_file)

    scons_command = resolve_scons_command() + secure_build_targets(secure_build_dir)
    run_command(scons_command, secure_project_dir, env)

    write_signing_config(
        secure_boot_config_template,
        secure_boot_config,
        os.path.join(secure_build_dir, "rtthread.hex"),
    )

    run_command(
        [edgeprotecttools, "run-config", "-i", secure_boot_config],
        secure_project_dir,
        env,
    )

    expected_files = [
        os.path.join(secure_build_dir, "rtthread.elf"),
        os.path.join(secure_build_dir, "rtthread.hex"),
        os.path.join(secure_build_dir, "rtthread.bin"),
    ]

    for src_file in expected_files:
        require_file(src_file)
        shutil.copy2(src_file, os.path.join(output_dir, os.path.basename(src_file)))


if __name__ == "__main__":
    main()
