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
	}
	catch (std::runtime_error& e)
	{
		std::cout << "Instance test failed with error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}