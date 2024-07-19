#pragma once

#include "Core/Core.h"

#include <string_view>

namespace Cobra {

	class GraphicsContext;

	class Shader
	{
	public:
		std::unique_ptr<Impl<Shader>> pimpl;
	public:
		Shader(GraphicsContext& context, std::string_view path);
		Shader(GraphicsContext& context, std::span<const uint32_t> code);
		virtual ~Shader();

		Shader(const Shader&) = delete;
		Shader& operator=(Shader& other) = delete;
		
		Shader(Shader&& other) noexcept;
		Shader& operator=(Shader&& other) noexcept;
	};

}