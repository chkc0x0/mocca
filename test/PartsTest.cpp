#include "TestSupport.h"
#include "core/Hooks.h"
#include "core/Node.h"
#include "core/NodeRef.h"
#include "core/Part.h"
#include "style/StyleHelpers.h"
#include "text/FontRegistry.h"
#include "text/TextGeometry.h"
#include "yoga/YGNode.h"
#include <optional>

#ifndef mc_testFont
#	define mc_testFont ""
#endif

namespace mtest
{
	using namespace mocca;
	using namespace mocca::styles;

	class PartsTest : public SurfaceTest
	{
	public:
		PartsTest() : SurfaceTest("mocca.test.parts")
		{
		}

	protected:
		void SetUp() override
		{
			SurfaceTest::SetUp();
			if (mc_testFont[0] == '\0')
			{
				GTEST_SKIP() << "test font path was not defined at build time";
			}
			App().Fonts().RegisterFile(
				mc_testFont,
				mocca::mtext::FontDescriptorOverrides{.Family = "sans-serif"}
			);
		}
	};

	auto indexOfFirstRectWithColor(Commands cmds, Color color) -> std::ptrdiff_t
	{
		for (std::size_t i = 0; i < cmds.size(); ++i)
		{
			const auto* paint = paintOf(cmds[i]);
			if (paint != nullptr && paint->Color == color)
			{
				return static_cast<std::ptrdiff_t>(i);
			}
		}
		return -1;
	}

	auto indexOfFirstText(Commands cmds) -> std::ptrdiff_t
	{
		for (std::size_t i = 0; i < cmds.size(); ++i)
		{
			if (std::get_if<gfx::cmds::DrawTextCmd>(&cmds[i]) != nullptr)
			{
				return static_cast<std::ptrdiff_t>(i);
			}
		}
		return -1;
	}

	TEST(ResolveAxisTest, StartAndEndDeriveSize)
	{
		auto axis = PartAxis::Resolve(
			Length{4.0F},
			Length{6.0F},
			std::nullopt,
			100.0F,
			50.0F,
			50.0F,
			16.0F
		);

		EXPECT_FLOAT_EQ(axis.Pos, 104.0F);
		EXPECT_FLOAT_EQ(axis.Size, 40.0F);
	}

	TEST(ResolveAxisTest, EndOnlyRightAligns)
	{
		auto axis = PartAxis::Resolve(
			std::nullopt,
			Length{6.0F},
			std::nullopt,
			100.0F,
			50.0F,
			10.0F,
			16.0F
		);

		EXPECT_FLOAT_EQ(axis.Pos, 134.0F);
		EXPECT_FLOAT_EQ(axis.Size, 10.0F);
	}

	TEST(ResolveAxisTest, ExplicitSizeWithStart)
	{
		auto axis = PartAxis::Resolve(
			Length{4.0F},
			std::nullopt,
			Length{8.0F},
			0.0F,
			50.0F,
			50.0F,
			16.0F
		);

		EXPECT_FLOAT_EQ(axis.Pos, 4.0F);
		EXPECT_FLOAT_EQ(axis.Size, 8.0F);
	}

	TEST(ResolveAxisTest, PercentResolvesAgainstReference)
	{
		auto axis = PartAxis::Resolve(
			percent(25.0F),
			std::nullopt,
			percent(50.0F),
			0.0F,
			200.0F,
			0.0F,
			16.0F
		);

		EXPECT_FLOAT_EQ(axis.Pos, 50.0F);
		EXPECT_FLOAT_EQ(axis.Size, 100.0F);
	}

	TEST(ResolveAxisTest, NegativeInsetAllowed)
	{
		auto axis = PartAxis::Resolve(
			Length{-2.0F},
			Length{-2.0F},
			std::nullopt,
			10.0F,
			100.0F,
			100.0F,
			16.0F
		);

		EXPECT_FLOAT_EQ(axis.Pos, 8.0F);
		EXPECT_FLOAT_EQ(axis.Size, 104.0F);
	}

	TEST(ResolveAxisTest, ClampsNegativeSizeToZero)
	{
		auto axis = PartAxis::Resolve(
			Length{60.0F},
			Length{60.0F},
			std::nullopt,
			0.0F,
			50.0F,
			50.0F,
			16.0F
		);

		EXPECT_FLOAT_EQ(axis.Size, 0.0F);
	}

