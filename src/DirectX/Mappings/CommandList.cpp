#include "DirectXRHI.h"

namespace Cobra {

	template<class... Ts>
	struct overloaded : Ts... { using Ts::operator()...; };
	template <typename... Ts>
	overloaded(Ts&&...) -> overloaded<std::decay_t<Ts>...>;

	CommandList::CommandList() { }
	CommandList::~CommandList() { }
	CommandList::CommandList(CommandList&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	CommandList& CommandList::operator=(CommandList&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void CommandList::ClearColorAttachment(uint32_t attachment, std::variant<Vec4, iVec4, uVec4> color, const uVec2& size)
	{
		pimpl->CommandList->ClearRenderTargetView(pimpl->ColorAttachmentHandles[attachment], std::visit(overloaded{
			[&](const Vec4& value) { return value.data; },
			[&](const iVec4& value) { return (const float*)value.data; },
			[&](const uVec4& value) { return (const float*)value.data; }
		}, color), 
		1, PtrTo(D3D12_RECT { .right = (LONG)size.x, .bottom = (LONG)size.y }));
	}

	/*void CommandList::ClearDepthAttachment(float depth, const uVec2& size)
	{
		// Get depth view
		pimpl->CommandList->ClearDepthStencilView(0, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 1, PtrTo(D3D12_RECT{
			.right = (LONG)size.x,
			.bottom = (LONG)size.y
		}));
	}*/

	void CommandList::Present(Swapchain& swapchain)
	{
		swapchain.GetCurrent().pimpl->TransitionLayout(pimpl->CommandList, D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_SYNC_NONE);
	}

	void CommandList::CopyToImage(const Buffer& src, const Image& dst, uint32_t srcOffset)
	{
		dst.pimpl->TransitionLayout(pimpl->CommandList, D3D12_BARRIER_LAYOUT_COPY_DEST, D3D12_BARRIER_SYNC_COPY);

		uint32_t bpp = 32; // TODO: make a utility function to calculate this off of format
		pimpl->CommandList->CopyTextureRegion(PtrTo(CD3DX12_TEXTURE_COPY_LOCATION(dst.pimpl->Image.Get())), 0, 0, 0, PtrTo(CD3DX12_TEXTURE_COPY_LOCATION(src.pimpl->Buffer.Get(), D3D12_PLACED_SUBRESOURCE_FOOTPRINT{
			.Footprint = {
				.Format = Utils::CBImageFormatToDirectX(dst.pimpl->Format),
				.Width = dst.pimpl->Size.x,
				.Height = dst.pimpl->Size.y,
				.Depth = 1,
				.RowPitch = (dst.pimpl->Size.x * bpp + 7) / 8
			}
		})), nullptr);
	}

	void CommandList::CopyToBuffer(const Image& src, const Buffer& dst, uint64_t dstOffset)
	{
		src.pimpl->TransitionLayout(pimpl->CommandList, D3D12_BARRIER_LAYOUT_COPY_SOURCE, D3D12_BARRIER_SYNC_COPY);

		uint32_t bpp = 32; // TODO: make a utility function to calculate this off of format
		pimpl->CommandList->CopyTextureRegion(PtrTo(CD3DX12_TEXTURE_COPY_LOCATION(dst.pimpl->Buffer.Get(), D3D12_PLACED_SUBRESOURCE_FOOTPRINT {
			.Footprint = {
				.Format = Utils::CBImageFormatToDirectX(src.pimpl->Format),
				.Width = src.pimpl->Size.x,
				.Height = src.pimpl->Size.y,
				.Depth = 1,
				.RowPitch = (src.pimpl->Size.x * bpp + 7) / 8
			}
		})), 0, 0, 0, PtrTo(CD3DX12_TEXTURE_COPY_LOCATION(src.pimpl->Image.Get())), nullptr);
	}

	void CommandList::BeginRendering(const uVec2& region, Span<Image> colorAttachments, const Image* depthAttachment)
	{
		pimpl->GraphicsKey.ColorAttachmentCount = 1;
		pimpl->GraphicsKey.ColorAttachments[0] = colorAttachments[0].pimpl->Format;
		pimpl->ColorAttachmentHandles[0] = colorAttachments[0].pimpl->CpuHandle;
		pimpl->GraphicsStateChanged = true;

		colorAttachments[0].pimpl->TransitionLayout(pimpl->CommandList, D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_SYNC_DRAW);
		pimpl->CommandList->OMSetRenderTargets(1, &colorAttachments[0].pimpl->CpuHandle, false, nullptr);
	}

	void CommandList::EndRendering()
	{

	}

	void CommandList::Barrier(PipelineStage src, PipelineStage dst)
	{
		pimpl->CommandList->Barrier(1, PtrTo(D3D12_BARRIER_GROUP{
			.Type = D3D12_BARRIER_TYPE_GLOBAL,
			.NumBarriers = 1,
			.pGlobalBarriers = PtrTo(D3D12_GLOBAL_BARRIER {
				.SyncBefore = Utils::CBPipelineStageToDirectX(src),
				.SyncAfter = Utils::CBPipelineStageToDirectX(dst),
				.AccessBefore = D3D12_BARRIER_ACCESS_COMMON,
				.AccessAfter = D3D12_BARRIER_ACCESS_COMMON
			})
		}));
	}

	void CommandList::BindShaders(Span<Shader> shaders)
	{
		for (int i = 0; i < shaders.size(); i++)
		{
			auto& shader = *shaders[i].pimpl;
			for (auto& stage : shader.ShaderStages)
			{
				if (stage.Stage & ShaderStage::Vertex)
				{
					pimpl->GraphicsKey.Shaders[0] = stage.ID;
					pimpl->GraphicsStateChanged = true;
				}

				if (stage.Stage & ShaderStage::Pixel)
				{
					pimpl->GraphicsKey.Shaders[4] = stage.ID;
					pimpl->GraphicsStateChanged = true;
				}

				if (stage.Stage & ShaderStage::Compute)
				{
					ComputePipelineKey key;
					key.Shader = stage.ID;

					pimpl->CommandList->SetPipelineState(GetComputePipeline(pimpl.get(), key));
				}
			}
		}
	}

	void CommandList::PushConstant(const void* data, size_t size, uint32_t offset)
	{
		pimpl->CommandList->SetComputeRoot32BitConstants(0, size / 4, data, offset);
		pimpl->CommandList->SetGraphicsRoot32BitConstants(0, size / 4, data, offset);
	}

	void CommandList::SetDefaultState()
	{
		pimpl->CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		pimpl->GraphicsKey.BlendEnable = false;
		pimpl->GraphicsKey.SrcBlend = BlendFactor::Zero;
		pimpl->GraphicsKey.DstBlend = BlendFactor::Zero;
		pimpl->GraphicsKey.BlendOp = BlendOp::Add;
		pimpl->GraphicsKey.SrcBlendAlpha = BlendFactor::Zero;
		pimpl->GraphicsKey.DstBlendAlpha = BlendFactor::Zero;
		pimpl->GraphicsKey.BlendAlpha = BlendOp::Add;
		pimpl->GraphicsStateChanged = true;
	}

	void CommandList::SetViewport(const iVec2& size)
	{
		pimpl->CommandList->RSSetViewports(1, PtrTo(D3D12_VIEWPORT{
			.Width = (FLOAT)size.x,
			.Height  = (FLOAT)size.y,
			.MinDepth = 0.0f,
			.MaxDepth = 1.0f
		}));
	}

	void CommandList::SetScissor(const iVec2& size, const iVec2& offset)
	{
		pimpl->CommandList->RSSetScissorRects(1, PtrTo(D3D12_RECT {
			.left = (LONG)offset.x,
			.top = (LONG)offset.y,
			.right = (LONG)(size.x + offset.x),
			.bottom = (LONG)(size.y + offset.y),
		}));
	}

	void CommandList::EnableColorBlend(BlendFactor srcBlend, BlendFactor dstBlend, BlendOp blendOp, BlendFactor srcBlendAlpha, BlendFactor dstBlendAlpha, BlendOp blendAlpha)
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

	void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		pimpl->BindPipelineIfNeeded();
		pimpl->CommandList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandList::DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)
	{
		pimpl->BindPipelineIfNeeded();
		//vkCmdDrawIndirect(pimpl->CommandBuffer, buffer.pimpl->Allocation.Buffer, offset, drawCount, stride);
	}

	void CommandList::DrawIndirectCount(const Buffer& buffer, uint64_t offset, const Buffer& countBuffer, uint64_t countOffset, uint32_t maxDrawCount, uint32_t stride)
	{
		pimpl->BindPipelineIfNeeded();
		//vkCmdDrawIndirectCount(pimpl->CommandBuffer, buffer.pimpl->Allocation.Buffer, offset, buffer.pimpl->Allocation.Buffer, countOffset, maxDrawCount, stride);
	}

	void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
	{
		pimpl->BindPipelineIfNeeded();
		pimpl->CommandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void CommandList::DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)
	{
		pimpl->BindPipelineIfNeeded();
		//vkCmdDrawIndexedIndirect(pimpl->CommandBuffer, buffer.pimpl->Allocation.Buffer, offset, drawCount, stride);
	}

	void CommandList::DrawIndexedIndirectCount(const Buffer& buffer, uint64_t offset, const Buffer& countBuffer, uint64_t countOffset, uint32_t maxDrawCount, uint32_t stride)
	{
		pimpl->BindPipelineIfNeeded();
		//vkCmdDrawIndexedIndirectCount(pimpl->CommandBuffer, buffer.pimpl->Allocation.Buffer, offset, buffer.pimpl->Allocation.Buffer, countOffset, maxDrawCount, stride);
	}

	void CommandList::Dispatch(uint32_t workX, uint32_t workY, uint32_t workZ)
	{
		pimpl->CommandList->Dispatch(workX, workY, workZ);
	}

	void CommandList::DispatchIndirect(const Buffer& buffer, uint64_t offset)
	{
		//vkCmdDispatchIndirect(pimpl->CommandBuffer, buffer.pimpl->Allocation.Buffer, offset);
	}

	Impl<CommandList>::Impl(Impl<GraphicsContext>* context, Impl<Queue>::CommandAllocator* allocator, ComPtr<ID3D12GraphicsCommandList7> commandList)
		: Context(context), Allocator(allocator), CommandList(commandList)
	{ }

	void Impl<CommandList>::BindPipelineIfNeeded()
	{
		if (GraphicsStateChanged)
		{
			CommandList->SetPipelineState(GetGraphicsPipeline(this, GraphicsKey));
			GraphicsStateChanged = false;
		}
	}

}