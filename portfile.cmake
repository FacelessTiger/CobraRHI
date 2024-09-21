vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

execute_process(
    COMMAND git log -1 --format=%H
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/FacelessTiger/CobraRHI
    REF ${GIT_HASH}
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
    vulkan-backend WITH_VULKAN
    directx-backend WITH_DIRECTX
)

if (WITH_VULKAN)
    set(COBRARHI_DEFINES "-DCOBRARHI_BACKEND=VK")
elseif (WITH_DIRECTX)
    set(COBRARHI_DEFINES "-DCOBRARHI_BACKEND=DX")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS ${COBRARHI_DEFINES}
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/${PORT})

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")