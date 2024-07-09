#include "VulkanRHI.h"

namespace Cobra {

	template<class... Ts>
	struct overloaded : Ts... { using Ts::operator()...; };

	CommandList::CommandList() { }
	CommandList::~CommandList() { }
	CommandList::CommandList(CommandList&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	CommandList& CommandList::operator=(CommandList&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void CommandList::Clear(const Image& image, std::variant<Vec4, iVec4, uVec4> color)
	{
		image.pimpl->TransitionLayout(pimpl->CommandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_2_CLEAR_BIT);

		vkCmdClearColorImage(
			pimpl->CommandBuffer, image.pimpl->Allocation.Image, VK_IMAGE_LAYOUT_GENERAL,
			PtrTo(std::visit(overloaded{
				[&](const Vec4& value)  { return VkClearColorValue { .float32 = { value.r, value.b, value.g, value.a } }; },
				[&](const iVec4& value) { return VkClearColorValue { .int32 = { value.r, value.b, value.g, value.a } }; },
				[&](const uVec4& value) { return VkClearColorValue { .uint32 = { value.r, value.b, value.g, value.a } }; }
			}, color)), 
			1, PtrTo(Utils::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT))
		);
	}

	void CommandList::ClearColorAttachment(uint32_t attachment, std::variant<Vec4, iVec4, uVec4> color, const uVec2& size)
	{
		vkCmdClearAttachments(
			pimpl->CommandBuffer, 1, 
			PtrTo(VkClearAttachment {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.colorAttachment = attachment,
				.clearValue = std::visit(overloaded {
					[&](const Vec4& value)  { return VkClearColorValue { .float32 = { value.r, value.b, value.g, value.a } }; },
					[&](const iVec4& value) { return VkClearColorValue { .int32 = { value.r, value.b, value.g, value.a } }; },
					[&](const uVec4& value) { return VkClearColorValue { .uint32 = { value.r, value.b, value.g, value.a } }; }
				}, color)
			}),
			1, PtrTo(VkClearRect {
				.rect = { .extent = { size.x, size.y } },
				.layerCount = 1
			})
		);
	}

	void CommandList::ClearDepthAttachment(float depth, const uVec2& size)
	{
		VkClearAttachment clear = {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.clearValue = { .depthStencil = { .depth = depth } }
		};

		VkClearRect rect = {
			.rect = {.extent = { size.x, size.y } },
			.layerCount = 1
		};

		vkCmdClearAttachments(pimpl->CommandBuffer, 1, &clear, 1, &rect);
	}

	void CommandList::Present(Swapchain& swapchain)
	{
		swapchain.GetCurrent().pimpl->TransitionLayout(pimpl->CommandBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_NONE);
	}

	void CommandList::CopyBufferRegion(const Buffer& src, const Buffer& dst, size_t size, uint32_t srcOffset, uint32_t dstOffset)
	{
		vkCmdCopyBuffer(pimpl->CommandBuffer, src.pimpl->Allocation.Buffer, dst.pimpl->Allocation.Buffer, 1, PtrTo(VkBufferCopy {
			.srcOffset = srcOffset,
			.dstOffset = dstOffset,
			.size = size
		}));
	}

	void CommandList::CopyToImage(const Buffer& src, const Image& dst, uint32_t srcOffset)
	{
		dst.pimpl->TransitionLayout(pimpl->CommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT);

		vkCmdCopyBufferToImage(pimpl->CommandBuffer, src.pimpl->Allocation.Buffer, dst.pimpl->Allocation.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, PtrTo(VkBufferImageCopy{
			.bufferOffset = srcOffset,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
			.imageExtent = { dst.pimpl->Size.x, dst.pimpl->Size.y, 1 }
		}));
	}

	void CommandList::BlitImage(const Image& src, const Image& dst, uVec2 srcSize)
	{
		src.pimpl->TransitionLayout(pimpl->CommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_BLIT_BIT);
		dst.pimpl->TransitionLayout(pimpl->CommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_BLIT_BIT);

		if (srcSize == uVec2{ 0, 0 })
			srcSize = src.pimpl->Size;

		vkCmdBlitImage(pimpl->CommandBuffer, src.pimpl->Allocation.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst.pimpl->Allocation.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, PtrTo(VkImageBlit{
			.srcSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
			.srcOffsets = {
				VkOffset3D {},
				VkOffset3D {
					.x = (int32_t)srcSize.x,
					.y = (int32_t)srcSize.y,
					.z = 1
				}
			},
			.dstSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
			.dstOffsets = {
				VkOffset3D {},
				VkOffset3D {
					.x = (int32_t)dst.pimpl->Size.x,
					.y = (int32_t)dst.pimpl->Size.y,
					.z = 1
				}
			}
		}), VK_FILTER_NEAREST);
	}

	void CommandList::BeginRendering(const uVec2& region, Span<Image> colorAttachments, const Image* depthAttachment)
	{
		pimpl->GraphicsKey.ColorAttachmentCount = 1;
		pimpl->GraphicsKey.ColorAttachments[0] = colorAttachments[0].pimpl->Format;
		pimpl->GraphicsStateChanged = true;

		colorAttachments[0].pimpl->TransitionLayout(pimpl->CommandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
		VkRenderingAttachmentInfo vulkanColorAttachment = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = colorAttachments[0].pimpl->View,
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

		VkRenderingAttachmentInfo depthInfo = { .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		if (depthAttachment)
		{
			depthAttachment->pimpl->TransitionLayout(pimpl->CommandBuffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT);

			depthInfo.imageView = depthAttachment->pimpl->View;
			depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			renderInfo.pDepthAttachment = &depthInfo;
			pimpl->GraphicsKey.DepthAttachment = depthAttachment->pimpl->Format;
		}

		vkCmdBeginRendering(pimpl->CommandBuffer, &renderInfo);
	}

	void CommandList::EndRendering()
	{
		vkCmdEndRendering(pimpl->CommandBuffer);
	}

	void CommandList::Barrier(PipelineStage src, PipelineStage dst)
	{
		vkCmdPipelineBarrier2(pimpl->CommandBuffer, PtrTo(VkDependencyInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.memoryBarrierCount = 1,
			.pMemoryBarriers = PtrTo(VkMemoryBarrier2 {
				.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
				.srcStageMask = Utils::CBPipelineStageToVulkan(src),
				.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
				.dstStageMask = Utils::CBPipelineStageToVulkan(dst),
				.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT
			})
		}));
	}

	void CommandList::Barrier(const Buffer& buffer, PipelineStage src, PipelineStage dst)
	{
		vkCmdPipelineBarrier2(pimpl->CommandBuffer, PtrTo(VkDependencyInfo {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.bufferMemoryBarrierCount = 1,
			.pBufferMemoryBarriers = PtrTo(VkBufferMemoryBarrier2 {
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
				.srcStageMask = Utils::CBPipelineStageToVulkan(src),
				.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
				.dstStageMask = Utils::CBPipelineStageToVulkan(dst),
				.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
				.buffer = buffer.pimpl->Allocation.Buffer,
				.size = VK_WHOLE_SIZE
			})
		}));
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
				if (g_ShaderObjectsSupported)
				{
					stages.push_back(stage.Stage);
					shaderObjects.push_back(stage.Shader);
				}
				else
				{
					switch (stage.Stage)
					{
						case VK_SHADER_STAGE_VERTEX_BIT:
						{
							pimpl->GraphicsKey.Shaders[0] = stage.ID;
							pimpl->GraphicsStateChanged = true;
							break;
						}
						case VK_SHADER_STAGE_FRAGMENT_BIT:
						{
							pimpl->GraphicsKey.Shaders[4] = stage.ID;
							pimpl->GraphicsStateChanged = true;
							break;
						}
						case VK_SHADER_STAGE_COMPUTE_BIT:
						{
							ComputePipelineKey key;
							key.Shader = stage.ID;

							vkCmdBindPipeline(pimpl->CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, GetComputePipeline(pimpl.get(), key));
							break;
						}
					}
				}
			}
		}

		if (g_ShaderObjectsSupported) vkCmdBindShadersEXT(pimpl->CommandBuffer, (uint32_t)stages.size(), stages.data(), shaderObjects.data());
	}

