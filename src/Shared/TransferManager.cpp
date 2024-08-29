#include "TransferManager.h"

namespace Cobra {

	TransferManager::TransferManager(GraphicsContext& context, bool skipBufferCreation)
		: m_Context(context), m_Queue(context.GetQueue(QueueType::Transfer))
	{
		// TODO: Mayhe handle buffer transfers?
		if (skipBufferCreation) return;

		m_Staging = std::make_unique<Buffer>(m_Context, 64ull * 1024 * 1024, BufferUsage::TransferSrc, BufferFlags::Mapped);
	}

	void TransferManager::Transfer(const Cobra::Image& image, std::span<std::byte> data)
	{
		if (data.size_bytes() > m_Staging->GetSize())
			m_Staging = std::make_unique<Buffer>(m_Context, data.size_bytes(), BufferUsage::TransferSrc, BufferFlags::Mapped);

		memcpy(m_Staging->GetHostAddress(), data.data(), data.size_bytes());

		auto cmd = m_Queue.Begin();
		cmd.CopyToImage(*m_Staging, image);
		m_Queue.Submit(cmd, {}).Wait();
	}

}