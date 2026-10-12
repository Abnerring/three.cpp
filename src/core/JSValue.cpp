#include "core/JSValue.h"
#include "core/OrderedMap.h"
#include "core/ArraySort.h"
#include <algorithm>
#include <atomic>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <locale.h>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {
// Unlike std::thread::id, these identities cannot be reused after a thread
// exits. Weak handles validate the token before touching heap storage.
std::uint64_t runtimeThreadToken() {
    static std::atomic<std::uint64_t> next{1};
    static thread_local const auto token = [] {
        auto candidate = next.load(std::memory_order_relaxed);
        for (;;) {
            if (candidate == std::numeric_limits<std::uint64_t>::max())
                throw JSRangeError("Runtime thread identity exhausted");
            if (next.compare_exchange_weak(candidate, candidate + 1,
                                          std::memory_order_relaxed)) return candidate;
        }
    }();
    return token;
}
std::optional<std::uint32_t> arrayIndex(const std::string& key) {
    if (key.empty() || (key.size() > 1 && key[0] == '0')) return std::nullopt;
    std::uint64_t number = 0;
    for (const char c : key) {
        if (c < '0' || c > '9') return std::nullopt;
        number = number * 10 + (c - '0');
        if (number >= 0xffffffffu) return std::nullopt;
    }
    return static_cast<std::uint32_t>(number);
}
// UTF-8 strings also accept WTF-8 for individual UTF-16 surrogate units.
std::uint32_t decode(const std::string& text, std::size_t& offset) {
    const auto first = static_cast<unsigned char>(text[offset++]);
    if (first < 0x80) return first;
    unsigned count = first >= 0xf0 && first <= 0xf4 ? 3 : first >= 0xe0 && first <= 0xef ? 2 :
                     first >= 0xc2 && first <= 0xdf ? 1 : 0;
    auto result = static_cast<std::uint32_t>(first & (0x7f >> count));
    if (!count || offset + count > text.size()) throw JSTypeError("Invalid UTF-8 runtime string");
    const auto second = static_cast<unsigned char>(text[offset]);
    if ((first == 0xe0 && second < 0xa0) || (first == 0xf0 && second < 0x90) ||
        (first == 0xf4 && second > 0x8f)) throw JSTypeError("Invalid UTF-8 runtime string");
    while (count--) {
        const auto next = static_cast<unsigned char>(text[offset]);
        if ((next & 0xc0) != 0x80) throw JSTypeError("Invalid UTF-8 runtime string");
        result = (result << 6) | (next & 0x3f);
        ++offset;
    }
    return result;
}
bool whitespace(std::uint32_t c) {
    return (c >= 9 && c <= 13) || c == 0x20 || c == 0xa0 || c == 0x1680 ||
           (c >= 0x2000 && c <= 0x200a) || c == 0x2028 || c == 0x2029 ||
           c == 0x202f || c == 0x205f || c == 0x3000 || c == 0xfeff;
}
std::string trim(const std::string& text) {
    std::size_t offset = 0, begin = text.size(), end = 0;
    while (offset < text.size()) {
        const auto start = offset;
        if (!whitespace(decode(text, offset))) {
            if (begin == text.size()) begin = start;
            end = offset;
        }
    }
    return begin == text.size() ? std::string{} : text.substr(begin, end - begin);
}
std::vector<std::uint16_t> utf16(const std::string& text) {
    std::vector<std::uint16_t> units;
    std::size_t offset = 0;
    while (offset < text.size()) {
        const auto c = decode(text, offset);
        if (c <= 0xffff) units.push_back(static_cast<std::uint16_t>(c));
        else {
            units.push_back(static_cast<std::uint16_t>(0xd800 + ((c - 0x10000) >> 10)));
            units.push_back(static_cast<std::uint16_t>(0xdc00 + ((c - 0x10000) & 1023)));
        }
    }
    return units;
}
std::string encodeUnit(std::uint16_t c) {
    if (c < 0x80) return std::string(1, static_cast<char>(c));
    if (c < 0x800) return {static_cast<char>(0xc0 | (c >> 6)), static_cast<char>(0x80 | (c & 63))};
    return {static_cast<char>(0xe0 | (c >> 12)), static_cast<char>(0x80 | ((c >> 6) & 63)),
            static_cast<char>(0x80 | (c & 63))};
}
double parseCNumber(const std::string& text) {
#ifdef _WIN32
    static const auto locale = _create_locale(LC_NUMERIC, "C");
    return _strtod_l(text.c_str(), nullptr, locale);
#else
    static const auto locale = newlocale(LC_NUMERIC_MASK, "C", nullptr);
    return strtod_l(text.c_str(), nullptr, locale);
#endif
}
double stringNumber(const std::string& input) {
    const auto text = trim(input);
    if (text.empty()) return 0;
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    if (text == "Infinity" || text == "+Infinity") return std::numeric_limits<double>::infinity();
    if (text == "-Infinity") return -std::numeric_limits<double>::infinity();
    if (text.size() > 2 && text[0] == '0') {
        const auto prefix = text[1];
        const unsigned radix = prefix == 'x' || prefix == 'X' ? 16 :
                               prefix == 'o' || prefix == 'O' ? 8 :
                               prefix == 'b' || prefix == 'B' ? 2 : 0;
        if (radix) {
            std::string bits;
            for (std::size_t i = 2; i < text.size(); ++i) {
                const char c = text[i];
                const unsigned digit = c >= '0' && c <= '9' ? c - '0' :
                                       c >= 'a' && c <= 'f' ? c - 'a' + 10 :
                                       c >= 'A' && c <= 'F' ? c - 'A' + 10 : 16;
                if (digit >= radix) return nan;
                for (unsigned shift = radix == 16 ? 4 : radix == 8 ? 3 : 1; shift > 0; --shift)
                    bits += ((digit >> (shift - 1)) & 1) ? '1' : '0';
            }
            const auto first = bits.find('1');
            if (first == std::string::npos) return 0;
            bits.erase(0, first);
            if (bits.size() > 1024) return std::numeric_limits<double>::infinity();
            // One exact conversion avoids intermediate rounding for long radix literals.
            bits.insert(0, (4 - bits.size() % 4) % 4, '0');
            std::string hexadecimal = "0x";
            for (std::size_t i = 0; i < bits.size(); i += 4) {
                unsigned digit = 0;
                for (unsigned j = 0; j < 4; ++j) digit = digit * 2 + (bits[i + j] - '0');
                hexadecimal += "0123456789abcdef"[digit];
            }
            return parseCNumber(hexadecimal);
        }
    }
    // StringNumericLiteral grammar excludes C's inf/nan and hexadecimal floats.
    const bool negative = text[0] == '-';
    std::size_t i = text[0] == '+' || negative ? 1 : 0;
    std::string significant;
    std::size_t digits = 0;
    while (i < text.size() && text[i] >= '0' && text[i] <= '9') { significant += text[i++]; ++digits; }
    const auto integerDigits = digits;
    if (i < text.size() && text[i] == '.') {
        ++i;
        while (i < text.size() && text[i] >= '0' && text[i] <= '9') { significant += text[i++]; ++digits; }
    }
    if (!digits) return nan;
    std::int64_t exponent = 0;
    if (i < text.size() && (text[i] == 'e' || text[i] == 'E')) {
        ++i;
        const bool negativeExponent = i < text.size() && text[i] == '-';
        if (i < text.size() && (text[i] == '+' || text[i] == '-')) ++i;
        const auto start = i;
        const auto cap = static_cast<std::int64_t>(text.size()) + 4000;
        while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
            exponent = std::min(cap, exponent * 10 + (text[i++] - '0'));
        }
        if (negativeExponent) exponent = -exponent;
        if (start == i) return nan;
    }
    if (i != text.size()) return nan;
    const auto first = significant.find_first_not_of('0');
    if (first == std::string::npos) return negative ? -0.0 : 0.0;
    exponent += static_cast<std::int64_t>(integerDigits) - static_cast<std::int64_t>(first) - 1;
    if (exponent > 309) return negative ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    if (exponent < -400) return negative ? -0.0 : 0.0;
    significant.erase(0, first);
    const auto last = significant.find_last_not_of('0');
    significant.resize(last + 1);
    // Binary64 midpoints have terminating decimal expansions shorter than
    // 1,200 significant digits. Preserve a sticky nonzero beyond that bound.
    // This avoids CRT overflow on a huge mantissa and cancelling exponent.
    if (significant.size() > 1200) { significant.resize(1200); significant += '1'; }
    std::string normalized = negative ? "-" : "";
    normalized += significant[0];
    if (significant.size() > 1) normalized += "." + significant.substr(1);
    normalized += "e" + std::to_string(exponent);
    double result = 0;
    const auto parsed = std::from_chars(normalized.data(), normalized.data() + normalized.size(), result,
                                         std::chars_format::general);
    if (parsed.ec == std::errc::result_out_of_range)
        return exponent >= 0 ? (negative ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity())
                             : (negative ? -0.0 : 0.0);
    if (parsed.ec != std::errc{} || parsed.ptr != normalized.data() + normalized.size())
        throw std::runtime_error("Normalized decimal conversion failed");
    return result;
}
std::string numberString(double number) {
    if (std::isnan(number)) return "NaN";
    if (std::isinf(number)) return number < 0 ? "-Infinity" : "Infinity";
    if (number == 0) return "0";
    const bool negative = number < 0;
    const auto magnitude = std::abs(number);
    char buffer[128];
    const auto result = std::to_chars(buffer, buffer + sizeof buffer, magnitude, std::chars_format::general);
    if (result.ec != std::errc{}) throw std::runtime_error("Number formatting failed");
    const std::string raw(buffer, result.ptr);
    const auto e = raw.find_first_of("eE");
    const auto mantissa = raw.substr(0, e);
    int exponent = e == std::string::npos ? 0 : std::stoi(raw.substr(e + 1));
    const auto point = mantissa.find('.');
    int position = static_cast<int>(point == std::string::npos ? mantissa.size() : point) + exponent;
    std::string digits = mantissa;
    if (point != std::string::npos) digits.erase(point, 1);
    while (digits.size() > 1 && digits[0] == '0') { digits.erase(0, 1); --position; }
    while (digits.size() > 1 && digits.back() == '0') digits.pop_back();
    std::string text;
    if (magnitude >= 1e-6 && magnitude < 1e21) {
        if (position <= 0) text = "0." + std::string(static_cast<std::size_t>(-position), '0') + digits;
        else if (position >= static_cast<int>(digits.size()))
            text = digits + std::string(static_cast<std::size_t>(position) - digits.size(), '0');
        else text = digits.substr(0, position) + "." + digits.substr(position);
    } else {
        text = digits.substr(0, 1);
        if (digits.size() > 1) text += "." + digits.substr(1);
        exponent = position - 1;
        text += "e" + std::string(exponent >= 0 ? "+" : "") + std::to_string(exponent);
    }
    return negative ? "-" + text : text;
}
JSPropertyDescriptor dataDescriptor(const JSValue& value, bool writable = true,
                                    bool enumerable = true, bool configurable = true) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value; descriptor.writable = writable;
    descriptor.enumerable = enumerable; descriptor.configurable = configurable;
    return descriptor;
}
JSValue objectValueOf(const JSValue& receiver, const std::vector<JSValue>&, const std::vector<JSValue>&) {
    if (receiver.isNull() || receiver.isUndefined()) throw JSTypeError("Cannot convert null/undefined to object");
    if (!receiver.isObject()) throw JSTypeError("Primitive object wrappers are not implemented");
    return receiver;
}
JSValue objectToString(const JSValue& receiver, const std::vector<JSValue>&, const std::vector<JSValue>&) {
    switch (receiver.type()) {
    case JSValue::Type::Undefined: return "[object Undefined]";
    case JSValue::Type::Null: return "[object Null]";
    case JSValue::Type::Array: return "[object Array]";
    case JSValue::Type::Function: return "[object Function]";
    case JSValue::Type::String: return "[object String]";
    case JSValue::Type::Boolean: return "[object Boolean]";
    case JSValue::Type::Number: return "[object Number]";
    default: return "[object Object]";
    }
}
JSValue emptyFunction(const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) { return {}; }
JSValue nativeToString(const JSValue& receiver, const std::vector<JSValue>&, const std::vector<JSValue>&) {
    return receiver.nativeFunctionSource();
}
JSValue arrayJoin(const JSValue& receiver, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
    if (receiver.isNull() || receiver.isUndefined()) throw JSTypeError("Array.prototype.join on null/undefined");
    double count = receiver.get("length").toNumber();
    if (std::isnan(count) || count < 0) count = 0;
    count = std::min(std::floor(count), 9007199254740991.0);
    const auto separator = args.empty() || args[0].isUndefined() ? std::string(",") : args[0].toString();
    static thread_local std::vector<JSValue> active;
    for (const auto& previous : active) if (previous.strictEquals(receiver)) return "";
    active.push_back(receiver);
    struct Pop { std::vector<JSValue>& values; ~Pop() { values.pop_back(); } } pop{active};
    std::string result;
    for (std::uint64_t i = 0; i < static_cast<std::uint64_t>(count); ++i) {
        if (i) result += separator;
        const auto value = receiver.get(std::to_string(i));
        if (!value.isNull() && !value.isUndefined()) result += value.toString();
    }
    return result;
}
JSValue arrayToString(const JSValue& receiver, const std::vector<JSValue>&, const std::vector<JSValue>&) {
    if (receiver.isNull() || receiver.isUndefined()) throw JSTypeError("Array.prototype.toString on null/undefined");
    const auto join = receiver.get("join");
    return join.isCallable() ? join.call(receiver) : objectToString(receiver, {}, {});
}
using Args = std::vector<JSValue>;
JSValue argument(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue(); }
double integerOrInfinity(JSValue value) {
    const double n = value.toNumber();
    return std::isnan(n) || n == 0 ? 0 : std::trunc(n);
}
std::uint64_t arrayLikeLength(JSValue object) {
    const auto n = integerOrInfinity(object.get("length"));
    return static_cast<std::uint64_t>(std::clamp(n, 0.0, 9007199254740991.0));
}
JSValue arrayObject(JSValue object) {
    if (!object.isObject()) throw JSTypeError("Array operation requires an object receiver");
    return object;
}
void deleteOrThrow(JSValue object, const std::string& key) {
    if (!object.erase(key)) throw JSTypeError("Cannot delete nonconfigurable array property");
}
std::uint64_t relativeIndex(JSValue index, std::uint64_t length) {
    const auto n = integerOrInfinity(index);
    return static_cast<std::uint64_t>(n < 0 ? std::max(double(length) + n, 0.0) : std::min(n, double(length)));
}
// The runtime lacks primitive wrapper prototypes. Preserve ToObject's own
// String-exotic properties and primitive conversion, as Array.map already does.
JSValue arrayMethodObject(const JSValue& receiver) {
    if (receiver.isNull() || receiver.isUndefined())
        throw JSTypeError("Array operation requires a non-null receiver");
    if (receiver.isObject()) return receiver;
    auto object = JSValue::object();
    object.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args&, const Args& captures) { return captures[0]; }, {receiver}));
    if (receiver.type() == JSValue::Type::String) {
        const auto units = utf16(receiver.string());
        for (std::size_t i = 0; i < units.size(); ++i)
            object.defineProperty(std::to_string(i), dataDescriptor(encodeUnit(units[i]), false, true, false));
        object.defineProperty("length", dataDescriptor(units.size(), false, false, false));
    }
    return object;
}
JSValue arrayUnshift(const JSValue& receiver, const Args& args, const Args&) {
    auto object = arrayMethodObject(receiver); const auto length = arrayLikeLength(object);
    const auto count = static_cast<std::uint64_t>(args.size());
    if (count > 9007199254740991ULL - length)
        throw JSTypeError("Array-like length exceeds maximum safe integer");
    if (count != 0) {
        for (auto k = length; k != 0; --k) {
            const auto from = std::to_string(k - 1), to = std::to_string(k + count - 1);
            if (object.has(from)) object.set(to, object.get(from));
            else deleteOrThrow(object, to);
        }
        for (std::uint64_t i = 0; i < count; ++i) object.set(std::to_string(i), args[static_cast<std::size_t>(i)]);
    }
    object.set("length", length + count); return length + count;
}
JSValue arraySort(const JSValue& receiver, const Args& args, const Args&) {
    const auto comparator = argument(args, 0);
    if (!comparator.isUndefined() && !comparator.isCallable())
        throw JSTypeError("Array sort comparator must be callable or undefined");
    auto object = arrayMethodObject(receiver); const auto length = arrayLikeLength(object);
    if (length < 2) return object;
    std::vector<JSValue> values;
    std::uint64_t undefinedCount = 0;
    for (std::uint64_t i = 0; i < length; ++i) {
        const auto key = std::to_string(i);
        if (!object.has(key)) continue;
        const auto value = object.get(key);
        if (value.isUndefined()) ++undefinedCount;
        else values.push_back(value);
    }
    v8ArraySort(values, [&](const JSValue& left, const JSValue& right)->double {
        if (!comparator.isUndefined()) return comparator.call(JSValue(), {left, right}).toNumber();
        const auto leftString = left.toString();
        const auto rightString = right.toString();
        const auto a = utf16(leftString), b = utf16(rightString);
        if (std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end())) return -1;
        if (std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end())) return 1;
        return 0;
    });
    std::uint64_t index = 0;
    for (const auto& value : values) object.set(std::to_string(index++), value);
    while (undefinedCount != 0) { object.set(std::to_string(index++), JSValue()); --undefinedCount; }
    // Write only the original span. Comparator-appended properties survive;
    // removed/shrunk receivers are recreated by the ordered strict writes.
    while (index < length) deleteOrThrow(object, std::to_string(index++));
    return object;
}
JSValue arrayPush(const JSValue& receiver, const Args& args, const Args&) {
    auto object = arrayObject(receiver); auto length = arrayLikeLength(object);
    if (args.size() > 9007199254740991ULL - length) throw JSTypeError("Array-like length exceeds maximum safe integer");
    for (const auto& value : args) object.set(std::to_string(length++), value);
    object.set("length", length); return length;
}
JSValue arrayPop(const JSValue& receiver, const Args&, const Args&) {
    auto object = arrayObject(receiver); const auto length = arrayLikeLength(object);
    if (!length) { object.set("length", 0); return {}; }
    const auto key = std::to_string(length - 1); const auto result = object.get(key);
    deleteOrThrow(object, key); object.set("length", length - 1); return result;
}
JSValue arrayResult(JSValue original, std::uint64_t length) {
    if (original.type() == JSValue::Type::Array) {
        // ArraySpeciesCreate observes constructor even when @@species is absent.
        // Symbol keys/custom species are outside the supported runtime types.
        const auto constructor = original.get("constructor");
        if (!constructor.isUndefined() && !constructor.isObject())
            throw JSTypeError("Array constructor is not an object");
    }
    if (length > 0xffffffffULL) throw JSRangeError("Invalid array length");
    return JSValue::array(static_cast<std::uint32_t>(length));
}
JSValue arraySlice(const JSValue& receiver, const Args& args, const Args&) {
    auto object = arrayObject(receiver); const auto length = arrayLikeLength(object);
    const auto start = relativeIndex(argument(args, 0), length);
    const auto end = argument(args, 1).isUndefined() ? length : relativeIndex(args[1], length);
    const auto count = end > start ? end - start : 0;
    auto result = arrayResult(object, count);
    for (std::uint64_t i = start, n = 0; i < end; ++i, ++n) {
        const auto key = std::to_string(i);
        if (object.has(key)) result.defineProperty(std::to_string(n), dataDescriptor(object.get(key)));
    }
    result.set("length", count); return result;
}
JSValue arrayMap(const JSValue& receiver, const Args& args, const Args&) {
    if (receiver.isNull() || receiver.isUndefined()) throw JSTypeError("Array.prototype.map requires a non-null receiver");
    auto object = receiver;
    if (!object.isObject()) {
        object = JSValue::object();
        // Primitive wrapper prototypes are not part of the current runtime.
        // Preserve ToObject's value conversion and String exotic own properties.
        object.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args&, const Args& captures) { return captures[0]; }, {receiver}));
        if (receiver.type() == JSValue::Type::String) {
            const auto units = utf16(receiver.string());
            for (std::size_t i = 0; i < units.size(); ++i)
                object.defineProperty(std::to_string(i), dataDescriptor(encodeUnit(units[i]), false, true, false));
            object.defineProperty("length", dataDescriptor(units.size(), false, false, false));
        }
    }
    const auto length = arrayLikeLength(object);
    const auto callback = argument(args, 0), thisArg = argument(args, 1);
    if (!callback.isCallable()) throw JSTypeError("Array.prototype.map callback is not callable");
    auto result = arrayResult(object, length);
    for (std::uint64_t i = 0; i < length; ++i) {
        const auto key = std::to_string(i);
        if (object.has(key)) {
            const auto value = object.get(key);
            const auto mapped = callback.call(thisArg, {value, JSValue(i), object});
            result.defineProperty(key, dataDescriptor(mapped));
        }
    }
    return result;
}
JSValue arrayIndexOf(const JSValue& receiver, const Args& args, const Args&) {
    auto object = arrayObject(receiver); const auto length = arrayLikeLength(object);
    if (!length) return -1;
    const auto n = integerOrInfinity(argument(args, 1));
    if (n >= double(length)) return -1;
    for (std::uint64_t i = static_cast<std::uint64_t>(n >= 0 ? n : std::max(double(length) + n, 0.0)); i < length; ++i) {
        const auto key = std::to_string(i);
        if (object.has(key) && object.get(key).strictEquals(argument(args, 0))) return i;
    }
    return -1;
}
JSValue arraySplice(const JSValue& receiver, const Args& args, const Args&) {
    auto object = arrayObject(receiver); const auto length = arrayLikeLength(object);
    const auto start = relativeIndex(argument(args, 0), length);
    const std::uint64_t inserted = args.size() > 2 ? args.size() - 2 : 0;
    const auto removed = args.empty() ? 0 : args.size() == 1 ? length - start :
        static_cast<std::uint64_t>(std::clamp(integerOrInfinity(args[1]), 0.0, double(length - start)));
    if (inserted > 9007199254740991ULL - (length - removed)) throw JSTypeError("Array-like length exceeds maximum safe integer");
    auto result = arrayResult(object, removed);
    for (std::uint64_t k = 0; k < removed; ++k) {
        const auto key = std::to_string(start + k);
        if (object.has(key)) result.defineProperty(std::to_string(k), dataDescriptor(object.get(key)));
    }
    result.set("length", removed);
    if (inserted < removed) {
        for (auto k = start; k < length - removed; ++k) {
            const auto from = std::to_string(k + removed), to = std::to_string(k + inserted);
            if (object.has(from)) object.set(to, object.get(from)); else deleteOrThrow(object, to);
        }
        for (auto k = length; k > length - removed + inserted; --k) deleteOrThrow(object, std::to_string(k - 1));
    } else if (inserted > removed) {
        for (auto k = length - removed; k > start; --k) {
            const auto from = std::to_string(k + removed - 1), to = std::to_string(k + inserted - 1);
            if (object.has(from)) object.set(to, object.get(from)); else deleteOrThrow(object, to);
        }
    }
    for (std::uint64_t k = 0; k < inserted; ++k) object.set(std::to_string(start + k), args[2 + k]);
    object.set("length", length - removed + inserted); return result;
}
JSValue functionCall(const JSValue& receiver, const Args& args, const Args&) {
    return receiver.call(argument(args, 0), Args(args.begin() + std::min<std::size_t>(1, args.size()), args.end()));
}
JSValue objectHasOwn(const JSValue& receiver, const Args& args, const Args&) {
    const auto key = argument(args, 0).toString(); return receiver.hasOwn(key);
}
JSValue objectEnumerable(const JSValue& receiver, const Args& args, const Args&) {
    const auto key = argument(args, 0).toString(); const auto descriptor = receiver.getOwnPropertyDescriptor(key);
    return descriptor && descriptor->enumerable.value_or(false);
}
JSValue objectPrototypeOf(const JSValue& receiver, const Args& args, const Args&) {
    auto candidate = argument(args, 0); if (!candidate.isObject()) return false;
    if (receiver.isNull() || receiver.isUndefined()) throw JSTypeError("Cannot convert null/undefined to object");
    while (!(candidate = candidate.getPrototype()).isNull()) if (candidate.strictEquals(receiver)) return true;
    return false;
}
JSValue objectGetProto(const JSValue& receiver, const Args&, const Args&) { return receiver.getPrototype(); }
JSValue objectSetProto(const JSValue& receiver, const Args& args, const Args&) {
    if (receiver.isNull() || receiver.isUndefined()) throw JSTypeError("Cannot convert null/undefined to object");
    const auto prototype = argument(args, 0);
    if (!prototype.isObject() && !prototype.isNull()) return {};
    if (receiver.isObject()) { auto object = receiver; if (!object.setPrototype(prototype)) throw JSTypeError("Cannot set prototype"); }
    return {};
}
JSValue objectConstructor(const JSValue&, const Args& args, const Args&) {
    auto value = argument(args, 0);
    if (value.isObject()) return value;
    if (!value.isNull() && !value.isUndefined()) throw JSTypeError("Primitive object wrappers are not implemented");
    return JSValue::object();
}
JSValue arrayConstructor(const JSValue&, const Args& args, const Args&) {
    if (args.size() == 1 && args[0].type() == JSValue::Type::Number) {
        const auto n = args[0].number();
        if (!std::isfinite(n) || n < 0 || n > 4294967295.0 || std::trunc(n) != n) throw JSRangeError("Invalid array length");
        return JSValue::array(static_cast<std::uint32_t>(n));
    }
    auto array = JSValue::array(); arrayPush(array, args, {}); return array;
}
JSValue objectLocaleString(const JSValue& receiver, const Args&, const Args&) {
    const auto method = receiver.get("toString"); return method.call(receiver);
}
JSValue objectDefineAccessor(const JSValue& receiver, const Args& args, const Args& captures) {
    auto object = arrayObject(receiver); const auto callback = argument(args, 1);
    if (!callback.isCallable()) throw JSTypeError("Accessor must be callable");
    const auto key = argument(args, 0).toString(); JSPropertyDescriptor descriptor;
    if (captures[0].toBoolean()) descriptor.getter = callback; else descriptor.setter = callback;
    descriptor.enumerable = true; descriptor.configurable = true; object.defineProperty(key, descriptor); return {};
}
JSValue objectLookupAccessor(const JSValue& receiver, const Args& args, const Args& captures) {
    auto object = arrayObject(receiver); const auto key = argument(args, 0).toString();
    while (!object.isNull()) {
        const auto descriptor = object.getOwnPropertyDescriptor(key);
        if (descriptor) return captures[0].toBoolean() ? descriptor->getter.value_or(JSValue()) : descriptor->setter.value_or(JSValue());
        object = object.getPrototype();
    }
    return {};
}
}

