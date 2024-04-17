#pragma once

#include <Core/Core.h>

#include <span>
#include <vector>

namespace Cobra {

	class Shader;
	class Swapchain;

	class CommandList
	{
	public:
		std::unique_ptr<Impl<CommandList>> pimpl;
	public:
		CommandList();
		virtual ~CommandList();

		CommandList(const CommandList&) = delete;
		CommandList& operator=(CommandList& other) = delete;

		CommandList(CommandList&& other) noexcept;
		CommandList& operator=(CommandList&& other) noexcept;

		void Present(Swapchain& swapchain);

		void BeginRendering(const iVec2& region, const Image& colorAttachment);
		void EndRendering();

		void BindShaders(Span<Shader> shaders);
		void PushConstant(const void* data, size_t size, uint32_t offset);
		template<typename T> void PushConstant(const T& data, uint32_t offset = 0) { PushConstant(&data, sizeof(T), offset); };

		void SetViewport(const iVec2& size);
		void SetScissor(const iVec2& size, const iVec2& offset = { 0, 0 });

		void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
	};

}