#pragma once
#include "DrawCommands.h"
#include "Texture.h"
#include "utils/Math.h"
#include <span>

namespace mocca::gfx
{
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

		virtual void Render(const FrameInfo& frame) = 0;
	};
}