#include "DirectXRHI.h"

#include <dxcapi.h>
#include <d3dcompiler.h>

namespace Cobra {

	static uint64_t s_IDCounter = 1;
	static std::unordered_map<uint64_t, Impl<Shader>::ShaderData> s_ShaderList;

	static void DiagnosticCallback(const char* message, void* data)
	{
		Impl<Shader>* shader = (Impl<Shader>*)data;

		// TODO: There has to be a better way to get the severity, right?
		MessageSeverity severity;
		if (std::string(message).find("error") != std::string::npos) severity = MessageSeverity::Error;
		else  if (std::string(message).find("warning") != std::string::npos) severity = MessageSeverity::Warning;

		shader->Context->Config.Callback(message, severity);
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
		auto dxilBlob = SlangCompile(path, &Context->Config, false);
		size_t dxilSizeUint = dxilBlob->getBufferSize() / sizeof(uint32_t);
		
		CreateModule({ (uint8_t*)dxilBlob->getBufferPointer(), dxilBlob->getBufferSize() }, stages);

		if (outputCode)
		{
			auto spirvBlob = SlangCompile(path, &Context->Config, true);
			size_t spirvSizeUint = spirvBlob->getBufferSize() / sizeof(uint32_t);
			outputCode->resize(spirvSizeUint + dxilSizeUint + 1);

			(*outputCode)[0] = (uint32_t)spirvSizeUint;
			memcpy(outputCode->data() + 1, spirvBlob->getBufferPointer(), spirvBlob->getBufferSize());
			memcpy(outputCode->data() + 1 + spirvSizeUint, dxilBlob->getBufferPointer(), dxilBlob->getBufferSize());
		}
	}

	Impl<Shader>::Impl(GraphicsContext& context, std::span<const uint32_t> code, ShaderStage stages)
		: Context(context.pimpl)
	{
		size_t spirvSizeUint = code[0];
		//CreateModule({ code.data() + 1 + spirvSizeUint, code.size() - 1 - spirvSizeUint});
	}

	Impl<Shader>::~Impl()
	{
		//s_ShaderList.erase(ID);
	}

	void Impl<Shader>::CreateModule(std::span<const uint8_t> code, ShaderStage stages)
	{
		ComPtr<IDxcLinker> linker;
		ComPtr<IDxcUtils> utils;
		DX_CHECK(DxcCreateInstance(CLSID_DxcLinker, IID_PPV_ARGS(&linker)), "Failed to create dxc linker");
		DX_CHECK(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)), "Failed to create dxc utils");

		ComPtr<IDxcBlobEncoding> blob;
		utils->CreateBlobFromPinned(code.data(), code.size_bytes(), DXC_CP_ACP, blob.GetAddressOf());
		linker->RegisterLibrary(L"Lib", blob.Get());

		for (ShaderStage stage = (ShaderStage)1; stage < ShaderStage::Max; stage <<= 1)
		{
			if (stages & stage)
			{
				ComPtr<IDxcOperationResult> result;
				if (stage == ShaderStage::Vertex) linker->Link(L"vertexMain", L"vs_6_6", PtrTo<const wchar_t*>(L"Lib"), 1, nullptr, 0, result.GetAddressOf());
				else linker->Link(L"pixelMain", L"ps_6_6", PtrTo<const wchar_t*>(L"Lib"), 1, nullptr, 0, result.GetAddressOf());

				ComPtr<IDxcBlob> shader;
				result->GetResult(shader.GetAddressOf());

				//Code.resize(shader->GetBufferSize());
				//memcpy(Code.data(), shader->GetBufferPointer(), shader->GetBufferSize());

				uint32_t id = s_IDCounter++;
				ShaderData data = {
					.Stage = stage,
					.ID = id,
				};
				shader.As(&data.Blob);
				
				s_ShaderList[id] = data;
				ShaderStages.emplace_back(data);
			}
		}
	}

	Impl<Shader>::ShaderData& Impl<Shader>::GetShaderByID(uint64_t id)
	{
		return s_ShaderList[id];
	}

}