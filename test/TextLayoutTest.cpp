#include "TestSupport.h"
#include "core/Node.h"
#include "style/ComputedStyle.h"
#include "style/StyleHelpers.h"
#include "text/Measure.h"
#include "text/TextLayoutBuilder.h"
#include "text/FontRegistry.h"
#include "text/TextLayout.h"
#include "text/TextShaper.h"
#include "text/Unicode.h"
#include <cmath>

#ifndef mc_testFont
#	define mc_testFont ""
#endif

#ifndef mc_testCjkFont
#	define mc_testCjkFont ""
#endif

namespace mtest
{
	using mocca::mtext::FontDescriptorOverrides;
	using mocca::mtext::TextLayoutStyle;
	using namespace mocca::styles;
	using namespace mocca::mtext::detail;

	auto testStyle() -> TextLayoutStyle
	{
		TextLayoutStyle style;
		style.Families = {"Test"};
		style.SizePx = 16.0F;
		return style;
	}

	auto fallbackStyle() -> TextLayoutStyle
	{
		TextLayoutStyle style = testStyle();
		style.Families = {"Test", "CJK"};
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

			App().Fonts().RegisterFile(
				mc_testCjkFont,
				FontDescriptorOverrides{.Family = "CJK"}
			);
		}

		auto CjkFace() -> mocca::mtext::FontFaceId
		{
			return App().Fonts().Match("CJK", mocca::mtext::FontRequest{}).Id;
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

		EXPECT_FLOAT_EQ(line.XOffset(100.0F, TextAlignment::Start), 0.0F);
		EXPECT_FLOAT_EQ(line.XOffset(100.0F, TextAlignment::End), 60.0F);
		EXPECT_FLOAT_EQ(line.XOffset(100.0F, TextAlignment::Right), 60.0F);
		EXPECT_FLOAT_EQ(line.XOffset(100.0F, TextAlignment::Center), 30.0F);
	}

	TEST_F(TextLayoutTest, EmptyContentHasNoLines)
	{
		ComputedStyle style;

		auto layout =
			TextLayoutBuilder::ResolveTextLayout(style, "", 100.0F, true);

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
		float halfLeading = (layout->LineHeight -
							 (layout->Ascent + layout->Descent)) *
							0.5F;
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
		EXPECT_EQ(layout->Lines[0].End, 3U)
			<< "the space still belongs to the line";
		EXPECT_EQ(layout->Lines[1].Start, 3U);
	}

	TEST_F(TextLayoutTest, LineHeightNumber)
	{
		ComputedStyle style;
		style.Set<StyleProperty::FontSize>(px(18.0F));
		style.Set<StyleProperty::LineHeight>(
			mocca::styles::LineHeightType{1.5F}
		);

		auto layout =
			TextLayoutBuilder::ResolveTextLayout(style, "x", 1000.0F, true);

		ASSERT_NE(layout, nullptr);
		EXPECT_FLOAT_EQ(layout->LineHeight, 27.0F);
	}

