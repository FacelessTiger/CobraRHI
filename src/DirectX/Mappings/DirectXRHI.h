#pragma once

#include <CobraRHI.h>
#include <Shared.h>

#include "../InternalManagers/DirectXPipelines.h"
#include "../InternalManagers/Utils.h"

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <directx/d3dx12.h>
#include <D3D12MemAlloc.h>

#include <array>
#include <deque>

namespace Cobra {

#define DX_CHECK(function, error) if (FAILED(function)) throw std::runtime_error(error);

	using namespace Microsoft::WRL;

	template <>
	struct Impl<GraphicsContext>
	{
		ContextConfig Config;
		ComPtr<ID3D12Device2> Device;
		ComPtr<D3D12MA::Allocator> Allocator;

		Queue GraphicsQueue, TransferQueue;
		TransferManager* TransferManager;

		ComPtr<ID3D12DescriptorHeap> BindlessDescriptorHeap, BindlessSamplerHeap;
		ComPtr<ID3D12RootSignature> BindlessRootSignature;

		std::unordered_map<GraphicsPipelineKey, ComPtr<ID3D12PipelineState>> GraphicsPipelines;
		std::unordered_map<ComputePipelineKey, ComPtr<ID3D12PipelineState>> ComputePipelines;

		Impl(const ContextConfig& config);
		~Impl();

		ComPtr<IDXGIAdapter4> PickAdapter();
		void CreateDeviceAndQueues(ComPtr<IDXGIAdapter4> adapter);
		void SetupBindless();
	};

	template <>
	struct Impl<Queue>
	{
		ComPtr<ID3D12CommandQueue> Queue;
		D3D12_COMMAND_LIST_TYPE Type;
		Fence Fence;

		Impl<GraphicsContext>* Context;

		struct CommandAllocator
		{
			ComPtr<ID3D12CommandAllocator> CommandAllocator;
			std::vector<CommandList> AvailableCommandLists;
		};

		struct SubmittedCommandList
		{
			CommandList CommandList;
			uint64_t FenceValue;
		};

		std::vector<CommandAllocator*> Allocators;
		std::deque<SubmittedCommandList> PendingCommandLists;

		Impl(Impl<GraphicsContext>* context, D3D12_COMMAND_LIST_TYPE  type);

		CommandAllocator* AcquireCommandAllocator();
		void Destroy();
	};

	template<>
	struct Impl<Swapchain>
	{
		uVec2 Size;
		bool EnableVsync;

		uint32_t ImageIndex = 0;
		std::vector<Image> Images;

		ComPtr<IDXGISwapChain4> Swapchain;
		ComPtr<ID3D12DescriptorHeap> RTVHeap;

		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(GraphicsContext& context, void* window, uVec2 size, bool enableVsync);

		void UpdateRenderTargetViews();
	};

	template <>
	struct Impl<Fence>
	{
		ComPtr<ID3D12Fence> Fence;
		HANDLE Event;
		uint64_t LastSubmittedValue = 0;
		uint64_t LastSeenValue = 0;

		Impl<GraphicsContext>* Context;

		Impl(Impl<GraphicsContext>* context);
	};

	template <>
	struct Impl<Shader>
	{
		struct ShaderData
		{
			ComPtr<ID3DBlob> Blob;

			ShaderStage Stage;
			uint64_t ID;
		};
		std::vector<ShaderData> ShaderStages;

		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(GraphicsContext& context, std::string_view path, ShaderStage stages, std::vector<uint32_t>* outputCode);
		Impl(GraphicsContext& context, std::span<const uint32_t> code, ShaderStage stages);
		~Impl();

		void CreateModule(std::span<const uint8_t> code, ShaderStage stages);

		static  Impl<Shader>::ShaderData& GetShaderByID(uint64_t id);
	};

	template <>
	struct Impl<Image>
	{
		ComPtr<D3D12MA::Allocation> Allocation;
		ComPtr<ID3D12Resource> Image;
		D3D12_BARRIER_LAYOUT Layout = D3D12_BARRIER_LAYOUT_UNDEFINED;
		D3D12_BARRIER_SYNC Stage = D3D12_BARRIER_SYNC_NONE;
		D3D12_BARRIER_ACCESS Access = D3D12_BARRIER_ACCESS_NO_ACCESS;
		ImageFormat Format;
		ImageUsage Usage;
		uVec2 Size;

		// These two are only used for depthstencil and color attachment
		ComPtr<ID3D12DescriptorHeap> Heap;
		D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;

		ResourceHandle RHandle = { ResourceType::Shared };
		ResourceHandle RWHandle = { ResourceType::Shared };
		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(std::shared_ptr<Impl<GraphicsContext>> context);
		Impl(GraphicsContext& context, const uVec2& size, ImageFormat format, ImageUsage usage);
		~Impl();

		void TransitionLayout(ComPtr<ID3D12GraphicsCommandList7> cmd, D3D12_BARRIER_LAYOUT newLayout, D3D12_BARRIER_SYNC newStage);

		void UpdateDescriptor();
	};

	template <>
	struct Impl<Buffer>
	{
		size_t Size;
		std::byte* HostAddress;

		ComPtr<D3D12MA::Allocation> Allocation;
		ComPtr<ID3D12Resource> Buffer;

		ResourceHandle RHandle = { ResourceType::Shared };
		ResourceHandle RWHandle = { ResourceType::Shared };
		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(GraphicsContext& context, size_t size, BufferUsage usage, BufferFlags flags);
		~Impl();
	};

	template <>
	struct Impl<CommandList>
	{
		ComPtr<ID3D12GraphicsCommandList7> CommandList;
		Impl<Queue>::CommandAllocator* Allocator;

		GraphicsPipelineKey GraphicsKey;
		D3D12_CPU_DESCRIPTOR_HANDLE ColorAttachmentHandles[8];
		bool GraphicsStateChanged = false;
		bool Recording = false;

		Impl<GraphicsContext>* Context;

		Impl(Impl<GraphicsContext>* context, Impl<Queue>::CommandAllocator* allocator, ComPtr<ID3D12GraphicsCommandList7> commandList);

		void BindPipelineIfNeeded();
	};

	template <typename T>
	inline T* PtrTo(T&& v) { return &v; }

}