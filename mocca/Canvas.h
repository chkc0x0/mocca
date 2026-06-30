#pragma once
#include "DrawCommands.h"
#include <utility>
#include <vector>

namespace mocca
{
	class Canvas
	{
	public:
		void DrawRect(float x, float y, float w, float h, Color color)
		{
			_commands.emplace_back(
				cmds::DrawRectCmd{
					.Rect = {.X = x, .Y = y, .Width = w, .Height = h},
					.Color = color
				}
			);
		}

		void DrawText(float x, float y, std::string text, Color color)
		{
			_commands.emplace_back(
				cmds::DrawTextCmd{
					.Position = {.X = x, .Y = y},
					.Content = std::move(text),
					.Color = color
				}
			);
		}

		void PushClip(float x, float y, float w, float h)
		{
			Rectangle rect{.X = x, .Y = y, .Width = w, .Height = h};
			if (!_clipStack.empty())
			{
				rect = _clipStack.back().Intersect(rect);
			}
			_clipStack.push_back(rect);
			_commands.emplace_back(cmds::PushClipCmd{rect});
		}

		void PopClip()
		{
			if (!_clipStack.empty())
			{
				_clipStack.pop_back();
			}
			_commands.emplace_back(cmds::PopClipCmd{});
		}

		[[nodiscard]] auto Commands() const
			-> const std::vector<cmds::DrawCommand>&
		{
			return _commands;
		}

		void Clear()
		{
			_commands.clear();
			_clipStack.clear();
		}

	private:
		std::vector<cmds::DrawCommand> _commands;
		std::vector<Rectangle> _clipStack;
	};
}