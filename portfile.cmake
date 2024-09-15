vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO FacelessTiger/CobraRHI
    REF "${VERSION}"
    SHA512 49aba38e5218df8d629db44b64aefdfe63b03d241c159291f17cbe174f079101c77ce900818168e32f3a2fefaea026ec5d4101624db992168ebabba5dcfb77d2
    HEAD_REF main
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
    vulkan-backend COBRARHI_BACKEND=VK
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS ${FEATURE_OPTIONS}
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/${PORT})

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")