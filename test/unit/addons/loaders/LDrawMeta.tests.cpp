#include "loaders/LDrawLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("LDrawLoader sorts the meta directives") {
    // The six the loader reacts to.
    REQUIRE(LDrawLoader::metaKind("!LDRAW_ORG") == LDrawLoader::MetaKind::LDrawOrg);
    REQUIRE(LDrawLoader::metaKind("!COLOUR") == LDrawLoader::MetaKind::Colour);
    REQUIRE(LDrawLoader::metaKind("!CATEGORY") == LDrawLoader::MetaKind::Category);
    REQUIRE(LDrawLoader::metaKind("!KEYWORDS") == LDrawLoader::MetaKind::Keywords);
    REQUIRE(LDrawLoader::metaKind("FILE") == LDrawLoader::MetaKind::File);
    REQUIRE(LDrawLoader::metaKind("BFC") == LDrawLoader::MetaKind::Bfc);
    // A bare comment, and a meta the loader does not know, name nothing.
    REQUIRE(LDrawLoader::metaKind("") == LDrawLoader::MetaKind::None);
    REQUIRE(LDrawLoader::metaKind("a comment") == LDrawLoader::MetaKind::None);
    REQUIRE(LDrawLoader::metaKind("!HISTORY") == LDrawLoader::MetaKind::None);
}

TEST_CASE("LDrawLoader splits keywords on commas") {
    // Two keywords with the spaces around them trimmed.
    const auto two = LDrawLoader::parseKeywords("brick, red");
    REQUIRE(two.size() == 2);
    REQUIRE(two[0] == "brick");
    REQUIRE(two[1] == "red");
}

TEST_CASE("LDrawLoader trims the whitespace of each keyword") {
    // Runs of spaces on both sides are removed.
    const auto padded = LDrawLoader::parseKeywords("   one   ,\ttwo\t");
    REQUIRE(padded.size() == 2);
    REQUIRE(padded[0] == "one");
    REQUIRE(padded[1] == "two");
}

TEST_CASE("LDrawLoader keeps the empty pieces") {
    // Adjacent commas give an empty piece rather than being skipped.
    const auto gaps = LDrawLoader::parseKeywords("a,,b");
    REQUIRE(gaps.size() == 3);
    REQUIRE(gaps[0] == "a");
    REQUIRE(gaps[1].empty());
    REQUIRE(gaps[2] == "b");
    // An empty rest still gives one piece, because splitting an empty string does.
    const auto none = LDrawLoader::parseKeywords("");
    REQUIRE(none.size() == 1);
    REQUIRE(none[0].empty());
}

TEST_CASE("LDrawLoader keeps a keyword with no comma") {
    const auto single = LDrawLoader::parseKeywords("just one");
    REQUIRE(single.size() == 1);
    REQUIRE(single[0] == "just one");
}