	TEST_F(TextLayoutTest, LargeLineHeightBaseline)
	{
		ComputedStyle style;
		style.Set<StyleProperty::FontSize>(px(20.0F));
		style.Set<StyleProperty::LineHeight>(
			mocca::styles::LineHeightType{1.0F}
		);
		auto tight =
			TextLayoutBuilder::ResolveTextLayout(style, "Hi", 1000.0F, true);

		style.Set<StyleProperty::LineHeight>(
			mocca::styles::LineHeightType{2.0F}
		);
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
		std::string content = "the quick brown fox jumps over the lazy dog";

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

	TEST_F(TextLayoutTest, FallbackSplitsItems)
	{
		auto shaped = TextShaper::Shape("Hello 日本語", fallbackStyle());

		ASSERT_NE(shaped, nullptr);
		ASSERT_EQ(shaped->Items.size(), 2U);

		const auto& latin = shaped->Items[0];
		EXPECT_EQ(latin.Face, 0U);
		EXPECT_EQ(latin.Start, 0U);
		EXPECT_EQ(latin.End, 6U);

		const auto& cjk = shaped->Items[1];
		EXPECT_EQ(cjk.Face, CjkFace());
		EXPECT_EQ(cjk.Start, 6U);
		EXPECT_EQ(cjk.End, 15U);
		ASSERT_FALSE(cjk.Glyphs.empty());
		for (const auto& glyph : cjk.Glyphs)
		{
			EXPECT_NE(glyph.GlyphId, 0U)
				<< "this text is covered by the fallback face";
			EXPECT_GE(glyph.Cluster, cjk.Start);
			EXPECT_LT(glyph.Cluster, cjk.End);
		}

		EXPECT_EQ(shaped->StrutFace, 0U)
			<< "block metrics come from the stack, not the first item";
	}

	TEST_F(TextLayoutTest, SpaceStaysWithFallback)
	{
		auto shaped = TextShaper::Shape("日本 語", fallbackStyle());

		ASSERT_NE(shaped, nullptr);
		ASSERT_EQ(shaped->Items.size(), 1U)
			<< "a space between fallback words must not fragment the run";
		EXPECT_EQ(shaped->Items[0].Face, CjkFace());
	}

	TEST_F(TextLayoutTest, FallbackKeepsLineMetrics)
	{
		auto style = fallbackStyle();

		auto latin = TextLayoutBuilder::LayoutFor(
			TextShaper::Shape("Hello", style),
			style,
			1000.0F,
			true
		);
		auto cjk = TextLayoutBuilder::LayoutFor(
			TextShaper::Shape("日本語", style),
			style,
			1000.0F,
			true
		);
		auto leading = TextLayoutBuilder::LayoutFor(
			TextShaper::Shape("日本語 Hello", style),
			style,
			1000.0F,
			true
		);

		ASSERT_NE(latin, nullptr);
		ASSERT_NE(cjk, nullptr);
		ASSERT_NE(leading, nullptr);

		EXPECT_FLOAT_EQ(cjk->LineHeight, latin->LineHeight);
		EXPECT_FLOAT_EQ(leading->LineHeight, latin->LineHeight);
		EXPECT_FLOAT_EQ(cjk->Lines[0].Baseline, latin->Lines[0].Baseline);
		EXPECT_FLOAT_EQ(leading->Lines[0].Baseline, latin->Lines[0].Baseline);

		ASSERT_FALSE(cjk->Runs.empty());
		EXPECT_EQ(cjk->Runs[0].Face, CjkFace()) << "the CJK text fell back";
		EXPECT_EQ(cjk->Face, 0U);
	}

	TEST_F(TextLayoutTest, TwoRunsOneLine)
	{
		auto style = fallbackStyle();

		auto layout = TextLayoutBuilder::LayoutFor(
			TextShaper::Shape("Hello 日本語", style),
			style,
			1000.0F,
			true
		);

		ASSERT_NE(layout, nullptr);
		ASSERT_EQ(layout->Lines.size(), 1U);

		const mocca::mtext::TextLine& line = layout->Lines[0];
		ASSERT_EQ(line.RunEnd - line.RunBegin, 2U);

		const auto& first = layout->Runs[line.RunBegin];
		const auto& second = layout->Runs[line.RunBegin + 1];
		EXPECT_EQ(first.Face, 0U);
		EXPECT_EQ(second.Face, CjkFace());
		EXPECT_FLOAT_EQ(second.X, first.Width);
	}

	TEST_F(TextLayoutTest, GraphemeSequencesNotSplit)
	{
		using namespace mocca::mtext;

		std::string vs16 = Unicode::Encode(0x2764) + Unicode::Encode(0xFE0F);
		std::string family = Unicode::Encode(0x1F468) +
							 Unicode::Encode(0x200D) +
							 Unicode::Encode(0x1F469) +
							 Unicode::Encode(0x200D) +
							 Unicode::Encode(0x1F467) +
							 Unicode::Encode(0x200D) + Unicode::Encode(0x1F466);

		for (const std::string& content : {vs16, family})
		{
			auto shaped = TextShaper::Shape(content, fallbackStyle());

			ASSERT_NE(shaped, nullptr);
			EXPECT_EQ(shaped->Items.size(), 1U);
			EXPECT_EQ(shaped->Items[0].Start, 0U);
			EXPECT_EQ(shaped->Items[0].End, content.size());
		}
	}

	TEST_F(TextLayoutTest, StackPartOfShapeKey)
	{
		auto before = TextShaper::GetStats();

		auto one = TextShaper::Shape("Hello", testStyle());
		auto two = TextShaper::Shape("Hello", fallbackStyle());

		auto after = TextShaper::GetStats();

		ASSERT_NE(one, nullptr);
		ASSERT_NE(two, nullptr);
		EXPECT_NE(one->Id, two->Id);
		EXPECT_EQ(after.ShapeCalls - before.ShapeCalls, 2U);
	}

	TEST_F(TextLayoutTest, FallbackShapeCached)
	{
		auto style = fallbackStyle();

		auto before = TextShaper::GetStats();
		auto first = TextShaper::Shape("Hello 日本語", style);
		auto afterFirst = TextShaper::GetStats();
		auto second = TextShaper::Shape("Hello 日本語", style);
		auto afterSecond = TextShaper::GetStats();

		ASSERT_NE(first, nullptr);
		EXPECT_EQ(first, second);

		EXPECT_EQ(afterFirst.ShapeCalls - before.ShapeCalls, 1U);
		EXPECT_EQ(afterSecond.ShapeCalls - afterFirst.ShapeCalls, 0U);
		EXPECT_EQ(afterSecond.ShapeHits - afterFirst.ShapeHits, 1U);
	}

	TEST_F(TextLayoutTest, SeparatorsKeepItemFace)
	{
		auto style = fallbackStyle();
		mocca::mtext::FontFaceId cjk = CjkFace();

		struct Case
		{
			std::string Content;
			size_t Items;
		};

		std::vector<Case> cases{
			{.Content = "日本語\n日本語", .Items = 2},
			{.Content = "日本語\t日本語", .Items = 1},
		};

		for (const Case& testCase : cases)
		{
			auto shaped = TextShaper::Shape(testCase.Content, style);

			ASSERT_NE(shaped, nullptr) << testCase.Content;

			ASSERT_EQ(shaped->Items.size(), testCase.Items) << testCase.Content;
			for (const auto& item : shaped->Items)
			{
				EXPECT_EQ(item.Face, cjk) << testCase.Content;
			}
		}
	}

	TEST_F(TextLayoutTest, ParagraphNotInheritFallback)
	{
		auto shaped = TextShaper::Shape("日本語\n[abc]", fallbackStyle());

		ASSERT_NE(shaped, nullptr);

		ASSERT_EQ(shaped->Items.size(), 2U);
		EXPECT_EQ(shaped->Items[0].Face, CjkFace());
		EXPECT_EQ(shaped->Items[1].Face, 0U);
		EXPECT_EQ(shaped->Items[1].Start, 10U);
	}

	TEST_F(TextLayoutTest, ItemizationNotChangeWrapping)
	{
		std::string content =
			"你好，世界。日本語のテキスト、折り返しのテスト。";

		TextLayoutStyle single = testStyle();
		single.Families = {"CJK"};
		TextLayoutStyle multi = fallbackStyle();

		auto one = TextLayoutBuilder::LayoutFor(
			TextShaper::Shape(content, single),
			single,
			200.0F,
			true
		);
		auto many = TextLayoutBuilder::LayoutFor(
			TextShaper::Shape(content, multi),
			multi,
			200.0F,
			true
		);

		ASSERT_NE(one, nullptr);
		ASSERT_NE(many, nullptr);

		ASSERT_EQ(one->Lines.size(), many->Lines.size());
		for (size_t i = 0; i < one->Lines.size(); i++)
		{
			EXPECT_EQ(one->Lines[i].Start, many->Lines[i].Start);
			EXPECT_EQ(one->Lines[i].End, many->Lines[i].End);
		}

		EXPECT_LE(one->Lines.size(), 4U) << "greedy fill must pack clusters";
	}

	TEST_F(TextLayoutTest, ItemsCoverContent)
	{
		using namespace mocca::mtext;

		std::string vs16 = Unicode::Encode(0x2764) + Unicode::Encode(0xFE0F);
		std::string family = Unicode::Encode(0x1F468) +
							 Unicode::Encode(0x200D) + Unicode::Encode(0x1F466);

		std::vector<std::string> contents{
			"Hello",
			"Hello 日本語",
			"日本語",
			"a\nb",
			"\n",
			"\n\n",
			"line\n",
			"  leading",
			"trailing  ",
			"Привет, мир",
			"مرحبا بالعالم",
			"שלום עולם",
			"你好，世界。",
			"e" + Unicode::Encode(0x0301),
			vs16,
			family,
		};

		for (const std::string& content : contents)
		{
			auto shaped = TextShaper::Shape(content, fallbackStyle());

			ASSERT_NE(shaped, nullptr);
			ASSERT_FALSE(shaped->Items.empty());

			uint32_t cursor = 0;
			for (const auto& item : shaped->Items)
			{
				EXPECT_EQ(item.Start, cursor);
				EXPECT_GT(item.End, item.Start);
				EXPECT_NE(item.Face, SIZE_MAX);
				cursor = item.End;
			}
			EXPECT_EQ(cursor, content.size());
		}
	}
}
