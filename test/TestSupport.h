#pragma once
#include "Application.h"
#include "DrawCommands.h"
#include "Element.h"
#include "StyleHelpers.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

namespace mtest
{
	using Commands = std::vector<mocca::cmds::DrawCommand>;

	inline auto countOf(const Commands& cmds, auto pred) -> int
	{
		int n = 0;
		for (const auto& c : cmds)
		{
			if (pred(c))
			{
				n++;
			}
		}
		return n;
	}

	inline auto rectCount(const Commands& cmds) -> int
	{
		return countOf(
			cmds,
			[](const auto& c)
		 -> auto	{ return std::get_if<mocca::cmds::DrawRectCmd>(&c) != nullptr; }
		);
	}

	inline auto textCount(const Commands& cmds) -> int
	{
		return countOf(
			cmds,
			[](const auto& c)
		 -> auto	{ return std::get_if<mocca::cmds::DrawTextCmd>(&c) != nullptr; }
		);
	}

	inline auto lastText(const Commands& cmds) -> std::string
	{
		for (const auto& c : cmds)
		{
			if (const auto* t = std::get_if<mocca::cmds::DrawTextCmd>(&c))
			{
				return t->Content;
			}
		}
		return {};
	}

	inline auto clipPushCount(const Commands& cmds) -> int
	{
		return countOf(
			cmds,
			[](const auto& c)
		 -> auto	{ return std::get_if<mocca::cmds::PushClipCmd>(&c) != nullptr; }
		);
	}

	inline auto clipPopCount(const Commands& cmds) -> int
	{
		return countOf(
			cmds,
			[](const auto& c)
		 -> auto	{ return std::get_if<mocca::cmds::PopClipCmd>(&c) != nullptr; }
		);
	}

	inline auto transformPushCount(const Commands& cmds) -> int
	{
		return countOf(
			cmds,
			[](const auto& c)
		 -> auto	{ return std::get_if<mocca::cmds::PushTransformCmd>(&c) != nullptr; }
		);
	}

	inline auto transformPopCount(const Commands& cmds) -> int
	{
		return countOf(
			cmds,
			[](const auto& c)
		 -> auto	{ return std::get_if<mocca::cmds::PopTransformCmd>(&c) != nullptr; }
		);
	}

	inline auto commandsOf(const mocca::Surface& s) -> const Commands&
	{
		return s.GetDrawData();
	}

	inline auto box(
		float w,
		float h,
		mocca::Color c = mocca::colors::Transparent
	) -> mocca::Element
	{
		return mocca::box(
			mocca::BoxDescriptor{
				.Style =
					{
						.Width = {mocca::styles::px(w)},
						.Height = {mocca::styles::px(h)},
						.BackgroundColor = c,
					},
			}
		);
	}

	inline auto boxWithText(
		float w,
		float h,
		std::string content
	) -> mocca::Element
	{
		return mocca::box(
			mocca::BoxDescriptor{
				.Style =
					{
						.Width = {mocca::styles::px(w)},
						.Height = {mocca::styles::px(h)},
					},
				.Children = {mocca::text(std::move(content))},
			}
		);
	}

	class AppTest : public ::testing::Test
	{
	public:
		explicit AppTest(const char* id = "mocca.test")
			: _app(id)
		{
		}

		[[nodiscard]] auto App() -> mocca::Application& { return _app; }

		auto Tick(int n = 1) -> void
		{
			for (int i = 0; i < n; ++i)
			{
				_app.Tick(0.016);
			}
		}

	private:
		mocca::Application _app;
	};

	class SurfaceTest : public AppTest
	{
	public:
		explicit SurfaceTest(const char* id = "mocca.test.surface")
			: AppTest(id)
		{
		}

		[[nodiscard]] auto AddSurface(
			mocca::SurfaceDesc desc,
			mocca::ComponentFn root
		) -> mocca::Surface*
		{
			desc.Root = std::move(root);
			return App().RegisterSurface(desc);
		}
	};

	class DeathWatch
	{
	public:
		void Watch(mocca::Surface* s) { _doomed = s; }

		[[nodiscard]] auto Count() const -> int { return _count; }

		auto Note(void* data) -> void
		{
			if (data == static_cast<void*>(_doomed))
			{
				_count++;
			}
		}

	private:
		mocca::Surface* _doomed = nullptr;
		int _count = 0;
	};

	class LifecycleTestBase : public AppTest
	{
	public:
		explicit LifecycleTestBase(const char* id = "mocca.test.lifecycle")
			: AppTest(id)
		{
			App().On(mocca::ApplicationEvent::SurfaceDestroyed,
					 [this](void* d, void*) -> bool
					 {
							Watch.Note(d);
							return true;
					 });
		}

		DeathWatch Watch;
	};

	inline auto makeParentChildTree(
		AppTest& t,
		mocca::Surface** outParent = nullptr,
		mocca::Surface** outChild = nullptr
	) -> std::pair<mocca::Surface*, mocca::Surface*>
	{
		auto* parent = t.App().RegisterSurface(
			{
				.Width = 200,
				.Height = 200,
				.Title = "parent",
				.Root = []() -> mocca::Element { return box(200, 200); },
			}
		);

		auto* child = t.App().RegisterSurface(
			{
				.Width = 50,
				.Height = 50,
				.Title = "child",
				.Parent = parent,
				.Root = []() -> mocca::Element { return box(50, 50); },
			}
		);

		if (outParent != nullptr)
		{
			*outParent = parent;
		}
		if (outChild != nullptr)
		{
			*outChild = child;
		}

		return {parent, child};
	}
}