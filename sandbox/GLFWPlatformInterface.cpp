#include "GLFWPlatformInterface.h"
#include "GLFW/glfw3.h"
#include "GLFWPlatformSurface.h"
#include "core/Logger.h"
#include "gfx/GfxTypes.h"
#include <cstring>

namespace sandbox
{
	auto GLFWPlatformClipboard::Formats() -> std::vector<std::string>
	{
		if (glfwGetClipboardString(glfwGetCurrentContext()) == nullptr)
		{
			return {};
		}

		return {"text/plain"};
	}

	auto GLFWPlatformClipboard::Get(std::string_view mime)
		-> std::optional<std::vector<uint8_t>>
	{
		if (mime != "text/plain")
		{
			return std::nullopt;
		}

		const char* text = glfwGetClipboardString(glfwGetCurrentContext());
		if (text == nullptr)
		{
			return std::nullopt;
		}

		const auto* begin = reinterpret_cast<const uint8_t*>(text);
		return std::vector<uint8_t>(begin, begin + std::strlen(text));
	}

	void GLFWPlatformClipboard::Set(
		const std::vector<mocca::platform::ClipboardEntry>& offers
	)
	{
		for (const auto& offer : offers)
		{
			if (offer.Mime != "text/plain")
			{
				continue;
			}

			std::string text(offer.Data.begin(), offer.Data.end());
			glfwSetClipboardString(glfwGetCurrentContext(), text.c_str());
			return;
		}
	}

	GLFWPlatformInterface::GLFWPlatformInterface()
	{
		PlatformClipboard = std::make_unique<GLFWPlatformClipboard>();
	}

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