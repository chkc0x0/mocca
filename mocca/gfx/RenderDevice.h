#pragma once
#include "GfxTypes.h"
#include "gfx/Capabilities.h"
#include "gfx/DrawCommands.h"
#include "utils/Math.h"
#include <memory>
#include <span>

namespace mocca
{
	class Surface;
}

namespace mocca::gfx
{
	class Renderer;

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

	struct FrameInfo
	{
	public:
		std::span<const cmds::DrawCommand> Commands;
		std::span<const Texture* const> Textures;
		Vector2 ViewportSize;
		Vector2 Scale;
	};

	class Renderer
	{
	public:
		virtual ~Renderer() = default;

		[[nodiscard]] virtual auto Capabilities() const
			-> const Capabilities& = 0;

		virtual void Render(const FrameInfo& frame) = 0;
	};
}