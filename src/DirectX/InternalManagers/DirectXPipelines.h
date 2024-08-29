#pragma once

#include "PipelineKeys.h"

#include <d3d12.h>

namespace Cobra {

	ID3D12PipelineState* GetGraphicsPipeline(Impl<CommandList>* cmd, const GraphicsPipelineKey& key);
	ID3D12PipelineState* GetComputePipeline(Impl<CommandList>* cmd, const ComputePipelineKey& key);

}