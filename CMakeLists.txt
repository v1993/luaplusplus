cmake_minimum_required(VERSION 3.13)
project(lua++-test CXX)
#set(CMAKE_CXX_STANDARD 17)

add_subdirectory(library)

add_executable(${PROJECT_NAME} src/main.cpp)
target_compile_features(lua++-test PUBLIC cxx_std_17)
target_link_libraries(${PROJECT_NAME} lua++_static)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
