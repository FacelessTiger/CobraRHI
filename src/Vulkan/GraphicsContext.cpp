#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#define VOLK_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "VulkanRHI.h"

#include <iostream>
#include <vector>
#include <array>

namespace Cobra {

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		DebugCallback callback = (DebugCallback)pUserData;

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) callback(pCallbackData->pMessage, MessageSeverity::Error);
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) callback(pCallbackData->pMessage, MessageSeverity::Warning);

		return false;
	}

	GraphicsContext::GraphicsContext(DebugCallback debugCallback)
	{
		pimpl = std::make_shared<Impl<GraphicsContext>>(debugCallback);
	}

	GraphicsContext::~GraphicsContext() { }
	GraphicsContext::GraphicsContext(GraphicsContext&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	GraphicsContext& GraphicsContext::operator=(GraphicsContext&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	Impl<GraphicsContext>::Impl(Cobra::DebugCallback debugCallback)
		: Callback(debugCallback)
	{
		VkCheck(Callback, volkInitialize());

		CreateInstance();
		if (Callback) CreateDebugMessenger();
		PickGPUAndQueueFamilys();
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

		VmaAllocatorCreateInfo allocatorInfo = {
			.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
			.physicalDevice = ChosenGPU,
			.device = Device,
			.pVulkanFunctions = &functions,
			.instance = Instance
		};
		vmaCreateAllocator(&allocatorInfo, &Allocator);
	}

	Impl<GraphicsContext>::~Impl()
	{
		vkDeviceWaitIdle(Device);

		if (Callback) vkDestroyDebugUtilsMessengerEXT(Instance, Messenger, nullptr);

		vmaDestroyAllocator(Allocator);

		vkDestroyDevice(Device, nullptr);
		vkDestroyInstance(Instance, nullptr);
	}

	void Impl<GraphicsContext>::CreateInstance()
	{
		std::vector<const char*> layers;
		std::vector<const char*> extensions = { VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME };
		
		if (Callback)
		{
			layers.push_back("VK_LAYER_KHRONOS_validation");
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		std::vector<VkValidationFeatureEnableEXT> featuresEnable = {
			VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT, VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
			VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT, VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
		};

		VkApplicationInfo appInfo = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pEngineName = "Cobra",
			.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
			.apiVersion = VK_API_VERSION_1_3
		};

		VkDebugUtilsMessengerCreateInfoEXT messengerInfo = DebugMessengerInfo();
		VkValidationFeaturesEXT features = {
			.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
			.enabledValidationFeatureCount = (uint32_t)featuresEnable.size(),
			.pEnabledValidationFeatures = featuresEnable.data()
		};
		if (Callback) features.pNext = &messengerInfo;

		VkInstanceCreateInfo instanceInfo = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = &features,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = (uint32_t)layers.size(),
			.ppEnabledLayerNames = layers.data(),
			.enabledExtensionCount = (uint32_t)extensions.size(),
			.ppEnabledExtensionNames = extensions.data()
		};
		
		VkCheck(Callback, vkCreateInstance(&instanceInfo, nullptr, &Instance));
		volkLoadInstance(Instance);
	}

	void Impl<GraphicsContext>::CreateDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT info = DebugMessengerInfo();
		VkCheck(Callback, vkCreateDebugUtilsMessengerEXT(Instance, &info, nullptr, &Messenger));
	}

	void Impl<GraphicsContext>::PickGPUAndQueueFamilys()
	{
		uint32_t deviceCount = 0;
		VkCheck(Callback, vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr));
		std::vector<VkPhysicalDevice> devices(deviceCount);
		VkCheck(Callback, vkEnumeratePhysicalDevices(Instance, &deviceCount, devices.data()));

		ChosenGPU = devices[0];

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(ChosenGPU, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(ChosenGPU, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; i++)
		{
			const auto& queueFamily = queueFamilies[i];
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				GraphicsQueueFamily = i;
				break;
			}
		}
	}

	void Impl<GraphicsContext>::CreateDeviceAndQueues()
	{
		std::vector<const char*> extensions = { VK_EXT_SHADER_OBJECT_EXTENSION_NAME, VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo queueInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = GraphicsQueueFamily,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority
		};

		VkPhysicalDeviceShaderObjectFeaturesEXT featuresShaderObject = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
			.shaderObject = true
		};

		VkPhysicalDeviceVulkan11Features features11 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
			.pNext = &featuresShaderObject,
			.variablePointersStorageBuffer = true,
			.variablePointers = true
		};

		VkPhysicalDeviceVulkan12Features features12 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
			.pNext = &features11,
			.descriptorBindingSampledImageUpdateAfterBind = true,
			.descriptorBindingStorageImageUpdateAfterBind = true,
			.descriptorBindingPartiallyBound = true,
			.scalarBlockLayout = true,
			.timelineSemaphore = true,
			.bufferDeviceAddress = true
		};

		VkPhysicalDeviceVulkan13Features features13 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
			.pNext = &features12,
			.synchronization2 = true,
			.dynamicRendering = true
		};

		VkDeviceCreateInfo deviceInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &features13,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &queueInfo,
			.enabledExtensionCount = (uint32_t)extensions.size(),
			.ppEnabledExtensionNames = extensions.data()
		};

		VkCheck(Callback, vkCreateDevice(ChosenGPU, &deviceInfo, nullptr, &Device));
		volkLoadDevice(Device);

		vkGetDeviceQueue(Device, GraphicsQueueFamily, 0, &GraphicsQueue);
	}

	void Impl<GraphicsContext>::SetupBindless()
	{
		uint32_t maxDescriptors = 100000;
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(ChosenGPU, &properties);

		struct BindingInfo
		{
			VkDescriptorType Type;
			uint32_t Count;
			uint32_t Binding;
		};

		const auto bindingInfos = std::to_array<BindingInfo>({
			{ VK_DESCRIPTOR_TYPE_SAMPLER, properties.limits.maxDescriptorSetSamplers, SAMPLER_BINDING },
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

		VkDescriptorPoolCreateInfo poolInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
			.maxSets = 1,
			.poolSizeCount = (uint32_t)poolSizes.size(),
			.pPoolSizes = poolSizes.data()
		};
		VkCheck(Callback, vkCreateDescriptorPool(Device, &poolInfo, nullptr, &BindlessPool));

		VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			.bindingCount = (uint32_t)bindingFlags.size(),
			.pBindingFlags = bindingFlags.data()
		};

		VkDescriptorSetLayoutCreateInfo layoutInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = &bindingFlagsInfo,
			.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
			.bindingCount = (uint32_t)bindings.size(),
			.pBindings = bindings.data()
		};
		VkCheck(Callback, vkCreateDescriptorSetLayout(Device, &layoutInfo, nullptr, &BindlessSetLayout));

		VkDescriptorSetAllocateInfo setInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = BindlessPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &BindlessSetLayout
		};
		VkCheck(Callback, vkAllocateDescriptorSets(Device, &setInfo, &BindlessSet));

		VkPipelineLayoutCreateInfo pipelineInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &BindlessSetLayout,
			.pushConstantRangeCount = (uint32_t)PUSH_CONSTANT_RANGES.size(),
			.pPushConstantRanges = PUSH_CONSTANT_RANGES.data()
		};
		VkCheck(Callback, vkCreatePipelineLayout(Device, &pipelineInfo, nullptr, &BindlessPipelineLayout));
	}

	VkDebugUtilsMessengerCreateInfoEXT Impl<GraphicsContext>::DebugMessengerInfo()
	{
		return {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = debugCallback,
			.pUserData = Callback
		};
	}

}