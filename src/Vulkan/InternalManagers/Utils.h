#pragma once

#include <CobraRHI.h>

#include <volk.h>
#include <unordered_map>

namespace Cobra::Utils {

	struct FeatureBuilder
	{
		std::unordered_map<VkStructureType, VkBaseInStructure*> deviceFeatures;
		std::vector<const char*> extensions;
		VkBaseInStructure* pNext = nullptr;

		~FeatureBuilder()
		{
			for (auto& [type, feature] : deviceFeatures)
				free(feature);
		}

		void AddExtension(const char* name)
		{
			extensions.push_back(name);
		}

		template<class T>
		void AddFeature(T&& feature)
		{
			auto& f = deviceFeatures[feature.sType];
			f = (VkBaseInStructure*)malloc(sizeof(T));
			*(T*)f = std::move(feature);
			f->pNext = pNext;

			pNext = f;
		}

		const void* GetChain() { return pNext; }
	};

	VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);

	VkCompareOp CBCompareOpToVulkan(CompareOperation op);
	VkFormat CBImageFormatToVulkan(ImageFormat format);
	VkImageUsageFlags CBImageUsageToVulkan(ImageUsage usage);
	VkImageLayout  CBImageLayoutToVulkan(ImageLayout layout);
	VkPipelineStageFlags2 CBPipelineStageToVulkan(PipelineStage stage);
	VkFilter CBFilterToVulkan(Filter filter);
	VkBlendFactor CBBlendFactorToVulkan(BlendFactor blend);
	VkBlendOp CBBlendOpToVulkan(BlendOp op);

	ImageFormat VulkanImageFormatToCB(VkFormat format);
	ImageUsage VulkanImageUsageToCB(VkImageUsageFlags usage);

}