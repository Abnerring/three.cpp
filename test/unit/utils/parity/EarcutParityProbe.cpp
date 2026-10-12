#include "extras/lib/earcut.h"
#include "core/JSValue.h"
#include <iostream>
#include <sstream>

namespace {
JsonValue normalize(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value < 0 ? "-Infinity" : "Infinity";
    if (value == 0 && std::signbit(value)) return "-0";
    return value;
}
double readNumber(std::istream& input) {
    std::string text; input >> text;
    if (text == "NaN") return std::numeric_limits<double>::quiet_NaN();
    if (text == "Infinity") return std::numeric_limits<double>::infinity();
    if (text == "-Infinity") return -std::numeric_limits<double>::infinity();
    return std::stod(text);
}
template<class T> JsonValue array(const std::vector<T>& values) {
    JsonValue::Array result;
    for (const auto value : values) result.push_back(normalize(static_cast<double>(value)));
    return result;
}
JsonValue query(std::istream& input) {
    char operation; input >> operation;
    if (operation == 'T') {
        std::size_t dim, length, holesLength; input >> dim >> length;
        std::vector<double> data; for (std::size_t i = 0; i < length; ++i) data.push_back(readNumber(input));
        input >> holesLength;
        std::vector<std::size_t> holes; for (std::size_t i = 0; i < holesLength; ++i) { std::size_t index; input >> index; holes.push_back(index); }
        const auto triangles = earcut(data, holes, dim);
        return JsonValue::Object{{"triangles", array(triangles)}, {"deviation", normalize(deviation(data, holes, dim, triangles))}};
    }
    if (operation == 'F') {
        std::size_t rings; input >> rings;
        std::vector<std::vector<std::vector<double>>> data;
        for (std::size_t i = 0; i < rings; ++i) {
            std::size_t count; input >> count;
            std::vector<std::vector<double>> ring;
            for (std::size_t j = 0; j < count; ++j) {
                std::size_t dim; input >> dim;
                std::vector<double> point; for (std::size_t k = 0; k < dim; ++k) point.push_back(readNumber(input));
                ring.push_back(std::move(point));
            }
            data.push_back(std::move(ring));
        }
        const auto result = flatten(data);
        return JsonValue::Object{{"vertices", array(result.vertices)}, {"holes", array(result.holes)}, {"dimensions", result.dimensions}};
    }
    throw std::invalid_argument("Unknown earcut probe operation");
}
}
int main() {
    try {
        std::string line;
        JsonValue::Array results;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;
            std::istringstream input(line);
            try { results.push_back(query(input)); }
            catch (const JSTypeError&) { results.push_back(JsonValue::Object{{"error", "TypeError"}}); }
            catch (const JSRangeError&) { results.push_back(JsonValue::Object{{"error", "RangeError"}}); }
        }
        std::cout << JsonValue(std::move(results)).dump() << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
