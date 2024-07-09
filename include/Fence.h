#pragma once

#include "Core/Core.h"

namespace Cobra {

	class GraphicsContext;

	inline constexpr uint64_t InvalidFenceValue = ~0ull;

	class Fence
	{
	public:
		std::unique_ptr<Impl<Fence>> pimpl;
	public:
		Fence(const Fence&) = delete;
		Fence& operator=(Fence& other) = delete;

		Fence(Fence&& other) noexcept;
		Fence& operator=(Fence&& other) noexcept;

		void Wait(uint64_t value = InvalidFenceValue) const;
		uint64_t Advance();

		uint64_t GetPendingValue() const;
		uint64_t GetCurrentValue() const;
	private:
		friend Impl<Queue>;
		Fence();
	};

	struct SyncPoint
	{
		const Fence* pFence = nullptr;
		uint64_t Value = InvalidFenceValue;

		SyncPoint() = default;
		SyncPoint(const Fence& fence)
			: pFence(&fence), Value(fence.GetPendingValue())
		{ }

		void Wait() const { if (pFence) { pFence->Wait(Value); } }
		uint64_t GetValue() const { return (Value == InvalidFenceValue && pFence) ? pFence->GetPendingValue() : Value; }
	};

}