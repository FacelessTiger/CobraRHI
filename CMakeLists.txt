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
    $<INSTALL_INTERFACE:include>  # for client in install mode
)
target_compile_definitions(CobraRHI PRIVATE COBRARHI_INCLUDE_DIR="${CMAKE_CURRENT_LIST_DIR}/include")

find_library(SLANG_PATH slang REQUIRED)
find_package(xxHash CONFIG REQUIRED)
target_link_libraries(CobraRHI PRIVATE ${SLANG_PATH} xxHash::xxhash)

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

if (COBRARHI_BACKEND STREQUAL DX)
    find_package(directx-headers CONFIG REQUIRED)
    find_package(directx12-agility CONFIG REQUIRED)
    target_link_libraries(CobraRHI PRIVATE Microsoft::DirectX-Headers Microsoft::DirectX-Guids Microsoft::DirectX12-Agility)

    if(TARGET Microsoft::DirectX12-Agility)
        file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/D3D12")
        add_custom_command(TARGET CobraRHI POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Core,IMPORTED_LOCATION_RELEASE> $<TARGET_FILE_DIR:main>/D3D12
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG> $<TARGET_FILE_DIR:main>/D3D12
           COMMAND_EXPAND_LISTS
        )
    endif()
endif()

if (COBRARHI_ENABLE_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif()

install(TARGETS CobraRHI
    EXPORT CobraRHI-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(EXPORT CobraRHI-targets
    FILE cobrarhiTargets.cmake
    NAMESPACE CobraRHI::
    DESTINATION lib/cmake/CobraRHI
)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ TYPE INCLUDE)
install(FILES "cmake/cobrarhiConfig.cmake" DESTINATION lib/cmake/CobraRHI)