#include "TestSupport.h"
#include "core/ElementEvents.h"

namespace mtest
{
	TEST(EventViewTest, MultipleHandlersOrder)
	{
		int calls = 0;

		mocca::EventView view{
			mocca::on(
				mocca::events::PointerDown,
				[&](mocca::PointerEvent&) -> void { calls += 1; }
			),
			mocca::on(
				mocca::events::PointerDown,
				[&](mocca::PointerEvent&) -> void { calls += 10; }
			),
		};

		mocca::PointerEvent ev;
		view.Call(mocca::EventType::PointerDown, ev);

		EXPECT_EQ(calls, 11);
	}

	TEST(EventViewTest, CustomEvents)
	{
		struct MyEvent
		{
			int Value = 0;
		};

		int seen = 0;

		mocca::EventView view{
			mocca::on(
				mocca::CustomEvent<MyEvent>{},
				[&](MyEvent& e) -> void { seen = e.Value; }
			),
		};

		MyEvent ev{.Value = 42};
		view.Call(mocca::CustomEvent<MyEvent>{}, ev);

		EXPECT_EQ(seen, 42);
	}

	TEST(EventViewTest, MultipleCustomHandlers)
	{
		struct MyEvent
		{
			int Value = 0;
		};

		int calls = 0;

		mocca::EventView view{
			mocca::on(
				mocca::CustomEvent<MyEvent>{},
				[&](MyEvent&) -> void { calls += 1; }
			),
			mocca::on(
				mocca::CustomEvent<MyEvent>{},
				[&](MyEvent&) -> void { calls += 10; }
			),
		};

		MyEvent ev;
		view.Call(mocca::CustomEvent<MyEvent>{}, ev);

		EXPECT_EQ(calls, 11);
	}

	TEST(EventViewTest, MismatchedPayloadNotCalled)
	{
		int calls = 0;

		mocca::EventView view{
			mocca::on(
				mocca::events::PointerDown,
				[&](mocca::PointerEvent&) -> void { calls++; }
			),
		};

		mocca::KeyEvent ev;
		view.Call(mocca::EventType::KeyDown, ev);

		EXPECT_EQ(calls, 0);
	}
}
