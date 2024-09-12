vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO FacelessTiger/CobraRHI
    REF "${VERSION}"
    SHA512 9154b0158c6cc5b962f8620888b8965397d1f7d866d80ba62d9be85d4a465a71b30de668d45f5e61fc88a4c2026d58b501cba6b76c8573e70ac77dfc13c992e7
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME "cobrahri")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")