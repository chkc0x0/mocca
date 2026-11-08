#include "SDL3PlatformInterface.h"
#include "SDL3PlatformSurface.h"
#include "core/Logger.h"
#include "gfx/GfxTypes.h"
#include <SDL3/SDL.h>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace sandbox
{
	auto SDL3PlatformClipboard::Formats() -> std::vector<std::string>
	{
		if (!SDL_HasClipboardText())
		{
			return {};
		}

		return {"text/plain"};
	}

	auto SDL3PlatformClipboard::Get(std::string_view mime)
		-> std::optional<std::vector<uint8_t>>
	{
		if (mime != "text/plain")
		{
			return std::nullopt;
		}

		char* text = SDL_GetClipboardText();
		if (text == nullptr)
		{
			return std::nullopt;
		}

		const auto* begin = reinterpret_cast<const uint8_t*>(text);
		std::vector<uint8_t> data(begin, begin + std::strlen(text));
		SDL_free(text);
		return data;
	}

	void SDL3PlatformClipboard::Set(
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
			SDL_SetClipboardText(text.c_str());
			return;
		}
	}

	SDL3PlatformInterface::SDL3PlatformInterface()
	{
		PlatClipboard = std::make_unique<SDL3PlatformClipboard>();
	}

	SDL3PlatformInterface::~SDL3PlatformInterface()
	{
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}

	void SDL3PlatformInterface::PumpEvents()
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			_dispatch(event);
		}
	}

	void SDL3PlatformInterface::WaitEvents()
	{
		SDL_Event event;
		if (SDL_WaitEvent(&event))
		{
			_dispatch(event);
		}

		while (SDL_PollEvent(&event))
		{
			_dispatch(event);
		}
	}

	void SDL3PlatformInterface::Wake()
	{
		SDL_Event event{};
		event.type = SDL_EVENT_USER;
		SDL_PushEvent(&event);
	}

	void SDL3PlatformInterface::RegisterSurface(
		std::uint32_t windowId,
		::SDL3PlatformSurface* surface
	)
	{
		_surfaces[windowId] = surface;
	}

	void SDL3PlatformInterface::UnregisterSurface(std::uint32_t windowId)
	{
		_surfaces.erase(windowId);
	}

	auto SDL3PlatformInterface::CreateSurface(
		const mocca::SurfaceDesc& desc,
		const mocca::gfx::GraphicsRequirements& requirements
	) -> std::unique_ptr<mocca::PlatformSurface>
	{
		mc_assert(
			requirements.Api == mocca::gfx::GraphicsApi::OpenGL,
			"sdl3 only supports gl"
		);

		SDL_GL_SetAttribute(
			SDL_GL_CONTEXT_MAJOR_VERSION,
			requirements.Major
		);
		SDL_GL_SetAttribute(
			SDL_GL_CONTEXT_MINOR_VERSION,
			requirements.Minor
		);
		if ((requirements.Flags & mocca::gfx::GraphicsFlags::Core) != 0U)
		{
			SDL_GL_SetAttribute(
				SDL_GL_CONTEXT_PROFILE_MASK,
				SDL_GL_CONTEXT_PROFILE_CORE
			);
		}

		return std::make_unique<::SDL3PlatformSurface>(*this, desc);
	}

	void SDL3PlatformInterface::_dispatch(const SDL_Event& event)
	{
		if (event.type == SDL_EVENT_QUIT)
		{
			for (auto& entry : _surfaces)
			{
				if (entry.second != nullptr)
				{
					entry.second->PushClose();
				}
			}
			return;
		}

		std::uint32_t windowId = 0;
		switch (event.type)
		{
		case SDL_EVENT_MOUSE_MOTION:
			windowId = event.motion.windowID;
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
			windowId = event.button.windowID;
			break;
		case SDL_EVENT_MOUSE_WHEEL:
			windowId = event.wheel.windowID;
			break;
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			windowId = event.key.windowID;
			break;
		case SDL_EVENT_TEXT_INPUT:
			windowId = event.text.windowID;
			break;
		case SDL_EVENT_TEXT_EDITING:
			windowId = event.edit.windowID;
			break;
		case SDL_EVENT_WINDOW_RESIZED:
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
		case SDL_EVENT_WINDOW_FOCUS_GAINED:
		case SDL_EVENT_WINDOW_FOCUS_LOST:
		case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
			windowId = event.window.windowID;
			break;
		default:
			return;
		}

		auto found = _surfaces.find(windowId);
		if (found != _surfaces.end() && found->second != nullptr)
		{
			found->second->HandleEvent(event);
		}
	}

	auto createSDL3PlatformInterface()
		-> std::unique_ptr<mocca::platform::PlatformInterface>
	{
		if (!SDL_Init(SDL_INIT_VIDEO))
		{
			mc_error(
				mocca::ErrorCode::InvalidState,
				"failed to initialize SDL3: {}",
				SDL_GetError()
			);
			return nullptr;
		}

		return std::make_unique<SDL3PlatformInterface>();
	}
}
