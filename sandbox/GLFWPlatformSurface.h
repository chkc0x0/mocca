#pragma once
#include "input/InputTypes.h"
#include "platform/PlatformSurface.h"
#include "core/Surface.h"
#include "text/FontTypes.h"
#include <unordered_map>
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

	void BeginFrame() override;
	void EndFrame() override;
	auto Size() -> mocca::IVec2 override;
	auto FramebufferSize() -> mocca::IVec2 override;
    auto Scale() -> mocca::Vector2 override;

private:
	static auto ToKeyCode(int glfwKey) -> mocca::KeyCode;
	auto FontIdFor(mocca::mtext::FontFaceId id) -> int;

	GLFWwindow* _window = nullptr;
	NVGcontext* _vg = nullptr;
	bool _glfwRefHeld = false;
	mocca::Surface* _surface = nullptr;

	std::unordered_map<mocca::mtext::FontFaceId, int> _nvgFonts;

	mocca::InputBatch _pending;
};
