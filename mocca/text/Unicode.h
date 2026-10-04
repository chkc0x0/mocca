#pragma once
#include <string>

namespace mocca::mtext::unicode
{
	auto encode(char32_t cp) -> std::string;
}