	TEST(ResolveAxisTest, MarginClampsExplicitSize)
	{
		auto axis = PartAxis::Resolve(
			Length{17.0F},
			std::nullopt,
			Length{18.0F},
			0.0F,
			33.0F,
			33.0F,
			16.0F,
			Length{2.0F},
			Length{2.0F}
		);

		EXPECT_FLOAT_EQ(axis.Pos, 13.0F);
		EXPECT_FLOAT_EQ(axis.Size, 18.0F);
	}

	TEST(ResolveAxisTest, MarginRightAnchorsInside)
	{
		auto axis = PartAxis::Resolve(
			std::nullopt,
			Length{0.0F},
			Length{18.0F},
			0.0F,
			33.0F,
			33.0F,
			16.0F,
			Length{2.0F},
			Length{2.0F}
		);

		EXPECT_FLOAT_EQ(axis.Pos, 13.0F);
		EXPECT_FLOAT_EQ(axis.Size, 18.0F);
	}

	TEST(ResolveAxisTest, MarginOvershootsWithinHostWhileAnimating)
	{
		auto axis = PartAxis::Resolve(
			Length{17.0F},
			std::nullopt,
			Length{18.0F},
			0.0F,
			33.0F,
			33.0F,
			16.0F,
			Length{2.0F},
			Length{2.0F},
			true
		);

		EXPECT_FLOAT_EQ(axis.Pos, 15.0F);
		EXPECT_FLOAT_EQ(axis.Size, 18.0F);
	}

	TEST(ResolveAxisTest, MarginCapsOversizedExplicitSize)
	{
		auto axis = PartAxis::Resolve(
			std::nullopt,
			std::nullopt,
			Length{500.0F},
			0.0F,
			33.0F,
			33.0F,
			16.0F,
			Length{2.0F},
			Length{2.0F}
		);

		EXPECT_FLOAT_EQ(axis.Pos, 2.0F);
		EXPECT_FLOAT_EQ(axis.Size, 29.0F);
	}

	TEST(ResolveAxisTest, AutoMarginsCenter)
	{
		auto autoMargin = EnumValue<Length>{Auto};

		auto axis = PartAxis::Resolve(
			std::nullopt,
			std::nullopt,
			Length{10.0F},
			0.0F,
			100.0F,
			100.0F,
			16.0F,
			autoMargin,
			autoMargin
		);

		EXPECT_FLOAT_EQ(axis.Pos, 45.0F);
		EXPECT_FLOAT_EQ(axis.Size, 10.0F);
	}

	TEST(ResolveAxisTest, ZeroMarginStillAllowsOverflow)
	{
		auto axis = PartAxis::Resolve(
			Length{0.0F},
			std::nullopt,
			Length{500.0F},
			0.0F,
			100.0F,
			100.0F,
			16.0F
		);

		EXPECT_FLOAT_EQ(axis.Pos, 0.0F);
		EXPECT_FLOAT_EQ(axis.Size, 500.0F);
	}

	TEST_F(PartsTest, RangePartPaintsBehindText)
	{
		auto* s = AddSurface(
			{.Width = 200, .Height = 100, .Title = "range"},
			[]() -> Element
			{
				return box(
					BoxDescriptor{
						.Style = {Width(px(200)), Height(px(100))},
						.Children = {
							text(
								TextDescriptor{
									.Style =
										{
											Width(px(200)),
											FontFamily("sans-serif"),
											FontSize(px(16)),
										},
									.Content = "hello world",
									.Parts = {
										partRange(
											"selection",
											{.Start = 0, .End = 5},
											{BackgroundColor(colors::Blue)},
											PartLayer::Behind
										),
									},
								}
							),
						},
					}
				);
			}
		);

		Tick();

		auto cmds = commandsOf(*s);
		auto rectAt = indexOfFirstRectWithColor(cmds, colors::Blue);
		auto textAt = indexOfFirstText(cmds);

		ASSERT_GE(rectAt, 0);
		ASSERT_GE(textAt, 0);
		EXPECT_LT(rectAt, textAt) << "Behind part paints under text";
	}

