#!/bin/bash

# Script to compile RS41ng on any platform using Docker.
# This script runs inside the container from the mounted source tree (the Dockerfile
# ENTRYPOINT points here), so edits take effect without rebuilding the image.

set -e # Exit if failed

SOURCE_PATH="/usr/local/src/RS41ng"

# Configuration file to build from, relative to the source directory. Defaults to
# config.yaml; the build-firmware.sh / build-firmware.bat wrappers override it via
# the CONFIG_FILE environment variable.
CONFIG_FILE="${CONFIG_FILE:-config.yaml}"

# Sanity check
if [ ! -d "${SOURCE_PATH}" ]
then
    echo "Source directory does not exist, please run the Docker command given in README to mount the source directory"
    exit 1
fi

# Generate config_generated.h / config_generated.c from the config file if present
if [ -f "${SOURCE_PATH}/${CONFIG_FILE}" ]; then
    echo "${CONFIG_FILE} found - generating src/config_generated.h and src/config_generated.c..."
    cd "${SOURCE_PATH}"
    bun install --silent
    bun run scripts/generate_config.ts "${CONFIG_FILE}"
    echo "Config generation complete"
fi

# Determine hardware type from the config file for CMake flags
CMAKE_EXTRA_FLAGS=""
if [ -f "${SOURCE_PATH}/${CONFIG_FILE}" ]; then
    # Extract hardware type from YAML (simple grep, avoids needing a YAML parser)
    HW_TYPE=$(grep -A1 '^hardware:' "${SOURCE_PATH}/${CONFIG_FILE}" | grep 'type:' | sed 's/.*type:\s*//' | tr -d '[:space:]"'"'")
    if [ -n "${HW_TYPE}" ]; then
        echo "Using hardware type from ${CONFIG_FILE}: ${HW_TYPE}"
        case "${HW_TYPE}" in
            RS41)
                CMAKE_EXTRA_FLAGS="-DRS41=1"
                ;;
            RS41_RSM4x4)
                CMAKE_EXTRA_FLAGS="-DRS41_RSM4X4=1"
                ;;
            DFM17)
                CMAKE_EXTRA_FLAGS="-DDFM17=1"
                ;;
            *)
                echo "WARNING: Unknown hardware type '${HW_TYPE}', defaulting to RS41"
                CMAKE_EXTRA_FLAGS="-DRS41=1"
                ;;
        esac
    fi
fi

echo "Building with RS41ng CMake flags: ${CMAKE_EXTRA_FLAGS}"

# Build RS41ng
cd "${SOURCE_PATH}"
rm -rf build
mkdir -p build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-gcc.cmake ${CMAKE_EXTRA_FLAGS} .. $@
make -j$(nproc)

# Remove ephemeral generated config so the working tree stays clean (re-generated next build)
if [ -f "${SOURCE_PATH}/${CONFIG_FILE}" ]; then
    rm -f "${SOURCE_PATH}/src/config_generated.c" "${SOURCE_PATH}/src/config_generated.h"
fi

echo "RS41ng build complete"
