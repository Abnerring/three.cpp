#include "core/JSRegExp.h"
#include "core/JSNumber.h"
#include "core/JSString.h"
#include "core/JSStringEncoding.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <regex>

namespace {
using Args = std::vector<JSValue>;
JSValue argument(const Args& args, std::size_t index) {
    return index < args.size() ? args[index] : JSValue{};
}
void check(const JSValue& value) { static_cast<void>(JSWeakValue(value)); }
double integer(const JSValue& value) {
    const auto n = value.toNumber();
    return std::isnan(n) || n == 0 ? 0 : std::trunc(n);
}
double lengthOf(const JSValue& value) {
    return std::min(std::max(integer(value), 0.0), 9007199254740991.0);
}
std::uint32_t uint32(const JSValue& value) {
    const auto n = value.toNumber();
    if (!std::isfinite(n) || n == 0)
        return 0;
    auto r = std::fmod(std::trunc(n), 4294967296.0);
    if (r < 0)
        r += 4294967296.0;
    return static_cast<std::uint32_t>(r);
}
JSPropertyDescriptor data(const JSValue& value, bool writable = true, bool enumerable = false,
                          bool configurable = true) {
    JSPropertyDescriptor d;
    d.value = value;
    d.writable = writable;
    d.enumerable = enumerable;
    d.configurable = configurable;
    return d;
}
JSPropertyDescriptor accessor(const JSValue& getter) {
    JSPropertyDescriptor d;
    d.getter = getter;
    d.setter = JSValue{};
    d.enumerable = false;
    d.configurable = true;
    return d;
}
bool asciiWord(wchar_t c) {
    return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9') ||
           c == L'_';
}
// Keep the C++ ECMAScript matcher, but replace its locale-dependent character
// classes with the precise non-Unicode ECMAScript sets (including all \s).
struct ECMATraits : std::regex_traits<wchar_t> {
    using char_class_type = std::uint32_t;
    template <class It> char_class_type lookup_classname(It first, It last, bool = false) const {
        const std::wstring name(first, last);
        if (name == L"s" || name == L"S")
            return 1;
        if (name == L"w" || name == L"W")
            return 2;
        if (name == L"d" || name == L"D")
            return 4;
        return 0;
    }
    bool isctype(wchar_t c, char_class_type flags) const {
        if (flags == std::numeric_limits<char_class_type>::max())
            return asciiWord(c);
        return ((flags & 1) && c <= 0xffff && jsWhitespace(static_cast<char16_t>(c))) ||
               ((flags & 2) && asciiWord(c)) || ((flags & 4) && c >= L'0' && c <= L'9');
    }
};
using Regex = std::basic_regex<wchar_t, ECMATraits>;
struct RegExpData {
    std::string source, flags;
    Regex matcher;
    bool global = false, sticky = false;
};
std::shared_ptr<RegExpData> requireData(const JSValue& value) {
    check(value);
    if (!value.isHostObject("three.RegExp"))
        throw JSTypeError("RegExp method called on incompatible receiver");
    return std::static_pointer_cast<RegExpData>(value.hostPayload("three.RegExp"));
}
bool decimal(char16_t c) { return c >= u'0' && c <= u'9'; }
bool hexadecimal(char16_t c) {
    return decimal(c) || (c >= u'a' && c <= u'f') || (c >= u'A' && c <= u'F');
}
bool classEscape(char16_t c) {
    return c == u's' || c == u'S' || c == u'd' || c == u'D' || c == u'w' || c == u'W';
}
void unitEscape(std::wstring& out, unsigned value) {
    static constexpr wchar_t digits[] = L"0123456789abcdef";
    out += L"\\u";
    for (int shift = 12; shift >= 0; shift -= 4)
        out += digits[(value >> shift) & 15];
}
std::wstring compilePattern(std::u16string_view source, bool dotAll) {
    std::size_t captures = 0;
    bool scanningClass = false;
    for (std::size_t i = 0; i < source.size(); ++i) {
        const auto c = source[i];
        if (c == u'\\') {
            ++i;
            continue;
        }
        if (c == u'[')
            scanningClass = true;
        else if (c == u']')
            scanningClass = false;
        else if (!scanningClass && c == u'(' && (i + 1 == source.size() || source[i + 1] != u'?'))
            ++captures;
    }
    std::wstring out;
    bool inClass = false, previousSetEscape = false;
    std::vector<bool> assertions;
    std::vector<std::size_t> openCaptures;
    std::size_t openedCaptures = 0;
    for (std::size_t i = 0; i < source.size(); ++i) {
        const auto c = source[i];
        if (c == u'\\') {
            if (i + 1 == source.size()) {
                out += L'\\';
                continue;
            }
            const auto escape = source[++i];
            previousSetEscape = classEscape(escape);
            if (escape == u'k' && i + 1 < source.size() && source[i + 1] == u'<')
                throw JSRegExpUnsupported("Named RegExp backreferences are not implemented");
            if (decimal(escape)) {
                std::size_t end = i;
                double number = 0;
                while (end < source.size() && decimal(source[end])) {
                    number = number * 10 + (source[end] - u'0');
                    ++end;
                }
                if (!inClass && escape != u'0' && number <= captures) {
                    // A forward or still-open capture is undefined at this
                    // point. ECMAScript backreferences to it match the empty
                    // string; the C++ matcher rejects forward references.
                    if (number > openedCaptures ||
                        std::find(openCaptures.begin(), openCaptures.end(),
                                  static_cast<std::size_t>(number)) != openCaptures.end()) {
                        out += L"(?:)";
                        i = end - 1;
                        continue;
                    }
                    out += L'\\';
                    for (; i < end; ++i)
                        out += static_cast<wchar_t>(source[i]);
                    --i;
                    continue;
                }
                if (escape >= u'8') {
                    out += static_cast<wchar_t>(escape);
                    continue;
                }
                unsigned value = escape - u'0';
                const unsigned maxDigits = escape <= u'3' ? 3 : 2;
                for (unsigned j = 1; j < maxDigits && i + 1 < source.size() &&
                                     source[i + 1] >= u'0' && source[i + 1] <= u'7';
                     ++j)
                    value = value * 8 + (source[++i] - u'0');
                unitEscape(out, value);
                continue;
            }
            if (escape == u'x' || escape == u'u') {
                const std::size_t count = escape == u'x' ? 2 : 4;
                bool valid = i + count < source.size();
                for (std::size_t j = 1; valid && j <= count; ++j)
                    valid = hexadecimal(source[i + j]);
                if (!valid) {
                    out += static_cast<wchar_t>(escape);
                    continue;
                }
            }
            if (escape == u'c') {
                const auto next = i + 1 < source.size() ? source[i + 1] : char16_t{};
                if (inClass && (decimal(next) || next == u'_')) {
                    unitEscape(out, next % 32);
                    ++i;
                    continue;
                }
                if (!((next >= u'a' && next <= u'z') || (next >= u'A' && next <= u'Z'))) {
                    out += L"\\\\c";
                    continue;
                }
            }
            out += L'\\';
            out += static_cast<wchar_t>(escape);
            continue;
        }
        if (!inClass && c == u'(') {
            if (i + 2 < source.size() && source[i + 1] == u'?' && source[i + 2] == u'<')
                throw JSRegExpUnsupported("Named captures and lookbehind are not implemented");
            assertions.push_back(i + 2 < source.size() && source[i + 1] == u'?' &&
                                 (source[i + 2] == u'=' || source[i + 2] == u'!'));
            openCaptures.push_back(
                i + 1 == source.size() || source[i + 1] != u'?' ? ++openedCaptures : 0);
        }
        if (!inClass && c == u')' && !assertions.empty()) {
            const bool assertion = assertions.back();
            assertions.pop_back();
            openCaptures.pop_back();
            if (assertion && i + 1 < source.size() &&
                (source[i + 1] == u'*' || source[i + 1] == u'+' || source[i + 1] == u'?' ||
                 source[i + 1] == u'{'))
                throw JSRegExpUnsupported("Quantified lookahead assertions are not implemented");
        }
        if (inClass && c == u'[') {
            unitEscape(out, c);
            previousSetEscape = false;
            continue;
        }
        if (inClass && c == u'-' &&
            (previousSetEscape ||
             (i + 2 < source.size() && source[i + 1] == u'\\' && classEscape(source[i + 2])))) {
            out += L"\\-";
            previousSetEscape = false;
            continue;
        }
        previousSetEscape = false;
        if (!inClass && c == u']') {
            out += L"\\]";
            continue;
        }
        if (c == u'[' && !inClass)
            inClass = true;
        else if (c == u']' && inClass)
            inClass = false;
        if (!inClass && c == u'{') {
            std::size_t end = i + 1;
            while (end < source.size() && decimal(source[end]))
                ++end;
            const bool hasMinimum = end > i + 1;
            if (end < source.size() && source[end] == u',') {
                ++end;
                while (end < source.size() && decimal(source[end]))
                    ++end;
            }
            if (hasMinimum && end < source.size() && source[end] == u'}') {
                for (; i <= end; ++i)
                    out += static_cast<wchar_t>(source[i]);
                --i;
                continue;
            }
            out += L"\\{";
            continue;
        }
        if (!inClass && c == u'}') {
            out += L"\\}";
            continue;
        }
        if (c == u'.' && !inClass && dotAll)
            out += L"[\\s\\S]";
        else
            out += static_cast<wchar_t>(c);
    }
    return out;
}
std::shared_ptr<RegExpData> compile(const std::string& source, const std::string& flags) {
    bool seen[128]{};
    for (unsigned char flag : flags) {
        if (flag >= 128 ||
            std::string_view("dgimsuvy").find(static_cast<char>(flag)) == std::string_view::npos ||
            seen[flag])
            throw JSSyntaxError("Invalid regular expression flags");
        seen[flag] = true;
    }
    if (seen['u'] && seen['v'])
        throw JSSyntaxError("Incompatible Unicode regular expression flags");
    if (seen['d'] || seen['i'] || seen['u'] || seen['v'])
        throw JSRegExpUnsupported("RegExp flags d, i, u and v are not implemented");
    auto result = std::make_shared<RegExpData>();
    result->source = source;
    result->flags = flags;
    result->global = seen['g'];
    result->sticky = seen['y'];
    auto options = std::regex_constants::ECMAScript;
    if (seen['m'])
        options |= std::regex_constants::multiline;
    const auto pattern = compilePattern(jsUTF16(source), seen['s']);
    try {
        result->matcher.assign(pattern.begin(), pattern.end(), options);
    } catch (const std::regex_error& error) {
        if (error.code() == std::regex_constants::error_complexity ||
            error.code() == std::regex_constants::error_space ||
            error.code() == std::regex_constants::error_stack)
            throw JSRegExpUnsupported("RegExp matcher resource limit");
        throw JSSyntaxError("Invalid regular expression pattern");
    }
    return result;
}
JSValue allocate(const JSValue& prototype, const JSValue& source, const JSValue& flags) {
    const auto text = source.isUndefined() ? std::string{} : source.toString();
    const auto flagText = flags.isUndefined() ? std::string{} : flags.toString();
    auto payload = compile(text, flagText);
    auto result = JSValue::hostObject(payload, payload.get(), "three.RegExp");
    result.setPrototype(prototype);
    result.defineProperty("lastIndex", data(0, true, false, false));
    return result;
}
JSValue constructor(const JSValue&, const JSValue& actualNewTarget, const Args& args, const Args&) {
    const auto pattern = argument(args, 0), flags = argument(args, 1);
    const bool regexp = jsIsRegExp(pattern);
    const auto active = nativeRegExpConstructor();
    const auto newTarget = actualNewTarget.isUndefined() ? active : actualNewTarget;
    if (actualNewTarget.isUndefined() && regexp && flags.isUndefined()) {
        if (pattern.get("constructor").sameValue(active))
            return pattern;
    }
    JSValue source = pattern, chosenFlags = flags;
    if (isNativeRegExpObject(pattern)) {
        const auto payload = requireData(pattern);
        source = payload->source;
        if (flags.isUndefined())
            chosenFlags = payload->flags;
    } else if (regexp) {
        source = pattern.get("source");
        if (flags.isUndefined())
            chosenFlags = pattern.get("flags");
    }
    auto prototype = newTarget.get("prototype");
    if (!prototype.isObject())
        prototype = nativeRegExpPrototype();
    return allocate(prototype, source, chosenFlags);
}
JSValue builtinExec(const JSValue& receiver, const JSValue& string) {
    const auto payload = requireData(receiver);
    const auto text = jsUTF16(string.string());
    const auto last = lengthOf(receiver.get("lastIndex"));
    const auto start = payload->global || payload->sticky ? last : 0;
    auto self = receiver;
    if (start > text.size()) {
        if (payload->global || payload->sticky)
            self.set("lastIndex", 0);
        return nullptr;
    }
    const std::wstring wide(text.begin(), text.end());
    const auto offset = static_cast<std::size_t>(start);
    std::match_results<std::wstring::const_iterator> match;
    auto options = std::regex_constants::match_default;
    if (payload->sticky)
        options |= std::regex_constants::match_continuous;
    if (offset)
        options |= std::regex_constants::match_prev_avail;
    bool found;
    try {
        found = std::regex_search(wide.cbegin() + offset, wide.cend(), match, payload->matcher,
                                  options);
    } catch (const std::regex_error&) {
        throw JSRegExpUnsupported("RegExp matcher resource limit");
    }
    if (!found) {
        if (payload->global || payload->sticky)
            self.set("lastIndex", 0);
        return nullptr;
    }
    const auto index = offset + static_cast<std::size_t>(match.position());
    if (payload->global || payload->sticky)
        self.set("lastIndex", index + match.length());
    auto result = JSValue::array(static_cast<std::uint32_t>(match.size()));
    for (std::size_t i = 0; i < match.size(); ++i) {
        JSValue capture;
        if (match[i].matched)
            capture = jsUTF8(std::u16string_view(text).substr(
                offset + static_cast<std::size_t>(match.position(i)),
                static_cast<std::size_t>(match.length(i))));
        result.defineProperty(std::to_string(i), data(capture, true, true, true));
    }
    result.defineProperty("index", data(index, true, true, true));
    result.defineProperty("input", data(string, true, true, true));
    result.defineProperty("groups", data({}, true, true, true));
    return result;
}
JSValue exec(const JSValue& self, const Args& args, const Args&) {
    requireData(self);
    const auto text = argument(args, 0).toString();
    return builtinExec(self, text);
}
JSValue test(const JSValue& self, const Args& args, const Args&) {
    if (!self.isObject())
        throw JSTypeError("RegExp.test requires an object");
    const auto text = argument(args, 0).toString();
    return !jsRegExpExec(self, text).isNull();
}
JSValue sourceGetter(const JSValue& self, const Args&, const Args&) {
    if (self.sameValue(nativeRegExpPrototype()))
        return "(?:)";
    const auto source = jsUTF16(requireData(self)->source);
    if (source.empty())
        return "(?:)";
    std::u16string out;
    bool escaped = false, inClass = false;
    for (const auto c : source) {
        if (c == u'\n') {
            if (escaped)
                out.pop_back();
            out += u"\\n";
            escaped = false;
            continue;
        }
        if (c == u'\r') {
            if (escaped)
                out.pop_back();
            out += u"\\r";
            escaped = false;
            continue;
        }
        if (c == 0x2028) {
            if (escaped)
                out.pop_back();
            out += u"\\u2028";
            escaped = false;
            continue;
        }
        if (c == 0x2029) {
            if (escaped)
                out.pop_back();
            out += u"\\u2029";
            escaped = false;
            continue;
        }
        if (!escaped && !inClass && c == u'/')
            out += u'\\';
        out += c;
        if (!escaped && c == u'[')
            inClass = true;
        else if (!escaped && c == u']')
            inClass = false;
        if (!escaped && c == u'\\')
            escaped = true;
        else
            escaped = false;
    }
    return jsUTF8(out);
}
JSValue flagGetter(const JSValue& self, const Args&, const Args& captures) {
    if (self.sameValue(nativeRegExpPrototype()))
        return {};
    const auto payload = requireData(self);
    return payload->flags.find(captures[0].string()) != std::string::npos;
}
JSValue flagsGetter(const JSValue& self, const Args&, const Args&) {
    if (!self.isObject())
        throw JSTypeError("RegExp.flags requires an object");
    const char* names[] = {"hasIndices", "global",  "ignoreCase",  "multiline",
                           "dotAll",     "unicode", "unicodeSets", "sticky"};
    const char* letters = "dgimsuvy";
    std::string result;
    bool enabled[8]{};
    // Fixed Node 26.5/V8 compatibility: V8 reads sticky before unicodeSets,
    // unlike ECMA-262 22.2.6.4; flag output remains in d-g-i-m-s-u-v-y order.
    const unsigned readOrder[] = {0, 1, 2, 3, 4, 5, 7, 6};
    for (const auto index : readOrder)
        enabled[index] = self.get(names[index]).toBoolean();
    for (unsigned i = 0; i < 8; ++i)
        if (enabled[i])
            result += letters[i];
    return result;
}
JSValue toString(const JSValue& self, const Args&, const Args&) {
    if (!self.isObject())
        throw JSTypeError("RegExp.toString requires an object");
    const auto source = self.get("source").toString();
    const auto flags = self.get("flags").toString();
    return "/" + source + "/" + flags;
}
JSValue groupsObject(const JSValue& groups) {
    if (groups.isObject())
        return groups;
    if (groups.isNull() || groups.isUndefined())
        throw JSTypeError("Cannot convert named captures to object");
    if (groups.type() == JSValue::Type::String)
        return createNativeStringObject(groups);
    if (groups.type() == JSValue::Type::Number)
        return createNativeNumberObject(groups);
    throw JSRegExpUnsupported("Boolean and Symbol named-capture boxing is not implemented");
}
JSValue replace(const JSValue& self, const Args& args, const Args&) {
    if (!self.isObject())
        throw JSTypeError("RegExp.replace requires an object");
    const auto text = jsUTF16(argument(args, 0).toString());
    const auto replacement = argument(args, 1);
    const bool functional = replacement.isCallable();
    const auto replacementText = functional ? std::u16string{} : jsUTF16(replacement.toString());
    const auto flags = self.get("flags").toString();
    const bool global = flags.find('g') != std::string::npos;
    const bool unicode =
        global && (flags.find('u') != std::string::npos || flags.find('v') != std::string::npos);
    auto receiver = self;
    if (global)
        receiver.set("lastIndex", 0);
    std::vector<JSValue> results;
    const JSValue input(jsUTF8(text));
    for (;;) {
        const auto result = jsRegExpExec(receiver, input);
        if (result.isNull())
            break;
        results.push_back(result);
        if (!global)
            break;
        if (result.get("0").toString().empty()) {
            const auto index = lengthOf(receiver.get("lastIndex"));
            // A fake exec can expose a safe integer beyond the input's bounds.
            const auto next =
                index >= text.size()
                    ? index + 1
                    : double(jsAdvanceStringIndex(text, static_cast<std::size_t>(index), unicode));
            receiver.set("lastIndex", next);
        }
    }
    std::u16string out;
    std::size_t nextSource = 0;
    for (const auto& result : results) {
        const auto count = lengthOf(result.get("length"));
        const auto matched = jsUTF16(result.get("0").toString());
        const auto position = static_cast<std::size_t>(
            std::clamp(integer(result.get("index")), 0.0, double(text.size())));
        std::vector<JSValue> captures;
        for (double i = 1; i < count; ++i) {
            const auto value = result.get(JSValue(i).toString());
            captures.push_back(value.isUndefined() ? JSValue{} : JSValue(value.toString()));
        }
        auto groups = result.get("groups");
        std::u16string substituted;
        if (functional) {
            Args values{jsUTF8(matched)};
            values.insert(values.end(), captures.begin(), captures.end());
            values.emplace_back(position);
            values.push_back(input);
            if (!groups.isUndefined())
                values.push_back(groups);
            substituted = jsUTF16(replacement.call({}, values).toString());
        } else {
            if (!groups.isUndefined())
                groups = groupsObject(groups);
            substituted =
                jsGetSubstitution(matched, text, position, captures, groups, replacementText);
        }
        if (position >= nextSource) {
            out.append(text, nextSource, position - nextSource);
            out += substituted;
            nextSource = position + matched.size();
        }
    }
    if (nextSource < text.size())
        out.append(text, nextSource, text.size() - nextSource);
    return jsUTF8(out);
}
JSValue speciesConstructor(const JSValue& receiver) {
    const auto ctor = receiver.get("constructor");
    if (ctor.isUndefined())
        return nativeRegExpConstructor();
    if (!ctor.isObject())
        throw JSTypeError("RegExp constructor property is not an object");
    const auto species = ctor.get(JSValue::wellKnownSymbol("species"));
    if (species.isUndefined() || species.isNull())
        return nativeRegExpConstructor();
    if (!species.isConstructible())
        throw JSTypeError("RegExp species is not a constructor");
    return species;
}
JSValue split(const JSValue& self, const Args& args, const Args&) {
    if (!self.isObject())
        throw JSTypeError("RegExp.split requires an object");
    const auto text = jsUTF16(argument(args, 0).toString());
    const auto ctor = speciesConstructor(self);
    auto flags = self.get("flags").toString();
    const bool unicode =
        flags.find('u') != std::string::npos || flags.find('v') != std::string::npos;
    if (flags.find('y') == std::string::npos)
        flags += 'y';
    auto splitter = ctor.construct({self, flags});
    auto out = JSValue::array();
    const auto limit = argument(args, 1).isUndefined() ? std::numeric_limits<std::uint32_t>::max()
                                                       : uint32(argument(args, 1));
    if (!limit)
        return out;
    const JSValue input(jsUTF8(text));
    std::uint32_t count = 0;
    auto append = [&](const JSValue& value) {
        out.defineProperty(std::to_string(count++), data(value, true, true, true));
        return count == limit;
    };
    if (text.empty()) {
        if (jsRegExpExec(splitter, input).isNull())
            append(input);
        return out;
    }
    std::size_t p = 0, q = 0;
    while (q < text.size()) {
        splitter.set("lastIndex", q);
        const auto result = jsRegExpExec(splitter, input);
        if (result.isNull()) {
            q = jsAdvanceStringIndex(text, q, unicode);
            continue;
        }
        const auto end = static_cast<std::size_t>(
            std::min(lengthOf(splitter.get("lastIndex")), double(text.size())));
        if (end == p) {
            q = jsAdvanceStringIndex(text, q, unicode);
            continue;
        }
        if (append(jsUTF8(std::u16string_view(text).substr(p, q - p))))
            return out;
        p = end;
        const auto length = lengthOf(result.get("length"));
        for (double i = 1; i < length; ++i)
            if (append(result.get(JSValue(i).toString())))
                return out;
        q = p;
    }
    append(jsUTF8(std::u16string_view(text).substr(p)));
    return out;
}
JSValue match(const JSValue& self, const Args& args, const Args&) {
    if (!self.isObject())
        throw JSTypeError("RegExp.match requires an object");
    const auto text = argument(args, 0).toString();
    const auto flags = self.get("flags").toString();
    if (flags.find('g') == std::string::npos)
        return jsRegExpExec(self, text);
    const bool unicode =
        flags.find('u') != std::string::npos || flags.find('v') != std::string::npos;
    const auto units = jsUTF16(text);
    auto receiver = self;
    receiver.set("lastIndex", 0);
    auto result = JSValue::array();
    std::uint32_t count = 0;
    for (;;) {
        const auto item = jsRegExpExec(receiver, text);
        if (item.isNull())
            return count ? result : JSValue(nullptr);
        const auto matched = item.get("0").toString();
        result.defineProperty(std::to_string(count++), data(matched, true, true, true));
        if (matched.empty()) {
            const auto index = lengthOf(receiver.get("lastIndex"));
            receiver.set("lastIndex", index >= units.size()
                                          ? index + 1
                                          : double(jsAdvanceStringIndex(
                                                units, static_cast<std::size_t>(index), unicode)));
        }
    }
}
} // namespace

