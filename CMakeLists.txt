cmake_minimum_required(VERSION 3.13)
project(RS41ng C CXX ASM)

include_directories(src)
link_directories(src)
add_subdirectory(src)

# add_subdirectory(tests)