#include "VulkanRHI.h"

#include <vector>
#include <array>
#include <iostream>
#include <algorithm>

namespace Cobra {

	static uint64_t s_IDCounter = 1;
	static std::unordered_map<uint64_t, Impl<Shader>::ShaderData> s_ShaderList;

	static void CBStageToVulkan(ShaderStage stage, VkShaderStageFlagBits* currentStage, VkShaderStageFlags* nextStages)
	{
		switch (stage)
		{
			case ShaderStage::Vertex:
			{
				*currentStage = VK_SHADER_STAGE_VERTEX_BIT;
				*nextStages = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			}
			case ShaderStage::Pixel:
			{
				*currentStage = VK_SHADER_STAGE_FRAGMENT_BIT;
				*nextStages = (VkShaderStageFlagBits)0;
				break;
			}
			case ShaderStage::Compute:
			{
				*currentStage = VK_SHADER_STAGE_COMPUTE_BIT;
				*nextStages = (VkShaderStageFlagBits)0;
				break;
			}
		}
	}

	Shader::Shader(GraphicsContext& context, std::string_view path, ShaderStage stages, std::vector<uint32_t>* outputCode)
	{
		pimpl = std::make_unique<Impl<Shader>>(context, path, stages, outputCode);
	}

	Shader::Shader(GraphicsContext& context, std::span<const uint32_t> code, ShaderStage stages)
	{
		pimpl = std::make_unique<Impl<Shader>>(context, code, stages);
	}

	Shader::~Shader() { }
	Shader::Shader(Shader&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Shader& Shader::operator=(Shader&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	Impl<Shader>::Impl(GraphicsContext& context, std::string_view path, ShaderStage stages, std::vector<uint32_t>* outputCode)
		: Context(context.pimpl)
	{
		auto spirvBlob = SlangCompile(path, &Context->Config, true);
		size_t spirvSizeUint = spirvBlob->getBufferSize() / sizeof(uint32_t);
		CreateStages({ (uint32_t*)spirvBlob->getBufferPointer(), spirvSizeUint }, stages);

		if (outputCode)
		{
			auto dxilBlob = SlangCompile(path, &Context->Config, false);
			size_t dxilSizeUint = dxilBlob->getBufferSize() / sizeof(uint32_t);
			outputCode->resize(spirvSizeUint + dxilSizeUint + 1);

			(*outputCode)[0] = (uint32_t)spirvSizeUint;
			memcpy(outputCode->data() + 1, spirvBlob->getBufferPointer(), spirvBlob->getBufferSize());
			memcpy(outputCode->data() + 1 + spirvSizeUint, dxilBlob->getBufferPointer(), dxilBlob->getBufferSize());
		}
	}

	Impl<Shader>::Impl(GraphicsContext& context, std::span<const uint32_t> code, ShaderStage stages)
		: Context(context.pimpl)
	{
		uint32_t spirvSizeUint = code[0];
		CreateStages({ code.data() + 1, spirvSizeUint }, stages);
	}

	Impl<Shader>::~Impl()
	{
		auto pendingValue = Context->GraphicsQueue.pimpl->Fence.GetPendingValue();
		for (auto& shader : ShaderStages)
		{
			if (g_ShaderObjectsSupported)
			{
				Context->DeletionQueues->Push(pendingValue, shader.Shader);
			}
			else
			{
				Context->DeletionQueues->Push(pendingValue, s_ShaderList[shader.ID].StageInfo.module);
				s_ShaderList.erase(shader.ID);
			}
		}
	}

	void Impl<Shader>::CreateStages(std::span<const uint32_t> code, ShaderStage stages)
	{
		for (ShaderStage stage = (ShaderStage)1; stage < ShaderStage::Max; stage <<= 1)
		{
			if (stages & stage)
			{
				VkShaderStageFlagBits currentStage;
				VkShaderStageFlags nextStages;
				CBStageToVulkan(stage, &currentStage, &nextStages);

				ShaderData data;
				data.Stage = currentStage;

				if (g_ShaderObjectsSupported)
				{
					VkShaderCreateInfoEXT shaderInfo = {
						.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
						.stage = currentStage,
						.nextStage = nextStages,
						.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT,
						.codeSize = code.size_bytes(),
						.pCode = code.data(),
						.pName = "main",
						.setLayoutCount = 1,
						.pSetLayouts = &Context->BindlessSetLayout,
						.pushConstantRangeCount = 1,
						.pPushConstantRanges = PUSH_CONSTANT_RANGES.data()
					};
					VK_CHECK(vkCreateShadersEXT(Context->Device, 1, &shaderInfo, nullptr, &data.Shader), "Failed to create shader object");
				}
				else
				{
					VkShaderModuleCreateInfo moduleInfo = {
						.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
						.codeSize = code.size_bytes(),
						.pCode = code.data()
					};

					VkShaderModule module;
					VK_CHECK(vkCreateShaderModule(Context->Device, &moduleInfo, nullptr, &module), "Failed to create shader module");

					data.StageInfo = {
						.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
						.stage = currentStage,
						.module = module,
						.pName = "main"
					};

					uint64_t id = s_IDCounter++;
					data.ID = id;
					s_ShaderList[id] = data;
				}

				ShaderStages.emplace_back(data);
			}
		}
	}

	Impl<Shader>::ShaderData& Impl<Shader>::GetShaderByID(uint64_t id)
	{
		return s_ShaderList[id];
	}

}