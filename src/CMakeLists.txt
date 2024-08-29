function(glob_sources var dir)
    set(patterns
        "*.cpp"
        "*.h"
    )
    list(TRANSFORM patterns PREPEND "${dir}/")

    file(GLOB_RECURSE files CONFIGURE_DEPENDS ${patterns})

    if(NOT WIN32)
        list(FILTER files EXCLUDE REGEX "(^|/)Windows/.*")
    endif()

    if(NOT UNIX)
        list(FILTER files EXCLUDE REGEX "(^|/)Unix/.*")
    endif()

    if(NOT ANDROID)
        list(FILTER files EXCLUDE REGEX "(^|/)Android/.*")
    endif()

    list(APPEND ${var} ${files})
    set(${var} ${${var}} PARENT_SCOPE)
endfunction()

glob_sources(source Shared)
if (BACKEND STREQUAL VK)
    glob_sources(source Vulkan)
elseif(BACKEND STREQUAL DX)
    glob_sources(source DirectX)
endif()

add_library(CobraRHI ${source})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source})

target_include_directories(CobraRHI PRIVATE Shared)