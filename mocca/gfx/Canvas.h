#pragma once
#include "DrawCommands.h"
#include <vector>

namespace mocca
{
	class Canvas
	{
	public:
		void DrawRect(float x, float y, float w, float h, Color color);
		void DrawText(float x, float y, std::string text, Color color);
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