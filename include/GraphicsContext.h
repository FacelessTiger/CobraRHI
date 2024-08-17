#pragma once

#include "Core/Core.h"

#include <functional>
#include <filesystem>
#include <string>
#include <expected>

namespace Cobra {

	class Queue;

	enum class MessageSeverity
	{
		Trace,
		Warning,
		Error
	};

	enum class QueueType
	{
		Graphics,
		Transfer
	};

	struct ContextConfig
	{
		using DebugCallback = std::function<void(const char*, MessageSeverity)>;

		bool Debug = false;
		bool Trace = false;
		std::filesystem::path PipelineCacheLocation = "pipeline.cache";
		DebugCallback Callback = nullptr;
	};

	class GraphicsContext
	{
	public:
		std::shared_ptr<Impl<GraphicsContext>> pimpl;
	public:
		GraphicsContext(const ContextConfig& config = {});
		virtual ~GraphicsContext();

		GraphicsContext(const GraphicsContext&) = delete;
		GraphicsContext& operator=(GraphicsContext& other) = delete;

		GraphicsContext(GraphicsContext&& other) noexcept;
		GraphicsContext& operator=(GraphicsContext&& other) noexcept;

		Queue& GetQueue(QueueType type);
	};		
}