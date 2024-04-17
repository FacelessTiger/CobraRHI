#include "VulkanRHI.h"

namespace Cobra {

	Queue::Queue(GraphicsContext& context, QueueType type)
	{
		pimpl = std::make_unique<Impl<Queue>>(context, type);
	}

	Queue::~Queue() { }
	Queue::Queue(Queue&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Queue& Queue::operator=(Queue&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void Queue::Acquire(const Swapchain& swapchain, Fence& fence)
	{
		if (swapchain.pimpl->Dirty) swapchain.pimpl->Recreate();
		VkCheck(pimpl->Context->Callback, vkAcquireNextImageKHR(pimpl->Context->Device, swapchain.pimpl->Swapchain, UINT64_MAX, swapchain.pimpl->Semaphores[swapchain.pimpl->SemaphoreIndex], nullptr, &swapchain.pimpl->ImageIndex));

		VkSemaphoreSubmitInfo waitInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = swapchain.pimpl->Semaphores[swapchain.pimpl->SemaphoreIndex],
			//.stageMask = VK_PIPELINE_STAGE_2_NONE
		};

		VkSemaphoreSubmitInfo signalInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = fence.pimpl->TimelineSemaphore,
			.value = ++fence.pimpl->Value,
		};

		VkSubmitInfo2 submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = 1,
			.pWaitSemaphoreInfos = &waitInfo,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &signalInfo
		};
		VkCheck(pimpl->Context->Callback, vkQueueSubmit2(pimpl->Queue, 1, &submitInfo, nullptr));
	}

	void Queue::Submit(const CommandList& cmd, Fence& wait, Fence& signal)
	{
		VkCheck(pimpl->Context->Callback, vkEndCommandBuffer(cmd.pimpl->CommandBuffer));

		VkSemaphoreSubmitInfo waitInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = wait.pimpl->TimelineSemaphore,
			.value = wait.pimpl->Value,
			//.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};

		VkSemaphoreSubmitInfo signalInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = signal.pimpl->TimelineSemaphore,
			.value = ++signal.pimpl->Value
		};

		VkCommandBufferSubmitInfo cmdInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = cmd.pimpl->CommandBuffer
		};

		VkSubmitInfo2 submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = 1,
			.pWaitSemaphoreInfos = &waitInfo,
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cmdInfo,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &signalInfo,
		};
		VkCheck(pimpl->Context->Callback, vkQueueSubmit2(pimpl->Queue, 1, &submitInfo, nullptr));
	}

	void Queue::Present(const Swapchain& swapchain, Fence& wait)
	{
		VkSemaphoreSubmitInfo waitInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = wait.pimpl->TimelineSemaphore,
			.value = wait.pimpl->Value,
			//.stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};

		VkSemaphoreSubmitInfo signalInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = swapchain.pimpl->Semaphores[swapchain.pimpl->SemaphoreIndex]
		};

		VkSubmitInfo2 submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = 1,
			.pWaitSemaphoreInfos = &waitInfo,
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &signalInfo
		};
		VkCheck(pimpl->Context->Callback, vkQueueSubmit2(pimpl->Queue, 1, &submitInfo, nullptr));

		VkSemaphore* binaryWaits = &swapchain.pimpl->Semaphores[swapchain.pimpl->SemaphoreIndex];
		swapchain.pimpl->SemaphoreIndex = (swapchain.pimpl->SemaphoreIndex + 1) % swapchain.pimpl->Semaphores.size();

		VkPresentInfoKHR presentInfo = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = binaryWaits,
			.swapchainCount = 1,
			.pSwapchains = &swapchain.pimpl->Swapchain,
			.pImageIndices = &swapchain.pimpl->ImageIndex
		};
		VkCheck(pimpl->Context->Callback, vkQueuePresentKHR(pimpl->Queue, &presentInfo));
	}

	Impl<Queue>::Impl(GraphicsContext& context, QueueType type)
		: Context(context.pimpl)
	{
		switch (type)
		{
			case QueueType::Graphics:
			{
				Queue = Context->GraphicsQueue;
				QueueFamily = Context->GraphicsQueueFamily;
				break;
			}
		}
	}

}