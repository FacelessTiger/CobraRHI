#include "DirectXRHI.h"

namespace Cobra {

	GraphicsContext::GraphicsContext(const ContextConfig& config)
	{
		pimpl = std::make_shared<Impl<GraphicsContext>>(config);
	}

	Queue& GraphicsContext::GetQueue(QueueType type)
	{
		switch (type)
		{
			case QueueType::Graphics: return pimpl->GraphicsQueue;
			case QueueType::Transfer: return pimpl->TransferQueue;
			default: std::unreachable();
		}
	}

	Impl<GraphicsContext>::Impl(const ContextConfig& config)
		: Config(config)
	{
		if (config.Debug)
		{
			ComPtr<ID3D12Debug> debugInterface;
			DX_CHECK(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)), "Failed to get debug interface");
			debugInterface->EnableDebugLayer();
		}

		CreateDeviceAndQueues(PickAdapter());
	}

	Impl<GraphicsContext>::~Impl()
	{

	}

	ComPtr<IDXGIAdapter4> Impl<GraphicsContext>::PickAdapter()
	{
		ComPtr<IDXGIFactory6> factory;
		DX_CHECK(CreateDXGIFactory2(Config.Debug ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&factory)), "Failed to create DXGI factory");

		ComPtr<IDXGIAdapter1> adapter1;
		DX_CHECK(factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter1)), "Failed to enum adapters by high performance preference");

		ComPtr<IDXGIAdapter4> adapter4;
		DX_CHECK(adapter1.As(&adapter4), "Failed to convert IDXGIAdapter1 to IDXGIAdapter4");
		return adapter4;
	}

	void Impl<GraphicsContext>::CreateDeviceAndQueues(ComPtr<IDXGIAdapter4> adapter)
	{
		DX_CHECK(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device)), "Failed to create device");

		if (Config.Debug)
		{
			ComPtr<ID3D12InfoQueue> infoQueue;
			DX_CHECK(Device.As(&infoQueue), "Failed to convert device to ID3D12InfoQueue");

			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

			DX_CHECK(infoQueue->PushStorageFilter(PtrTo(D3D12_INFO_QUEUE_FILTER {
				.DenyList = {
					.NumSeverities = 1,
					.pSeverityList = PtrTo(D3D12_MESSAGE_SEVERITY_INFO),
					.NumIDs = 3,
					.pIDList = std::to_array<D3D12_MESSAGE_ID>({
						D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
						D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
						D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
					}).data()
				}
			})), "Failed to set custom debug filter");
		}

		GraphicsQueue.pimpl = std::make_unique<Impl<Queue>>(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
		TransferQueue.pimpl = std::make_unique<Impl<Queue>>(this, D3D12_COMMAND_LIST_TYPE_COPY);
	}

}