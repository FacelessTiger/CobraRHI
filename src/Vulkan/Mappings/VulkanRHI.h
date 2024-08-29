#pragma once

#include <CobraRHI.h>
#include <Shared.h>

#include "../InternalManagers/DeletionQueue.h"
#include "../InternalManagers/VulkanPipelines.h"
#include "../InternalManagers/Utils.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <volk.h>
#include <vk_mem_alloc.h>

#include <thread>
#include <memory>
#include <array>
#include <vector>
#include <deque>
#include <sstream>

namespace Cobra {

#define VK_CHECK(function, error) if (function != VK_SUCCESS) throw std::runtime_error(error);

	inline constexpr uint32_t SAMPLER_BINDING = 0;
	inline constexpr uint32_t STORAGE_IMAGE_BINDING = 1;
	inline constexpr uint32_t SAMPLED_IMAGE_BINDING = 2;

	inline bool g_ShaderObjectsSupported = false;
	inline bool g_GPLSupported = false;
	inline bool g_HostImageSupported = false;

	inline constexpr auto PUSH_CONSTANT_RANGES = std::to_array<VkPushConstantRange>({
		{ VK_SHADER_STAGE_ALL, 0, 128 }
	});

	template <>
	struct Impl<GraphicsContext>
	{
		VkInstance Instance;
		VkDebugUtilsMessengerEXT Messenger;
		ContextConfig Config;
		VkPhysicalDevice ChosenGPU;
		VkDevice Device;
		VmaAllocator Allocator;

		Queue GraphicsQueue, TransferQueue;
		TransferManager* TransferManager;
		std::unique_ptr<DeletionQueue> DeletionQueues;

		VkDescriptorPool BindlessPool;
		VkDescriptorSetLayout BindlessSetLayout;
		VkDescriptorSet BindlessSet;
		VkPipelineLayout BindlessPipelineLayout;

		VkPipelineCache PipelineCache;
		std::unordered_map<GraphicsPipelineKey, VkPipeline> GraphicsPipelines;
		std::unordered_map<ComputePipelineKey, VkPipeline> ComputePipelines;

		Impl(const ContextConfig& config);
		~Impl();

		void CreateInstance();
		void CreateDebugMessenger();
		void PickGPU();
		void CreateDeviceAndQueues();
		void SetupBindless();

		VkDebugUtilsMessengerCreateInfoEXT DebugMessengerInfo();
	};

	template <>
	struct Impl<Queue>
	{
		VkQueue Queue;
		uint32_t QueueFamily;
		VkQueueFlags Flags;

		Fence Fence;

		Impl<GraphicsContext>* Context;

		struct CommandAllocator
		{
			VkCommandPool CommandPool;
			std::vector<CommandList> AvailableCommandLists;
		};

		struct SubmittedCommandList
		{
			CommandList CommandList;
			uint64_t FenceValue;
		};

		std::vector<CommandAllocator*> Allocators;
		std::deque<SubmittedCommandList> PendingCommandLists;

		Impl(Impl<GraphicsContext>* context, VkQueue queue, uint32_t queueFamily, VkQueueFlags flags);

		CommandAllocator* AcquireCommandAllocator();
		void Destroy();
	};

	template<>
	struct Impl<Swapchain>
	{
		uVec2 Size;
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

		Impl(GraphicsContext& context, void* window, uVec2 size, bool enableVsync = true);
		~Impl();

		void Recreate();
		void CreateSwapchain(bool firstCreation);

		VkSurfaceFormatKHR ChooseSurfaceFormat(std::span<VkSurfaceFormatKHR> availableFormats);
		VkPresentModeKHR ChooseVsyncOffPresent(std::span<VkPresentModeKHR> presentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	};

	template <>
	struct Impl<Fence>
	{
		VkSemaphore TimelineSemaphore;
		uint64_t LastSubmittedValue = 0;
		uint64_t LastSeenValue = 0;

		Impl<GraphicsContext>* Context;

		Impl(Impl<GraphicsContext>* context);
	};

	template <>
	struct Impl<Shader>
	{
		struct ShaderData
		{
			union
			{
				VkShaderEXT Shader;
				VkPipelineShaderStageCreateInfo StageInfo;
			};

			VkShaderStageFlagBits Stage;
			uint64_t ID;
		};
		std::vector<ShaderData> ShaderStages;

		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(GraphicsContext& context, std::string_view path, ShaderStage stages, std::vector<uint32_t>* outputCode);
		Impl(GraphicsContext& context, std::span<const uint32_t> code, ShaderStage stages);
		~Impl();

		void CreateStages(std::span<const uint32_t> code, ShaderStage stages);

		static ShaderData& GetShaderByID(uint64_t id);
	};

	template <>
	struct Impl<Image>
	{
		ImageAllocation Allocation;
		VkImageView View;
		VkImageLayout Layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkPipelineStageFlags2 Stage = VK_PIPELINE_STAGE_2_NONE;
		VkAccessFlags2 Mask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
		ImageFormat Format;
		ImageUsage Usage;
		uVec2 Size;
		bool ExternalAllocation = false;

		ResourceHandle Handle = { ResourceType::Image };
		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(std::shared_ptr<Impl<GraphicsContext>> context);
		Impl(GraphicsContext& context, const uVec2& size, ImageFormat format, ImageUsage usage);
		~Impl();

		void TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout, VkPipelineStageFlags2 newStage);

		void UpdateDescriptor();
	};

	template <>
	struct Impl<Buffer>
	{
		size_t Size;
		uint64_t Address;

		BufferAllocation Allocation;

		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(GraphicsContext& context, size_t size, BufferUsage usage, BufferFlags flags);
		~Impl();
	};

	template <>
	struct Impl<Sampler>
	{
		VkSampler Sampler;
		ResourceHandle Handle = { ResourceType::Sampler };

		std::shared_ptr<Impl<GraphicsContext>> Context;

		Impl(GraphicsContext& context, Filter min, Filter mag);
		~Impl();
	};

	template <>
	struct Impl<CommandList>
	{
		VkCommandBuffer CommandBuffer;
		Impl<Queue>::CommandAllocator* Allocator;

		GraphicsPipelineKey GraphicsKey;
		bool GraphicsStateChanged = false;
		bool Recording = false;

		Impl<GraphicsContext>* Context;

		Impl(Impl<GraphicsContext>* context, Impl<Queue>::CommandAllocator* allocator, VkCommandBuffer commandBuffer);

		void BindPipelineIfNeeded();
	};

	template <>
	struct Impl<VirtualAllocation>
	{
		VmaVirtualAllocation Allocation;

		Impl(VmaVirtualAllocation allocation);
	};

	template <>
	struct Impl<VirtualAllocator>
	{
		VmaVirtualBlock Block;

		Impl(size_t size);
	};

	template <typename T>
	inline T* PtrTo(T&& v) { return &v; }

	namespace Platform {

		void AddPlatformExtensions(std::vector<const char*>& extensions);
		VkSurfaceKHR CreateVulkanSurface(std::shared_ptr<Impl<GraphicsContext>> context, void* handle);

	}

}