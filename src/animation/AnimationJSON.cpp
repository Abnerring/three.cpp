#include "animation/AnimationUtils.h"
#include <cctype>

namespace Animation_detail {
namespace {
class Parser {
    const std::string& text;
    std::size_t position = 0;
    [[noreturn]] void fail() const { throw JSONSyntaxError("Invalid JSON at position " + std::to_string(position)); }
    void whitespace() { while (position < text.size() && (text[position] == ' ' || text[position] == '\n' || text[position] == '\r' || text[position] == '\t')) ++position; }
    bool consume(char c) { if (position < text.size() && text[position] == c) { ++position; return true; } return false; }
    static void appendUTF8(std::string& output, unsigned value) {
        if (value < 0x80) output += char(value);
        else if (value < 0x800) { output += char(0xc0 | (value >> 6)); output += char(0x80 | (value & 0x3f)); }
        else { output += char(0xe0 | (value >> 12)); output += char(0x80 | ((value >> 6) & 0x3f)); output += char(0x80 | (value & 0x3f)); }
    }
    std::string string() {
        if (!consume('"')) fail();
        std::string result;
        while (position < text.size()) {
            const auto c = static_cast<unsigned char>(text[position++]);
            if (c == '"') return JSValue(result).string();
            if (c < 0x20) fail();
            if (c != '\\') { result += char(c); continue; }
            if (position == text.size()) fail();
            switch (text[position++]) {
            case '"': result += '"'; break;
            case '\\': result += '\\'; break;
            case '/': result += '/'; break;
            case 'b': result += '\b'; break;
            case 'f': result += '\f'; break;
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            case 'u': {
                unsigned unit = 0;
                for (int i = 0; i < 4; ++i) {
                    if (position == text.size()) fail();
                    const auto hex = text[position++];
                    unit <<= 4;
                    if (hex >= '0' && hex <= '9') unit += hex - '0';
                    else if (hex >= 'a' && hex <= 'f') unit += hex - 'a' + 10;
                    else if (hex >= 'A' && hex <= 'F') unit += hex - 'A' + 10;
                    else fail();
                }
                appendUTF8(result, unit);
                break;
            }
            default: fail();
            }
        }
        fail();
    }
    bool digit() const { return position < text.size() && text[position] >= '0' && text[position] <= '9'; }
    JSValue number() {
        const auto start = position;
        consume('-');
        if (!consume('0')) {
            if (!digit() || text[position] == '0') fail();
            while (digit()) ++position;
        }
        if (consume('.')) { if (!digit()) fail(); while (digit()) ++position; }
        if (consume('e') || consume('E')) {
            if (!consume('+')) consume('-');
            if (!digit()) fail();
            while (digit()) ++position;
        }
        return JSValue(text.substr(start, position - start)).toNumber();
    }
    JSValue value() {
        whitespace();
        if (position == text.size()) fail();
        if (text[position] == '"') return string();
        if (text[position] == '-' || digit()) return number();
        if (consume('[')) {
            auto result = JSValue::array(); whitespace();
            if (consume(']')) return result;
            for (;;) {
                JSPropertyDescriptor descriptor;
                descriptor.value = value(); descriptor.writable = true;
                descriptor.configurable = true; descriptor.enumerable = true;
                result.defineProperty(std::to_string(result.length()), descriptor); whitespace();
                if (consume(']')) return result;
                if (!consume(',')) fail();
            }
        }
        if (consume('{')) {
            auto result = JSValue::object(); whitespace();
            if (consume('}')) return result;
            for (;;) {
                whitespace(); const auto key = string(); whitespace();
                if (!consume(':')) fail();
                JSPropertyDescriptor descriptor;
                descriptor.value = value(); descriptor.writable = true;
                descriptor.configurable = true; descriptor.enumerable = true;
                result.defineProperty(key, descriptor); whitespace();
                if (consume('}')) return result;
                if (!consume(',')) fail();
            }
        }
        for (const auto& [word, result] : std::initializer_list<std::pair<std::string, JSValue>>{{"true", true}, {"false", false}, {"null", nullptr}})
            if (text.compare(position, word.size(), word) == 0) { position += word.size(); return result; }
        fail();
    }
  public:
    explicit Parser(const std::string& source) : text(source) {}
    JSValue parse() { const auto result = value(); whitespace(); if (position != text.size()) fail(); return result; }
};
std::string stringify(const JsonValue& value) {
    return std::visit([](const auto& raw) -> std::string {
        using T = std::decay_t<decltype(raw)>;
        if constexpr (std::is_same_v<T, double>) return std::isfinite(raw) ? JSValue(raw).toString() : "null";
        else if constexpr (std::is_same_v<T, JsonValue::Array>) {
            std::string result = "[";
            for (std::size_t i = 0; i < raw.size(); ++i) { if (i) result += ','; result += stringify(raw[i]); }
            return result + ']';
        } else if constexpr (std::is_same_v<T, JsonValue::Object>) {
            std::string result = "{"; bool first = true;
            for (const auto& [key, value] : raw) {
                if (!first) result += ','; first = false;
                result += JsonValue(key).dump() + ':' + stringify(value);
            }
            return result + '}';
        } else return JsonValue(raw).dump();
    }, value.storage());
}
}
JSValue parseJSON(const std::string& text) { return Parser(text).parse(); }
std::optional<std::string> stringifyJSON(const JSValue& value) {
    const auto json = value.toJSON();
    return json ? std::optional<std::string>(stringify(*json)) : std::nullopt;
}
}
