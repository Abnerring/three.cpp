#include "loaders/LDrawLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("LDrawLoader keys the cache by a lower cased name") {
    // Two spellings of the same name share a key.
    REQUIRE(LDrawLoader::cacheKey("3001.DAT") == "3001.dat");
    REQUIRE(LDrawLoader::cacheKey("3001.dat") == "3001.dat");
    REQUIRE(LDrawLoader::cacheKey("3001.dat") == LDrawLoader::cacheKey("3001.DAT"));
    // A path with mixed case lower cases every part.
    REQUIRE(LDrawLoader::cacheKey("Parts/Sub/Thing.dat") == "parts/sub/thing.dat");
    // Nothing stays nothing.
    REQUIRE(LDrawLoader::cacheKey("").empty());
}

TEST_CASE("LDrawLoader treats sixteen and twenty four differently") {
    // A face inherits with sixteen, an edge with twenty four.
    REQUIRE(LDrawLoader::isPassthroughCode("16", false));
    REQUIRE(LDrawLoader::isPassthroughCode("24", true));
    // Swapping them means neither is a passthrough.
    REQUIRE_FALSE(LDrawLoader::isPassthroughCode("24", false));
    REQUIRE_FALSE(LDrawLoader::isPassthroughCode("16", true));
    // Any other code is a real colour.
    REQUIRE_FALSE(LDrawLoader::isPassthroughCode("4", false));
    REQUIRE_FALSE(LDrawLoader::isPassthroughCode("4", true));
}

TEST_CASE("LDrawLoader resolves a passthrough to the parent colour") {
    // The passthrough takes the parent's code.
    REQUIRE(LDrawLoader::resolveColourCode("16", "4", false) == "4");
    REQUIRE(LDrawLoader::resolveColourCode("24", "4", true) == "4");
    // A real code is its own answer.
    REQUIRE(LDrawLoader::resolveColourCode("7", "4", false) == "7");
    // The wrong one for the kind is not a passthrough, so it stays.
    REQUIRE(LDrawLoader::resolveColourCode("16", "4", true) == "16");
    REQUIRE(LDrawLoader::resolveColourCode("24", "4", false) == "24");
}

TEST_CASE("LDrawLoader gives an edge the edge code of a face") {
    // A face that inherits its colour inherits its edge colour instead.
    REQUIRE(LDrawLoader::edgeColourCode("16") == "24");
    // Every other code is its own edge colour.
    REQUIRE(LDrawLoader::edgeColourCode("24") == "24");
    REQUIRE(LDrawLoader::edgeColourCode("4") == "4");
    REQUIRE(LDrawLoader::edgeColourCode("") == "");
}

TEST_CASE("LDrawLoader composes the edge and passthrough rules") {
    // A face asking for the containing colour gives its edge the edge passthrough, which then
    // resolves to the parent's colour.
    const std::string edgeOfFace = LDrawLoader::edgeColourCode("16");
    REQUIRE(edgeOfFace == "24");
    REQUIRE(LDrawLoader::resolveColourCode(edgeOfFace, "12", true) == "12");
    // The same composition for a face whose colour is its own leaves both alone.
    const std::string ownEdge = LDrawLoader::edgeColourCode("4");
    REQUIRE(ownEdge == "4");
    REQUIRE(LDrawLoader::resolveColourCode(ownEdge, "12", true) == "4");
}
