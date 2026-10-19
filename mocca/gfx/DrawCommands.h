#pragma once
#include "text/TextLayout.h"
#include "utils/Color.h"
#include "utils/Math.h"
#include <memory>
#include <string>
#include <variant>

namespace mocca::cmds
{
	struct CornerRadii
	{
	public:
		float TopLeft;
		float TopRight;
		float BottomRight;
		float BottomLeft;
	};

	struct EdgeSizes
	{
	public:
		float Top;
		float Right;
		float Bottom;
		float Left;
	};

	struct DrawRectCmd
	{
	public:
		Rectangle Rect;
		CornerRadii Radius;
		EdgeSizes Border;
		struct Color Color;
		struct Color BorderColor;
	};

	struct DrawTextCmd
	{
	public:
		Vector2 Position;
		std::string Content;
		struct Color Color;
		float Size;
		mtext::FontFaceId Face = static_cast<mtext::FontFaceId>(-1);
		std::shared_ptr<const mtext::TextLayout> Layout;
		float BoxWidth = 0;
		TextAlignment Align = TextAlignment::Start;
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

	using DrawCommand = std::variant<
		DrawRectCmd,
		DrawTextCmd,
		PushClipCmd,
		PopClipCmd,
		PushTransformCmd,
		PopTransformCmd>;
}