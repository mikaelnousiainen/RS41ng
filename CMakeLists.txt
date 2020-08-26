cmake_minimum_required(VERSION 3.6)

SET(CMAKE_SYSTEM_NAME "Generic")
SET(CMAKE_SYSTEM_VERSION 1)

SET(UNIX 1)

include_directories(src)
link_directories(src)
add_subdirectory(src)

add_subdirectory(tests)
