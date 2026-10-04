#include "TestSupport.h"
#include "core/Node.h"
#include "yoga/YGNode.h"
#include "yoga/YGNodeLayout.h"
#include "yoga/YGNodeStyle.h"
#include <algorithm>
#include <vector>

namespace mtest
{
	using mocca::Alignment;
	using mocca::BoxDescriptor;
	using mocca::ComponentDescriptor;
	using mocca::Element;
	using mocca::ElementKey;
	using mocca::FlexDirections;
	using mocca::TextDescriptor;
	using mocca::styles::px;

	class LayoutTest : public SurfaceTest
	{
	public:
		explicit LayoutTest(const char* id = "mocca.test.layout")
			: SurfaceTest(id)
		{
		}

		static void ArmInvalidationCounter(mocca::detail::Node* root)
		{
			counter = 0;
			ArmOn(root);
		}

		static auto OnDirty(YGNodeConstRef ref) -> void
		{
			++counter;
		}

		static auto ArmOn(mocca::detail::Node* n) -> void
		{
			if (n == nullptr)
			{
				return;
			}
			if (n->YogaNode != nullptr)
			{
				YGNodeSetDirtiedFunc(n->YogaNode, &OnDirty);
			}
			for (auto& c : n->Children)
			{
				ArmOn(c.get());
			}
		}

		[[nodiscard]] static auto ProveCounterFires(mocca::detail::Node* root)
			-> int
		{
			counter = 0;
			if (root == nullptr || root->YogaNode == nullptr)
			{
				return 0;
			}
			YGNodeSetDirtiedFunc(root->YogaNode, &OnDirty);

			YGNodeStyleSetFlexGrow(root->YogaNode, 1.0F);
			YGNodeStyleSetFlexGrow(root->YogaNode, 0.0F);
			return counter;
		}

		static inline int counter = 0;
	};

	using YogaSyncTest = LayoutTest;

	TEST_F(YogaSyncTest, UnchangedTreeNotInvalidating)
	{
		auto* s = AddSurface(
			{
				.Width = 400,
				.Height = 300,
				.Title = "stable",
			},
			[]() -> Element
			{
				BoxDescriptor root{};
				root.Style.Width = {px(400.0F)};
				root.Style.Height = {px(300.0F)};
				root.Style.FlexDirection = FlexDirections::Column;

				std::vector<Element> rows;
				for (int i = 0; i < 5; ++i)
				{
					BoxDescriptor row{};
					row.Key = static_cast<mocca::ElementKey>(i + 1);
					row.Style.Height = {px(20.0F)};
					row.Children = {
						box(10, 10, mocca::colors::Red),
						boxWithText(80, 10, "row" + std::to_string(i)),
						box(10, 10, mocca::colors::AlmostBlack),
					};
					rows.push_back(mocca::box(row));
				}
				root.Children = std::move(rows);
				return mocca::box(root);
			}
		);

		Tick();
		ASSERT_NE(s->Tree(), nullptr);

		ArmInvalidationCounter(s->Tree());

		EXPECT_GT(ProveCounterFires(s->Tree()), 0)
			<< "the dirtied callback never fired -- a zero below would be "
			   "meaningless";

		for (int i = 0; i < 10; ++i)
		{
			ArmInvalidationCounter(s->Tree());
			s->MarkDirty();
			Tick();
		}
		EXPECT_EQ(LayoutTest::counter, 0)
			<< "an unchanged tree must not invalidate any Yoga node "
			   "(got "
			<< LayoutTest::counter << " invalidations over 10 rerenders)";
	}

	TEST_F(YogaSyncTest, TreeStaysCleanOnTicks)
	{
		auto* s = AddSurface(
			{
				.Width = 200,
				.Height = 200,
				.Title = "idle",
			},
			[]() -> Element
			{
				BoxDescriptor root{};
				root.Style.Width = {px(200.0F)};
				root.Style.Height = {px(200.0F)};
				root.Children = {
					box(50, 50, mocca::colors::Red),
					boxWithText(50, 50, "static"),
				};
				return mocca::box(root);
			}
		);

		Tick();
		ASSERT_GT(ProveCounterFires(s->Tree()), 0)
			<< "the dirtied callback never fired -- a zero below would be "
			   "meaningless";

		for (int i = 0; i < 40; ++i)
		{
			ArmInvalidationCounter(s->Tree());
			s->MarkDirty();
			Tick();
			ASSERT_EQ(LayoutTest::counter, 0)
				<< "tree started invalidating at frame " << i;
		}
	}

