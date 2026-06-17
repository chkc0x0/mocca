#include "Application.h"
#include "Logger.h"

namespace mocca
{
	Application::Application(const std::string& appId)
	{
		if (main != nullptr)
		{
			mc_error(ErrorCode::InvalidState,
					 "an application instance already exists");
			return;
		}

		_id = ApplicationID(appId);
		main = this;

		_context._store.SetMarkDirty(
			[this](detail::NodeId id)
			{
				_context._store.InsertDirty(id);
				_context._currentSurface->MarkDirty();
			});
	}

	Application::~Application()
	{
		main = nullptr;
	}

	void Application::Tick(double dt)
	{
		for (auto it = _surfaces.begin(); it != _surfaces.end();)
		{
			if (!it->get()->IsRunning())
			{
				it->reset();
				it = _surfaces.erase(it);
				continue;
			}

			_context._currentSurface = it->get();
			if (it->get()->IsDirty())
			{
				it->get()->Tick(dt);
				it->get()->Paint();
			}

			it->get()->Update();
			_context._currentSurface = nullptr;

			++it;
		}

		_context._store.ClearDirty();
		_context._store.FlushEffects();
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

	void Application::Print()
	{
		mc_info("[Application id={}]", GetAppID().GetCompoundID());

		for (auto& surface : _surfaces)
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