vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO FacelessTiger/CobraRHI
    REF "${VERSION}"
    SHA512 04aa282fdfa898e462ae7754e7ec74dfcda5920460bce5b884474b2e866c1591dc607dc55d179f64a74a53800cc1e011613df0d6b36d7ecb67b2f5e1fc76a595
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS -DCOBRARHI_BACKEND=VK
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME "cobrahri")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")