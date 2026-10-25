#include "TestSupport.h"
#include "core/Hooks.h"
#include "gfx/Canvas.h"

namespace mtest
{
	using RenderTest = SurfaceTest;

	TEST(CanvasDrawImage, EmitsImageCommand)
	{
		mocca::Canvas canvas;
		canvas.DrawImage(
			mocca::cmds::DrawImageCmd{
				.Rect = {.X = 4, .Y = 8, .Width = 32, .Height = 16},
				.Texture = {.Id = 7, .Gen = 1},
				.Radius = {.TopLeft = 2, .TopRight = 2},
			}
		);

		const auto& cmds = canvas.Commands();
		ASSERT_EQ(cmds.size(), 1U);
		const auto* image = std::get_if<mocca::cmds::DrawImageCmd>(&cmds.front());
		ASSERT_NE(image, nullptr);
		EXPECT_EQ(image->Texture.Id, 7U);
		EXPECT_EQ(image->Texture.Gen, 1U);
		EXPECT_FLOAT_EQ(image->Rect.X, 4.0F);
		EXPECT_FLOAT_EQ(image->Rect.Width, 32.0F);
		EXPECT_FLOAT_EQ(image->Radius.TopLeft, 2.0F);
	}

	TEST_F(RenderTest, FirstTickRecordsCommands)
	{
		auto* s = AddSurface(
			{
				.Width = 200,
				.Height = 100,
				.Title = "solo",
			},
			[]() -> mocca::Element { return box(200, 100, mocca::colors::Red); }
		);

		ASSERT_NE(s, nullptr);
		EXPECT_EQ(s->Tree(), nullptr)
			<< "nothing is reconciled before the first tick";

		Tick();

		ASSERT_NE(s->Tree(), nullptr)
			<< "the tree must reconcile on the first tick";
		EXPECT_GE(rectCount(commandsOf(*s)), 1)
			<< "the first tick must record draw commands";

		Tick();
		EXPECT_GE(rectCount(commandsOf(*s)), 1)
			<< "the canvas must survive an idle frame";
	}

	TEST_F(RenderTest, CleanRepaintChangesNothing)
	{
		auto* s = AddSurface(
			{
				.Width = 120,
				.Height = 60,
				.Title = "stable",
			},
			[]() -> mocca::Element
			{ return box(120, 60, mocca::colors::Black); }
		);

		Tick();
		const size_t AfterFirst = commandsOf(*s).size();
		ASSERT_GT(AfterFirst, 0U);

		Tick(5);
		EXPECT_EQ(commandsOf(*s).size(), AfterFirst)
			<< "an unchanged tree must not accumulate commands";
	}

	TEST_F(RenderTest, ChildIsSplicedIntoParent)
	{
		auto [parent, child] = makeParentChildTree(*this);

		Tick();

		EXPECT_GE(rectCount(commandsOf(*child)), 1)
			<< "a composited child records into its own canvas";
		EXPECT_GE(rectCount(commandsOf(*parent)), 2)
			<< "the parent must splice the child's commands into its own";
	}

	TEST_F(RenderTest, ChildReachesParentInSameTick)
	{
		static std::optional<mocca::StateSetter<std::string>> setText;

		auto* parent = AddSurface(
			{
				.Width = 400,
				.Height = 300,
				.Title = "parent",
			},
			[]() -> mocca::Element { return box(400, 300, mocca::colors::Red); }
		);

		(void)AddSurface(
			{
				.Width = 100,
				.Height = 20,
				.X = 10,
				.Y = 10,
				.Title = "child",
				.Parent = parent,
			},
			[]() -> mocca::Element
			{
				auto [text, set] = mocca::useState(std::string{});
				if (!setText)
				{
					setText.emplace(set);
				}
				return boxWithText(100, 20, text);
			}
		);

		Tick();
		ASSERT_EQ(textCount(commandsOf(*parent)), 1);

		for (const char* typed : {"h", "he", "hel", "hell", "hello"})
		{
			SCOPED_TRACE(typed);
			(*setText)(std::string(typed));
			Tick();

			EXPECT_EQ(lastText(commandsOf(*parent)), typed)
				<< "the parent's canvas must carry the child's current text on "
				   "the same tick";
		}

		Tick();
		EXPECT_EQ(lastText(commandsOf(*parent)), std::string("hello"))
			<< "an idle frame must not lose or duplicate the content";
	}

