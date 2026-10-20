#include "TestSupport.h"
#include "core/Node.h"
#include "style/ComputedStyle.h"
#include "style/StyleHelpers.h"
#include "text/Measure.h"
#include "text/TextLayoutBuilder.h"
#include "text/FontRegistry.h"
#include "text/TextLayout.h"
#include "text/TextShaper.h"
#include <cmath>

#ifndef mc_testFont
#	define mc_testFont ""
#endif

namespace mtest
{
	using mocca::mtext::FontDescriptorOverrides;
	using mocca::mtext::TextLayoutStyle;
	using mocca::styles::ComputedStyle;
	using mocca::styles::px;
	using mocca::styles::StyleProperty;
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
		for (const auto& item : shaped.Items)
		{
			for (const auto& glyph : item.Glyphs)
			{
				sum += glyph.XAdvance;
			}
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
			TextLayoutBuilder::PurgeCache();

			if (mc_testFont[0] == '\0')
			{
				GTEST_SKIP() << "test font path was not defined at build time";
			}

			App().Fonts().RegisterFile(
				mc_testFont,
				FontDescriptorOverrides{.Family = "Test"}
			);
			App().Fonts().RegisterFile(
				mc_testFont,
				FontDescriptorOverrides{.Family = "sans-serif"}
			);
		}
	};

	TEST_F(TextLayoutTest, ShapesGlyphs)
	{
		auto shaped = TextShaper::Shape("Hello", testStyle());

		ASSERT_NE(shaped, nullptr);
		EXPECT_EQ(shaped->StrutFace, 0U);
		EXPECT_GT(shaped->StrutUpem, 0U);
		ASSERT_EQ(shaped->Items.size(), 1U);
		EXPECT_FALSE(shaped->Items[0].Glyphs.empty());
		EXPECT_GT(totalAdvance(*shaped), 0);
		EXPECT_GT(shaped->StrutAscender, 0);
		EXPECT_GT(shaped->StrutDescender, 0);
	}

	TEST_F(TextLayoutTest, GlyphClustersInsideItem)
	{
		auto shaped = TextShaper::Shape("Hello, world", testStyle());

		ASSERT_NE(shaped, nullptr);
		ASSERT_FALSE(shaped->Items.empty());
		for (const auto& item : shaped->Items)
		{
			for (const auto& glyph : item.Glyphs)
			{
				EXPECT_GE(glyph.Cluster, item.Start);
				EXPECT_LT(glyph.Cluster, item.End);
			}
		}
	}

	TEST_F(TextLayoutTest, EmptyShapesNothing)
	{
		auto shaped = TextShaper::Shape("", testStyle());

		ASSERT_NE(shaped, nullptr);
		ASSERT_EQ(shaped->Items.size(), 1U);
		EXPECT_TRUE(shaped->Items[0].Glyphs.empty());
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

	TEST_F(TextLayoutTest, EmptyContentHasNoLines)
	{
		ComputedStyle style;

		auto layout = TextLayoutBuilder::ResolveTextLayout(style, "", 100.0F, true);

		ASSERT_NE(layout, nullptr);
		EXPECT_TRUE(layout->Lines.empty());
		EXPECT_FLOAT_EQ(layout->Size.X, 0.0F);
		EXPECT_FLOAT_EQ(layout->Size.Y, 0.0F);
	}

	TEST_F(TextLayoutTest, ResolveProducesLineAndRun)
	{
		ComputedStyle style;

		auto layout =
			TextLayoutBuilder::ResolveTextLayout(style, "Hello", 1000.0F, true);

		ASSERT_NE(layout, nullptr);
		ASSERT_EQ(layout->Lines.size(), 1U);
		ASSERT_EQ(layout->Runs.size(), 1U);
		EXPECT_FALSE(layout->Runs[0].Glyphs.empty());
		EXPECT_GT(layout->Runs[0].Width, 0.0F);
		EXPECT_GT(layout->Lines[0].Width, 0.0F);
		EXPECT_FLOAT_EQ(layout->Lines[0].Width, layout->Size.X);
		EXPECT_FLOAT_EQ(layout->Size.Y, layout->LineHeight);

		const mocca::mtext::TextLine& line = layout->Lines[0];
		float halfLeading =
			(layout->LineHeight - (layout->Ascent + layout->Descent)) * 0.5F;
		EXPECT_FLOAT_EQ(line.Baseline, halfLeading + layout->Ascent);
	}

	TEST_F(TextLayoutTest, WidthScalesWithFontSize)
	{
		ComputedStyle small;
		small.Set<StyleProperty::FontSize>(px(16.0F));

		ComputedStyle large;
		large.Set<StyleProperty::FontSize>(px(32.0F));

		auto at16 =
			TextLayoutBuilder::ResolveTextLayout(small, "Hello", 1000.0F, true);
		auto at32 =
			TextLayoutBuilder::ResolveTextLayout(large, "Hello", 1000.0F, true);

		ASSERT_NE(at16, nullptr);
		ASSERT_NE(at32, nullptr);
		EXPECT_NEAR(at32->Lines[0].Width, 2.0F * at16->Lines[0].Width, 0.5F);
	}

	TEST_F(TextLayoutTest, FontSizeDoesNotReshape)
	{
		ComputedStyle small;
		small.Set<StyleProperty::FontSize>(px(16.0F));
		ComputedStyle large;
		large.Set<StyleProperty::FontSize>(px(32.0F));

		auto before = TextShaper::GetStats();
		TextLayoutBuilder::ResolveTextLayout(small, "Hello", 1000.0F, true);
		auto mids = TextShaper::GetStats();
		auto midl = TextLayoutBuilder::GetStats();
		TextLayoutBuilder::ResolveTextLayout(large, "Hello", 1000.0F, true);
		auto afterl = TextLayoutBuilder::GetStats();
		auto afters = TextShaper::GetStats();

		EXPECT_EQ(mids.ShapeCalls - before.ShapeCalls, 1U);
		EXPECT_EQ(afters.ShapeCalls - mids.ShapeCalls, 0U);
		EXPECT_EQ(afterl.LayoutBuilds - midl.LayoutBuilds, 1U);
	}

	TEST_F(TextLayoutTest, WrapsAtNarrowWidth)
	{
		ComputedStyle style;

		auto layout = TextLayoutBuilder::ResolveTextLayout(
			style,
			"the quick brown fox jumps over the lazy dog",
			80.0F,
			true
		);

		ASSERT_NE(layout, nullptr);
		EXPECT_GT(layout->Lines.size(), 1U);
		for (const auto& line : layout->Lines)
		{
			EXPECT_LE(line.Width, 80.0F + 0.01F);
		}
	}

	TEST_F(TextLayoutTest, LongWordBreaksAcrossLines)
	{
		ComputedStyle style;

		auto layout = TextLayoutBuilder::ResolveTextLayout(
			style,
			"Supercalifragilisticexpialidocious",
			60.0F,
			true
		);

		ASSERT_NE(layout, nullptr);
		EXPECT_GT(layout->Lines.size(), 1U);
		for (const auto& line : layout->Lines)
		{
			ASSERT_LT(line.RunBegin, layout->Runs.size());
			EXPECT_EQ(line.RunEnd - line.RunBegin, 1U);
			EXPECT_FALSE(layout->Runs[line.RunBegin].Glyphs.empty());
		}
	}

	TEST_F(TextLayoutTest, NewlinesBreak)
	{
		ComputedStyle style;

		EXPECT_EQ(
			TextLayoutBuilder::ResolveTextLayout(style, "\n", 1000.0F, true)
				->Lines.size(),
			2U
		);
		EXPECT_EQ(
			TextLayoutBuilder::ResolveTextLayout(style, "\n\n", 1000.0F, true)
				->Lines.size(),
			3U
		);

		auto layout =
			TextLayoutBuilder::ResolveTextLayout(style, "a\nb", 1000.0F, true);
		ASSERT_EQ(layout->Lines.size(), 2U);
		EXPECT_EQ(layout->Lines[0].Start, 0U);
		EXPECT_EQ(layout->Lines[0].End, 1U);
		EXPECT_EQ(layout->Lines[1].Start, 2U);
		EXPECT_EQ(layout->Lines[1].End, 3U);
	}

	TEST_F(TextLayoutTest, TrailingSpaceHangsSoftWrap)
	{
		ComputedStyle style;

		float abWidth =
			TextLayoutBuilder::ResolveTextLayout(style, "ab", 1000.0F, true)
				->Lines[0]
				.Width;

		auto layout = TextLayoutBuilder::ResolveTextLayout(
			style,
			"ab cd",
			abWidth + 1.0F,
			true
		);

		ASSERT_EQ(layout->Lines.size(), 2U);
		EXPECT_NEAR(layout->Lines[0].Width, abWidth, 0.2F)
			<< "the hanging space must not count toward the line width";
		EXPECT_EQ(layout->Lines[0].End, 3U) << "the space still belongs to the line";
		EXPECT_EQ(layout->Lines[1].Start, 3U);
	}

	TEST_F(TextLayoutTest, LineHeightNumber)
	{
		ComputedStyle style;
		style.Set<StyleProperty::FontSize>(px(18.0F));
		style.Set<StyleProperty::LineHeight>(mocca::styles::LineHeightType{1.5F});

		auto layout =
			TextLayoutBuilder::ResolveTextLayout(style, "x", 1000.0F, true);

		ASSERT_NE(layout, nullptr);
		EXPECT_FLOAT_EQ(layout->LineHeight, 27.0F);
	}

	TEST_F(TextLayoutTest, LargeLineHeightBaseline)
	{
		ComputedStyle style;
		style.Set<StyleProperty::FontSize>(px(20.0F));
		style.Set<StyleProperty::LineHeight>(mocca::styles::LineHeightType{1.0F});
		auto tight =
			TextLayoutBuilder::ResolveTextLayout(style, "Hi", 1000.0F, true);

		style.Set<StyleProperty::LineHeight>(mocca::styles::LineHeightType{2.0F});
		auto loose =
			TextLayoutBuilder::ResolveTextLayout(style, "Hi", 1000.0F, true);

		ASSERT_NE(tight, nullptr);
		ASSERT_NE(loose, nullptr);
		EXPECT_FLOAT_EQ(tight->LineHeight, 20.0F);
		EXPECT_FLOAT_EQ(loose->LineHeight, 40.0F);
		EXPECT_NEAR(
			loose->Lines[0].Baseline - tight->Lines[0].Baseline,
			(loose->LineHeight - tight->LineHeight) * 0.5F,
			0.01F
		);
	}

	TEST_F(TextLayoutTest, LayoutCacheReuse)
	{
		ComputedStyle style;

		auto a = TextLayoutBuilder::ResolveTextLayout(
			style,
			"Cache me",
			120.2F,
			true
		);
		auto b = TextLayoutBuilder::ResolveTextLayout(
			style,
			"Cache me",
			120.9F,
			true
		);
		auto c = TextLayoutBuilder::ResolveTextLayout(
			style,
			"Cache me",
			121.0F,
			true
		);

		ASSERT_NE(a, nullptr);
		EXPECT_EQ(a, b) << "subpixel widths = same key";
		EXPECT_NE(a, c) << "pixel change makes new layout";
	}

	TEST_F(TextLayoutTest, MeasureMatchesShrinkWrap)
	{
		ComputedStyle style;
		std::string content =
			"the quick brown fox jumps over the lazy dog";

		auto reference =
			TextLayoutBuilder::ResolveTextLayout(style, content, 80.0F, true);
		ASSERT_NE(reference, nullptr);
		ASSERT_GT(reference->Lines.size(), 1U);

		mocca::detail::Node node;
		node.Kind = mocca::TextElement{.Content = content};

		YGNodeRef yoga = YGNodeNew();
		YGNodeSetContext(yoga, &node);

		YGSize measured = measureFunc(
			yoga,
			80.0F,
			YGMeasureModeAtMost,
			0.0F,
			YGMeasureModeUndefined
		);
		YGNodeFree(yoga);

		EXPECT_LE(measured.width, 80.0F);
		EXPECT_FLOAT_EQ(measured.height, std::ceil(reference->Size.Y));

		auto atMeasured = TextLayoutBuilder::ResolveTextLayout(
			style,
			content,
			measured.width,
			true
		);

		ASSERT_NE(atMeasured, nullptr);
		EXPECT_EQ(atMeasured->Lines.size(), reference->Lines.size())
			<< "a shrink-wrapped measure must not change the line count";
		EXPECT_FLOAT_EQ(atMeasured->Size.Y, reference->Size.Y);
	}
}
