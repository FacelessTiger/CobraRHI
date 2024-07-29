#include "VulkanRHI.h"
#include <vulkan/utility/vk_format_utils.h>

namespace Cobra {

	Image::Image(GraphicsContext& context, const uVec2& size, ImageFormat format, ImageUsage usage)
	{
		pimpl = std::make_unique<Impl<Image>>(context, size, format, usage);
	}

	Image::Image(std::shared_ptr<Impl<GraphicsContext>> context)
	{
		pimpl = std::make_unique<Impl<Image>>(context);
	}

	Image::~Image() { }
	Image::Image(Image&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Image& Image::operator=(Image&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void Image::SetDebugName(std::string_view name)
	{
		if (pimpl->Context->Config.Debug)
		{
			VkCheck(pimpl->Context->Config, vkSetDebugUtilsObjectNameEXT(pimpl->Context->Device, PtrTo(VkDebugUtilsObjectNameInfoEXT{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_IMAGE,
				.objectHandle = (uint64_t)pimpl->Allocation.Image,
				.pObjectName = name.data()
			})));
		}
	}

	void Image::Set(const void* data) const
	{
		if (g_HostImageSupported)
		{
			if (pimpl->Layout != VK_IMAGE_LAYOUT_GENERAL)
			{
				vkTransitionImageLayoutEXT(pimpl->Context->Device, 1, PtrTo(VkHostImageLayoutTransitionInfoEXT{
					.sType = VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO_EXT,
					.image = pimpl->Allocation.Image,
					.oldLayout = pimpl->Layout,
					.newLayout = VK_IMAGE_LAYOUT_GENERAL,
					.subresourceRange = Utils::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT)
				}));
			}

			vkCopyMemoryToImageEXT(pimpl->Context->Device, PtrTo(VkCopyMemoryToImageInfoEXT {
				.sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO_EXT,
				.dstImage = pimpl->Allocation.Image,
				.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL,
				.regionCount = 1,
				.pRegions = PtrTo(VkMemoryToImageCopyEXT {
					.sType = VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY_EXT,
					.pHostPointer = data,
					.imageSubresource = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel = 0,
						.baseArrayLayer = 0,
						.layerCount = 1
					},
					.imageOffset = { 0, 0, 0 },
					.imageExtent = { pimpl->Size.x, pimpl->Size.y, 1 }
				})
			}));

			pimpl->Layout = VK_IMAGE_LAYOUT_GENERAL;
			pimpl->Stage = VK_PIPELINE_STAGE_2_HOST_BIT;
		}
		else
		{
			pimpl->Context->TransferManager->Transfer(*this, { (std::byte*)data, pimpl->Size.x * pimpl->Size.y * (uint32_t)vkuFormatTexelSize(Utils::CBImageFormatToVulkan(pimpl->Format)) });
		}
	}

	void Image::Transition(ImageLayout layout) const
	{
		VkImageLayout vulkanLayout = Utils::CBImageLayoutToVulkan(layout);
		if (vulkanLayout == pimpl->Layout) return;

		if (g_HostImageSupported)
		{
			vkTransitionImageLayoutEXT(pimpl->Context->Device, 1, PtrTo(VkHostImageLayoutTransitionInfoEXT {
				.sType = VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO_EXT,
				.image = pimpl->Allocation.Image,
				.oldLayout = pimpl->Layout,
				.newLayout = vulkanLayout,
				.subresourceRange = Utils::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT)
			}));

			pimpl->Layout = vulkanLayout;
			pimpl->Stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		}
		else
		{
			auto& queue = pimpl->Context->TransferManager->GetQueue();

			auto cmd = queue.Begin();
			pimpl->TransitionLayout(cmd.pimpl->CommandBuffer, vulkanLayout, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
			queue.Submit(cmd, {}).Wait();
		}
	}

	uint32_t Image::GetHandle() const { return pimpl->Handle.GetPendingValue(); }

