#pragma once

namespace Slang {

#ifdef __cplusplus
	#include <CobraRHI.h>

	using float2 = Cobra::Vec2;
	using float3 = Cobra::Vec3;
	using float4 = Cobra::Vec4;
#endif

	struct Vertex
	{
		float2 position;
		float2 uv;
		uint32_t color;
	};

	struct DrawPush
	{
		const Vertex* vertices;
		float2 scale;
		float2 translate;
		uint32_t image;
	};

}