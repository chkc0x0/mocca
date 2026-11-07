#pragma once
#include "gfx/GfxTypes.h"
#include "gfx/Matrix.h"
#include "gfx/Paint.h"
#include "gfx/Texture.h"
#include "text/TextLayout.h"
#include "utils/Math.h"
#include <memory>
#include <string>
#include <variant>

namespace mocca::cmds
{
	struct DrawRectCmd
	{
	public:
		Rectangle Rect;
		gfx::Paint Paint;
	};

	struct DrawRRectCmd
	{
	public:
		RRect RRect;
		gfx::Paint Paint;
	};

	struct DrawDRRectCmd
	{
	public:
		RRect Outer;
		RRect Inner;
		gfx::Paint Paint;
	};

	struct DrawImageCmd
	{
	public:
		RRect Dst;
		gfx::TextureId Texture;
		gfx::Paint Paint;
		gfx::Sampling Sampling{};
	};

	struct DrawTextCmd
	{
	public:
		Vector2 Position;
		std::string Content;
		gfx::Paint Paint;
		float Size;
		mtext::FontFaceId Face = static_cast<mtext::FontFaceId>(-1);
		std::shared_ptr<const mtext::TextLayout> Layout;
		float BoxWidth = 0;
		styles::TextAlignment Align = styles::TextAlignment::Start;
	};

	struct SaveCmd
	{
	};

	struct RestoreCmd
	{
	};

	struct ConcatCmd
	{
	public:
		gfx::Matrix Matrix;
	};

	struct ClipRectCmd
	{
	public:
		Rectangle Rect;
		gfx::ClipOp Op = gfx::ClipOp::Intersect;
	};

	struct ClipRRectCmd
	{
	public:
		RRect RRect;
		gfx::ClipOp Op = gfx::ClipOp::Intersect;
	};

	using DrawCommand = std::variant<
		DrawRectCmd,
		DrawRRectCmd,
		DrawDRRectCmd,
		DrawImageCmd,
		DrawTextCmd,
		SaveCmd,
		RestoreCmd,
		ConcatCmd,
		ClipRectCmd,
		ClipRRectCmd>;
}
