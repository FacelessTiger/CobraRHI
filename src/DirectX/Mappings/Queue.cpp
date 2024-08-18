#include "DirectXRHI.h"

namespace Cobra {

	Queue::Queue(Queue&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Queue& Queue::operator=(Queue&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	SyncPoint Queue::Acquire(const Swapchain& swapchain)
	{
		throw std::runtime_error("Not yet implemented");
	}

	SyncPoint Queue::Submit(CommandList& cmd, Span<SyncPoint> wait)
	{
		throw std::runtime_error("Not yet implemented");
	}

	void Queue::Present(const Swapchain& swapchain, Span<SyncPoint> wait)
	{
		throw std::runtime_error("Not yet implemented");
	}

	void Queue::WaitIdle() const
	{
		pimpl->Fence.Wait();
	}

	CommandList Queue::Begin() const
	{
		throw std::runtime_error("Not yet implemented");
	}

	Impl<Queue>::Impl(Impl<GraphicsContext>* context, D3D12_COMMAND_LIST_TYPE type)
		: Context(context), Type(type)
	{
		DX_CHECK(context->Device->CreateCommandQueue(PtrTo(D3D12_COMMAND_QUEUE_DESC{ .Type = type }), IID_PPV_ARGS(&Queue)), "Failed to create queue");
		Fence.pimpl = std::make_unique<Impl<Cobra::Fence>>(context);
	}

}