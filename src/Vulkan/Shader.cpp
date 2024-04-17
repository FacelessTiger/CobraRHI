#include "VulkanRHI.h"

#include <slang.h>
#include <slang-com-ptr.h>

#include <vector>
#include <array>
#include <iostream>

namespace Cobra {

	using Slang::ComPtr;

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
		}
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
		compileRequest->setTargetFlags(0, SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY);
		compileRequest->setTargetForceGLSLScalarBufferLayout(0, true);

		auto compileArguments = std::to_array<const char*>({
			"-fvk-use-entrypoint-name"
		});
		if (SLANG_FAILED(compileRequest->processCommandLineArguments(compileArguments.data(), (int)compileArguments.size())))
			Context->Callback((std::string("Shader error: ") + compileRequest->getDiagnosticOutput()).c_str(), MessageSeverity::Error);

		if (SLANG_FAILED(compileRequest->compile()))
		{
			Context->Callback((std::string("Shader error: ") + compileRequest->getDiagnosticOutput()).c_str(), MessageSeverity::Error);
			return;
		}

		auto* reflection = (slang::ShaderReflection*)compileRequest->getReflection();
		for (int i = 0; i < reflection->getEntryPointCount(); i++)
		{
			auto* entryPoint = reflection->getEntryPointByIndex(i);

			size_t codeSize;
			const void* code = compileRequest->getEntryPointCode(i, &codeSize);

			VkShaderStageFlagBits currentStage;
			VkShaderStageFlags nextStages;
			SlangStageToVulkan(entryPoint->getStage(), &currentStage, &nextStages);

			VkShaderCreateInfoEXT shaderInfo = {
				.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
				.stage = currentStage,
				.nextStage = nextStages,
				.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT,
				.codeSize = codeSize,
				.pCode = code,
				.pName = entryPoint->getName()
			};

			ShaderData data;
			data.Stage = currentStage;
			VkCheck(Context->Callback, vkCreateShadersEXT(Context->Device, 1, &shaderInfo, nullptr, &data.Shader));
			ShaderStages.emplace_back(data);
		}
	}

	Impl<Shader>::~Impl()
	{
		// TODO: Push to deletion queue
		for (auto& shader : ShaderStages)
			vkDestroyShaderEXT(Context->Device, shader.Shader, nullptr);
	}

}