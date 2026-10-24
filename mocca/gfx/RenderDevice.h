#pragma once
#include "GfxTypes.h"
#include "Renderer.h"
#include <memory>

namespace mocca
{
	class Surface;
}

namespace mocca::gfx
{
	class RenderDevice
	{
	public:
		virtual ~RenderDevice() = default;

		[[nodiscard]] virtual auto Requirements() const
			-> GraphicsRequirements = 0;

		// called once at app start, before any renderer is created
		virtual auto Initialize() -> bool
		{
			return true;
		};

		virtual auto CreateRenderer(Surface* surf, const GraphicsHandle& handle)
			-> std::unique_ptr<Renderer> = 0;
	};
}