#include "TestSupport.h"

namespace mtest
{
	using EventBusTest = AppTest;

	TEST_F(EventBusTest, RegistrationDeferred)
	{
		static int deferred = 0;
		deferred = 0;

		App().On(
			"go",
			[](void*, void*) -> bool
			{
				mocca::Application::main->On(
					"later",
					[](void*, void*) -> bool
					{
						deferred++;
						return true;
					}
				);
				return true;
			}
		);

		App().EmitEvent("go", nullptr);
		App().EmitEvent("later", nullptr);
		EXPECT_EQ(deferred, 1)
			<< "a callback registered during a dispatch must be callable";

		static int enumHits = 0;
		enumHits = 0;
		App().On(
			mocca::ApplicationEvent::Poll,
			[](void*, void*) -> bool
			{
				enumHits++;
				return true;
			}
		);
		App().EmitEvent(mocca::ApplicationEvent::Poll, nullptr);
		EXPECT_EQ(enumHits, 1);
	}

	TEST_F(EventBusTest, SurfaceCloseCancel)
	{
		static bool cancel = true;

		auto* s = App().RegisterSurface({
			.Width = 100,
			.Height = 100,
			.Title = "veto",
			.Root = []() -> mocca::Element { return box(50, 50); },
		});
		ASSERT_NE(s, nullptr);

		App().On(
			mocca::ApplicationEvent::SurfaceClosed,
			[](void*, void*) -> bool { return !cancel; }
		);

		s->RequestClose();
		EXPECT_EQ(s->GetState(), mocca::SurfaceState::Alive)
			<< "a cancelled close must leave the surface Alive";

		cancel = false;
		s->RequestClose();
		EXPECT_EQ(s->GetState(), mocca::SurfaceState::Zombie)
			<< "an accepted close must move the surface to Zombie";
	}
}