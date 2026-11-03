#ifdef TRACY_ENABLE

#	include <algorithm>
#	include <tracy/Tracy.hpp>

#	include <cstddef>
#	include <cstdio>
#	include <cstdlib>
#	include <new>

#	if defined(__GLIBC__)
#		include <malloc.h>
#	endif

#	ifndef MC_TRACY_ALLOC_DEPTH
#		define MC_TRACY_ALLOC_DEPTH 12
#	endif

namespace
{
	inline auto Track(void* p, std::size_t n) -> void*
	{
		if (p != nullptr)
		{
#	if MC_TRACY_ALLOC_DEPTH > 0
			TracyAllocS(p, n, MC_TRACY_ALLOC_DEPTH);
#	else
			TracyAlloc(p, n);
#	endif
		}
		return p;
	}

	inline void Untrack(void* p)
	{
		if (p != nullptr)
		{
#	if MC_TRACY_ALLOC_DEPTH > 0
			TracyFreeS(p, MC_TRACY_ALLOC_DEPTH);
#	else
			TracyFree(p);
#	endif
		}
	}

	inline auto RawAlloc(std::size_t n) -> void*
	{
		return std::malloc(n == 0 ? 1 : n);
	}

	inline auto RawAlignedAlloc(std::size_t n, std::size_t align) -> void*
	{
		align = std::max(align, sizeof(void*));
		void* p = nullptr;
		if (posix_memalign(&p, align, n == 0 ? 1 : n) != 0)
		{
			return nullptr;
		}
		return p;
	}
}

namespace
{
	auto NewImpl(std::size_t n) -> void*
	{
		for (;;)
		{
			if (void* p = RawAlloc(n))
			{
				return Track(p, n);
			}
			std::new_handler h = std::get_new_handler();
			if (h == nullptr)
			{
				throw std::bad_alloc();
			}
			h();
		}
	}

	auto NewAlignedImpl(std::size_t n, std::size_t align) -> void*
	{
		for (;;)
		{
			if (void* p = RawAlignedAlloc(n, align))
			{
				return Track(p, n);
			}
			std::new_handler h = std::get_new_handler();
			if (h == nullptr)
			{
				throw std::bad_alloc();
			}
			h();
		}
	}

	auto NewNoThrowImpl(std::size_t n) noexcept -> void*
	{
		try
		{
			return NewImpl(n);
		}
		catch (...)
		{
			return nullptr;
		}
	}

	auto NewAlignedNoThrowImpl(std::size_t n, std::size_t align) noexcept
		-> void*
	{
		try
		{
			return NewAlignedImpl(n, align);
		}
		catch (...)
		{
			return nullptr;
		}
	}

	void DeleteImpl(void* p) noexcept
	{
		Untrack(p);
		std::free(p);
	}
}

auto operator new(std::size_t n) -> void*
{
	return NewImpl(n);
}
auto operator new[](std::size_t n) -> void*
{
	return NewImpl(n);
}

void operator delete(void* p) noexcept
{
	DeleteImpl(p);
}
void operator delete[](void* p) noexcept
{
	DeleteImpl(p);
}
void operator delete(void* p, std::size_t) noexcept
{
	DeleteImpl(p);
}
void operator delete[](void* p, std::size_t) noexcept
{
	DeleteImpl(p);
}

auto operator new(std::size_t n, const std::nothrow_t&) noexcept -> void*
{
	return NewNoThrowImpl(n);
}
auto operator new[](std::size_t n, const std::nothrow_t&) noexcept -> void*
{
	return NewNoThrowImpl(n);
}
void operator delete(void* p, const std::nothrow_t&) noexcept
{
	DeleteImpl(p);
}
void operator delete[](void* p, const std::nothrow_t&) noexcept
{
	DeleteImpl(p);
}

auto operator new(std::size_t n, std::align_val_t a) -> void*
{
	return NewAlignedImpl(n, static_cast<std::size_t>(a));
}
auto operator new[](std::size_t n, std::align_val_t a) -> void*
{
	return NewAlignedImpl(n, static_cast<std::size_t>(a));
}
auto operator new(
	std::size_t n,
	std::align_val_t a,
	const std::nothrow_t&
) noexcept -> void*
{
	return NewAlignedNoThrowImpl(n, static_cast<std::size_t>(a));
}
auto operator new[](
	std::size_t n,
	std::align_val_t a,
	const std::nothrow_t&
) noexcept -> void*
{
	return NewAlignedNoThrowImpl(n, static_cast<std::size_t>(a));
}

void operator delete(void* p, std::align_val_t) noexcept
{
	DeleteImpl(p);
}
void operator delete[](void* p, std::align_val_t) noexcept
{
	DeleteImpl(p);
}
void operator delete(void* p, std::size_t, std::align_val_t) noexcept
{
	DeleteImpl(p);
}
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept
{
	DeleteImpl(p);
}
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept
{
	DeleteImpl(p);
}
void operator delete[](
	void* p,
	std::align_val_t,
	const std::nothrow_t&
) noexcept
{
	DeleteImpl(p);
}

namespace mocca::detail
{
	auto readRssBytes() -> double
	{
		std::FILE* f = std::fopen("/proc/self/statm", "r");
		if (f == nullptr)
		{
			return 0.0;
		}
		unsigned long size = 0;
		unsigned long resident = 0;
		int got = std::fscanf(f, "%lu %lu", &size, &resident);
		std::fclose(f);
		if (got != 2)
		{
			return 0.0;
		}
		return static_cast<double>(resident) * 4096.0;
	}

	void plotMemory()
	{
#	if defined(__GLIBC__) &&                                                  \
		(__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 33))
		struct mallinfo2 mi = mallinfo2();
		
		TracyPlot("heap in use", static_cast<double>(mi.uordblks));
		TracyPlot("heap free", static_cast<double>(mi.fordblks));
#	endif
		TracyPlot("rss", readRssBytes());
	}
}

#else

namespace mocca::detail
{
	void PlotMemory()
	{
	}
}

#endif