#include "DirectXRHI.h"

namespace Cobra {

	namespace Utils {

		D3D12_RESOURCE_FLAGS CBImageUsageToD3D12Flags(ImageUsage usage)
		{
			D3D12_RESOURCE_FLAGS ret = D3D12_RESOURCE_FLAG_NONE;

			if (usage & ImageUsage::DepthStencilAttachment)	ret |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			if (usage & ImageUsage::ColorAttachment)		ret |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			if (usage & ImageUsage::Storage)				ret |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			return ret;
		}

	}

	Image::Image(GraphicsContext& context, const uVec2& size, ImageFormat format, ImageUsage usage)
	{
		pimpl = std::make_unique<Impl<Image>>(context, size, format, usage);
	}

	Image::Image(std::shared_ptr<Impl<GraphicsContext>> context)
	{
		pimpl = std::make_unique<Impl<Image>>(context);
	}

	Image::~Image() { }
	Image::Image(Image&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Image& Image::operator=(Image&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void Image::SetDebugName(std::string_view name)
	{
		pimpl->Image->SetName(std::wstring(name.begin(), name.end()).c_str());
	}

	void Image::Set(const void* data) const
	{
		// TODO: figure out per format
		pimpl->Context->TransferManager->Transfer(*this, { (std::byte*)data, pimpl->Size.x * pimpl->Size.y * 4 });
	}

	void Image::Transition(ImageLayout layout) const
	{
		D3D12_BARRIER_LAYOUT dxLayout = Utils::CBImageLayoutToDirectX(layout);
		auto& queue = pimpl->Context->TransferManager->GetQueue();

		auto cmd = queue.Begin();
		pimpl->TransitionLayout(cmd.pimpl->CommandList, dxLayout, D3D12_BARRIER_SYNC_ALL);
		queue.Submit(cmd, {}).Wait();
	}

	uVec2 Image::GetSize() const
	{
		return pimpl->Size;
	}

	uint32_t Image::GetHandle() const { return pimpl->RHandle.GetPendingValue(); }

	Impl<Image>::Impl(std::shared_ptr<Impl<GraphicsContext>> context)
		: Context(context)
	{ }

	Impl<Image>::Impl(GraphicsContext& context, const uVec2& size, ImageFormat format, ImageUsage usage)
		: Context(context.pimpl), Size(size), Format(format), Usage(usage)
	{
		DX_CHECK(Context->Allocator->CreateResource3(PtrTo(D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_DEFAULT }),
			PtrTo(CD3DX12_RESOURCE_DESC1::Tex2D(Utils::CBImageFormatToDirectX(format), size.x, size.y, 1, 1, 1, 0, Utils::CBImageUsageToD3D12Flags(Usage))),
			D3D12_BARRIER_LAYOUT_UNDEFINED,
			nullptr,
			0, nullptr,
			&Allocation, IID_PPV_ARGS(&Image)), "Failed to allocate image");

		UpdateDescriptor();
	}

	Impl<Image>::~Impl()
	{
		
	}

	void Impl<Image>::TransitionLayout(ComPtr<ID3D12GraphicsCommandList7> cmd, D3D12_BARRIER_LAYOUT newLayout, D3D12_BARRIER_SYNC newStage)
	{
		if (Layout == newLayout) return;

		D3D12_BARRIER_ACCESS dstAccess = D3D12_BARRIER_ACCESS_COMMON;
		switch (newLayout)
		{
			case D3D12_BARRIER_LAYOUT_RENDER_TARGET: dstAccess = D3D12_BARRIER_ACCESS_RENDER_TARGET; break;
			case (D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ | D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE): dstAccess = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ | D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE; break;
			case D3D12_BARRIER_LAYOUT_PRESENT: dstAccess = D3D12_BARRIER_ACCESS_COMMON; break;
			case D3D12_BARRIER_LAYOUT_COPY_SOURCE: dstAccess = D3D12_BARRIER_ACCESS_COPY_SOURCE; break;
			case D3D12_BARRIER_LAYOUT_COPY_DEST: dstAccess = D3D12_BARRIER_ACCESS_COPY_DEST; break;
		}

		cmd->Barrier(1, PtrTo(D3D12_BARRIER_GROUP {
			.Type = D3D12_BARRIER_TYPE_TEXTURE,
			.NumBarriers = 1,
			.pTextureBarriers = PtrTo(D3D12_TEXTURE_BARRIER {
				.SyncBefore = Stage,
				.SyncAfter = newStage,
				.AccessBefore = Access,
				.AccessAfter = dstAccess,
				.LayoutBefore = Layout,
				.LayoutAfter = newLayout,
				.pResource = Image.Get(),
				.Subresources = {
					.IndexOrFirstMipLevel = 0xffffffff
				}
			})
		}));

		Layout = newLayout;
		Stage = newStage;
		Access = dstAccess;
	}

	void Impl<Image>::UpdateDescriptor()
	{
		DXGI_FORMAT d3d12Format = Utils::CBImageFormatToDirectX(Format);

		if (Usage & ImageUsage::ColorAttachment)
		{
			DX_CHECK(Context->Device->CreateDescriptorHeap(PtrTo(D3D12_DESCRIPTOR_HEAP_DESC{
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
				.NumDescriptors = 1
			}), IID_PPV_ARGS(&Heap)), "Failed to create descriptor heap for ColorAttachment image");
			CpuHandle = Heap->GetCPUDescriptorHandleForHeapStart();

			Context->Device->CreateRenderTargetView(Image.Get(), PtrTo(D3D12_RENDER_TARGET_VIEW_DESC{
				.Format = d3d12Format,
				.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D
			}), CpuHandle);
		}

		if (Usage & ImageUsage::Sampled)
		{

		}

		if (Usage & ImageUsage::Storage)
		{
			
		}
	}

}