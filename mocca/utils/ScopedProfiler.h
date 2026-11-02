#pragma once
#include <unordered_map>
#ifdef MC_TRACY
#	include <tracy/Tracy.hpp>

namespace mocca::detail
{
	class ProfilerPlot
	{
	public:
		struct Entry
		{
			int64_t Value;
			uint64_t Gen;
		};

		static auto Plot(std::string_view str) -> int64_t
		{
			auto& e = data[str];

			if (e.Gen != gen)
			{
				e.Value = 0;
				e.Gen = gen;
			}

			return ++e.Value;
		}

		static void Reset()
		{
			gen++;
		}

	private:
		inline static std::unordered_map<std::string_view, Entry> data;
		inline static uint32_t gen = 0;
	};
}

#	define mc_profileScope(name) ZoneScopedN(name)
#	define mc_profileFunction() ZoneScoped
#	define mc_profileFrame() FrameMark
#	define mc_profilePlot(name)                                               \
		TracyPlot(name, mocca::detail::ProfilerPlot::Plot(name))
#	define mc_profilePlotFunction() mc_profilePlot(__PRETTY_FUNCTION__)

#else

#	define mc_profileScope(name) ((void)0)
#	define mc_profileFunction() ((void)0)
#	define mc_profileFrame() ((void)0)

#endif
