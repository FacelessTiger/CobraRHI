#include "DirectXRHI.h"

#include <iostream>

namespace Cobra {

	Buffer::Buffer(GraphicsContext& context, size_t size, BufferUsage usage, BufferFlags flags)
	{
		pimpl = std::make_unique<Impl<Buffer>>(context, size, usage, flags);
	}

	Buffer::~Buffer() { }
	Buffer::Buffer(Buffer&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Buffer& Buffer::operator=(Buffer&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void Buffer::SetDebugName(std::string_view name)
	{
		pimpl->Buffer->SetName(std::wstring(name.begin(), name.end()).c_str());
	}

	size_t Buffer::GetSize() const
	{
		return pimpl->Size;
	}

	std::byte* Buffer::GetHostAddress() const
	{
		return pimpl->HostAddress;
	}

	uint64_t Buffer::GetDeviceAddress() const
	{
		return (uint64_t)pimpl->RHandle.GetPendingValue() << 32;
	}

	Impl<Buffer>::Impl(GraphicsContext& context, size_t size, BufferUsage usage, BufferFlags flags)
		: Context(context.pimpl), Size(size)
	{
		D3D12_HEAP_TYPE heapType;
		if (flags & BufferFlags::DeviceLocal)
		{
			if (flags & BufferFlags::Mapped) heapType = D3D12_HEAP_TYPE_GPU_UPLOAD;
			else heapType = D3D12_HEAP_TYPE_DEFAULT;
		}
		else
		{
			if (usage & BufferUsage::TransferDst) heapType = D3D12_HEAP_TYPE_READBACK;
			else heapType = D3D12_HEAP_TYPE_UPLOAD;
		}

		DX_CHECK(Context->Allocator->CreateResource3(PtrTo(D3D12MA::ALLOCATION_DESC{ .HeapType = heapType }), 
			PtrTo(CD3DX12_RESOURCE_DESC1::Buffer(size, D3D12_RESOURCE_FLAG_NONE)), 
			D3D12_BARRIER_LAYOUT_UNDEFINED,
			nullptr,
			0, nullptr,
			&Allocation, IID_PPV_ARGS(&Buffer)), "Failed to allocate buffer");

		if (flags & BufferFlags::Mapped)
			DX_CHECK(Buffer->Map(0, PtrTo(D3D12_RANGE {}), (void**)&HostAddress), "Failed to map buffer");

		if (usage & BufferUsage::StorageBuffer)
		{
			uint32_t offsetSize = Context->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			Context->Device->CreateShaderResourceView(Buffer.Get(), PtrTo(D3D12_SHADER_RESOURCE_VIEW_DESC{
				.Format = DXGI_FORMAT_R32_TYPELESS,
				.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Buffer = {
					.NumElements = (UINT)size / 4,
					.Flags = D3D12_BUFFER_SRV_FLAG_RAW
				}
			}), CD3DX12_CPU_DESCRIPTOR_HANDLE(Context->BindlessDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), RHandle.GetPendingValue(), offsetSize));

			Context->Device->CreateUnorderedAccessView(Buffer.Get(), nullptr, PtrTo(D3D12_UNORDERED_ACCESS_VIEW_DESC{
				.Format = DXGI_FORMAT_R32_TYPELESS,
				.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
				.Buffer = {
					.NumElements = (UINT)size / 4,
					.Flags = D3D12_BUFFER_UAV_FLAG_RAW
				}
			}), CD3DX12_CPU_DESCRIPTOR_HANDLE(Context->BindlessDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), RWHandle.GetPendingValue(), offsetSize));
		}
	}

	Impl<Buffer>::~Impl()
	{

	}

}