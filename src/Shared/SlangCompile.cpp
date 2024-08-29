#include "SlangCompile.h"

#include <array>

namespace Cobra {

	using Slang::ComPtr;

	static ComPtr<slang::IGlobalSession> s_GlobalSession;
	static bool s_Initalized = false;

	static void DiagnosticCallback(const char* message, void* data)
	{
		ContextConfig* config = (ContextConfig*)data;

		// TODO: There has to be a better way to get the severity, right?
		MessageSeverity severity;
		if (std::string(message).find("error") != std::string::npos) severity = MessageSeverity::Error;
		else  if (std::string(message).find("warning") != std::string::npos) severity = MessageSeverity::Warning;
		else return;

		config->Callback(message, severity);
	}

	Slang::ComPtr<slang::IBlob> SlangCompile(std::string_view path, ContextConfig* config, bool compileSpirv)
	{
		if (!s_Initalized)
		{
			slang::createGlobalSession(s_GlobalSession.writeRef());
			s_Initalized = true;
		}

		ComPtr<slang::ICompileRequest> compileRequest;
		s_GlobalSession->createCompileRequest(compileRequest.writeRef());

		const int translationUnitIndex = compileRequest->addTranslationUnit(SlangSourceLanguage::SLANG_SOURCE_LANGUAGE_SLANG, nullptr);
		compileRequest->addTranslationUnitSourceFile(translationUnitIndex, path.data());
		compileRequest->addSearchPath(COBRARHI_INCLUDE_DIR);
		compileRequest->setMatrixLayoutMode(SLANG_MATRIX_LAYOUT_COLUMN_MAJOR);
		compileRequest->setDiagnosticCallback(DiagnosticCallback, config);

		if (compileSpirv)
		{
			compileRequest->setCodeGenTarget(SlangCompileTarget::SLANG_SPIRV);
			compileRequest->setTargetFlags(0, SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY | SLANG_TARGET_FLAG_GENERATE_WHOLE_PROGRAM);
			compileRequest->setTargetForceGLSLScalarBufferLayout(0, true);
			compileRequest->addPreprocessorDefine("SPIRV_COMPILE", "true");
		}
		else
		{
			compileRequest->setCodeGenTarget(SlangCompileTarget::SLANG_DXIL);
			compileRequest->setTargetFlags(0, SLANG_TARGET_FLAG_GENERATE_WHOLE_PROGRAM);
			compileRequest->setTargetProfile(0, s_GlobalSession->findProfile("lib_6_6"));
			compileRequest->addPreprocessorDefine("DXIL_COMPILE", "true");
		}

		if (config->Debug)
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
		});
		if (SLANG_FAILED(compileRequest->processCommandLineArguments(compileArguments.data(), (int)compileArguments.size())))
			throw std::runtime_error("Shader error at path \"" + std::string(path) + "\" with error: " + compileRequest->getDiagnosticOutput());

		if (SLANG_FAILED(compileRequest->compile()))
			throw std::runtime_error("Failed to compile shader at path \"" + std::string(path) + "\"");

		Slang::ComPtr<slang::IBlob> blob;
		compileRequest->getTargetCodeBlob(0, blob.writeRef());
		return blob;
	}

}