#include "Utils.h"
#include "../Mappings/DirectXRHI.h"

#include <assert.h>

namespace Cobra::Utils {

	/*VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask)
	{
		return VkImageSubresourceRange {
			.aspectMask = aspectMask,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};
	}*/

	/*VkCompareOp CBCompareOpToVulkan(CompareOperation op)
	{
		switch (op)
		{
			case CompareOperation::Never:			return VK_COMPARE_OP_NEVER;
			case CompareOperation::Greater:			return VK_COMPARE_OP_GREATER;
			case CompareOperation::GreaterEqual:	return VK_COMPARE_OP_GREATER_OR_EQUAL;
			case CompareOperation::LesserEqual:		return VK_COMPARE_OP_LESS_OR_EQUAL;
			default:								std::unreachable();
		}
	}*/

	DXGI_FORMAT CBImageFormatToDirectX(ImageFormat format)
	{
		switch (format)
		{
			case ImageFormat::Unknown:				return DXGI_FORMAT_UNKNOWN;
			case ImageFormat::R32_SINT:				return DXGI_FORMAT_R32_SINT;
			case ImageFormat::R16G16B16A16_SFLOAT:	return DXGI_FORMAT_R16G16B16A16_FLOAT;
			case ImageFormat::R16G16B16A16_UNORM:	return DXGI_FORMAT_R16G16B16A16_UNORM;
			case ImageFormat::R8G8B8A8_UNORM:		return DXGI_FORMAT_R8G8B8A8_UNORM;
			case ImageFormat::B8G8R8A8_SRGB:		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
			case ImageFormat::D32_SFLOAT:			return DXGI_FORMAT_D32_FLOAT;
			default:								std::unreachable();
		}
	}

	/*VkImageUsageFlags CBImageUsageToVulkan(ImageUsage usage)
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
	}*/

	D3D12_BARRIER_LAYOUT CBImageLayoutToDirectX(ImageLayout layout)
	{
		switch (layout)
		{
			case ImageLayout::General:			return D3D12_BARRIER_LAYOUT_COMMON;
			case ImageLayout::ReadOnlyOptimal:	return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
			default:							std::unreachable();
		}
	}

	D3D12_BARRIER_SYNC CBPipelineStageToDirectX(PipelineStage stage)
	{
		D3D12_BARRIER_SYNC ret = D3D12_BARRIER_SYNC_NONE;
		
		if (stage & PipelineStage::Compute)		ret |= D3D12_BARRIER_SYNC_COMPUTE_SHADING;
		if (stage & PipelineStage::Transfer)	ret |= D3D12_BARRIER_SYNC_COPY;
		if (stage & PipelineStage::Graphics)	ret |= D3D12_BARRIER_SYNC_DRAW;

		return ret;
	}

	/*VkFilter CBFilterToVulkan(Filter filter)
	{
		switch (filter)
		{
			case Filter::Nearest:	return VK_FILTER_NEAREST;
			case Filter::Linear:	return VK_FILTER_LINEAR;
			default:				std::unreachable();
		}
	}*/

	D3D12_BLEND CBBlendFactorToDirectX(BlendFactor blend)
	{
		switch (blend)
		{
			case BlendFactor::Zero:				return D3D12_BLEND_ZERO;
			case BlendFactor::One:				return D3D12_BLEND_ONE;
			case BlendFactor::SrcAlpha:			return D3D12_BLEND_SRC_ALPHA;
			case BlendFactor::DstAlpha:			return D3D12_BLEND_DEST_ALPHA;
			case BlendFactor::OneMinusSrcAlpha:	return D3D12_BLEND_INV_SRC_ALPHA;
			default:							std::unreachable();
		}
	}

	D3D12_BLEND_OP CBBlendOpToDirectX(BlendOp op)
	{
		switch (op)
		{
			case BlendOp::Add:	return D3D12_BLEND_OP_ADD;
			default:			std::unreachable();
		}
	}

	/*ImageFormat VulkanImageFormatToCB(VkFormat format)
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
	}*/

}