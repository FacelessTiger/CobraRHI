#pragma once

#include <Core/Core.h>

namespace Cobra {

	class GraphicsContext;
	class Image;
	class Window;

	class Swapchain
	{
	public:
		std::unique_ptr<Impl<Swapchain>> pimpl;
	public:
		Swapchain(GraphicsContext& context, const Window& window, bool enableVsync = true);
		virtual ~Swapchain();

		Swapchain(const Swapchain&) = delete;
		Swapchain& operator=(Swapchain& other) = delete;

		Swapchain(Swapchain&& other) noexcept;
		Swapchain& operator=(Swapchain&& other) noexcept;
		
		void Resize(uint32_t width, uint32_t height);

		Image& GetCurrent();
	};

}