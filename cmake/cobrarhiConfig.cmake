include(CMakeFindDependencyMacro)

find_dependency(xxHash CONFIG REQUIRED)

# TODO: find dependency depending on backend
find_dependency(VulkanHeaders CONFIG REQUIRED)
find_dependency(volk CONFIG REQUIRED)
find_dependency(VulkanMemoryAllocator CONFIG REQUIRED)

find_dependency(directx-headers CONFIG REQUIRED)
find_dependency(directx12-agility CONFIG REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/cobrarhiTargets.cmake")