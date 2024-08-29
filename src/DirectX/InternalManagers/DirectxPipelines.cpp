#include "DirectXPipelines.h"
#include "../Mappings/DirectXRHI.h"

#include <unordered_map>
#include <format>

namespace Cobra {

	ID3D12PipelineState* GetGraphicsPipeline(Impl<CommandList>* cmd, const GraphicsPipelineKey& key)
    {
		auto context = cmd->Context;
		auto pipeline = context->GraphicsPipelines[key];
		if (pipeline) return pipeline.Get();

		auto start = std::chrono::steady_clock::now();

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
			.pRootSignature = context->BindlessRootSignature.Get(),
			.VS = CD3DX12_SHADER_BYTECODE(Impl<Shader>::GetShaderByID(key.Shaders[0]).Blob.Get()),
			.PS = CD3DX12_SHADER_BYTECODE(Impl<Shader>::GetShaderByID(key.Shaders[4]).Blob.Get()),
			.BlendState = {
				.IndependentBlendEnable = true
			},
			.SampleMask = (UINT)~0,
			.RasterizerState = {
				.FillMode = D3D12_FILL_MODE_SOLID,
				.CullMode = D3D12_CULL_MODE_NONE
			},
			.DepthStencilState = {
				.DepthEnable = false
			},
			.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			.NumRenderTargets = key.ColorAttachmentCount,
			.DSVFormat = Utils::CBImageFormatToDirectX(key.DepthAttachment),
			.SampleDesc = {
				.Count = 1,
				.Quality = 0
			}
		};

		for (uint32_t i = 0; i < key.ColorAttachmentCount; i++)
		{
			desc.BlendState.RenderTarget[i] = {
				.BlendEnable = key.BlendEnable,
				.SrcBlend = Utils::CBBlendFactorToDirectX(key.SrcBlend),
				.DestBlend = Utils::CBBlendFactorToDirectX(key.DstBlend),
				.BlendOp = Utils::CBBlendOpToDirectX(key.BlendOp),
				.SrcBlendAlpha = Utils::CBBlendFactorToDirectX(key.SrcBlendAlpha),
				.DestBlendAlpha = Utils::CBBlendFactorToDirectX(key.DstBlendAlpha),
				.BlendOpAlpha = Utils::CBBlendOpToDirectX(key.BlendAlpha),
				.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
			};

			desc.RTVFormats[i] = Utils::CBImageFormatToDirectX(key.ColorAttachments[i]);
		}

		context->Device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline));
		context->GraphicsPipelines[key] = pipeline;

		std::chrono::duration<float> duration = std::chrono::steady_clock::now() - start;
		if (context->Config.Trace)
			context->Config.Callback(std::format("Compiled graphics pipeline in {} microseconds", std::chrono::duration_cast<std::chrono::microseconds>(duration).count()).c_str(), MessageSeverity::Trace);

		return pipeline.Get();
    }

	ID3D12PipelineState* GetComputePipeline(Impl<CommandList>* cmd, const ComputePipelineKey& key)
	{
		auto context = cmd->Context;
		auto pipeline = context->ComputePipelines[key];
		if (pipeline) return pipeline.Get();

		context->Device->CreateComputePipelineState(PtrTo(D3D12_COMPUTE_PIPELINE_STATE_DESC {
			.pRootSignature = context->BindlessRootSignature.Get(),
			.CS = CD3DX12_SHADER_BYTECODE(Impl<Shader>::GetShaderByID(key.Shader).Blob.Get())
		}), IID_PPV_ARGS(&pipeline));
		context->ComputePipelines[key] = pipeline;

		return pipeline.Get();
	}

}