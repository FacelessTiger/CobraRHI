#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "VulkanRHI.h"

#include <iostream>
#include <vector>
#include <array>
#include <fstream>

namespace Cobra {

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		ContextConfig::DebugCallback callback = *(ContextConfig::DebugCallback*)pUserData;

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) callback(pCallbackData->pMessage, MessageSeverity::Error);
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) callback(pCallbackData->pMessage, MessageSeverity::Warning);

		return false;
	}

	GraphicsContext::GraphicsContext(const ContextConfig& config)
	{
		pimpl = std::make_shared<Impl<GraphicsContext>>(config);
		pimpl->TransferManager = new TransferManager(*this);
	}

	GraphicsContext::~GraphicsContext() { }
	GraphicsContext::GraphicsContext(GraphicsContext&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	GraphicsContext& GraphicsContext::operator=(GraphicsContext&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	Queue& GraphicsContext::GetQueue(QueueType type)
	{
		switch (type)
		{
			case QueueType::Graphics: return pimpl->GraphicsQueue;
			case QueueType::Transfer: return pimpl->TransferQueue;
			default: std::unreachable();
		}
	}

	Impl<GraphicsContext>::Impl(const ContextConfig& config)
		: Config(config)
	{
		VkCheck(config, volkInitialize());

		CreateInstance();
		if (config.Debug) CreateDebugMessenger();
		PickGPU();
		CreateDeviceAndQueues();
		SetupBindless();

		VmaVulkanFunctions functions = {
#pragma region DynamicFunctions
			.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
			.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
			.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
			.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
			.vkAllocateMemory = vkAllocateMemory,
			.vkFreeMemory = vkFreeMemory,
			.vkMapMemory = vkMapMemory,
			.vkUnmapMemory = vkUnmapMemory,
			.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
			.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
			.vkBindBufferMemory = vkBindBufferMemory,
			.vkBindImageMemory = vkBindImageMemory,
			.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
			.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
			.vkCreateBuffer = vkCreateBuffer,
			.vkDestroyBuffer = vkDestroyBuffer,
			.vkCreateImage = vkCreateImage,
			.vkDestroyImage = vkDestroyImage,
			.vkCmdCopyBuffer = vkCmdCopyBuffer,
		#if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
			.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR,
			.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR,
		#endif
		#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
			.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR,
			.vkBindImageMemory2KHR = vkBindImageMemory2KHR,
		#endif
		#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
			.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR,
		#endif
		#if VMA_VULKAN_VERSION >= 1003000
			.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements,
			.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements
		#endif
#pragma endregion
		};

		// Setup VMA
		vmaCreateAllocator(PtrTo(VmaAllocatorCreateInfo {
			.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
			.physicalDevice = ChosenGPU,
			.device = Device,
			.pVulkanFunctions = &functions,
			.instance = Instance
		}), &Allocator);

		// Setup pipeline cache
		std::ifstream file(Config.PipelineCacheLocation, std::ios::binary | std::ios::ate);
		std::vector<char> data;
		if (file.good())
		{
			auto size = file.tellg();
			file.seekg(0, std::ios::beg);

			data.resize(size);
			file.read(data.data(), size);
		}

		VkPipelineCacheCreateInfo cacheInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
			.initialDataSize = data.size(),
			.pInitialData = data.data()
		};
		VkCheck(Config, vkCreatePipelineCache(Device, &cacheInfo, nullptr, &PipelineCache));

		// Setup deletion queue(s)
		DeletionQueues = std::make_unique<DeletionQueue>(*this);
	}

	Impl<GraphicsContext>::~Impl()
	{
		delete TransferManager;
		vkDeviceWaitIdle(Device);

		// Write pipeline cache to disk
		size_t dataSize;
		vkGetPipelineCacheData(Device, PipelineCache, &dataSize, nullptr);
		std::vector<char> data(dataSize);
		vkGetPipelineCacheData(Device, PipelineCache, &dataSize, data.data());

		std::ofstream file(Config.PipelineCacheLocation, std::ios::binary);
		file.write(data.data(), dataSize);
			
		// Delete everything
		if (Config.Debug) vkDestroyDebugUtilsMessengerEXT(Instance, Messenger, nullptr);

		for (auto& [key, pipeline] : GraphicsPipelines)
			vkDestroyPipeline(Device, pipeline, nullptr);
		for (auto& [key, pipeline] : ComputePipelines)
			vkDestroyPipeline(Device, pipeline, nullptr);

		DeletionQueues->Flush(GraphicsQueue.pimpl->Fence.GetCurrentValue());
		GraphicsQueue.pimpl->Destroy();
		TransferQueue.pimpl->Destroy();

		vkDestroyDescriptorSetLayout(Device, BindlessSetLayout, nullptr);
		vkDestroyPipelineLayout(Device, BindlessPipelineLayout, nullptr);
		vkDestroyDescriptorPool(Device, BindlessPool, nullptr);
		vkDestroyPipelineCache(Device, PipelineCache, nullptr);

		vmaDestroyAllocator(Allocator);

		vkDestroyDevice(Device, nullptr);
		vkDestroyInstance(Instance, nullptr);
	}

	void Impl<GraphicsContext>::CreateInstance()
	{
		std::vector<const char*> layers;
		std::vector<const char*> extensions = { VK_KHR_SURFACE_EXTENSION_NAME };
		Platform::AddPlatformExtensions(extensions);
		
		if (Config.Debug)
		{
			layers.push_back("VK_LAYER_KHRONOS_validation");
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		auto featuresEnable = std::to_array<VkValidationFeatureEnableEXT>({
			VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT, VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT
		});
		
		VkCheck(Config, vkCreateInstance(PtrTo(VkInstanceCreateInfo {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = PtrTo(VkValidationFeaturesEXT {
				.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
				.pNext = Config.Debug ? PtrTo(DebugMessengerInfo()) : nullptr,
				.enabledValidationFeatureCount = (uint32_t)featuresEnable.size(),
				.pEnabledValidationFeatures = featuresEnable.data()
			}),
			.pApplicationInfo = PtrTo(VkApplicationInfo {
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pEngineName = "Cobra",
				.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
				.apiVersion = VK_API_VERSION_1_3
			}),
			.enabledLayerCount = (uint32_t)layers.size(),
			.ppEnabledLayerNames = layers.data(),
			.enabledExtensionCount = (uint32_t)extensions.size(),
			.ppEnabledExtensionNames = extensions.data()
		}), nullptr, &Instance));
		volkLoadInstance(Instance);
	}

	void Impl<GraphicsContext>::CreateDebugMessenger()
	{
		VkCheck(Config, vkCreateDebugUtilsMessengerEXT(Instance, PtrTo(DebugMessengerInfo()), nullptr, &Messenger));
	}

	void Impl<GraphicsContext>::PickGPU()
	{
		uint32_t deviceCount = 0;
		VkCheck(Config, vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr));
		std::vector<VkPhysicalDevice> devices(deviceCount);
		VkCheck(Config, vkEnumeratePhysicalDevices(Instance, &deviceCount, devices.data()));

		ChosenGPU = devices[0];
		for (auto& device : devices)
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(device, &properties);

			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				ChosenGPU = device;
				break;
			}
		}

		// Check optional extensions
		uint32_t propertyCount;
		vkEnumerateDeviceExtensionProperties(ChosenGPU, nullptr, &propertyCount, nullptr);
		std::vector<VkExtensionProperties> properties(propertyCount);
		vkEnumerateDeviceExtensionProperties(ChosenGPU, nullptr, &propertyCount, properties.data());

		for (auto& property : properties)
		{
			if (!std::strcmp(property.extensionName, VK_EXT_SHADER_OBJECT_EXTENSION_NAME)) g_ShaderObjectsSupported = true;
			else if (!std::strcmp(property.extensionName, VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME)) g_GPLSupported = true;

			if (!std::strcmp(property.extensionName, VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME)) g_HostImageSupported = true;
		}
	}

	void Impl<GraphicsContext>::CreateDeviceAndQueues()
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(ChosenGPU, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(ChosenGPU, &queueFamilyCount, queueFamilies.data());

		// TODO: select queues in a better way
		uint32_t graphicsQueueFamily;
		uint32_t transferQueueFamily;

		for (uint32_t i = 0; i < queueFamilyCount; i++)
		{
			const auto& queueFamily = queueFamilies[i];
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				graphicsQueueFamily = i;
			else if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
				transferQueueFamily = i;
		}

		float queuePriority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> queueInfos = {
			{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = graphicsQueueFamily,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority
			},
			{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = transferQueueFamily,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority
			}
		};

		Utils::FeatureBuilder builder;
		builder.AddExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		builder.AddFeature<VkPhysicalDeviceFeatures2>({
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
			.features = {
				.shaderInt64 = true
			}
		});

		builder.AddFeature<VkPhysicalDeviceVulkan11Features>({
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
			.variablePointersStorageBuffer = true,
			.variablePointers = true
		});

		builder.AddFeature<VkPhysicalDeviceVulkan12Features>({
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
			.descriptorBindingSampledImageUpdateAfterBind = true,
			.descriptorBindingStorageImageUpdateAfterBind = true,
			.descriptorBindingPartiallyBound = true,
			.runtimeDescriptorArray = true,
			.scalarBlockLayout = true,
			.timelineSemaphore = true,
			.bufferDeviceAddress = true
		});

		builder.AddFeature<VkPhysicalDeviceVulkan13Features>({
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
			.synchronization2 = true,
			.dynamicRendering = true
		});

		if (g_ShaderObjectsSupported)
		{
			builder.AddExtension(VK_EXT_SHADER_OBJECT_EXTENSION_NAME);
			builder.AddFeature<VkPhysicalDeviceShaderObjectFeaturesEXT>({
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
				.shaderObject = true
			});
		}
		else if (g_GPLSupported)
		{
			builder.AddExtension(VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME);
			builder.AddExtension(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
			builder.AddFeature<VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT>({
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT,
				.graphicsPipelineLibrary = true
			});
		}

		if (g_HostImageSupported)
		{
			builder.AddExtension(VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME);
			builder.AddFeature<VkPhysicalDeviceHostImageCopyFeaturesEXT>(VkPhysicalDeviceHostImageCopyFeaturesEXT {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_FEATURES_EXT,
				.hostImageCopy = true
			});
		}

		VkCheck(Config, vkCreateDevice(ChosenGPU, PtrTo(VkDeviceCreateInfo {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = builder.GetChain(),
			.queueCreateInfoCount = (uint32_t)queueInfos.size(),
			.pQueueCreateInfos = queueInfos.data(),
			.enabledExtensionCount = (uint32_t)builder.extensions.size(),
			.ppEnabledExtensionNames = builder.extensions.data()
		}), nullptr, &Device));
		volkLoadDevice(Device);

		VkQueue graphicsQueue;
		VkQueue transferQueue;
		vkGetDeviceQueue(Device, graphicsQueueFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(Device, transferQueueFamily, 0, &transferQueue);

		GraphicsQueue.pimpl = std::make_unique<Impl<Queue>>(this, graphicsQueue, graphicsQueueFamily, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
		TransferQueue.pimpl = std::make_unique<Impl<Queue>>(this, transferQueue, transferQueueFamily, VK_QUEUE_TRANSFER_BIT);
	}

	void Impl<GraphicsContext>::SetupBindless()
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(ChosenGPU, &properties);

		struct BindingInfo
		{
			VkDescriptorType Type;
			uint32_t Count;
			uint32_t Binding;
		};

		const auto bindingInfos = std::to_array<BindingInfo>({
			{ VK_DESCRIPTOR_TYPE_SAMPLER, properties.limits.maxSamplerAllocationCount, SAMPLER_BINDING },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, properties.limits.maxDescriptorSetStorageImages, STORAGE_IMAGE_BINDING },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, properties.limits.maxDescriptorSetSampledImages, SAMPLED_IMAGE_BINDING }
		});

		std::vector<VkDescriptorPoolSize> poolSizes;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		std::vector<VkDescriptorBindingFlags> bindingFlags;

		for (auto& bindingInfo : bindingInfos)
		{
			poolSizes.push_back({ bindingInfo.Type, bindingInfo.Count });
			bindings.push_back({
				.binding = bindingInfo.Binding,
				.descriptorType = bindingInfo.Type,
				.descriptorCount = bindingInfo.Count,
				.stageFlags = VK_SHADER_STAGE_ALL
			});

			bindingFlags.push_back(VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
		}

		VkCheck(Config, vkCreateDescriptorPool(Device, PtrTo(VkDescriptorPoolCreateInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
			.maxSets = 1,
			.poolSizeCount = (uint32_t)poolSizes.size(),
			.pPoolSizes = poolSizes.data()
		}), nullptr, &BindlessPool));

		VkCheck(Config, vkCreateDescriptorSetLayout(Device, PtrTo(VkDescriptorSetLayoutCreateInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = PtrTo(VkDescriptorSetLayoutBindingFlagsCreateInfo {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
				.bindingCount = (uint32_t)bindingFlags.size(),
				.pBindingFlags = bindingFlags.data()
			}),
			.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
			.bindingCount = (uint32_t)bindings.size(),
			.pBindings = bindings.data()
		}), nullptr, &BindlessSetLayout));

		VkCheck(Config, vkAllocateDescriptorSets(Device, PtrTo(VkDescriptorSetAllocateInfo {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = BindlessPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &BindlessSetLayout
		}), &BindlessSet));

		VkCheck(Config, vkCreatePipelineLayout(Device, PtrTo(VkPipelineLayoutCreateInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &BindlessSetLayout,
			.pushConstantRangeCount = (uint32_t)PUSH_CONSTANT_RANGES.size(),
			.pPushConstantRanges = PUSH_CONSTANT_RANGES.data()
		}), nullptr, &BindlessPipelineLayout));

		if (Config.Debug)
		{
			VkDebugUtilsObjectNameInfoEXT debugNameInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };

			debugNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL;
			debugNameInfo.objectHandle = (uint64_t)BindlessPool;
			debugNameInfo.pObjectName = "Bindless Pool";
			VkCheck(Config, vkSetDebugUtilsObjectNameEXT(Device, &debugNameInfo));

			debugNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
			debugNameInfo.objectHandle = (uint64_t)BindlessSetLayout;
			debugNameInfo.pObjectName = "Bindless Set Layout";
			VkCheck(Config, vkSetDebugUtilsObjectNameEXT(Device, &debugNameInfo));

			debugNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
			debugNameInfo.objectHandle = (uint64_t)BindlessSet;
			debugNameInfo.pObjectName = "Bindless Set";
			VkCheck(Config, vkSetDebugUtilsObjectNameEXT(Device, &debugNameInfo));

			debugNameInfo.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
			debugNameInfo.objectHandle = (uint64_t)BindlessPipelineLayout;
			debugNameInfo.pObjectName = "Bindless Pipeline Layout";
			VkCheck(Config, vkSetDebugUtilsObjectNameEXT(Device, &debugNameInfo));
		}
	}

	VkDebugUtilsMessengerCreateInfoEXT Impl<GraphicsContext>::DebugMessengerInfo()
	{
		return {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = debugCallback,
			.pUserData = &Config.Callback
		};
	}

}