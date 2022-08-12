cmake_minimum_required(VERSION 3.20)

project(EDMCIgnoreMouse)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED 18)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FindPython3)

find_package(Python3 COMPONENTS Interpreter Development)

Python3_add_library(edmcignoremouse MODULE src/main.c)
target_compile_definitions(edmcignoremouse PRIVATE UNICODE NOMINMAX WIN32_LEAN_AND_MEAN PY_SSIZE_T_CLEAN)
target_link_libraries(edmcignoremouse PUBLIC kernel32 user32 comctl32)
