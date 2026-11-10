#include "TestSupport.h"
#include "a11y/A11y.h"
#include "a11y/Snapshot.h"
#include "core/Hooks.h"
#include "core/NodeRef.h"
#include "utils/TypeInterners.h"
#include <memory>

namespace mtest
{
	auto sampleProps() -> mocca::aria::Props
	{
		mocca::aria::Props props;
		props.Role = mocca::aria::Role::Button;
		props.Id = mocca::aria::id("save");
		props.Name = "Save";
		props.Selected = true;
		props.Level = 2;
		props.LabelledBy = {mocca::aria::id("label")};
		return props;
	}

	TEST(A11yProps, EqualityAndHash)
	{
		auto a = sampleProps();
		auto b = sampleProps();

		EXPECT_EQ(a, b);
		EXPECT_EQ(mocca::aria::PropsHash{}(a), mocca::aria::PropsHash{}(b));

		b.Name = "Save as";
		EXPECT_NE(a, b);

		b = sampleProps();
		b.LabelledBy.push_back(mocca::aria::id("hint"));
		EXPECT_NE(a, b);
	}

	TEST(A11yProps, SameA11yFallsBackToContent)
	{
		auto a = std::make_shared<mocca::aria::Props>(sampleProps());
		auto b = std::make_shared<mocca::aria::Props>(sampleProps());

		EXPECT_NE(a, b);
		EXPECT_TRUE(mocca::detail::sameA11y(a, b));
		EXPECT_FALSE(mocca::detail::sameA11y(a, nullptr));
		EXPECT_FALSE(mocca::detail::sameA11y(nullptr, b));
	}

	TEST(A11yProps, ConvenienceRoleAndName)
	{
		auto a = mocca::a11y(mocca::aria::Role::Button, "Save");
		auto b = mocca::a11y({
			.Role = mocca::aria::Role::Button,
			.Name = "Save",
		});

		EXPECT_TRUE(mocca::detail::sameA11y(a, b));
	}

	using A11yInternerTest = AppTest;

	TEST_F(A11yInternerTest, EqualPropsShareHandle)
	{
		auto a = mocca::a11y(sampleProps());
		auto b = mocca::a11y(sampleProps());

		EXPECT_EQ(a, b);
	}

	TEST_F(A11yInternerTest, DifferentPropsDiffer)
	{
		auto other = sampleProps();
		other.Name = "Cancel";

		auto a = mocca::a11y(sampleProps());
		auto b = mocca::a11y(other);

		EXPECT_NE(a, b);
		EXPECT_FALSE(mocca::detail::sameA11y(a, b));
	}

	TEST_F(A11yInternerTest, PruneDropsUnreferencedEntries)
	{
		std::weak_ptr<const mocca::aria::Props> weak;
		{
			const auto Handle = mocca::a11y(sampleProps());
			weak = Handle;
		}

		EXPECT_TRUE(weak.expired());

		App().GetInterners()->A11y.Prune();

		auto handle = mocca::a11y(sampleProps());
		EXPECT_TRUE(
			mocca::detail::sameA11y(handle, mocca::a11y(sampleProps()))
		);
	}

	using SnapshotTest = SurfaceTest;

	using mocca::aria::Role;
	using namespace mocca::styles;

	auto snapshotOf(mocca::Surface& surface)
		-> std::shared_ptr<const mocca::aria::Snapshot>
	{
		return surface.BuildA11ySnapshot();
	}

	auto countRole(const mocca::aria::Snapshot& snapshot, Role role) -> int
	{
		int n = 0;
		for (const auto& node : snapshot.Nodes)
		{
			if (node.Role == role)
			{
				n++;
			}
		}
		return n;
	}

	TEST_F(SnapshotTest, RootIsWindow)
	{
		auto* s = AddSurface(
			{.Width = 200, .Height = 100, .Title = "win"},
			[]() -> mocca::Element
			{ return mocca::box(mocca::BoxDescriptor{}); }
		);

		Tick();

		auto snap = snapshotOf(*s);
		ASSERT_NE(snap, nullptr);
		EXPECT_EQ(snap->Root, 0U);
		EXPECT_EQ(snap->Title, "win");

		ASSERT_FALSE(snap->Nodes.empty());
		const auto& root = snap->Nodes.front();
		EXPECT_EQ(root.Id, 0U);
		EXPECT_EQ(root.Role, Role::Window);
		EXPECT_EQ(root.Name, "win");
		EXPECT_FLOAT_EQ(root.Bounds.Width, 200.0F);
		EXPECT_FLOAT_EQ(root.Bounds.Height, 100.0F);
	}