class JSHeap : public std::enable_shared_from_this<JSHeap> {
  public:
    struct Property {
        JSValue value, getter, setter;
        bool accessor = false, writable = false, enumerable = false, configurable = false;
    };
    struct Record {
        enum class ConstructMode { None, Class, Callable, Object, Array, Builtin };
        OrderedMap<Property> properties;
        OrderedMap<JSValue> hostSlots;
        std::shared_ptr<void> hostPayload;
        std::shared_ptr<const JSHostObjectHooks> hostHooks;
        std::string hostTag;
        JSValue::Type hostType = JSValue::Type::Object;
        JSValue prototype = nullptr, primitiveConverter;
        std::vector<JSValue> captures;
        // Ordinary records and empty WeakMaps allocate no conditional table.
        // Keys are non-owning identities. Values are graph edges which become
        // strong only when both this map and its key have been marked.
        bool weakMap = false;
        std::unique_ptr<std::unordered_map<std::uint64_t, JSValue>> weakEntries;
        JSValue::NativeFunction callback = nullptr;
        ConstructMode constructMode = ConstructMode::None;
        std::string nativeName;
        std::uint32_t length = 0;
        std::size_t roots = 0;
        bool lengthWritable = true, extensible = true, intrinsic = false;
        bool immutablePrototype = false, hasPrimitiveConverter = false;
    };
    struct HostKey {
        const void* identity;
        std::string tag;
        JSValue::Type type;
        bool operator==(const HostKey&) const = default;
    };
    struct HostKeyHash {
        std::size_t operator()(const HostKey& key) const {
            return std::hash<const void*>{}(key.identity) ^
                   (std::hash<std::string>{}(key.tag) << 1) ^ (std::size_t(key.type) << 2);
        }
    };
    std::unordered_map<std::uint64_t, std::unique_ptr<Record>> records;
    std::unordered_map<HostKey, std::uint64_t, HostKeyHash> hosts;
    std::uint64_t nextId = 0;
    JSValue objectPrototype, arrayPrototype, functionPrototype;
    bool initialized = false, collecting = false;
    static std::weak_ptr<JSHeap>& currentHeapStorage() {
        static thread_local std::weak_ptr<JSHeap> heap;
        return heap;
    }
    static std::shared_ptr<JSHeap> current() {
        auto& currentHeap = currentHeapStorage();
        auto heap = currentHeap.lock();
        if (!heap) { heap = std::make_shared<JSHeap>(); currentHeap = heap; }
        heap->initialize();
        return heap;
    }
    JSValue allocate(JSValue::Type type, std::uint32_t length = 0) {
        if (nextId == std::numeric_limits<std::uint64_t>::max())
            throw JSRangeError("Runtime record identity exhausted");
        const auto id = nextId++;
        auto record = std::make_unique<Record>(); record->length = length;
        records.emplace(id, std::move(record));
        return JSValue(shared_from_this(), id, type);
    }
    Record& record(const JSValue& value) const { return *records.at(std::get<JSValue::Reference>(value.value_).id); }
    void validateEdge(const JSValue& value) const {
        if (value.isObject() && value.heap_ != this)
            throw std::logic_error("Cross-heap runtime references are not implemented");
    }
    JSValue createFunction(JSValue::NativeFunction callback, const std::vector<JSValue>& captures,
                           const std::string& name, std::size_t arity) {
        for (const auto& value : captures) validateEdge(value);
        auto value = allocate(JSValue::Type::Function);
        auto& entry = record(value);
        entry.prototype = functionPrototype.graphEdge(); entry.callback = callback; entry.nativeName = name;
        for (const auto& capture : captures) entry.captures.push_back(capture.graphEdge());
        value.defineProperty("length", dataDescriptor(arity, false, false, true));
        value.defineProperty("name", dataDescriptor(name, false, false, true));
        return value;
    }
    void initialize() {
        if (initialized) return;
        initialized = true;
        auto object = allocate(JSValue::Type::Object);
        auto function = allocate(JSValue::Type::Function);
        auto array = allocate(JSValue::Type::Array);
        objectPrototype = object.graphEdge(); functionPrototype = function.graphEdge(); arrayPrototype = array.graphEdge();
        record(object).intrinsic = record(function).intrinsic = record(array).intrinsic = true;
        record(object).immutablePrototype = true;
        record(function).prototype = object.graphEdge(); record(function).callback = emptyFunction;
        record(array).prototype = object.graphEdge();
        function.defineProperty("length", dataDescriptor(0, false, false, true));
        function.defineProperty("name", dataDescriptor("", false, false, true));
        auto install = [&](JSValue& target, const std::string& name, JSValue::NativeFunction callback, unsigned arity) {
            auto value = createFunction(callback, {}, name, arity); record(value).intrinsic = true;
            target.defineProperty(name, dataDescriptor(value, true, false, true));
        };
        install(object, "valueOf", objectValueOf, 0); install(object, "toString", objectToString, 0);
        install(object, "hasOwnProperty", objectHasOwn, 1);
        install(object, "propertyIsEnumerable", objectEnumerable, 1);
        install(object, "isPrototypeOf", objectPrototypeOf, 1);
        install(object, "toLocaleString", objectLocaleString, 0);
        for (const bool getter : {true, false}) {
            const auto defineName = getter ? "__defineGetter__" : "__defineSetter__";
            const auto lookupName = getter ? "__lookupGetter__" : "__lookupSetter__";
            object.defineProperty(defineName, dataDescriptor(createFunction(objectDefineAccessor, {getter}, defineName, 2), true, false, true));
            object.defineProperty(lookupName, dataDescriptor(createFunction(objectLookupAccessor, {getter}, lookupName, 1), true, false, true));
        }
        auto constructor = createFunction(objectConstructor, {}, "Object", 1);
        record(constructor).intrinsic = true;
        record(constructor).constructMode = Record::ConstructMode::Object;
        constructor.defineProperty("prototype", dataDescriptor(object, false, false, false));
        object.defineProperty("constructor", dataDescriptor(constructor, true, false, true));
        JSPropertyDescriptor proto;
        proto.getter = createFunction(objectGetProto, {}, "get __proto__", 0);
        proto.setter = createFunction(objectSetProto, {}, "set __proto__", 1);
        proto.enumerable = false; proto.configurable = true; object.defineProperty("__proto__", proto);
        install(function, "toString", nativeToString, 0);
        install(function, "call", functionCall, 1);
        install(array, "join", arrayJoin, 1); install(array, "toString", arrayToString, 0);
        install(array, "push", arrayPush, 1); install(array, "pop", arrayPop, 0);
        install(array, "unshift", arrayUnshift, 1); install(array, "sort", arraySort, 1);
        install(array, "slice", arraySlice, 2); install(array, "indexOf", arrayIndexOf, 1);
        install(array, "splice", arraySplice, 2);
        install(array, "map", arrayMap, 1);
        auto arrayCtor = createFunction(arrayConstructor, {}, "Array", 1);
        record(arrayCtor).intrinsic = true;
        record(arrayCtor).constructMode = Record::ConstructMode::Array;
        arrayCtor.defineProperty("prototype", dataDescriptor(array, false, false, false));
        array.defineProperty("constructor", dataDescriptor(arrayCtor, true, false, true));
    }
    std::size_t collect() {
        // Native destructors may read JS fields, retain them externally, or ask
        // for another collection. Keep records readable during finalization and
        // re-mark after each owner; nested collection cannot start a second sweep.
        if (collecting) return 0;
        collecting = true;
        struct Reset { bool& value; ~Reset() { value = false; } } reset{collecting};
        const auto keepHeap = shared_from_this();
        std::vector<std::uint64_t> finalizePayloads;
        auto mark = [&] {
        finalizePayloads.clear();
        using Owner = std::weak_ptr<void>;
        struct NativeNode {
            std::unordered_set<const JSValue*> roots;
            std::map<const void*, Owner> edges;
            std::size_t internalOwners = 0;
            std::vector<std::uint64_t> hosts;
        };
        std::map<Owner, NativeNode, std::owner_less<Owner>> native;
        for (const auto& [id, value] : records) {
            const auto hooks = value->hostHooks;
            if (value->hostPayload && hooks && hooks->nativeOwnership) {
                const JSValue self(shared_from_this(), id, value->hostType);
                for (const auto& description : hooks->nativeOwnership(self)) {
                    if (description.owner.expired()) continue;
                    auto& node = native[description.owner];
                    for (const auto* root : description.roots) if (root) node.roots.insert(root);
                    for (const auto& link : description.edges) if (!link.owner.expired()) node.edges[link.slot] = link.owner;
                }
            }
        }
        // An edge can name an owner without describing fields (for example a
        // child payload without its own ownership hook). Its cached wrappers
        // still belong to the same native reachability graph.
        std::vector<Owner> referencedOwners;
        for (const auto& [owner, node] : native)
            for (const auto& [slot, child] : node.edges) referencedOwners.push_back(child);
        for (const auto& owner : referencedOwners) native.try_emplace(owner);
        // Every host payload is an internal owner, including aliasing payloads
        // with a different host tag but the same shared_ptr control block.
        std::map<Owner, std::vector<std::uint64_t>, std::owner_less<Owner>> payloadGroups;
        for (const auto& [id, value] : records) {
            if (value->hostPayload) {
                payloadGroups[Owner(value->hostPayload)].push_back(id);
                auto found = native.find(Owner(value->hostPayload));
                if (found != native.end()) {
                    ++found->second.internalOwners;
                    found->second.hosts.push_back(id);
                }
            }
        }
        std::unordered_set<const void*> countedSlots;
        std::unordered_set<const JSValue*> countedRoots;
        std::unordered_map<std::uint64_t, std::size_t> internalRoots;
        for (const auto& [owner, node] : native) {
            for (const auto& [slot, child] : node.edges) {
                if (countedSlots.insert(slot).second) {
                    auto found = native.find(child);
                    if (found != native.end()) ++found->second.internalOwners;
                }
            }
            for (const auto* value : node.roots) {
                if (countedRoots.insert(value).second && value->isObject()) {
                    validateEdge(*value);
                    if (value->owner_) ++internalRoots[std::get<JSValue::Reference>(value->value_).id];
                }
            }
        }
        std::unordered_set<std::uint64_t> marked;
        std::vector<std::uint64_t> pending;
        // Register conditional edges only when their map becomes reachable.
        // A key's first mark activates all values waiting on that identity.
        // This index is rebuilt after every native finalizer's graph mutation.
        std::unordered_map<std::uint64_t, std::vector<std::uint64_t>> ephemeronWaiting;
        for (const auto& [id, value] : records) {
            if (value->roots > internalRoots[id] || value->intrinsic) pending.push_back(id);
        }
        auto edge = [&](const JSValue& value) {
            if (value.isObject()) pending.push_back(std::get<JSValue::Reference>(value.value_).id);
        };
        std::vector<Owner> nativePending;
        std::map<Owner, bool, std::owner_less<Owner>> nativeMarked;
        // A pure shared_ptr cycle cannot be destroyed by removing JS records.
        // Preserve its JS fields (and native descendants) until the application
        // explicitly breaks that cycle. Pruning zero-incoming nodes leaves
        // precisely cycles and their exits, without retaining acyclic JS/native
        // cycles whose ownership can actually be released by the sweep.
        std::map<Owner, std::size_t, std::owner_less<Owner>> incoming;
        for (const auto& [owner, node] : native) incoming.emplace(owner, 0);
        for (const auto& [owner, node] : native) {
            for (const auto& [slot, child] : node.edges) {
                const auto found = incoming.find(child);
                if (found != incoming.end()) ++found->second;
            }
        }
        std::vector<Owner> leaves;
        for (const auto& [owner, count] : incoming) if (count == 0) leaves.push_back(owner);
        while (!leaves.empty()) {
            const auto owner = leaves.back(); leaves.pop_back();
            for (const auto& [slot, child] : native.at(owner).edges) {
                const auto found = incoming.find(child);
                if (found != incoming.end() && --found->second == 0) leaves.push_back(child);
            }
        }
        for (const auto& [owner, node] : native) {
            if (owner.use_count() > static_cast<long>(node.internalOwners) || incoming.at(owner) != 0)
                nativePending.push_back(owner);
        }
        while (!pending.empty() || !nativePending.empty()) {
            while (!nativePending.empty()) {
                const auto owner = nativePending.back(); nativePending.pop_back();
                if (!nativeMarked.emplace(owner, true).second) continue;
                const auto found = native.find(owner);
                if (found == native.end()) continue;
                for (const auto host : found->second.hosts) pending.push_back(host);
                for (const auto* root : found->second.roots) edge(*root);
                for (const auto& [slot, child] : found->second.edges) nativePending.push_back(child);
            }
            if (pending.empty()) continue;
            const auto id = pending.back(); pending.pop_back();
            if (!marked.insert(id).second) continue;
            if (auto waiting = ephemeronWaiting.find(id); waiting != ephemeronWaiting.end()) {
                pending.insert(pending.end(), waiting->second.begin(), waiting->second.end());
                ephemeronWaiting.erase(waiting);
            }
            const auto& record = *records.at(id);
            if (record.weakMap && record.weakEntries) {
                for (const auto& [key, value] : *record.weakEntries) {
                    if (!value.isObject()) continue;
                    const auto target = std::get<JSValue::Reference>(value.value_).id;
                    if (marked.contains(target)) continue;
                    if (marked.contains(key)) pending.push_back(target);
                    else ephemeronWaiting[key].push_back(target);
                }
            }
            if (record.hostPayload) nativePending.emplace_back(record.hostPayload);
            const auto hooks = record.hostHooks;
            if (record.hostPayload && hooks && hooks->trace) {
                const JSValue host(shared_from_this(), id, record.hostType);
                for (const auto& value : hooks->trace(host)) { validateEdge(value); edge(value); }
            }
            edge(record.prototype); edge(record.primitiveConverter);
            for (const auto& capture : record.captures) edge(capture);
            for (const auto& [key, value] : record.hostSlots) { (void) key; edge(value); }
            for (const auto& [key, value] : record.properties) {
                (void) key; edge(value.value); edge(value.getter); edge(value.setter);
            }
        }
        // Release one unreachable control block at a time, including all alias
        // payloads. Parents precede their native children: this keeps ownership
        // descriptions available until each described native field is destroyed.
        for (const auto& [owner, hosts] : payloadGroups) {
            if (std::any_of(hosts.begin(), hosts.end(), [&](auto id) { return marked.contains(id); })) continue;
            const auto found = native.find(owner);
            if (found == native.end() || found->second.internalOwners == hosts.size()) {
                finalizePayloads = hosts;
                break;
            }
        }
        return marked;
        };
        auto marked = mark();
        while (!finalizePayloads.empty()) {
            std::vector<std::shared_ptr<void>> payloads;
            for (const auto id : finalizePayloads) {
                auto& record = *records.at(id);
                payloads.push_back(std::move(record.hostPayload));
                record.hostHooks.reset();
                for (auto entry = hosts.begin(); entry != hosts.end();) {
                    if (entry->second == id) entry = hosts.erase(entry); else ++entry;
                }
            }
            // All alias identity entries are gone before the final shared_ptr
            // is released. An allocation in a destructor cannot reuse a retired
            // wrapper if its native address is reused.
            payloads.clear();
            marked = mark();
        }
        // Finalizers above can resurrect ordinary JS records or change weak
        // entries. Prune only after the final re-mark, before detached records
        // are destroyed, so no stale key can later match an unrelated record.
        for (const auto& [id, record] : records) {
            if (record->weakEntries && marked.contains(id))
                std::erase_if(*record->weakEntries, [&](const auto& entry) { return !marked.contains(entry.first); });
        }
        std::vector<std::unique_ptr<Record>> garbage;
        for (auto entry = records.begin(); entry != records.end();) {
            if (!marked.contains(entry->first)) {
                garbage.push_back(std::move(entry->second));
                entry = records.erase(entry);
            }
            else ++entry;
        }
        for (auto entry = hosts.begin(); entry != hosts.end();) {
            if (!records.contains(entry->second)) entry = hosts.erase(entry);
            else ++entry;
        }
        return garbage.size();
    }
};