	TEST_F(PartsTest, CaretPartMatchesCaretRect)
	{
		NodeRef* textRef = nullptr;

		auto* s = AddSurface(
			{.Width = 200, .Height = 100, .Title = "caret"},
			[&]() -> Element
			{
				textRef = useNode();
				return box(
					BoxDescriptor{
						.Style = {Width(px(200)), Height(px(100))},
						.Children = {
							text(
								TextDescriptor{
									.Style =
										{
											Width(px(120)),
											FontFamily("sans-serif"),
											FontSize(px(16)),
										},
									.Content = "hello world",
									.Parts = {
										partCaret(
											"caret",
											3,
											{BackgroundColor(colors::Red)}
										),
									},
									.Ref = textRef,
								}
							),
						},
					}
				);
			}
		);

		Tick();

		ASSERT_NE(textRef, nullptr);
		ASSERT_TRUE(textRef->Valid());

		auto cmds = commandsOf(*s);
		auto rectAt = indexOfFirstRectWithColor(cmds, colors::Red);
		auto textAt = indexOfFirstText(cmds);

		ASSERT_GE(rectAt, 0);
		EXPECT_GT(rectAt, textAt) << "Above part paints over text";

		const auto* rect = std::get_if<gfx::cmds::DrawRectCmd>(&cmds[rectAt]);
		ASSERT_NE(rect, nullptr);

		auto expected = textRef->CaretRect(
			{.Offset = 3, .Affinity = mtext::CaretAffinity::Downstream}
		);

		EXPECT_FLOAT_EQ(rect->Rect.X, expected.X);
		EXPECT_FLOAT_EQ(rect->Rect.Y, expected.Y);
		EXPECT_FLOAT_EQ(rect->Rect.Width, expected.Width);
		EXPECT_FLOAT_EQ(rect->Rect.Height, expected.Height);
	}

	TEST_F(PartsTest, CaretWidthOffsetComeFromStyle)
	{
		auto* s = AddSurface(
			{.Width = 200, .Height = 100, .Title = "caret-style"},
			[]() -> Element
			{
				return box(
					BoxDescriptor{
						.Style = {Width(px(200)), Height(px(100))},
						.Children = {
							text(
								TextDescriptor{
									.Style =
										{
											Width(px(120)),
											FontFamily("sans-serif"),
											FontSize(px(16)),
										},
									.Content = "hello world",
									.Parts = {
										partCaret(
											"caret",
											3,
											{
												BackgroundColor(colors::Red),
												Width(px(2)),
												Left(px(-1)),
											}
										),
									},
								}
							),
						},
					}
				);
			}
		);

		Tick();

		auto cmds = commandsOf(*s);
		auto rectAt = indexOfFirstRectWithColor(cmds, colors::Red);
		ASSERT_GE(rectAt, 0);

		const auto* rect = std::get_if<gfx::cmds::DrawRectCmd>(&cmds[rectAt]);
		ASSERT_NE(rect, nullptr);
		EXPECT_FLOAT_EQ(rect->Rect.Width, 2.0F);
	}

	TEST_F(PartsTest, CaretEndStaysInsideBorderBox)
	{
		NodeRef* textRef = nullptr;

		auto* s = AddSurface(
			{.Width = 200, .Height = 100, .Title = "caret-clamp"},
			[&]() -> Element
			{
				textRef = useNode();
				return box(
					BoxDescriptor{
						.Style = {Width(px(200)), Height(px(100))},
						.Children = {
							text(
								TextDescriptor{
									.Style =
										{
											Width(px(40)),
											FontFamily("sans-serif"),
											FontSize(px(16)),
										},
									.Content = "hello world hello world",
									.Parts = {
										partCaret(
											"caret",
											5,
											{BackgroundColor(colors::Red)}
										),
									},
									.Ref = textRef,
								}
							),
						},
					}
				);
			}
		);

		(void)s;
		Tick();

		ASSERT_NE(textRef, nullptr);
		ASSERT_TRUE(textRef->Valid());

		auto cmds = commandsOf(*s);
		auto rectAt = indexOfFirstRectWithColor(cmds, colors::Red);
		ASSERT_GE(rectAt, 0);

		const auto* rect = std::get_if<gfx::cmds::DrawRectCmd>(&cmds[rectAt]);
		ASSERT_NE(rect, nullptr);

		auto border = textRef->LayoutRect();
		EXPECT_LE(
			rect->Rect.X + rect->Rect.Width,
			border.X + border.Width + 0.001F
		);
		EXPECT_GE(rect->Rect.X, border.X - 0.001F);
	}

