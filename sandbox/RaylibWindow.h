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
		EndDrawing();
	}

	auto IsRunning() -> bool override
	{
		return !WindowShouldClose();
	}
};