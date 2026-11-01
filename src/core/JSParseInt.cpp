#include "core/JSParseInt.h"
#include "core/JSStringEncoding.h"
#include <cmath>
#include <cstdint>
#include <limits>

namespace {
int digit(char16_t c) {
    if (c >= u'0' && c <= u'9') return c - u'0';
    if (c >= u'a' && c <= u'z') return c - u'a' + 10;
    if (c >= u'A' && c <= u'Z') return c - u'A' + 10;
    return 36;
}
std::int32_t toInt32(const JSValue& value) {
    const auto number = value.toNumber();
    if (!std::isfinite(number) || number == 0) return 0;
    auto integer = std::fmod(std::trunc(number), 4294967296.0);
    if (integer < 0) integer += 4294967296.0;
    const auto signedInteger = integer >= 2147483648.0 ? integer - 4294967296.0 : integer;
    return static_cast<std::int32_t>(signedInteger);
}
JSValue parse(const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
    // Conversion of the string precedes conversion of the radix, including
    // user-defined coercion, exceptions and reentrant calls.
    const auto text = jsUTF16((args.empty() ? JSValue{} : args[0]).toString());
    std::size_t offset = 0;
    while (offset < text.size() && jsWhitespace(text[offset])) ++offset;
    bool negative = false;
    if (offset < text.size() && (text[offset] == u'+' || text[offset] == u'-')) {
        negative = text[offset] == u'-'; ++offset;
    }
    int radix = toInt32(args.size() < 2 ? JSValue{} : args[1]);
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const bool stripPrefix = radix == 0 || radix == 16;
    if (radix == 0) radix = 10;
    if (radix < 2 || radix > 36) return nan;
    if (stripPrefix && offset + 1 < text.size() && text[offset] == u'0' && (text[offset + 1] == u'x' || text[offset + 1] == u'X')) {
        offset += 2; radix = 16;
    }
    const auto begin = offset;
    while (offset < text.size() && digit(text[offset]) < radix) ++offset;
    if (offset == begin) return nan;
    auto significant = begin;
    while (significant < offset && text[significant] == u'0') ++significant;
    if (significant == offset) return negative ? -0.0 : 0.0;
    double number = 0;
    if (radix == 10) {
        number = JSValue(jsUTF8(std::u16string_view(text).substr(significant, offset - significant))).toNumber();
    } else if ((radix & (radix - 1)) == 0) {
        unsigned width = 0; for (auto r = radix; r > 1; r >>= 1) ++width;
        std::string bits;
        for (auto i = significant; i < offset; ++i) {
            const auto d = digit(text[i]);
            for (auto shift = width; shift > 0; --shift) bits += (d & (1u << (shift - 1))) ? '1' : '0';
        }
        const auto first = bits.find('1'); bits.erase(0, first);
        if (bits.size() > 1024) number = std::numeric_limits<double>::infinity();
        else {
            bits.insert(0, (4 - bits.size() % 4) % 4, '0'); std::string hexadecimal = "0x";
            for (std::size_t i = 0; i < bits.size(); i += 4) {
                unsigned nibble = 0; for (unsigned j = 0; j < 4; ++j) nibble = nibble * 2 + bits[i + j] - '0';
                hexadecimal += "0123456789abcdef"[nibble];
            }
            number = JSValue(hexadecimal).toNumber();
        }
    } else {
        // V8 14.6 NumberParseIntHelper uses bounded integer chunks for radices
        // with implementation-defined rounding. Preserve its multiplication
        // boundaries rather than replacing them with a correctly-rounded bigint.
        constexpr auto chunkLimit = std::numeric_limits<std::uint32_t>::max() / 36;
        while (significant < offset) {
            std::uint32_t multiplier = 1, part = 0;
            while (significant < offset) {
                const auto nextMultiplier = multiplier * static_cast<unsigned>(radix);
                if (nextMultiplier > chunkLimit) break;
                part = part * static_cast<unsigned>(radix) + digit(text[significant++]);
                multiplier = nextMultiplier;
            }
            number = number * multiplier + part;
        }
    }
    return negative ? -number : number;
}
}
JSValue nativeParseIntFunction() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isCallable()) return *value;
    auto function = JSValue::function(parse, {}, "parseInt", 2);
    function.retainForHeapLifetime(); weak = JSWeakValue(function); return function;
}
