#include "VulkanRHI.h"

namespace Cobra {

	Fence::Fence() { }
	Fence::Fence(Fence&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Fence& Fence::operator=(Fence&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void Fence::Wait(uint64_t value) const
	{
		if (value == InvalidFenceValue)
			value = pimpl->LastSubmittedValue;

		if (pimpl->LastSeenValue >= value)
			return;

		vkWaitSemaphores(pimpl->Context->Device, PtrTo(VkSemaphoreWaitInfo {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &pimpl->TimelineSemaphore,
			.pValues = &value
		}), UINT64_MAX);

		pimpl->LastSeenValue = value;
	}

	uint64_t Fence::Advance()
	{
		return ++pimpl->LastSubmittedValue;
	}

	uint64_t Fence::GetPendingValue() const
	{
		return pimpl->LastSubmittedValue;
	}

	uint64_t Fence::GetCurrentValue() const
	{
		if (pimpl->LastSeenValue >= pimpl->LastSubmittedValue)
			return pimpl->LastSeenValue;

		vkGetSemaphoreCounterValue(pimpl->Context->Device, pimpl->TimelineSemaphore, &pimpl->LastSeenValue);
		return pimpl->LastSeenValue;
	}

	Impl<Fence>::Impl(Impl<GraphicsContext>* context)
		: Context(context)
	{
		VK_CHECK(vkCreateSemaphore(Context->Device, PtrTo(VkSemaphoreCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = PtrTo(VkSemaphoreTypeCreateInfo {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
				.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
				.initialValue = LastSubmittedValue
			})
		}), nullptr, &TimelineSemaphore), "Failed to create Fence");
	}

}