void JSValue::retainForHeapLifetime() const {
    static_cast<void>(JSWeakValue(*this));
    if (!isObject()) throw JSTypeError("A runtime module root must be an object");
    heap_->record(*this).intrinsic = true;
}

JSValue JSValue::createWeakMapData(const JSValue& prototype) {
    static_cast<void>(JSWeakValue(prototype));
    if (!prototype.isObject() && !prototype.isNull()) throw JSTypeError("WeakMap prototype must be object or null");
    const auto heap = JSHeap::current();
    heap->validateEdge(prototype);
    auto result = heap->allocate(Type::Object);
    auto& record = heap->record(result); record.weakMap = true;
    record.prototype = prototype.graphEdge();
    return result;
}
bool JSValue::hasWeakMapData() const {
    static_cast<void>(JSWeakValue(*this));
    return isObject() && heap_->record(*this).weakMap;
}
JSValue JSValue::weakMapGet(const JSValue& key) const {
    if (!hasWeakMapData()) throw JSTypeError("WeakMap.get receiver lacks WeakMap data");
    if (!key.isObject()) return {};
    static_cast<void>(JSWeakValue(key)); heap_->validateEdge(key);
    const auto& entries = heap_->record(*this).weakEntries;
    if (!entries) return {};
    const auto found = entries->find(std::get<Reference>(key.value_).id);
    return found == entries->end() ? JSValue{} : JSValue(found->second);
}
bool JSValue::weakMapHas(const JSValue& key) const {
    if (!hasWeakMapData()) throw JSTypeError("WeakMap.has receiver lacks WeakMap data");
    if (!key.isObject()) return false;
    static_cast<void>(JSWeakValue(key)); heap_->validateEdge(key);
    const auto& entries = heap_->record(*this).weakEntries;
    return entries && entries->contains(std::get<Reference>(key.value_).id);
}
bool JSValue::weakMapErase(const JSValue& key) {
    if (!hasWeakMapData()) throw JSTypeError("WeakMap.delete receiver lacks WeakMap data");
    if (!key.isObject()) return false;
    static_cast<void>(JSWeakValue(key)); heap_->validateEdge(key);
    const auto& entries = heap_->record(*this).weakEntries;
    return entries && entries->erase(std::get<Reference>(key.value_).id) != 0;
}
void JSValue::weakMapSet(const JSValue& key, const JSValue& value) {
    if (!hasWeakMapData()) throw JSTypeError("WeakMap.set receiver lacks WeakMap data");
    if (!key.isObject()) throw JSTypeError("Invalid value used as weak map key");
    static_cast<void>(JSWeakValue(key)); static_cast<void>(JSWeakValue(value));
    heap_->validateEdge(key); heap_->validateEdge(value);
    auto& entries = heap_->record(*this).weakEntries;
    if (!entries) entries = std::make_unique<std::unordered_map<std::uint64_t, JSValue>>();
    entries->insert_or_assign(std::get<Reference>(key.value_).id, value.graphEdge());
}

