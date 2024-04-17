#include "VulkanRHI.h"

namespace Cobra {

	Image::Image(std::shared_ptr<Impl<GraphicsContext>> context)
	{
		pimpl = std::make_unique<Impl<Image>>(context);
	}

	Image::~Image() { }
	Image::Image(Image&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Image& Image::operator=(Image&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	Impl<Image>::Impl(std::shared_ptr<Impl<GraphicsContext>> context)
		: Context(context)
	{ }

	Impl<Image>::~Impl()
	{
		// TODO: Push to deletion queue
	}

	void Impl<Image>::TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout, VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask)
	{
		if (Layout == newLayout) return;
		VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

		VkImageSubresourceRange range = {
			.aspectMask = aspectMask,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

		VkImageMemoryBarrier2 imageBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = srcStageMask,
			.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
			.dstStageMask = dstStageMask,
			.dstAccessMask = dstAccessMask,

			.oldLayout = Layout,
			.newLayout = newLayout,

			.image = Image,
			.subresourceRange = range,
		};

		VkDependencyInfo info = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &imageBarrier
		};
		vkCmdPipelineBarrier2(cmd, &info);
	}

}