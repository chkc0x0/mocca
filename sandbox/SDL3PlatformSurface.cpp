#include "SDL3PlatformSurface.h"
#include "SDL3PlatformInterface.h"
#include "a11y/AccessKitBridge.h"
#include "core/Logger.h"
#include "gfx/GfxTypes.h"
#include "utils/Math.h"
#include <SDL3/SDL.h>
#include <string_view>
#include "GL/glew.h"

SDL3PlatformSurface::SDL3PlatformSurface(
	sandbox::SDL3PlatformInterface& owner,
	const mocca::SurfaceDesc& desc
) :
	_owner(owner)
{
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
							SDL_WINDOW_HIGH_PIXEL_DENSITY;
#if defined(MC_A11Y_ENABLED) && (defined(_WIN32) || defined(__APPLE__))
	// native adapters must be installed before the window is shown
	flags |= SDL_WINDOW_HIDDEN;
#endif

	_window = SDL_CreateWindow(
		desc.Title.c_str(),
		static_cast<int>(desc.Width),
		static_cast<int>(desc.Height),
		flags
	);
	if (_window == nullptr)
	{
		mc_error(
			mocca::ErrorCode::InvalidState,
			"failed to create SDL3 window: {}",
			SDL_GetError()
		);
		return;
	}

	if (desc.X >= 0.0F && desc.Y >= 0.0F)
	{
		SDL_SetWindowPosition(
			_window,
			static_cast<int>(desc.X),
			static_cast<int>(desc.Y)
		);
	}

	_glContext = SDL_GL_CreateContext(_window);
	if (_glContext == nullptr)
	{
		mc_error(
			mocca::ErrorCode::InvalidState,
			"failed to create SDL3 GL context: {}",
			SDL_GetError()
		);
		SDL_DestroyWindow(_window);
		_window = nullptr;
		return;
	}

	SDL_GL_MakeCurrent(_window, static_cast<SDL_GLContext>(_glContext));
	SDL_GL_SetSwapInterval(1);
	glewInit();
	SDL_StartTextInput(_window);

	_windowId = SDL_GetWindowID(_window);
	_owner.RegisterSurface(_windowId, this);
}

SDL3PlatformSurface::~SDL3PlatformSurface()
{
	_owner.UnregisterSurface(_windowId);

	for (auto* cursor : _cursors)
	{
		if (cursor != nullptr)
		{
			SDL_DestroyCursor(cursor);
		}
	}

	if (_glContext != nullptr)
	{
		SDL_GL_MakeCurrent(_window, nullptr);
		SDL_GL_DestroyContext(static_cast<SDL_GLContext>(_glContext));
		_glContext = nullptr;
	}

	if (_window != nullptr)
	{
		SDL_DestroyWindow(_window);
		_window = nullptr;
	}
}

void SDL3PlatformSurface::CollectEvents(mocca::InputBatch& batch)
{
	if (_window == nullptr)
	{
		_pending.Surface.push_back({
			.EventType = mocca::SurfaceEvent::Type::Close,
		});
	}

	batch = std::move(_pending);
	_pending.Clear();
}

auto SDL3PlatformSurface::ShouldClose() -> bool
{
	return _window == nullptr || _shouldClose;
}

void SDL3PlatformSurface::PushClose()
{
	_shouldClose = true;
	_pending.Surface.push_back({
		.EventType = mocca::SurfaceEvent::Type::Close,
	});
}

