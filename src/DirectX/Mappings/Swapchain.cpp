#include "DirectXRHI.h"

namespace Cobra {

	Swapchain::Swapchain(GraphicsContext& context, void* window, uVec2 size, bool enableVsync)
	{
		pimpl = std::make_unique<Impl<Swapchain>>(context, window, size, enableVsync);
	}

	Swapchain::~Swapchain() { }
	Swapchain::Swapchain(Swapchain&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Swapchain& Swapchain::operator=(Swapchain&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void Swapchain::Resize(uVec2 newSize)
	{

	}

	Image& Swapchain::GetCurrent()
	{
		return pimpl->Images[pimpl->ImageIndex];
	}

	uVec2 Swapchain::GetSize() const
	{
		return pimpl->Size;
	}

	Impl<Swapchain>::Impl(GraphicsContext& context, void* window, uVec2 size, bool enableVsync)
		: Context(context.pimpl), Size(size), EnableVsync(enableVsync)
	{
		ComPtr<IDXGIFactory4> factory;
		DX_CHECK(CreateDXGIFactory2(Context->Config.Debug ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&factory)), "Failed to create swapchain factory");

		ComPtr<IDXGISwapChain1> swapchain1;
		DX_CHECK(factory->CreateSwapChainForHwnd(Context->GraphicsQueue.pimpl->Queue.Get(), (HWND)window, PtrTo(DXGI_SWAP_CHAIN_DESC1 {
			.Width = size.x,
			.Height = size.y,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc = { 1, 0 },
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = 3,
			.Scaling = DXGI_SCALING_STRETCH,
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
			.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
			.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
		}), nullptr, nullptr, &swapchain1), "Failed to create swapchain for window");

		DX_CHECK(factory->MakeWindowAssociation((HWND)window, DXGI_MWA_NO_ALT_ENTER), "Failed to disable Alt+Enter for swapchain");
		DX_CHECK(swapchain1.As(&Swapchain), "Failed to convert swapchain1 to swapchain4");

		DX_CHECK(Context->Device->CreateDescriptorHeap(PtrTo(D3D12_DESCRIPTOR_HEAP_DESC{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			.NumDescriptors = 3
		}), IID_PPV_ARGS(&RTVHeap)), "Failed to create RTV heap for swapchain");

		UpdateRenderTargetViews();
	}

	void Impl<Swapchain>::UpdateRenderTargetViews()
	{
		auto descriptorSize = Context->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(RTVHeap->GetCPUDescriptorHandleForHeapStart());

		Images.clear();
		for (int i = 0; i < 3; i++)
		{
			ComPtr<ID3D12Resource> backBuffer;
			DX_CHECK(Swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)), "Couldn't get swapchain buffer");

			Context->Device->CreateRenderTargetView(backBuffer.Get(), PtrTo(D3D12_RENDER_TARGET_VIEW_DESC{
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D
			}), rtvHandle);

			Image image(Context);
			image.pimpl->Image = backBuffer;
			image.pimpl->CpuHandle = rtvHandle;
			image.pimpl->Layout = D3D12_BARRIER_LAYOUT_UNDEFINED;
			image.pimpl->Format = Cobra::ImageFormat::R8G8B8A8_UNORM;
			image.pimpl->Usage = Cobra::ImageUsage::ColorAttachment;
			image.pimpl->Size = Size;

			Images.push_back(std::move(image));
			rtvHandle.Offset(1, descriptorSize);
		}
	}

}