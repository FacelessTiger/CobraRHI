#pragma once

#include "Core/Core.h"

namespace Cobra {

	class GraphicsContext;

	enum class Filter
	{
		Nearest,
		Linear
	};

	class Sampler
	{
	public:
		std::unique_ptr<Impl<Sampler>> pimpl;
	public:
		Sampler(GraphicsContext& context, Filter min, Filter mag);
		virtual ~Sampler();

		Sampler(const Sampler&) = delete;
		Sampler& operator=(Sampler& other) = delete;

		Sampler(Sampler&& other) noexcept;
		Sampler& operator=(Sampler&& other) noexcept;

		uint32_t GetHandle() const;
	};

}