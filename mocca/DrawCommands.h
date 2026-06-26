#pragma once
#include "Math.h"
#include <string>
#include <variant>

namespace mocca::cmds
{
	struct DrawRectCmd
	{
	public:
		Rectangle Rect;
		struct Color Color;
	};

	struct DrawTextCmd
	{
	public:
		Vector2 Position;
		std::string Content;
		struct Color Color;
	};

	using DrawCommand = std::variant<DrawRectCmd, DrawTextCmd>;
}