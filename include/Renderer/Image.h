#pragma once

#include <Core/Core.h>

namespace Cobra {

	class GraphicsContext;
	class Swapchain;

	class Image
	{
	public:
		std::unique_ptr<Impl<Image>> pimpl;
	public:
		//Image(GraphicsContext& context);
		virtual ~Image();

		Image(const Image&) = delete;
		Image& operator=(Image& other) = delete;

		Image(Image&& other) noexcept;
		Image& operator=(Image&& other) noexcept;
	private:
		friend struct Impl<Swapchain>;
		Image(std::shared_ptr<Impl<GraphicsContext>> context);
	};

}