#pragma once
#include <cstdint>
#include <string_view>
#include <vector>

namespace mocca::mtext::detail
{
	enum class BreakFlag : uint8_t
	{
		No = 0,
		Allow = 1,
		Must = 2,
	};

	[[nodiscard]] auto breakOpportunities(std::string_view utf8)
		-> std::vector<BreakFlag>;
}
