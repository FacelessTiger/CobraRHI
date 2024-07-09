#pragma once

#include "Core/Core.h"

namespace Cobra {

	class GraphicsContext;

	enum class BufferUsage : uint32_t
	{
		StorageBuffer = 1,
		TransferSrc = 2,
		TransferDst = 4,
		IndexBuffer = 8,
		IndirectBuffer = 16
	};

	enum class BufferFlags : uint32_t
	{
		None = 0,
		Mapped = 1,
		DeviceLocal = 2
	};

	inline BufferUsage operator|(BufferUsage a, BufferUsage b) { return (BufferUsage)((int)a | (int)b); };
	inline bool operator&(BufferUsage a, BufferUsage b) { return (int)a & (int)b; };

	inline BufferFlags operator|(BufferFlags a, BufferFlags b) { return (BufferFlags)((int)a | (int)b); };
	inline bool operator&(BufferFlags a, BufferFlags b) { return (int)a & (int)b; };

	class Buffer
	{
	public:
		std::unique_ptr<Impl<Buffer>> pimpl;
	public:
		Buffer(GraphicsContext& context, size_t size, BufferUsage usage, BufferFlags flags = BufferFlags::None);
		virtual ~Buffer();

		Buffer(const Buffer&) = delete;
		Buffer& operator=(Buffer& other) = delete;

		Buffer(Buffer&& other) noexcept;
		Buffer& operator=(Buffer&& other) noexcept;

		void SetDebugName(std::string_view name);

		size_t GetSize() const;

		std::byte* GetHostAddress() const;
		uint64_t GetDeviceAddress() const;
	};

}