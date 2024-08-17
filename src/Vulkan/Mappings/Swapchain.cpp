#include "VulkanRHI.h"

#include <algorithm>

namespace Cobra {

	Swapchain::Swapchain(GraphicsContext& context, void* window, uVec2 size, bool enableVsync)
	{
		pimpl = std::make_unique<Impl<Swapchain>>(context, window, size, enableVsync);
	}

	Swapchain::~Swapchain() { }
	Swapchain::Swapchain(Swapchain&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Swapchain& Swapchain::operator=(Swapchain&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void Swapchain::Resize(uVec2 newSize)
	{
		if (pimpl->Size == newSize) return;
		pimpl->Size = newSize;

		for (auto& image : pimpl->Images)
			image.pimpl->Size = newSize;

		pimpl->Dirty = true;
	}

	Image& Swapchain::GetCurrent()
	{
		return pimpl->Images[pimpl->ImageIndex];
	}

	uVec2 Swapchain::GetSize() const
	{
		return pimpl->Size;
	}

	Impl<Swapchain>::Impl(GraphicsContext& context, void* window, uVec2 size, bool enableVsync)
		: Context(context.pimpl), Size(size), EnableVsync(enableVsync)
	{
		Surface = Platform::CreateVulkanSurface(Context, window);
		CreateSwapchain(true);

		VkSemaphoreCreateInfo semaphoreInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr
		};

		Semaphores.resize(Images.size() * 2);
		for (int i = 0; i < Semaphores.size(); i++)
			VK_CHECK(vkCreateSemaphore(Context->Device, &semaphoreInfo, nullptr, &Semaphores[i]), "Failed to create semaphore for swapchain with index " + std::to_string(i));
	}

	Impl<Swapchain>::~Impl()
	{
		// TODO: Push to deletion queue
		vkDestroySwapchainKHR(Context->Device, Swapchain, nullptr);
		vkDestroySurfaceKHR(Context->Instance, Surface, nullptr);

		for (const auto& semaphore : Semaphores)
			Context->DeletionQueues->Push(Context->GraphicsQueue.pimpl->Fence.GetPendingValue(), semaphore);
	}

	void Impl<Swapchain>::Recreate()
	{
		CreateSwapchain(false);
		Dirty = false;
	}

	void Impl<Swapchain>::CreateSwapchain(bool firstCreation)
	{
		if (firstCreation)
		{
			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(Context->ChosenGPU, Surface, &formatCount, nullptr);
			std::vector<VkSurfaceFormatKHR> formats(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(Context->ChosenGPU, Surface, &formatCount, formats.data());

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(Context->ChosenGPU, Surface, &presentModeCount, nullptr);
			std::vector<VkPresentModeKHR> presentModes(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(Context->ChosenGPU, Surface, &presentModeCount, presentModes.data());

			ChosenSurfaceFormat = ChooseSurfaceFormat(formats);
			VsyncOnPresent = VK_PRESENT_MODE_FIFO_KHR; // support for this is required
			VsyncOffPresent = ChooseVsyncOffPresent(presentModes);
		}

		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Context->ChosenGPU, Surface, &capabilities);

		VkExtent2D extent = ChooseSwapExtent(capabilities);
		Size.x = extent.width;
		Size.y = extent.height;

		uint32_t imageCount = capabilities.minImageCount + 1;
		imageCount = (capabilities.maxImageCount && imageCount > capabilities.maxImageCount) ? capabilities.maxImageCount : imageCount;

		VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		VkSwapchainCreateInfoKHR createInfo = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = Surface,
			.minImageCount = imageCount,
			.imageFormat = ChosenSurfaceFormat.format,
			.imageColorSpace = ChosenSurfaceFormat.colorSpace,
			.imageExtent = extent,
			.imageArrayLayers = 1,
			.imageUsage = usageFlags,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.preTransform = capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = EnableVsync ? VsyncOnPresent : VsyncOffPresent,
			.clipped = VK_TRUE
		};

		VkSwapchainKHR oldSwapchain = Swapchain;
		if (!firstCreation) createInfo.oldSwapchain = oldSwapchain;
		VK_CHECK(vkCreateSwapchainKHR(Context->Device, &createInfo, nullptr, &Swapchain), "Failed to create swapchain");
		if (!firstCreation) vkDestroySwapchainKHR(Context->Device, oldSwapchain, nullptr);

		vkGetSwapchainImagesKHR(Context->Device, Swapchain, &imageCount, nullptr);
		std::vector<VkImage> vulkanImages(imageCount);
		vkGetSwapchainImagesKHR(Context->Device, Swapchain, &imageCount, vulkanImages.data());

		ImageFormat = ChosenSurfaceFormat.format;
		Images.clear();

		for (uint32_t i = 0; i < vulkanImages.size(); i++)
		{
			VkImageSubresourceRange range = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			};
			VkImageViewCreateInfo imageViewInfo = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = vulkanImages[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = ImageFormat,
				.subresourceRange = range
			};

			VkImageView view;
			VK_CHECK(vkCreateImageView(Context->Device, &imageViewInfo, nullptr, &view), "Failed to create image view for swapchain image with index " + std::to_string(i));

			Image image(Context);
			image.pimpl->Allocation.Image = vulkanImages[i];
			image.pimpl->View = view;
			image.pimpl->Layout = VK_IMAGE_LAYOUT_UNDEFINED;
			image.pimpl->Format = Utils::VulkanImageFormatToCB(ImageFormat);
			image.pimpl->Usage = Utils::VulkanImageUsageToCB(usageFlags);
			image.pimpl->Size = Size;
			Images.push_back(std::move(image));
		}
	}

	VkSurfaceFormatKHR Impl<Swapchain>::ChooseSurfaceFormat(std::span<VkSurfaceFormatKHR> availableFormats)
	{
		for (const auto& format : availableFormats)
		{
			if (format.format == VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return format;
		}

		return availableFormats[0];
	}

	VkPresentModeKHR Impl<Swapchain>::ChooseVsyncOffPresent(std::span<VkPresentModeKHR> presentModes)
	{
		// return immediately if mailbox, otherwise prefer immediate and if not available default to fifo
		VkPresentModeKHR ret = VK_PRESENT_MODE_FIFO_KHR;
		for (const auto& present : presentModes)
		{
			if (present == VK_PRESENT_MODE_MAILBOX_KHR) return present;
			if (present == VK_PRESENT_MODE_IMMEDIATE_KHR) ret = present;
		}

		return ret;
	}

	VkExtent2D Impl<Swapchain>::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) return capabilities.currentExtent;

		VkExtent2D actualExtent = { Size.x, Size.y };
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}

}