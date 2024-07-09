#pragma once

#include "PipelineKeys.h"

#include <volk.h>

namespace Cobra {

	VkPipeline GetGraphicsPipeline(Impl<CommandList>* cmd, const GraphicsPipelineKey& key);
	VkPipeline GetComputePipeline(Impl<CommandList>* cmd, const ComputePipelineKey& key);

}