	TEST_F(SnapshotTest, TextIsLabel)
	{
		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "text"},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{.Children = {mocca::text("Hello")}}
				);
			}
		);

		Tick();

		auto snap = snapshotOf(*s);
		ASSERT_EQ(snap->Nodes.size(), 2U) << "window + one label";
		EXPECT_EQ(countRole(*snap, Role::Label), 1);

		const auto& label = snap->Nodes[1];
		EXPECT_EQ(label.Role, Role::Label);
		EXPECT_EQ(label.TextValue, "Hello");
		EXPECT_EQ(label.Props, nullptr) << "inferred text nodes carry no props";
	}

	TEST_F(SnapshotTest, BoxIsTransparent)
	{
		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "transparent"},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.Style = {Width(px(100)), Height(px(100))},
						.Children = {mocca::text("Body")},
					}
				);
			}
		);

		Tick();

		auto snap = snapshotOf(*s);
		EXPECT_EQ(snap->Nodes.size(), 2U) << "box must not be emitted";
		EXPECT_EQ(countRole(*snap, Role::Group), 0);
	}

	TEST_F(SnapshotTest, AnnotatedBox)
	{
		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "button"},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.A11y = mocca::a11y({.Role = Role::Button}),
						.Children = {mocca::text("Click me!")},
					}
				);
			}
		);

		Tick();

		auto snap = snapshotOf(*s);
		ASSERT_EQ(snap->Nodes.size(), 2U) << "button + consumed text";
		EXPECT_EQ(countRole(*snap, Role::Label), 0)
			<< "text consumed into the button name";

		const auto& button = snap->Nodes[1];
		EXPECT_EQ(button.Role, Role::Button);
		EXPECT_EQ(button.Name, "Click me!");
		EXPECT_NE(button.Props, nullptr);
	}

	TEST_F(SnapshotTest, ExplicitNameWins)
	{
		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "named"},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.A11y =
							mocca::a11y({.Role = Role::Button, .Name = "Save"}),
						.Children = {mocca::text("ignored")},
					}
				);
			}
		);

		Tick();

		auto snap = snapshotOf(*s);
		ASSERT_EQ(snap->Nodes.size(), 2U);
		EXPECT_EQ(snap->Nodes[1].Name, "Save");
		EXPECT_EQ(countRole(*snap, Role::Label), 0)
			<< "name-from-contents roles still consume text";
	}

	TEST_F(SnapshotTest, HiddenSubtreePurged)
	{
		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "hidden"},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.Style = {Width(px(100)), Height(px(100))},
						.Children = {
							mocca::box(
								mocca::BoxDescriptor{
									.A11y = mocca::a11y({.Hidden = true}),
									.Children = {mocca::text("secret")},
								}
							),
							mocca::text("visible"),
						},
					}
				);
			}
		);

		Tick();

		auto snap = snapshotOf(*s);
		ASSERT_EQ(snap->Nodes.size(), 2U) << "only the visible label";
		EXPECT_EQ(snap->Nodes[1].TextValue, "visible");
	}

	TEST_F(SnapshotTest, DisplayNonePurged)
	{
		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "display"},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.Style = {Width(px(100)), Height(px(100))},
						.Children = {
							mocca::box(
								mocca::BoxDescriptor{
									.Style = {Display(DisplayType::None)},
									.Children = {mocca::text("gone")},
								}
							),
							mocca::text("here"),
						},
					}
				);
			}
		);

		Tick();

		auto snap = snapshotOf(*s);
		ASSERT_EQ(snap->Nodes.size(), 2U);
		EXPECT_EQ(snap->Nodes[1].TextValue, "here");
	}

	TEST_F(SnapshotTest, NamedAnnotatedBoxIsGroup)
	{
		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "group"},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.A11y = mocca::a11y({.Name = "Toolbar"}),
						.Children = {mocca::text("x")},
					}
				);
			}
		);

		Tick();

		auto snap = snapshotOf(*s);
		ASSERT_EQ(countRole(*snap, Role::Group), 1);
		
		EXPECT_EQ(countRole(*snap, Role::Label), 1);
		EXPECT_EQ(snap->Nodes[1].Name, "Toolbar");
	}

	TEST_F(SnapshotTest, BoundsUseLayoutPosition)
	{
		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "bounds"},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.Style = {Width(px(100)), Height(px(100))},
						.Children = {
							mocca::box(
								mocca::BoxDescriptor{
									.A11y = mocca::a11y({.Role = Role::Button}),
									.Style = {
										Width(px(40)),
										Height(px(20)),
										MarginLeft(px(10)),
										MarginTop(px(30)),
									},
								}
							),
						},
					}
				);
			}
		);

		Tick();

		auto snap = snapshotOf(*s);
		ASSERT_EQ(countRole(*snap, Role::Button), 1);
		const auto* button = snap->Find(snap->Nodes[1].Id);
		ASSERT_NE(button, nullptr);
		EXPECT_FLOAT_EQ(button->Bounds.X, 10.0F);
		EXPECT_FLOAT_EQ(button->Bounds.Y, 30.0F);
		EXPECT_FLOAT_EQ(button->Bounds.Width, 40.0F);
		EXPECT_FLOAT_EQ(button->Bounds.Height, 20.0F);
	}

	TEST_F(SnapshotTest, ScrollSubtractsAncestorOffset)
	{
		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "scrolled"},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.Style =
							{
								Width(px(100)),
								Height(px(100)),
								Overflow(OverflowType::Scroll),
							},
						.Children = {
							mocca::box(
								mocca::BoxDescriptor{
									.A11y = mocca::a11y({.Role = Role::Button}),
									.Style = {
										Width(px(20)),
										Height(px(20)),
										MarginLeft(px(10)),
										MarginTop(px(50)),
									},
								}
							),
						},
					}
				);
			}
		);

		Tick();

		auto* root = s->Tree();
		ASSERT_NE(root, nullptr);
		root->ScrollOffset = {.X = 0.0F, .Y = 20.0F};

		auto snap = snapshotOf(*s);
		ASSERT_EQ(countRole(*snap, Role::Button), 1);
		const auto& button = snap->Nodes[1];
		EXPECT_FLOAT_EQ(button.Bounds.Y, 30.0F)
			<< "descendant bounds subtract the scroll offset";
	}

	TEST_F(SnapshotTest, FocusMapsToIncludedNode)
	{
		mocca::NodeRef* button = nullptr;

		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "focus"},
			[&]() -> mocca::Element
			{
				button = mocca::useNode();
				return mocca::box(
					mocca::BoxDescriptor{
						.Style = {Width(px(100)), Height(px(100))},
						.Children = {
							mocca::box(
								mocca::BoxDescriptor{
									.A11y = mocca::a11y({.Role = Role::Button}),
									.Style = {Width(px(50)), Height(px(50))},
									.Ref = button,
									.Focus = mocca::FocusMode::Tab,
								}
							),
						},
					}
				);
			}
		);

		Tick();
		ASSERT_NE(button, nullptr);
		ASSERT_TRUE(button->Valid());

		button->Focus();
		Tick();

		auto snap = snapshotOf(*s);
		EXPECT_EQ(snap->Focus, button->Id());
	}

	TEST_F(SnapshotTest, FocusUnannotatedBoxMapsToRoot)
	{
		mocca::NodeRef* inner = nullptr;

		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "unmapped"},
			[&]() -> mocca::Element
			{
				inner = mocca::useNode();
				return mocca::box(
					mocca::BoxDescriptor{
						.Style = {Width(px(100)), Height(px(100))},
						.Children = {
							mocca::box(
								mocca::BoxDescriptor{
									.Style = {Width(px(50)), Height(px(50))},
									.Ref = inner,
									.Focus = mocca::FocusMode::Tab,
								}
							),
						},
					}
				);
			}
		);

		Tick();
		ASSERT_NE(inner, nullptr);
		ASSERT_TRUE(inner->Valid());

		inner->Focus();
		Tick();

		auto snap = snapshotOf(*s);
		EXPECT_EQ(snap->Focus, 0U) << "falls back to the window root";
	}

	TEST_F(SnapshotTest, RelationsResolveByPropsId)
	{
		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "relations"},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.Style = {Width(px(100)), Height(px(100))},
						.Children = {
							mocca::text(
								mocca::TextDescriptor{
									.A11y = mocca::a11y(
										{.Id = mocca::aria::id("email-label")}
									),
									.Content = "Email",
								}
							),
							mocca::box(
								mocca::BoxDescriptor{
									.A11y = mocca::a11y({
										.Role = Role::TextInput,
										.LabelledBy =
											{mocca::aria::id("email-label")},
									}),
									.Style = {Width(px(50)), Height(px(20))},
								}
							),
						},
					}
				);
			}
		);

		Tick();

		auto snap = snapshotOf(*s);
		ASSERT_EQ(countRole(*snap, Role::TextInput), 1);

		const mocca::aria::Node* input = nullptr;
		const mocca::aria::Node* label = nullptr;
		for (const auto& node : snap->Nodes)
		{
			if (node.Role == Role::TextInput)
			{
				input = &node;
			}
			if (node.Props != nullptr &&
				node.Props->Id == mocca::aria::id("email-label"))
			{
				label = &node;
			}
		}

		ASSERT_NE(input, nullptr);
		ASSERT_NE(label, nullptr);
		ASSERT_EQ(input->LabelledBy.size(), 1U);
		EXPECT_EQ(input->LabelledBy[0], label->Id);
	}

	TEST_F(SnapshotTest, NameFromContentsJoinsDescendantText)
	{
		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "joined"},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.A11y = mocca::a11y({.Role = Role::Button}),
						.Children = {
							mocca::text("Save"),
							mocca::box(
								mocca::BoxDescriptor{
									.Children = {mocca::text(" changes")},
								}
							),
						},
					}
				);
			}
		);

		Tick();

		auto snap = snapshotOf(*s);
		ASSERT_EQ(countRole(*snap, Role::Button), 1);
		EXPECT_EQ(snap->Nodes[1].Name, "Save changes");
		EXPECT_EQ(countRole(*snap, Role::Label), 0);
	}

	TEST_F(SnapshotTest, ComponentNodeTransparent)
	{
		auto* s = AddSurface(
			{.Width = 100, .Height = 100, .Title = "component"},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.Style = {Width(px(100)), Height(px(100))},
						.Children = {
							mocca::component(
								[]() -> mocca::Element
								{
									return mocca::box(
										mocca::BoxDescriptor{
											.A11y = mocca::a11y({
												.Role = Role::Button,
												.Name = "Inner",
											}),
											.Style =
												{Width(px(20)), Height(px(20))},
										}
									);
								}
							),
						},
					}
				);
			}
		);

		Tick();

		auto snap = snapshotOf(*s);
		ASSERT_EQ(countRole(*snap, Role::Button), 1);
		const auto& button = snap->Nodes[1];
		EXPECT_EQ(button.Name, "Inner");
	}

	TEST_F(SnapshotTest, CompositeSurfaceFlattensWithOriginOffset)
	{
		auto* parent = AddSurface(
			{.Width = 200, .Height = 200, .Title = "parent"},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.Style = {Width(px(200)), Height(px(200))},
					}
				);
			}
		);
		ASSERT_NE(parent, nullptr);

		AddSurface(
			{
				.Width = 50,
				.Height = 50,
				.X = 100,
				.Y = 40,
				.Title = "child",
				.Parent = parent,
			},
			[]() -> mocca::Element
			{
				return mocca::box(
					mocca::BoxDescriptor{
						.A11y = mocca::a11y(
							{.Role = Role::Button, .Name = "Child"}
						),
						.Style = {Width(px(30)), Height(px(20))},
					}
				);
			}
		);

		Tick();

		auto snap = snapshotOf(*parent);
		ASSERT_EQ(countRole(*snap, Role::Button), 1);

		const auto& button = snap->Nodes[1];
		EXPECT_EQ(button.Name, "Child");
		EXPECT_FLOAT_EQ(button.Bounds.X, 100.0F)
			<< "composite origins offset flattened bounds";
		EXPECT_FLOAT_EQ(button.Bounds.Y, 40.0F);
	}
}
