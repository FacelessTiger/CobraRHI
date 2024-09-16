vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO FacelessTiger/CobraRHI
    REF "${VERSION}"
    SHA512 5fa445a3102ab8a21bc6ba996aeffb20fc6ed8aa87ed4d17170f60acf5b71bcadaa69414f44447999d791a242f518ad1606d12c3c7f04323bcf7c57775420b15
    HEAD_REF main
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