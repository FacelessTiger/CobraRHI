#include "VulkanRHI.h"

namespace Cobra {

	namespace Utils {

		static VkBufferUsageFlags GABufferUsageToVulkan(BufferUsage usage)
		{
			VkBufferUsageFlags ret = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

			if (usage & BufferUsage::StorageBuffer)		ret |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
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

	void Buffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		uint32_t bufferSize = size ? size : pimpl->Size; // If user passes a size use it, otherwise use creation size
		memcpy((char*)pimpl->Info.pMappedData + offset, data, bufferSize);
	}

	uint64_t Buffer::GetAddress() const
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

		vmaCreateBuffer(Context->Allocator, &bufferInfo, &vmaAllocInfo, &Buffer, &Allocation, &Info);

		VkBufferDeviceAddressInfo addressInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = Buffer
		};
		Address = (uint64_t)vkGetBufferDeviceAddress(Context->Device, &addressInfo);
	}

	Impl<Buffer>::~Impl()
	{
		// TODO: Push to deletion queue
		
	}

}