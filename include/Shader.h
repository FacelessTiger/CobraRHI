#pragma once

#include "Core/Core.h"

#include <string_view>

namespace Cobra {

	class GraphicsContext;

	enum class ShaderStage : uint32_t
	{
		Vertex = 1,
		Pixel = 2,
		Compute = 4,
		Max = 8
	};

	inline ShaderStage operator|(ShaderStage a, ShaderStage b) { return (ShaderStage)((uint32_t)a | (uint32_t)b); };
	inline ShaderStage& operator|=(ShaderStage& a, ShaderStage b) { return a = a | b; };
	inline ShaderStage operator<<(ShaderStage a, uint32_t b) { return (ShaderStage)((uint32_t)a << b); }
	inline ShaderStage& operator<<=(ShaderStage& a, uint32_t b) { return a = a << b; }
	inline bool operator&(ShaderStage a, ShaderStage b) { return (uint32_t)a & (uint32_t)b; };

	class Shader
	{
	public:
		std::unique_ptr<Impl<Shader>> pimpl;
	public:
		Shader(GraphicsContext& context, std::string_view path, ShaderStage stages, std::vector<uint32_t>* outputCode = nullptr);
		Shader(GraphicsContext& context, std::span<const uint32_t> code, ShaderStage stages);
		virtual ~Shader();

		Shader(const Shader&) = delete;
		Shader& operator=(Shader& other) = delete;
		
		Shader(Shader&& other) noexcept;
		Shader& operator=(Shader&& other) noexcept;
	};

}