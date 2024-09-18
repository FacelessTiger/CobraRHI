vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/FacelessTiger/CobraRHI
    REF c3a27b2083f611d9f8c8b227a2162cdbed7fa1ba
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