	TEST_F(YogaSyncTest, AppendedChildJoinsYogaTree)
	{
		static int g_count = 2;

		auto* s = AddSurface(
			{
				.Width = 200,
				.Height = 200,
				.Title = "grow",
			},
			[]() -> Element
			{
				BoxDescriptor root{};
				root.Style.Width = {px(200.0F)};
				root.Style.Height = {px(200.0F)};
				root.Style.FlexDirection = FlexDirections::Column;
				for (int i = 0; i < g_count; ++i)
				{
					BoxDescriptor row{};
					row.Key = static_cast<mocca::ElementKey>(i + 1);
					row.Style.Height = {px(20.0F)};
					root.Children.push_back(box(20, 20, mocca::colors::Red));
				}
				return mocca::box(root);
			}
		);

		Tick();
		ASSERT_NE(s->Tree(), nullptr);
		auto before = YGNodeGetChildCount(s->Tree()->YogaNode);
		ASSERT_EQ(before, 2U);

		g_count = 3;
		s->MarkDirty();
		Tick();

		EXPECT_EQ(YGNodeGetChildCount(s->Tree()->YogaNode), 3U)
			<< "an appended child must be inserted into Yoga";

		g_count = 2;
	}

	TEST_F(YogaSyncTest, RemovedChildLeavesYoga)
	{
		static int g_count = 3;

		auto* s = AddSurface(
			{
				.Width = 200,
				.Height = 200,
				.Title = "shrink",
			},
			[]() -> Element
			{
				BoxDescriptor root{};
				root.Style.Width = {px(200.0F)};
				root.Style.Height = {px(200.0F)};
				root.Style.FlexDirection = FlexDirections::Column;
				for (int i = 0; i < g_count; ++i)
				{
					BoxDescriptor row{};
					row.Key = static_cast<mocca::ElementKey>(i + 1);
					row.Style.Height = {px(20.0F)};
					root.Children.push_back(box(20, 20, mocca::colors::Red));
				}
				return mocca::box(root);
			}
		);

		Tick();
		ASSERT_EQ(YGNodeGetChildCount(s->Tree()->YogaNode), 3U);

		g_count = 1;
		s->MarkDirty();
		Tick();

		EXPECT_EQ(YGNodeGetChildCount(s->Tree()->YogaNode), 1U)
			<< "removed children must be detached from Yoga";

		g_count = 3;
	}

	TEST_F(YogaSyncTest, ReorderedChildrenRebuild)
	{
		static bool g_reversed = false;

		auto* s = AddSurface(
			{
				.Width = 300,
				.Height = 100,
				.Title = "reorder",
			},
			[]() -> Element
			{
				BoxDescriptor root{};
				root.Style.Width = {px(300.0F)};
				root.Style.Height = {px(100.0F)};
				root.Style.FlexDirection = FlexDirections::Row;
				root.Style.AlignItems = Alignment::FlexStart;

				std::vector<mocca::Element> cells;
				for (int i = 0; i < 3; ++i)
				{
					BoxDescriptor cell{};
					cell.Key = static_cast<mocca::ElementKey>(i + 1);
					cell.Style.Width = {px(30.0F)};
					cell.Style.Height = {px(30.0F)};
					cells.push_back(mocca::box(cell));
				}
				if (g_reversed)
				{
					std::reverse(cells.begin(), cells.end());
				}
				root.Children = std::move(cells);
				return mocca::box(root);
			}
		);

		Tick();
		ASSERT_NE(s->Tree(), nullptr);
		ASSERT_EQ(YGNodeGetChildCount(s->Tree()->YogaNode), 3U);

		mocca::detail::Node* key1Before = nullptr;
		for (auto& c : s->Tree()->Children)
		{
			if (c->Key == 1)
			{
				key1Before = c.get();
			}
		}
		ASSERT_NE(key1Before, nullptr);

		g_reversed = true;
		s->MarkDirty();
		Tick();

		mocca::detail::Node* key1After = nullptr;
		for (auto& c : s->Tree()->Children)
		{
			if (c->Key == 1)
			{
				key1After = c.get();
			}
		}
		ASSERT_NE(key1After, nullptr);
		EXPECT_EQ(key1After, key1Before)
			<< "a keyed reorder must reuse the existing node";

		ASSERT_EQ(YGNodeGetChildCount(s->Tree()->YogaNode), 3U);
		auto* yFirst = YGNodeGetChild(s->Tree()->YogaNode, 0);
		auto* yLast = YGNodeGetChild(s->Tree()->YogaNode, 2);
		EXPECT_NE(yFirst, key1After->YogaNode)
			<< "key 1 moved to the end; Yoga still has it first (stale order)";
		EXPECT_EQ(yLast, key1After->YogaNode)
			<< "Yoga's last child must be the node carrying key 1";

		g_reversed = false;
	}

