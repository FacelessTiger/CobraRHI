#include "VulkanPipelines.h"
#include "../Mappings/VulkanRHI.h"

#include <unordered_map>
#include <format>

namespace Cobra {

	inline constexpr auto DYNAMIC_STATES = std::to_array<VkDynamicState>({
		VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT, VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT,
		VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE, VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE, VK_DYNAMIC_STATE_DEPTH_COMPARE_OP
	});

    VkPipeline GetGraphicsPipeline(Impl<CommandList>* cmd, const GraphicsPipelineKey& key)
    {
		auto context = cmd->Context;
		auto pipeline = context->GraphicsPipelines[key];
		if (pipeline) return pipeline;

		auto start = std::chrono::steady_clock::now();

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		shaderStages.reserve(key.Shaders.size());
		for (uint64_t id : key.Shaders)
		{
			if (id)
			{
				const auto& shader = Impl<Shader>::GetShaderByID(id);
				shaderStages.push_back(shader.StageInfo);
			}
		}

		std::vector<VkFormat> colorAttachmentFormats(key.ColorAttachmentCount);
		std::vector<VkPipelineColorBlendAttachmentState> colorAttachmentBlends(key.ColorAttachmentCount);

		for (uint32_t i = 0; i < key.ColorAttachmentCount; i++)
		{
			colorAttachmentFormats[i] = Utils::CBImageFormatToVulkan(key.ColorAttachments[i]);
			colorAttachmentBlends[i] = {
				.blendEnable = key.BlendEnable,
				.srcColorBlendFactor = Utils::CBBlendFactorToVulkan(key.SrcBlend),
				.dstColorBlendFactor = Utils::CBBlendFactorToVulkan(key.DstBlend),
				.colorBlendOp = Utils::CBBlendOpToVulkan(key.BlendOp),
				.srcAlphaBlendFactor = Utils::CBBlendFactorToVulkan(key.SrcBlendAlpha),
				.dstAlphaBlendFactor = Utils::CBBlendFactorToVulkan(key.DstBlendAlpha),
				.alphaBlendOp = Utils::CBBlendOpToVulkan(key.BlendAlpha),
				.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
			};
		}

		vkCreateGraphicsPipelines(context->Device, context->PipelineCache, 1, PtrTo(VkGraphicsPipelineCreateInfo {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = PtrTo(VkPipelineRenderingCreateInfo {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.colorAttachmentCount = (uint32_t)colorAttachmentFormats.size(),
				.pColorAttachmentFormats = colorAttachmentFormats.data(),
				.depthAttachmentFormat = Utils::CBImageFormatToVulkan(key.DepthAttachment)
			}),
			.stageCount = (uint32_t)shaderStages.size(),
			.pStages = shaderStages.data(),
			.pVertexInputState = PtrTo(VkPipelineVertexInputStateCreateInfo {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
			}),
			.pInputAssemblyState = PtrTo(VkPipelineInputAssemblyStateCreateInfo {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			}),
			.pViewportState = PtrTo(VkPipelineViewportStateCreateInfo {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
			}),
			.pRasterizationState = PtrTo(VkPipelineRasterizationStateCreateInfo {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.polygonMode = VK_POLYGON_MODE_FILL,
				.cullMode = VK_CULL_MODE_NONE,
				.frontFace = VK_FRONT_FACE_CLOCKWISE,
				.depthBiasEnable = false,
				.lineWidth = 1.0f
			}),
			.pMultisampleState = PtrTo(VkPipelineMultisampleStateCreateInfo {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
				.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
				.minSampleShading = 1.0f,
				.pSampleMask = nullptr,
				.alphaToCoverageEnable = VK_FALSE,
				.alphaToOneEnable = VK_FALSE
			}),
			.pDepthStencilState = PtrTo(VkPipelineDepthStencilStateCreateInfo {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.depthBoundsTestEnable = false,
				.stencilTestEnable = false
			}),
			.pColorBlendState = PtrTo(VkPipelineColorBlendStateCreateInfo {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
				.attachmentCount = (uint32_t)colorAttachmentBlends.size(),
				.pAttachments = colorAttachmentBlends.data()
			}),
			.pDynamicState = PtrTo(VkPipelineDynamicStateCreateInfo {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.dynamicStateCount = (uint32_t)DYNAMIC_STATES.size(),
				.pDynamicStates = DYNAMIC_STATES.data()
			}),
			.layout = context->BindlessPipelineLayout
		}), nullptr, &pipeline);
		context->GraphicsPipelines[key] = pipeline;

		// TODO: For some reason std::format isn't working on android despite specificing c++20, will investiage in the future
#ifndef __ANDROID__
		std::chrono::duration<float> duration = std::chrono::steady_clock::now() - start;
		if (context->Config.Trace)
			context->Config.Callback(std::format("Compiled graphics pipeline in {} microseconds", std::chrono::duration_cast<std::chrono::microseconds>(duration).count()).c_str(), MessageSeverity::Trace);
#endif

		return pipeline;
    }

	VkPipeline GetComputePipeline(Impl<CommandList>* cmd, const ComputePipelineKey& key)
	{
		auto context = cmd->Context;
		auto pipeline = context->ComputePipelines[key];
		if (pipeline) return pipeline;

		auto& shader = Impl<Shader>::GetShaderByID(key.Shader);
		vkCreateComputePipelines(context->Device, context->PipelineCache, 1, PtrTo(VkComputePipelineCreateInfo{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = shader.StageInfo,
			.layout = context->BindlessPipelineLayout
		}), nullptr, &pipeline);
		context->ComputePipelines[key] = pipeline;

		return pipeline;
	}

}