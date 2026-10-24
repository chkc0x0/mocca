#include "GLFWPlatformSurface.h"
#include "core/Logger.h"
#include "gfx/GfxTypes.h"
#include "text/Unicode.h"
#include "utils/Math.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "GL/glew.h"

static int glfwInitialized = 0;

GLFWPlatformSurface::GLFWPlatformSurface(
	mocca::Surface* surface,
	const mocca::SurfaceDesc& desc
)
{
	(void)surface;

	if (glfwInitialized == 0)
	{
		if (glfwInit() == GLFW_FALSE)
		{
			mc_error(
				mocca::ErrorCode::InvalidState,
				"failed to initialize GLFW"
			);
			return;
		}
	}
	++glfwInitialized;
	_glfwRefHeld = true;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);

	_window = glfwCreateWindow(
		desc.Width,
		desc.Height,
		desc.Title.c_str(),
		nullptr,
		nullptr
	);
	if (_window == nullptr)
	{
		mc_error(
			mocca::ErrorCode::InvalidState,
			"failed to create GLFW window"
		);
		return;
	}

	glfwMakeContextCurrent(_window);
	glfwSwapInterval(1);

	glewInit();

	glfwSetWindowUserPointer(_window, this);
	glfwSetCursorPosCallback(_window, OnMouseMove);
	glfwSetMouseButtonCallback(_window, OnMouseButton);
	glfwSetKeyCallback(_window, OnKey);
	glfwSetCharCallback(_window, OnChar);
	glfwSetWindowSizeCallback(_window, OnWindowResize);
	glfwSetWindowCloseCallback(_window, OnWindowClose);
	glfwSetWindowFocusCallback(_window, OnWindowFocus);
	glfwSetScrollCallback(_window, OnScroll);
}

GLFWPlatformSurface::~GLFWPlatformSurface()
{
	if (_window != nullptr)
	{
		glfwMakeContextCurrent(_window);
	}

	if (_window != nullptr)
	{
		glfwMakeContextCurrent(nullptr);
		glfwDestroyWindow(_window);
		_window = nullptr;
	}

	if (_glfwRefHeld && --glfwInitialized == 0)
	{
		glfwTerminate();
	}
}

void GLFWPlatformSurface::CollectEvents(mocca::InputBatch& batch)
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

auto GLFWPlatformSurface::ShouldClose() -> bool
{
	if (_window == nullptr)
	{
		return true;
	}

	return glfwWindowShouldClose(_window) != 0;
}

