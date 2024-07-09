#include "DeletionQueue.h"
#include "../Mappings/VulkanRHI.h"

#include <iostream>

namespace Cobra {

	DeletionQueue::DeletionQueue(Impl<GraphicsContext>& context)
		: m_Context(context)
	{ }

	void DeletionQueue::Flush(uint64_t currentValue)
	{
		FlushInternal(m_Semaphores, currentValue, [&](VkSemaphore semaphore) { vkDestroySemaphore(m_Context.Device, semaphore, nullptr); });
		FlushInternal(m_ImageViews, currentValue, [&](VkImageView view) { vkDestroyImageView(m_Context.Device, view, nullptr); });
		FlushInternal(m_Buffers, currentValue, [&](BufferAllocation buffer) { vmaDestroyBuffer(m_Context.Allocator, buffer.Buffer, buffer.Allocation); });
		FlushInternal(m_Images, currentValue, [&](ImageAllocation image) { vmaDestroyImage(m_Context.Allocator, image.Image, image.Allocation); });
		FlushInternal(m_Shaders, currentValue, [&](VkShaderEXT shader) { vkDestroyShaderEXT(m_Context.Device, shader, nullptr); });
		FlushInternal(m_Modules, currentValue, [&](VkShaderModule module) { vkDestroyShaderModule(m_Context.Device, module, nullptr); });
		FlushInternal(m_Samplers, currentValue, [&](VkSampler sampler) { vkDestroySampler(m_Context.Device, sampler, nullptr); });
	}

}