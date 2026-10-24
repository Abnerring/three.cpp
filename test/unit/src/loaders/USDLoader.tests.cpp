#include "loaders/USDLoader.h"

#include "catch.hpp"

#include <string>

using BufferCase = USDLoader::BufferCase;

TEST_CASE("USDLoader finds a lower case extension") {
    // A plain name gives its extension in lower case.
    REQUIRE(USDLoader::extension("model.usdz") == "usdz");
    REQUIRE(USDLoader::extension("MODEL.USDZ") == "usdz");
    REQUIRE(USDLoader::extension("model.UsDz") == "usdz");
    // A name with no dot has none.
    REQUIRE(USDLoader::extension("model").empty());
    REQUIRE(USDLoader::extension("").empty());
    // And a dot inside a directory followed by a bare file name gives none either, because the
    // last dot is before a slash; a reader taking the last dot alone would answer with the file
    // name instead.
    REQUIRE(USDLoader::extension("dir.d/model").empty());
    REQUIRE(USDLoader::extension("dir.d/model") != "d/model");
    // With a dot after the slash it is found as usual.
    REQUIRE(USDLoader::extension("dir/model.usda") == "usda");
    REQUIRE(std::string(USDLoader::kTypeName) == "USDLoader");
}

TEST_CASE("USDLoader's extension answers nothing twice for two different reasons") {
    // Both a missing dot and a dot before a slash give nothing, so the two cases are not
    // distinguishable from the answer alone.
    REQUIRE(USDLoader::extension("nodot") == USDLoader::extension("dir.d/bare"));
    REQUIRE(USDLoader::extension("nodot").empty());
    REQUIRE(USDLoader::extension("dir.d/bare").empty());
}

TEST_CASE("USDLoader converts whatever kind of view it is given") {
    // Already the whole buffer: passed through.
    REQUIRE(USDLoader::bufferCase(true, true) == BufferCase::WholeBuffer);
    // A view covering all of its backing store: replaced by that store.
    REQUIRE(USDLoader::bufferCase(false, true) == BufferCase::FullView);
    // A view covering part of it: copied out, so the answer's start is its own.
    REQUIRE(USDLoader::bufferCase(false, false) == BufferCase::PartialView);
    REQUIRE(USDLoader::bufferCase(true, false) == BufferCase::WholeBuffer);
    // The three differ, which is why there are three cases rather than two.
    REQUIRE(USDLoader::bufferCase(false, false) != USDLoader::bufferCase(false, true));
    REQUIRE(std::string(USDLoader::kResponseType) == "arraybuffer");
}

TEST_CASE("USDLoader takes its own path and answers before its textures") {
    // The loader's own path unless it is the empty string.
    REQUIRE(USDLoader::basePath("models/", "http://example/a/") == "models/");
    REQUIRE(USDLoader::basePath("", "http://example/a/") == "http://example/a/");
    // The group comes back at once while some textures arrive later.
    REQUIRE(USDLoader::returnsBeforeTexturesLoaded());
}