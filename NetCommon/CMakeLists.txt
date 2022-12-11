cmake_minimum_required(VERSION 3.0.0)
project(NetCommon VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

file (GLOB SOURCE_FILES include/*.cpp)
file (GLOB HEADER_FILES include/*.h)
add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC "Z:/Program Files (x86)/asio-1.24.0/include")