	TEST_F(PartsTest, BoxPartNegativeInsetsExtendOutsideHost)
	{
		auto* s = AddSurface(
			{.Width = 200, .Height = 200, .Title = "ring"},
			[]() -> Element
			{
				return box(
					BoxDescriptor{
						.Style = {Width(px(100)), Height(px(100))},
						.Parts = {
							part(
								"focus",
								{
									Left(px(-2)),
									Right(px(-2)),
									Top(px(-2)),
									Bottom(px(-2)),
									BackgroundColor(colors::Lime),
								}
							),
						},
					}
				);
			}
		);

		Tick();

		auto cmds = commandsOf(*s);
		auto rectAt = indexOfFirstRectWithColor(cmds, colors::Lime);
		ASSERT_GE(rectAt, 0);

		const auto* rect = std::get_if<gfx::cmds::DrawRectCmd>(&cmds[rectAt]);
		ASSERT_NE(rect, nullptr);

		EXPECT_FLOAT_EQ(rect->Rect.X, -2.0F);
		EXPECT_FLOAT_EQ(rect->Rect.Y, -2.0F);
		EXPECT_FLOAT_EQ(rect->Rect.Width, 104.0F);
		EXPECT_FLOAT_EQ(rect->Rect.Height, 104.0F);
	}

	TEST_F(PartsTest, MarginKeepsThumbInsideHost)
	{
		auto* s = AddSurface(
			{.Width = 200, .Height = 200, .Title = "margin"},
			[]() -> Element
			{
				return box(
					BoxDescriptor{
						.Style = {Width(px(33)), Height(px(18))},
						.Parts =
							{
								part(
									"thumb",
									{
										Margin(px(2)),
										Left(px(17)),
										Width(px(18)),
										Height(px(14)),
										BackgroundColor(colors::Red),
									}
								),
							},
					}
				);
			}
		);

		Tick();

		auto cmds = commandsOf(*s);
		auto rectAt = indexOfFirstRectWithColor(cmds, colors::Red);
		ASSERT_GE(rectAt, 0);

		const auto* rect = std::get_if<gfx::cmds::DrawRectCmd>(&cmds[rectAt]);
		ASSERT_NE(rect, nullptr);

		EXPECT_FLOAT_EQ(rect->Rect.X, 13.0F);
		EXPECT_FLOAT_EQ(rect->Rect.Y, 2.0F);
		EXPECT_FLOAT_EQ(rect->Rect.Width, 18.0F);
		EXPECT_FLOAT_EQ(rect->Rect.Height, 14.0F);
		EXPECT_LE(rect->Rect.X + rect->Rect.Width, 33.0F);
	}

	TEST_F(PartsTest, NestedPartPaintsInsideParentRect)
	{
		auto nested = part(
			"inner",
			{
				Left(px(5)),
				Top(px(5)),
				Width(px(10)),
				Height(px(10)),
				BackgroundColor(colors::Blue),
			}
		);
		auto outer = part(
			"outer",
			{
				Left(px(20)),
				Top(px(20)),
				Width(px(40)),
				Height(px(40)),
				BackgroundColor(colors::Red),
			}
		);
		outer.Parts.push_back(nested);

		auto* s = AddSurface(
			{.Width = 200, .Height = 200, .Title = "nested"},
			[outer = std::move(outer)]() -> Element
			{
				return box(
					BoxDescriptor{
						.Style = {Width(px(100)), Height(px(100))},
						.Parts = {outer},
					}
				);
			}
		);

		Tick();

		auto cmds = commandsOf(*s);
		auto innerAt = indexOfFirstRectWithColor(cmds, colors::Blue);
		ASSERT_GE(innerAt, 0);

		const auto* rect = std::get_if<gfx::cmds::DrawRectCmd>(&cmds[innerAt]);
		ASSERT_NE(rect, nullptr);

		EXPECT_FLOAT_EQ(rect->Rect.X, 25.0F);
		EXPECT_FLOAT_EQ(rect->Rect.Y, 25.0F);
		EXPECT_FLOAT_EQ(rect->Rect.Width, 10.0F);
	}

