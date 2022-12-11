cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE x64)
project(NetServer VERSION 0.1.0)

add_executable(${PROJECT_NAME} src/Server.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE 
	../NetCommon/include)

target_link_libraries(${PROJECT_NAME} PRIVATE NetCommon)