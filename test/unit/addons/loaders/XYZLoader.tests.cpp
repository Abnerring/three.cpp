#include "catch.hpp"
#include "core/JSValue.h"
#include "loaders/XYZLoader.h"
#include <cmath>
#include <string>

// The parse is asserted through the JS visible geometry, which is the surface the pinned class also
// exposes: each case checks the position count, a few array entries and whether a colour attribute
// exists at all.
//
// The attribute is a Float32Array, so a decimal literal that is not exactly representable in single
// precision is compared against its float rounding - 1.2 is stored as 1.2000000476837158.
namespace {
JSValue parseText(const std::string& text) {
    XYZLoader loader;
    return loader.parse(JSValue(text));
}
double countOf(const JSValue& geometry, const char* attribute) {
    return geometry.get("attributes").get(attribute).get("count").toNumber();
}
double arrayAt(const JSValue& geometry, const char* attribute, double index) {
    return geometry.get("attributes").get(attribute).get("array").get(index).toNumber();
}
bool hasAttribute(const JSValue& geometry, const char* attribute) {
    const auto attributes = geometry.get("attributes");
    return !attributes.get(attribute).isUndefined() && !attributes.get(attribute).isNull();
}
}

TEST_CASE("XYZLoader parses plain XYZ lines", "[xyzloader]") {
    const auto geometry = parseText("1 2 3\n4 5 6\n");
    REQUIRE(countOf(geometry, "position") == 2);
    REQUIRE(arrayAt(geometry, "position", 0) == 1.0);
    REQUIRE(arrayAt(geometry, "position", 5) == 6.0);
    REQUIRE_FALSE(hasAttribute(geometry, "color"));
}

TEST_CASE("XYZLoader skips comments, blank lines and other field counts", "[xyzloader]") {
    const auto geometry = parseText("# comment\n\n1 2 3\n1 2 3 4\n1 2 3 4 5\n\n7 8 9\n");
    REQUIRE(countOf(geometry, "position") == 2);
    REQUIRE(arrayAt(geometry, "position", 0) == 1.0);
    REQUIRE(arrayAt(geometry, "position", 3) == 7.0);
}

TEST_CASE("XYZLoader reads XYZRGB colours", "[xyzloader]") {
    const auto geometry = parseText("0 0 0 255 0 0\n1 1 1 0 255 0\n");
    REQUIRE(countOf(geometry, "position") == 2);
    REQUIRE(hasAttribute(geometry, "color"));
    REQUIRE(countOf(geometry, "color") == 2);
    // A fully saturated sRGB channel stays exactly one after the conversion to working space.
    REQUIRE(arrayAt(geometry, "color", 0) == 1.0);
    REQUIRE(arrayAt(geometry, "color", 1) == 0.0);
    REQUIRE(arrayAt(geometry, "color", 4) == 1.0);
}

TEST_CASE("XYZLoader follows JS parseFloat rather than stream extraction", "[xyzloader]") {
    const auto geometry = parseText("1e3 .5 1.2.3\nInfinity garbage -Infinity\n");
    REQUIRE(countOf(geometry, "position") == 2);
    REQUIRE(arrayAt(geometry, "position", 0) == 1000.0);
    REQUIRE(arrayAt(geometry, "position", 1) == 0.5);
    // The second decimal point ends the number, and the result lands in a Float32Array.
    REQUIRE(arrayAt(geometry, "position", 2) == static_cast<double>(1.2f));
    REQUIRE(std::isinf(arrayAt(geometry, "position", 3)));
    REQUIRE(arrayAt(geometry, "position", 3) > 0);
    // "garbage" has no numeric prefix, so it becomes NaN, and NaN survives into the float array.
    REQUIRE(std::isnan(arrayAt(geometry, "position", 4)));
    REQUIRE(arrayAt(geometry, "position", 5) < 0);
}

TEST_CASE("XYZLoader ignores whitespace runs and leading blanks", "[xyzloader]") {
    const auto geometry = parseText("   \t 1\t\t2   3  \n");
    REQUIRE(countOf(geometry, "position") == 1);
    REQUIRE(arrayAt(geometry, "position", 0) == 1.0);
    REQUIRE(arrayAt(geometry, "position", 2) == 3.0);
}