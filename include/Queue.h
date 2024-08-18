#pragma once

#include "Core/Core.h"

namespace Cobra {

	class GraphicsContext;
	class CommandList;
	class Swapchain;
	struct SyncPoint;

	class Queue
	{
	public:
		std::unique_ptr<Impl<Queue>> pimpl;
	public:
		Queue(const Queue&) = delete;
		Queue& operator=(Queue& other) = delete;

		Queue(Queue&& other) noexcept;
		Queue& operator=(Queue&& other) noexcept;

		SyncPoint Acquire(const Swapchain& swapchain);
		SyncPoint Submit(CommandList& cmd, Span<SyncPoint> wait);
		void Present(const Swapchain& swapchain, Span<SyncPoint> wait);

		void WaitIdle() const;

		CommandList Begin() const;
	private:
		friend Impl<GraphicsContext>;
		Queue() = default;
	};

}