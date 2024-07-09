#pragma once

#include "Core/Core.h"

namespace Cobra {

	class VirtualAllocation
	{
	public:
		std::unique_ptr<Impl<VirtualAllocation>> pimpl;
		uint64_t Offset;
	public:
		VirtualAllocation();
		virtual ~VirtualAllocation();

		VirtualAllocation(const VirtualAllocation&) = delete;
		VirtualAllocation& operator=(VirtualAllocation& other) = delete;

		VirtualAllocation(VirtualAllocation&& other) noexcept;
		VirtualAllocation& operator=(VirtualAllocation&& other) noexcept;
	};

	class VirtualAllocator
	{
	public:
		std::unique_ptr<Impl<VirtualAllocator>> pimpl;
	public:
		VirtualAllocator(size_t size);
		virtual ~VirtualAllocator();

		VirtualAllocator(const VirtualAllocator&) = delete;
		VirtualAllocator& operator=(VirtualAllocator& other) = delete;

		VirtualAllocator(VirtualAllocator&& other) noexcept;
		VirtualAllocator& operator=(VirtualAllocator&& other) noexcept;

		VirtualAllocation Allocate(size_t size, uint32_t alignment = 0);
		void Free(const VirtualAllocation& allocation);
	};

}