JSWeakValue::JSWeakValue(const JSValue& value) : value_(value.value_) {
    if (!std::holds_alternative<JSValue::Reference>(value_)) return;
    // Compare identities before reading heap storage. A foreign JSValue can
    // only name a different thread's heap; this check never creates a heap.
    const auto heap = JSHeap::currentHeapStorage().lock();
    if (!heap || heap.get() != value.heap_)
        throw JSTypeError("Weak runtime references cannot access another thread's heap");
    threadToken_ = runtimeThreadToken();
    heap_ = heap;
}
JSValue::Type JSWeakValue::type() const {
    if (const auto reference = std::get_if<JSValue::Reference>(&value_)) return reference->type;
    return static_cast<JSValue::Type>(value_.index());
}
bool JSWeakValue::isObject() const {
    return std::holds_alternative<JSValue::Reference>(value_);
}
void JSWeakValue::checkThread() const {
    if (isObject() && threadToken_ != runtimeThreadToken())
        throw JSTypeError("Weak runtime references cannot access another thread's heap");
}
bool JSWeakValue::expired() const {
    if (!isObject()) return false;
    checkThread();
    const auto heap = heap_.lock();
    return !heap || !heap->records.contains(std::get<JSValue::Reference>(value_).id);
}
std::optional<JSValue> JSWeakValue::lock() const {
    if (!isObject()) {
        JSValue result;
        result.value_ = value_;
        return result;
    }
    checkThread();
    const auto heap = heap_.lock();
    const auto reference = std::get<JSValue::Reference>(value_);
    if (!heap || !heap->records.contains(reference.id)) return std::nullopt;
    return JSValue(heap, reference.id, reference.type);
}

