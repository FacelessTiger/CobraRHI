#pragma once

#include <CobraRHI.h>

#include <d3d12.h>
#include <dxgi.h>

#include <unordered_map>

namespace Cobra::Utils {

	//VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);

	//VkCompareOp CBCompareOpToVulkan(CompareOperation op);
	DXGI_FORMAT CBImageFormatToDirectX(ImageFormat format);
	//VkImageUsageFlags CBImageUsageToVulkan(ImageUsage usage);
	D3D12_BARRIER_LAYOUT  CBImageLayoutToDirectX(ImageLayout layout);
	D3D12_BARRIER_SYNC CBPipelineStageToDirectX(PipelineStage stage);
	//VkFilter CBFilterToVulkan(Filter filter);
	D3D12_BLEND CBBlendFactorToDirectX(BlendFactor blend);
	D3D12_BLEND_OP CBBlendOpToDirectX(BlendOp op);

	/*ImageFormat VulkanImageFormatToCB(VkFormat format);
	ImageUsage VulkanImageUsageToCB(VkImageUsageFlags usage);*/

}