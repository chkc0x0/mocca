#include "Application.h"
#include "Detail.h"
#include "Logger.h"
#include <utility>

namespace
{
	struct EventEmitScope
	{
		int& Depth;

		explicit EventEmitScope(int& depth) : Depth{depth}
		{
			++Depth;
		}

		~EventEmitScope()
		{
			--Depth;
		}
	};
}

namespace mocca
{
	Application::Application(const std::string& appId)
	{
		mc_assert(
			main == nullptr,
			"an application instance must not already exist"
		);
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
		for (auto& s : _surfaces)
		{
			Surface::AnnounceSubtreeDestroyed(s.get());
			s.reset();
		}

		if (main == this)
		{
			main = nullptr;
		}
	}

	void Application::Tick(double dt)
	{
		_inTick = true;

		std::erase_if(
			_surfaces,
			[](const auto& s) -> auto
			{
				if (s->GetState() == SurfaceState::Dead)
				{
					Surface::AnnounceSubtreeDestroyed(s.get());
					return true;
				}

				return false;
			}
		);

		EmitEvent(ApplicationEvent::Poll, nullptr);

		for (auto& surface : _surfaces)
		{
			surface->Tick(dt);
		}

		_inTick = false;

		_context._store.ClearDirty();
		_context._store.FlushEffects();

		if (_context._store.DirtyCount() != 0)
		{
			if (++_stateOnEffectStreak > 16 && !_stateOnEffectWarned)
			{
				_stateOnEffectWarned = true;
				mc_error(
					ErrorCode::InvalidState,
					"effects have set state on {} frames with no "
					"input. an effect is probably setting state "
					"unconditionally (last dirty component: #{})",
					_stateOnEffectStreak,
					_context._store.LastDirty()
				);
			}
		}
		else
		{
			_stateOnEffectStreak = 0;
			_stateOnEffectWarned = false;
		}

		std::erase_if(
			_pendingSurfaces,
			[this](std::pair<Surface*, std::unique_ptr<Surface>>& pair) -> auto
			{
				if (pair.second->IsPlatformBacked())
				{
					_platformSurfaces.push_back(pair.second.get());
				}

				if (pair.first != nullptr &&
					pair.first->GetState() == SurfaceState::Alive)
				{
					pair.first->_children.push_back(std::move(pair.second));
					pair.first->MarkDirty();
				}
				else
				{
					_surfaces.push_back(std::move(pair.second));
				}
				return true;
			}
		);

		for (auto& [origin, child, target] : _pendingReparents)
		{
			if (child->GetState() != SurfaceState::Alive ||
				child->_desc.Parent != origin)
			{
				continue;
			}

			auto owned = child->_desc.Parent->DetachChild(child);
			if (owned == nullptr)
			{
				continue;
			}
			
			if (target != nullptr)
			{
				child->_desc.Parent = target;
				target->_children.push_back(std::move(owned));
			}
			else
			{
				child->_desc.Parent = nullptr;
				_surfaces.push_back(std::move(owned));
			}
			((target != nullptr) ? target : child)->MarkDirty();
		}
		_pendingReparents.clear();
	}

	void Application::On(
		std::string_view event,
		std::function<bool(void*, void*)> cb,
		void* userData
	)
	{
		auto hash = detail::hashString(event);
		if (_events.EmitDepth > 0)
		{
			_events.PendingEvents.push_back(
				{.Hash = hash, .Clear = false, .Cb = {std::move(cb), userData}}
			);
			return;
		}

		_events.Events[hash].push_back(
			{.Callback = std::move(cb), .User = userData}
		);
	}

	void Application::On(
		ApplicationEvent event,
		std::function<bool(void*, void*)> cb,
		void* userData
	)
	{
		auto hash = (uint64_t)event;
		if (_events.EmitDepth > 0)
		{
			_events.PendingEvents.push_back(
				{.Hash = hash,
				 .Clear = false,
				 .Cb = {
					 .Callback = std::move(cb),
					 .User = userData,
				 }}
			);
			return;
		}

		_events.Events[hash].push_back(
			{.Callback = std::move(cb), .User = userData}
		);
	}

	auto Application::EmitEvent(std::string_view event, void* data) -> bool
	{
		auto it = _events.Events.find(detail::hashString(event));
		if (it == _events.Events.end())
		{
			return true;
		}

		bool result = true;

		{
			EventEmitScope scope{_events.EmitDepth};

			result = std::ranges::all_of(
				it->second,
				[data](const auto& cb) -> auto
				{ return cb.Callback(data, cb.User); }
			);
		}

		_drainPendingEvents();
		return result;
	}

	auto Application::EmitEvent(ApplicationEvent event, void* data) -> bool
	{
		auto it = _events.Events.find((uint64_t)event);
		if (it == _events.Events.end())
		{
			return true;
		}

		bool result = true;

		{
			EventEmitScope scope{_events.EmitDepth};

			result = std::ranges::all_of(
				it->second,
				[data](const auto& cb) -> auto
				{ return cb.Callback(data, cb.User); }
			);
		}

		_drainPendingEvents();
		return result;
	}

	void Application::RemoveCallbacks(std::string_view event)
	{
		auto hash = detail::hashString(event);
		if (_events.EmitDepth > 0)
		{
			_events.PendingEvents.push_back(
				{.Hash = hash, .Clear = true, .Cb = {}}
			);
			return;
		}

		auto it = _events.Events.find(hash);
		if (it != _events.Events.end())
		{
			it->second.clear();
		}
	}

	void Application::_drainPendingEvents()
	{
		if (_events.EmitDepth > 0 || _events.PendingEvents.empty())
		{
			return;
		}

		for (auto& op : _events.PendingEvents)
		{
			if (op.Clear)
			{
				_events.Events[op.Hash].clear();
			}
			else
			{
				_events.Events[op.Hash].push_back(std::move(op.Cb));
			}
		}

		_events.PendingEvents.clear();
	}

	auto Application::RegisterSurface(const SurfaceDesc& desc) -> Surface*
	{
		auto surface = std::make_unique<Surface>(desc);
		auto* ptr = surface.get();
		if (!EmitEvent(ApplicationEvent::SurfaceCreated, ptr))
		{
			return nullptr;
		}

		ptr->_desc.Parent = desc.Parent;

		if (!_inTick)
		{
			if (ptr->IsPlatformBacked())
			{
				_platformSurfaces.push_back(ptr);
			}

			if (desc.Parent != nullptr)
			{
				desc.Parent->_children.push_back(std::move(surface));
				desc.Parent->MarkDirty();
			}
			else
			{
				_surfaces.push_back(std::move(surface));
			}
		}
		else
		{
			_pendingSurfaces.emplace_back(desc.Parent, std::move(surface));
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
			else if (
				(std::isalnum((unsigned char)c) != 0) || c == '_' || c == '-'
			)
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
		else if (segments.size() == 3)
		{
			Domain = segments[0];
			Organization = segments[1];
			Name = segments[2];
		}
		else
		{
			_id = "<invalid>";
			Name = Organization = Domain = std::string_view();
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
		mc_assert(
			Application::main != nullptr,
			"expected to have an application instance running"
		);
		return &Application::main->_context;
	}
}