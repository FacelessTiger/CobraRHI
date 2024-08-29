#include <slang.h>
#include <slang-com-ptr.h>

#include <GraphicsContext.h>

namespace Cobra {

	Slang::ComPtr<slang::IBlob> SlangCompile(std::string_view path, ContextConfig* config, bool compileSpirv);

}