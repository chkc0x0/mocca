#pragma once
#include "core/Surface.h"
#include "gfx/GfxTypes.h"
#include "platform/Clipboard.h"
#include "platform/PlatformSurface.h"
#include <memory>

namespace mocca::platform
{
	class PlatformInterface
	{
	public:
		virtual ~PlatformInterface() = default;

		virtual auto CreateSurface(
			const SurfaceDesc& desc,
			const gfx::GraphicsRequirements& requirements
		) -> std::unique_ptr<PlatformSurface> = 0;

		virtual void PumpEvents() = 0;
		virtual void WaitEvents() = 0;
		virtual void Wake() = 0;

		auto Clipboard() -> PlatformClipboard*
		{
			return PlatformClipboard.get();
		}

	protected:
		std::unique_ptr<PlatformClipboard> PlatformClipboard =
			std::make_unique<platform::PlatformClipboard>();
	};
}