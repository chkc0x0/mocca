#pragma once
#include "a11y/Snapshot.h"
#include <cstdint>
#include <memory>

namespace mocca::aria
{
	struct NativeWindow
	{
		enum class Kind : uint8_t
		{
			Unix,
			Win32,
			MacOS,
		};

		Kind Kind = Kind::Unix;
		void* Handle = nullptr; // hwnd or nswindow*
		const char* MacWindowClass = nullptr;
		bool Focused = false;
		float Scale = 1.0F;
	};

	auto createAccessKitBridge(Provider& provider, const NativeWindow& window)
		-> std::unique_ptr<Bridge>;

#ifdef MC_A11Y_ENABLED
	auto accessKitRole(Role role) -> uint8_t;
#endif
}
