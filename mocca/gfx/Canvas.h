#pragma once
#include "DrawCommands.h"
#include <vector>

namespace mocca
{
	struct RectStyle
	{
		cmds::CornerRadii Radius{};
		cmds::EdgeSizes Border{};
		Color BorderColor = colors::Transparent;
	};

	class Canvas
	{
	public:
		void DrawRect(
			float x,
			float y,
			float w,
			float h,
			Color color,
			const RectStyle& style = {}
		);
		void DrawImage(cmds::DrawImageCmd cmd);
		void DrawText(cmds::DrawTextCmd cmd);
		void PushClip(float x, float y, float w, float h);
		void PopClip();
		void PushTransform(float dx, float dy);
		void PopTransform();
		[[nodiscard]] auto Commands() const
			-> const std::vector<cmds::DrawCommand>&;
		void Clear();
		void Append(const Canvas& other);

	private:
		std::vector<cmds::DrawCommand> _commands;
		std::vector<Rectangle> _clipStack;
		std::vector<Vector2> _transformStack;
	};
}