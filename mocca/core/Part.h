#pragma once
#include "Identifiers.h"
#include "style/StyleHelpers.h"
#include "style/StyleList.h"
#include "text/CaretAffinity.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mocca
{
	enum class PartLayer : uint8_t
	{
		Behind,
		Above,
	};

	enum class PartSpace : uint8_t
	{
		Host,
		Content,
	};

	struct TextRange
	{
		uint32_t Start = 0;
		uint32_t End = 0;
	};

	struct PartAnchor
	{
		enum class Kind : uint8_t
		{
			Box,
			Range,
			Caret,
		};

		Kind Tag = Kind::Box;
		TextRange Range{};
		mtext::CaretAffinity Affinity = mtext::CaretAffinity::Downstream;
	};

	struct Part;

	using PartList = std::shared_ptr<const std::vector<Part>>;

	struct Part
	{
		ElementKey Name;
		PartLayer Layer = PartLayer::Above;
		PartSpace Space = PartSpace::Host;
		PartAnchor Anchor{};
		styles::StyleList Style;
		std::vector<Part> Parts;
	};

	inline auto part(
		ElementKey name,
		styles::StyleList style,
		PartLayer layer = PartLayer::Above,
		PartSpace space = PartSpace::Host
	) -> Part
	{
		return Part{
			.Name = name,
			.Layer = layer,
			.Space = space,
			.Anchor = {.Tag = PartAnchor::Kind::Box},
			.Style = std::move(style),
		};
	}

	inline auto partRange(
		ElementKey name,
		TextRange textRange,
		styles::StyleList style,
		PartLayer layer = PartLayer::Behind
	) -> Part
	{
		return Part{
			.Name = name,
			.Layer = layer,
			.Space = PartSpace::Content,
			.Anchor = {.Tag = PartAnchor::Kind::Range, .Range = textRange},
			.Style = std::move(style),
		};
	}

	inline auto partCaret(
		ElementKey name,
		uint32_t offset,
		styles::StyleList style,
		mtext::CaretAffinity affinity = mtext::CaretAffinity::Downstream
	) -> Part
	{
		return Part{
			.Name = name,
			.Layer = PartLayer::Above,
			.Space = PartSpace::Content,
			.Anchor =
				{
					.Tag = PartAnchor::Kind::Caret,
					.Range = {.Start = offset, .End = offset},
					.Affinity = affinity,
				},
			.Style = std::move(style),
		};
	}

	inline auto partList(std::vector<Part> parts) -> PartList
	{
		if (parts.empty())
		{
			return nullptr;
		}

		return std::make_shared<const std::vector<Part>>(std::move(parts));
	}

	inline auto
	partKey(std::uint64_t parentKey, const Part& part, std::size_t siblingIndex)
		-> std::uint64_t
	{
		auto key = static_cast<std::size_t>(parentKey);
		detail::hashCombine(
			key,
			static_cast<std::size_t>(static_cast<std::uint64_t>(part.Name))
		);
		detail::hashCombine(key, siblingIndex);
		return static_cast<std::uint64_t>(key);
	}

	namespace detail
	{
		inline auto
		resolveLength(const styles::Length& value, float basis, float emBasis)
			-> float
		{
			switch (value.Unit)
			{
			case styles::LengthUnit::Percent:
				return value.Value / 100.0F * basis;
			case styles::LengthUnit::Em:
				return value.Value * emBasis;
			case styles::LengthUnit::Rem:
				return value.Value * styles::RootFontSize;
			case styles::LengthUnit::Pixels:
				break;
			}

			return value.Value;
		}

		inline auto specifiedLength(
			const styles::EnumValue<styles::Length>& value
		) -> std::optional<styles::Length>
		{
			if (const auto* length = std::get_if<styles::Length>(&value))
			{
				return *length;
			}

			return std::nullopt;
		}

		inline auto resolvedLength(
			const std::optional<styles::Length>& value,
			float basis,
			float emBasis
		) -> std::optional<float>
		{
			if (!value.has_value())
			{
				return std::nullopt;
			}

			return resolveLength(*value, basis, emBasis);
		}
	}

	struct PartAxis
	{
		float Pos = 0.0F;
		float Size = 0.0F;

		static auto Resolve(
			const std::optional<styles::Length>& start,
			const std::optional<styles::Length>& end,
			const std::optional<styles::Length>& size,
			float refPos,
			float refSize,
			float intrinsic,
			float emBasis,
			const styles::EnumValue<styles::Length>& marginStart = {},
			const styles::EnumValue<styles::Length>& marginEnd = {},
			bool allowOvershoot = false
		) -> PartAxis;
	};
}
