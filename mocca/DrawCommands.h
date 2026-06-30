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

	struct PushClipCmd
	{
		Rectangle Rect;
	};

	struct PopClipCmd
	{
	};

	using DrawCommand =
		std::variant<DrawRectCmd, DrawTextCmd, PushClipCmd, PopClipCmd>;
}