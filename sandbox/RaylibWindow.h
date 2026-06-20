#pragma once
#include "Surface.h"
#include "raylib.h"

class RaylibWindow : public mocca::Surface
{
public:
	RaylibWindow(const mocca::SurfaceDesc& desc) : mocca::Surface(desc)
	{
		InitWindow(desc.Width, desc.Height, desc.Title.c_str());
	}

	~RaylibWindow()
	{
		CloseWindow();
	}

	void Update() override
	{
		BeginDrawing();

		for (const auto& command : GetDrawData())
		{
			switch (command.index())
			{
			case 0:
			{
				auto cmd = std::get<mocca::cmds::DrawRectCmd>(command);

				DrawRectangle(
					cmd.Rect.Position.X,
					cmd.Rect.Position.Y,
					cmd.Rect.Width,
					cmd.Rect.Height,
					{.r = cmd.Color.R,
					 .g = cmd.Color.G,
					 .b = cmd.Color.B,
					 .a = 255}
				);

				DrawRectangleLines(
					cmd.Rect.Position.X,
					cmd.Rect.Position.Y,
					cmd.Rect.Width,
					cmd.Rect.Height,
					{.r = 255, .g = 0, .b = 0, .a = 255}
				);

				/*
mc_info(
					"rect({}, {}, {}, {})",
					cmd.Rect.Position.X,
					cmd.Rect.Position.Y,
					cmd.Rect.Width,
					cmd.Rect.Height
				);*/
				break;
			}

			case 1:
			{
				auto cmd = std::get<mocca::cmds::DrawTextCmd>(command);

				DrawText(
					cmd.Content.c_str(),
					cmd.Position.X,
					cmd.Position.Y,
					16,
					{.r = cmd.Color.R,
					 .g = cmd.Color.G,
					 .b = cmd.Color.B,
					 .a = 255}
				);

				/*mc_info("text({},{},{},{})", cmd.Position.X, cmd.Position.Y, 8 * cmd.Content.size(), 16);*/
				break;
			}

			default:
				break;
			}
		}

		EndDrawing();
	}

	auto IsRunning() -> bool override
	{
		return !WindowShouldClose();
	}
};