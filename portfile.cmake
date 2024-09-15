vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO FacelessTiger/CobraRHI
    REF "${VERSION}"
    SHA512 cb58f04bce512febc6db930571dff746f1bd435f43aaba41406c8416278405491c3cd10a6f859d5a6aa0ddabd4c1f2fb47ca0c4064a82d64833fdf6a2abc5171
    HEAD_REF main
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
    vulkan-backend -DCOBRARHI_BACKEND=VK
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS ${FEATURE_OPTIONS}
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/${PORT})

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")