#include "VulkanRHI.h"

#include <slang.h>
#include <slang-com-ptr.h>

#include <vector>
#include <array>
#include <iostream>
#include <algorithm>

namespace Cobra {

	using Slang::ComPtr;

	static uint64_t s_IDCounter = 1;
	static std::unordered_map<uint64_t, Impl<Shader>::ShaderData> s_ShaderList;

	static Impl<Shader>::ShaderStage SlangStageToCB(SlangStage stage)
	{
		switch (stage)
		{
			case SlangStage::SLANG_STAGE_VERTEX:    return Impl<Shader>::ShaderStage::Vertex;
			case SlangStage::SLANG_STAGE_FRAGMENT:  return Impl<Shader>::ShaderStage::Fragment;
			case SlangStage::SLANG_STAGE_COMPUTE:   return Impl<Shader>::ShaderStage::Compute;
			default:                                std::unreachable();
		}
	}

	static void CBStageToVulkan(Impl<Shader>::ShaderStage stage, VkShaderStageFlagBits* currentStage, VkShaderStageFlags* nextStages)
	{
		switch (stage)
		{
			case Impl<Shader>::ShaderStage::Vertex:
			{
				*currentStage = VK_SHADER_STAGE_VERTEX_BIT;
				*nextStages = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			}
			case Impl<Shader>::ShaderStage::Fragment:
			{
				*currentStage = VK_SHADER_STAGE_FRAGMENT_BIT;
				*nextStages = (VkShaderStageFlagBits)0;
				break;
			}
			case Impl<Shader>::ShaderStage::Compute:
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

	Shader::Shader(GraphicsContext& context, std::string_view path, std::vector<uint32_t>* outputCode)
	{
		pimpl = std::make_unique<Impl<Shader>>(context, path, outputCode);
	}

	Shader::Shader(GraphicsContext& context, std::span<const uint32_t> code)
	{
		pimpl = std::make_unique<Impl<Shader>>(context, code);
	}

	Shader::~Shader() { }
	Shader::Shader(Shader&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Shader& Shader::operator=(Shader&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	Impl<Shader>::Impl(GraphicsContext& context, std::string_view path, std::vector<uint32_t>* outputCode)
		: Context(context.pimpl)
	{
#ifndef __ANDROID__
		ComPtr<slang::IGlobalSession> globalSession;
		slang::createGlobalSession(globalSession.writeRef());

		ComPtr<slang::ICompileRequest> compileRequest;
		globalSession->createCompileRequest(compileRequest.writeRef());
		compileRequest->setCodeGenTarget(SlangCompileTarget::SLANG_SPIRV);

		const int translationUnitIndex = compileRequest->addTranslationUnit(SlangSourceLanguage::SLANG_SOURCE_LANGUAGE_SLANG, nullptr);
		compileRequest->addTranslationUnitSourceFile(translationUnitIndex, path.data());
		compileRequest->addSearchPath(COBRARHI_INCLUDE_DIR);
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

		SlangProgramLayout* layout = compileRequest->getReflection();
		uint32_t entryPointCount = spReflection_getEntryPointCount(layout);
		std::vector<EntryPoint> entryPoints(entryPointCount);

		size_t addedSizeBytes = sizeof(uint32_t);
		for (int i = 0; i < entryPointCount; i++)
		{
			SlangEntryPointLayout* entryPoint = spReflection_getEntryPointByIndex(layout, i);
			entryPoints[i].Stage = SlangStageToCB(spReflectionEntryPoint_getStage(entryPoint));
			entryPoints[i].Name = spReflectionEntryPoint_getName(entryPoint);

			addedSizeBytes += (sizeof(ShaderStage) + (entryPoints[i].Name.size() + 1));
		}
		CreateStages({ (const uint32_t*)code, codeSize / sizeof(uint32_t) }, entryPoints);

		auto addedSizeUints = (size_t)std::ceil((float)addedSizeBytes / sizeof(uint32_t));
		if (outputCode)
		{
			outputCode->resize((codeSize / sizeof(uint32_t)) + addedSizeUints);
			(*outputCode)[0] = entryPointCount;

			uint32_t byteOffset = sizeof(uint32_t);
			for (int i = 0; i < entryPointCount; i++)
			{
				*(ShaderStage*)((std::byte*)outputCode->data() + byteOffset) = entryPoints[i].Stage;
				memcpy(((std::byte*)outputCode->data() + byteOffset + sizeof(ShaderStage)), entryPoints[i].Name.data(), entryPoints[i].Name.size() + 1);

				byteOffset += sizeof(ShaderStage) + (entryPoints[i].Name.size() + 1);
			}

			memcpy(outputCode->data() + addedSizeUints, code, codeSize);
		}
#endif
	}

	Impl<Shader>::Impl(GraphicsContext& context, std::span<const uint32_t> code)
		: Context(context.pimpl)
	{
		uint32_t stageCount = code[0];
		std::vector<EntryPoint> entryPoints(stageCount);

		uint32_t byteOffset = sizeof(uint32_t);
		for (int i = 0; i < stageCount; i++)
		{
			entryPoints[i].Stage = *(ShaderStage*)((std::byte*)code.data() + byteOffset);
			entryPoints[i].Name = (const char*)((std::byte*)code.data() + byteOffset + sizeof(ShaderStage));

			byteOffset += sizeof(ShaderStage) + (entryPoints[i].Name.size() + 1);
		}

		auto uintOffset = (size_t)std::ceil((float)byteOffset / sizeof(uint32_t));
		CreateStages({ code.data() + uintOffset, code.size() - uintOffset }, entryPoints);
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

	void Impl<Shader>::CreateStages(std::span<const uint32_t> code, std::span<EntryPoint> entryPoints)
	{
		for (auto& entryPoint : entryPoints)
		{
			VkShaderStageFlagBits currentStage;
			VkShaderStageFlags nextStages;
			CBStageToVulkan(entryPoint.Stage, &currentStage, &nextStages);

			ShaderData data;
			data.Stage = currentStage;
			data.EntryPoint = entryPoint.Name;

			if (g_ShaderObjectsSupported)
			{
				VkShaderCreateInfoEXT shaderInfo = {
					.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
					.stage = currentStage,
					.nextStage = nextStages,
					.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT,
					.codeSize = code.size_bytes(),
					.pCode = code.data(),
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
					.codeSize = code.size_bytes(),
					.pCode = code.data()
				};
				VkCheck(Context->Config, vkCreateShaderModule(Context->Device, &shaderInfo, nullptr, &data.Module));

				uint64_t id = s_IDCounter++;
				data.ID = id;
				s_ShaderList[id] = data;
			}

			ShaderStages.emplace_back(data);
		}
	}

	Impl<Shader>::ShaderData& Impl<Shader>::GetShaderByID(uint64_t id)
	{
		return s_ShaderList[id];
	}

}