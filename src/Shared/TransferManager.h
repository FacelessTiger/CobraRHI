#pragma once

#include <CobraRHI.h>

#include <memory>

namespace Cobra {

	class TransferManager
	{
	public:
		TransferManager(GraphicsContext& context, bool skipBufferCreation);

		void Transfer(const Cobra::Image& image, std::span<std::byte> data);

		Queue& GetQueue() { return m_Queue; }
	private:
		GraphicsContext& m_Context;
		Queue& m_Queue;

		std::unique_ptr<Buffer> m_Staging;
	};

}