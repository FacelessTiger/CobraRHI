#pragma once

#include <Core/Core.h>

namespace Cobra {

	class GraphicsContext;

	class Fence
	{
	public:
		std::unique_ptr<Impl<Fence>> pimpl;
	public:
		Fence(GraphicsContext& context);
		virtual ~Fence();

		Fence(const Fence&) = delete;
		Fence& operator=(Fence& other) = delete;

		Fence(Fence&& other) noexcept;
		Fence& operator=(Fence&& other) noexcept;

		void Wait() const;
	};

}