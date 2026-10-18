#include "TestSupport.h"
#include "text/FontRegistry.h"
#include "text/TextShaper.h"
#include "text/TextLayout.h"

#ifndef mc_testFont
#	define mc_testFont ""
#endif

namespace mtest
{
	using mocca::mtext::FontDescriptorOverrides;
	using mocca::mtext::TextLayoutStyle;
	using namespace mocca::mtext::detail;

	auto testStyle() -> TextLayoutStyle
	{
		TextLayoutStyle style;
		style.Families = {"Test"};
		style.SizePx = 16.0F;
		return style;
	}

	auto totalAdvance(const ShapedText& shaped) -> int64_t
	{
		int64_t sum = 0;
		for (const auto& glyph : shaped.Glyphs)
		{
			sum += glyph.XAdvance;
		}
		return sum;
	}

	class TextLayoutTest : public AppTest
	{
	public:
		TextLayoutTest() : AppTest("mocca.test.text")
		{
		}

	protected:
		void SetUp() override
		{
			AppTest::SetUp();
			TextShaper::PurgeCache();

			if (mc_testFont[0] == '\0')
			{
				GTEST_SKIP() << "test font path was not defined at build time";
			}

			App().Fonts().RegisterFile(
				mc_testFont,
				FontDescriptorOverrides{.Family = "Test"}
			);
		}
	};

	TEST_F(TextLayoutTest, ShapesGlyphs)
	{
		auto shaped = TextShaper::Shape("Hello", testStyle());

		ASSERT_NE(shaped, nullptr);
		EXPECT_EQ(shaped->Face, 0U);
		EXPECT_GT(shaped->Upem, 0U);
		EXPECT_FALSE(shaped->Glyphs.empty());
		EXPECT_GT(totalAdvance(*shaped), 0);
		EXPECT_GT(shaped->Ascender, 0);
		EXPECT_GT(shaped->Descender, 0);
	}

	TEST_F(TextLayoutTest, EmptyShapesNothing)
	{
		auto shaped = TextShaper::Shape("", testStyle());

		ASSERT_NE(shaped, nullptr);
		EXPECT_TRUE(shaped->Glyphs.empty());
	}

	TEST_F(TextLayoutTest, CachedShapeReused)
	{
		auto style = testStyle();

		auto before = TextShaper::GetStats();
		auto first = TextShaper::Shape("cache me", style);
		auto afterFirst = TextShaper::GetStats();
		auto second = TextShaper::Shape("cache me", style);
		auto afterSecond = TextShaper::GetStats();

		ASSERT_NE(first, nullptr);
		EXPECT_EQ(first, second) << "same key must return same shape";
		EXPECT_EQ(afterFirst.ShapeCalls - before.ShapeCalls, 1U);
		EXPECT_EQ(second->Id, first->Id);
		EXPECT_EQ(afterSecond.ShapeHits - afterFirst.ShapeHits, 1U);
	}

	TEST_F(TextLayoutTest, UnequalShapesSeparately)
	{
		auto style = testStyle();

		auto one = TextShaper::Shape("one", style);
		auto two = TextShaper::Shape("two", style);

		ASSERT_NE(one, nullptr);
		ASSERT_NE(two, nullptr);
		EXPECT_NE(one, two);
		EXPECT_NE(one->Id, two->Id);
	}

	TEST_F(TextLayoutTest, UnknownFamilyNoShape)
	{
		auto style = testStyle();
		style.Families = {"Not Registered"};

		EXPECT_EQ(TextShaper::Shape("x", style), nullptr);
	}

	TEST_F(TextLayoutTest, LineOffsetAlignment)
	{
		mocca::mtext::TextLine line{.Width = 40.0F};

		EXPECT_FLOAT_EQ(
			line.XOffset(100.0F, mocca::TextAlignment::Start),
			0.0F
		);
		EXPECT_FLOAT_EQ(line.XOffset(100.0F, mocca::TextAlignment::End), 60.0F);
		EXPECT_FLOAT_EQ(
			line.XOffset(100.0F, mocca::TextAlignment::Right),
			60.0F
		);
		EXPECT_FLOAT_EQ(
			line.XOffset(100.0F, mocca::TextAlignment::Center),
			30.0F
		);
	}
}
