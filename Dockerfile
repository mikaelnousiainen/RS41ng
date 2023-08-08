FROM fedora:36

RUN dnf install -y  \
    gcc-c++ \
    arm-none-eabi-gcc-cs \
    arm-none-eabi-gcc-cs-c++ \
    arm-none-eabi-binutils-cs \
    arm-none-eabi-newlib \
    cmake

COPY docker_build.sh /build.sh
RUN chmod +x /build.sh

ENTRYPOINT ["/bin/bash", "/build.sh"]
