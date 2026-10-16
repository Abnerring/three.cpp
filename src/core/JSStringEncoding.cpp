#include "core/JSStringEncoding.h"
#include <cstdint>
std::u16string jsUTF16(std::string_view value) {
    std::u16string result;
    for (std::size_t i = 0; i < value.size();) {
        const auto first = static_cast<unsigned char>(value[i++]);
        const unsigned count = first < 0x80 ? 0 : first < 0xe0 ? 1 : first < 0xf0 ? 2 : 3;
        std::uint32_t point = count == 0 ? first : first & (0x7f >> count);
        for (unsigned j = 0; j < count; ++j)
            point = (point << 6) | (static_cast<unsigned char>(value[i++]) & 0x3f);
        if (point <= 0xffff)
            result.push_back(static_cast<char16_t>(point));
        else {
            point -= 0x10000;
            result.push_back(static_cast<char16_t>(0xd800 + (point >> 10)));
            result.push_back(static_cast<char16_t>(0xdc00 + (point & 0x3ff)));
        }
    }
    return result;
}
std::string jsUTF8(std::u16string_view value) {
    std::string result;
    for (std::size_t i = 0; i < value.size(); ++i) {
        std::uint32_t point = value[i];
        if (point >= 0xd800 && point <= 0xdbff && i + 1 < value.size() && value[i + 1] >= 0xdc00 &&
            value[i + 1] <= 0xdfff)
            point = 0x10000 + ((point - 0xd800) << 10) + (value[++i] - 0xdc00);
        if (point < 0x80)
            result += static_cast<char>(point);
        else if (point < 0x800) {
            result += static_cast<char>(0xc0 | (point >> 6));
            result += static_cast<char>(0x80 | (point & 0x3f));
        } else if (point < 0x10000) {
            result += static_cast<char>(0xe0 | (point >> 12));
            result += static_cast<char>(0x80 | ((point >> 6) & 0x3f));
            result += static_cast<char>(0x80 | (point & 0x3f));
        } else {
            result += static_cast<char>(0xf0 | (point >> 18));
            result += static_cast<char>(0x80 | ((point >> 12) & 0x3f));
            result += static_cast<char>(0x80 | ((point >> 6) & 0x3f));
            result += static_cast<char>(0x80 | (point & 0x3f));
        }
    }
    return result;
}
bool jsWhitespace(char16_t value) {
    return value == 0x09 || value == 0x0a || value == 0x0b || value == 0x0c || value == 0x0d ||
           value == 0x20 || value == 0xa0 || value == 0x1680 ||
           (value >= 0x2000 && value <= 0x200a) || value == 0x2028 || value == 0x2029 ||
           value == 0x202f || value == 0x205f || value == 0x3000 || value == 0xfeff;
}
std::size_t jsAdvanceStringIndex(std::u16string_view value, std::size_t index, bool unicode) {
    if (unicode && index + 1 < value.size() && value[index] >= 0xd800 && value[index] <= 0xdbff &&
        value[index + 1] >= 0xdc00 && value[index + 1] <= 0xdfff)
        return index + 2;
    return index + 1;
}
