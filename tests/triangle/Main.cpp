#include <CobraRHI.h>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
		Cobra::Queue& queue = context.GetQueue(Cobra::QueueType::Graphics);

		Cobra::uVec2 imageSize = { 1600, 900 };
		Cobra::Shader shader(context, "tests/triangle/triangle.slang", Cobra::ShaderStage::Vertex | Cobra::ShaderStage::Pixel);
		Cobra::Image image(context, imageSize, Cobra::ImageFormat::R8G8B8A8_UNORM, Cobra::ImageUsage::ColorAttachment | Cobra::ImageUsage::TransferSrc);
		Cobra::Buffer outputBuffer(context, imageSize.x * imageSize.y * 4, Cobra::BufferUsage::TransferDst, Cobra::BufferFlags::Mapped);

		auto cmd = queue.Begin();
		cmd.BeginRendering(imageSize, { image });
		cmd.ClearColorAttachment(0, Cobra::Vec4{ 0.0f, 0.0f, 0.0f, 1.0f }, imageSize);

		cmd.SetDefaultState();
		cmd.SetScissor((Cobra::iVec2)imageSize);
		cmd.SetViewport((Cobra::iVec2)imageSize);

		cmd.BindShaders({ shader });
		cmd.Draw(3, 1, 0, 0);
		cmd.EndRendering();

		cmd.Barrier(Cobra::PipelineStage::Graphics, Cobra::PipelineStage::Transfer);
		cmd.CopyToBuffer(image, outputBuffer);
		queue.Submit(cmd, {}).Wait();
		
		int w, h, c;
		uint8_t* img = stbi_load("tests/triangle/compare.png", &w, &h, &c, 4);
		if (memcmp(outputBuffer.GetHostAddress(), img, imageSize.x * imageSize.y * 4) != 0)
		{
			stbi_image_free(img);
			throw std::runtime_error("Images are not identical");
		}

		stbi_image_free(img);
	}
	catch (std::runtime_error& e)
	{
		std::cout << "Shader test failed with error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}