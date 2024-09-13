cmake_minimum_required(VERSION 3.25)

if (COBRARHI_ENABLE_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

project(CobraRHI)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(AVAILABLE_BACKENDS DX VK)
if (NOT COBRARHI_BACKEND IN_LIST AVAILABLE_BACKENDS)
	message(FATAL_ERROR "Unknown backend ${BACKEND}, must be one of ${AVAILABLE_BACKENDS}")
endif()

add_subdirectory(src)

target_include_directories(CobraRHI PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>   # for headers when building
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>  # for client in install mode
)
target_compile_definitions(CobraRHI PRIVATE COBRARHI_INCLUDE_DIR="${CMAKE_CURRENT_LIST_DIR}/include")

find_library(SLANG_PATH slang REQUIRED)
find_package(xxHash CONFIG REQUIRED)
target_link_libraries(CobraRHI PRIVATE SLANG_PATH xxHash::xxhash)

if (WIN32)
    target_compile_definitions(CobraRHI PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

if (COBRARHI_BACKEND STREQUAL VK)
    if (WIN32)
        target_compile_definitions(CobraRHI PRIVATE VK_USE_PLATFORM_WIN32_KHR)
    elseif (ANDROID)
        target_compile_definitions(CobraRHI PRIVATE VK_USE_PLATFORM_ANDROID_KHR)
    endif()

    find_package(VulkanHeaders CONFIG REQUIRED)
    find_package(volk CONFIG REQUIRED)
    find_package(VulkanMemoryAllocator CONFIG REQUIRED)
    target_link_libraries(CobraRHI PRIVATE Vulkan::Headers volk::volk_headers GPUOpen::VulkanMemoryAllocator)
endif()

if (COBRARHI_ENABLE_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif()

install(TARGETS CobraRHI EXPORT CobraRHI-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
install(EXPORT CobraRHI-targets
    NAMESPACE CobraRHI::
    DESTINATION ${CMAKE_INSTALL_DATADIR}/CobraRHI
)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ TYPE INCLUDE)