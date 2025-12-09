#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Build static libraries for opus and edge-impulse SDK
This script extracts object files from the build directory and creates static libraries.
Run this after a successful full build.

Usage:
    python build_libs.py

The script will:
1. Find all .o files for opus and edge-impulse in the build directory
2. Create static libraries using arm-none-eabi-ar
3. Copy libraries to packages/opus-1.4/lib/ and packages/edge-impulse/lib/
"""

import os
import subprocess
import glob
import shutil

# Configuration
PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
BUILD_DIR = os.path.join(PROJECT_DIR, 'build')
TOOLCHAIN_PATH = os.environ.get('RTT_EXEC_PATH', r'D:\RT-ThreadStudio\repo\Extract\ToolChain_Support_Packages\ARM\GNU_Tools_for_ARM_Embedded_Processors\13.3\bin')

AR = os.path.join(TOOLCHAIN_PATH, 'arm-none-eabi-ar')
if os.name == 'nt':
    AR += '.exe'

def find_object_files(base_dir, pattern):
    """Find all .o files matching the pattern"""
    obj_files = []
    for root, dirs, files in os.walk(base_dir):
        for f in files:
            if f.endswith('.o'):
                full_path = os.path.join(root, f)
                if pattern in full_path.replace('\\', '/'):
                    obj_files.append(full_path)
    return obj_files

def create_static_library(obj_files, output_lib, ar_path):
    """Create a static library from object files"""
    if not obj_files:
        print(f"  No object files found for {output_lib}")
        return False
    
    # Ensure output directory exists
    os.makedirs(os.path.dirname(output_lib), exist_ok=True)
    
    # Remove existing library
    if os.path.exists(output_lib):
        os.remove(output_lib)
    
    print(f"  Creating {os.path.basename(output_lib)} with {len(obj_files)} object files...")
    
    # Build library incrementally to avoid command line length limits
    batch_size = 50
    for i in range(0, len(obj_files), batch_size):
        batch = obj_files[i:i+batch_size]
        cmd = [ar_path, 'rcs', output_lib] + batch
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, cwd=PROJECT_DIR)
            if result.returncode != 0:
                print(f"  Error: {result.stderr}")
                return False
        except Exception as e:
            print(f"  Exception: {e}")
            return False
    
    # Get library size
    lib_size = os.path.getsize(output_lib)
    print(f"  Created: {output_lib} ({lib_size / 1024 / 1024:.2f} MB)")
    return True

def build_opus_lib():
    """Build libopus.a"""
    print("\n=== Building libopus.a ===")
    
    # Find opus object files - they should be in build/packages/opus-1.4/
    opus_obj_files = []
    opus_patterns = [
        os.path.join(BUILD_DIR, 'packages', 'opus-1.4'),
    ]
    
    for pattern_dir in opus_patterns:
        if os.path.exists(pattern_dir):
            for root, dirs, files in os.walk(pattern_dir):
                for f in files:
                    if f.endswith('.o'):
                        opus_obj_files.append(os.path.join(root, f))
    
    print(f"  Found {len(opus_obj_files)} opus object files")
    
    output_lib = os.path.join(PROJECT_DIR, 'packages', 'opus-1.4', 'lib', 'libopus.a')
    return create_static_library(opus_obj_files, output_lib, AR)

def build_edge_impulse_lib():
    """Build libedge_impulse.a"""
    print("\n=== Building libedge_impulse.a ===")
    
    ei_build_dir = os.path.join(BUILD_DIR, 'packages', 'edge-impulse')
    
    # Find all edge-impulse SDK object files (exclude application-specific files)
    sdk_obj_files = []
    
    if os.path.exists(ei_build_dir):
        for root, dirs, files in os.walk(ei_build_dir):
            for f in files:
                if f.endswith('.o'):
                    full_path = os.path.join(root, f)
                    # Only include SDK files (edge-impulse-sdk directory)
                    if 'edge-impulse-sdk' in full_path:
                        sdk_obj_files.append(full_path)
    
    print(f"  Found {len(sdk_obj_files)} edge-impulse SDK object files")
    
    output_lib = os.path.join(PROJECT_DIR, 'packages', 'edge-impulse', 'lib', 'libedge_impulse.a')
    return create_static_library(sdk_obj_files, output_lib, AR)

def main():
    print("=" * 60)
    print("Static Library Builder for Opus and Edge Impulse")
    print("=" * 60)
    
    # Check if ar exists
    if not os.path.exists(AR):
        print(f"Error: arm-none-eabi-ar not found at {AR}")
        print("Please set RTT_EXEC_PATH environment variable to your toolchain bin directory")
        return 1
    
    print(f"Using AR: {AR}")
    print(f"Build directory: {BUILD_DIR}")
    
    # Check if build directory exists
    if not os.path.exists(BUILD_DIR):
        print(f"\nError: Build directory not found: {BUILD_DIR}")
        print("Please run a full build first: scons -j8")
        return 1
    
    success = True
    
    # Build libraries
    if not build_opus_lib():
        success = False
    
    if not build_edge_impulse_lib():
        success = False
    
    print("\n" + "=" * 60)
    if success:
        print("All libraries built successfully!")
        print("\nNext steps:")
        print("1. The SConscript files will automatically detect and use these libraries")
        print("2. Run 'scons -c' to clean, then 'scons -j8' to rebuild with prebuilt libs")
        print("3. To rebuild from source, delete the lib/ directories")
    else:
        print("Some libraries failed to build.")
        print("Make sure you have run a full build first: scons -j8")
    print("=" * 60)
    
    return 0 if success else 1

if __name__ == '__main__':
    exit(main())
