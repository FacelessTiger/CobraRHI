#include "VulkanRHI.h"

namespace Cobra {

	CommandList::CommandList() { }
	CommandList::~CommandList() { }
	CommandList::CommandList(CommandList&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	CommandList& CommandList::operator=(CommandList&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void CommandList::Present(Swapchain& swapchain)
	{
		swapchain.GetCurrent().pimpl->TransitionLayout(pimpl->CommandBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_NONE, 0);
	}

	void CommandList::BeginRendering(const iVec2& region, const Image& colorAttachment)
	{
		colorAttachment.pimpl->TransitionLayout(pimpl->CommandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_NONE, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
		VkRenderingAttachmentInfo vulkanColorAttachment = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = colorAttachment.pimpl->View,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE
		};

		VkRenderingInfo renderInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = {
				.extent = { (uint32_t)region.x, (uint32_t)region.y }
			},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &vulkanColorAttachment
		};
		vkCmdBeginRendering(pimpl->CommandBuffer, &renderInfo);
	}

	void CommandList::EndRendering()
	{
		vkCmdEndRendering(pimpl->CommandBuffer);
	}

	void CommandList::BindShaders(Span<Shader> shaders)
	{
		std::vector<VkShaderStageFlagBits> stages;
		std::vector<VkShaderEXT> shaderObjects;

		for (int i = 0; i < shaders.size(); i++)
		{
			auto& shader = shaders[i];
			for (auto& stage : shader.pimpl->ShaderStages)
			{
				stages.push_back(stage.Stage);
				shaderObjects.push_back(stage.Shader);
			}
		}

		vkCmdBindShadersEXT(pimpl->CommandBuffer, (uint32_t)stages.size(), stages.data(), shaderObjects.data());
	}

	void CommandList::PushConstant(const void* data, size_t size, uint32_t offset)
	{
		vkCmdPushConstants(pimpl->CommandBuffer, pimpl->Context->BindlessPipelineLayout, VK_SHADER_STAGE_ALL, offset, (uint32_t)size, data);
	}

	void CommandList::SetViewport(const iVec2& size)
	{
		float yOffset = (size.y < 0) ? 0.0f : (uint32_t)size.y;
		VkViewport viewport = {
			.x = 0.0f,
			.y = yOffset,
			.width = (float)size.x,
			.height = -((float)size.y),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		vkCmdSetViewportWithCount(pimpl->CommandBuffer, 1, &viewport);
	}

	void CommandList::SetScissor(const iVec2& size, const iVec2& offset)
	{
		VkRect2D scissor = {
			.offset = { (int32_t)offset.x, (int32_t)offset.y },
			.extent = { (uint32_t)size.x, (uint32_t)size.y }
		};

		vkCmdSetScissorWithCount(pimpl->CommandBuffer, 1, &scissor);
	}

	void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		VkSampleMask mask = ~0;
		VkBool32 colorBlendEnable = false;
		VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		VkColorBlendEquationEXT colorBlendEquation = {
			.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD
		};

		vkCmdSetLineWidth(pimpl->CommandBuffer, 1.0f);
		vkCmdSetCullMode(pimpl->CommandBuffer, VK_CULL_MODE_NONE);
		vkCmdSetFrontFace(pimpl->CommandBuffer, VK_FRONT_FACE_CLOCKWISE);
		vkCmdSetPrimitiveTopology(pimpl->CommandBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		vkCmdSetDepthTestEnable(pimpl->CommandBuffer, false);
		vkCmdSetDepthWriteEnable(pimpl->CommandBuffer, false);
		vkCmdSetDepthCompareOp(pimpl->CommandBuffer, VK_COMPARE_OP_NEVER);
		vkCmdSetDepthBoundsTestEnable(pimpl->CommandBuffer, false);
		vkCmdSetDepthBounds(pimpl->CommandBuffer, 0, 1);
		vkCmdSetStencilTestEnable(pimpl->CommandBuffer, false);
		vkCmdSetStencilOp(pimpl->CommandBuffer, VK_STENCIL_FACE_FRONT_BIT, {}, {}, {}, {});
		vkCmdSetRasterizerDiscardEnable(pimpl->CommandBuffer, false);
		vkCmdSetDepthBiasEnable(pimpl->CommandBuffer, false);
		vkCmdSetPrimitiveRestartEnable(pimpl->CommandBuffer, false);
		vkCmdSetPolygonModeEXT(pimpl->CommandBuffer, VK_POLYGON_MODE_FILL);
		vkCmdSetRasterizationSamplesEXT(pimpl->CommandBuffer, VK_SAMPLE_COUNT_1_BIT);
		vkCmdSetSampleMaskEXT(pimpl->CommandBuffer, VK_SAMPLE_COUNT_1_BIT, &mask);
		vkCmdSetAlphaToCoverageEnableEXT(pimpl->CommandBuffer, false);
		vkCmdSetColorBlendEnableEXT(pimpl->CommandBuffer, 0, 1, &colorBlendEnable);
		vkCmdSetColorBlendEquationEXT(pimpl->CommandBuffer, 0, 1, &colorBlendEquation);
		vkCmdSetColorWriteMaskEXT(pimpl->CommandBuffer, 0, 1, &colorWriteMask);
		vkCmdSetVertexInputEXT(pimpl->CommandBuffer, 0, nullptr, 0, nullptr);

		vkCmdDraw(pimpl->CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	Impl<CommandList>::Impl(std::shared_ptr<Impl<GraphicsContext>> context, VkCommandBuffer commandBuffer)
		: Context(context), CommandBuffer(commandBuffer)
	{ }

}