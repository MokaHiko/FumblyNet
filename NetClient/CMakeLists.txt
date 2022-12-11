cmake_minimum_required(VERSION 3.0.0)
project(NetClient VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE x64)
add_executable(${PROJECT_NAME} src/SimpleClient.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE 
	../NetCommon/include)

target_link_libraries(${PROJECT_NAME} PRIVATE NetCommon)