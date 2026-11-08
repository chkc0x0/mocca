#pragma once
#include "core/Surface.h"
#include "input/InputTypes.h"
#include "platform/PlatformSurface.h"
#include <array>
#include <cstddef>
#include <cstdint>

struct SDL_Window;
struct SDL_Cursor;
union SDL_Event;

namespace sandbox
{
	class SDL3PlatformInterface;
}

class SDL3PlatformSurface : public mocca::PlatformSurface
{
public:
	SDL3PlatformSurface(
		sandbox::SDL3PlatformInterface& owner,
		const mocca::SurfaceDesc& desc
	);
	~SDL3PlatformSurface() override;

	void CollectEvents(mocca::InputBatch& batch) override;
	auto ShouldClose() -> bool override;

	void HandleEvent(const SDL_Event& event);
	void PushClose();

	void BeginFrame() override;
	void EndFrame() override;
	auto Size() -> mocca::IVec2 override;
	auto FramebufferSize() -> mocca::IVec2 override;
	auto Scale() -> mocca::Vector2 override;

	auto GfxHandle() -> mocca::gfx::GraphicsHandle override;

	void SetCursor(mocca::CursorShapeType shape) override;
	void SetImeCursorRect(mocca::Rectangle rect) override;

private:
	static auto ToKeyCode(int scancode) -> mocca::KeyCode;
	static auto ToModifiers(std::uint32_t mods) -> mocca::Modifiers;
	auto _cursorFor(mocca::CursorShapeType shape) -> SDL_Cursor*;

	static constexpr std::size_t CursorCount =
		static_cast<std::size_t>(mocca::CursorShapeType::AllResize) + 1;

	sandbox::SDL3PlatformInterface& _owner;
	SDL_Window* _window = nullptr;
	void* _glContext = nullptr;
	std::uint32_t _windowId = 0;
	bool _shouldClose = false;

	std::array<SDL_Cursor*, CursorCount> _cursors{};
	mocca::CursorShapeType _cursor = mocca::CursorShapeType::Default;

	mocca::InputBatch _pending;
};