void SDL3PlatformSurface::HandleEvent(const SDL_Event& event)
{
	switch (event.type)
	{
	case SDL_EVENT_MOUSE_MOTION:
		_pending.Pointer.push_back({
			.EventType = mocca::PointerEvent::Type::Move,
			.X = event.motion.x,
			.Y = event.motion.y,
			.Mods = ToModifiers(SDL_GetModState()),
		});
		break;
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP:
		_pending.Pointer.push_back({
			.EventType = event.button.down ? mocca::PointerEvent::Type::Down
										   : mocca::PointerEvent::Type::Up,
			.X = event.button.x,
			.Y = event.button.y,
			.Button = static_cast<int>(event.button.button) - 1,
			.Mods = ToModifiers(SDL_GetModState()),
		});
		break;
	case SDL_EVENT_MOUSE_WHEEL:
	{
		float x = event.wheel.x;
		float y = event.wheel.y;
		if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
		{
			x = -x;
			y = -y;
		}

		_pending.Pointer.push_back({
			.EventType = mocca::PointerEvent::Type::Scroll,
			.X = event.wheel.mouse_x,
			.Y = event.wheel.mouse_y,
			.ScrollX = x,
			.ScrollY = y,
			.Mods = ToModifiers(SDL_GetModState()),
		});
		break;
	}
	case SDL_EVENT_KEY_DOWN:
	case SDL_EVENT_KEY_UP:
	{
		auto type = mocca::KeyEvent::Type::Down;
		if (!event.key.down)
		{
			type = mocca::KeyEvent::Type::Up;
		}
		else if (event.key.repeat)
		{
			type = mocca::KeyEvent::Type::Repeat;
		}

		_pending.Keyboard.push_back({
			.EventType = type,
			.Code = ToKeyCode(static_cast<int>(event.key.scancode)),
			.Mods = ToModifiers(event.key.mod),
		});
		break;
	}
	case SDL_EVENT_TEXT_INPUT:
		_pending.Text.push_back({
			.EventType = mocca::TextEvent::Type::Commit,
			.Text = event.text.text != nullptr ? event.text.text : "",
		});
		break;
	case SDL_EVENT_WINDOW_RESIZED:
		_pending.Surface.push_back({
			.EventType = mocca::SurfaceEvent::Type::Resize,
			.Data1 = static_cast<float>(event.window.data1),
			.Data2 = static_cast<float>(event.window.data2),
		});
		_pending.Surface.push_back({
			.EventType = mocca::SurfaceEvent::Type::RootBoundsChanged,
		});
		break;
	case SDL_EVENT_WINDOW_MOVED:
	case SDL_EVENT_WINDOW_MAXIMIZED:
	case SDL_EVENT_WINDOW_RESTORED:
	case SDL_EVENT_WINDOW_SHOWN:
		_pending.Surface.push_back({
			.EventType = mocca::SurfaceEvent::Type::RootBoundsChanged,
		});
		break;
	case SDL_EVENT_WINDOW_MOUSE_ENTER:
	{
		float x = 0.0F;
		float y = 0.0F;
		SDL_GetMouseState(&x, &y);
		_pending.Pointer.push_back({
			.EventType = mocca::PointerEvent::Type::Move,
			.X = x,
			.Y = y,
			.Mods = ToModifiers(SDL_GetModState()),
		});
		break;
	}
	case SDL_EVENT_WINDOW_MOUSE_LEAVE:
		_pending.Pointer.push_back({
			.EventType = mocca::PointerEvent::Type::Leave,
		});
		break;
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
		PushClose();
		break;
	case SDL_EVENT_WINDOW_FOCUS_GAINED:
		_pending.Surface.push_back({
			.EventType = mocca::SurfaceEvent::Type::FocusGained,
		});
		break;
	case SDL_EVENT_WINDOW_FOCUS_LOST:
		_pending.Surface.push_back({
			.EventType = mocca::SurfaceEvent::Type::FocusLost,
		});
		break;
	case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
	{
		auto scale = Scale();
		_pending.Surface.push_back({
			.EventType = mocca::SurfaceEvent::Type::DPIChange,
			.Data1 = scale.X,
			.Data2 = scale.Y,
		});
		break;
	}
	default:
		break;
	}
}

