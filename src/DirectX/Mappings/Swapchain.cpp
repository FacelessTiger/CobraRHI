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
		throw std::runtime_error("Not yet implemented");
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
	}

}