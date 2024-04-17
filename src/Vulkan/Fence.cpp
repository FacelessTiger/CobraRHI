#include "VulkanRHI.h"

namespace Cobra {

	Fence::Fence(GraphicsContext& context)
	{
		pimpl = std::make_unique<Impl<Fence>>(context);
	}

	Fence::~Fence() { }
	Fence::Fence(Fence&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Fence& Fence::operator=(Fence&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void Fence::Wait() const
	{
		VkSemaphoreWaitInfo waitInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &pimpl->TimelineSemaphore,
			.pValues = &pimpl->Value
		};

		vkWaitSemaphores(pimpl->Context->Device, &waitInfo, UINT64_MAX);
	}

	Impl<Fence>::Impl(GraphicsContext& context)
		: Context(context.pimpl)
	{
		VkSemaphoreTypeCreateInfo timelineCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
			.pNext = nullptr,
			.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
			.initialValue = Value
		};

		VkSemaphoreCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = &timelineCreateInfo
		};

		VkCheck(Context->Callback, vkCreateSemaphore(Context->Device, &createInfo, nullptr, &TimelineSemaphore));
	}

	Impl<Fence>::~Impl()
	{
		// TODO: Push to deletion queue
		vkDestroySemaphore(Context->Device, TimelineSemaphore, nullptr);
	}

}