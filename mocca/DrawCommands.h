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

	struct PushTransformCmd
	{
		Vector2 Offset;
	};

	struct PopTransformCmd
	{
	};

	using DrawCommand =
		std::variant<DrawRectCmd, DrawTextCmd, PushClipCmd, PopClipCmd, PushTransformCmd, PopTransformCmd>;
}