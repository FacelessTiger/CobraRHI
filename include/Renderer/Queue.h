#pragma once

#include <Core/Core.h>

namespace Cobra {

	class GraphicsContext;
	class CommandList;
	class Swapchain;
	class Fence;

	enum class QueueType
	{
		Graphics
	};

	class Queue
	{
	public:
		std::unique_ptr<Impl<Queue>> pimpl;
	public:
		Queue(GraphicsContext& context, QueueType type);
		virtual ~Queue();

		Queue(const Queue&) = delete;
		Queue& operator=(Queue& other) = delete;

		Queue(Queue&& other) noexcept;
		Queue& operator=(Queue&& other) noexcept;

		void Acquire(const Swapchain& swapchain, Fence& fence);
		void Submit(const CommandList& cmd, Fence& wait, Fence& signal);
		void Present(const Swapchain& swapchain, Fence& wait);
	};

}