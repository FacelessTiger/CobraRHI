#pragma once

#include "Core/Core.h"
#include "Image.h"

#include <span>
#include <vector>
#include <variant>

namespace Cobra {

	class Shader;
	class Buffer;
	class Swapchain;

	enum class CompareOperation
	{
		Never = 0,
		Greater,
		GreaterEqual,
		LesserEqual
	};

	enum class IndexType
	{
		Uint16,
		Uint32
	};

	enum class PipelineStage : uint32_t
	{
		None = 0,
		Compute = 1,
		Transfer = 2,
		Graphics = 4,
		All = Compute | Transfer | Graphics
	};

	enum class BlendFactor
	{
		Zero,
		One,
		SrcAlpha,
		DstAlpha,
		OneMinusSrcAlpha
	};

	enum class BlendOp
	{
		Add
	};

	inline PipelineStage operator|(PipelineStage a, PipelineStage b) { return (PipelineStage)((int)a | (int)b); };
	inline PipelineStage& operator|=(PipelineStage& a, PipelineStage b) { return a = a | b; };
	inline bool operator&(PipelineStage a, PipelineStage b) { return (int)a & (int)b; };

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

		void Clear(const Image& image, std::variant<Vec4, iVec4, uVec4> color);
		void ClearColorAttachment(uint32_t attachment, std::variant<Vec4, iVec4, uVec4> color, const uVec2& size);
		void ClearDepthAttachment(float depth, const uVec2& size);
		void Present(Swapchain& swapchain);

		void CopyBufferRegion(const Buffer& src, const Buffer& dst, size_t size, uint32_t srcOffset = 0, uint32_t dstOffset = 0);
		void CopyToImage(const Buffer& src, const Image& dst, uint32_t srcOffset = 0);
		void BlitImage(const Image& src, const Image& dst, uVec2 srcSize = { 0, 0 });

		void BeginRendering(const uVec2& region, Span<Image> colorAttachments, const Image* depthAttachment = nullptr);
		void EndRendering();
		void Barrier(PipelineStage src, PipelineStage dst);
		void Barrier(const Buffer& buffer, PipelineStage src, PipelineStage dst);

		void BindShaders(Span<Shader> shaders);
		void BindIndexBuffer(const Buffer& buffer, IndexType type, uint64_t offset = 0);
		void PushConstant(const void* data, size_t size, uint32_t offset = 0);
		template<typename T> void PushConstant(const T& data, uint32_t offset = 0) { PushConstant(&data, sizeof(T), offset); };

		void SetDefaultState();
		void SetViewport(const iVec2& size);
		void SetScissor(const iVec2& size, const iVec2& offset = { 0, 0 });
		void EnableDepthTest(bool writeEnabled, CompareOperation op);
		void EnableColorBlend(BlendFactor srcBlend, BlendFactor dstBlend, BlendOp blendOp, BlendFactor srcBlendAlpha, BlendFactor dstBlendAlpha, BlendOp blendAlpha);

		void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
		void DrawIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawCount, uint32_t stride);
		void DrawIndirectCount(const Buffer& buffer, uint64_t offset, const Buffer& countBuffer, uint64_t countOffset, uint32_t maxDrawCount, uint32_t stride);

		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance);
		void DrawIndexedIndirect(const Buffer& buffer, uint64_t offset, uint32_t drawCount, uint32_t stride);
		void DrawIndexedIndirectCount(const Buffer& buffer, uint64_t offset, const Buffer& countBuffer, uint64_t countOffset, uint32_t maxDrawCount, uint32_t stride);

		void Dispatch(uint32_t workX, uint32_t workY, uint32_t workZ);
		void DispatchIndirect(const Buffer& buffer, uint64_t offset);
	};

}