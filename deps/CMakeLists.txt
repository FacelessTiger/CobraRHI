target_include_directories(CobraRHI PRIVATE slang/include xxHash)
target_link_libraries(CobraRHI PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/slang/lib/slang.lib")
target_sources(CobraRHI PRIVATE xxHash/xxhash.c)

if (WIN32)
    target_compile_definitions(CobraRHI PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

if (BACKEND STREQUAL "VK")
    if (WIN32)
        set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)  
    endif()

    add_subdirectory(volk)
    add_subdirectory(vma)
    add_subdirectory(spirv-cross)

    target_compile_definitions(CobraRHI PRIVATE VK_NO_PROTOTYPES)

    include(FindVulkan)
    target_link_libraries(CobraRHI PRIVATE
        volk
        GPUOpen::VulkanMemoryAllocator
        spirv-cross-c
    )
endif()