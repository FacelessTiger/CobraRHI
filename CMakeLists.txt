cmake_minimum_required(VERSION 3.25)
project(CobraRHI)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(BACKEND "VK" CACHE STRING "Select which graphics API backend to use")

add_subdirectory(src)
add_subdirectory(deps)

target_include_directories(CobraRHI PUBLIC include)
target_compile_definitions(CobraRHI PRIVATE COBRARHI_INCLUDE_DIR="${CMAKE_CURRENT_LIST_DIR}/include")