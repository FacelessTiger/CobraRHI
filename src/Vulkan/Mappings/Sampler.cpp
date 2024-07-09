#include "VulkanRHI.h"

namespace Cobra {

	Sampler::Sampler(GraphicsContext& context, Filter min, Filter mag)
	{
		pimpl = std::make_unique<Impl<Sampler>>(context, min, mag);
	}

	Sampler::~Sampler() { }
	Sampler::Sampler(Sampler&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; }
	Sampler& Sampler::operator=(Sampler&& other) noexcept { pimpl = std::move(other.pimpl); other.pimpl = nullptr; return *this; }

	uint32_t Sampler::GetHandle() const { return pimpl->Handle.GetPendingValue(); }

	Impl<Sampler>::Impl(GraphicsContext& context, Filter min, Filter mag)
		: Context(context.pimpl)
	{
		VkCheck(Context->Config, vkCreateSampler(Context->Device, PtrTo(VkSamplerCreateInfo {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = Utils::CBFilterToVulkan(mag),
			.minFilter = Utils::CBFilterToVulkan(min)
		}), nullptr, &Sampler));

		// Update descriptor
		vkUpdateDescriptorSets(Context->Device, 1, PtrTo(VkWriteDescriptorSet {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = Context->BindlessSet,
			.dstBinding = SAMPLER_BINDING,
			.dstArrayElement = Handle.GetPendingValue(),
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
			.pImageInfo = PtrTo(VkDescriptorImageInfo {
				.sampler = Sampler
			})
		}), 0, nullptr);
	}

	Impl<Sampler>::~Impl()
	{
		Context->DeletionQueues->Push(Context->GraphicsQueue.pimpl->Fence.GetPendingValue(), Sampler);
	}

}