vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO FacelessTiger/CobraRHI
    REF "${VERSION}"
    SHA512 32366ba9e8b5f5103f0df3051dc12844d05b6f50a1dfce2d26843f7f18e89dc2a0d57fcb385453e15b6139d026c0695a8ae942fc7c5d5254073b6a1b6e14c347
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS -DCOBRARHI_BACKEND=VK
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME "cobrahri")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")