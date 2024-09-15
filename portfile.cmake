vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO FacelessTiger/CobraRHI
    REF "${VERSION}"
    SHA512 65e01d45da37f19537a5dc0c111738b0047fb5c57e3520ba97809988693f87cbd9f0031425ad27df40b4fcfeea8298c5cc86de63a2d4676afb992aa7a472fd28
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS -DCOBRARHI_BACKEND=VK
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/${PORT})

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")