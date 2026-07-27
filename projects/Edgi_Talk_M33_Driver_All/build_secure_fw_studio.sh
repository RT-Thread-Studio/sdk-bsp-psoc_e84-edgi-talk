#!/bin/sh

set -eu

die()
{
    echo "ERROR: $*" >&2
    exit 2
}

require_file()
{
    [ -f "$1" ] || die "Required file not found: $1"
}

require_dir()
{
    [ -d "$1" ] || die "Required directory not found: $1"
}

rtt_build_tool=${1:-}

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_root=$(cd "$script_dir" && (pwd -W 2>/dev/null || pwd))

secure_root="$project_root/libraries/components/infineon-pse84-secure-firmware-latest"
secure_script="$secure_root/build_secure_fw.py"
secure_output="$secure_root/build"

path_to_posix()
{
    printf '%s' "$1" | tr '\\' '/'
}

dirname_posix()
{
    value=$(path_to_posix "$1")
    value=${value%/}
    echo "${value%/*}"
}

is_usable_python_dir()
{
    [ -f "$1/python.exe" ] && [ -f "$1/Scripts/scons.bat" ]
}

find_python_dir_from_env()
{
    for candidate in "${PYTHONHOME:-}" "${PYTHONPATH:-}"; do
        [ "$candidate" != "" ] || continue
        candidate=$(path_to_posix "$candidate")
        candidate=${candidate%;}
        is_usable_python_dir "$candidate" && {
            echo "$candidate"
            return 0
        }
    done

    if [ "${SCONS:-}" != "" ]; then
        candidate=$(dirname_posix "$SCONS")
        is_usable_python_dir "$candidate" && {
            echo "$candidate"
            return 0
        }
    fi

    return 1
}

find_python_dir_from_tools()
{
    tools_root=$(path_to_posix "$1")

    for candidate in "$tools_root"/*; do
        [ -d "$candidate" ] || continue
        is_usable_python_dir "$candidate" && {
            echo "$candidate"
            return 0
        }
    done

    return 1
}

find_studio_env_tools_from_build_tool()
{
    [ "$rtt_build_tool" != "" ] || return 1

    build_tool=$(path_to_posix "$rtt_build_tool")
    if [ -f "$build_tool" ]; then
        build_tool=$(dirname_posix "$build_tool")
    fi

    case "$build_tool" in
        */platform/env_released/env/tools/BuildTools/*|*/platform/env_released/env/tools/buildtools/*)
            echo "${build_tool%%/platform/env_released/env/tools/[Bb]uild[Tt]ools/*}/platform/env_released/env/tools"
            return 0
            ;;
    esac

    if find_python_dir_from_tools "$build_tool" >/dev/null 2>&1; then
        echo "$build_tool"
        return 0
    fi

    return 1
}

find_studio_env_tools()
{
    find_studio_env_tools_from_build_tool && return 0

    sh_path=$(command -v sh 2>/dev/null || true)
    sh_path=$(path_to_posix "$sh_path")
    case "$sh_path" in
        */platform/env_released/env/tools/BuildTools/*|*/platform/env_released/env/tools/buildtools/*)
            studio_root=${sh_path%%/platform/env_released/env/tools/[Bb]uild[Tt]ools/*}
            echo "$studio_root/platform/env_released/env/tools"
            return 0
            ;;
    esac

    path_slashes=$(path_to_posix "$PATH")
    case "$path_slashes" in
        */platform/env_released/env/tools/BuildTools/*|*/platform/env_released/env/tools/buildtools/*)
            studio_root=${path_slashes%%/platform/env_released/env/tools/[Bb]uild[Tt]ools/*}
            studio_root=${studio_root##*:}
            echo "$studio_root/platform/env_released/env/tools"
            return 0
            ;;
    esac

    return 1
}

echo "==> Building CM33 Secure firmware for RT-Thread Studio"
echo "Project root: $project_root"

require_dir "$secure_root"
require_file "$secure_script"

studio_env_tools=$(find_studio_env_tools || true)
if [ "$studio_env_tools" != "" ]; then
    python_dir=$(find_python_dir_from_env || find_python_dir_from_tools "$studio_env_tools" || true)
    [ "$python_dir" != "" ] || die "Python with SCons was not found under Studio env/tools: $studio_env_tools"

    python_exe="$python_dir/python.exe"
    scons_bat="$python_dir/Scripts/scons.bat"

    require_file "$python_exe"
    require_file "$scons_bat"

    export PATH="$python_dir/Scripts:$python_dir:$PATH"
else
    python_exe=$(command -v python 2>/dev/null || true)
    [ "$python_exe" != "" ] || die "RT-Thread Studio env/tools was not found from PATH, and python was not found either."
    echo "WARNING: RT-Thread Studio env/tools was not found; using python from PATH: $python_exe" >&2
fi

export NS_PROJECT_ROOT="$project_root"
export NS_BSP_ROOT="$project_root"
export NS_LIBRARIES_ROOT="$project_root"
export SCONSFLAGS="${SCONSFLAGS:-} --warn=no-visual-c-missing"

"$python_exe" "$secure_script" \
    --project-root "$project_root" \
    --bsp-root "$project_root" \
    --libraries-root "$project_root" \
    --output-dir "$secure_output"

require_file "$secure_output/rtthread.elf"
require_file "$secure_output/rtthread.hex"
require_file "$secure_output/rtthread.bin"

echo "==> CM33 Secure firmware is ready: $secure_output/rtthread.hex"
