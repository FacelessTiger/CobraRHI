#pragma once

#include <Core/Core.h>

namespace Cobra {

	class GraphicsContext;
	class CommandList;
	class Queue;

	class CommandAllocator
	{
	public:
		std::unique_ptr<Impl<CommandAllocator>> pimpl;
	public:
		CommandAllocator(GraphicsContext& context, const Queue& queue);
		virtual ~CommandAllocator();

		CommandAllocator(const CommandAllocator&) = delete;
		CommandAllocator& operator=(CommandAllocator& other) = delete;

		CommandAllocator(CommandAllocator&& other) noexcept;
		CommandAllocator& operator=(CommandAllocator&& other) noexcept;

		CommandAllocator& Reset();
		CommandList Begin();
	};

}