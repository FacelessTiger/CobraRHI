#include <CobraRHI.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <iostream>

int main()
{
	try
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		GLFWwindow* window = glfwCreateWindow(1600, 900, "Swapchain Test", nullptr, nullptr);

		Cobra::GraphicsContext context(Cobra::ContextConfig{
			.Debug = true,
			.Trace = true,
			.Callback = [](const char* message, Cobra::MessageSeverity severity) {
				if (severity != Cobra::MessageSeverity::Trace) std::cout << message << std::endl;
			}
		});

		Cobra::Queue& queue = context.GetQueue(Cobra::QueueType::Graphics);
		Cobra::Shader shader(context, "tests/triangle/triangle.slang", Cobra::ShaderStage::Vertex | Cobra::ShaderStage::Pixel);
		Cobra::Swapchain swapchain(context, glfwGetWin32Window(window), { 1600, 900 });

		glfwSetWindowUserPointer(window, &swapchain);
		glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) {
			auto& swapchain = *(Cobra::Swapchain*)glfwGetWindowUserPointer(window);
			swapchain.Resize({ (uint32_t)width, (uint32_t)height });
		});

		uint32_t frameCounter = 0;
		Cobra::SyncPoint wait[2];

		while (frameCounter < 100)
		{
			if (frameCounter == 50) glfwSetWindowSize(window, 200, 200);
			glfwPollEvents();

			uint32_t fif = frameCounter++ % 2;
			wait[fif].Wait();
			queue.Acquire(swapchain);

			auto cmd = queue.Begin();
			cmd.BeginRendering(swapchain.GetSize(), { swapchain.GetCurrent() });
			cmd.ClearColorAttachment(0, Cobra::Vec4{ 0.0f, 0.0f, 0.0f, 1.0f }, swapchain.GetSize());

			cmd.SetDefaultState();
			cmd.SetScissor((Cobra::iVec2)swapchain.GetSize());
			cmd.SetViewport((Cobra::iVec2)swapchain.GetSize());

			cmd.BindShaders({ shader });
			cmd.Draw(3, 1, 0, 0);
			cmd.EndRendering();

			cmd.Present(swapchain);
			wait[fif] = queue.Submit(cmd, {});
			queue.Present(swapchain, {});
		}

		glfwDestroyWindow(window);
		glfwTerminate();
	}
	catch (std::runtime_error& e)
	{
		std::cout << "Shader test failed with error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}