void GLFWPlatformSurface::BeginFrame()
{
	glfwMakeContextCurrent(_window);

	auto size = FramebufferSize();
	glViewport(0, 0, size.X, size.Y);

	glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void GLFWPlatformSurface::EndFrame()
{
	glfwSwapBuffers(_window);
}

auto GLFWPlatformSurface::Size() -> mocca::IVec2
{
	int w;
	int h;
	glfwGetWindowSize(_window, &w, &h);
	return {.X=w, .Y=h};
}

auto GLFWPlatformSurface::FramebufferSize() -> mocca::IVec2
{
	int w;
	int h;
	glfwGetFramebufferSize(_window, &w, &h);
	return {.X = w, .Y = h};
}

auto GLFWPlatformSurface::Scale() -> mocca::Vector2
{
	float scaleX = 1.0F;
	float scaleY = 1.0F;
	glfwGetWindowContentScale(_window, &scaleX, &scaleY);
	return {.X=scaleX, .Y=scaleY};
}

void GLFWPlatformSurface::OnMouseMove(GLFWwindow* window, double x, double y)
{
	auto* self = static_cast<GLFWPlatformSurface*>(
		glfwGetWindowUserPointer(window)
	);
	self->_pending.Pointer.push_back({
		.EventType = mocca::PointerEvent::Type::Move,
		.X = static_cast<float>(x),
		.Y = static_cast<float>(y),
	});
}

void GLFWPlatformSurface::OnMouseButton(
	GLFWwindow* window,
	int button,
	int action,
	int mods
)
{
	auto* self = static_cast<GLFWPlatformSurface*>(
		glfwGetWindowUserPointer(window)
	);
	(void)mods;

	double x;
	double y;
	glfwGetCursorPos(window, &x, &y);

	if (action == GLFW_PRESS)
	{
		self->_pending.Pointer.push_back({
			.EventType = mocca::PointerEvent::Type::Down,
			.X = static_cast<float>(x),
			.Y = static_cast<float>(y),
			.Button = button,
		});
	}
	else if (action == GLFW_RELEASE)
	{
		self->_pending.Pointer.push_back({
			.EventType = mocca::PointerEvent::Type::Up,
			.X = static_cast<float>(x),
			.Y = static_cast<float>(y),
			.Button = button,
		});
	}
}

void GLFWPlatformSurface::OnScroll(
	GLFWwindow* window,
	double xoffset,
	double yoffset
)
{
	auto* self = static_cast<GLFWPlatformSurface*>(
		glfwGetWindowUserPointer(window)
	);
	double x;
	double y;
	glfwGetCursorPos(window, &x, &y);
	self->_pending.Pointer.push_back({
		.EventType = mocca::PointerEvent::Type::Scroll,
		.X = static_cast<float>(x),
		.Y = static_cast<float>(y),
		.ScrollX = static_cast<float>(xoffset),
		.ScrollY = static_cast<float>(yoffset),
	});
}

void GLFWPlatformSurface::OnKey(
	GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mods
)
{
	auto* self = static_cast<GLFWPlatformSurface*>(
		glfwGetWindowUserPointer(window)
	);
	(void)scancode;
	(void)mods;

	if (action == GLFW_PRESS)
	{
		self->_pending.Keyboard.push_back({
			.EventType = mocca::KeyEvent::Type::Down,
			.Code = ToKeyCode(key),
		});
	}
	if (action == GLFW_REPEAT)
	{
		self->_pending.Keyboard.push_back({
			.EventType = mocca::KeyEvent::Type::Repeat,
			.Code = ToKeyCode(key),
		});
	}
	else if (action == GLFW_RELEASE)
	{
		self->_pending.Keyboard.push_back({
			.EventType = mocca::KeyEvent::Type::Up,
			.Code = ToKeyCode(key),
		});
	}
}

void GLFWPlatformSurface::OnChar(GLFWwindow* window, unsigned int codepoint)
{
	auto* self = static_cast<GLFWPlatformSurface*>(
		glfwGetWindowUserPointer(window)
	);
	self->_pending.Text.push_back({
		.EventType = mocca::TextEvent::Type::Commit,
		.Text = mocca::mtext::unicode::encode(static_cast<char32_t>(codepoint)),
	});
}

void GLFWPlatformSurface::OnWindowResize(
	GLFWwindow* window,
	int width,
	int height
)
{
	auto* self = static_cast<GLFWPlatformSurface*>(
		glfwGetWindowUserPointer(window)
	);
	self->_pending.Surface.push_back({
		.EventType = mocca::SurfaceEvent::Type::Resize,
		.Data1 = (float)width,
		.Data2 = (float)height,
	});
}

void GLFWPlatformSurface::OnWindowClose(GLFWwindow* window)
{
	auto* self = static_cast<GLFWPlatformSurface*>(
		glfwGetWindowUserPointer(window)
	);
	self->_pending.Surface.push_back({
		.EventType = mocca::SurfaceEvent::Type::Close,
	});
}

void GLFWPlatformSurface::OnWindowFocus(GLFWwindow* window, int focused)
{
	auto* self = static_cast<GLFWPlatformSurface*>(
		glfwGetWindowUserPointer(window)
	);
	self->_pending.Surface.push_back({
		.EventType = (focused != 0) ? mocca::SurfaceEvent::Type::FocusGained
									: mocca::SurfaceEvent::Type::FocusLost,
	});
}

auto GLFWPlatformSurface::ToKeyCode(int glfwKey) -> mocca::KeyCode
{
	switch (glfwKey)
	{
	case GLFW_KEY_A:
		return mocca::KeyCode::A;
	case GLFW_KEY_B:
		return mocca::KeyCode::B;
	case GLFW_KEY_C:
		return mocca::KeyCode::C;
	case GLFW_KEY_D:
		return mocca::KeyCode::D;
	case GLFW_KEY_E:
		return mocca::KeyCode::E;
	case GLFW_KEY_F:
		return mocca::KeyCode::F;
	case GLFW_KEY_G:
		return mocca::KeyCode::G;
	case GLFW_KEY_H:
		return mocca::KeyCode::H;
	case GLFW_KEY_I:
		return mocca::KeyCode::I;
	case GLFW_KEY_J:
		return mocca::KeyCode::J;
	case GLFW_KEY_K:
		return mocca::KeyCode::K;
	case GLFW_KEY_L:
		return mocca::KeyCode::L;
	case GLFW_KEY_M:
		return mocca::KeyCode::M;
	case GLFW_KEY_N:
		return mocca::KeyCode::N;
	case GLFW_KEY_O:
		return mocca::KeyCode::O;
	case GLFW_KEY_P:
		return mocca::KeyCode::P;
	case GLFW_KEY_Q:
		return mocca::KeyCode::Q;
	case GLFW_KEY_R:
		return mocca::KeyCode::R;
	case GLFW_KEY_S:
		return mocca::KeyCode::S;
	case GLFW_KEY_T:
		return mocca::KeyCode::T;
	case GLFW_KEY_U:
		return mocca::KeyCode::U;
	case GLFW_KEY_V:
		return mocca::KeyCode::V;
	case GLFW_KEY_W:
		return mocca::KeyCode::W;
	case GLFW_KEY_X:
		return mocca::KeyCode::X;
	case GLFW_KEY_Y:
		return mocca::KeyCode::Y;
	case GLFW_KEY_Z:
		return mocca::KeyCode::Z;
	case GLFW_KEY_0:
		return mocca::KeyCode::Num0;
	case GLFW_KEY_1:
		return mocca::KeyCode::Num1;
	case GLFW_KEY_2:
		return mocca::KeyCode::Num2;
	case GLFW_KEY_3:
		return mocca::KeyCode::Num3;
	case GLFW_KEY_4:
		return mocca::KeyCode::Num4;
	case GLFW_KEY_5:
		return mocca::KeyCode::Num5;
	case GLFW_KEY_6:
		return mocca::KeyCode::Num6;
	case GLFW_KEY_7:
		return mocca::KeyCode::Num7;
	case GLFW_KEY_8:
		return mocca::KeyCode::Num8;
	case GLFW_KEY_9:
		return mocca::KeyCode::Num9;
	case GLFW_KEY_ESCAPE:
		return mocca::KeyCode::Escape;
	case GLFW_KEY_ENTER:
		return mocca::KeyCode::Enter;
	case GLFW_KEY_TAB:
		return mocca::KeyCode::Tab;
	case GLFW_KEY_BACKSPACE:
		return mocca::KeyCode::Backspace;
	case GLFW_KEY_SPACE:
		return mocca::KeyCode::Space;
	case GLFW_KEY_LEFT:
		return mocca::KeyCode::Left;
	case GLFW_KEY_RIGHT:
		return mocca::KeyCode::Right;
	case GLFW_KEY_UP:
		return mocca::KeyCode::Up;
	case GLFW_KEY_DOWN:
		return mocca::KeyCode::Down;
	case GLFW_KEY_LEFT_SHIFT:
		return mocca::KeyCode::LShift;
	case GLFW_KEY_RIGHT_SHIFT:
		return mocca::KeyCode::RShift;
	case GLFW_KEY_LEFT_CONTROL:
		return mocca::KeyCode::LCtrl;
	case GLFW_KEY_RIGHT_CONTROL:
		return mocca::KeyCode::RCtrl;
	case GLFW_KEY_LEFT_ALT:
		return mocca::KeyCode::LAlt;
	case GLFW_KEY_RIGHT_ALT:
		return mocca::KeyCode::RAlt;
	default:
		return mocca::KeyCode::Unknown;
	}
}

auto GLFWPlatformSurface::GfxHandle() -> mocca::gfx::GraphicsHandle
{
	return {
		.Api = mocca::gfx::GraphicsApi::OpenGL,
		.GetProcAddress =
			[](const char* name) -> void*
		{ return reinterpret_cast<void*>(glfwGetProcAddress(name)); },
		.Native = _window,
	};
}
