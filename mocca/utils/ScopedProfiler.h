#pragma once
#ifdef MC_TRACY
#	include <tracy/Tracy.hpp>

#	define mc_profileScope(name) ZoneScopedN(name)
#	define mc_profileFunction() ZoneScoped
#	define mc_profileFrame() FrameMark

#else

#	define mc_profileScope(name) ((void)0)
#	define mc_profileFunction() ((void)0)
#	define mc_profileFrame() ((void)0)

#endif
