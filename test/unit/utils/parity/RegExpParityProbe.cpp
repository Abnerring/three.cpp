#include "core/JSRegExp.h"
#include "core/JSStringEncoding.h"
#include <iomanip>
#include <iostream>
#include <sstream>
namespace {
using Args = std::vector<JSValue>;
std::string unhex(const std::string& input) {
    std::u16string text;
    if (input != "-")
        for (std::size_t i = 0; i < input.size(); i += 4)
            text += static_cast<char16_t>(std::stoul(input.substr(i, 4), nullptr, 16));
    return jsUTF8(text);
}
std::string hex(const std::string& input) {
    std::ostringstream stream;
    for (auto c : jsUTF16(input))
        stream << std::hex << std::setw(4) << std::setfill('0') << unsigned(c);
    return stream.str();
}
JsonValue encode(const JSValue& value) {
    if (value.isUndefined())
        return JsonValue::Object{{"undefined", true}};
    if (value.type() == JSValue::Type::String)
        return JsonValue::Object{{"string", hex(value.string())}};
    if (value.type() == JSValue::Type::Array) {
        JsonValue::Array out;
        for (unsigned i = 0; i < value.length(); ++i)
            out.push_back(encode(value.get(std::to_string(i))));
        return out;
    }
    return value.toJSON().value_or(JsonValue(nullptr));
}
JSValue callback(const JSValue&, const Args& args, const Args&) {
    std::string result = "{";
    for (const auto& value : args)
        result += encode(value).dump() + ";";
    return result + "}";
}
} // namespace
int main() {
    const auto realm = JSValue::object();
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream fields(line);
        char mode;
        std::string p, f, s, r;
        double number = 0;
        fields >> mode >> p >> f >> s >> r >> number;
        try {
            const JSValue input(unhex(s));
            if (mode == 'T' || mode == 'U') {
                std::cout
                    << encode(input.get(mode == 'T' ? "trim" : "toUpperCase").call(input)).dump()
                    << '\n';
                continue;
            }
            if (mode == 'L' || mode == 'S') {
                auto value = input.get(mode == 'L' ? "replace" : "split")
                                 .call(input, mode == 'L' ? Args{unhex(p), unhex(r)}
                                                          : Args{unhex(p), number});
                std::cout << encode(value).dump() << '\n';
                continue;
            }
            auto regexp = createNativeRegExp(unhex(p), unhex(f));
            regexp.set("lastIndex", number);
            if (mode == 'E') {
                JsonValue::Array results;
                for (unsigned i = 0; i < 3; ++i) {
                    const auto value = regexp.get("exec").call(regexp, {input});
                    results.emplace_back(JsonValue::Object{
                        {"value", encode(value)},
                        {"index", value.isNull() ? JsonValue(nullptr) : encode(value.get("index"))},
                        {"lastIndex", encode(regexp.get("lastIndex"))}});
                }
                std::cout << JsonValue(JsonValue::Object{{"results", results},
                                                         {"source", encode(regexp.get("source"))},
                                                         {"flags", encode(regexp.get("flags"))}})
                                 .dump()
                          << '\n';
            } else {
                const auto replacement =
                    mode == 'F' ? JSValue::function(callback) : JSValue(unhex(r));
                const auto value = input.get(mode == 'P' ? "split" : "replace")
                                       .call(input, mode == 'P' ? Args{regexp, number}
                                                                : Args{regexp, replacement});
                std::cout << JsonValue(
                                 JsonValue::Object{{"value", encode(value)},
                                                   {"lastIndex", encode(regexp.get("lastIndex"))}})
                                 .dump()
                          << '\n';
            }
        } catch (const JSSyntaxError&) {
            std::cout << "{\"error\":\"SyntaxError\"}\n";
        } catch (const JSRegExpUnsupported& e) {
            std::cout << JsonValue(JsonValue::Object{{"unsupported", e.what()}}).dump() << '\n';
        } catch (const JSTypeError&) {
            std::cout << "{\"error\":\"TypeError\"}\n";
        } catch (const std::exception& e) {
            std::cout << JsonValue(JsonValue::Object{{"nativeError", e.what()}}).dump() << '\n';
        }
    }
}
