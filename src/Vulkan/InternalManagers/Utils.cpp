#include "Utils.h"
#include "../Mappings/VulkanRHI.h"

#include <assert.h>

namespace Cobra::Utils {

	VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask)
	{
		return VkImageSubresourceRange {
			.aspectMask = aspectMask,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};
	}

	VkCompareOp CBCompareOpToVulkan(CompareOperation op)
	{
		switch (op)
		{
			case CompareOperation::Never:			return VK_COMPARE_OP_NEVER;
			case CompareOperation::Greater:			return VK_COMPARE_OP_GREATER;
			case CompareOperation::GreaterEqual:	return VK_COMPARE_OP_GREATER_OR_EQUAL;
			case CompareOperation::LesserEqual:		return VK_COMPARE_OP_LESS_OR_EQUAL;
			default:								std::unreachable();
		}
	}

	VkFormat CBImageFormatToVulkan(ImageFormat format)
	{
		switch (format)
		{
			case ImageFormat::Unknown:				return VK_FORMAT_UNDEFINED;
			case ImageFormat::R32_SINT:				return VK_FORMAT_R32_SINT;
			case ImageFormat::R16G16B16A16_SFLOAT:	return VK_FORMAT_R16G16B16A16_SFLOAT;
			case ImageFormat::R16G16B16A16_UNORM:	return VK_FORMAT_R16G16B16A16_UNORM;
			case ImageFormat::R8G8B8A8_UNORM:		return VK_FORMAT_R8G8B8A8_UNORM;
			case ImageFormat::B8G8R8A8_SRGB:		return VK_FORMAT_B8G8R8A8_SRGB;
			case ImageFormat::D32_SFLOAT:			return VK_FORMAT_D32_SFLOAT;
			default:								std::unreachable();
		}
	}

	VkImageUsageFlags CBImageUsageToVulkan(ImageUsage usage)
	{
		VkImageUsageFlags ret = 0;
		if (g_HostImageSupported) ret |= VK_IMAGE_USAGE_HOST_TRANSFER_BIT_EXT;

		if (usage & ImageUsage::ColorAttachment)		ret |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (usage & ImageUsage::DepthStencilAttachment)	ret |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (usage & ImageUsage::TransferSrc)			ret |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (usage & ImageUsage::TransferDst)			ret |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if (usage & ImageUsage::Storage)				ret |= VK_IMAGE_USAGE_STORAGE_BIT;
		if (usage & ImageUsage::Sampled)				ret |= VK_IMAGE_USAGE_SAMPLED_BIT;

		return ret;
	}

	VkImageLayout CBImageLayoutToVulkan(ImageLayout layout)
	{
		switch (layout)
		{
			case ImageLayout::General:			return VK_IMAGE_LAYOUT_GENERAL;
			case ImageLayout::ReadOnlyOptimal:	return VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
			default:							std::unreachable();
		}
	}

	VkPipelineStageFlags2 CBPipelineStageToVulkan(PipelineStage stage)
	{
		VkPipelineStageFlags2 ret = 0;
		
		if (stage & PipelineStage::Compute)		ret |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		if (stage & PipelineStage::Transfer)	ret |= VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
		if (stage & PipelineStage::Graphics)	ret |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

		return ret;
	}

	VkFilter CBFilterToVulkan(Filter filter)
	{
		switch (filter)
		{
			case Filter::Nearest:	return VK_FILTER_NEAREST;
			case Filter::Linear:	return VK_FILTER_LINEAR;
			default:				std::unreachable();
		}
	}

	VkBlendFactor CBBlendFactorToVulkan(BlendFactor blend)
	{
		switch (blend)
		{
			case BlendFactor::Zero:				return VK_BLEND_FACTOR_ZERO;
			case BlendFactor::One:				return VK_BLEND_FACTOR_ONE;
			case BlendFactor::SrcAlpha:			return VK_BLEND_FACTOR_SRC_ALPHA;
			case BlendFactor::DstAlpha:			return VK_BLEND_FACTOR_DST_ALPHA;
			case BlendFactor::OneMinusSrcAlpha:	return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			default:							std::unreachable();
		}
	}

	VkBlendOp CBBlendOpToVulkan(BlendOp op)
	{
		switch (op)
		{
			case BlendOp::Add:	return VK_BLEND_OP_ADD;
			default:			std::unreachable();
		}
	}

	ImageFormat VulkanImageFormatToCB(VkFormat format)
	{
		switch (format)
		{
			case VK_FORMAT_UNDEFINED:			return ImageFormat::Unknown;
			case VK_FORMAT_R16G16B16A16_SFLOAT:	return ImageFormat::R16G16B16A16_SFLOAT;
			case VK_FORMAT_R8G8B8A8_UNORM:		return ImageFormat::R8G8B8A8_UNORM;
			case VK_FORMAT_B8G8R8A8_SRGB:		return ImageFormat::B8G8R8A8_SRGB;
			case VK_FORMAT_D32_SFLOAT:			return ImageFormat::D32_SFLOAT;
			default:							std::unreachable();
		}
	}

	ImageUsage VulkanImageUsageToCB(VkImageUsageFlags usage)
	{
		ImageUsage ret = ImageUsage::None;

		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)			ret |= ImageUsage::ColorAttachment;
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)	ret |= ImageUsage::DepthStencilAttachment;
		if (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)				ret |= ImageUsage::TransferSrc;
		if (usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)				ret |= ImageUsage::TransferDst;
		if (usage & VK_IMAGE_USAGE_STORAGE_BIT)						ret |= ImageUsage::Storage;
		if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)						ret |= ImageUsage::Sampled;

		return ret;
	}

}