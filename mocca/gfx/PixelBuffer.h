#pragma once
#include <cstddef>
#include <cstring>
#include <memory>
#include <span>
#include <utility>

namespace mocca::gfx
{
	class PixelBuffer
	{
	public:
		PixelBuffer() = default;

		template <class Deleter>
		static auto
		Adopt(const std::byte* data, std::size_t size, Deleter deleter)
			-> PixelBuffer
		{
			return PixelBuffer{
				std::shared_ptr<const std::byte[]>(data, std::move(deleter)),
				size,
			};
		}

		static auto Copy(std::span<const std::byte> src) -> PixelBuffer
		{
			if (src.empty())
			{
				return {};
			}

			auto mem = std::make_shared_for_overwrite<std::byte[]>(src.size());
			std::memcpy(mem.get(), src.data(), src.size());

			return PixelBuffer{
				std::shared_ptr<const std::byte[]>(std::move(mem)),
				src.size(),
			};
		}

		[[nodiscard]] auto Data() const noexcept -> const std::byte*
		{
			return _data.get();
		}

		[[nodiscard]] auto Size() const noexcept -> std::size_t
		{
			return _size;
		}

		[[nodiscard]] auto Empty() const noexcept -> bool
		{
			return _size == 0;
		}

		[[nodiscard]] auto Span() const noexcept -> std::span<const std::byte>
		{
			return {_data.get(), _size};
		}

		auto Reset() noexcept -> void
		{
			_data.reset();
			_size = 0;
		}

	private:
		PixelBuffer(
			std::shared_ptr<const std::byte[]> data,
			std::size_t size
		) noexcept
			: _data(std::move(data)), _size(size)
		{
		}

		std::shared_ptr<const std::byte[]> _data;
		std::size_t _size = 0;
	};
}