	TEST_F(RenderTest, RoundedRectResolved)
	{
		auto* s = AddSurface(
			{
				.Width = 200,
				.Height = 100,
				.Title = "rounded",
			},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.Style =
							{
								mocca::styles::Width(mocca::styles::px(100)),
								mocca::styles::Height(mocca::styles::px(40)),
								mocca::styles::BackgroundColor(
									mocca::colors::Red
								),
								mocca::styles::BorderRadius(
									mocca::styles::percent(50)
								),
							},
					}
				);
			}
		);

		Tick();

		const auto* rect = firstRect(commandsOf(*s));
		ASSERT_NE(rect, nullptr);
		EXPECT_FLOAT_EQ(rect->Radius.TopLeft, 20.0F);
		EXPECT_FLOAT_EQ(rect->Radius.TopRight, 20.0F);
		EXPECT_FLOAT_EQ(rect->Radius.BottomRight, 20.0F);
		EXPECT_FLOAT_EQ(rect->Radius.BottomLeft, 20.0F);
	}

	TEST_F(RenderTest, BorderResolved)
	{
		auto* s = AddSurface(
			{
				.Width = 200,
				.Height = 100,
				.Title = "bordered",
			},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.Style =
							{
								mocca::styles::Width(mocca::styles::px(50)),
								mocca::styles::Height(mocca::styles::px(50)),
								mocca::styles::BorderColor(
									mocca::colors::Red
								),
								mocca::styles::Border(mocca::styles::px(3)),
							},
					}
				);
			}
		);

		Tick();

		const auto* rect = firstRect(commandsOf(*s));
		ASSERT_NE(rect, nullptr);
		EXPECT_FLOAT_EQ(rect->Border.Top, 3.0F);
		EXPECT_FLOAT_EQ(rect->Border.Right, 3.0F);
		EXPECT_FLOAT_EQ(rect->Border.Bottom, 3.0F);
		EXPECT_FLOAT_EQ(rect->Border.Left, 3.0F);
		EXPECT_EQ(rect->BorderColor, mocca::colors::Red);
	}

	TEST_F(RenderTest, PlainBoxNoBorder)
	{
		auto* s = AddSurface(
			{
				.Width = 20,
				.Height = 20,
				.Title = "plain",
			},
			[]() -> mocca::Element
			{ return box(20, 20, mocca::colors::Black); }
		);

		Tick();

		const auto* rect = firstRect(commandsOf(*s));
		ASSERT_NE(rect, nullptr);
		EXPECT_FLOAT_EQ(rect->Border.Top, 0.0F);
		EXPECT_FLOAT_EQ(rect->Radius.TopLeft, 0.0F);
		EXPECT_EQ(rect->BorderColor, mocca::colors::Transparent);
	}

	TEST_F(RenderTest, SpliceIsBalancedAndFramed)
	{
		auto [parent, child] = makeParentChildTree(*this);
		(void)child;

		Tick();

		const auto& cmds = commandsOf(*parent);
		EXPECT_EQ(clipPushCount(cmds), clipPopCount(cmds))
			<< "every PushClip must be matched by a PopClip";
		EXPECT_EQ(transformPushCount(cmds), transformPopCount(cmds))
			<< "every PushTransform must be matched by a PopTransform";
		EXPECT_GE(clipPushCount(cmds), 1)
			<< "a spliced child must be clipped to its own bounds";
		EXPECT_GE(transformPushCount(cmds), 1)
			<< "a spliced child must be offset by its own position";
	}
}