JSValue::JSValue(std::shared_ptr<JSHeap> heap, std::uint64_t id, Type type)
    : value_(Reference{id, type}), owner_(std::move(heap)), heap_(owner_.get()) { ++heap_->record(*this).roots; }
void JSValue::retain() { if (heap_) { owner_ = heap_->shared_from_this(); ++heap_->record(*this).roots; } }
void JSValue::release() {
    if (owner_) {
        const auto id = std::get<Reference>(value_).id;
        const auto found = heap_->records.find(id);
        // Owned native fields can be destroyed after their unreachable JS graph
        // has been detached from the heap during the current sweep.
        if (found != heap_->records.end()) --found->second->roots;
    }
    owner_.reset(); heap_ = nullptr;
}
JSValue::JSValue(const JSValue& other) : value_(other.value_), heap_(other.heap_) { retain(); }
JSValue::JSValue(JSValue&& other) noexcept
    : value_(std::move(other.value_)), owner_(std::move(other.owner_)), heap_(other.heap_) {
    other.heap_ = nullptr; other.value_ = Undefined{};
}
JSValue& JSValue::operator=(const JSValue& other) {
    if (this != &other) { JSValue replacement(other); *this = std::move(replacement); }
    return *this;
}
JSValue& JSValue::operator=(JSValue&& other) noexcept {
    if (this != &other) {
        release(); value_ = std::move(other.value_); owner_ = std::move(other.owner_); heap_ = other.heap_;
        other.heap_ = nullptr; other.value_ = Undefined{};
    }
    return *this;
}
JSValue::~JSValue() { release(); }
std::string JSValue::normalizeString(const std::string& value) {
    const auto units = utf16(value);
    std::string result;
    for (std::size_t i = 0; i < units.size(); ++i) {
        const auto unit = units[i];
        if (unit >= 0xd800 && unit <= 0xdbff && i + 1 < units.size() && units[i + 1] >= 0xdc00 && units[i + 1] <= 0xdfff) {
            const auto code = 0x10000u + ((unit - 0xd800u) << 10) + (units[++i] - 0xdc00u);
            result += static_cast<char>(0xf0 | (code >> 18));
            result += static_cast<char>(0x80 | ((code >> 12) & 63));
            result += static_cast<char>(0x80 | ((code >> 6) & 63));
            result += static_cast<char>(0x80 | (code & 63));
        } else result += encodeUnit(unit);
    }
    return result;
}
JSValue JSValue::graphEdge() const { JSValue edge; edge.value_ = value_; edge.heap_ = heap_; return edge; }
JSValue JSValue::object() {
    auto heap = JSHeap::current(); auto value = heap->allocate(Type::Object);
    heap->record(value).prototype = heap->objectPrototype.graphEdge(); return value;
}
JSValue JSValue::object(const JSValue& prototype) {
    if (!prototype.isObject() && !prototype.isNull()) throw JSTypeError("Prototype must be object or null");
    auto heap = prototype.isObject() ? prototype.heap_->shared_from_this() : JSHeap::current();
    auto value = heap->allocate(Type::Object); heap->record(value).prototype = prototype.graphEdge(); return value;
}
JSValue JSValue::array(std::uint32_t length) {
    auto heap = JSHeap::current(); auto value = heap->allocate(Type::Array, length);
    heap->record(value).prototype = heap->arrayPrototype.graphEdge(); return value;
}
JSValue JSValue::hostObject(std::shared_ptr<void> payload, const void* identity,
                           const std::string& tag, Type type) {
    if (!payload || !identity || tag.empty()) throw JSTypeError("Native host requires payload, identity and tag");
    if (type != Type::Object && type != Type::Array) throw JSTypeError("Native host must be an object or array");
    auto heap = JSHeap::current();
    JSHeap::HostKey key{identity, tag, type};
    const auto found = heap->hosts.find(key);
    if (found != heap->hosts.end()) return JSValue(heap, found->second, type);
    auto value = heap->allocate(type);
    auto& record = heap->record(value);
    record.prototype = (type == Type::Array ? heap->arrayPrototype : heap->objectPrototype).graphEdge();
    record.hostPayload = std::move(payload); record.hostTag = tag; record.hostType = type;
    heap->hosts.emplace(std::move(key), std::get<Reference>(value.value_).id);
    return value;
}
bool JSValue::isHostObject(const std::string& tag) const {
    return isObject() && bool(heap_->record(*this).hostPayload) && heap_->record(*this).hostTag == tag;
}
std::shared_ptr<void> JSValue::hostPayload(const std::string& expectedTag) const {
    if (!isHostObject(expectedTag)) throw JSTypeError("Incompatible native receiver for " + expectedTag);
    return heap_->record(*this).hostPayload;
}
void JSValue::setHostHooks(std::shared_ptr<const JSHostObjectHooks> hooks) {
    if (!isObject() || !heap_->record(*this).hostPayload) throw JSTypeError("Expected native host");
    heap_->record(*this).hostHooks = std::move(hooks);
}
void JSValue::setHostSlot(const std::string& key, const JSValue& value) {
    if (!isObject() || !heap_->record(*this).hostPayload) throw JSTypeError("Expected native host");
    heap_->validateEdge(value);
    heap_->record(*this).hostSlots[key] = value.graphEdge();
}
JSValue JSValue::getHostSlot(const std::string& key) const {
    if (!isObject() || !heap_->record(*this).hostPayload) throw JSTypeError("Expected native host");
    const auto& slots = heap_->record(*this).hostSlots;
    const auto found = slots.find(key);
    return found == slots.end() ? JSValue{} : found->second;
}
JSValue JSValue::findHostObject(const void* identity, const std::string& tag, Type type) const {
    if (!isObject()) throw JSTypeError("Native cache lookup requires a runtime object");
    const auto found = heap_->hosts.find(JSHeap::HostKey{identity, tag, type});
    return found == heap_->hosts.end() ? JSValue{} : JSValue(heap_->shared_from_this(), found->second, type);
}
JSValue JSValue::function(NativeFunction callback, const std::vector<JSValue>& captures,
                         const std::string& name, std::uint32_t arity) {
    if (!callback) throw JSTypeError("Function requires a callback");
    return JSHeap::current()->createFunction(callback, captures, name, arity);
}
JSValue JSValue::constructor(NativeFunction callback, const std::vector<JSValue>& captures,
                            const std::string& name, std::size_t arity, ConstructorKind kind) {
    if (!callback) throw JSTypeError("Constructor requires a callback");
    const auto heap = JSHeap::current();
    auto value = heap->createFunction(callback, captures, name, arity);
    heap->record(value).constructMode = kind == ConstructorKind::Class ?
        JSHeap::Record::ConstructMode::Class : JSHeap::Record::ConstructMode::Callable;
    auto prototype = object();
    prototype.defineProperty("constructor", dataDescriptor(value, true, false, true));
    value.defineProperty("prototype", dataDescriptor(prototype, kind == ConstructorKind::Callable, false, false));
    return value;
}
JSValue JSValue::builtinConstructor(NativeFunction callback, const std::vector<JSValue>& captures,
                                   const std::string& name, std::size_t arity) {
    auto value = constructor(callback, captures, name, arity);
    value.heap_->record(value).constructMode = JSHeap::Record::ConstructMode::Builtin;
    return value;
}
bool JSValue::isConstructible() const {
    return isCallable() && heap_->record(*this).constructMode != JSHeap::Record::ConstructMode::None;
}
JSValue::Type JSValue::type() const {
    if (const auto reference = std::get_if<Reference>(&value_)) return reference->type;
    return static_cast<Type>(value_.index());
}
bool JSValue::strictEquals(const JSValue& other) const {
    if (type() != other.type()) return false;
    switch (type()) {
    case Type::Undefined: case Type::Null: return true;
    case Type::Boolean: return std::get<bool>(value_) == std::get<bool>(other.value_);
    case Type::Number: return number() == other.number();
    case Type::String: return string() == other.string();
    default: return heap_ == other.heap_ && std::get<Reference>(value_).id == std::get<Reference>(other.value_).id;
    }
}
bool JSValue::sameValue(const JSValue& other) const {
    if (type() == Type::Number && other.type() == Type::Number) {
        if (std::isnan(number()) && std::isnan(other.number())) return true;
        if (number() == 0 && other.number() == 0) return std::signbit(number()) == std::signbit(other.number());
    }
    return strictEquals(other);
}
bool JSValue::toBoolean() const {
    switch (type()) {
    case Type::Undefined: case Type::Null: return false;
    case Type::Boolean: return std::get<bool>(value_);
    case Type::Number: return number() != 0 && !std::isnan(number());
    case Type::String: return !string().empty();
    default: return true;
    }
}
JSValue JSValue::toPrimitive(PreferredType hint) const {
    const JSValue self = *this;
    if (!self.isObject()) return self;
    JSValue converter;
    for (auto value = self; !value.isNull(); value = value.getPrototype()) {
        const auto& record = value.heap_->record(value);
        if (record.hasPrimitiveConverter) { converter = record.primitiveConverter; break; }
    }
    if (!converter.isUndefined() && !converter.isNull()) {
        if (!converter.isCallable()) throw JSTypeError("@@toPrimitive is not callable");
        auto value = converter.call(self, {hint == PreferredType::String ? "string" :
                                          hint == PreferredType::Number ? "number" : "default"});
        if (value.isObject()) throw JSTypeError("@@toPrimitive returned an object");
        return value;
    }
    const std::string names[2] = {hint == PreferredType::String ? "toString" : "valueOf",
                                  hint == PreferredType::String ? "valueOf" : "toString"};
    for (const auto& name : names) {
        const auto method = self.get(name);
        if (method.isCallable()) { auto value = method.call(self); if (!value.isObject()) return value; }
    }
    throw JSTypeError("Cannot convert object to primitive value");
}
double JSValue::toNumber() const {
    switch (type()) {
    case Type::Undefined: return std::numeric_limits<double>::quiet_NaN();
    case Type::Null: return 0;
    case Type::Boolean: return std::get<bool>(value_) ? 1 : 0;
    case Type::Number: return number();
    case Type::String: return stringNumber(string());
    default: return toPrimitive(PreferredType::Number).toNumber();
    }
}
std::string JSValue::toString() const {
    switch (type()) {
    case Type::Undefined: return "undefined";
    case Type::Null: return "null";
    case Type::Boolean: return std::get<bool>(value_) ? "true" : "false";
    case Type::Number: return numberString(number());
    case Type::String: return string();
    default: return toPrimitive(PreferredType::String).toString();
    }
}
JSValue JSValue::call() const { return call(JSValue{}); }
JSValue JSValue::call(const JSValue& receiver, const std::vector<JSValue>& args) const {
    if (!isCallable()) throw JSTypeError("Value is not callable");
    // Root everything before user code detaches properties or runs collection.
    const JSValue function = *this, rootedReceiver = receiver;
    const std::vector<JSValue> rootedArgs = args;
    const auto& record = heap_->record(function);
    if (record.constructMode == JSHeap::Record::ConstructMode::Class ||
        record.constructMode == JSHeap::Record::ConstructMode::Builtin)
        throw JSTypeError("Class constructor cannot be invoked without new");
    const std::vector<JSValue> captures = record.captures;
    const auto callback = record.callback;
    return callback(rootedReceiver, rootedArgs, captures);
}
JSValue JSValue::construct(const std::vector<JSValue>& args) const { return construct(args, *this); }
JSValue JSValue::construct(const std::vector<JSValue>& args, const JSValue& newTarget) const {
    if (!isConstructible()) throw JSTypeError("Value is not a constructor");
    if (!newTarget.isConstructible()) throw JSTypeError("newTarget is not a constructor");
    // Prototype access can invoke user code, detach the constructor and its
    // arguments, collect garbage, or reenter construction. Root them first.
    const JSValue function = *this, rootedNewTarget = newTarget;
    const std::vector<JSValue> rootedArgs = args;
    heap_->validateEdge(rootedNewTarget);
    for (const auto& argument : rootedArgs) heap_->validateEdge(argument);
    const auto& record = heap_->record(function);
    const auto mode = record.constructMode;
    const auto callback = record.callback;
    const std::vector<JSValue> captures = record.captures;
    if (mode == JSHeap::Record::ConstructMode::Object && rootedNewTarget.sameValue(function))
        return callback(JSValue{}, rootedArgs, captures);
    if (mode == JSHeap::Record::ConstructMode::Builtin) {
        auto result = callback(rootedNewTarget, rootedArgs, captures);
        if (!result.isObject()) throw JSTypeError("Intrinsic constructor must return an object");
        return result;
    }
    auto prototype = rootedNewTarget.get("prototype");
    if (!prototype.isObject()) prototype = mode == JSHeap::Record::ConstructMode::Array ?
        heap_->arrayPrototype : heap_->objectPrototype;
    auto receiver = object(prototype);
    if (mode == JSHeap::Record::ConstructMode::Object) return receiver;
    auto result = callback(receiver, rootedArgs, captures);
    if (mode == JSHeap::Record::ConstructMode::Array) result.setPrototype(prototype);
    return result.isObject() ? result : receiver;
}
std::string JSValue::nativeFunctionSource() const {
    if (!isCallable()) throw JSTypeError("Function.prototype.toString requires a function");
    return "function " + heap_->record(*this).nativeName + "() { [native code] }";
}
std::optional<JSValue::PropertyDescriptor> JSValue::getOwnPropertyDescriptor(const std::string& rawKey) const {
    const auto key = normalizeString(rawKey);
    if (!isObject()) throw JSTypeError("Expected runtime object");
    const JSValue self = *this;
    const auto hooks = heap_->record(self).hostHooks;
    if (hooks && hooks->getOwnProperty) {
        auto result = hooks->getOwnProperty(self, key);
        if (result.handled) return result.descriptor;
    }
    const auto& record = self.heap_->record(self);
    if (self.type() == Type::Array && key == "length") return dataDescriptor(record.length, record.lengthWritable, false, false);
    const auto entry = record.properties.find(key);
    if (entry == record.properties.end()) return std::nullopt;
    PropertyDescriptor descriptor;
    const auto& property = entry->second;
    if (property.accessor) { descriptor.getter = property.getter; descriptor.setter = property.setter; }
    else { descriptor.value = property.value; descriptor.writable = property.writable; }
    descriptor.enumerable = property.enumerable; descriptor.configurable = property.configurable;
    return descriptor;
}
JSValue JSValue::get(const std::string& key) const { return get(key, *this); }
JSValue JSValue::get(const std::string& rawKey, const JSValue& receiver) const {
    const auto key = normalizeString(rawKey);
    const JSValue self = *this, rootedReceiver = receiver;
    if (self.isUndefined() || self.isNull()) throw JSTypeError("Property access on null/undefined");
    if (self.type() == Type::String) {
        const auto units = utf16(self.string());
        if (key == "length") return units.size();
        if (const auto index = arrayIndex(key); index && *index < units.size()) return encodeUnit(units[*index]);
    }
    if (!self.isObject()) return {};
    const auto descriptor = self.getOwnPropertyDescriptor(key);
    if (descriptor) {
        if (descriptor->value) return *descriptor->value;
        return descriptor->getter->isUndefined() ? JSValue{} : descriptor->getter->call(rootedReceiver);
    }
    const auto prototype = self.getPrototype();
    return prototype.isNull() ? JSValue{} : prototype.get(key, rootedReceiver);
}
bool JSValue::hasOwn(const std::string& rawKey) const {
    const auto key = normalizeString(rawKey);
    if (isUndefined() || isNull()) throw JSTypeError("Property access on null/undefined");
    if (type() == Type::String) {
        const auto units = utf16(string()); const auto index = arrayIndex(key);
        return key == "length" || (index && *index < units.size());
    }
    return isObject() && getOwnPropertyDescriptor(key).has_value();
}
bool JSValue::has(const std::string& rawKey) const {
    const auto key = normalizeString(rawKey);
    const JSValue self = *this;
    if (!self.isObject()) throw JSTypeError("Right operand of 'in' must be an object");
    if (self.hasOwn(key)) return true;
    const auto prototype = self.getPrototype(); return !prototype.isNull() && prototype.has(key);
}
bool JSValue::reflectSet(const std::string& rawKey, const JSValue& value, const JSValue& receiver) const {
    const auto key = normalizeString(rawKey);
    const JSValue self = *this, rootedValue = value, rootedReceiver = receiver;
    if (!self.isObject()) throw JSTypeError("Expected runtime object");
    self.heap_->validateEdge(rootedValue); self.heap_->validateEdge(rootedReceiver);
    const auto hooks = self.heap_->record(self).hostHooks;
    if (hooks && hooks->reflectSet) {
        const auto handled = hooks->reflectSet(self, key, rootedValue, rootedReceiver);
        if (handled) return *handled;
    }
    auto descriptor = self.getOwnPropertyDescriptor(key);
    if (!descriptor) {
        const auto prototype = self.getPrototype();
        if (!prototype.isNull()) return prototype.reflectSet(key, rootedValue, rootedReceiver);
        descriptor = dataDescriptor(JSValue{});
    }
    if (descriptor->value) {
        if (!*descriptor->writable || !rootedReceiver.isObject()) return false;
        const auto existing = rootedReceiver.getOwnPropertyDescriptor(key);
        if (existing && (!existing->value || !*existing->writable)) return false;
        PropertyDescriptor update; update.value = rootedValue;
        auto target = rootedReceiver;
        return target.defineOwnProperty(key, existing ? update : dataDescriptor(rootedValue));
    }
    if (descriptor->setter->isUndefined()) return false;
    descriptor->setter->call(rootedReceiver, {rootedValue}); return true;
}
void JSValue::set(const std::string& key, const JSValue& value) {
    if (!reflectSet(key, value, *this)) throw JSTypeError("Cannot assign property '" + key + "'");
}
bool JSValue::defineOwnProperty(const std::string& rawKey, const PropertyDescriptor& input) {
    const auto key = normalizeString(rawKey);
    JSValue self = *this;
    const PropertyDescriptor descriptor = input;
    if (!self.isObject()) throw JSTypeError("Expected runtime object");
    const bool accessor = descriptor.getter.has_value() || descriptor.setter.has_value();
    const bool data = descriptor.value.has_value() || descriptor.writable.has_value();
    if (accessor && data) throw JSTypeError("Invalid property descriptor: accessor and value/writable");
    for (const auto* value : {&descriptor.value, &descriptor.getter, &descriptor.setter})
        if (*value) self.heap_->validateEdge(**value);
    for (const auto* method : {&descriptor.getter, &descriptor.setter})
        if (*method && !(**method).isUndefined() && !(**method).isCallable())
            throw JSTypeError("Getter/setter must be callable or undefined");
    const auto hooks = self.heap_->record(self).hostHooks;
    if (hooks && hooks->defineOwnProperty) {
        const auto handled = hooks->defineOwnProperty(self, key, descriptor);
        if (handled) return *handled;
    }
    auto& record = self.heap_->record(self);
    if (self.type() == Type::Array && key == "length") {
        if (!descriptor.value) {
            if (accessor || descriptor.configurable.value_or(false) || descriptor.enumerable.value_or(false) ||
                (!record.lengthWritable && descriptor.writable.value_or(false))) return false;
            if (descriptor.writable) record.lengthWritable = *descriptor.writable;
            return true;
        }
        // ArraySetLength calls ToUint32 and ToNumber separately before reading
        // the existing length descriptor. Coercion may mutate the array.
        const auto first = descriptor.value->toNumber();
        double reduced = !std::isfinite(first) || first == 0 ? 0 : std::fmod(std::trunc(first), 4294967296.0);
        if (reduced < 0) reduced += 4294967296.0;
        const auto length = static_cast<std::uint32_t>(reduced);
        const auto second = descriptor.value->toNumber();
        if (static_cast<double>(length) != second) throw JSRangeError("Invalid array length");
        if (descriptor.configurable.value_or(false) || descriptor.enumerable.value_or(false) ||
            (!record.lengthWritable && descriptor.writable.value_or(false))) return false;
        if (!record.lengthWritable && length != record.length) return false;
        const bool makeReadonly = descriptor.writable && !*descriptor.writable;
        if (length >= record.length) {
            record.length = length;
            if (makeReadonly) record.lengthWritable = false;
            return true;
        }
        if (!record.lengthWritable) return false;
        record.length = length;
        auto names = self.ownPropertyNames();
        for (auto name = names.rbegin(); name != names.rend(); ++name) {
            const auto index = arrayIndex(*name);
            if (!index || *index < length) continue;
            if (!self.erase(*name)) {
                record.length = *index + 1;
                if (makeReadonly) record.lengthWritable = false;
                return false;
            }
        }
        if (makeReadonly) record.lengthWritable = false;
        return true;
    }
    const auto entry = record.properties.find(key);
    const bool present = entry != record.properties.end();
    const auto index = self.type() == Type::Array ? arrayIndex(key) : std::nullopt;
    if (index && *index >= record.length && !record.lengthWritable) return false;
    if (!present && !record.extensible) return false;
    if (present) {
        const auto& old = entry->second;
        if (!old.configurable) {
            if (descriptor.configurable.value_or(false) ||
                (descriptor.enumerable && *descriptor.enumerable != old.enumerable)) return false;
            if ((accessor || data) && accessor != old.accessor) return false;
            if (old.accessor) {
                if ((descriptor.getter && !descriptor.getter->sameValue(old.getter)) ||
                    (descriptor.setter && !descriptor.setter->sameValue(old.setter))) return false;
            } else if (!old.writable && (descriptor.writable.value_or(false) ||
                       (descriptor.value && !descriptor.value->sameValue(old.value)))) return false;
        }
    }
    auto& property = record.properties[key];
    if ((!present && accessor) || (present && (accessor || data) && accessor != property.accessor)) {
        property.value = JSValue{}; property.getter = JSValue{}; property.setter = JSValue{};
        property.writable = false; property.accessor = accessor;
    }
    if (descriptor.value) property.value = descriptor.value->graphEdge();
    if (descriptor.writable) property.writable = *descriptor.writable;
    if (descriptor.getter) property.getter = descriptor.getter->graphEdge();
    if (descriptor.setter) property.setter = descriptor.setter->graphEdge();
    if (descriptor.enumerable) property.enumerable = *descriptor.enumerable;
    if (descriptor.configurable) property.configurable = *descriptor.configurable;
    if (index && *index >= record.length) record.length = *index + 1;
    return true;
}
void JSValue::defineProperty(const std::string& key, const PropertyDescriptor& descriptor) {
    if (!defineOwnProperty(key, descriptor)) throw JSTypeError("Cannot redefine property '" + key + "'");
}
bool JSValue::erase(const std::string& rawKey) {
    const auto key = normalizeString(rawKey);
    if (!isObject()) throw JSTypeError("Expected runtime object");
    const JSValue self = *this;
    const auto hooks = heap_->record(self).hostHooks;
    if (hooks && hooks->erase) {
        const auto handled = hooks->erase(self, key);
        if (handled) return *handled;
    }
    if (self.type() == Type::Array && key == "length") return false;
    auto& properties = self.heap_->record(self).properties; const auto entry = properties.find(key);
    if (entry != properties.end() && !entry->second.configurable) return false;
    properties.erase(key); return true;
}
std::vector<std::string> JSValue::keys() const {
    if (!isObject()) throw JSTypeError("Expected runtime object");
    std::vector<std::string> result;
    const JSValue self = *this;
    for (const auto& key : self.ownPropertyNames()) {
        const auto descriptor = self.getOwnPropertyDescriptor(key);
        if (descriptor && descriptor->enumerable.value_or(false)) result.push_back(key);
    }
    return result;
}
std::vector<std::string> JSValue::ownPropertyNames() const {
    if (!isObject()) throw JSTypeError("Expected runtime object");
    const JSValue self = *this;
    const auto hooks = heap_->record(self).hostHooks;
    std::vector<std::string> nativeKeys;
    if (hooks && hooks->ownPropertyNames) nativeKeys = hooks->ownPropertyNames(self);
    std::vector<std::string> result;
    bool inserted = self.type() != Type::Array;
    for (const auto& [key, value] : self.heap_->record(self).properties) {
        (void) value;
        if (!inserted && !arrayIndex(key)) { result.emplace_back("length"); inserted = true; }
        result.push_back(key);
    }
    if (!inserted) result.emplace_back("length");
    if (!nativeKeys.empty()) {
        // OrderedMap implements JS ordering: numeric indices first, then the
        // original insertion order of ordinary string keys.
        OrderedMap<bool> merged;
        for (const auto& key : nativeKeys) merged[normalizeString(key)] = true;
        for (const auto& key : result) merged[key] = true;
        result.clear();
        for (const auto& [key, unused] : merged) { (void) unused; result.push_back(key); }
    }
    return result;
}
JSValue JSValue::getPrototype() const {
    if (!isObject()) throw JSTypeError("Expected runtime object");
    return heap_->record(*this).prototype;
}
bool JSValue::setPrototype(const JSValue& prototype) {
    if (!isObject() || (!prototype.isObject() && !prototype.isNull())) throw JSTypeError("Invalid prototype");
    heap_->validateEdge(prototype);
    auto& record = heap_->record(*this);
    if (record.prototype.strictEquals(prototype)) return true;
    if (!record.extensible || record.immutablePrototype) return false;
    for (auto previous = prototype; !previous.isNull(); previous = previous.getPrototype())
        if (previous.strictEquals(*this)) return false;
    record.prototype = prototype.graphEdge(); return true;
}
void JSValue::preventExtensions() { if (!isObject()) throw JSTypeError("Expected runtime object"); heap_->record(*this).extensible = false; }
bool JSValue::isExtensible() const { if (!isObject()) throw JSTypeError("Expected runtime object"); return heap_->record(*this).extensible; }
void JSValue::setPrimitiveConverter(const JSValue& converter) {
    if (!isObject()) throw JSTypeError("Expected runtime object");
    heap_->validateEdge(converter);
    auto& record = heap_->record(*this); record.primitiveConverter = converter.graphEdge(); record.hasPrimitiveConverter = true;
}
std::uint32_t JSValue::length() const {
    if (type() != Type::Array) throw JSTypeError("Expected runtime array");
    const auto hooks = heap_->record(*this).hostHooks;
    if (hooks && hooks->getOwnProperty) {
        const auto value = get("length").toNumber();
        if (!std::isfinite(value) || value < 0 || value > 4294967295.0 || value != std::trunc(value))
            throw JSRangeError("Invalid native array length");
        return static_cast<std::uint32_t>(value);
    }
    return heap_->record(*this).length;
}
JSValue JSValue::fromJSON(const JsonValue& value) {
    return std::visit([](const auto& data) -> JSValue {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, JsonValue::Object>) {
            auto result = object();
            for (const auto& [key, entry] : data) result.set(key, fromJSON(entry));
            return result;
        } else if constexpr (std::is_same_v<T, JsonValue::Array>) {
            auto result = array();
            for (std::size_t i = 0; i < data.size(); ++i) result.set(std::to_string(i), fromJSON(data[i]));
            return result;
        } else return JSValue(data);
    }, value.storage());
}
std::optional<JsonValue> JSValue::toJSON() const {
    std::unordered_set<std::uint64_t> active;
    std::function<std::optional<JsonValue>(JSValue, const std::string&)> encode = [&](JSValue value, const std::string& key) -> std::optional<JsonValue> {
        if (value.isObject()) { const auto method = value.get("toJSON"); if (method.isCallable()) value = method.call(value, {key}); }
        switch (value.type()) {
        case Type::Undefined: case Type::Function: return std::nullopt;
        case Type::Null: return JsonValue(nullptr);
        case Type::Boolean: return JsonValue(std::get<bool>(value.value_));
        case Type::Number: return std::isfinite(value.number()) ? JsonValue(value.number() == 0 ? 0 : value.number()) : JsonValue(nullptr);
        case Type::String: return JsonValue(value.string());
        default: break;
        }
        const auto id = std::get<Reference>(value.value_).id;
        if (!active.insert(id).second) throw JSTypeError("Circular structure in JSON");
        JsonValue result;
        if (value.type() == Type::Array) {
            JsonValue::Array entries;
            const auto length = value.length(); entries.reserve(length);
            for (std::uint32_t i = 0; i < length; ++i) {
                const auto name = std::to_string(i);
                entries.push_back(encode(value.get(name), name).value_or(JsonValue(nullptr)));
            }
            result = std::move(entries);
        } else {
            JsonValue::Object entries;
            for (const auto& name : value.keys())
                if (auto entry = encode(value.get(name), name)) entries[name] = std::move(*entry);
            result = std::move(entries);
        }
        active.erase(id); return result;
    };
    return encode(*this, "");
}
JsonValue JSValue::trace() const {
    std::unordered_map<std::uint64_t, std::uint64_t> identities;
    std::function<JsonValue(JSValue)> encode = [&](JSValue value) -> JsonValue {
        switch (value.type()) {
        case Type::Undefined: return JsonValue::Object{{"type", "undefined"}};
        case Type::Null: return JsonValue::Object{{"type", "null"}};
        case Type::Boolean: return JsonValue::Object{{"type", "boolean"}, {"value", std::get<bool>(value.value_)}};
        case Type::String: return JsonValue::Object{{"type", "string"}, {"value", value.string()}};
        case Type::Number: {
            JsonValue number = value.number();
            if (std::isnan(value.number())) number = "NaN";
            else if (std::isinf(value.number())) number = value.number() > 0 ? "Infinity" : "-Infinity";
            else if (value.number() == 0 && std::signbit(value.number())) number = "-0";
            return JsonValue::Object{{"type", "number"}, {"value", std::move(number)}};
        }
        default: break;
        }
        const auto id = std::get<Reference>(value.value_).id;
        if (const auto existing = identities.find(id); existing != identities.end())
            return JsonValue::Object{{"type", "reference"}, {"id", existing->second}};
        const auto identity = static_cast<std::uint64_t>(identities.size() + 1); identities.emplace(id, identity);
        JsonValue::Array properties;
        for (const auto& key : value.keys()) properties.push_back(JsonValue::Array{key, encode(value.get(key))});
        JsonValue result = JsonValue::Object{{"type", value.type() == Type::Array ? "array" : value.isCallable() ? "function" : "object"},
                                            {"id", identity}, {"properties", std::move(properties)}};
        if (value.type() == Type::Array) result["length"] = value.length();
        return result;
    };
    return encode(*this);
}
std::size_t JSValue::collectGarbage() const { return heap_ ? heap_->collect() : 0; }
std::size_t JSValue::heapObjectCount() const { return heap_ ? heap_->records.size() : 0; }
