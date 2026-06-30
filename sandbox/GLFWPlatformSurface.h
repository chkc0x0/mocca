#pragma once
#include "InputTypes.h"
#include "PlatformSurface.h"
#include "Surface.h"
#include <vector>

struct GLFWwindow;
struct NVGcontext;

namespace mocca
{
	class Surface;
	struct IVec2;
}

class GLFWPlatformSurface : public mocca::PlatformSurface
{
public:
	explicit GLFWPlatformSurface(
		mocca::Surface* surface,
		const mocca::SurfaceDesc& desc
	);
	~GLFWPlatformSurface() override;

	void CollectEvents(mocca::InputBatch& batch) override;
	auto ShouldClose() -> bool override;
	void Submit(const std::vector<mocca::cmds::DrawCommand>& commands) override;

	[[nodiscard]] auto GetSize() const -> mocca::IVec2;

	static void OnMouseMove(GLFWwindow* window, double x, double y);
	static void OnScroll(GLFWwindow* window, double xoffset, double yoffset);
	static void
	OnMouseButton(GLFWwindow* window, int button, int action, int mods);
	static void
	OnKey(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void OnChar(GLFWwindow* window, unsigned int codepoint);
	static void OnWindowResize(GLFWwindow* window, int width, int height);
	static void OnWindowClose(GLFWwindow* window);
	static void OnWindowFocus(GLFWwindow* window, int focused);

private:
	static auto ToKeyCode(int glfwKey) -> mocca::KeyCode;

	GLFWwindow* _window = nullptr;
	NVGcontext* _vg = nullptr;
	mocca::IVec2 _size = {.X = 0, .Y = 0};
	bool _glfwRefHeld = false;
	mocca::Surface* _surface;

	mocca::InputBatch _pending;
};
