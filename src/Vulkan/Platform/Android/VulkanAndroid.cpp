#include "../../Mappings/VulkanRHI.h"

namespace Cobra::Platform {

	void AddPlatformExtensions(std::vector<const char*>& extensions)
	{
		extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
	}

	VkSurfaceKHR CreateVulkanSurface(std::shared_ptr<Impl<GraphicsContext>> context, void* handle)
	{
		VkSurfaceKHR surface;
		VkCheck(context->Config, vkCreateAndroidSurfaceKHR(context->Instance, PtrTo(VkAndroidSurfaceCreateInfoKHR{
			.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
			.window = (ANativeWindow*)handle
		}), nullptr, &surface));

		return surface;
	}

}