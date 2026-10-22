#include "loaders/SVGLoader.h"

#include <cmath>
#include <cstdlib>
#include <stdexcept>

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

namespace {
bool isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
bool isDigit(char c) { return c >= '0' && c <= '9'; }
bool isSign(char c) { return c == '-' || c == '+'; }
bool isPoint(char c) { return c == '.'; }
bool isComma(char c) { return c == ','; }
bool isExp(char c) { return c == 'e' || c == 'E'; }
bool isFlag(char c) { return c == '0' || c == '1'; }
}  // namespace

std::vector<double> SVGLoader::parseFloats(const std::string& input,
                                           const std::vector<int>& flagPositions, int stride) {
    enum State { SEP, INT, FLOAT, EXP };
    State state = SEP;
    bool seenComma = true;
    std::string number;
    std::string exponent;
    std::vector<double> result;

    // A number is only pushed once it is complete, and the exponent scales it afterwards.
    const auto newNumber = [&result, &number, &exponent]() {
        if (!number.empty()) {
            const double base = std::strtod(number.c_str(), nullptr);
            if (exponent.empty()) {
                result.push_back(base);
            } else {
                result.push_back(base * std::pow(10.0, std::strtod(exponent.c_str(), nullptr)));
            }
        }
        number.clear();
        exponent.clear();
    };

    for (std::size_t i = 0; i < input.size(); ++i) {
        const char current = input[i];

        // A flag is recognised by its place in the group and needs no separator around it.
        if (stride > 0) {
            const int position = static_cast<int>(result.size()) % stride;
            bool isFlagPosition = false;
            for (const int flag : flagPositions) {
                if (flag == position) isFlagPosition = true;
            }
            if (isFlagPosition && isFlag(current)) {
                state = INT;
                number = std::string(1, current);
                newNumber();
                continue;
            }
        }

        if (state == SEP) {
            if (isWhitespace(current)) continue;
            if (isDigit(current) || isSign(current)) {
                state = INT;
                number = std::string(1, current);
                continue;
            }
            if (isPoint(current)) {
                state = FLOAT;
                number = std::string(1, current);
                continue;
            }
            if (isComma(current)) {
                // Two commas with no number between them are a syntax error.
                if (seenComma) {
                    throw std::runtime_error("Unexpected character \"" + std::string(1, current) +
                                             "\" at index " + std::to_string(i) + ".");
                }
                seenComma = true;
            }
        }

        if (state == INT) {
            if (isDigit(current)) { number += current; continue; }
            if (isPoint(current)) { number += current; state = FLOAT; continue; }
            if (isExp(current)) { state = EXP; continue; }
            // A sign directly after a sign is an error, but a sign after a number separates it.
            if (isSign(current) && number.size() == 1 && isSign(number[0])) {
                throw std::runtime_error("Unexpected character \"" + std::string(1, current) +
                                         "\" at index " + std::to_string(i) + ".");
            }
        }

        if (state == FLOAT) {
            if (isDigit(current)) { number += current; continue; }
            if (isExp(current)) { state = EXP; continue; }
            // Two decimal points in one number are an error.
            if (isPoint(current) && !number.empty() && number.back() == '.') {
                throw std::runtime_error("Unexpected character \"" + std::string(1, current) +
                                         "\" at index " + std::to_string(i) + ".");
            }
        }

        if (state == EXP) {
            if (isDigit(current)) { exponent += current; continue; }
            // Only the first character of the exponent may be a sign.
            if (isSign(current) && exponent.empty()) { exponent += current; continue; }
        }

        // Anything else ends the number in progress.
        state = SEP;
        newNumber();
        if (isComma(current)) seenComma = true;
        else seenComma = false;
    }
    newNumber();
    return result;
}