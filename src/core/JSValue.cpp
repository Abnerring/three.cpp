#include "core/JSValue.h"
#include "core/JSNumber.h"
#include "core/JSProxy.h"
#include "core/JSRegExp.h"
#include "core/JSJobs.h"
#include "core/JSIterator.h"
#include "core/JSString.h"
#include "core/JSObject.h"
#include "core/NativeArray.h"
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
    if(receiver.isUndefined())return "[object Undefined]";
    if(receiver.isNull())return "[object Null]";
    const auto array=isNativeArray(receiver);
    const auto custom=receiver.get(JSValue::wellKnownSymbol("toStringTag"));
    if(custom.type()==JSValue::Type::String)return "[object "+custom.string()+"]";
    if(array)return "[object Array]";
    if(isNativeStringObject(receiver))return "[object String]";
    if(isNativeNumberObject(receiver))return "[object Number]";
    if(isNativeRegExpObject(receiver))return "[object RegExp]";
    if(receiver.isHostObject("runtime.Error"))return "[object Error]";
    switch(receiver.type()){
    case JSValue::Type::Array:return "[object Array]";
    case JSValue::Type::Function:return "[object Function]";
    case JSValue::Type::String:return "[object String]";
    case JSValue::Type::Boolean:return "[object Boolean]";
    case JSValue::Type::Number:return "[object Number]";
    case JSValue::Type::Symbol:return "[object Symbol]";
    default:return "[object Object]";
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
// String and Number use their real boxed prototypes. Boolean boxing still uses
// the existing primitive conversion adapter until its wrapper intrinsic lands.
JSValue arrayMethodObject(const JSValue& receiver) {
    if (receiver.isNull() || receiver.isUndefined())
        throw JSTypeError("Array operation requires a non-null receiver");
    if (receiver.isObject()) return receiver;
    if (receiver.type() == JSValue::Type::String) return createNativeStringObject(receiver);
    if (receiver.type() == JSValue::Type::Number) return createNativeNumberObject(receiver);
    auto object = JSValue::object();
    object.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args&, const Args& captures) { return captures[0]; }, {receiver}));
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
JSValue arrayShift(const JSValue& receiver, const Args&, const Args&) {
    auto object = arrayMethodObject(receiver);
    const auto length = arrayLikeLength(object);
    if (length == 0) { object.set("length", 0); return {}; }
    const auto first = object.get("0");
    for (std::uint64_t index = 1; index < length; ++index) {
        const auto from = std::to_string(index), to = std::to_string(index - 1);
        if (object.has(from)) {
            const auto value = object.get(from);
            object.set(to, value);
        } else deleteOrThrow(object, to);
    }
    deleteOrThrow(object, std::to_string(length - 1));
    object.set("length", length - 1);
    return first;
}
JSValue arrayResult(JSValue original, std::uint64_t length) {
    JSValue constructor;
    if (isNativeArray(original)) {
        constructor = original.get("constructor");
        // Cross-realm intrinsic replacement is outside the single-realm
        // runtime. All supported constructor/species reads are live.
        if (constructor.isObject()) {
            constructor = constructor.get(JSValue::wellKnownSymbol("species"));
            if (constructor.isNull()) constructor = {};
        }
    }
    if (!constructor.isUndefined()) {
        if (!constructor.isConstructible()) throw JSTypeError("Array species is not a constructor");
        return constructor.construct({JSValue(length)});
    }
    if (length > 0xffffffffULL) throw JSRangeError("Invalid array length");
    return JSValue::array(static_cast<std::uint32_t>(length));
}
JSValue arrayConcat(const JSValue& receiver, const Args& args, const Args&) {
    const auto object = arrayMethodObject(receiver);
    auto result = arrayResult(object, 0);
    std::uint64_t next = 0;
    const auto append = [&](const JSValue& item) {
        // Custom species is handled by arrayResult. The remaining
        // @@isConcatSpreadable hook still needs its own implementation.
        if (isNativeArray(item)) {
            const auto length = arrayLikeLength(item);
            if (length > 9007199254740991ULL - next)
                throw JSTypeError("Array-like length exceeds maximum safe integer");
            for (std::uint64_t index = 0; index < length; ++index, ++next) {
                const auto source = std::to_string(index);
                if (item.has(source)) {
                    const auto value = item.get(source);
                    result.defineProperty(std::to_string(next), dataDescriptor(value));
                }
            }
        } else {
            if (next >= 9007199254740991ULL)
                throw JSTypeError("Array-like length exceeds maximum safe integer");
            result.defineProperty(std::to_string(next++), dataDescriptor(item));
        }
    };
    append(object);
    for (const auto& item : args) append(item);
    result.set("length", next);
    return result;
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
JSValue arrayFilter(const JSValue& receiver, const Args& args, const Args&) {
    const auto object = arrayMethodObject(receiver);
    const auto length = arrayLikeLength(object);
    const auto callback = argument(args, 0), thisArg = argument(args, 1);
    if (!callback.isCallable()) throw JSTypeError("Array.prototype.filter callback is not callable");
    auto result = arrayResult(object, 0);
    std::uint64_t next = 0;
    for (std::uint64_t index = 0; index < length; ++index) {
        const auto key = std::to_string(index);
        if (object.has(key)) {
            const auto value = object.get(key);
            if (callback.call(thisArg, {value, JSValue(index), object}).toBoolean())
                result.defineProperty(std::to_string(next++), dataDescriptor(value));
        }
    }
    return result;
}
JSValue arrayReduce(const JSValue& receiver, const Args& args, const Args&) {
    const auto object=arrayMethodObject(receiver);const auto length=arrayLikeLength(object);
    const auto callback=argument(args,0);if(!callback.isCallable())throw JSTypeError("Array.reduce callback is not callable");
    std::uint64_t index=0;JSValue accumulator;
    if(args.size()>1)accumulator=args[1];
    else {
        bool found=false;
        while(index<length){const auto key=std::to_string(index++);if(object.has(key)){accumulator=object.get(key);found=true;break;}}
        if(!found)throw JSTypeError("Reduce of empty array with no initial value");
    }
    for(;index<length;++index){const auto key=std::to_string(index);if(object.has(key)){const auto value=object.get(key);accumulator=callback.call({}, {accumulator,value,index,object});}}
    return accumulator;
}
JSValue arrayEvery(const JSValue& receiver, const Args& args, const Args&) {
    const auto object=arrayMethodObject(receiver);const auto length=arrayLikeLength(object);
    const auto callback=argument(args,0),thisArg=argument(args,1);if(!callback.isCallable())throw JSTypeError("Array.every callback is not callable");
    for(std::uint64_t index=0;index<length;++index){const auto key=std::to_string(index);if(object.has(key)){const auto value=object.get(key);if(!callback.call(thisArg,{value,index,object}).toBoolean())return false;}}
    return true;
}
JSValue arrayFill(const JSValue& receiver, const Args& args, const Args&) {
    auto object=arrayMethodObject(receiver);const auto length=arrayLikeLength(object);
    const auto relative=integerOrInfinity(argument(args,1));
    auto from=relative<0?std::max(double(length)+relative,0.0):std::min(relative,double(length));
    const auto end=argument(args,2);const auto limit=end.isUndefined()?double(length):integerOrInfinity(end);
    const auto to=limit<0?std::max(double(length)+limit,0.0):std::min(limit,double(length));
    for(;from<to;++from)object.set(JSValue(from).toString(),argument(args,0));return object;
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
JSValue arrayIncludes(const JSValue& receiver, const Args& args, const Args&) {
    const auto object = arrayMethodObject(receiver);
    const auto length = arrayLikeLength(object);
    // The zero-length exit precedes conversion of fromIndex, including an
    // observable or throwing Symbol.toPrimitive callback.
    if (length == 0) return false;
    const auto start = integerOrInfinity(argument(args, 1));
    if (start >= double(length)) return false;
    const auto search = argument(args, 0);
    const auto first = start >= 0 ? start : std::max(double(length) + start, 0.0);
    for (auto i = static_cast<std::uint64_t>(first); i < length; ++i) {
        // Includes uses Get, not HasProperty: a hole is the value undefined.
        const auto value = object.get(std::to_string(i));
        if (value.strictEquals(search) ||
            (value.type() == JSValue::Type::Number && search.type() == JSValue::Type::Number &&
             std::isnan(value.toNumber()) && std::isnan(search.toNumber()))) return true;
    }
    return false;
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
    const auto key = argument(args, 0).toPropertyKey(); return receiver.hasOwn(key);
}
JSValue objectEnumerable(const JSValue& receiver, const Args& args, const Args&) {
    const auto key = argument(args, 0).toPropertyKey(); const auto descriptor = receiver.getOwnPropertyDescriptor(key);
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
JSValue arrayIsArray(const JSValue&, const Args& args, const Args&) {
    return isNativeArray(argument(args, 0));
}
JSValue arrayFrom(const JSValue& constructor, const Args& args, const Args&) {
    const auto items=argument(args,0), mapper=argument(args,1), thisArg=argument(args,2);
    const bool mapping=!mapper.isUndefined();
    if(mapping&&!mapper.isCallable())throw JSTypeError("Array.from mapper must be callable");
    const auto iteratorMethod=items.get(JSValue::wellKnownSymbol("iterator"));
    if(!iteratorMethod.isUndefined()&&!iteratorMethod.isNull()) {
        if(!iteratorMethod.isCallable())throw JSTypeError("Iterator method is not callable");
        auto result=constructor.isConstructible()?constructor.construct():JSValue::array();
        const auto iterator=iteratorMethod.call(items);
        if(!iterator.isObject())throw JSTypeError("Iterator method result is not an object");
        JSIteratorRecord record{iterator,iterator.get("next"),false};
        std::uint64_t k=0;
        while(true) {
            if(k>=9007199254740991ULL) {
                try{throw JSTypeError("Array.from length exceeds maximum safe integer");}
                catch(...){closeJSIterator(record,std::current_exception());}
            }
            // Abrupt IteratorStepValue already completes the iterator record.
            const auto next=stepJSIterator(record);
            if(!next){result.set("length",double(k));return result;}
            try{
                const auto value=mapping?mapper.call(thisArg,{*next,double(k)}):*next;
                JSPropertyDescriptor descriptor;descriptor.value=value;
                descriptor.writable=descriptor.enumerable=descriptor.configurable=true;
                result.defineProperty(std::to_string(k),descriptor);
            }catch(...){closeJSIterator(record,std::current_exception());}
            ++k;
        }
    }
    const auto source=arrayMethodObject(items);const auto length=arrayLikeLength(source);
    auto result=constructor.isConstructible()?constructor.construct({double(length)}):arrayConstructor({}, {double(length)}, {});
    for(std::uint64_t k=0;k<length;++k) {
        const auto item=source.get(std::to_string(k));
        const auto value=mapping?mapper.call(thisArg,{item,double(k)}):item;
        JSPropertyDescriptor descriptor;descriptor.value=value;
        descriptor.writable=descriptor.enumerable=descriptor.configurable=true;
        result.defineProperty(std::to_string(k),descriptor);
    }
    result.set("length",double(length));return result;
}
JSValue objectLocaleString(const JSValue& receiver, const Args&, const Args&) {
    const auto method = receiver.get("toString"); return method.call(receiver);
}
JSValue objectDefineAccessor(const JSValue& receiver, const Args& args, const Args& captures) {
    auto object = arrayObject(receiver); const auto callback = argument(args, 1);
    if (!callback.isCallable()) throw JSTypeError("Accessor must be callable");
    const auto key = argument(args, 0).toPropertyKey(); JSPropertyDescriptor descriptor;
    if (captures[0].toBoolean()) descriptor.getter = callback; else descriptor.setter = callback;
    descriptor.enumerable = true; descriptor.configurable = true; object.defineProperty(key, descriptor); return {};
}
JSValue objectLookupAccessor(const JSValue& receiver, const Args& args, const Args& captures) {
    auto object = arrayObject(receiver); const auto key = argument(args, 0).toPropertyKey();
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

    struct KeyHash {
        std::size_t operator()(const JSValue& key) const {
            std::size_t result=static_cast<std::size_t>(key.type())*0x9e3779b9u;
            switch(key.type()) {
                case JSValue::Type::Boolean: return result^std::hash<bool>{}(key.toBoolean());
                case JSValue::Type::Number: return result^(std::isnan(key.number())?std::size_t{0x7ff80000u}:std::hash<double>{}(key.number()==0?0:key.number()));
                case JSValue::Type::String: return result^std::hash<std::string>{}(key.string());
                case JSValue::Type::Object:case JSValue::Type::Array:case JSValue::Type::Function:case JSValue::Type::Symbol:
                    return result^std::hash<std::uint64_t>{}(std::get<JSValue::Reference>(key.value_).id);
                default:return result;
            }
        }
    };
    struct KeyEqual {
        bool operator()(const JSValue& left,const JSValue& right) const {
            return left.strictEquals(right)||(left.type()==JSValue::Type::Number&&right.type()==JSValue::Type::Number&&std::isnan(left.number())&&std::isnan(right.number()));
        }
    };
    struct Cursor { std::size_t index=0;bool finished=false; };
    struct CollectionData {
        struct Entry {JSValue key,value;};
        unsigned brand;
        std::vector<std::optional<Entry>> entries;
        std::unordered_map<JSValue,std::size_t,KeyHash,KeyEqual> index;
        std::vector<std::weak_ptr<Cursor>> cursors;
        explicit CollectionData(unsigned type):brand(type) {}
        std::shared_ptr<Cursor> cursor() {
            std::erase_if(cursors,[](const auto& value){const auto item=value.lock();return !item||item->finished;});
            auto value=std::make_shared<Cursor>();cursors.push_back(value);return value;
        }
        void compact(bool force=false) {
            if(!force && (entries.size()<64 || entries.size()-index.size()<=index.size()))return;
            // A cursor denotes the boundary before its next entry. Relocating
            // that boundary preserves unseen insertions after delete/clear.
            std::vector<std::size_t> prefix(entries.size()+1);
            for(std::size_t i=0;i<entries.size();++i)prefix[i+1]=prefix[i]+(entries[i]?1:0);
            std::erase_if(cursors,[&](const auto& weak){
                const auto value=weak.lock();if(!value||value->finished)return true;
                value->index=prefix[std::min(value->index,entries.size())];return false;
            });
            std::vector<std::optional<Entry>> live;live.reserve(index.size());
            index.clear();
            for(auto& entry:entries)if(entry) {
                index.emplace(entry->key.graphEdge(),live.size());
                live.push_back(std::move(entry));
            }
            entries.swap(live);
        }
    };
    struct IteratorData {
        unsigned brand,kind;
        JSValue target;
        std::shared_ptr<CollectionData> collection;
        std::shared_ptr<Cursor> cursor;
        std::vector<std::uint16_t> units;
    };

    struct Record {
        enum class ConstructMode { None, Class, Callable, Object, Array, Builtin, CallableBuiltin, Bound };
        OrderedMap<Property> properties;
        OrderedMap<JSValue> hostSlots;
        struct SymbolProperty { JSValue key; Property property; };
        std::vector<SymbolProperty> symbolProperties;
        std::optional<std::string> symbolDescription;
        std::shared_ptr<void> hostPayload;
        std::shared_ptr<const JSHostObjectHooks> hostHooks;
        bool proxy = false, proxyConstructor = false;
        std::string hostTag;
        JSValue::Type hostType = JSValue::Type::Object;
        JSValue prototype = nullptr, primitiveConverter;
        std::vector<JSValue> captures;
        // Ordinary records and empty WeakMaps allocate no conditional table.
        // Keys are non-owning identities. Values are graph edges which become
        // strong only when both this map and its key have been marked.
        std::shared_ptr<CollectionData> collection;
        std::unique_ptr<IteratorData> iterator;
        bool weakMap = false;
        std::unique_ptr<std::unordered_map<std::uint64_t, JSValue>> weakEntries;
        // Weak identities never own a heap or count as graph roots. Callback
        // and held values are ordinary strong edges of a reachable registry.
        bool weakRef = false;
        std::optional<JSValue::Reference> weakTarget;
        struct FinalizationCell {
            std::optional<std::uint64_t> target, token;
            JSValue heldValue;
        };
        struct FinalizationData {
            JSValue callback;
            std::vector<FinalizationCell> cells;
            bool queued = false, running = false;
        };
        std::unique_ptr<FinalizationData> finalization;
        JSValue::NativeFunction callback = nullptr;
        JSValue::NativeBuiltinFunction builtinCallback = nullptr;
        ConstructMode constructMode = ConstructMode::None;
        std::string nativeName;
        std::optional<std::string> functionSourceText;
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
    std::unordered_set<std::uint64_t> keptObjects;
    JSValue objectPrototype, arrayPrototype, functionPrototype, intrinsicObject, intrinsicArray;
    bool initialized = false, collecting = false, initializing = false;
    std::size_t totalRoots = 0, nativeOwnershipRecords = 0, executionDepth = 0;
    struct ExecutionScope {
        JSHeap& heap;
        explicit ExecutionScope(JSHeap& heap) : heap(heap) { ++heap.executionDepth; }
        ~ExecutionScope() { --heap.executionDepth; }
    };
    struct NativeRoot {
        JSValue edge;
        bool owning;
        NativeRoot(JSValue edge, bool owning) : edge(std::move(edge)), owning(owning) {}
        NativeRoot(NativeRoot&&) = default;
        NativeRoot& operator=(NativeRoot&&) = default;
        NativeRoot(const NativeRoot&) = delete;
        NativeRoot& operator=(const NativeRoot&) = delete;
    };
    NativeRoot snapshotNativeRoot(const JSValue& value) {
        validateEdge(value);
        return {value.graphEdge(), bool(value.owner_)};
    }
    bool hasNativeApplicationRoots() {
        // Only described native fields can disguise an owning handle as an
        // internal heap edge. Audit the host identity index, not every ordinary
        // record, and compare exact root counts after all temporary hook roots
        // have gone away. A negative result is rechecked by full collection.
        collecting = true;
        struct Reset { bool& value; ~Reset() { value = false; } } reset{collecting};
        using Owner = std::weak_ptr<void>;
        struct Node {
            std::shared_ptr<void> storage;
            std::map<const JSValue*, NativeRoot> roots;
            std::map<const void*, Owner> edges;
            std::size_t internalOwners = 0;
        };
        struct Snapshot {
            Owner owner;
            std::vector<std::pair<const JSValue*, NativeRoot>> roots;
            std::vector<JSNativeOwnerEdge> edges;
        };
        // Preserve the existing exactly-one pin count and owner-ordered release
        // even while later hooks run or an application root ends this audit.
        std::map<Owner, std::shared_ptr<void>, std::owner_less<Owner>> pins;
        std::vector<Snapshot> snapshots;
        const auto beforeAllocation = nextId;
        std::vector<std::uint64_t> hostIds;
        hostIds.reserve(hosts.size());
        for (const auto& [key, id] : hosts) hostIds.push_back(id);
        for (const auto id : hostIds) {
            const auto found = records.find(id);
            if (found == records.end()) continue;
            const auto& value = *found->second;
            const auto hooks = value.hostHooks;
            if (!value.hostPayload || !hooks || !hooks->nativeOwnership) continue;
            const JSValue self(shared_from_this(), id, value.hostType);
            for (const auto& description : hooks->nativeOwnership(self)) {
                auto storage = description.owner.lock();
                if (storage.use_count() == 0) continue;
                pins.try_emplace(description.owner, std::move(storage));
                Snapshot snapshot{description.owner, {}, {}};
                snapshot.roots.reserve(description.roots.size());
                for (const auto* root : description.roots)
                    if (root) snapshot.roots.emplace_back(root, snapshotNativeRoot(*root));
                snapshot.edges.reserve(description.edges.size());
                for (const auto& link : description.edges)
                    if (!link.owner.expired()) snapshot.edges.push_back(link);
                snapshots.push_back(std::move(snapshot));
            }
        }
        // Every hook has run and each returned field has already been copied.
        // Most native mathematical wrappers describe no JS fields. Avoid
        // constructing their ownership graph when exact JS root accounting
        // already proves that this realm still has an application root.
        using Roots = std::map<const JSValue*, NativeRoot>;
        std::map<Owner, Roots, std::owner_less<Owner>> rootsByOwner;
        for (auto& snapshot : snapshots) {
            if (snapshot.roots.empty()) continue;
            auto& roots = rootsByOwner[snapshot.owner];
            for (auto& [address, root] : snapshot.roots)
                roots.insert_or_assign(address, std::move(root));
        }
        std::unordered_set<const JSValue*> roots;
        std::size_t internalRootCount = 0;
        for (const auto& [owner, fields] : rootsByOwner)
            for (const auto& [address, value] : fields)
                if (roots.insert(address).second && (value.edge.isObject() || value.edge.isSymbol()))
                    if (value.owning) ++internalRootCount;
        if (nextId != beforeAllocation || totalRoots > internalRootCount || !keptObjects.empty()) return true;
        std::map<Owner, Node, std::owner_less<Owner>> native;
        for (const auto& [owner, storage] : pins) native.try_emplace(owner);
        for (auto& snapshot : snapshots) {
            auto& node = native[snapshot.owner];
            for (const auto& link : snapshot.edges) node.edges[link.slot] = link.owner;
        }
        for (auto& [owner, fields] : rootsByOwner) native.at(owner).roots = std::move(fields);
        std::vector<Owner> children;
        for (const auto& [owner, node] : native)
            for (const auto& [slot, child] : node.edges) children.push_back(child);
        for (const auto& child : children) native.try_emplace(child);
        for (const auto id : hostIds) {
            const auto record = records.find(id);
            if (record == records.end() || !record->second->hostPayload) continue;
            const auto node = native.find(Owner(record->second->hostPayload));
            if (node != native.end()) ++node->second.internalOwners;
        }
        std::unordered_set<const void*> slots;
        for (const auto& [owner, node] : native)
            for (const auto& [slot, child] : node.edges)
                if (slots.insert(slot).second) {
                    const auto found = native.find(child);
                    if (found != native.end()) ++found->second.internalOwners;
                }
        std::map<Owner, std::size_t, std::owner_less<Owner>> incoming;
        for (const auto& [owner, node] : native) incoming.emplace(owner, 0);
        for (const auto& [owner, node] : native)
            for (const auto& [slot, child] : node.edges)
                if (const auto found = incoming.find(child); found != incoming.end()) ++found->second;
        std::vector<Owner> leaves;
        for (const auto& [owner, count] : incoming) if (count == 0) leaves.push_back(owner);
        while (!leaves.empty()) {
            const auto owner = leaves.back(); leaves.pop_back();
            for (const auto& [slot, child] : native.at(owner).edges)
                if (const auto found = incoming.find(child); found != incoming.end() && --found->second == 0)
                    leaves.push_back(child);
        }
        // Finish every allocating graph operation while the original
        // owner-ordered map still holds all strong pins. Allocation failure
        // must not split payload release between two partially built maps.
        for (auto& [owner, storage] : pins) native.at(owner).storage = std::move(storage);
        snapshots.clear(); rootsByOwner.clear(); pins.clear(); children.clear();
        for (const auto& [owner, node] : native)
            if (owner.use_count() > static_cast<long>(node.internalOwners + (node.storage.use_count() != 0)) || incoming.at(owner) != 0)
                return true;
        return false;
    }
    std::size_t safepoint() {
        if (collecting || initializing || executionDepth != 0) return 0;
        // Without described native fields every owning handle is an application
        // root. This exact fast path does not inspect shared_ptr::use_count().
        if (nativeOwnershipRecords == 0 && totalRoots != 0) return 0;
        if (nativeOwnershipRecords != 0 && hasNativeApplicationRoots()) return 0;
        return collect(true);
    }
    static std::weak_ptr<JSHeap>& currentHeapStorage() {
        static thread_local std::weak_ptr<JSHeap> heap;
        return heap;
    }
    static std::shared_ptr<JSHeap> current() {
        auto& currentHeap = currentHeapStorage();
        auto heap = currentHeap.lock();
        if (heap) {
            heap->safepoint();
            if (heap->records.empty()) heap.reset();
        }
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
        if ((id % 10000u) == 0u && std::getenv("THREECPP_GC_TRACE") != nullptr) std::fprintf(stderr, "[heap] allocations=%llu records=%zu\n", static_cast<unsigned long long>(id), records.size());
        return JSValue(shared_from_this(), id, type);
    }
    Record& record(const JSValue& value) const { return *records.at(std::get<JSValue::Reference>(value.value_).id); }
    void validateEdge(const JSValue& value) const {
        if ((value.isObject() || value.isSymbol()) && value.heap_ != this)
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
        initializing = true;
        struct InitializationReset { bool& value; ~InitializationReset() { value = false; } } reset{initializing};
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
        intrinsicObject = constructor.graphEdge();
        record(constructor).constructMode = Record::ConstructMode::Object;
        constructor.defineProperty("prototype", dataDescriptor(object, false, false, false));
        object.defineProperty("constructor", dataDescriptor(constructor, true, false, true));
        initializeNativeObjectMethods(constructor);
        JSPropertyDescriptor proto;
        proto.getter = createFunction(objectGetProto, {}, "get __proto__", 0);
        proto.setter = createFunction(objectSetProto, {}, "set __proto__", 1);
        proto.enumerable = false; proto.configurable = true; object.defineProperty("__proto__", proto);
        install(function, "toString", nativeToString, 0);
        install(function, "call", functionCall, 1);
        install(function, "bind", [](const JSValue& receiver, const Args& args, const Args&) -> JSValue {
            return receiver.bind(argument(args, 0), args.empty() ? Args{} : Args(args.begin() + 1, args.end()));
        }, 1);
        install(array, "join", arrayJoin, 1); install(array, "toString", arrayToString, 0);
        install(array, "push", arrayPush, 1); install(array, "pop", arrayPop, 0);
        install(array, "shift", arrayShift, 0);
        install(array, "unshift", arrayUnshift, 1); install(array, "sort", arraySort, 1);
        install(array, "concat", arrayConcat, 1); install(array, "slice", arraySlice, 2); install(array, "indexOf", arrayIndexOf, 1);
        install(array, "includes", arrayIncludes, 1);
        install(array, "splice", arraySplice, 2);
        install(array, "map", arrayMap, 1);
        install(array, "filter", arrayFilter, 1);
        install(array, "reduce", arrayReduce, 1);
        install(array, "every", arrayEvery, 1);
        install(array, "fill", arrayFill, 1);
        auto arrayCtor = createFunction(arrayConstructor, {}, "Array", 1);
        record(arrayCtor).intrinsic = true;
        intrinsicArray = arrayCtor.graphEdge();
        record(arrayCtor).constructMode = Record::ConstructMode::Array;
        arrayCtor.defineProperty("prototype", dataDescriptor(array, false, false, false));
        array.defineProperty("constructor", dataDescriptor(arrayCtor, true, false, true));
        install(arrayCtor,"from",arrayFrom,1);
        install(arrayCtor,"isArray",arrayIsArray,1);
        JSPropertyDescriptor arraySpecies;
        arraySpecies.getter = createFunction([](const JSValue& receiver, const Args&, const Args&) {
            return receiver;
        }, {}, "get [Symbol.species]", 0);
        record(*arraySpecies.getter).intrinsic = true;
        arraySpecies.enumerable = false;
        arraySpecies.configurable = true;
        arrayCtor.defineProperty(JSValue::wellKnownSymbol("species"), arraySpecies);
        // Well-known identities are realm intrinsics, never user allocations.
        for(const auto* name:{"iterator","asyncIterator","toStringTag","toPrimitive","species","isConcatSpreadable",
            "hasInstance","match","matchAll","replace","search","split","unscopables","dispose","asyncDispose"})
            static_cast<void>(JSValue::wellKnownSymbol(name));
        initializeNativeIteration(array);
    }
    std::size_t collect(bool retireRealm = false) {
        // Native destructors may read JS fields, retain them externally, or ask
        // for another collection. Keep records readable during finalization and
        // re-mark after each owner; nested collection cannot start a second sweep.
        if (collecting) return 0;
        collecting = true;
        struct Reset { bool& value; ~Reset() { value = false; } } reset{collecting};
        const auto keepHeap = shared_from_this();
        std::vector<std::uint64_t> finalizePayloads;
        bool retirementStarted = false, abortRetirement = false, intrinsicCommit = false, latestApplicationRoots = false;
        using Owner = std::weak_ptr<void>;
        struct NativeNode {
            // A trace hook may replace the last native edge to a described
            // child. Keep its owner alive for this mark; this
            // collector reference is excluded from external-owner counts.
            std::shared_ptr<void> storage;
            std::map<const JSValue*, NativeRoot> roots;
            std::map<const void*, Owner> edges;
            std::size_t internalOwners = 0;
            std::vector<std::uint64_t> hosts;
        };
        using NativeGraph = std::map<Owner, NativeNode, std::owner_less<Owner>>;
        auto readNativeGraph = [&] {
        std::map<Owner, NativeNode, std::owner_less<Owner>> native;
        std::vector<std::uint64_t> recordIds;
        recordIds.reserve(records.size());
        for (const auto& [id, value] : records) recordIds.push_back(id);
        for (const auto id : recordIds) {
            const auto found = records.find(id);
            if (found == records.end()) continue;
            const auto& value = *found->second;
            const auto hooks = value.hostHooks;
            if (value.hostPayload && hooks && hooks->nativeOwnership) {
                const JSValue self(shared_from_this(), id, value.hostType);
                for (const auto& description : hooks->nativeOwnership(self)) {
                    auto storage = description.owner.lock();
                    if (storage.use_count() == 0) continue;
                    auto& node = native[description.owner];
                    if (node.storage.use_count() == 0) node.storage = std::move(storage);
                    for (const auto* root : description.roots)
                        if (root) node.roots.insert_or_assign(root, snapshotNativeRoot(*root));
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
        for (const auto& [id, value] : records) {
            if (value->hostPayload) {
                auto found = native.find(Owner(value->hostPayload));
                if (found != native.end()) {
                    ++found->second.internalOwners;
                    found->second.hosts.push_back(id);
                }
            }
        }
        std::unordered_set<const void*> countedSlots;
        for (const auto& [owner, node] : native)
            for (const auto& [slot, child] : node.edges)
                if (countedSlots.insert(slot).second)
                    if (const auto found = native.find(child); found != native.end())
                        ++found->second.internalOwners;
        return native;
        };
        struct RootIdentity {
            std::uint64_t id = 0;
            bool reference = false, owning = false;
            bool operator==(const RootIdentity&) const = default;
        };
        struct AuditNode {
            std::map<const JSValue*, RootIdentity> roots;
            std::map<const void*, Owner> edges;
            std::vector<std::uint64_t> hosts;
            long strongOwners = 0;
        };
        using NativeAudit = std::map<Owner, AuditNode, std::owner_less<Owner>>;
        auto auditGraph = [&](const NativeGraph& native) {
            NativeAudit audit;
            for (const auto& [owner, node] : native) {
                auto& copy = audit[owner];
                copy.edges = node.edges;
                copy.hosts = node.hosts;
                std::sort(copy.hosts.begin(), copy.hosts.end());
                copy.strongOwners = owner.use_count() - static_cast<long>(node.storage.use_count() != 0);
                for (const auto& [address, root] : node.roots) {
                    auto& identity = copy.roots[address];
                    if (root.edge.isObject() || root.edge.isSymbol()) {
                        identity.reference = true;
                        identity.id = std::get<JSValue::Reference>(root.edge.value_).id;
                        identity.owning = root.owning;
                    }
                }
            }
            return audit;
        };
        auto sameAudit = [&](const NativeAudit& left, const NativeAudit& right) {
            const std::owner_less<Owner> less;
            auto sameOwner = [&](const Owner& a, const Owner& b) { return !less(a,b) && !less(b,a); };
            if (left.size() != right.size()) return false;
            auto a = left.begin(), b = right.begin();
            for (; a != left.end(); ++a, ++b) {
                if (!sameOwner(a->first,b->first) || a->second.roots != b->second.roots ||
                    a->second.hosts != b->second.hosts || a->second.strongOwners != b->second.strongOwners ||
                    a->second.edges.size() != b->second.edges.size()) return false;
                auto ae = a->second.edges.begin(), be = b->second.edges.begin();
                for (; ae != a->second.edges.end(); ++ae, ++be)
                    if (ae->first != be->first || !sameOwner(ae->second,be->second)) return false;
            }
            return true;
        };
        NativeAudit beforeNative;
        auto mark = [&] {
        finalizePayloads.clear();
        auto native = readNativeGraph();
        beforeNative = auditGraph(native);
        std::map<Owner, std::vector<std::uint64_t>, std::owner_less<Owner>> payloadGroups;
        for (const auto& [id, value] : records)
            if (value->hostPayload) payloadGroups[Owner(value->hostPayload)].push_back(id);
        std::unordered_set<const JSValue*> countedRoots;
        std::unordered_map<std::uint64_t, std::size_t> internalRoots;
        for (const auto& [owner, node] : native) {
            for (const auto& [address, value] : node.roots) {
                if (countedRoots.insert(address).second && (value.edge.isObject() || value.edge.isSymbol())) {
                    if (value.owning) ++internalRoots[std::get<JSValue::Reference>(value.edge.value_).id];
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
            if (value->roots > internalRoots[id] || (!retireRealm && value->intrinsic)) pending.push_back(id);
        }
        pending.insert(pending.end(), keptObjects.begin(), keptObjects.end());
        auto edge = [&](const JSValue& value) {
            if (value.isObject() || value.isSymbol()) pending.push_back(std::get<JSValue::Reference>(value.value_).id);
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
            if (owner.use_count() > static_cast<long>(node.internalOwners + (node.storage.use_count() != 0)) || incoming.at(owner) != 0)
                nativePending.push_back(owner);
        }
        const bool hasApplicationRoots = !pending.empty() || !nativePending.empty();
        latestApplicationRoots = hasApplicationRoots;
        if (retireRealm && !retirementStarted && hasApplicationRoots) {
            abortRetirement = true;
            return marked;
        }
        retirementStarted = retirementStarted || retireRealm;
        // Each native finalizer can publish JS handles, native owners, or a new
        // pure C++ cycle. Rebuild the graph above and restore intrinsic roots
        // if retirement has been interrupted by that escape.
        if (retireRealm && (hasApplicationRoots || !intrinsicCommit))
            for (const auto& [id, value] : records)
                if (value->intrinsic) pending.push_back(id);
        while (!pending.empty() || !nativePending.empty()) {
            while (!nativePending.empty()) {
                const auto owner = nativePending.back(); nativePending.pop_back();
                if (!nativeMarked.emplace(owner, true).second) continue;
                const auto found = native.find(owner);
                if (found == native.end()) continue;
                for (const auto host : found->second.hosts) pending.push_back(host);
                for (const auto& [address, root] : found->second.roots) edge(root.edge);
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
                    if (!value.isObject() && !value.isSymbol()) continue;
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
            if(record.collection) for(const auto& entry:record.collection->entries) if(entry) {edge(entry->key);edge(entry->value);}
            if(record.iterator) {
                edge(record.iterator->target);
                if(record.iterator->collection)for(const auto& entry:record.iterator->collection->entries)
                    if(entry){edge(entry->key);edge(entry->value);}
            }
            if (record.finalization) {
                edge(record.finalization->callback);
                for (const auto& cell : record.finalization->cells) edge(cell.heldValue);
            }
            for (const auto& entry : record.symbolProperties) {
                edge(entry.key);edge(entry.property.value);edge(entry.property.getter);edge(entry.property.setter);
            }
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
        const bool gcTrace = std::getenv("THREECPP_GC_TRACE") != nullptr; int gcPasses = 0;
        auto stableMark = [&] {
            for (;;) {
                ++gcPasses;
                if (gcTrace) std::fprintf(stderr, "[gc] stableMark pass %d records=%zu nextId=%llu\n", gcPasses, records.size(), static_cast<unsigned long long>(nextId));
                const auto beforeAllocation = nextId;
                std::unordered_map<std::uint64_t, std::size_t> beforeRoots;
                for (const auto& [id, value] : records) beforeRoots.emplace(id, value->roots);
                auto result = mark();
                if (abortRetirement) return result;
                // A trace/ownership hook can replace an already-existing
                // native edge or graph field without changing JS root counts.
                // Re-read the described live graph before committing a sweep.
                const auto afterNative = auditGraph(readNativeGraph());
                const bool nativeChanged = !sameAudit(beforeNative, afterNative);
                // Hooks can allocate/re-hash storage or publish a new owning
                // handle. Temporary hook roots have been released here. Rebuild
                // the complete native graph before retiring any payload/sweeping.
                bool changed = nativeChanged || nextId != beforeAllocation || beforeRoots.size() != records.size();
                if (!changed)
                    for (const auto& [id, value] : records)
                        if (const auto found = beforeRoots.find(id); found == beforeRoots.end() || found->second != value->roots) {
                            changed = true; break;
                        }
                if (!changed) return result;
            }
        };
        auto marked = stableMark();
        if (abortRetirement) return 0;
        for (;;) {
            if (finalizePayloads.empty()) {
                if (retireRealm && !latestApplicationRoots && !intrinsicCommit) {
                    // Keep the complete intrinsic graph while unrelated native
                    // finalizers still have a chance to publish application
                    // roots. Only after these owners are gone can realm exit
                    // commit to finalizing intrinsic-owned payloads.
                    intrinsicCommit = true;
                    marked = stableMark();
                }
                if (finalizePayloads.empty()) break;
            }
            std::vector<std::shared_ptr<void>> payloads;
            for (const auto id : finalizePayloads) {
                auto& record = *records.at(id);
                payloads.push_back(std::move(record.hostPayload));
                if (record.hostHooks && record.hostHooks->nativeOwnership) --nativeOwnershipRecords;
                record.hostHooks.reset();
                for (auto entry = hosts.begin(); entry != hosts.end();) {
                    if (entry->second == id) entry = hosts.erase(entry); else ++entry;
                }
            }
            // All alias identity entries are gone before the final shared_ptr
            // is released. An allocation in a destructor cannot reuse a retired
            // wrapper if its native address is reused.
            payloads.clear();
            marked = stableMark();
        }
        // Finalizers above can resurrect ordinary JS records or change weak
        // entries. Prune only after the final re-mark, before detached records
        // are destroyed, so no stale key can later match an unrelated record.
        std::vector<std::uint64_t> cleanupRegistries;
        for (const auto& [id, record] : records) {
            if (record->weakEntries && marked.contains(id))
                std::erase_if(*record->weakEntries, [&](const auto& entry) { return !marked.contains(entry.first); });
            if (!marked.contains(id)) continue;
            if (record->weakRef && record->weakTarget && !marked.contains(record->weakTarget->id))
                record->weakTarget.reset();
            if (record->finalization) {
                auto& data = *record->finalization;
                bool ready = false;
                for (auto& cell : data.cells) {
                    if (cell.target && !marked.contains(*cell.target)) cell.target.reset();
                    if (cell.token && !marked.contains(*cell.token)) cell.token.reset();
                    ready = ready || !cell.target;
                }
                if (ready && !data.queued && !data.running) {
                    data.queued = true; cleanupRegistries.push_back(id);
                }
            }
        }
        std::vector<std::unique_ptr<Record>> garbage;
        for (auto entry = records.begin(); entry != records.end();) {
            if (!marked.contains(entry->first)) {
                if (entry->second->hostHooks && entry->second->hostHooks->nativeOwnership) --nativeOwnershipRecords;
                totalRoots -= entry->second->roots;
                garbage.push_back(std::move(entry->second));
                entry = records.erase(entry);
            }
            else ++entry;
        }
        for (auto entry = hosts.begin(); entry != hosts.end();) {
            if (!records.contains(entry->second)) entry = hosts.erase(entry);
            else ++entry;
        }
        // Enqueue only after the final native re-mark and sweep. The host queue
        // is weak: dropping the registry before its task cancels the cleanup.
        for (const auto id : cleanupRegistries)
            queueFinalizationRegistryCleanup(JSWeakValue(JSValue(keepHeap, id, JSValue::Type::Object)));
        return garbage.size();
    }
};

void JSValue::retainForHeapLifetime() const {
    static_cast<void>(JSWeakValue(*this));
    if (!isObject()) throw JSTypeError("A runtime module root must be an object");
    heap_->record(*this).intrinsic = true;
}


JSValue JSValue::createCollectionData(const JSValue& prototype,unsigned brand) {
    static_cast<void>(JSWeakValue(prototype));
    if(brand!=1&&brand!=2)throw std::logic_error("Invalid collection brand");
    if(!prototype.isObject()&&!prototype.isNull())throw JSTypeError("Collection prototype must be object or null");
    const auto heap=JSHeap::current();heap->validateEdge(prototype);
    auto result=heap->allocate(Type::Object);auto& record=heap->record(result);
    record.collection=std::make_shared<JSHeap::CollectionData>(brand);record.prototype=prototype.graphEdge();return result;
}
bool JSValue::hasCollectionData(unsigned brand) const {
    static_cast<void>(JSWeakValue(*this));
    return isObject()&&heap_->record(*this).collection&&heap_->record(*this).collection->brand==brand;
}
std::size_t JSValue::collectionSize(unsigned brand) const {
    if(!hasCollectionData(brand))throw JSTypeError("Collection receiver has incompatible brand");
    return heap_->record(*this).collection->index.size();
}
bool JSValue::collectionHas(const JSValue& key,unsigned brand) const {
    if(!hasCollectionData(brand))throw JSTypeError("Collection receiver has incompatible brand");
    static_cast<void>(JSWeakValue(key));heap_->validateEdge(key);
    return heap_->record(*this).collection->index.contains(key);
}
JSValue JSValue::collectionGet(const JSValue& key,unsigned brand) const {
    if(!hasCollectionData(brand))throw JSTypeError("Collection receiver has incompatible brand");
    static_cast<void>(JSWeakValue(key));heap_->validateEdge(key);
    const auto& data=*heap_->record(*this).collection;const auto found=data.index.find(key);
    return found==data.index.end()?JSValue{}:JSValue(data.entries[found->second]->value);
}
void JSValue::collectionSet(const JSValue& key,const JSValue& value,unsigned brand) {
    if(!hasCollectionData(brand))throw JSTypeError("Collection receiver has incompatible brand");
    static_cast<void>(JSWeakValue(key));static_cast<void>(JSWeakValue(value));
    heap_->validateEdge(key);heap_->validateEdge(value);
    auto& data=*heap_->record(*this).collection;
    if(const auto found=data.index.find(key);found!=data.index.end()) {
        if(brand==1)data.entries[found->second]->value=value.graphEdge();
        return;
    }
    const auto normalized=key.type()==Type::Number&&key.number()==0?JSValue(0):key;
    data.entries.push_back(JSHeap::CollectionData::Entry{normalized.graphEdge(),value.graphEdge()});
    try { data.index.emplace(normalized.graphEdge(),data.entries.size()-1); }
    catch(...) {data.entries.pop_back();throw;}
}
bool JSValue::collectionErase(const JSValue& key,unsigned brand) {
    if(!hasCollectionData(brand))throw JSTypeError("Collection receiver has incompatible brand");
    static_cast<void>(JSWeakValue(key));heap_->validateEdge(key);
    auto& data=*heap_->record(*this).collection;const auto found=data.index.find(key);
    if(found==data.index.end())return false;
    data.entries[found->second].reset();data.index.erase(found);data.compact();return true;
}
void JSValue::collectionClear(unsigned brand) {
    if(!hasCollectionData(brand))throw JSTypeError("Collection receiver has incompatible brand");
    auto& data=*heap_->record(*this).collection;
    data.index.clear();for(auto& entry:data.entries)entry.reset();data.compact(true);
}
void JSValue::collectionForEach(const JSValue& callback,const JSValue& thisArg,unsigned brand) const {
    if(!hasCollectionData(brand))throw JSTypeError("Collection receiver has incompatible brand");
    static_cast<void>(JSWeakValue(callback));static_cast<void>(JSWeakValue(thisArg));
    heap_->validateEdge(callback);heap_->validateEdge(thisArg);
    if(!callback.isCallable())throw JSTypeError("Collection forEach callback is not callable");
    const auto self=*this,rootedCallback=callback,rootedThis=thisArg;
    auto& data=*heap_->record(self).collection;const auto cursor=data.cursor();
    while(cursor->index<data.entries.size()) {
        const auto index=cursor->index++;
        if(!data.entries[index])continue;
        const auto key=JSValue(data.entries[index]->key);
        const auto value=brand==1?JSValue(data.entries[index]->value):key;
        rootedCallback.call(rootedThis,{value,key,self});
    }
    cursor->finished=true;
}
JSValue JSValue::createIteratorData(const JSValue& prototype,const JSValue& receiver,unsigned brand,unsigned kind) {
    static_cast<void>(JSWeakValue(prototype));static_cast<void>(JSWeakValue(receiver));
    const auto heap=JSHeap::current();heap->validateEdge(prototype);heap->validateEdge(receiver);
    JSValue target=receiver;
    if(brand<=2) {
        if(!receiver.hasCollectionData(brand))throw JSTypeError("Iterator receiver has incompatible collection brand");
    } else if(brand==3)target=arrayMethodObject(receiver);
    else if(brand==4) {
        if(receiver.isNull()||receiver.isUndefined())throw JSTypeError("String iterator requires a non-null receiver");
        target=receiver.toString();
    } else throw std::logic_error("Invalid iterator brand");
    auto result=heap->allocate(Type::Object);auto& record=heap->record(result);
    record.prototype=prototype.graphEdge();record.iterator=std::make_unique<JSHeap::IteratorData>();
    auto& iterator=*record.iterator;iterator.brand=brand;iterator.kind=kind;
    if(brand<=2)iterator.collection=heap->record(target).collection;
    else iterator.target=target.graphEdge();
    iterator.cursor=brand<=2?iterator.collection->cursor():std::make_shared<JSHeap::Cursor>();
    if(brand==4)iterator.units=utf16(target.string());
    return result;
}
JSValue JSValue::iteratorNext(unsigned brand) const {
    static_cast<void>(JSWeakValue(*this));
    if(!isObject()||!heap_->record(*this).iterator||heap_->record(*this).iterator->brand!=brand)
        throw JSTypeError("Iterator next receiver has incompatible brand");
    const auto self=*this;
    auto& iterator=*heap_->record(self).iterator;const auto cursor=iterator.cursor;
    auto result=[](const JSValue& value,bool done) {
        auto object=JSValue::object();object.defineProperty("value",dataDescriptor(value,true,true,true));
        object.defineProperty("done",dataDescriptor(done,true,true,true));return object;
    };
    auto finish=[&] {
        cursor->finished=true;iterator.target={};iterator.collection.reset();iterator.units.clear();return result({},true);
    };
    auto pair=[](const JSValue& key,const JSValue& value) {
        auto array=JSValue::array(2);array.defineProperty("0",dataDescriptor(key,true,true,true));
        array.defineProperty("1",dataDescriptor(value,true,true,true));return array;
    };
    if(cursor->finished)return result({},true);
    const auto target=JSValue(iterator.target);
    if(brand<=2) {
        auto& data=*iterator.collection;
        while(cursor->index<data.entries.size()) {
            const auto index=cursor->index++;
            if(!data.entries[index])continue;
            const auto key=JSValue(data.entries[index]->key);
            const auto value=brand==1?JSValue(data.entries[index]->value):key;
            return result(iterator.kind==0?key:iterator.kind==1?value:pair(key,value),false);
        }
        return finish();
    }
    if(brand==3) {
        // V8's Array Iterator captures the next index before observing length.
        // A reentrant length getter can advance the same iterator in between.
        const auto index=cursor->index;
        // Typed arrays use their internal array length even when an own length
        // property shadows the prototype getter. Array.prototype.values borrowed
        // onto a typed array follows this same branch.
        const auto length=target.isHostObject("three.AnimationArray")
            ? static_cast<std::uint64_t>(NativeArray_detail::unwrap(target).size()) : arrayLikeLength(target);
        if(index>=length)return finish();
        cursor->index=index+1;
        if(iterator.kind==0)return result(index,false);
        const auto value=target.get(std::to_string(index));
        return result(iterator.kind==1?value:pair(index,value),false);
    }
    if(cursor->index>=iterator.units.size())return finish();
    const auto first=iterator.units[cursor->index++];std::string value=encodeUnit(first);
    if(first>=0xd800&&first<=0xdbff&&cursor->index<iterator.units.size()) {
        const auto second=iterator.units[cursor->index];
        if(second>=0xdc00&&second<=0xdfff) {++cursor->index;value+=encodeUnit(second);}
    }
    return result(value,false);
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
    if (!key.isObject() && !key.isSymbol()) return {};
    static_cast<void>(JSWeakValue(key)); heap_->validateEdge(key);
    const auto& entries = heap_->record(*this).weakEntries;
    if (!entries) return {};
    const auto found = entries->find(std::get<Reference>(key.value_).id);
    return found == entries->end() ? JSValue{} : JSValue(found->second);
}
bool JSValue::weakMapHas(const JSValue& key) const {
    if (!hasWeakMapData()) throw JSTypeError("WeakMap.has receiver lacks WeakMap data");
    if (!key.isObject() && !key.isSymbol()) return false;
    static_cast<void>(JSWeakValue(key)); heap_->validateEdge(key);
    const auto& entries = heap_->record(*this).weakEntries;
    return entries && entries->contains(std::get<Reference>(key.value_).id);
}
bool JSValue::weakMapErase(const JSValue& key) {
    if (!hasWeakMapData()) throw JSTypeError("WeakMap.delete receiver lacks WeakMap data");
    if (!key.isObject() && !key.isSymbol()) return false;
    static_cast<void>(JSWeakValue(key)); heap_->validateEdge(key);
    const auto& entries = heap_->record(*this).weakEntries;
    return entries && entries->erase(std::get<Reference>(key.value_).id) != 0;
}
void JSValue::weakMapSet(const JSValue& key, const JSValue& value) {
    if (!hasWeakMapData()) throw JSTypeError("WeakMap.set receiver lacks WeakMap data");
    // Runtime Symbol factories currently create unregistered symbols only.
    // They have heap identities and use the same conditional edges as objects.
    if (!key.isObject() && !key.isSymbol()) throw JSTypeError("Invalid value used as weak map key");
    static_cast<void>(JSWeakValue(key)); static_cast<void>(JSWeakValue(value));
    heap_->validateEdge(key); heap_->validateEdge(value);
    auto& entries = heap_->record(*this).weakEntries;
    if (!entries) entries = std::make_unique<std::unordered_map<std::uint64_t, JSValue>>();
    entries->insert_or_assign(std::get<Reference>(key.value_).id, value.graphEdge());
}

JSValue JSValue::createWeakRefData(const JSValue& prototype, const JSValue& target) {
    if (!target.isObject()) throw JSTypeError("WeakRef target must be held weakly");
    static_cast<void>(JSWeakValue(target)); static_cast<void>(JSWeakValue(prototype));
    const auto heap = JSHeap::current(); heap->validateEdge(target); heap->validateEdge(prototype);
    if (!prototype.isObject() && !prototype.isNull()) throw JSTypeError("WeakRef prototype must be object or null");
    auto result = heap->allocate(Type::Object);
    auto& record = heap->record(result); record.weakRef = true;
    record.weakTarget = std::get<Reference>(target.value_);
    record.prototype = prototype.graphEdge();
    heap->keptObjects.insert(record.weakTarget->id);
    return result;
}
bool JSValue::hasWeakRefData() const {
    static_cast<void>(JSWeakValue(*this));
    return isObject() && heap_->record(*this).weakRef;
}
JSValue JSValue::weakRefDeref() const {
    if (!hasWeakRefData()) throw JSTypeError("WeakRef.deref receiver lacks WeakRef data");
    const auto target = heap_->record(*this).weakTarget;
    if (!target) return {};
    heap_->keptObjects.insert(target->id);
    return JSValue(heap_->shared_from_this(), target->id, target->type);
}
void JSValue::clearKeptObjects() {
    if (const auto heap = JSHeap::currentHeapStorage().lock()) heap->keptObjects.clear();
}
JSValue JSValue::createFinalizationRegistryData(const JSValue& prototype, const JSValue& callback) {
    if (!callback.isCallable()) throw JSTypeError("FinalizationRegistry cleanup must be callable");
    static_cast<void>(JSWeakValue(callback)); static_cast<void>(JSWeakValue(prototype));
    const auto heap = JSHeap::current(); heap->validateEdge(callback); heap->validateEdge(prototype);
    if (!prototype.isObject() && !prototype.isNull()) throw JSTypeError("FinalizationRegistry prototype must be object or null");
    auto result = heap->allocate(Type::Object);
    auto& record = heap->record(result); record.prototype = prototype.graphEdge();
    record.finalization = std::make_unique<JSHeap::Record::FinalizationData>();
    record.finalization->callback = callback.graphEdge();
    return result;
}
bool JSValue::hasFinalizationRegistryData() const {
    static_cast<void>(JSWeakValue(*this));
    return isObject() && bool(heap_->record(*this).finalization);
}
void JSValue::finalizationRegistryRegister(const JSValue& target, const JSValue& heldValue,
                                         const JSValue& token) {
    if (!hasFinalizationRegistryData()) throw JSTypeError("FinalizationRegistry.register receiver lacks registry data");
    if (!target.isObject()) throw JSTypeError("FinalizationRegistry target must be held weakly");
    if (target.sameValue(heldValue)) throw JSTypeError("FinalizationRegistry target and held value must differ");
    if (!token.isUndefined() && !token.isObject()) throw JSTypeError("FinalizationRegistry token must be held weakly");
    for (const auto* value : {&target, &heldValue, &token}) {
        static_cast<void>(JSWeakValue(*value)); heap_->validateEdge(*value);
    }
    const auto tokenId = token.isObject() ? std::optional<std::uint64_t>(std::get<Reference>(token.value_).id) : std::nullopt;
    heap_->record(*this).finalization->cells.push_back({std::get<Reference>(target.value_).id, tokenId, heldValue.graphEdge()});
}
bool JSValue::finalizationRegistryUnregister(const JSValue& token) {
    if (!hasFinalizationRegistryData()) throw JSTypeError("FinalizationRegistry.unregister receiver lacks registry data");
    if (!token.isObject()) throw JSTypeError("FinalizationRegistry token must be held weakly");
    static_cast<void>(JSWeakValue(token)); heap_->validateEdge(token);
    const auto id = std::get<Reference>(token.value_).id;
    auto& cells = heap_->record(*this).finalization->cells;
    return std::erase_if(cells, [&](const auto& cell) { return cell.token && *cell.token == id; }) != 0;
}
void JSValue::finalizationRegistryCleanup() {
    if (!hasFinalizationRegistryData()) throw JSTypeError("Cleanup receiver lacks registry data");
    auto& data = *heap_->record(*this).finalization;
    if (data.running) return;
    data.queued = false; data.running = true;
    struct Reset { bool& running; ~Reset() { running = false; } } reset{data.running};
    try {
        for (;;) {
            const auto cell = std::find_if(data.cells.begin(), data.cells.end(), [](const auto& item) { return !item.target; });
            if (cell == data.cells.end()) break;
            auto held = JSValue(cell->heldValue);
            data.cells.erase(cell); // Remove before user code; unregister/reentry sees the current list.
            data.callback.call({}, {held});
        }
    } catch (...) {
        if (std::any_of(data.cells.begin(), data.cells.end(), [](const auto& cell) { return !cell.target; })) {
            data.queued = true; queueFinalizationRegistryCleanup(JSWeakValue(*this));
        }
        throw;
    }
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
    return type()==JSValue::Type::Object || type()==JSValue::Type::Array || type()==JSValue::Type::Function;
}
void JSWeakValue::checkThread() const {
    if (std::holds_alternative<JSValue::Reference>(value_) && threadToken_ != runtimeThreadToken())
        throw JSTypeError("Weak runtime references cannot access another thread's heap");
}
bool JSWeakValue::expired() const {
    if (!std::holds_alternative<JSValue::Reference>(value_)) return false;
    checkThread();
    const auto heap = heap_.lock();
    if (heap) heap->safepoint();
    return !heap || !heap->records.contains(std::get<JSValue::Reference>(value_).id);
}
std::optional<JSValue> JSWeakValue::lock() const {
    if (!std::holds_alternative<JSValue::Reference>(value_)) {
        JSValue result;
        result.value_ = value_;
        return result;
    }
    checkThread();
    const auto heap = heap_.lock();
    const auto reference = std::get<JSValue::Reference>(value_);
    if (heap) heap->safepoint();
    if (!heap || !heap->records.contains(reference.id)) return std::nullopt;
    return JSValue(heap, reference.id, reference.type);
}

JSValue::JSValue(std::shared_ptr<JSHeap> heap, std::uint64_t id, Type type)
    : value_(Reference{id, type}), owner_(std::move(heap)), heap_(owner_.get()) { ++heap_->record(*this).roots; ++heap_->totalRoots; }
void JSValue::retain() { if (heap_) { owner_ = heap_->shared_from_this(); ++heap_->record(*this).roots; ++heap_->totalRoots; } }
void JSValue::release() {
    if (owner_) {
        const auto id = std::get<Reference>(value_).id;
        const auto found = heap_->records.find(id);
        // Owned native fields can be destroyed after their unreachable JS graph
        // has been detached from the heap during the current sweep.
        if (found != heap_->records.end()) { --found->second->roots; --heap_->totalRoots; }
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
JSValue JSValue::symbol(const std::optional<std::string>& description) {
    const auto heap=JSHeap::current();auto value=heap->allocate(Type::Symbol);
    if(description)heap->record(value).symbolDescription=normalizeString(*description);
    return value;
}
JSValue JSValue::wellKnownSymbol(const std::string& name) {
    static const std::unordered_set<std::string> names{"iterator","asyncIterator","toStringTag","toPrimitive","species","isConcatSpreadable","hasInstance","match","matchAll","replace","search","split","unscopables","dispose","asyncDispose"};
    if(!names.contains(name))throw JSTypeError("Unknown well-known Symbol");
    static thread_local std::unordered_map<std::string,JSWeakValue> cached;
    if(const auto value=cached[name].lock();value&&value->isSymbol())return *value;
    auto value=symbol("Symbol."+name);value.heap_->record(value).intrinsic=true;
    cached[name]=JSWeakValue(value);return value;
}
std::optional<std::string> JSValue::symbolDescription() const {
    static_cast<void>(JSWeakValue(*this));
    if(!isSymbol())throw JSTypeError("Symbol receiver required");
    return heap_->record(*this).symbolDescription;
}
JSValue JSValue::toPropertyKey() const {
    static_cast<void>(JSWeakValue(*this));
    const auto primitive=toPrimitive(PreferredType::String);
    return primitive.isSymbol()?primitive:JSValue(primitive.toString());
}
JSValue JSValue::intrinsicObjectConstructor() {
    auto heap=JSHeap::current();return JSValue(heap->intrinsicObject);
}
JSValue JSValue::intrinsicArrayConstructor() {
    auto heap=JSHeap::current();return JSValue(heap->intrinsicArray);
}
std::size_t JSValue::nativeSafepoint() {
    const auto heap = JSHeap::currentHeapStorage().lock();
    return heap ? heap->safepoint() : 0;
}
JSValue JSValue::proxy(const JSValue& target, const JSValue& handler) {
    if (!target.isObject() || !handler.isObject()) throw JSTypeError("Proxy target and handler must be objects");
    auto heap = JSHeap::current(); heap->validateEdge(target); heap->validateEdge(handler);
    auto result = heap->allocate(target.isCallable() ? Type::Function : Type::Object);
    auto& record = heap->record(result); record.proxy = true; record.proxyConstructor = target.isConstructible();
    result.setHostSlot("Proxy.target", target); result.setHostSlot("Proxy.handler", handler);
    return result;
}
bool JSValue::isProxy() const { return isObject() && heap_->record(*this).proxy; }
JSValue createNativeProxy(const JSValue& target, const JSValue& handler) { return JSValue::proxy(target, handler); }
bool JSValue::hasActiveHeap() {
    static_cast<void>(nativeSafepoint());
    return !JSHeap::currentHeapStorage().expired();
}
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
    auto& record = heap_->record(*this);
    const bool hadOwnership = record.hostHooks && record.hostHooks->nativeOwnership;
    const bool hasOwnership = hooks && hooks->nativeOwnership;
    if (!hadOwnership && hasOwnership) ++heap_->nativeOwnershipRecords;
    if (hadOwnership && !hasOwnership) --heap_->nativeOwnershipRecords;
    record.hostHooks = std::move(hooks);
}
void JSValue::setHostSlot(const std::string& key, const JSValue& value) {
    if (!isObject() || (!heap_->record(*this).hostPayload && !heap_->record(*this).proxy)) throw JSTypeError("Expected native host");
    heap_->validateEdge(value);
    heap_->record(*this).hostSlots[key] = value.graphEdge();
}
JSValue JSValue::getHostSlot(const std::string& key) const {
    if (!isObject() || (!heap_->record(*this).hostPayload && !heap_->record(*this).proxy)) throw JSTypeError("Expected native host");
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
JSValue JSValue::callableBuiltinConstructor(NativeBuiltinFunction callback,
                                           const std::vector<JSValue>& captures,
                                           const std::string& name, std::size_t arity) {
    if (!callback) throw JSTypeError("Intrinsic constructor requires a callback");
    auto value = constructor(emptyFunction, captures, name, arity);
    auto& record = value.heap_->record(value);
    record.constructMode = JSHeap::Record::ConstructMode::CallableBuiltin;
    record.builtinCallback = callback;
    return value;
}
bool JSValue::isConstructible() const {
    if (isProxy()) return heap_->record(*this).proxyConstructor;
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
    static_cast<void>(JSWeakValue(*this));
    const JSValue self = *this;
    if (!self.isObject()) return self;
    const auto converter=self.get(wellKnownSymbol("toPrimitive"));
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
    case Type::Symbol: throw JSTypeError("Cannot convert a Symbol value to a number");
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
    case Type::Symbol: throw JSTypeError("Cannot convert a Symbol value to a string");
    default: return toPrimitive(PreferredType::String).toString();
    }
}
JSValue JSValue::call() const { return call(JSValue{}); }
JSValue JSValue::call(const JSValue& receiver, const std::vector<JSValue>& args) const {
    static_cast<void>(JSWeakValue(*this));
    static_cast<void>(JSWeakValue(receiver));
    for (const auto& argument : args) static_cast<void>(JSWeakValue(argument));
    if (!isCallable()) throw JSTypeError("Value is not callable");
    if (isProxy()) return JSProxyDetail::call(*this, receiver, args);
    // Root everything before user code detaches properties or runs collection.
    const JSValue function = *this, rootedReceiver = receiver;
    JSHeap::ExecutionScope execution(*heap_);
    const std::vector<JSValue> rootedArgs = args;
    const auto& record = heap_->record(function);
    if (record.constructMode == JSHeap::Record::ConstructMode::Class ||
        record.constructMode == JSHeap::Record::ConstructMode::Builtin)
        throw JSTypeError("Class constructor cannot be invoked without new");
    const std::vector<JSValue> captures = record.captures;
    const auto callback = record.callback;
    if (record.constructMode == JSHeap::Record::ConstructMode::CallableBuiltin)
        return record.builtinCallback(rootedReceiver, JSValue{}, rootedArgs, captures);
    return callback(rootedReceiver, rootedArgs, captures);
}
JSValue JSValue::construct(const std::vector<JSValue>& args) const { return construct(args, *this); }
JSValue JSValue::bind(const JSValue& receiver, const std::vector<JSValue>& args) const {
    static_cast<void>(JSWeakValue(*this));
    if (!isCallable()) throw JSTypeError("Bind must be called on a function");
    const JSValue target = *this;
    const auto prototype = target.getPrototype();
    std::vector<JSValue> captures{target, receiver};
    captures.insert(captures.end(), args.begin(), args.end());
    auto bound = function([](const JSValue&, const Args& args, const Args& captures) -> JSValue {
        Args combined(captures.begin() + 2, captures.end());
        combined.insert(combined.end(), args.begin(), args.end());
        return captures[0].call(captures[1], combined);
    }, captures, "", 0);
    bound.setPrototype(prototype);
    if (target.isConstructible())
        bound.heap_->record(bound).constructMode = JSHeap::Record::ConstructMode::Bound;
    double length = 0;
    if (target.hasOwn("length")) {
        const auto targetLength = target.get("length");
        if (targetLength.type() == Type::Number) {
            const double n = targetLength.number();
            if (n == std::numeric_limits<double>::infinity()) length = n;
            else if (std::isfinite(n)) length = std::max(0.0, std::trunc(n) - double(args.size()));
        }
    }
    JSPropertyDescriptor descriptor;
    descriptor.value = length;
    bound.defineProperty("length", descriptor);
    const auto targetName = target.get("name");
    descriptor.value = "bound " + (targetName.type() == Type::String ? targetName.string() : std::string{});
    bound.defineProperty("name", descriptor);
    return bound;
}
JSValue JSValue::construct(const std::vector<JSValue>& args, const JSValue& newTarget) const {
    if (!isConstructible()) throw JSTypeError("Value is not a constructor");
    if (!newTarget.isConstructible()) throw JSTypeError("newTarget is not a constructor");
    if (isProxy()) return JSProxyDetail::construct(*this, args, newTarget);
    // Prototype access can invoke user code, detach the constructor and its
    // arguments, collect garbage, or reenter construction. Root them first.
    const JSValue function = *this, rootedNewTarget = newTarget;
    JSHeap::ExecutionScope execution(*heap_);
    const std::vector<JSValue> rootedArgs = args;
    heap_->validateEdge(rootedNewTarget);
    for (const auto& argument : rootedArgs) heap_->validateEdge(argument);
    const auto& record = heap_->record(function);
    const auto mode = record.constructMode;
    const auto callback = record.callback;
    const std::vector<JSValue> captures = record.captures;
    if (mode == JSHeap::Record::ConstructMode::Bound) {
        Args combined(captures.begin() + 2, captures.end());
        combined.insert(combined.end(), rootedArgs.begin(), rootedArgs.end());
        return captures[0].construct(combined, rootedNewTarget.sameValue(function) ? captures[0] : rootedNewTarget);
    }
    if (mode == JSHeap::Record::ConstructMode::Object && rootedNewTarget.sameValue(function))
        return callback(JSValue{}, rootedArgs, captures);
    if (mode == JSHeap::Record::ConstructMode::CallableBuiltin) {
        auto result = record.builtinCallback(JSValue{}, rootedNewTarget, rootedArgs, captures);
        if (!result.isObject()) throw JSTypeError("Intrinsic constructor must return an object");
        return result;
    }
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
    static_cast<void>(JSWeakValue(*this));
    if (!isCallable()) throw JSTypeError("Function.prototype.toString requires a function");
    if(isProxy())return "function () { [native code] }";
    const auto& function = heap_->record(*this);
    if (function.functionSourceText) return *function.functionSourceText;
    return "function " + function.nativeName + "() { [native code] }";
}
void JSValue::setFunctionSourceText(const std::string& source) {
    static_cast<void>(JSWeakValue(*this));
    if (!isCallable()) throw JSTypeError("Function source metadata requires a function");
    auto normalized = normalizeString(source);
    heap_->record(*this).functionSourceText = std::move(normalized);
}
std::optional<JSValue::PropertyDescriptor> JSValue::getOwnPropertyDescriptor(const std::string& rawKey) const {
    const auto key = normalizeString(rawKey);
    if (!isObject()) throw JSTypeError("Expected runtime object");
    const JSValue self = *this;
    if (self.isProxy()) return JSProxyDetail::descriptor(self, key);
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
    static_cast<void>(JSWeakValue(*this));
    static_cast<void>(JSWeakValue(receiver));
    const auto key = normalizeString(rawKey);
    const JSValue self = *this, rootedReceiver = receiver;
    if (self.isUndefined() || self.isNull()) throw JSTypeError("Property access on null/undefined");
    if (self.type() == Type::String) {
        const auto units = utf16(self.string());
        if (key == "length") return units.size();
        if (const auto index = arrayIndex(key); index && *index < units.size()) return encodeUnit(units[*index]);
        return nativeStringPrototype().get(key,rootedReceiver);
    }
    if (self.type() == Type::Number) return nativeNumberPrototype().get(key, rootedReceiver);
    if (!self.isObject()) return {};
    if (self.isProxy()) return JSProxyDetail::get(self, key, rootedReceiver);
    const auto descriptor = self.getOwnPropertyDescriptor(key);
    if (descriptor) {
        if (descriptor->value) return *descriptor->value;
        return descriptor->getter->isUndefined() ? JSValue{} : descriptor->getter->call(rootedReceiver);
    }
    if (NativeArray_detail::isNumericProperty(self, key)) return {};
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
    if (self.isProxy()) return JSProxyDetail::has(self, key);
    if (self.hasOwn(key)) return true;
    if (NativeArray_detail::isNumericProperty(self, key)) return false;
    const auto prototype = self.getPrototype(); return !prototype.isNull() && prototype.has(key);
}
bool JSValue::reflectSet(const std::string& rawKey, const JSValue& value, const JSValue& receiver) const {
    const auto key = normalizeString(rawKey);
    const JSValue self = *this, rootedValue = value, rootedReceiver = receiver;
    if (!self.isObject()) throw JSTypeError("Expected runtime object");
    self.heap_->validateEdge(rootedValue); self.heap_->validateEdge(rootedReceiver);
    if (self.isProxy()) return JSProxyDetail::set(self, key, rootedValue, rootedReceiver);
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
    if (self.isProxy()) return JSProxyDetail::define(self, key, descriptor);
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
    if (self.isProxy()) return JSProxyDetail::erase(self, key);
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
    if (self.isProxy()) {
        std::vector<std::string> result;
        for (const auto& key : JSProxyDetail::keys(self)) if (!key.isSymbol()) result.push_back(key.string());
        return result;
    }
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
std::optional<JSValue::PropertyDescriptor> JSValue::getOwnPropertyDescriptor(const char* key) const {
    return getOwnPropertyDescriptor(std::string(key));
}
std::optional<JSValue::PropertyDescriptor> JSValue::getOwnPropertyDescriptor(const JSValue& input) const {
    static_cast<void>(JSWeakValue(*this));
    if(!isObject())throw JSTypeError("Expected runtime object");
    const auto key=input.toPropertyKey();if(!key.isSymbol())return getOwnPropertyDescriptor(key.string());
    static_cast<void>(JSWeakValue(*this));static_cast<void>(JSWeakValue(key));
    if(!isObject())throw JSTypeError("Expected runtime object");heap_->validateEdge(key);
    if(isProxy())return JSProxyDetail::descriptor(*this,key);
    const auto& properties=heap_->record(*this).symbolProperties;
    const auto found=std::find_if(properties.begin(),properties.end(),[&](const auto& entry){return entry.key.sameValue(key);});
    if(found==properties.end())return {};
    const auto& p=found->property;PropertyDescriptor d;
    if(p.accessor){d.getter=p.getter;d.setter=p.setter;}else{d.value=p.value;d.writable=p.writable;}
    d.enumerable=p.enumerable;d.configurable=p.configurable;return d;
}
JSValue JSValue::get(const JSValue& key) const {return get(key,*this);}
JSValue JSValue::get(const JSValue& input,const JSValue& receiver) const {
    static_cast<void>(JSWeakValue(*this));static_cast<void>(JSWeakValue(receiver));
    if(isNull()||isUndefined())throw JSTypeError("Property access on null/undefined");
    const auto key=input.toPropertyKey();if(!key.isSymbol())return get(key.string(),receiver);
    static_cast<void>(JSWeakValue(*this));static_cast<void>(JSWeakValue(key));static_cast<void>(JSWeakValue(receiver));
    const JSValue self=*this,rootedReceiver=receiver;
    if(self.isNull()||self.isUndefined())throw JSTypeError("Property access on null/undefined");
    if(self.type()==Type::String)return nativeStringPrototype().get(key,rootedReceiver);
    if(self.type()==Type::Number)return nativeNumberPrototype().get(key,rootedReceiver);
    if(!self.isObject())return {};
    if(self.isProxy())return JSProxyDetail::get(self,key,rootedReceiver);
    const auto d=self.getOwnPropertyDescriptor(key);
    if(d){if(d->value)return *d->value;return d->getter->isUndefined()?JSValue{}:d->getter->call(rootedReceiver);}
    const auto prototype=self.getPrototype();return prototype.isNull()?JSValue{}:prototype.get(key,rootedReceiver);
}
bool JSValue::hasOwn(const JSValue& input) const {
    static_cast<void>(JSWeakValue(*this));
    if(isNull()||isUndefined())throw JSTypeError("Property access on null/undefined");
    const auto key=input.toPropertyKey();if(!key.isSymbol())return hasOwn(key.string());
    static_cast<void>(JSWeakValue(*this));static_cast<void>(JSWeakValue(key));
    if(isNull()||isUndefined())throw JSTypeError("Property access on null/undefined");
    return isObject()&&getOwnPropertyDescriptor(key).has_value();
}
bool JSValue::has(const JSValue& input) const {
    static_cast<void>(JSWeakValue(*this));
    if(!isObject())throw JSTypeError("Right operand of 'in' must be an object");
    const auto key=input.toPropertyKey();if(!key.isSymbol())return has(key.string());
    static_cast<void>(JSWeakValue(*this));static_cast<void>(JSWeakValue(key));
    if(!isObject())throw JSTypeError("Right operand of 'in' must be an object");
    if(isProxy())return JSProxyDetail::has(*this,key);
    if(hasOwn(key))return true;const auto prototype=getPrototype();return !prototype.isNull()&&prototype.has(key);
}
bool JSValue::reflectSet(const JSValue& input,const JSValue& value,const JSValue& receiver) const {
    for(const auto* v:{this,&value,&receiver})static_cast<void>(JSWeakValue(*v));
    if(!isObject())throw JSTypeError("Expected runtime object");
    const auto key=input.toPropertyKey();if(!key.isSymbol())return reflectSet(key.string(),value,receiver);
    for(const auto* v:{this,&key,&value,&receiver})static_cast<void>(JSWeakValue(*v));
    const JSValue self=*this,rootedValue=value,rootedReceiver=receiver;
    if(!self.isObject())throw JSTypeError("Expected runtime object");
    self.heap_->validateEdge(key);self.heap_->validateEdge(rootedValue);self.heap_->validateEdge(rootedReceiver);
    if(self.isProxy())return JSProxyDetail::set(self,key,rootedValue,rootedReceiver);
    auto d=self.getOwnPropertyDescriptor(key);
    if(!d){const auto prototype=self.getPrototype();if(!prototype.isNull())return prototype.reflectSet(key,rootedValue,rootedReceiver);d=dataDescriptor(JSValue{});}
    if(d->value){
        if(!*d->writable||!rootedReceiver.isObject())return false;
        const auto existing=rootedReceiver.getOwnPropertyDescriptor(key);
        if(existing&&(!existing->value||!*existing->writable))return false;
        PropertyDescriptor update;update.value=rootedValue;auto target=rootedReceiver;
        return target.defineOwnProperty(key,existing?update:dataDescriptor(rootedValue));
    }
    if(d->setter->isUndefined())return false;
    d->setter->call(rootedReceiver,{rootedValue});return true;
}
void JSValue::set(const JSValue& key,const JSValue& value) {
    if(!reflectSet(key,value,*this))throw JSTypeError("Cannot assign property");
}
bool JSValue::defineOwnProperty(const JSValue& input,const PropertyDescriptor& descriptor) {
    static_cast<void>(JSWeakValue(*this));
    if(!isObject())throw JSTypeError("Expected runtime object");
    for(const auto* v:{&descriptor.value,&descriptor.getter,&descriptor.setter})if(*v)static_cast<void>(JSWeakValue(**v));
    const auto key=input.toPropertyKey();if(!key.isSymbol())return defineOwnProperty(key.string(),descriptor);
    static_cast<void>(JSWeakValue(*this));static_cast<void>(JSWeakValue(key));
    if(!isObject())throw JSTypeError("Expected runtime object");heap_->validateEdge(key);
    const bool accessor=descriptor.getter.has_value()||descriptor.setter.has_value();
    const bool data=descriptor.value.has_value()||descriptor.writable.has_value();
    if(accessor&&data)throw JSTypeError("Invalid property descriptor: accessor and value/writable");
    for(const auto* value:{&descriptor.value,&descriptor.getter,&descriptor.setter})if(*value){static_cast<void>(JSWeakValue(**value));heap_->validateEdge(**value);}
    for(const auto* method:{&descriptor.getter,&descriptor.setter})if(*method&&!(**method).isUndefined()&&!(**method).isCallable())throw JSTypeError("Getter/setter must be callable or undefined");
    if(isProxy())return JSProxyDetail::define(*this,key,descriptor);
    auto& record=heap_->record(*this);auto& properties=record.symbolProperties;
    auto found=std::find_if(properties.begin(),properties.end(),[&](const auto& entry){return entry.key.sameValue(key);});
    const bool present=found!=properties.end();if(!present&&!record.extensible)return false;
    if(present){const auto& old=found->property;if(!old.configurable){
        if(descriptor.configurable.value_or(false)||(descriptor.enumerable&&*descriptor.enumerable!=old.enumerable))return false;
        if((accessor||data)&&accessor!=old.accessor)return false;
        if(old.accessor){if((descriptor.getter&&!descriptor.getter->sameValue(old.getter))||(descriptor.setter&&!descriptor.setter->sameValue(old.setter)))return false;}
        else if(!old.writable&&(descriptor.writable.value_or(false)||(descriptor.value&&!descriptor.value->sameValue(old.value))))return false;
    }}
    if(!present){properties.push_back({key.graphEdge(),{}});found=std::prev(properties.end());}
    auto& p=found->property;
    if((!present&&accessor)||(present&&(accessor||data)&&accessor!=p.accessor)){p.value={};p.getter={};p.setter={};p.writable=false;p.accessor=accessor;}
    if(descriptor.value)p.value=descriptor.value->graphEdge();if(descriptor.writable)p.writable=*descriptor.writable;
    if(descriptor.getter)p.getter=descriptor.getter->graphEdge();if(descriptor.setter)p.setter=descriptor.setter->graphEdge();
    if(descriptor.enumerable)p.enumerable=*descriptor.enumerable;if(descriptor.configurable)p.configurable=*descriptor.configurable;
    return true;
}
void JSValue::defineProperty(const JSValue& key,const PropertyDescriptor& descriptor) {
    if(!defineOwnProperty(key,descriptor))throw JSTypeError("Cannot redefine property");
}
bool JSValue::erase(const JSValue& input) {
    static_cast<void>(JSWeakValue(*this));
    if(!isObject())throw JSTypeError("Expected runtime object");
    const auto key=input.toPropertyKey();if(!key.isSymbol())return erase(key.string());
    static_cast<void>(JSWeakValue(*this));static_cast<void>(JSWeakValue(key));
    if(!isObject())throw JSTypeError("Expected runtime object");heap_->validateEdge(key);
    if(isProxy())return JSProxyDetail::erase(*this,key);
    auto& properties=heap_->record(*this).symbolProperties;
    const auto found=std::find_if(properties.begin(),properties.end(),[&](const auto& entry){return entry.key.sameValue(key);});
    if(found==properties.end())return true;if(!found->property.configurable)return false;properties.erase(found);return true;
}
std::vector<JSValue> JSValue::ownPropertySymbols() const {
    static_cast<void>(JSWeakValue(*this));if(!isObject())throw JSTypeError("Expected runtime object");
    if(isProxy()){std::vector<JSValue> result;for(const auto& key:JSProxyDetail::keys(*this))if(key.isSymbol())result.push_back(key);return result;}
    std::vector<JSValue> result;for(const auto& entry:heap_->record(*this).symbolProperties)result.push_back(entry.key);return result;
}
std::vector<JSValue> JSValue::ownPropertyKeys() const {
    if(isProxy())return JSProxyDetail::keys(*this);
    static_cast<void>(JSWeakValue(*this));
    std::vector<JSValue> result;for(const auto& name:ownPropertyNames())result.emplace_back(name);
    for(const auto& symbol:ownPropertySymbols())result.push_back(symbol);return result;
}
JSValue JSValue::getPrototype() const {
    if (!isObject()) throw JSTypeError("Expected runtime object");
    if(isProxy())return JSProxyDetail::prototype(*this);
    return heap_->record(*this).prototype;
}
bool JSValue::setPrototype(const JSValue& prototype) {
    if (!isObject() || (!prototype.isObject() && !prototype.isNull())) throw JSTypeError("Invalid prototype");
    heap_->validateEdge(prototype);
    if(isProxy())return JSProxyDetail::setPrototype(*this,prototype);
    auto& record = heap_->record(*this);
    if (record.prototype.strictEquals(prototype)) return true;
    if (!record.extensible || record.immutablePrototype) return false;
    for (auto previous = prototype; !previous.isNull(); previous = previous.getPrototype()) {
        if (previous.strictEquals(*this)) return false;
        // OrdinarySetPrototypeOf stops before invoking an exotic [[GetPrototypeOf]].
        if (previous.isProxy()) break;
    }
    record.prototype = prototype.graphEdge(); return true;
}
void JSValue::preventExtensions() { if (!isObject()) throw JSTypeError("Expected runtime object"); if(isProxy()){if(!JSProxyDetail::preventExtensions(*this))throw JSTypeError("Proxy refused preventExtensions");return;} heap_->record(*this).extensible = false; }
bool JSValue::isExtensible() const { if (!isObject()) throw JSTypeError("Expected runtime object"); if(isProxy())return JSProxyDetail::extensible(*this);return heap_->record(*this).extensible; }
void JSValue::setPrimitiveConverter(const JSValue& converter) {
    defineProperty(wellKnownSymbol("toPrimitive"),dataDescriptor(converter,true,false,true));
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
    static_cast<void>(JSWeakValue(*this));
    std::unordered_set<std::uint64_t> active;
    std::function<std::optional<JsonValue>(JSValue, const std::string&)> encode = [&](JSValue value, const std::string& key) -> std::optional<JsonValue> {
        if (value.isObject()) { const auto method = value.get("toJSON"); if (method.isCallable()) value = method.call(value, {key}); }
        switch (value.type()) {
        case Type::Undefined: case Type::Function: case Type::Symbol: return std::nullopt;
        case Type::Null: return JsonValue(nullptr);
        case Type::Boolean: return JsonValue(std::get<bool>(value.value_));
        case Type::Number: return std::isfinite(value.number()) ? JsonValue(value.number() == 0 ? 0 : value.number()) : JsonValue(nullptr);
        case Type::String: return JsonValue(value.string());
        default: break;
        }
        const auto id = std::get<Reference>(value.value_).id;
        if (!active.insert(id).second) throw JSTypeError("Circular structure in JSON");
        JsonValue result;
        if (isNativeArray(value)) {
            JsonValue::Array entries;
            const auto length = arrayLikeLength(value);
            for (std::uint64_t i = 0; i < length; ++i) {
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
    static_cast<void>(JSWeakValue(*this));
    std::unordered_map<std::uint64_t, std::uint64_t> identities;
    std::function<JsonValue(JSValue)> encode = [&](JSValue value) -> JsonValue {
        switch (value.type()) {
        case Type::Undefined: return JsonValue::Object{{"type", "undefined"}};
        case Type::Null: return JsonValue::Object{{"type", "null"}};
        case Type::Boolean: return JsonValue::Object{{"type", "boolean"}, {"value", std::get<bool>(value.value_)}};
        case Type::String: return JsonValue::Object{{"type", "string"}, {"value", value.string()}};
        case Type::Symbol: {const auto description=value.symbolDescription();return JsonValue::Object{{"type","symbol"},{"description",description?JsonValue(*description):JsonValue(nullptr)}};}
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
