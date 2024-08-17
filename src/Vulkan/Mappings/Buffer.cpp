#include "VulkanRHI.h"

#include <iostream>

namespace Cobra {

	namespace Utils {

		static VkBufferUsageFlags GABufferUsageToVulkan(BufferUsage usage)
		{
			VkBufferUsageFlags ret;

			if (usage & BufferUsage::StorageBuffer)		ret |= (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
			if (usage & BufferUsage::TransferSrc)		ret |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			if (usage & BufferUsage::TransferDst)		ret |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			if (usage & BufferUsage::IndexBuffer)		ret |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			if (usage & BufferUsage::IndirectBuffer)	ret |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

			return ret;
		}

	}

	Buffer::Buffer(GraphicsContext& context, size_t size, BufferUsage usage, BufferFlags flags)
	{
		pimpl = std::make_unique<Impl<Buffer>>(context, size, usage, flags);
	}

	Buffer::~Buffer() { }
	Buffer::Buffer(Buffer&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Buffer& Buffer::operator=(Buffer&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void Buffer::SetDebugName(std::string_view name)
	{
		if (pimpl->Context->Config.Debug)
		{
			VK_CHECK(vkSetDebugUtilsObjectNameEXT(pimpl->Context->Device, PtrTo(VkDebugUtilsObjectNameInfoEXT {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_BUFFER,
				.objectHandle = (uint64_t)pimpl->Allocation.Buffer,
				.pObjectName = name.data()
			})), "Failed to set buffer's debug name");
		}
	}

	size_t Buffer::GetSize() const
	{
		return pimpl->Size;
	}

	std::byte* Buffer::GetHostAddress() const
	{
		return (std::byte*)pimpl->Allocation.Info.pMappedData;
	}

	uint64_t Buffer::GetDeviceAddress() const
	{
		return pimpl->Address;
	}

	Impl<Buffer>::Impl(GraphicsContext& context, size_t size, BufferUsage usage, BufferFlags flags)
		: Context(context.pimpl), Size(size)
	{
		VmaAllocationCreateFlags vmaFlags = {};
		if (flags & BufferFlags::Mapped)
		{
			vmaFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
			if (flags & BufferFlags::DeviceLocal) vmaFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			else vmaFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		}

		VkBufferCreateInfo bufferInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = (VkDeviceSize)Size,
			.usage = Utils::GABufferUsageToVulkan(usage),
		};

		VmaAllocationCreateInfo vmaAllocInfo = {
			.flags = vmaFlags,
			.usage = VMA_MEMORY_USAGE_AUTO,
			.requiredFlags = (flags & BufferFlags::DeviceLocal) ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : (VkMemoryPropertyFlags)0
		};

		VK_CHECK(vmaCreateBuffer(Context->Allocator, &bufferInfo, &vmaAllocInfo, &Allocation.Buffer, &Allocation.Allocation, &Allocation.Info), "Failed to create buffer");

		VkBufferDeviceAddressInfo addressInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = Allocation.Buffer
		};
		if (usage & BufferUsage::StorageBuffer) Address = (uint64_t)vkGetBufferDeviceAddress(Context->Device, &addressInfo);
	}

	Impl<Buffer>::~Impl()
	{
		Context->DeletionQueues->Push(Context->GraphicsQueue.pimpl->Fence.GetPendingValue(), Allocation);
	}

}