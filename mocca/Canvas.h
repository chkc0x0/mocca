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
			_commands.emplace_back(cmds::DrawRectCmd{
				.Rect = {.Position = {.X = x, .Y = y}, .Width = w, .Height = h},
				.Color = color});
		}

		void DrawText(float x, float y, std::string text, Color color)
		{
			_commands.emplace_back(cmds::DrawTextCmd{.Position = {.X = x, .Y = y},
												  .Content = std::move(text),
												  .Color = color});
		}

		[[nodiscard]] auto Commands() const -> const std::vector<cmds::DrawCommand>&
		{
			return _commands;
		}
		
		void Clear()
		{
			_commands.clear();
		}

	private:
		std::vector<cmds::DrawCommand> _commands;
	};
}