	TEST_F(YogaSyncTest, SwappedSubtreeIsRebuilt)
	{
		static bool g_swapped = false;

		auto* s = AddSurface(
			{
				.Width = 300,
				.Height = 100,
				.Title = "swap",
			},
			[]() -> Element
			{
				BoxDescriptor root{};
				root.Style.Width = {px(300.0F)};
				root.Style.Height = {px(100.0F)};
				root.Style.FlexDirection = FlexDirections::Row;
				root.Style.AlignItems = Alignment::FlexStart;

				auto leaf = [](ElementKey k, float w) -> Element
				{
					BoxDescriptor d{};
					d.Key = k;
					d.Style.Width = {px(w)};
					d.Style.Height = {px(10.0F)};
					return mocca::box(d);
				};

				auto first = leaf(1, 20.0F);
				auto second = leaf(2, 60.0F);

				if (g_swapped)
				{
					std::swap(first, second);
				}
				root.Children = {first, second};
				return mocca::box(root);
			}
		);

		Tick();
		ASSERT_EQ(YGNodeGetChildCount(s->Tree()->YogaNode), 2U);
		auto* yBefore = YGNodeGetChild(s->Tree()->YogaNode, 0);

		g_swapped = true;
		s->MarkDirty();
		Tick();

		auto* yAfter = YGNodeGetChild(s->Tree()->YogaNode, 0);
		EXPECT_NE(yAfter, yBefore)
			<< "the subtrees exchanged places; Yoga's first child must change";

		g_swapped = false;
	}

	TEST_F(YogaSyncTest, ComponentLeavesSpliced)
	{
		struct Props
		{
			int Index;
		};

		auto* s = AddSurface(
			{
				.Width = 300,
				.Height = 100,
				.Title = "components",
			},
			[]() -> Element
			{
				BoxDescriptor row{};
				row.Style.Width = {px(100.0F)};
				row.Style.Height = {px(20.0F)};
				row.Children = {
					mocca::box(
						BoxDescriptor{
							.Key = 10,
							.Style =
								{
									.Width = {px(10.0F)},
									.Height = {px(10.0F)},
								},
						}
					),
					mocca::box(
						BoxDescriptor{
							.Key = 11,
							.Style = {
								.Width = {px(10.0F)},
								.Height = {px(10.0F)},
							},
						}
					),
				};

				BoxDescriptor root{};
				root.Style.Width = {px(300.0F)};
				root.Style.Height = {px(100.0F)};
				root.Style.FlexDirection = FlexDirections::Row;
				root.Style.AlignItems = Alignment::FlexStart;
				root.Children = {
					mocca::box(row),
					mocca::component(
						[](const Props& p) -> Element
						{
							BoxDescriptor c{};
							c.Style.Width = {px(40.0F)};
							c.Style.Height = {px(10.0F)};
							c.Children = {
								mocca::box(
									BoxDescriptor{
										.Style = {
											.Width = {px(10.0F)},
											.Height = {px(10.0F)},
										},
									}
								),
							};
							(void)p;
							return mocca::box(c);
						},
						Props{.Index = 0},
						ComponentDescriptor{
							.Key = 20,
						}
					),
				};
				return mocca::box(root);
			}
		);

		Tick();
		ASSERT_NE(s->Tree(), nullptr);

		EXPECT_EQ(YGNodeGetChildCount(s->Tree()->YogaNode), 2U)
			<< "component leaves splice into the nearest ancestor's Yoga list";

		mocca::detail::Node* comp = s->Tree()->Children[1].get();
		ASSERT_TRUE(comp->IsComponent());
		ASSERT_EQ(
			YGNodeGetOwner(comp->Children[0]->YogaNode),
			s->Tree()->YogaNode
		) << "a spliced leaf's Yoga owner is the ancestor";
	}