	Impl<Image>::Impl(std::shared_ptr<Impl<GraphicsContext>> context)
		: Context(context), ExternalAllocation(true)
	{ }

	Impl<Image>::Impl(GraphicsContext& context, const uVec2& size, ImageFormat format, ImageUsage usage)
		: Context(context.pimpl), Size(size), Format(format), Usage(usage)
	{
		VkFormat vulkanFormat = Utils::CBImageFormatToVulkan(Format);
		vmaCreateImage(Context->Allocator, 
			PtrTo(VkImageCreateInfo {
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.imageType = VK_IMAGE_TYPE_2D, // TODO: specify somehow

				.format = vulkanFormat,
				.extent = { Size.x, Size.y , 1 },

				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,

				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = Utils::CBImageUsageToVulkan(Usage)
			}), 
			PtrTo(VmaAllocationCreateInfo {
				.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
				.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			}),
			&Allocation.Image, &Allocation.Allocation, nullptr
		);

		VkCheck(Context->Config, vkCreateImageView(Context->Device, PtrTo(VkImageViewCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = Allocation.Image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D, // TODO: specify, will be the same as image type above
			.format = vulkanFormat,
			.subresourceRange = {
				.aspectMask = (VkImageAspectFlags)((Usage & ImageUsage::DepthStencilAttachment) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT),
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			}
		}), nullptr, &View));

		UpdateDescriptor();
	}

	Impl<Image>::~Impl()
	{
		Context->DeletionQueues->Push(Context->GraphicsQueue.pimpl->Fence.GetPendingValue(), View);
		if (!ExternalAllocation)
			Context->DeletionQueues->Push(Context->GraphicsQueue.pimpl->Fence.GetPendingValue(), Allocation);
	}

	void Impl<Image>::TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout, VkPipelineStageFlags2 newStage)
	{
		if (Layout == newLayout) return;
		VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

		VkAccessFlags2 dstMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
		switch (newLayout)
		{
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: dstMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT; break;
			case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL: dstMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; break;
			case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: dstMask = VK_ACCESS_2_NONE; break;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: dstMask = VK_ACCESS_2_TRANSFER_READ_BIT; break;
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: dstMask = VK_ACCESS_2_TRANSFER_WRITE_BIT; break;
		}

		vkCmdPipelineBarrier2(cmd, PtrTo(VkDependencyInfo {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = PtrTo(VkImageMemoryBarrier2 {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = Stage,
				.srcAccessMask = Mask,
				.dstStageMask = newStage,
				.dstAccessMask = dstMask,

				.oldLayout = Layout,
				.newLayout = newLayout,

				.image = Allocation.Image,
				.subresourceRange = Utils::ImageSubresourceRange(aspectMask)
			})
		}));

		Layout = newLayout;
		Stage = newStage;
		Mask = dstMask;
	}

	void Impl<Image>::UpdateDescriptor()
	{
		if (Usage & ImageUsage::Storage)
		{
			vkUpdateDescriptorSets(Context->Device, 1, PtrTo(VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = Context->BindlessSet,
				.dstBinding = STORAGE_IMAGE_BINDING,
				.dstArrayElement = Handle.GetPendingValue(),
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.pImageInfo = PtrTo(VkDescriptorImageInfo {
					.imageView = View,
					.imageLayout = VK_IMAGE_LAYOUT_GENERAL
				})
			}), 0, nullptr);
		}

		if (Usage & ImageUsage::Sampled)
		{
			vkUpdateDescriptorSets(Context->Device, 1, PtrTo(VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = Context->BindlessSet,
				.dstBinding = SAMPLED_IMAGE_BINDING,
				.dstArrayElement = Handle.GetPendingValue(),
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				.pImageInfo = PtrTo(VkDescriptorImageInfo {
					.imageView = View,
					.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				})
			}), 0, nullptr);
		}
	}

}