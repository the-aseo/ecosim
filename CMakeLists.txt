cmake_minimum_required(VERSION 3.25)

project(ecosim LANGUAGES CXX)

find_package(raylib REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES source/*.cpp)

add_executable(ecosim ${SOURCES})

target_include_directories(ecosim PRIVATE include)

target_link_libraries(ecosim PRIVATE raylib)
