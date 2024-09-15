include(CMakeFindDependencyMacro)

find_dependency(xxHash CONFIG REQUIRED)

if (COBRARHI_BACKEND STREQUAL VK)
	find_dependency(VulkanHeaders CONFIG REQUIRED)
	find_dependency(volk CONFIG REQUIRED)
	find_dependency(VulkanMemoryAllocator CONFIG REQUIRED)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/cobrarhiTargets.cmake")