#include "loaders/SVGLoader.h"

namespace {
bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }
}  // namespace

std::string SVGLoader::gradientId(const std::string& value) {
    std::size_t at = 0;
    // Whitespace before the word is allowed, and only before it.
    while (at < value.size() && isSpace(value[at])) ++at;
    if (value.compare(at, 4, "url(") != 0) return std::string();
    at += 4;

    // Whitespace, then an optional opening quote, then more whitespace.
    while (at < value.size() && isSpace(value[at])) ++at;
    if (at < value.size() && (value[at] == '"' || value[at] == '\'')) {
        ++at;
        while (at < value.size() && isSpace(value[at])) ++at;
    }

    // The identifier must open with a hash.
    if (at >= value.size() || value[at] != '#') return std::string();
    ++at;

    // It runs until a quote, a closing bracket or whitespace, and must not be empty.
    const std::size_t start = at;
    while (at < value.size()) {
        const char c = value[at];
        if (c == ')' || c == '"' || c == '\'' || isSpace(c)) break;
        ++at;
    }
    if (at == start) return std::string();
    const std::string id = value.substr(start, at - start);

    // What follows may be whitespace, an optional closing quote, more whitespace and the
    // bracket, and then only whitespace to the end. Anything else means the value is not a
    // single reference.
    while (at < value.size() && isSpace(value[at])) ++at;
    if (at < value.size() && (value[at] == '"' || value[at] == '\'')) {
        ++at;
        while (at < value.size() && isSpace(value[at])) ++at;
    }
    if (at >= value.size() || value[at] != ')') return std::string();
    ++at;
    while (at < value.size() && isSpace(value[at])) ++at;
    if (at != value.size()) return std::string();

    return id;
}
