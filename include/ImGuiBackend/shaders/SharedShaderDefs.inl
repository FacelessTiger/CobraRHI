#pragma once

#include <Slang/Cobra.slang>

namespace ImGui::Slang {

	struct Vertex
	{
		float2 position;
		float2 uv;
		uint32_t color;
	};

	struct Push
	{
		const Ptr<Vertex> vertices;
		float2 scale;
		float2 translate;
		Cobra::ImageHandle<float4> image;
	};

}