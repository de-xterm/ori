cmake_minimum_required(VERSION 3.21)
project(ori)

set(CMAKE_CXX_STANDARD 14)

add_executable(ori main.cpp)

if(UNIX)
    target_link_libraries(ori tinfo)
endif()