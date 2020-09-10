project(RS41ng C CXX)

if (UNIX)
  set(TOOLCHAIN_PATH /usr/bin)
  set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/arm-none-eabi-gcc)
  set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PATH}/arm-none-eabi-g++)
  set(CMAKE_OBJCOPY ${TOOLCHAIN_PATH}/arm-none-eabi-objcopy)
else ()
  set(TOOLCHAIN_PATH C:\\)
  set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/arm-none-eabi-gcc.exe)
  set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PATH}/arm-none-eabi-g++.exe)
  set(CMAKE_OBJCOPY ${TOOLCHAIN_PATH}/arm-none-eabi-objcopy.exe)
endif ()

add_definitions(-DSTM32F100C8)
add_definitions(-DSTM32F10X_MD_VL)
add_definitions(-DUSE_STDPERIPH_DRIVER)
add_definitions(-DSUPPORT_CPLUSPLUS)
add_definitions(-D__ASSEMBLY__)

SET(LINKER_SCRIPT arm-gcc-link.ld)
SET(COMMON_FLAGS " -mcpu=cortex-m3 -mthumb -Wall -ffunction-sections -fdata-sections -g -O3 -nostartfiles")
SET(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -std=c++11")
SET(CMAKE_C_FLAGS "${COMMON_FLAGS} -std=gnu99")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map -lstdc++ -O3 -mcpu=cortex-m3 -mthumb -Wl,--gc-sections --specs=nano.specs -T ${LINKER_SCRIPT}")
# -u _printf_float
#SET(CMAKE_EXE_LINKER_FLAGS "-Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map -lstdc++ -O3 -mcpu=cortex-m3 -mthumb -Wl,--gc-sections --specs=rdimon.specs -lc -lrdimon -T ${LINKER_SCRIPT}")

file(GLOB_RECURSE USER_SOURCES "*.c")
file(GLOB_RECURSE USER_SOURCES_CXX "*.cpp")
file(GLOB_RECURSE USER_HEADERS "*.h")

include_directories(hal/cmsis
    hal/cmsis_boot
    hal/stm_lib/inc
    ..)

add_executable(${PROJECT_NAME}.elf ${USER_SOURCES} ${USER_SOURCES_CXX} ${USER_HEADERS} ${HAL_SOURCES} ${LINKER_SCRIPT})

set(HEX_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin)
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
    COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}"
    COMMAND ${TOOLCHAIN_PATH}/arm-none-eabi-size ${PROJECT_NAME}.elf)

set(CMAKE_CXX_STANDARD 11)

add_custom_target(program
    DEPENDS ${PROJECT_NAME}.elf

    if (UNIX)
    #TODO
    else ()
    COMMAND C:/Programs/stlink-1.3.0-win64/bin/st-flash --reset write ${BIN_FILE} 0x08000000
    endif ()
    COMMENT "flashing ${BIN_FILE}")
