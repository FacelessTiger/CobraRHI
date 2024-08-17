#include "VulkanRHI.h"

namespace Cobra {

	Queue::Queue() { }
	Queue::Queue(Queue&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Queue& Queue::operator=(Queue&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	SyncPoint Queue::Acquire(const Swapchain& swapchain)
	{
		if (swapchain.pimpl->Dirty) swapchain.pimpl->Recreate();
		VK_CHECK(vkAcquireNextImageKHR(pimpl->Context->Device, swapchain.pimpl->Swapchain, UINT64_MAX, swapchain.pimpl->Semaphores[swapchain.pimpl->SemaphoreIndex], nullptr, &swapchain.pimpl->ImageIndex), "Couldn't acquire swapchain image");

		// Props to @darianopolis for this empty-submit idea
		VK_CHECK(vkQueueSubmit2(pimpl->Queue, 1, PtrTo(VkSubmitInfo2 {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = 1,
			.pWaitSemaphoreInfos = PtrTo(VkSemaphoreSubmitInfo {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = swapchain.pimpl->Semaphores[swapchain.pimpl->SemaphoreIndex],
				.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
			}),
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = PtrTo(VkSemaphoreSubmitInfo {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = pimpl->Fence.pimpl->TimelineSemaphore,
				.value = pimpl->Fence.Advance(),
				.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
			})
		}), nullptr), "Swapchain binary -> timeline adapter submit failed");
		swapchain.pimpl->SemaphoreIndex = (swapchain.pimpl->SemaphoreIndex + 1) % swapchain.pimpl->Semaphores.size();

		return SyncPoint(pimpl->Fence);
	}

	SyncPoint Queue::Submit(CommandList& cmd, Span<SyncPoint> wait)
	{
		VK_CHECK(vkEndCommandBuffer(cmd.pimpl->CommandBuffer), "Couldn't end command buffer");
		std::vector<VkSemaphoreSubmitInfo> waitInfos(wait.size());

		for (int i = 0; i < wait.size(); i++)
		{
			waitInfos[i] = {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = wait[i].pFence->pimpl->TimelineSemaphore,
				.value = wait[i].GetValue(),
				.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
			};
		}

		VK_CHECK(vkQueueSubmit2(pimpl->Queue, 1, PtrTo(VkSubmitInfo2 {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = (uint32_t)waitInfos.size(),
			.pWaitSemaphoreInfos = waitInfos.data(),
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = PtrTo(VkCommandBufferSubmitInfo {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
				.commandBuffer = cmd.pimpl->CommandBuffer
			}),
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = PtrTo(VkSemaphoreSubmitInfo {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = pimpl->Fence.pimpl->TimelineSemaphore,
				.value = pimpl->Fence.Advance(),
				.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
			})
		}), nullptr), "CommandList queue submit failed");

		if (cmd.pimpl->Recording)
			pimpl->Allocators.push_back(std::move(cmd.pimpl->Allocator));
		cmd.pimpl->Recording = false;
		pimpl->PendingCommandLists.push_back({ std::move(cmd), pimpl->Fence.GetPendingValue() });

		return SyncPoint(pimpl->Fence);
	}

	void Queue::Present(const Swapchain& swapchain, Span<SyncPoint> wait)
	{
		std::vector<VkSemaphoreSubmitInfo> waitInfos(wait.size());
		for (int i = 0; i < wait.size(); i++)
		{
			waitInfos[i] = {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = wait[i].pFence->pimpl->TimelineSemaphore,
				.value = wait[i].GetValue(),
				.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
			};
		}

		VK_CHECK(vkQueueSubmit2(pimpl->Queue, 1, PtrTo(VkSubmitInfo2 {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = (uint32_t)waitInfos.size(),
			.pWaitSemaphoreInfos = waitInfos.data(),
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = PtrTo(VkSemaphoreSubmitInfo {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = swapchain.pimpl->Semaphores[swapchain.pimpl->SemaphoreIndex],
				.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
			})
		}), nullptr), "Swapchain timeline -> binary adapter submit failed");

		VkSemaphore* binaryWaits = &swapchain.pimpl->Semaphores[swapchain.pimpl->SemaphoreIndex];
		swapchain.pimpl->SemaphoreIndex = (swapchain.pimpl->SemaphoreIndex + 1) % swapchain.pimpl->Semaphores.size();

		VK_CHECK(vkQueuePresentKHR(pimpl->Queue, PtrTo(VkPresentInfoKHR {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = binaryWaits,
			.swapchainCount = 1,
			.pSwapchains = &swapchain.pimpl->Swapchain,
			.pImageIndices = &swapchain.pimpl->ImageIndex
		})), "Swapchain present failed");
	}

	void Queue::WaitIdle() const
	{
		pimpl->Fence.Wait();
	}

	CommandList Queue::Begin() const
	{
		CommandList cmd;

		// Clear pending command lists
		auto currentValue = pimpl->Fence.GetCurrentValue();
		pimpl->Context->DeletionQueues->Flush(currentValue);

		while (!pimpl->PendingCommandLists.empty())
		{
			auto& pending = pimpl->PendingCommandLists.front();
			if (currentValue >= pending.FenceValue)
			{
				vkResetCommandBuffer(pending.CommandList.pimpl->CommandBuffer, 0);

				auto* allocator = pending.CommandList.pimpl->Allocator;
				allocator->AvailableCommandLists.push_back(std::move(pending.CommandList));
				pimpl->PendingCommandLists.pop_front();
			}
			else
			{
				break;
			}
		}

		auto* allocator = pimpl->AcquireCommandAllocator();
		if (allocator->AvailableCommandLists.empty())
		{
			VkCommandBuffer buffer;
			VK_CHECK(vkAllocateCommandBuffers(pimpl->Context->Device, PtrTo(VkCommandBufferAllocateInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool = allocator->CommandPool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = 1
			}), &buffer), "Failed to create command buffer on-demand");

			cmd.pimpl = std::make_unique<Impl<CommandList>>(pimpl->Context, allocator, buffer);
		}
		else
		{
			cmd = std::move(allocator->AvailableCommandLists.back());
			allocator->AvailableCommandLists.pop_back();
		}
		
		cmd.pimpl->Recording = true;
		VK_CHECK(vkBeginCommandBuffer(cmd.pimpl->CommandBuffer, PtrTo(VkCommandBufferBeginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
		})), "Couldn't begin command buffer");

		if (pimpl->Flags & VK_QUEUE_GRAPHICS_BIT) vkCmdBindDescriptorSets(cmd.pimpl->CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pimpl->Context->BindlessPipelineLayout, 0, 1, &pimpl->Context->BindlessSet, 0, nullptr);
		if (pimpl->Flags & VK_QUEUE_COMPUTE_BIT) vkCmdBindDescriptorSets(cmd.pimpl->CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pimpl->Context->BindlessPipelineLayout, 0, 1, &pimpl->Context->BindlessSet, 0, nullptr);

		return cmd;
	}

	Impl<Queue>::Impl(Impl<GraphicsContext>* context, VkQueue queue, uint32_t queueFamily, VkQueueFlags flags)
		: Context(context), Queue(queue), QueueFamily(queueFamily), Flags(flags)
	{
		Fence.pimpl = std::make_unique<Impl<Cobra::Fence>>(context);
	}

	Impl<Queue>::CommandAllocator* Impl<Queue>::AcquireCommandAllocator()
	{
		CommandAllocator* allocator;
		if (Allocators.empty())
		{
			allocator = new CommandAllocator();
			VK_CHECK(vkCreateCommandPool(Context->Device, PtrTo(VkCommandPoolCreateInfo {
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = QueueFamily
			}), nullptr, &allocator->CommandPool), "Failed to create command pool on-demand");
		}
		else
		{
			allocator = Allocators.back();
			Allocators.pop_back();
		}

		return allocator;
	}

	void Impl<Queue>::Destroy()
	{
		for (auto& allocator : Allocators)
		{
			vkDestroyCommandPool(Context->Device, allocator->CommandPool, nullptr);
			delete allocator;
		}

		vkDestroySemaphore(Context->Device, Fence.pimpl->TimelineSemaphore, nullptr);
	}

}