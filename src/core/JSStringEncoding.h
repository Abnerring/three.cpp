#pragma once
#include <cstddef>
#include <string>
#include <string_view>

// The runtime stores UTF-8/WTF-8; ECMAScript string offsets count UTF-16 units.
std::u16string jsUTF16(std::string_view value);
std::string jsUTF8(std::u16string_view value);
bool jsWhitespace(char16_t value);
std::size_t jsAdvanceStringIndex(std::u16string_view value, std::size_t index, bool unicode);
