#pragma once

#include <CobraRHI.h>
#include <volk.h>
#include <vk_mem_alloc.h>

#include <vector>
#include <unordered_map>

namespace Cobra {

	struct BufferAllocation
	{
		VkBuffer Buffer;
		VmaAllocation Allocation;
		VmaAllocationInfo Info;
	};

	struct ImageAllocation
	{
		VkImage Image;
		VmaAllocation Allocation;
	};

	class DeletionQueue
	{
	public:
		DeletionQueue(Impl<GraphicsContext>& context);

		void Push(uint64_t pendingValue, VkSemaphore semaphore) { m_Semaphores.insert({ pendingValue, semaphore }); }
		void Push(uint64_t pendingValue, VkImageView imageView) { m_ImageViews.insert({ pendingValue, imageView }); }
		void Push(uint64_t pendingValue, BufferAllocation buffer) { m_Buffers.insert({ pendingValue, buffer }); }
		void Push(uint64_t pendingValue, ImageAllocation image) { m_Images.insert({ pendingValue, image }); }
		void Push(uint64_t pendingValue, VkShaderEXT shader) { m_Shaders.insert({ pendingValue, shader }); }
		void Push(uint64_t pendingValue, VkShaderModule module) { m_Modules.insert({ pendingValue, module }); }
		void Push(uint64_t pendingValue, VkSampler sampler) { m_Samplers.insert({ pendingValue, sampler }); }

		void Flush(uint64_t currentValue);
	private:
		template<typename T, typename F>
		void FlushInternal(std::unordered_multimap<uint64_t, T>& map, uint64_t currentValue, F f)
		{
			for (auto it = map.cbegin(); it != map.cend();)
			{
				if (currentValue >= it->first)
				{
					f(it->second);
					map.erase(it++);
				}
				else
				{
					++it;
				}
			}
		}
	private:
		Impl<GraphicsContext>& m_Context;

		std::unordered_multimap<uint64_t, VkSemaphore> m_Semaphores;
		std::unordered_multimap<uint64_t, VkImageView> m_ImageViews;
		std::unordered_multimap<uint64_t, BufferAllocation> m_Buffers;
		std::unordered_multimap<uint64_t, ImageAllocation> m_Images;
		std::unordered_multimap<uint64_t, VkShaderEXT> m_Shaders;
		std::unordered_multimap<uint64_t, VkShaderModule> m_Modules;
		std::unordered_multimap<uint64_t, VkSampler> m_Samplers;
	};

}