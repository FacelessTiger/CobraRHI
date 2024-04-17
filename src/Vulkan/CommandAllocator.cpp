#include "VulkanRHI.h"

namespace Cobra {

	CommandAllocator::CommandAllocator(GraphicsContext& context, const Queue& queue)
	{
		pimpl = std::make_unique<Impl<CommandAllocator>>(context, queue);
	}

	CommandAllocator::~CommandAllocator() { }
	CommandAllocator::CommandAllocator(CommandAllocator&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	CommandAllocator& CommandAllocator::operator=(CommandAllocator&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	CommandAllocator& CommandAllocator::Reset()
	{
		VkCheck(pimpl->Context->Callback, vkResetCommandPool(pimpl->Context->Device, pimpl->CommandPool, 0));
		return *this;
	}

	CommandList CommandAllocator::Begin()
	{
		VkCommandBufferBeginInfo info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

		VkCheck(pimpl->Context->Callback, vkBeginCommandBuffer(pimpl->MainCommandBuffer, &info));
		vkCmdBindDescriptorSets(pimpl->MainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pimpl->Context->BindlessPipelineLayout, 0, 1, &pimpl->Context->BindlessSet, 0, nullptr);
		vkCmdBindDescriptorSets(pimpl->MainCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pimpl->Context->BindlessPipelineLayout, 0, 1, &pimpl->Context->BindlessSet, 0, nullptr);

		CommandList list;
		list.pimpl = std::make_unique<Impl<CommandList>>(pimpl->Context, pimpl->MainCommandBuffer);
		return list;
	}

	Impl<CommandAllocator>::Impl(GraphicsContext& context, const Queue& queue)
		: Context(context.pimpl)
	{
		VkCommandPoolCreateInfo commandPoolInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.queueFamilyIndex = queue.pimpl->QueueFamily
		};

		VkCheck(Context->Callback, vkCreateCommandPool(Context->Device, &commandPoolInfo, nullptr, &CommandPool));

		VkCommandBufferAllocateInfo cmdAllocInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = CommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		VkCheck(Context->Callback, vkAllocateCommandBuffers(Context->Device, &cmdAllocInfo, &MainCommandBuffer));
	}

	Impl<CommandAllocator>::~Impl()
	{
		// TODO: Push to deletion queue
		vkDestroyCommandPool(Context->Device, CommandPool, nullptr);
	}

}