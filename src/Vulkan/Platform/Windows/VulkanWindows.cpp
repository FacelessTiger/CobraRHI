#include "../../Mappings/VulkanRHI.h"

namespace Cobra::Platform {

	void AddPlatformExtensions(std::vector<const char*>& extensions)
	{
		extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	}

	VkSurfaceKHR CreateVulkanSurface(std::shared_ptr<Impl<GraphicsContext>> context, void* handle)
	{
		VkSurfaceKHR surface;
		VkCheck(context->Config, vkCreateWin32SurfaceKHR(context->Instance, PtrTo(VkWin32SurfaceCreateInfoKHR{
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance = GetModuleHandle(nullptr),
			.hwnd = (HWND)handle
		}), nullptr, &surface));

		return surface;
	}

}