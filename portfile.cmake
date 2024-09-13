vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO FacelessTiger/CobraRHI
    REF "${VERSION}"
    SHA512 8edc575b5fe1220c4fc7826cdd08a8bcadc95d6df26b4beecf6d5a9ffc14b9e34cc5745a3aa7783cd6b3dde9ba4c6636a47cba398f85306109958f0314a95de1
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS -DCOBRARHI_BACKEND=VK
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME "cobrahri")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")