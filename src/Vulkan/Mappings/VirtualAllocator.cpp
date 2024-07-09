#include "VulkanRHI.h"

namespace Cobra {

	VirtualAllocator::VirtualAllocator(size_t size)
	{
		pimpl = std::make_unique<Impl<VirtualAllocator>>(size);
	}

	VirtualAllocation::VirtualAllocation() { }
	VirtualAllocation::~VirtualAllocation()  { }
	VirtualAllocation::VirtualAllocation(VirtualAllocation&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; Offset = other.Offset; other.Offset = 0; }
	VirtualAllocation& VirtualAllocation::operator=(VirtualAllocation&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; Offset = other.Offset; other.Offset = 0; return *this; }

	VirtualAllocator::~VirtualAllocator() 
	{
		vmaClearVirtualBlock(pimpl->Block);
		vmaDestroyVirtualBlock(pimpl->Block);
	}
	VirtualAllocator::VirtualAllocator(VirtualAllocator&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	VirtualAllocator& VirtualAllocator::operator=(VirtualAllocator&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	VirtualAllocation VirtualAllocator::Allocate(size_t size, uint32_t alignment)
	{
		VmaVirtualAllocationCreateInfo info = { .size = size, .alignment = alignment };

		VirtualAllocation ret;
		VmaVirtualAllocation allocation;
		vmaVirtualAllocate(pimpl->Block, &info, &allocation, &ret.Offset);

		ret.pimpl = std::make_unique<Impl<VirtualAllocation>>(allocation);
		return ret;
	}

	void VirtualAllocator::Free(const VirtualAllocation& allocation)
	{
		vmaVirtualFree(pimpl->Block, allocation.pimpl->Allocation);
	}

	Impl<VirtualAllocation>::Impl(VmaVirtualAllocation allocation)
		: Allocation(allocation)
	{ }

	Impl<VirtualAllocator>::Impl(size_t size)
	{
		VmaVirtualBlockCreateInfo info = { .size = size };
		vmaCreateVirtualBlock(&info, &Block);
	}

}