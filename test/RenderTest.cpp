#include "TestSupport.h"

namespace mtest
{
	using RenderTest = SurfaceTest;

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