bool isNativeRegExpObject(const JSValue& value) {
    check(value);
    return value.isHostObject("three.RegExp");
}
bool jsIsRegExp(const JSValue& value) {
    check(value);
    if (!value.isObject())
        return false;
    const auto matcher = value.get(JSValue::wellKnownSymbol("match"));
    if (!matcher.isUndefined())
        return matcher.toBoolean();
    return isNativeRegExpObject(value);
}
JSValue jsRegExpExec(const JSValue& receiver, const JSValue& string) {
    check(receiver);
    check(string);
    const auto method = receiver.get("exec");
    if (method.isCallable()) {
        const auto result = method.call(receiver, {string});
        if (!result.isObject() && !result.isNull())
            throw JSTypeError("RegExp exec result must be an object or null");
        return result;
    }
    return builtinExec(receiver, string);
}
std::u16string jsGetSubstitution(std::u16string_view matched, std::u16string_view string,
                                 std::size_t position, const Args& captures, const JSValue& groups,
                                 std::u16string_view replacement) {
    std::u16string out;
    for (std::size_t i = 0; i < replacement.size(); ++i) {
        if (replacement[i] != u'$' || i + 1 == replacement.size()) {
            out += replacement[i];
            continue;
        }
        const auto next = replacement[i + 1];
        if (next == u'$') {
            out += u'$';
            ++i;
            continue;
        }
        if (next == u'&') {
            out += matched;
            ++i;
            continue;
        }
        if (next == u'`') {
            out += string.substr(0, position);
            ++i;
            continue;
        }
        if (next == u'\'') {
            out += string.substr(std::min(position + matched.size(), string.size()));
            ++i;
            continue;
        }
        if (next >= u'0' && next <= u'9') {
            unsigned index = next - u'0';
            std::size_t consumed = 1;
            if (i + 2 < replacement.size() && replacement[i + 2] >= u'0' &&
                replacement[i + 2] <= u'9') {
                const auto pair = index * 10 + replacement[i + 2] - u'0';
                if (pair > 0 && pair <= captures.size()) {
                    index = pair;
                    consumed = 2;
                }
            }
            if (index > 0 && index <= captures.size()) {
                if (!captures[index - 1].isUndefined())
                    out += jsUTF16(captures[index - 1].string());
                i += consumed;
                continue;
            }
        }
        if (next == u'<' && !groups.isUndefined()) {
            const auto end = replacement.find(u'>', i + 2);
            if (end != std::u16string_view::npos) {
                const auto capture = groups.get(jsUTF8(replacement.substr(i + 2, end - i - 2)));
                if (!capture.isUndefined())
                    out += jsUTF16(capture.toString());
                i = end;
                continue;
            }
        }
        out += u'$';
    }
    return out;
}
JSValue nativeRegExpConstructor() {
    static thread_local JSWeakValue cached;
    if (const auto value = cached.lock(); value && value->isObject())
        return *value;
    auto ctor = JSValue::callableBuiltinConstructor(constructor, {}, "RegExp", 2);
    cached = JSWeakValue(ctor);
    auto proto = ctor.get("prototype");
    ctor.defineProperty("prototype", data(proto, false, false, false));
    const auto install = [&](const JSValue& key, JSValue::NativeFunction callback, const char* name,
                             unsigned arity) {
        auto method = JSValue::function(callback, {}, name, arity);
        method.retainForHeapLifetime();
        proto.defineProperty(key, data(method));
    };
    install("exec", exec, "exec", 1);
    const char* names[] = {"dotAll",    "flags",  "global", "hasIndices", "ignoreCase",
                           "multiline", "source", "sticky", "unicode",    "unicodeSets"};
    const char* letters = "s gdim yuv";
    for (unsigned i = 0; i < 10; ++i) {
        auto callback = i == 1 ? flagsGetter : i == 6 ? sourceGetter : flagGetter;
        auto getter = JSValue::function(
            callback, i == 1 || i == 6 ? Args{} : Args{std::string(1, letters[i])},
            std::string("get ") + names[i], 0);
        getter.retainForHeapLifetime();
        proto.defineProperty(names[i], accessor(getter));
    }
    install("toString", toString, "toString", 0);
    install("test", test, "test", 1);
    install(JSValue::wellKnownSymbol("match"), match, "[Symbol.match]", 1);
    install(JSValue::wellKnownSymbol("replace"), replace, "[Symbol.replace]", 2);
    install(JSValue::wellKnownSymbol("split"), split, "[Symbol.split]", 2);
    auto species = JSValue::function(
        +[](const JSValue& self, const Args&, const Args&) -> JSValue { return self; }, {},
        "get [Symbol.species]", 0);
    species.retainForHeapLifetime();
    ctor.defineProperty(JSValue::wellKnownSymbol("species"), accessor(species));
    ctor.retainForHeapLifetime();
    return ctor;
}
JSValue nativeRegExpPrototype() { return nativeRegExpConstructor().get("prototype"); }
JSValue createNativeRegExp(const JSValue& pattern, const JSValue& flags) {
    check(pattern);
    check(flags);
    return nativeRegExpConstructor().construct({pattern, flags});
}
