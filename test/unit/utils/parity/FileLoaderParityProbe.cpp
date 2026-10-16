// Differential probe for the native src/loaders/FileLoader.js port.
// stdin scenario: <status> <statusText> <body> <contentLength> <responseType> <cacheEnabled> <loads>
// Space separated, so "-" marks an empty field and "+" marks a space.
// The realm handle lives for the whole process: Cache mutations need a live realm.
#include "core/JSValue.h"
#include "core/JsonValue.h"
#include "loaders/Cache.h"
#include "loaders/FileLoader.h"
#include <charconv>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
namespace {
using Args = std::vector<JSValue>;
const char* kUrl = "https://host/asset.txt";
int calls = 0, loads = 0;
std::string lastUrl, received, errorText, body, statusText, contentLength, responseType;
double status = 200, progressLoaded = -1, progressTotal = -1;
bool progressComputable = false;
JsonValue number(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value < 0 ? "-Infinity" : "Infinity";
    if (value == 0 && std::signbit(value)) return "-0";
    return value;
}
double parseNumber(const std::string& token) {
    if (token == "NaN") return std::numeric_limits<double>::quiet_NaN();
    if (token == "Infinity") return std::numeric_limits<double>::infinity();
    if (token == "-Infinity") return -std::numeric_limits<double>::infinity();
    if (token == "-0") return -0.;
    double value = 0;
    std::from_chars(token.data(), token.data() + token.size(), value);
    return value;
}
std::string unescape(const std::string& token) {
    if (token == "-") return std::string{};
    std::string result = token;
    for (char& ch : result) if (ch == '+') ch = ' ';
    return result;
}
JSValue headerFn(const JSValue&, const Args& args, const Args&) {
    if (args.empty()) return {};
    return args[0].toString() == "Content-Length" ? JSValue(contentLength) : JSValue{};
}
JSValue transportFn(const JSValue&, const Args& args, const Args&) {
    calls += 1;
    if (!args.empty()) lastUrl = args[0].get("url").toString();
    auto response = JSValue::object();
    response.set("status", status);
    response.set("statusText", statusText);
    response.set("url", kUrl);
    auto headers = JSValue::object();
    headers.set("get", JSValue::function(headerFn, {}, "get", 1));
    response.set("headers", headers);
    response.set("body", body);
    return response;
}
JSValue onLoadFn(const JSValue&, const Args& args, const Args&) {
    loads += 1;
    received = args.empty() ? std::string{} : args[0].toString();
    return {};
}
JSValue onErrorFn(const JSValue&, const Args& args, const Args&) {
    errorText = args.empty() ? std::string{} : args[0].get("message").toString();
    return {};
}
JSValue onProgressFn(const JSValue&, const Args& args, const Args&) {
    if (args.empty()) return {};
    progressLoaded = args[0].get("loaded").toNumber();
    progressTotal = args[0].get("total").toNumber();
    progressComputable = args[0].get("lengthComputable").toBoolean();
    return {};
}
}
int main() {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream stream(line);
        std::string token;
        double cacheEnabled = 0, count = 0;
        try {
            stream >> token; status = parseNumber(token);
            stream >> token; statusText = unescape(token);
            stream >> token; body = unescape(token);
            stream >> token; contentLength = unescape(token);
            stream >> token; responseType = unescape(token);
            stream >> token; cacheEnabled = parseNumber(token);
            stream >> token; count = parseNumber(token);
            calls = 0; loads = 0; lastUrl.clear(); received.clear(); errorText.clear();
            progressLoaded = -1; progressTotal = -1; progressComputable = false;
            Cache.clear();
            Cache.enabled = cacheEnabled != 0;
            FileLoader::setTransport(JSValue::function(transportFn, {}, "transport", 1));
            FileLoader loader;
            if (!responseType.empty()) loader.setResponseType(responseType);
            for (double i = 0; i < count; i += 1) {
                try {
                    loader.load(kUrl, JSValue::function(onLoadFn, {}, "onLoad", 1),
                        JSValue::function(onProgressFn, {}, "onProgress", 1),
                        JSValue::function(onErrorFn, {}, "onError", 1));
                } catch (const std::exception& error) {
                    errorText = error.what();
                }
            }
            Cache.clear();
            Cache.enabled = false;
            FileLoader::setTransport(JSValue{});
            std::cout << JsonValue(JsonValue::Object{{"calls", number(calls)}, {"loads", number(loads)},
                                      {"received", received}, {"error", errorText}, {"lastUrl", lastUrl},
                                      {"progressLoaded", number(progressLoaded)},
                                      {"progressTotal", number(progressTotal)},
                                      {"progressComputable", progressComputable}})
                             .dump()
                      << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}