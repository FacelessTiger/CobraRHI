#pragma once

#include "Core/Core.h"

namespace Cobra {

	class GraphicsContext;
	class Image;

	class Swapchain
	{
	public:
		std::unique_ptr<Impl<Swapchain>> pimpl;
	public:
		Swapchain(GraphicsContext& context, void* window, uVec2 size, bool enableVsync = true);
		virtual ~Swapchain();

		Swapchain(const Swapchain&) = delete;
		Swapchain& operator=(Swapchain& other) = delete;

		Swapchain(Swapchain&& other) noexcept;
		Swapchain& operator=(Swapchain&& other) noexcept;
		
		void Resize(uVec2 newSize);

		Image& GetCurrent();
		uVec2 GetSize() const;
	};

}