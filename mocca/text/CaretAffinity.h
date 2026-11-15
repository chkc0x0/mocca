#pragma once
#include <cstdint>

namespace mocca::mtext
{
	enum class CaretAffinity : uint8_t
	{
		Downstream,
		Upstream,
	};
}
