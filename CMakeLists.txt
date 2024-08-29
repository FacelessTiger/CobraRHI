cmake_minimum_required(VERSION 3.25)
project(CobraRHI)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(BACKEND VK CACHE STRING "Select which graphics API backend to use")

set(AVAILABLE_BACKENDS DX VK)
if (NOT BACKEND IN_LIST AVAILABLE_BACKENDS)
	message(FATAL_ERROR "Uknown backend ${BACKEND}, must be one of ${AVAILABLE_BACKENDS}")
endif()

add_subdirectory(src)
add_subdirectory(deps)

include(CTest)
add_subdirectory(tests)

target_include_directories(CobraRHI PUBLIC include)
target_compile_definitions(CobraRHI PRIVATE COBRARHI_INCLUDE_DIR="${CMAKE_CURRENT_LIST_DIR}/include")