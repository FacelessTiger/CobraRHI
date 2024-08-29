#include "DirectXRHI.h"

#include <dxgidebug.h>

extern "C" { __declspec(dllexport) extern const uint32_t D3D12SDKVersion = 614; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

namespace Cobra {

	GraphicsContext::GraphicsContext(const ContextConfig& config)
	{
		pimpl = std::make_shared<Impl<GraphicsContext>>(config);
		pimpl->TransferManager = new TransferManager(*this, false);
	}

	GraphicsContext::~GraphicsContext()
	{
		delete pimpl->TransferManager;
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

		auto adapter = PickAdapter();
		CreateDeviceAndQueues(adapter);
		SetupBindless();

		DX_CHECK(D3D12MA::CreateAllocator(PtrTo(D3D12MA::ALLOCATOR_DESC {
			.pDevice = Device.Get(),
			.pAdapter = adapter.Get()
		}), &Allocator), "Failed to create D3D12MA allocator");
	}

	Impl<GraphicsContext>::~Impl()
	{
		GraphicsQueue.WaitIdle();
		TransferQueue.WaitIdle();

		GraphicsQueue.pimpl->Destroy();
		TransferQueue.pimpl->Destroy();
		
		Allocator.Reset();
		BindlessDescriptorHeap.Reset();
		BindlessSamplerHeap.Reset();
		BindlessRootSignature.Reset();

		GraphicsPipelines.clear();
		ComputePipelines.clear();

		Device.Reset();
		
		if (Config.Debug)
		{
			ComPtr<IDXGIDebug1> dxgiDebug;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
				dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		}
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

	void Impl<GraphicsContext>::SetupBindless()
	{
		CD3DX12_ROOT_PARAMETER1 pushConstants[2];
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC descVersion;
		pushConstants[0].InitAsConstants(128 / 4, 0);
		pushConstants[1].InitAsConstants(2, 1);
		descVersion.Init_1_1(2, pushConstants, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED);

		ComPtr<ID3DBlob> signature, error;
		DX_CHECK(D3DX12SerializeVersionedRootSignature(&descVersion, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error), "Failed to serialize root signature");
		DX_CHECK(Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&BindlessRootSignature)), "Failed to create root signature");

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 1'000'000,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		};
		DX_CHECK(Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&BindlessDescriptorHeap)), "Failed to create bindless descriptor heap");

		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		heapDesc.NumDescriptors = 2000;
		DX_CHECK(Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&BindlessSamplerHeap)), "Failed to create bindless sampler heap");
	}

}