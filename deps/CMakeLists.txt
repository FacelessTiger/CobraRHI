target_include_directories(CobraRHI PRIVATE slang/include xxHash)
target_sources(CobraRHI PRIVATE xxHash/xxhash.c)

if (WIN32)
    target_compile_definitions(CobraRHI PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(CobraRHI PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/slang/lib/windows/slang.lib")
endif()

if (BACKEND STREQUAL VK)
    if (WIN32)
        target_compile_definitions(CobraRHI PRIVATE VK_USE_PLATFORM_WIN32_KHR)
    elseif (ANDROID)
        target_compile_definitions(CobraRHI PRIVATE VK_USE_PLATFORM_ANDROID_KHR)
    endif()

    set(VOLK_PULL_IN_VULKAN OFF)
    set(VOLK_HEADERS_ONLY ON)
    add_subdirectory(volk)
    add_subdirectory(vma)
    add_subdirectory(vulkan-headers)

    target_compile_definitions(CobraRHI PRIVATE VK_NO_PROTOTYPES)

    target_link_libraries(CobraRHI PRIVATE
        volk_headers
        GPUOpen::VulkanMemoryAllocator
        Vulkan::Headers
    )
endif()

if (BACKEND STREQUAL DX)
    target_include_directories(CobraRHI PRIVATE agility/include)

    add_subdirectory(d3d12ma)

    target_link_libraries(CobraRHI PRIVATE
        D3D12MemoryAllocator
        dxcompiler.lib
    )
endif()