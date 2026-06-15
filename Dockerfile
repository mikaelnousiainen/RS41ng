FROM fedora:43

RUN dnf install -y  \
    gcc-c++ \
    arm-none-eabi-gcc-cs \
    arm-none-eabi-gcc-cs-c++ \
    arm-none-eabi-binutils-cs \
    arm-none-eabi-newlib \
    cmake \
    curl \
    unzip

# Install Bun (TypeScript runtime for config generator), pinned to match CI (deploy-web.yml)
RUN curl -fsSL https://bun.sh/install | bash -s "bun-v1.3.14"
ENV PATH="/root/.bun/bin:${PATH}"

# Run the build script from the mounted source tree (not a baked-in copy) so that
# edits to docker_build.sh take effect without rebuilding the image. The source is
# mounted at /usr/local/src/RS41ng by the build-firmware.sh / build-firmware.bat wrappers.
ENTRYPOINT ["/bin/bash", "/usr/local/src/RS41ng/docker_build.sh"]

# FROM debian:bookworm-slim

# RUN apt-get -y update && \
#     apt-get -y install \
#     build-essential cmake gcc-arm-none-eabi

# ENTRYPOINT ["/bin/bash", "/usr/local/src/RS41ng/docker_build.sh"]
