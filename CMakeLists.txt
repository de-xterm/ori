cmake_minimum_required(VERSION 3.21)
project(termwidth)

set(CMAKE_CXX_STANDARD 14)

add_executable(termwidth main.cpp)

target_link_libraries(termwidth tinfo)