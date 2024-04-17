#pragma once

#include <Core/Core.h>
#include <Renderer/GraphicsContext.h>
#include <Renderer/Queue.h>
#include <Renderer/Fence.h>
#include <Renderer/Shader.h>
#include <Renderer/Image.h>
#include <Renderer/Buffer.h>
#include <Renderer/CommandAllocator.h>
#include <Renderer/CommandList.h>
#include <Renderer/Swapchain.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <volk.h>
#include <vk_mem_alloc.h>

#include <thread>
#include <memory>
#include <array>
#include <vector>
#include <source_location>
#include <sstream>

namespace Cobra {

	inline constexpr uint32_t SAMPLER_BINDING = 0;
	inline constexpr uint32_t STORAGE_IMAGE_BINDING = 1;
	inline constexpr uint32_t SAMPLED_IMAGE_BINDING = 2;

	inline constexpr auto PUSH_CONSTANT_RANGES = std::to_array<VkPushConstantRange>({
		{ VK_SHADER_STAGE_ALL, 0, 128 }
	});

	template <>
	struct Impl<GraphicsContext>
	{
		VkInstance Instance;
		VkDebugUtilsMessengerEXT Messenger;
		DebugCallback Callback;
		VkPhysicalDevice ChosenGPU;
		VkDevice Device;
		VmaAllocator Allocator;

		VkQueue GraphicsQueue;
		uint32_t GraphicsQueueFamily;

		VkDescriptorPool BindlessPool;
		VkDescriptorSetLayout BindlessSetLayout;
		VkDescriptorSet BindlessSet;
		VkPipelineLayout BindlessPipelineLayout;

		Impl(DebugCallback debugCallback);
		~Impl();

		void CreateInstance();
		void CreateDebugMessenger();
		void PickGPUAndQueueFamilys();
		void CreateDeviceAndQueues();
		void SetupBindless();

		VkDebugUtilsMessengerCreateInfoEXT DebugMessengerInfo();
	};

	template <>
	struct Impl<Queue>
	{
		VkQueue Queue;
		uint32_t QueueFamily;

		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(GraphicsContext& context, QueueType type);
	};

	template<>
	struct Impl<Swapchain>
	{
		uint32_t Width, Height;
		bool EnableVsync;
		bool Dirty = false;

		std::vector<VkSemaphore> Semaphores;
		uint32_t SemaphoreIndex = 0;
		uint32_t ImageIndex = 0;

		VkSurfaceKHR Surface;
		VkSwapchainKHR Swapchain;

		VkSurfaceFormatKHR ChosenSurfaceFormat;
		VkPresentModeKHR VsyncOnPresent, VsyncOffPresent;

		std::vector<Image> Images;
		VkFormat ImageFormat;

		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(GraphicsContext& context, const Window& window, bool enableVsync);
		~Impl();

		void Recreate();

		void CreateSurface(const Window& window);
		void CreateSwapchain(bool firstCreation);

		VkSurfaceFormatKHR ChooseSurfaceFormat(std::span<VkSurfaceFormatKHR> availableFormats);
		VkPresentModeKHR ChooseVsyncOffPresent(std::span<VkPresentModeKHR> presentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	};

	template <>
	struct Impl<Fence>
	{
		VkSemaphore TimelineSemaphore;
		uint64_t Value = 0;

		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(GraphicsContext& context);
		~Impl();
	};

	template <>
	struct Impl<Shader>
	{
		struct ShaderData
		{
			VkShaderEXT Shader;
			VkShaderStageFlagBits Stage;
		};
		std::vector<ShaderData> ShaderStages;

		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(GraphicsContext& context, std::string_view path);
		~Impl();
	};

	template <>
	struct Impl<Image>
	{
		VkImage Image;
		VkImageView View;
		VkImageLayout Layout;
		VkFormat Format;
		VkImageUsageFlags UsageFlags;
		uint32_t Width, Height;

		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(std::shared_ptr<Impl<GraphicsContext>> context);
		~Impl();

		void TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout, VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask);
	};

	template <>
	struct Impl<Buffer>
	{
		size_t Size;
		uint64_t Address;

		VkBuffer Buffer;
		VmaAllocation Allocation;
		VmaAllocationInfo Info;

		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(GraphicsContext& context, size_t size, BufferUsage usage, BufferFlags flags);
		~Impl();
	};

	template <>
	struct Impl<CommandAllocator>
	{
		VkCommandPool CommandPool;
		VkCommandBuffer MainCommandBuffer;

		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(GraphicsContext& context, const Queue& queue);
		~Impl();
	};

	template <> 
	struct Impl<CommandList> 
	{
		VkCommandBuffer CommandBuffer;

		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(std::shared_ptr<Impl<GraphicsContext>> context, VkCommandBuffer commandBuffer);
	};

	inline void VkCheck(DebugCallback callback, VkResult result, std::string_view message = "Result is not VK_SUCCESS", std::source_location location = std::source_location::current())
	{
		if (!callback) return;
		if (result == VK_SUCCESS) return;

		std::stringstream error;
		error << "File: " << location.file_name() << "(" << location.line() << ":" << location.column() << ") '" << location.function_name() << "': ";
		error << message;
		callback(error.str().c_str(), MessageSeverity::Error);
	}

}