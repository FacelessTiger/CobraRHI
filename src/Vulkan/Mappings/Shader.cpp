#include "VulkanRHI.h"

#include <slang.h>
#include <slang-com-ptr.h>

#include <vector>
#include <array>
#include <iostream>

namespace Cobra {

	using Slang::ComPtr;

	static uint64_t s_IDCounter = 1;
	static std::unordered_map<uint64_t, Impl<Shader>::ShaderData> s_ShaderList;

	static void SlangStageToVulkan(SlangStage stage, VkShaderStageFlagBits* currentStage, VkShaderStageFlags* nextStages)
	{
		switch (stage)
		{
			case SlangStage::SLANG_STAGE_VERTEX:
			{
				*currentStage = VK_SHADER_STAGE_VERTEX_BIT;
				*nextStages = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			}
			case SlangStage::SLANG_STAGE_FRAGMENT:
			{
				*currentStage = VK_SHADER_STAGE_FRAGMENT_BIT;
				*nextStages = (VkShaderStageFlagBits)0;
				break;
			}
			case SlangStage::SLANG_STAGE_COMPUTE:
			{
				*currentStage = VK_SHADER_STAGE_COMPUTE_BIT;
				*nextStages = (VkShaderStageFlagBits)0;
				break;
			}
		}
	}

	static void DiagnosticCallback(const char* message, void* data)
	{
		Impl<Shader>* shader = (Impl<Shader>*)data;

		// TODO: There has to be a better way to get the severity, right?
		MessageSeverity severity;
		if (std::string(message).find("error") != std::string::npos) severity = MessageSeverity::Error;
		else  if (std::string(message).find("warning") != std::string::npos) severity = MessageSeverity::Warning;

		shader->Context->Config.Callback(message, severity);
	}

	Shader::Shader(GraphicsContext& context, std::string_view path)
	{
		pimpl = std::make_unique<Impl<Shader>>(context, path);
	}

	Shader::~Shader() { }
	Shader::Shader(Shader&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Shader& Shader::operator=(Shader&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	Impl<Shader>::Impl(GraphicsContext& context, std::string_view path)
		: Context(context.pimpl)
	{
		ComPtr<slang::IGlobalSession> globalSession;
		slang::createGlobalSession(globalSession.writeRef());

		ComPtr<slang::ICompileRequest> compileRequest;
		globalSession->createCompileRequest(compileRequest.writeRef());
		compileRequest->setCodeGenTarget(SlangCompileTarget::SLANG_SPIRV);

		const int translationUnitIndex = compileRequest->addTranslationUnit(SlangSourceLanguage::SLANG_SOURCE_LANGUAGE_SLANG, nullptr);
		compileRequest->addTranslationUnitSourceFile(translationUnitIndex, path.data());
		compileRequest->setMatrixLayoutMode(SLANG_MATRIX_LAYOUT_COLUMN_MAJOR);
		compileRequest->setTargetFlags(0, SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY | SLANG_TARGET_FLAG_GENERATE_WHOLE_PROGRAM);
		compileRequest->setTargetForceGLSLScalarBufferLayout(0, true);
		compileRequest->setDiagnosticCallback(DiagnosticCallback, this);

		if (Context->Config.Debug)
		{
			compileRequest->setOptimizationLevel(SLANG_OPTIMIZATION_LEVEL_NONE);
			compileRequest->setDebugInfoLevel(SLANG_DEBUG_INFO_LEVEL_MAXIMAL);
		}
		else
		{
			compileRequest->setOptimizationLevel(SLANG_OPTIMIZATION_LEVEL_MAXIMAL);
			compileRequest->setDebugInfoLevel(SLANG_DEBUG_INFO_LEVEL_NONE);
		}

		auto compileArguments = std::to_array<const char*>({
			"-warnings-disable", "39001", // Disables descriptor binding aliasing warning
			"-fvk-use-entrypoint-name"
		});
		if (SLANG_FAILED(compileRequest->processCommandLineArguments(compileArguments.data(), (int)compileArguments.size())))
			Context->Config.Callback((std::string("Shader error: ") + compileRequest->getDiagnosticOutput()).c_str(), MessageSeverity::Error);

		if (SLANG_FAILED(compileRequest->compile()))
			return;

		size_t codeSize;
		const void* code = compileRequest->getCompileRequestCode(&codeSize);

		auto* reflection = (slang::ShaderReflection*)compileRequest->getReflection();
		for (int i = 0; i < reflection->getEntryPointCount(); i++)
		{
			auto* entryPoint = reflection->getEntryPointByIndex(i);

			VkShaderStageFlagBits currentStage;
			VkShaderStageFlags nextStages;
			SlangStageToVulkan(entryPoint->getStage(), &currentStage, &nextStages);

			ShaderData data;
			data.Stage = currentStage;
			data.EntryPoint = entryPoint->getName();

			if (g_ShaderObjectsSupported)
			{
				VkShaderCreateInfoEXT shaderInfo = {
					.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
					.stage = currentStage,
					.nextStage = nextStages,
					.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT,
					.codeSize = codeSize,
					.pCode = code,
					.pName = data.EntryPoint.c_str(),
					.setLayoutCount = 1,
					.pSetLayouts = &Context->BindlessSetLayout,
					.pushConstantRangeCount = 1,
					.pPushConstantRanges = PUSH_CONSTANT_RANGES.data()
				};
				VkCheck(Context->Config, vkCreateShadersEXT(Context->Device, 1, &shaderInfo, nullptr, &data.Shader));
			}
			else
			{
				VkShaderModuleCreateInfo shaderInfo = {
					.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
					.codeSize = codeSize,
					.pCode = (const uint32_t*)code
				};
				VkCheck(Context->Config, vkCreateShaderModule(Context->Device, &shaderInfo, nullptr, &data.Module));

				uint64_t id = s_IDCounter++;
				data.ID = id;
				s_ShaderList[id] = data;
			}

			ShaderStages.emplace_back(data);
		}
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
				Context->DeletionQueues->Push(pendingValue, s_ShaderList[shader.ID].Module);
				s_ShaderList.erase(shader.ID);
			}
		}
	}

	Impl<Shader>::ShaderData& Impl<Shader>::GetShaderByID(uint64_t id)
	{
		return s_ShaderList[id];
	}

}