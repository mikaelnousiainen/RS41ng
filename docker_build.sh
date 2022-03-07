# Script to compile RS41ng on any platform
# This script is copied into the docker image when it is built, if you plan to change this script, please make sure to rebuild the docker image
set -e # Exit if failed

# Sanity check
if [ ! -d "/usr/src/rs41ng" ]
then
    echo "Build directory does not exist, please run the command given in README to mount the source directory"
    exit 1
fi

# Build RS41ng
cd /tmp/
mkdir rs41ng_build && cd rs41ng_build
cmake /usr/src/rs41ng
make -j$(nproc)
# Copy the binary, ELF and hex to the home directory
cp src/RS41ng.elf /usr/src/rs41ng
cp RS41ng.hex /usr/src/rs41ng
cp RS41ng.bin /usr/src/rs41ng

echo "RS41ng build complete"
