#pragma once
#include "core/Application.h"
#include "gfx/DrawCommands.h"
#include "core/Element.h"
#include "style/StyleHelpers.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <span>
#include <string>
#include <vector>

namespace mtest
{
	using Commands = std::span<const mocca::gfx::cmds::DrawCommand>;

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
			[](const auto& c) -> auto
			{
				return std::get_if<mocca::gfx::cmds::DrawRectCmd>(&c) != nullptr ||
					   std::get_if<mocca::gfx::cmds::DrawRRectCmd>(&c) != nullptr ||
					   std::get_if<mocca::gfx::cmds::DrawDRRectCmd>(&c) != nullptr;
			}
		);
	}

	inline auto paintOf(const mocca::gfx::cmds::DrawCommand& c)
		-> const mocca::gfx::Paint*
	{
		if (const auto* r = std::get_if<mocca::gfx::cmds::DrawRectCmd>(&c))
		{
			return &r->Paint;
		}
		if (const auto* rr = std::get_if<mocca::gfx::cmds::DrawRRectCmd>(&c))
		{
			return &rr->Paint;
		}
		if (const auto* dr = std::get_if<mocca::gfx::cmds::DrawDRRectCmd>(&c))
		{
			return &dr->Paint;
		}
		return nullptr;
	}

	inline auto rectHasColor(const Commands& cmds, mocca::Color color) -> bool
	{
		return std::ranges::any_of(
			cmds,
			[color](const auto& c) -> auto
			{
				const auto* paint = paintOf(c);
				return paint != nullptr && paint->Color == color;
			}
		);
	}

	inline auto firstFillColor(const Commands& cmds) -> mocca::Color
	{
		for (const auto& c : cmds)
		{
			const auto* paint = paintOf(c);
			if (paint != nullptr &&
				paint->Style != mocca::gfx::PaintStyle::Stroke)
			{
				return paint->Color;
			}
		}
		return mocca::colors::Transparent;
	}

	inline auto firstBorderColor(const Commands& cmds) -> mocca::Color
	{
		for (const auto& c : cmds)
		{
			if (const auto* dr = std::get_if<mocca::gfx::cmds::DrawDRRectCmd>(&c))
			{
				return dr->Paint.Color;
			}

			const auto* paint = paintOf(c);
			if (paint != nullptr &&
				paint->Style != mocca::gfx::PaintStyle::Fill)
			{
				return paint->Color;
			}
		}
		return mocca::colors::Transparent;
	}

	inline auto firstRect(const Commands& cmds)
		-> const mocca::gfx::cmds::DrawRectCmd*
	{
		for (const auto& c : cmds)
		{
			if (const auto* r = std::get_if<mocca::gfx::cmds::DrawRectCmd>(&c))
			{
				return r;
			}
		}
		return nullptr;
	}

	inline auto firstRRect(const Commands& cmds)
		-> const mocca::gfx::cmds::DrawRRectCmd*
	{
		for (const auto& c : cmds)
		{
			if (const auto* r = std::get_if<mocca::gfx::cmds::DrawRRectCmd>(&c))
			{
				return r;
			}
		}
		return nullptr;
	}

	inline auto firstDRRect(const Commands& cmds)
		-> const mocca::gfx::cmds::DrawDRRectCmd*
	{
		for (const auto& c : cmds)
		{
			if (const auto* dr = std::get_if<mocca::gfx::cmds::DrawDRRectCmd>(&c))
			{
				return dr;
			}
		}
		return nullptr;
	}

	inline auto imageCount(const Commands& cmds) -> int
	{
		return countOf(
			cmds,
			[](const auto& c) -> auto
			{ return std::get_if<mocca::gfx::cmds::DrawImageCmd>(&c) != nullptr; }
		);
	}

	inline auto firstImage(const Commands& cmds)
		-> const mocca::gfx::cmds::DrawImageCmd*
	{
		for (const auto& c : cmds)
		{
			if (const auto* image = std::get_if<mocca::gfx::cmds::DrawImageCmd>(&c))
			{
				return image;
			}
		}
		return nullptr;
	}

	inline auto firstText(const Commands& cmds)
		-> const mocca::gfx::cmds::DrawTextCmd*
	{
		for (const auto& c : cmds)
		{
			if (const auto* t = std::get_if<mocca::gfx::cmds::DrawTextCmd>(&c))
			{
				return t;
			}
		}
		return nullptr;
	}

	inline auto textCount(const Commands& cmds) -> int
	{
		return countOf(
			cmds,
			[](const auto& c) -> auto
			{ return std::get_if<mocca::gfx::cmds::DrawTextCmd>(&c) != nullptr; }
		);
	}

	inline auto lastText(const Commands& cmds) -> std::string
	{
		for (const auto& c : cmds)
		{
			if (const auto* t = std::get_if<mocca::gfx::cmds::DrawTextCmd>(&c))
			{
				return t->Content;
			}
		}
		return {};
	}

	inline auto saveCount(const Commands& cmds) -> int
	{
		return countOf(
			cmds,
			[](const auto& c) -> auto
			{ return std::get_if<mocca::gfx::cmds::SaveCmd>(&c) != nullptr; }
		);
	}

	inline auto restoreCount(const Commands& cmds) -> int
	{
		return countOf(
			cmds,
			[](const auto& c) -> auto
			{ return std::get_if<mocca::gfx::cmds::RestoreCmd>(&c) != nullptr; }
		);
	}

	inline auto clipRectCount(const Commands& cmds) -> int
	{
		return countOf(
			cmds,
			[](const auto& c) -> auto
			{ return std::get_if<mocca::gfx::cmds::ClipRectCmd>(&c) != nullptr; }
		);
	}

	inline auto clipRRectCount(const Commands& cmds) -> int
	{
		return countOf(
			cmds,
			[](const auto& c) -> auto
			{ return std::get_if<mocca::gfx::cmds::ClipRRectCmd>(&c) != nullptr; }
		);
	}

	inline auto concatCount(const Commands& cmds) -> int
	{
		return countOf(
			cmds,
			[](const auto& c) -> auto
			{ return std::get_if<mocca::gfx::cmds::ConcatCmd>(&c) != nullptr; }
		);
	}

	inline auto firstConcat(const Commands& cmds)
		-> const mocca::gfx::cmds::ConcatCmd*
	{
		for (const auto& c : cmds)
		{
			if (const auto* concat =
					std::get_if<mocca::gfx::cmds::ConcatCmd>(&c))
			{
				return concat;
			}
		}
		return nullptr;
	}

	inline auto commandsOf(const mocca::Surface& s) -> Commands
	{
		return s.GetDrawData();
	}

	inline auto
	box(float w, float h, mocca::Color c = mocca::colors::Transparent)
		-> mocca::Element
	{
		return mocca::box(
			mocca::BoxDescriptor{
				.Style = {
					mocca::styles::Width(mocca::styles::px(w)),
					mocca::styles::Height(mocca::styles::px(h)),
					mocca::styles::BackgroundColor(c),
				},
			}
		);
	}

	inline auto boxWithText(float w, float h, std::string content)
		-> mocca::Element
	{
		return mocca::box(
			mocca::BoxDescriptor{
				.Style =
					{
						mocca::styles::Width(mocca::styles::px(w)),
						mocca::styles::Height(mocca::styles::px(h)),
					},
				.Children = {mocca::text(std::move(content))},
			}
		);
	}

	class AppTest : public ::testing::Test
	{
	public:
		explicit AppTest(const char* id = "mocca.test") : _app(id)
		{
		}

		[[nodiscard]] auto App() -> mocca::Application&
		{
			return _app;
		}

		auto Tick(int n = 1) -> void
		{
			for (int i = 0; i < n; ++i)
			{
				_app.Tick(0.016);
				_app.Render();
			}
		}

		auto Tick(double dt, int n = 1) -> void
		{
			for (int i = 0; i < n; ++i)
			{
				_app.Tick(dt);
				_app.Render();
			}
		}

	protected:
		void SetUp() override
		{
			mocca::Logger::SetLogCallback(
				[this](const mocca::LogMessage& m, void*) -> void
				{ _log.emplace_back(m.Message); }
			);
		}

		void TearDown() override
		{
			mocca::Logger::SetLogCallback(nullptr);
			_log.clear();
		}

		[[nodiscard]] auto Saw(std::string_view str) const -> bool
		{
			return std::ranges::any_of(
				_log,
				[str](const auto& m) -> auto
				{ return m.find(str) != std::string::npos; }
			);
		}

		[[nodiscard]] auto LogDump() const -> std::string
		{
			return _log.empty() ? "<nothing>" : _log.front();
		}

	private:
		mocca::Application _app;
		std::vector<std::string> _log;
	};

	class SurfaceTest : public AppTest
	{
	public:
		explicit SurfaceTest(const char* id = "mocca.test.surface")
			: AppTest(id)
		{
		}

		auto AddSurface(
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
		void Watch(mocca::Surface* s)
		{
			_doomed = s;
		}

		[[nodiscard]] auto Count() const -> int
		{
			return _count;
		}

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
			App().On(
				mocca::ApplicationEvent::SurfaceDestroyed,
				[this](void* d, void*) -> bool
				{
					Watch.Note(d);
					return true;
				}
			);
		}

		DeathWatch Watch;
	};

	inline auto makeParentChildTree(
		AppTest& t,
		mocca::Surface** outParent = nullptr,
		mocca::Surface** outChild = nullptr
	) -> std::pair<mocca::Surface*, mocca::Surface*>
	{
		auto* parent = t.App().RegisterSurface({
			.Width = 200,
			.Height = 200,
			.Title = "parent",
			.Root = []() -> mocca::Element
			{ return box(200, 200, mocca::colors::Black); },
		});

		auto* child = t.App().RegisterSurface({
			.Width = 50,
			.Height = 50,
			.Title = "child",
			.Parent = parent,
			.Root = []() -> mocca::Element
			{ return box(50, 50, mocca::colors::Blue); },
		});

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