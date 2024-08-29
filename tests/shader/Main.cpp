#include <CobraRHI.h>

#include <iostream>

int main()
{
	try
	{
		Cobra::GraphicsContext context(Cobra::ContextConfig{
			.Debug = true,
			.Trace = true,
			.Callback = [](const char* message, Cobra::MessageSeverity severity) {
				if (severity == Cobra::MessageSeverity::Error) std::cout << message << std::endl;
			}
		});

		std::vector<uint32_t> code;
		Cobra::Shader fromSourceShader(context, "tests/shader/shader.slang", Cobra::ShaderStage::Vertex | Cobra::ShaderStage::Pixel, &code);
		Cobra::Shader fromCodeShader(context, code, Cobra::ShaderStage::Vertex | Cobra::ShaderStage::Pixel);
	}
	catch (std::runtime_error& e)
	{
		std::cout << "Shader test failed with error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}