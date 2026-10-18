#pragma once
#ifdef MC_PROFILER
#	include <cstdint>

namespace mocca::detail
{
	class Profiler
	{
	public:
		struct Slot
		{
			const char* Name;
			const char* File;
			int Line;
		};

		class Scope
		{
		public:
			explicit Scope(const Slot* slot);
			~Scope();

			Scope(const Scope&) = delete;
			auto operator=(const Scope&) -> Scope& = delete;

		private:
			void* _node = nullptr;
			uint64_t _start = 0;
		};

		static void Frame();
		static void Report();
	};
}

#	define mc_profCat_(a, b) a##b
#	define mc_profCat(a, b) mc_profCat_(a, b)

#	define mc_profileScope(name)                                              \
		static constexpr ::mocca::detail::Profiler::Slot mc_profCat(          \
			mc_prof_slot_,                                                     \
			__LINE__                                                           \
		){name, __FILE__, __LINE__};                                           \
		::mocca::detail::Profiler::Scope mc_profCat(mc_prof_scope_, __LINE__)  \
		{                                                                      \
			&mc_profCat(mc_prof_slot_, __LINE__)                               \
		}

#	define mc_profileFunction() mc_profileScope(__func__)
#	define mc_profileFrame() ::mocca::detail::Profiler::Frame()

#else

#	define mc_profileScope(name) ((void)0)
#	define mc_profileFunction() ((void)0)
#	define mc_profileFrame() ((void)0)

#endif
