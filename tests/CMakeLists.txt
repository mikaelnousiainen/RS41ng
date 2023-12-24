cmake_minimum_required(VERSION 3.6)

#SET(CMAKE_SYSTEM_NAME Linux)
#SET(CMAKE_SYSTEM_VERSION 1)

project(RS41ng_test C CXX)

set(BINARY ${CMAKE_PROJECT_NAME})

SET(CMAKE_C_FLAGS "${COMMON_FLAGS} -std=gnu99")

file(GLOB_RECURSE USER_SOURCES "../src/config.c" "../src/codecs/*.c" "../src/template.c" "../src/utils.c" "../src/strlcpy.c")
file(GLOB_RECURSE USER_SOURCES_CXX "../src/codecs/*.cpp")
file(GLOB_RECURSE USER_HEADERS "../src/codecs/*.h" "../src/template.h" "../src/utils.h" "../src/config.h" "../src/strlcpy.h")

file(GLOB_RECURSE TEST_SOURCES "*.c")
file(GLOB_RECURSE TEST_SOURCES_CXX "*.cpp")
file(GLOB_RECURSE TEST_HEADERS "*.h")

set(SOURCES ${TEST_SOURCES})

add_executable(${BINARY} ${TEST_SOURCES} ${USER_SOURCES})

add_test(NAME ${BINARY} COMMAND ${BINARY})
