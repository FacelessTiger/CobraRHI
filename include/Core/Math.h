#pragma once

#include <cstdint>

namespace Cobra {

	struct iVec2
	{
		union { uint32_t x, r; };
		union { uint32_t y, g; };

		iVec2(uint32_t x, uint32_t y)
			: x(x), y(y)
		{ }
	};

}