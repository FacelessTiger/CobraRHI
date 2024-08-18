#include "DirectXRHI.h"

namespace Cobra {

	Fence::Fence(Fence&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Fence& Fence::operator=(Fence && other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	void Fence::Wait(uint64_t value) const
	{
		if (value == InvalidFenceValue)
			value = pimpl->LastSubmittedValue;

		if (pimpl->LastSeenValue >= value)
			return;

		pimpl->Fence->SetEventOnCompletion(value, pimpl->Event);
		WaitForSingleObject(pimpl->Event, DWORD_MAX);

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

		pimpl->LastSeenValue = pimpl->Fence->GetCompletedValue();
		return pimpl->LastSeenValue;
	}

	Impl<Fence>::Impl(Impl<GraphicsContext>* context)
		: Context(context)
	{
		DX_CHECK(context->Device->CreateFence(LastSubmittedValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)), "Failed to create Fence");
		if (!(Event = CreateEvent(NULL, FALSE, FALSE, NULL))) throw std::runtime_error("Failed to create Fence event");
	}

}