#pragma once

#include <Core/Core.h>

namespace Cobra {

	enum class MessageSeverity
	{
		Warning,
		Error
	};

	using DebugCallback = void (*)(const char*, MessageSeverity);

	class GraphicsContext
	{
	public:
		std::shared_ptr<Impl<GraphicsContext>> pimpl;
	public:
		GraphicsContext(DebugCallback debugCallback = nullptr);
		virtual ~GraphicsContext();

		GraphicsContext(const GraphicsContext&) = delete;
		GraphicsContext& operator=(GraphicsContext& other) = delete;

		GraphicsContext(GraphicsContext&& other) noexcept;
		GraphicsContext& operator=(GraphicsContext&& other) noexcept;
	};

}