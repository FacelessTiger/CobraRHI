#pragma once

#include <optional>

namespace Cobra {

	enum class ResourceType
	{
		Sampler,
		Image,
		Shared
	};

	class ResourceHandle
	{
	public:
		ResourceHandle(ResourceType type);
		~ResourceHandle();

		uint32_t GetPendingValue() const;
	private:
		ResourceType m_Type;
		mutable std::optional<uint32_t> m_ID;
	};

}