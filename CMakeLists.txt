cmake_minimum_required(VERSION 3.0.0)
project(FumblyNet VERSION 0.1.0)

set(CMAKE_BUILD_TYPE x64)
set(CMAKE_CXX_STANDARD 17)
add_subdirectory(./NetCommon)
add_subdirectory(./NetClient)
add_subdirectory(./NetServer)
