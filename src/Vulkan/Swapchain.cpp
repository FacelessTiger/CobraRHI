#include "VulkanRHI.h"

#include <Core/Window.h>
#include <algorithm>

namespace Cobra {

	Swapchain::Swapchain(GraphicsContext& context, const Window& window, bool enableVsync)
	{
		pimpl = std::make_unique<Impl<Swapchain>>(context, window, enableVsync);
	}

	Swapchain::~Swapchain() { }
	Swapchain::Swapchain(Swapchain&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Swapchain& Swapchain::operator=(Swapchain&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void Swapchain::Resize(uint32_t width, uint32_t height)
	{
		pimpl->Width = width;
		pimpl->Height = height;

		for (auto& image : pimpl->Images)
		{
			image.pimpl->Width = width;
			image.pimpl->Height = height;
		}

		pimpl->Dirty = true;
	}

	Image& Swapchain::GetCurrent()
	{
		return pimpl->Images[pimpl->ImageIndex];
	}

	Impl<Swapchain>::Impl(GraphicsContext& context, const Window& window, bool enableVsync)
		: Context(context.pimpl), Width(window.GetWidth()), Height(window.GetHeight()), EnableVsync(enableVsync)
	{
		CreateSurface(window);
		CreateSwapchain(true);

		VkSemaphoreCreateInfo semaphoreInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr
		};

		Semaphores.resize(Images.size());
		for (int i = 0; i < Images.size(); i++)
			VkCheck(Context->Callback, vkCreateSemaphore(Context->Device, &semaphoreInfo, nullptr, &Semaphores[i]));
	}

	Impl<Swapchain>::~Impl()
	{
		// TODO: Push to deletion queue
		vkDestroySwapchainKHR(Context->Device, Swapchain, nullptr);
		vkDestroySurfaceKHR(Context->Instance, Surface, nullptr);

		for (auto& semaphore : Semaphores)
			vkDestroySemaphore(Context->Device, semaphore, nullptr);
	}

	void Impl<Swapchain>::Recreate()
	{
		CreateSwapchain(false);
		Dirty = false;
	}

	void Impl<Swapchain>::CreateSurface(const Window& window)
	{
		VkWin32SurfaceCreateInfoKHR info = {
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance = GetModuleHandle(nullptr),
			.hwnd = (HWND)window.GetNativeWindow()
		};
		VkCheck(Context->Callback, vkCreateWin32SurfaceKHR(Context->Instance, &info, nullptr, &Surface));
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
		Width = extent.width;
		Height = extent.height;

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
		VkCheck(Context->Callback, vkCreateSwapchainKHR(Context->Device, &createInfo, nullptr, &Swapchain));
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
			VkCheck(Context->Callback, vkCreateImageView(Context->Device, &imageViewInfo, nullptr, &view));

			Image image(Context);
			image.pimpl->Image = vulkanImages[i];
			image.pimpl->View = view;
			image.pimpl->Layout = VK_IMAGE_LAYOUT_UNDEFINED;
			image.pimpl->Format = ImageFormat;
			image.pimpl->UsageFlags = usageFlags;
			image.pimpl->Width = Width;
			image.pimpl->Height = Height;
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

		VkExtent2D actualExtent = { Width, Height };
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}

}