	TEST_F(PartsTest, HostPartStaysContentScrolls)
	{
		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "space"},
			[]() -> Element
			{
				return box(
					BoxDescriptor{
						.Style =
							{
								Width(px(100)),
								Height(px(100)),
								Overflow(OverflowType::Scroll),
								FlexDirection(FlexDirections::Column),
							},
						.Children = {
							box(BoxDescriptor{
								.Style = {
									Width(px(100)),
									Height(px(400)),
									FlexShrink(0),
								},
							}),
						},
						.Parts =
							{
								part(
									"host",
									{
										Left(px(0)),
										Top(px(0)),
										Width(px(5)),
										Height(px(5)),
										BackgroundColor(colors::Lime),
									},
									PartLayer::Above,
									PartSpace::Host
								),
								part(
									"content",
									{
										Left(px(0)),
										Top(px(0)),
										Width(px(5)),
										Height(px(5)),
										BackgroundColor(colors::Blue),
									},
									PartLayer::Above,
									PartSpace::Content
								),
							},
					}
				);
			}
		);

		Tick();

		auto* root = s->Tree();
		ASSERT_NE(root, nullptr);

		auto findRect = [&](Color color) -> std::optional<Rectangle>
		{
			auto cmds = commandsOf(*s);
			gfx::Matrix ctm = gfx::Matrix::Identity();
			std::vector<gfx::Matrix> stack;

			for (const auto& cmd : cmds)
			{
				if (std::get_if<gfx::cmds::SaveCmd>(&cmd) != nullptr)
				{
					stack.push_back(ctm);
					continue;
				}
				if (std::get_if<gfx::cmds::RestoreCmd>(&cmd) != nullptr)
				{
					if (!stack.empty())
					{
						ctm = stack.back();
						stack.pop_back();
					}
					continue;
				}
				if (const auto* concat =
						std::get_if<gfx::cmds::ConcatCmd>(&cmd))
				{
					ctm = gfx::Matrix::Concat(ctm, concat->Matrix);
					continue;
				}

				const auto* rect = std::get_if<gfx::cmds::DrawRectCmd>(&cmd);
				if (rect != nullptr && rect->Paint.Color == color)
				{
					return ctm.MapRect(rect->Rect);
				}
			}

			return std::nullopt;
		};

		auto hostBefore = findRect(colors::Lime);
		auto contentBefore = findRect(colors::Blue);
		ASSERT_TRUE(hostBefore.has_value());
		ASSERT_TRUE(contentBefore.has_value());

		root->ScrollOffset = {.X = 0.0F, .Y = 40.0F};
		s->MarkDirty();
		Tick();

		auto hostAfter = findRect(colors::Lime);
		auto contentAfter = findRect(colors::Blue);
		ASSERT_TRUE(hostAfter.has_value());
		ASSERT_TRUE(contentAfter.has_value());

		EXPECT_FLOAT_EQ(hostAfter->Y, hostBefore->Y)
			<< "host part stays still";
		EXPECT_FLOAT_EQ(contentAfter->Y, contentBefore->Y - 40.0F)
			<< "content part scrolls";
	}

	TEST_F(PartsTest, BoxHostPaintOrder)
	{
		auto* s = AddSurface(
			{.Width = 200, .Height = 200, .Title = "order"},
			[]() -> Element
			{
				return box(
					BoxDescriptor{
						.Style = {Width(px(100)), Height(px(100))},
						.Children = {
							box(BoxDescriptor{
								.Style = {
									Width(px(10)),
									Height(px(10)),
									BackgroundColor(colors::Blue),
								},
							}),
						},
						.Parts =
							{
								part(
									"behind",
									{
										Left(px(0)),
										Top(px(0)),
										Right(px(0)),
										Bottom(px(0)),
										BackgroundColor(colors::Red),
									},
									PartLayer::Behind
								),
								part(
									"above",
									{
										Left(px(0)),
										Top(px(0)),
										Right(px(0)),
										Bottom(px(0)),
										BackgroundColor(colors::Green),
									},
									PartLayer::Above
								),
							},
					}
				);
			}
		);

		Tick();

		auto cmds = commandsOf(*s);
		auto behind = indexOfFirstRectWithColor(cmds, colors::Red);
		auto child = indexOfFirstRectWithColor(cmds, colors::Blue);
		auto above = indexOfFirstRectWithColor(cmds, colors::Green);

		ASSERT_GE(behind, 0);
		ASSERT_GE(child, 0);
		ASSERT_GE(above, 0);

		EXPECT_LT(behind, child) << "Behind parts paint before children";
		EXPECT_GT(above, child) << "Above parts paint after children";
	}

	TEST_F(PartsTest, PartsNoLayoutOrHitTesting)
	{
		NodeRef* hostRef = nullptr;

		auto* s = AddSurface(
			{.Width = 200, .Height = 200, .Title = "no-layout"},
			[&]() -> Element
			{
				hostRef = useNode();
				return box(
					BoxDescriptor{
						.Style = {Width(px(50)), Height(px(50))},
						.Parts = {
							part(
								"huge",
								{
									Left(px(0)),
									Top(px(0)),
									Width(px(500)),
									Height(px(500)),
									BackgroundColor(colors::Red),
								}
							),
						},
						.Ref = hostRef,
					}
				);
			}
		);

		Tick();

		ASSERT_NE(hostRef, nullptr);
		ASSERT_TRUE(hostRef->Valid());

		auto rect = hostRef->LayoutRect();
		EXPECT_FLOAT_EQ(rect.Width, 50.0F)
			<< "part must not affect host layout size";
		EXPECT_FLOAT_EQ(rect.Height, 50.0F);

		auto* root = s->Tree();
		ASSERT_NE(root, nullptr);

		EXPECT_EQ(mocca::detail::Node::HitTest(root, 100.0F, 100.0F), nullptr)
			<< "parts are not hit testable";
		EXPECT_EQ(mocca::detail::Node::HitTest(root, 10.0F, 10.0F), root)
			<< "host still hit inside bounds";
	}

	int yogaDirtyCount = 0;

	void noteYogaDirty(YGNodeConstRef ref)
	{
		++yogaDirtyCount;
	}

	TEST_F(PartsTest, ChangingPartsNotDirtyLayout)
	{
		static int partWidth = 10;

		auto* s = AddSurface(
			{.Width = 200, .Height = 200, .Title = "reconcile-parts"},
			[]() -> Element
			{
				return box(
					BoxDescriptor{
						.Style = {Width(px(100)), Height(px(100))},
						.Children = {
							text(
								TextDescriptor{
									.Style =
										{
											Width(px(100)),
											FontFamily("sans-serif"),
											FontSize(px(16)),
										},
									.Content = "hello",
									.Parts = {
										part(
											"overlay",
											{
												Left(px(0)),
												Top(px(0)),
												Width(
													px(static_cast<float>(
														partWidth
													))
												),
												Height(px(10)),
												BackgroundColor(colors::Red),
											}
										),
									},
								}
							),
						},
					}
				);
			}
		);

		partWidth = 10;
		Tick();
		ASSERT_NE(s->Tree(), nullptr);

		auto* textNode = s->Tree()->Children[0].get();
		ASSERT_NE(textNode, nullptr);
		ASSERT_NE(textNode->YogaNode, nullptr);

		YGNodeSetDirtiedFunc(textNode->YogaNode, &noteYogaDirty);

		yogaDirtyCount = 0;
		partWidth = 18;
		s->MarkDirty();
		Tick();

		EXPECT_EQ(yogaDirtyCount, 0)
			<< "changing parts does not mark host Yoga dirty";

		auto cmds = commandsOf(*s);
		auto rectAt = indexOfFirstRectWithColor(cmds, colors::Red);
		ASSERT_GE(rectAt, 0) << "the updated part must paint";

		const auto* rect = std::get_if<gfx::cmds::DrawRectCmd>(&cmds[rectAt]);
		ASSERT_NE(rect, nullptr);
		EXPECT_FLOAT_EQ(rect->Rect.Width, 18.0F)
			<< "next paint reflects new part";
	}

	TEST_F(PartsTest, TextChangeDirtiesLayout)
	{
		static std::string content = "hello";

		auto* s = AddSurface(
			{.Width = 200, .Height = 200, .Title = "reconcile-text"},
			[]() -> Element
			{
				return box(
					BoxDescriptor{
						.Style = {Width(px(100)), Height(px(100))},
						.Children = {
							text(
								TextDescriptor{
									.Style =
										{
											Width(px(100)),
											FontFamily("sans-serif"),
											FontSize(px(16)),
										},
									.Content = content,
								}
							),
						},
					}
				);
			}
		);

		Tick();
		ASSERT_NE(s->Tree(), nullptr);

		auto* textNode = s->Tree()->Children[0].get();
		ASSERT_NE(textNode, nullptr);
		YGNodeSetDirtiedFunc(textNode->YogaNode, &noteYogaDirty);

		yogaDirtyCount = 0;
		content = "hello world, this is longer";
		s->MarkDirty();
		Tick();

		EXPECT_GT(yogaDirtyCount, 0)
			<< "content change still marks Yoga node dirty";
	}
}
