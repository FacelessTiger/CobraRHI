#pragma once

#include <CobraRHI.h>
#include <xxhash.h>

#include <array>
#include <cstdint>

#define CB_PIPELINE_EQUALITY(name)														\
	bool operator==(const name& other) const											\
	{																					\
		return std::memcmp(this, &other, sizeof(name)) == 0;							\
	}
#define CB_PIPELINE_HASH(name)															\
	template<>																			\
	struct hash<Cobra::name>															\
	{																					\
		std::size_t operator()(const Cobra::name& key) const							\
		{																				\
			return (std::size_t)XXH64(&key, sizeof(Cobra::name), 0);					\
		}																				\
	};

namespace Cobra {

	struct GraphicsPipelineKey
	{
		std::array<uint64_t, 5> Shaders;
		std::array<ImageFormat, 8> ColorAttachments;
		ImageFormat DepthAttachment;
		uint32_t ColorAttachmentCount;

		bool BlendEnable = false;
		BlendFactor SrcBlend = BlendFactor::Zero;
		BlendFactor DstBlend = BlendFactor::Zero;
		BlendOp BlendAlpha = BlendOp::Add;
		BlendOp BlendOp = BlendOp::Add;
		BlendFactor SrcBlendAlpha = BlendFactor::Zero;
		BlendFactor DstBlendAlpha = BlendFactor::Zero;

		GraphicsPipelineKey() { std::memset(this, 0, sizeof(GraphicsPipelineKey)); }
		CB_PIPELINE_EQUALITY(GraphicsPipelineKey)
	};

	struct ComputePipelineKey
	{
		uint64_t Shader;

		ComputePipelineKey() { std::memset(this, 0, sizeof(ComputePipelineKey)); }
		CB_PIPELINE_EQUALITY(ComputePipelineKey)
	};

}

namespace std {

	CB_PIPELINE_HASH(GraphicsPipelineKey)
	CB_PIPELINE_HASH(ComputePipelineKey)

}

#undef CB_PIPELINE_EQUALITY
#undef CB_PIPELINE_HASH