#include "LineBreak.h"
#include "linebreak.h"

namespace mocca::mtext::detail
{
	auto breakOpportunities(std::string_view utf8) -> std::vector<BreakFlag>
	{
		std::vector<BreakFlag> out(utf8.size() + 1, BreakFlag::No);
		if (utf8.empty())
		{
			return out;
		}

		static const bool Initialized = [] -> bool
		{
			init_linebreak();
			return true;
		}();
		(void)Initialized;

		std::vector<char> flags(utf8.size(), LINEBREAK_NOBREAK);
		set_linebreaks_utf8(
			reinterpret_cast<const utf8_t*>(utf8.data()),
			utf8.size(),
			"en",
			flags.data()
		);

		for (size_t i = 0; i < utf8.size(); i++)
		{
			// libunibreak reports after byte i
			switch (flags[i])
			{
			case LINEBREAK_MUSTBREAK:
				out[i + 1] = BreakFlag::Must;
				break;
			case LINEBREAK_ALLOWBREAK:
				out[i + 1] = BreakFlag::Allow;
				break;
			default:
				break;
			}
		}

		return out;
	}
}
