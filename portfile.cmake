vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO FacelessTiger/CobraRHI
    REF "${VERSION}"
    SHA512 fe465801ee91f46871df1a38f2a9e1d08798679914432f71f7fb48594b8bfc05e58936509ec4f91da03f1fed4914db567d27070cb41e84f4902163844ea788fd
    HEAD_REF main
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
    vulkan-backend WITH_VULKAN
)

if (WITH_VULKAN)
    set(COBRARHI_DEFINES "-DCOBRARHI_BACKEND=VK")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS ${COBRARHI_DEFINES}
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/${PORT})

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")