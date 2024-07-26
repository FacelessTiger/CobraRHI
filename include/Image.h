#pragma once

#include "Core/Core.h"

namespace Cobra {

	class GraphicsContext;
	class Swapchain;

	enum class ImageFormat
	{
		Unknown = 0,
		R32_SINT,
		R16G16B16A16_SFLOAT,
		R8G8B8A8_UNORM,
		B8G8R8A8_SRGB,
		D32_SFLOAT
	};

	enum class ImageUsage : uint32_t
	{
		None = 0,
		ColorAttachment = 1,
		DepthStencilAttachment = 2,
		TransferSrc = 4,
		TransferDst = 8,
		Storage = 16,
		Sampled = 32
	};

	enum class ImageLayout : uint32_t
	{
		General,
		ReadOnlyOptimal
	};

	inline ImageUsage operator|(ImageUsage a, ImageUsage b) { return (ImageUsage)((int)a | (int)b); };
	inline ImageUsage& operator|=(ImageUsage& a, ImageUsage b) { return a = a | b; };
	inline bool operator&(ImageUsage a, ImageUsage b) { return (int)a & (int)b; };

	class Image
	{
	public:
		std::unique_ptr<Impl<Image>> pimpl;
	public:
		Image(GraphicsContext& context, const uVec2& size, ImageFormat format, ImageUsage usage);
		virtual ~Image();

		Image(const Image&) = delete;
		Image& operator=(Image& other) = delete;

		Image(Image&& other) noexcept;
		Image& operator=(Image&& other) noexcept;

		void SetDebugName(std::string_view name);

		void Set(const void* data) const;
		void Transition(ImageLayout layout) const;

		uint32_t GetHandle() const;
	private:
		friend struct Impl<Swapchain>;
		Image(std::shared_ptr<Impl<GraphicsContext>> context);
	};

}