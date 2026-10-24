#include "GLFWPlatformInterface.h"
#include "GLFW/glfw3.h"
#include "GLFWPlatformSurface.h"
#include "core/Logger.h"
#include "gfx/GfxTypes.h"

namespace sandbox
{
	GLFWPlatformInterface::~GLFWPlatformInterface()
	{
		glfwTerminate();
	}

	void GLFWPlatformInterface::PumpEvents()
	{
		glfwPollEvents();
	}

	void GLFWPlatformInterface::WaitEvents()
	{
		glfwWaitEvents();
	}

	void GLFWPlatformInterface::Wake()
	{
		glfwPostEmptyEvent();
	}

	auto GLFWPlatformInterface::CreateSurface(
		const mocca::SurfaceDesc& desc,
		const mocca::gfx::GraphicsRequirements& requirements
	) -> std::unique_ptr<mocca::PlatformSurface>
	{
		mc_assert(
			requirements.Api == mocca::gfx::GraphicsApi::OpenGL,
			"glfw only supports gl"
		);

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, requirements.Major);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, requirements.Minor);
		if ((requirements.Flags & mocca::gfx::GraphicsFlags::Core) != 0U)
		{
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		}

		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);

		return std::make_unique<GLFWPlatformSurface>(desc);
	}

	auto createGLFWPlatformInterface()
		-> std::unique_ptr<mocca::platform::PlatformInterface>
	{
		if (glfwInit() == GLFW_FALSE)
		{
			mc_error(
				mocca::ErrorCode::InvalidState,
				"failed to initialize GLFW"
			);
			return nullptr;
		}

		return std::make_unique<GLFWPlatformInterface>();
	}
}