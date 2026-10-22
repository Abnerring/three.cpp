#include "loaders/PLYLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

namespace {
std::vector<std::string> tokens(const std::string& text) {
    std::vector<std::string> out;
    std::size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
        if (i >= text.size()) break;
        const std::size_t start = i;
        while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i]))) ++i;
        out.push_back(text.substr(start, i - start));
    }
    return out;
}
}  // namespace

TEST_CASE("PLYLoader reads scalar properties in declaration order") {
    const std::vector<PLYLoader::Property> properties{{"float", "x", "", ""},
                                                      {"float", "y", "", ""},
                                                      {"float", "z", "", ""}};
    const auto stream = tokens("1.5 2.5 3.5");
    std::size_t cursor = 0;
    const auto element = PLYLoader::parseASCIIElement(properties, stream, cursor);
    REQUIRE(element.has_value());
    REQUIRE(std::get<double>(element->at("x")) == Approx(1.5).epsilon(1e-12));
    REQUIRE(std::get<double>(element->at("z")) == Approx(3.5).epsilon(1e-12));
    REQUIRE(cursor == 3);
}

TEST_CASE("PLYLoader reads a list property from its count") {
    const std::vector<PLYLoader::Property> properties{{"list", "vertex_indices", "uchar", "int"}};
    const auto stream = tokens("3 0 1 2");
    std::size_t cursor = 0;
    const auto element = PLYLoader::parseASCIIElement(properties, stream, cursor);
    REQUIRE(element.has_value());
    const auto& list = std::get<std::vector<double>>(element->at("vertex_indices"));
    REQUIRE(list.size() == 3);
    REQUIRE(list[0] == 0.0);
    REQUIRE(list[2] == 2.0);
    REQUIRE(cursor == 4);
}

TEST_CASE("PLYLoader returns nothing when the stream runs out") {
    const std::vector<PLYLoader::Property> properties{{"float", "x", "", ""},
                                                      {"float", "y", "", ""}};
    const auto stream = tokens("1.5");
    std::size_t cursor = 0;
    REQUIRE_FALSE(PLYLoader::parseASCIIElement(properties, stream, cursor).has_value());

    // A list whose items run out also fails rather than returning a short list.
    const std::vector<PLYLoader::Property> listy{{"list", "vertex_indices", "uchar", "int"}};
    const auto shortStream = tokens("3 0 1");
    std::size_t cursor2 = 0;
    REQUIRE_FALSE(PLYLoader::parseASCIIElement(listy, shortStream, cursor2).has_value());
}

TEST_CASE("PLYLoader mixes scalars and lists") {
    const std::vector<PLYLoader::Property> properties{{"uchar", "red", "", ""},
                                                      {"list", "vertex_indices", "uchar", "int"},
                                                      {"float", "quality", "", ""}};
    const auto stream = tokens("7 2 4 5 0.25");
    std::size_t cursor = 0;
    const auto element = PLYLoader::parseASCIIElement(properties, stream, cursor);
    REQUIRE(element.has_value());
    REQUIRE(std::get<double>(element->at("red")) == 7.0);
    REQUIRE(std::get<std::vector<double>>(element->at("vertex_indices")).size() == 2);
    REQUIRE(std::get<double>(element->at("quality")) == Approx(0.25).epsilon(1e-12));
}
