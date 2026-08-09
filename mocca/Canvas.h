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
			Vector2 currentOffset = _transformStack.empty()
										? Vector2{0, 0}
										: _transformStack.back();
			Rectangle rect{
				.X = currentOffset.X + x,
				.Y = currentOffset.Y + y,
				.Width = w,
				.Height = h
			};
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

		void PushTransform(float dx, float dy)
		{
			Vector2 currentOffset = _transformStack.empty()
										? Vector2{.X = 0, .Y = 0}
										: _transformStack.back();
			Vector2 newOffset = {
				.X = currentOffset.X + dx,
				.Y = currentOffset.Y + dy
			};
			_transformStack.push_back(newOffset);
			_commands.emplace_back(
				cmds::PushTransformCmd{Vector2{.X = dx, .Y = dy}}
			);
		}

		void PopTransform()
		{
			if (!_transformStack.empty())
			{
				_transformStack.pop_back();
			}
			_commands.emplace_back(cmds::PopTransformCmd{});
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
			_transformStack.clear();
		}

		void Append(const Canvas& other)
		{
			_commands.insert(
				_commands.end(),
				other._commands.begin(),
				other._commands.end()
			);
		}

	private:
		std::vector<cmds::DrawCommand> _commands;
		std::vector<Rectangle> _clipStack;
		std::vector<Vector2> _transformStack;
	};
}