#include "DirectXRHI.h"

namespace Cobra {

	Queue::Queue(Queue&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Queue& Queue::operator=(Queue&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	SyncPoint Queue::Acquire(const Swapchain& swapchain)
	{
		swapchain.pimpl->ImageIndex = swapchain.pimpl->Swapchain->GetCurrentBackBufferIndex();
		DX_CHECK(pimpl->Queue->Signal(pimpl->Fence.pimpl->Fence.Get(), pimpl->Fence.Advance()), "Failed to signal Fence when acquiring");

		return SyncPoint(pimpl->Fence);
	}

	SyncPoint Queue::Submit(CommandList& cmd, Span<SyncPoint> wait)
	{
		DX_CHECK(cmd.pimpl->CommandList->Close(), "Couldn't end command list");
		for (int i = 0; i < wait.size(); i++)
		{
			DX_CHECK(pimpl->Queue->Wait(wait[i].pFence->pimpl->Fence.Get(), wait[i].GetValue()), "Failed to wait when submitting");
		}

		ID3D12CommandList* list = cmd.pimpl->CommandList.Get();
		pimpl->Queue->ExecuteCommandLists(1, &list);
		DX_CHECK(pimpl->Queue->Signal(pimpl->Fence.pimpl->Fence.Get(), pimpl->Fence.Advance()), "Failed to signal Fence when submitting");

		if (cmd.pimpl->Recording)
			pimpl->Allocators.push_back(std::move(cmd.pimpl->Allocator));
		cmd.pimpl->Recording = false;
		pimpl->PendingCommandLists.push_back({ std::move(cmd), pimpl->Fence.GetPendingValue() });

		return SyncPoint(pimpl->Fence);
	}

	void Queue::Present(const Swapchain& swapchain, Span<SyncPoint> wait)
	{
		for (int i = 0; i < wait.size(); i++)
		{
			DX_CHECK(pimpl->Queue->Wait(wait[i].pFence->pimpl->Fence.Get(), wait[i].GetValue()), "Failed to wait when submitting");
		}

		swapchain.pimpl->Swapchain->Present(1, 0);
	}

	void Queue::WaitIdle() const
	{
		pimpl->Fence.Wait();
	}

	CommandList Queue::Begin() const
	{
		CommandList cmd;

		// Clear pending command lists
		auto currentValue = pimpl->Fence.GetCurrentValue();
		//pimpl->Context->DeletionQueues->Flush(currentValue);

		while (!pimpl->PendingCommandLists.empty())
		{
			auto& pending = pimpl->PendingCommandLists.front();
			if (currentValue >= pending.FenceValue)
			{
				pending.CommandList.pimpl->CommandList->Reset(pending.CommandList.pimpl->Allocator->CommandAllocator.Get(), nullptr);

				auto* allocator = pending.CommandList.pimpl->Allocator;
				allocator->AvailableCommandLists.push_back(std::move(pending.CommandList));
				pimpl->PendingCommandLists.pop_front();
			}
			else
			{
				break;
			}
		}

		auto* allocator = pimpl->AcquireCommandAllocator();
		if (allocator->AvailableCommandLists.empty())
		{
			ComPtr<ID3D12GraphicsCommandList> list;
			DX_CHECK(pimpl->Context->Device->CreateCommandList(0, pimpl->Type, allocator->CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&list)), "Failed to create command buffer on-demand");

			ComPtr<ID3D12GraphicsCommandList7> list7;
			DX_CHECK(list.As(&list7), "Could not convert CommandList to CommandList7");
			cmd.pimpl = std::make_unique<Impl<CommandList>>(pimpl->Context, allocator, list7);
		}
		else
		{
			cmd = std::move(allocator->AvailableCommandLists.back());
			allocator->AvailableCommandLists.pop_back();
		}

		cmd.pimpl->Recording = true;

		ID3D12DescriptorHeap* descriptorHeap[2] = { pimpl->Context->BindlessDescriptorHeap.Get(), pimpl->Context->BindlessSamplerHeap.Get() };
		cmd.pimpl->CommandList->SetDescriptorHeaps(2, descriptorHeap);

		cmd.pimpl->CommandList->SetGraphicsRootSignature(pimpl->Context->BindlessRootSignature.Get());
		//if (pimpl->Flags & VK_QUEUE_GRAPHICS_BIT) vkCmdBindDescriptorSets(cmd.pimpl->CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pimpl->Context->BindlessPipelineLayout, 0, 1, &pimpl->Context->BindlessSet, 0, nullptr);
		//if (pimpl->Flags & VK_QUEUE_COMPUTE_BIT) vkCmdBindDescriptorSets(cmd.pimpl->CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pimpl->Context->BindlessPipelineLayout, 0, 1, &pimpl->Context->BindlessSet, 0, nullptr);

		return cmd;
	}

	Impl<Queue>::Impl(Impl<GraphicsContext>* context, D3D12_COMMAND_LIST_TYPE type)
		: Context(context), Type(type)
	{
		DX_CHECK(context->Device->CreateCommandQueue(PtrTo(D3D12_COMMAND_QUEUE_DESC{ .Type = type }), IID_PPV_ARGS(&Queue)), "Failed to create queue");
		Fence.pimpl = std::make_unique<Impl<Cobra::Fence>>(context);
	}

	Impl<Queue>::CommandAllocator* Impl<Queue>::AcquireCommandAllocator()
	{
		CommandAllocator* allocator;
		if (Allocators.empty())
		{
			allocator = new CommandAllocator();
			DX_CHECK(Context->Device->CreateCommandAllocator(Type, IID_PPV_ARGS(&allocator->CommandAllocator)), "Failed to create command allocator on-demand");
		}
		else
		{
			allocator = Allocators.back();
			Allocators.pop_back();
		}

		return allocator;
	}

	void Impl<Queue>::Destroy()
	{
		for (auto& allocator : Allocators)
			delete allocator;

		Allocators.clear();
		PendingCommandLists.clear();

		Fence.pimpl->Fence.Reset();
		Queue.Reset();
	}

}