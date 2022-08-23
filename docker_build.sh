#!/bin/bash

# Script to compile RS41ng on any platform using Docker
# This script is copied into the Docker image when it is built.
# If you plan to change this script, please make sure to rebuild the Docker image.

set -e # Exit if failed

SOURCE_PATH="/usr/local/src/RS41ng"

# Sanity check
if [ ! -d "${SOURCE_PATH}" ]
then
    echo "Source directory does not exist, please run the Docker command given in README to mount the source directory"
    exit 1
fi

# Build RS41ng
cd "${SOURCE_PATH}"
rm -rf build
mkdir -p build
cd build
cmake ..
make -j$(nproc)

echo "RS41ng build complete"
