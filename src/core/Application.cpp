#include "Application.h"
#include "Detail.h"
#include "Logger.h"
#include <utility>

namespace mocca
{
	Application::Application(const std::string& appId)
	{
		if (main != nullptr)
		{
			mc_error(
				ErrorCode::InvalidState,
				"an application instance already exists"
			);
			return;
		}

		_id = ApplicationID(appId);
		main = this;

		_context._store.SetMarkDirty(
			[this](detail::NodeId id) -> void
			{
				_context._store.InsertDirty(id);
				Surface* owner = nullptr;

				for (auto& surface : _surfaces)
				{
					owner = surface->FindSurfaceContaining(id);
					if (owner != nullptr)
					{
						owner->MarkDirty();
						return;
					}
				}

				if (owner == nullptr)
				{
					owner = _context._currentSurface;
				}

				if (owner != nullptr)
				{
					owner->MarkDirty();
				}
			}
		);
	}

	Application::~Application()
	{
		if (main == this)
		{
			main = nullptr;
		}
	}

	void Application::Tick(double dt)
	{
		EmitEvent(ApplicationEvent::Poll, nullptr);

		for (auto& surface : _surfaces)
		{
			surface->Tick(dt);
		}

		std::erase_if(
			_surfaces,
			[](const auto& s) -> auto
			{ return s->GetState() == SurfaceState::Dead; }
		);

		_context._store.ClearDirty();
		_context._store.FlushEffects();
	}

	void Application::On(
		std::string_view event,
		std::function<bool(void*, void*)> cb,
		void* userData
	)
	{
		_events[detail::hashString(event)].push_back(
			{.Callback = std::move(cb), .User = userData}
		);
	}

	void Application::On(
		ApplicationEvent event,
		std::function<bool(void*, void*)> cb,
		void* userData
	)
	{
		_events[(uint64_t)event].push_back(
			{.Callback = std::move(cb), .User = userData}
		);
	}

	auto Application::EmitEvent(std::string_view event, void* data) -> bool
	{
		auto hash = detail::hashString(event);
		if (!_events.contains(hash))
		{
			return true;
		}

		return std::ranges::all_of(
			_events[hash],
			[data](const auto& cb) -> auto
			{ return cb.Callback(data, cb.User); }
		);

		return true;
	}

	auto Application::EmitEvent(ApplicationEvent event, void* data) -> bool
	{
		if (!_events.contains((uint64_t)event))
		{
			return true;
		}

		return std::ranges::all_of(
			_events[(uint64_t)event],
			[data](const auto& cb) -> auto
			{ return cb.Callback(data, cb.User); }
		);
	}

	void Application::RemoveCallbacks(std::string_view event)
	{
		if (!_events.contains(detail::hashString(event)))
		{
			return;
		}

		_events[detail::hashString(event)].clear();
	}

	auto Application::RegisterSurface(const SurfaceDesc& desc) -> Surface*
	{
		auto surface = std::make_unique<Surface>(desc);
		surface->_rootId = detail::nextNodeId++;
		auto* ptr = surface.get();
		if (!EmitEvent(ApplicationEvent::SurfaceCreated, ptr))
		{
			return nullptr;
		}

		if (ptr->IsPlatformBacked())
		{
			_platformSurfaces.push_back(ptr);
		}

		if (desc.Parent != nullptr)
		{
			ptr->_desc.Parent = desc.Parent;
			desc.Parent->_children.push_back(std::move(surface));
		}
		else
		{
			_surfaces.push_back(std::move(surface));
		}
		return ptr;
	}

	constexpr auto ApplicationID::ValidateID(std::string_view id) -> bool
	{
		if (id.empty())
		{
			return false;
		}

		int segments = 0;
		int segmentLen = 0;

		for (char c : id)
		{
			if (c == '.')
			{
				if (segmentLen == 0)
				{
					return false;
				}
				segments++;
				segmentLen = 0;
			}
			else if ((std::isalnum(c) != 0) || c == '_' || c == '-')
			{
				segmentLen++;
			}
			else
			{
				return false;
			}
		}

		if (segmentLen == 0)
		{
			return false;
		}
		segments++;

		return segments >= 2 && segments <= 3;
	}

	ApplicationID::ApplicationID(const std::string& id)
	{
		mc_assert(ValidateID(id), "invalid application id {}", id);

		_id = id;

		std::vector<std::string_view> segments;
		{
			std::string_view sv(_id);
			size_t start = 0;
			while (true)
			{
				size_t pos = sv.find('.', start);
				if (pos == std::string_view::npos)
				{
					segments.emplace_back(sv.substr(start));
					break;
				}
				segments.emplace_back(sv.substr(start, pos - start));
				start = pos + 1;
			}
		}

		if (segments.size() == 2)
		{
			Organization = segments[0];
			Name = segments[1];
			Domain = "com";
		}
		else
		{
			Domain = segments[0];
			Organization = segments[1];
			Name = segments[2];
		}
	}

	auto ApplicationID::GetCompoundID() const -> std::string
	{
		return std::string(Domain) + "." + std::string(Organization) + "." +
			   std::string(Name);
	}

	void Application::Print() const
	{
		mc_info("[Application id={}]", GetAppID().GetCompoundID());

		for (const auto& surface : _surfaces)
		{
			surface->Print(1);
		}
	}

	auto getCtx() -> Context*
	{
		if (Application::main == nullptr)
		{
			mc_error(ErrorCode::InvalidState, "no application instance exists");
			return nullptr;
		}
		return &Application::main->_context;
	}
}