	void CommandList::BindIndexBuffer(const Buffer& buffer, IndexType type, uint64_t offset)
	{
		vkCmdBindIndexBuffer(pimpl->CommandBuffer, buffer.pimpl->Allocation.Buffer, offset, (type == IndexType::Uint16) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
	}

	void CommandList::PushConstant(const void* data, size_t size, uint32_t offset)
	{
		vkCmdPushConstants(pimpl->CommandBuffer, pimpl->Context->BindlessPipelineLayout, VK_SHADER_STAGE_ALL, offset, (uint32_t)size, data);
	}

	void CommandList::SetDefaultState()
	{
		vkCmdSetDepthTestEnable(pimpl->CommandBuffer, false);
		vkCmdSetDepthWriteEnable(pimpl->CommandBuffer, false);
		vkCmdSetDepthCompareOp(pimpl->CommandBuffer, VK_COMPARE_OP_NEVER);

		if (g_ShaderObjectsSupported)
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
		}
		else
		{
			pimpl->GraphicsKey.BlendEnable = false;
			pimpl->GraphicsKey.SrcBlend = BlendFactor::Zero;
			pimpl->GraphicsKey.DstBlend = BlendFactor::Zero;
			pimpl->GraphicsKey.BlendOp = BlendOp::Add;
			pimpl->GraphicsKey.SrcBlendAlpha = BlendFactor::Zero;
			pimpl->GraphicsKey.DstBlendAlpha = BlendFactor::Zero;
			pimpl->GraphicsKey.BlendAlpha = BlendOp::Add;
			pimpl->GraphicsStateChanged = true;
		}
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

	void CommandList::EnableDepthTest(bool writeEnabled, CompareOperation op)
	{
		vkCmdSetDepthTestEnable(pimpl->CommandBuffer, true);
		vkCmdSetDepthWriteEnable(pimpl->CommandBuffer, writeEnabled);
		vkCmdSetDepthCompareOp(pimpl->CommandBuffer, Utils::CBCompareOpToVulkan(op));
	}

	void CommandList::EnableColorBlend(BlendFactor srcBlend, BlendFactor dstBlend, BlendOp blendOp, BlendFactor srcBlendAlpha, BlendFactor dstBlendAlpha, BlendOp blendAlpha)
	{
		if (g_ShaderObjectsSupported)
		{
			vkCmdSetColorBlendEnableEXT(pimpl->CommandBuffer, 0, 1, PtrTo<VkBool32>(true));
			vkCmdSetColorBlendEquationEXT(pimpl->CommandBuffer, 0, 1, PtrTo(VkColorBlendEquationEXT {
				.srcColorBlendFactor = Utils::CBBlendFactorToVulkan(srcBlend),
				.dstColorBlendFactor = Utils::CBBlendFactorToVulkan(dstBlend),
				.colorBlendOp = Utils::CBBlendOpToVulkan(blendOp),
				.srcAlphaBlendFactor = Utils::CBBlendFactorToVulkan(srcBlendAlpha),
				.dstAlphaBlendFactor = Utils::CBBlendFactorToVulkan(dstBlendAlpha),
				.alphaBlendOp = Utils::CBBlendOpToVulkan(blendAlpha)
			}));
		}
		else
		{
			pimpl->GraphicsKey.BlendEnable = true;
			pimpl->GraphicsKey.SrcBlend = srcBlend;
			pimpl->GraphicsKey.DstBlend = dstBlend;
			pimpl->GraphicsKey.BlendOp = blendOp;
			pimpl->GraphicsKey.SrcBlendAlpha = srcBlendAlpha;
			pimpl->GraphicsKey.DstBlendAlpha = dstBlendAlpha;
			pimpl->GraphicsKey.BlendAlpha = blendAlpha;
			pimpl->GraphicsStateChanged = true;
		}
	}

	void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		pimpl->BindPipelineIfNeeded();
		vkCmdDraw(pimpl->CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandList::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)
	{
		pimpl->BindPipelineIfNeeded();
		vkCmdDrawIndirect(pimpl->CommandBuffer, buffer.pimpl->Allocation.Buffer, offset, drawCount, stride);
	}

	void CommandList::DrawIndirectCount(const Buffer& buffer, uint64_t offset, const Buffer& countBuffer, uint64_t countOffset, uint32_t maxDrawCount, uint32_t stride)
	{
		pimpl->BindPipelineIfNeeded();
		vkCmdDrawIndirectCount(pimpl->CommandBuffer, buffer.pimpl->Allocation.Buffer, offset, buffer.pimpl->Allocation.Buffer, countOffset, maxDrawCount, stride);
	}

	void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
	{
		pimpl->BindPipelineIfNeeded();
		vkCmdDrawIndexed(pimpl->CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void CommandList::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)
	{
		pimpl->BindPipelineIfNeeded();
		vkCmdDrawIndexedIndirect(pimpl->CommandBuffer, buffer.pimpl->Allocation.Buffer, offset, drawCount, stride);
	}

	void CommandList::DrawIndexedIndirectCount(const Buffer& buffer, uint64_t offset, const Buffer& countBuffer, uint64_t countOffset, uint32_t maxDrawCount, uint32_t stride)
	{
		pimpl->BindPipelineIfNeeded();
		vkCmdDrawIndexedIndirectCount(pimpl->CommandBuffer, buffer.pimpl->Allocation.Buffer, offset, buffer.pimpl->Allocation.Buffer, countOffset, maxDrawCount, stride);
	}

	void CommandList::Dispatch(uint32_t workX, uint32_t workY, uint32_t workZ)
	{
		vkCmdDispatch(pimpl->CommandBuffer, workX, workY, workZ);
	}

	void CommandList::DispatchIndirect(const Buffer& buffer, uint64_t offset)
	{
		vkCmdDispatchIndirect(pimpl->CommandBuffer, buffer.pimpl->Allocation.Buffer, offset);
	}

	Impl<CommandList>::Impl(Impl<GraphicsContext>* context, Impl<Queue>::CommandAllocator* allocator, VkCommandBuffer commandBuffer)
		: Context(context), Allocator(allocator), CommandBuffer(commandBuffer)
	{ }

	void Impl<CommandList>::BindPipelineIfNeeded()
	{
		if (GraphicsStateChanged && !g_ShaderObjectsSupported)
		{
			vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GetGraphicsPipeline(this, GraphicsKey));
			GraphicsStateChanged = false;
		}
	}

}