#!/bin/bash

# Build the RS41ng firmware using the Docker build environment.
#
# Usage:
#   ./build-firmware.sh [--vaisala-cal [SERIAL]] [config-file.yaml] [extra cmake flags...]
#
# Examples:
#   ./build-firmware.sh                     # use config.yaml (default)
#   ./build-firmware.sh my-tracker.yaml     # use a different config file
#   ./build-firmware.sh -DRS41=1            # no config file; pass target flag for the built-in config
#   ./build-firmware.sh my-tracker.yaml -DRS41=1
#   ./build-firmware.sh --vaisala-cal V1221335 -DRS41=1
#
# --vaisala-cal fetches the sonde's factory PTU calibration from SondeHub (by the
# serial printed on the sticker; prompts for it when omitted) and generates
# src/vaisala_boom_cal.h before building -- see tools/fetch_vaisala_boom_cal.py.
# Use together with SENSOR_VAISALA_BOOM_ENABLE and SENSOR_VAISALA_BOOM_CAL_MODE 2.
#
# The first argument, if it does not start with "-", selects the configuration
# YAML file (relative to this script's directory). When that file exists, the
# hardware target is derived from its hardware.type field, so no -D flag is needed.
# Any remaining arguments are forwarded to cmake (e.g. a -DRS41=1 target flag when
# building without a config file). Requires the rs41ng_compiler Docker image
# (build it once with: docker build -t rs41ng_compiler .).

set -e

# Run from the repository root (where this script lives) so the volume mount and
# the config file path are resolved consistently regardless of the caller's cwd.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Optional: fetch the sonde's factory calibration from SondeHub before building.
if [ "$1" = "--vaisala-cal" ]; then
    shift
    CAL_SERIAL=""
    if [ -n "$1" ] && [ "${1#-}" = "$1" ] && [ "${1%.yaml}" = "$1" ]; then
        CAL_SERIAL="$1"
        shift
    fi
    PYTHON=python3
    command -v python3 >/dev/null 2>&1 || PYTHON=python
    "$PYTHON" tools/fetch_vaisala_boom_cal.py $CAL_SERIAL || exit 1
fi

CONFIG_FILE="config.yaml"
CONFIG_EXPLICIT=0
if [ -n "$1" ] && [ "${1#-}" = "$1" ]; then
    CONFIG_FILE="$1"
    CONFIG_EXPLICIT=1
    shift
fi

if ! docker image inspect rs41ng_compiler >/dev/null 2>&1; then
    echo "ERROR: Docker image 'rs41ng_compiler' not found." >&2
    echo "Build it once with: docker build -t rs41ng_compiler ." >&2
    exit 1
fi

if [ ! -f "$CONFIG_FILE" ]; then
    if [ "$CONFIG_EXPLICIT" = "1" ]; then
        echo "ERROR: config file '$CONFIG_FILE' not found in $SCRIPT_DIR" >&2
        exit 1
    fi
    echo "No '$CONFIG_FILE' found - building with the built-in manual configuration (config.h / config.c)."
    echo "You must pass a target flag, e.g.: ./build-firmware.sh -DRS41=1"
else
    echo "Using configuration file: $CONFIG_FILE"
fi

docker run --rm -it \
    -v "$(pwd)":/usr/local/src/RS41ng \
    -e CONFIG_FILE="$CONFIG_FILE" \
    rs41ng_compiler "$@"