void SDL3PlatformSurface::BeginFrame()
{
	if (_window == nullptr || _glContext == nullptr)
	{
		return;
	}

	SDL_GL_MakeCurrent(_window, static_cast<SDL_GLContext>(_glContext));

	auto size = FramebufferSize();
	glViewport(0, 0, size.X, size.Y);

	glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void SDL3PlatformSurface::EndFrame()
{
	if (_window != nullptr)
	{
		SDL_GL_SwapWindow(_window);
	}
}

auto SDL3PlatformSurface::Size() -> mocca::IVec2
{
	int w = 0;
	int h = 0;
	if (_window != nullptr)
	{
		SDL_GetWindowSize(_window, &w, &h);
	}
	return {.X = w, .Y = h};
}

auto SDL3PlatformSurface::FramebufferSize() -> mocca::IVec2
{
	int w = 0;
	int h = 0;
	if (_window != nullptr)
	{
		SDL_GetWindowSizeInPixels(_window, &w, &h);
	}
	return {.X = w, .Y = h};
}

auto SDL3PlatformSurface::Scale() -> mocca::Vector2
{
	float scale = 1.0F;
	if (_window != nullptr)
	{
		scale = SDL_GetWindowDisplayScale(_window);
	}
	return {.X = scale, .Y = scale};
}

auto SDL3PlatformSurface::GfxHandle() -> mocca::gfx::GraphicsHandle
{
	return {
		.Api = mocca::gfx::GraphicsApi::OpenGL,
		.GetProcAddress = [](const char* name) -> void*
		{ return reinterpret_cast<void*>(SDL_GL_GetProcAddress(name)); },
		.Native = _window,
	};
}

auto SDL3PlatformSurface::ToKeyCode(int scancode) -> mocca::KeyCode
{
	switch (scancode)
	{
	case SDL_SCANCODE_A:
		return mocca::KeyCode::A;
	case SDL_SCANCODE_B:
		return mocca::KeyCode::B;
	case SDL_SCANCODE_C:
		return mocca::KeyCode::C;
	case SDL_SCANCODE_D:
		return mocca::KeyCode::D;
	case SDL_SCANCODE_E:
		return mocca::KeyCode::E;
	case SDL_SCANCODE_F:
		return mocca::KeyCode::F;
	case SDL_SCANCODE_G:
		return mocca::KeyCode::G;
	case SDL_SCANCODE_H:
		return mocca::KeyCode::H;
	case SDL_SCANCODE_I:
		return mocca::KeyCode::I;
	case SDL_SCANCODE_J:
		return mocca::KeyCode::J;
	case SDL_SCANCODE_K:
		return mocca::KeyCode::K;
	case SDL_SCANCODE_L:
		return mocca::KeyCode::L;
	case SDL_SCANCODE_M:
		return mocca::KeyCode::M;
	case SDL_SCANCODE_N:
		return mocca::KeyCode::N;
	case SDL_SCANCODE_O:
		return mocca::KeyCode::O;
	case SDL_SCANCODE_P:
		return mocca::KeyCode::P;
	case SDL_SCANCODE_Q:
		return mocca::KeyCode::Q;
	case SDL_SCANCODE_R:
		return mocca::KeyCode::R;
	case SDL_SCANCODE_S:
		return mocca::KeyCode::S;
	case SDL_SCANCODE_T:
		return mocca::KeyCode::T;
	case SDL_SCANCODE_U:
		return mocca::KeyCode::U;
	case SDL_SCANCODE_V:
		return mocca::KeyCode::V;
	case SDL_SCANCODE_W:
		return mocca::KeyCode::W;
	case SDL_SCANCODE_X:
		return mocca::KeyCode::X;
	case SDL_SCANCODE_Y:
		return mocca::KeyCode::Y;
	case SDL_SCANCODE_Z:
		return mocca::KeyCode::Z;
	case SDL_SCANCODE_0:
		return mocca::KeyCode::Num0;
	case SDL_SCANCODE_1:
		return mocca::KeyCode::Num1;
	case SDL_SCANCODE_2:
		return mocca::KeyCode::Num2;
	case SDL_SCANCODE_3:
		return mocca::KeyCode::Num3;
	case SDL_SCANCODE_4:
		return mocca::KeyCode::Num4;
	case SDL_SCANCODE_5:
		return mocca::KeyCode::Num5;
	case SDL_SCANCODE_6:
		return mocca::KeyCode::Num6;
	case SDL_SCANCODE_7:
		return mocca::KeyCode::Num7;
	case SDL_SCANCODE_8:
		return mocca::KeyCode::Num8;
	case SDL_SCANCODE_9:
		return mocca::KeyCode::Num9;
	case SDL_SCANCODE_GRAVE:
		return mocca::KeyCode::Grave;
	case SDL_SCANCODE_ESCAPE:
		return mocca::KeyCode::Escape;
	case SDL_SCANCODE_RETURN:
	case SDL_SCANCODE_KP_ENTER:
		return mocca::KeyCode::Enter;
	case SDL_SCANCODE_TAB:
		return mocca::KeyCode::Tab;
	case SDL_SCANCODE_BACKSPACE:
		return mocca::KeyCode::Backspace;
	case SDL_SCANCODE_SPACE:
		return mocca::KeyCode::Space;
	case SDL_SCANCODE_INSERT:
		return mocca::KeyCode::Insert;
	case SDL_SCANCODE_DELETE:
		return mocca::KeyCode::Delete;
	case SDL_SCANCODE_HOME:
		return mocca::KeyCode::Home;
	case SDL_SCANCODE_END:
		return mocca::KeyCode::End;
	case SDL_SCANCODE_PAGEUP:
		return mocca::KeyCode::PageUp;
	case SDL_SCANCODE_PAGEDOWN:
		return mocca::KeyCode::PageDown;
	case SDL_SCANCODE_CAPSLOCK:
		return mocca::KeyCode::CapsLock;
	case SDL_SCANCODE_LEFT:
		return mocca::KeyCode::Left;
	case SDL_SCANCODE_RIGHT:
		return mocca::KeyCode::Right;
	case SDL_SCANCODE_UP:
		return mocca::KeyCode::Up;
	case SDL_SCANCODE_DOWN:
		return mocca::KeyCode::Down;
	case SDL_SCANCODE_LSHIFT:
		return mocca::KeyCode::LShift;
	case SDL_SCANCODE_RSHIFT:
		return mocca::KeyCode::RShift;
	case SDL_SCANCODE_LCTRL:
		return mocca::KeyCode::LCtrl;
	case SDL_SCANCODE_RCTRL:
		return mocca::KeyCode::RCtrl;
	case SDL_SCANCODE_LALT:
		return mocca::KeyCode::LAlt;
	case SDL_SCANCODE_RALT:
		return mocca::KeyCode::RAlt;
	case SDL_SCANCODE_LGUI:
		return mocca::KeyCode::LSuper;
	case SDL_SCANCODE_RGUI:
		return mocca::KeyCode::RSuper;
	default:
		return mocca::KeyCode::Unknown;
	}
}

auto SDL3PlatformSurface::ToModifiers(std::uint32_t mods) -> mocca::Modifiers
{
	return {
		.Shift = (mods & SDL_KMOD_SHIFT) != 0,
		.Ctrl = (mods & SDL_KMOD_CTRL) != 0,
		.Alt = (mods & SDL_KMOD_ALT) != 0,
		.Meta = (mods & SDL_KMOD_GUI) != 0,
	};
}

auto SDL3PlatformSurface::_cursorFor(mocca::CursorShapeType shape)
	-> SDL_Cursor*
{
	auto index = static_cast<std::size_t>(shape);
	if (index >= _cursors.size())
	{
		return nullptr;
	}

	if (_cursors[index] != nullptr)
	{
		return _cursors[index];
	}

	auto systemCursor = SDL_SYSTEM_CURSOR_DEFAULT;
	switch (shape)
	{
	case mocca::CursorShapeType::Text:
	case mocca::CursorShapeType::VerticalText:
		systemCursor = SDL_SYSTEM_CURSOR_TEXT;
		break;
	case mocca::CursorShapeType::Crosshair:
	case mocca::CursorShapeType::Cell:
		systemCursor = SDL_SYSTEM_CURSOR_CROSSHAIR;
		break;
	case mocca::CursorShapeType::Pointer:
		systemCursor = SDL_SYSTEM_CURSOR_POINTER;
		break;
	case mocca::CursorShapeType::Progress:
		systemCursor = SDL_SYSTEM_CURSOR_PROGRESS;
		break;
	case mocca::CursorShapeType::Wait:
		systemCursor = SDL_SYSTEM_CURSOR_WAIT;
		break;
	case mocca::CursorShapeType::Move:
	case mocca::CursorShapeType::Grab:
	case mocca::CursorShapeType::Grabbing:
	case mocca::CursorShapeType::ResizeAll:
	case mocca::CursorShapeType::AllResize:
	case mocca::CursorShapeType::ScrollAll:
		systemCursor = SDL_SYSTEM_CURSOR_MOVE;
		break;
	case mocca::CursorShapeType::NoDrop:
	case mocca::CursorShapeType::NotAllowed:
		systemCursor = SDL_SYSTEM_CURSOR_NOT_ALLOWED;
		break;
	case mocca::CursorShapeType::ResizeE:
		systemCursor = SDL_SYSTEM_CURSOR_E_RESIZE;
		break;
	case mocca::CursorShapeType::ResizeN:
		systemCursor = SDL_SYSTEM_CURSOR_N_RESIZE;
		break;
	case mocca::CursorShapeType::ResizeNE:
		systemCursor = SDL_SYSTEM_CURSOR_NE_RESIZE;
		break;
	case mocca::CursorShapeType::ResizeNW:
		systemCursor = SDL_SYSTEM_CURSOR_NW_RESIZE;
		break;
	case mocca::CursorShapeType::ResizeS:
		systemCursor = SDL_SYSTEM_CURSOR_S_RESIZE;
		break;
	case mocca::CursorShapeType::ResizeSE:
		systemCursor = SDL_SYSTEM_CURSOR_SE_RESIZE;
		break;
	case mocca::CursorShapeType::ResizeSW:
		systemCursor = SDL_SYSTEM_CURSOR_SW_RESIZE;
		break;
	case mocca::CursorShapeType::ResizeW:
		systemCursor = SDL_SYSTEM_CURSOR_W_RESIZE;
		break;
	case mocca::CursorShapeType::ResizeH:
	case mocca::CursorShapeType::ResizeCol:
		systemCursor = SDL_SYSTEM_CURSOR_EW_RESIZE;
		break;
	case mocca::CursorShapeType::ResizeV:
	case mocca::CursorShapeType::ResizeRow:
		systemCursor = SDL_SYSTEM_CURSOR_NS_RESIZE;
		break;
	case mocca::CursorShapeType::ResizeNESW:
		systemCursor = SDL_SYSTEM_CURSOR_NESW_RESIZE;
		break;
	case mocca::CursorShapeType::ResizeNWSE:
		systemCursor = SDL_SYSTEM_CURSOR_NWSE_RESIZE;
		break;
	case mocca::CursorShapeType::Default:
	default:
		systemCursor = SDL_SYSTEM_CURSOR_DEFAULT;
		break;
	}

	_cursors[index] = SDL_CreateSystemCursor(systemCursor);
	return _cursors[index];
}

void SDL3PlatformSurface::SetCursor(mocca::CursorShapeType shape)
{
	if (_window == nullptr || shape == _cursor)
	{
		return;
	}

	if (shape == mocca::CursorShapeType::None)
	{
		SDL_HideCursor();
	}
	else
	{
		if (_cursor == mocca::CursorShapeType::None)
		{
			SDL_ShowCursor();
		}

		SDL_SetCursor(_cursorFor(shape));
	}

	_cursor = shape;
}

void SDL3PlatformSurface::SetImeCursorRect(mocca::Rectangle rect)
{
	if (_window == nullptr)
	{
		return;
	}

	SDL_Rect area{
		.x = static_cast<int>(rect.X),
		.y = static_cast<int>(rect.Y),
		.w = static_cast<int>(rect.Width),
		.h = static_cast<int>(rect.Height),
	};
	SDL_SetTextInputArea(_window, &area, 0);
}

auto SDL3PlatformSurface::CreateA11yBridge(mocca::Surface& owner)
	-> std::unique_ptr<mocca::aria::Bridge>
{
#ifdef MC_A11Y_ENABLED
	if (_window == nullptr)
	{
		return nullptr;
	}

	mocca::aria::NativeWindow window{};
	window.Focused =
		(SDL_GetWindowFlags(_window) & SDL_WINDOW_INPUT_FOCUS) != 0;
	window.Scale = Scale().X;

#ifdef _WIN32
	auto props = SDL_GetWindowProperties(_window);
	window.Kind = mocca::aria::NativeWindow::Kind::Win32;
	window.Handle = SDL_GetPointerProperty(
		props,
		SDL_PROP_WINDOW_WIN32_HWND_POINTER,
		nullptr
	);
#elifdef __APPLE__
	auto props = SDL_GetWindowProperties(_window);
	window.Kind = mocca::aria::NativeWindow::Kind::MacOS;
	window.Handle = SDL_GetPointerProperty(
		props,
		SDL_PROP_WINDOW_COCOA_WINDOW_POINTER,
		nullptr
	);
	window.MacWindowClass = "SDL3Window";
#else
	window.Kind = mocca::aria::NativeWindow::Kind::Unix;
#endif

	auto bridge = mocca::aria::createAccessKitBridge(owner, window);

	if ((SDL_GetWindowFlags(_window) & SDL_WINDOW_HIDDEN) != 0)
	{
		SDL_ShowWindow(_window);
	}

	return bridge;
#else
	(void)owner;
	return nullptr;
#endif
}

auto SDL3PlatformSurface::A11yRootBounds()
	-> std::optional<mocca::aria::RootBounds>
{
#ifdef MC_A11Y_ENABLED
	if (_window == nullptr)
	{
		return std::nullopt;
	}

	const char* driver = SDL_GetCurrentVideoDriver();
	if (driver != nullptr && std::string_view{driver} == "wayland")
	{
		return std::nullopt;
	}

	int x = 0;
	int y = 0;
	if (!SDL_GetWindowPosition(_window, &x, &y))
	{
		return std::nullopt;
	}

	int width = 0;
	int height = 0;
	SDL_GetWindowSize(_window, &width, &height);

	int top = 0;
	int left = 0;
	int bottom = 0;
	int right = 0;
	SDL_GetWindowBordersSize(_window, &top, &left, &bottom, &right);

	return mocca::aria::RootBounds{
		.Outer = {
			.X = static_cast<float>(x - left),
			.Y = static_cast<float>(y - top),
			.Width = static_cast<float>(width + left + right),
			.Height = static_cast<float>(height + top + bottom),
		},
		.Inner = {
			.X = static_cast<float>(x),
			.Y = static_cast<float>(y),
			.Width = static_cast<float>(width),
			.Height = static_cast<float>(height),
		},
	};
#else
	return std::nullopt;
#endif
}