	TEST_F(YogaSyncTest, UnchangedComponentTreeNotDirty)
	{
		struct Props
		{
			int Index;
		};

		auto* s = AddSurface(
			{
				.Width = 400,
				.Height = 300,
				.Title = "stable components",
			},
			[]() -> Element
			{
				BoxDescriptor root{};
				root.Style.Width = {px(400.0F)};
				root.Style.Height = {px(300.0F)};
				root.Style.FlexDirection = FlexDirections::Column;

				for (int i = 0; i < 4; ++i)
				{
					root.Children.push_back(
						mocca::component(
							[](const Props& p) -> Element
							{
								(void)p;
								BoxDescriptor c{};
								c.Style.Height = {px(20.0F)};
								c.Children = {
									mocca::text(
										TextDescriptor{
											.Content = "row",
										}
									),
								};
								return mocca::box(c);
							},
							Props{.Index = i},
							ComponentDescriptor{
								.Key = static_cast<mocca::ElementKey>(i + 1),
							}
						)
					);
				}
				return mocca::box(root);
			}
		);

		Tick();
		ASSERT_GT(ProveCounterFires(s->Tree()), 0)
			<< "the dirtied callback never fired";

		for (int i = 0; i < 15; ++i)
		{
			ArmInvalidationCounter(s->Tree());
			s->MarkDirty();
			Tick();
			ASSERT_EQ(LayoutTest::counter, 0)
				<< "component tree started invalidating at frame " << i;
		}
	}

	TEST_F(YogaSyncTest, LayoutMatchesPostReconcile)
	{
		auto* s = AddSurface(
			{
				.Width = 300,
				.Height = 300,
				.Title = "layout",
			},
			[]() -> Element
			{
				BoxDescriptor root{};
				root.Style.Width = {px(300.0F)};
				root.Style.Height = {px(300.0F)};
				root.Style.FlexDirection = FlexDirections::Column;

				BoxDescriptor a{};
				a.Key = 1;
				a.Style.Height = {px(50.0F)};
				BoxDescriptor b{};
				b.Key = 2;
				b.Style.Height = {px(70.0F)};
				BoxDescriptor c{};
				c.Key = 3;
				c.Style.Height = {px(90.0F)};
				root.Children = {mocca::box(a), mocca::box(b), mocca::box(c)};
				return mocca::box(root);
			}
		);

		Tick();
		ASSERT_NE(s->Tree(), nullptr);
		ASSERT_EQ(s->Tree()->Children.size(), 3U);

		EXPECT_FLOAT_EQ(
			YGNodeLayoutGetHeight(s->Tree()->Children[0]->YogaNode),
			50.0F
		);
		EXPECT_FLOAT_EQ(
			YGNodeLayoutGetHeight(s->Tree()->Children[1]->YogaNode),
			70.0F
		);
		EXPECT_FLOAT_EQ(
			YGNodeLayoutGetHeight(s->Tree()->Children[2]->YogaNode),
			90.0F
		);

		EXPECT_FLOAT_EQ(
			YGNodeLayoutGetTop(s->Tree()->Children[0]->YogaNode),
			0.0F
		);
		EXPECT_FLOAT_EQ(
			YGNodeLayoutGetTop(s->Tree()->Children[1]->YogaNode),
			50.0F
		);
		EXPECT_FLOAT_EQ(
			YGNodeLayoutGetTop(s->Tree()->Children[2]->YogaNode),
			120.0F
		);
	}

