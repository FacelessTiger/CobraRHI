#pragma once

#include <CobraRHI.h>

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include <array>

namespace Cobra {

#define DX_CHECK(function, error) if (FAILED(function)) throw std::runtime_error(error);

	using namespace Microsoft::WRL;

	template <>
	struct Impl<GraphicsContext>
	{
		ContextConfig Config;
		ComPtr<ID3D12Device2> Device;

		Queue GraphicsQueue, TransferQueue;

		Impl(const ContextConfig& config);
		~Impl();

		ComPtr<IDXGIAdapter4> PickAdapter();
		void CreateDeviceAndQueues(ComPtr<IDXGIAdapter4> adapter);
	};

	template <>
	struct Impl<Queue>
	{
		ComPtr<ID3D12CommandQueue> Queue;
		D3D12_COMMAND_LIST_TYPE Type;
		Fence Fence;

		Impl<GraphicsContext>* Context;

		Impl(Impl<GraphicsContext>* context, D3D12_COMMAND_LIST_TYPE  type);
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

	template <typename T>
	inline T* PtrTo(T&& v) { return &v; }

}