	TEST_F(YogaSyncTest, DeclaredSizesSurvive)
	{
		auto* s = AddSurface(
			{
				.Width = 800,
				.Height = 600,
				.Title = "smaller-than-surface",
			},
			[]() -> Element
			{
				BoxDescriptor root{};
				root.Style.Width = {px(300.0F)};
				root.Style.Height = {px(500.0F)};
				root.Style.Padding = mocca::styles::padding(px(8.0F));
				root.Style.AlignItems = Alignment::Stretch;

				BoxDescriptor input{};
				input.Key = 1;
				input.Style.Width = {mocca::styles::percent(100.0F)};
				input.Style.Height = {px(40.0F)};

				BoxDescriptor list{};
				list.Key = 2;
				list.Style.Width = {mocca::styles::percent(100.0F)};
				list.Style.Height = {px(400.0F)};
				list.Style.Margin = mocca::styles::margin(px(5.0F));

				root.Children = {mocca::box(input), mocca::box(list)};
				return mocca::box(root);
			}
		);

		Tick();
		ASSERT_NE(s->Tree(), nullptr);
		ASSERT_EQ(s->Tree()->Children.size(), 2U);

		auto* rootYoga = s->Tree()->YogaNode;
		ASSERT_NE(rootYoga, nullptr);

		EXPECT_FLOAT_EQ(YGNodeLayoutGetWidth(rootYoga), 300.0F)
			<< "root smaller than surface must keep declared width";
		EXPECT_FLOAT_EQ(YGNodeLayoutGetHeight(rootYoga), 500.0F)
			<< "root smaller than surface must keep declared height";

		auto* inputYoga = s->Tree()->Children[0]->YogaNode;
		auto* listYoga = s->Tree()->Children[1]->YogaNode;
		ASSERT_NE(inputYoga, nullptr);
		ASSERT_NE(listYoga, nullptr);

		EXPECT_FLOAT_EQ(YGNodeLayoutGetWidth(inputYoga), 284.0F)
			<< "100% of the root's content box";
		EXPECT_FLOAT_EQ(YGNodeLayoutGetHeight(inputYoga), 40.0F)
			<< "the input bar declared height must reach layout";

		EXPECT_FLOAT_EQ(YGNodeLayoutGetTop(inputYoga), 8.0F)
			<< "the input bar must be inset by the root padding";
		EXPECT_FLOAT_EQ(YGNodeLayoutGetTop(listYoga), 53.0F)
			<< "the list must sit below the input plus 5px margin";
		EXPECT_FLOAT_EQ(YGNodeLayoutGetHeight(listYoga), 400.0F);
	}

	TEST_F(YogaSyncTest, StylesReachLeaves)
	{
		struct Props
		{
			int Unused = 0;
		};

		auto* s = AddSurface(
			{
				.Width = 400,
				.Height = 400,
				.Title = "component-styles",
			},
			[]() -> Element
			{
				BoxDescriptor root{};
				root.Style.Width = {px(200.0F)};
				root.Style.Height = {px(200.0F)};
				root.Style.FlexDirection = FlexDirections::Column;

				auto inner = [](const Props&) -> Element
				{
					BoxDescriptor leaf{};
					leaf.Style.Width = {px(120.0F)};
					leaf.Style.Height = {px(60.0F)};
					return mocca::box(leaf);
				};

				ComponentDescriptor comp{};
				comp.Key = 7;
				comp.Style = {};

				root.Children = {
					mocca::component(inner, Props{}, comp),
				};
				return mocca::box(root);
			}
		);

		Tick();
		ASSERT_NE(s->Tree(), nullptr);
		ASSERT_EQ(s->Tree()->Children.size(), 1U);

		auto* comp = s->Tree()->Children[0].get();
		ASSERT_TRUE(comp->IsComponent())
			<< "the child must still be a component node";
		ASSERT_EQ(comp->YogaNode, nullptr)
			<< "a component must not own a Yoga node";
		ASSERT_EQ(comp->Children.size(), 1U);

		auto* leafYoga = comp->Children[0]->YogaNode;
		ASSERT_NE(leafYoga, nullptr);

		EXPECT_FLOAT_EQ(YGNodeLayoutGetWidth(leafYoga), 120.0F)
			<< "a leaf spliced through a component must still be styled";
		EXPECT_FLOAT_EQ(YGNodeLayoutGetHeight(leafYoga), 60.0F)
			<< "SyncYogaChildren must descend through component nodes";
	}
}