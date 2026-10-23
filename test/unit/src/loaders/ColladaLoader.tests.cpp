#include "loaders/ColladaLoader.h"

#include "catch.hpp"

#include <string>

using Outcome = ColladaLoader::Outcome;

TEST_CASE("ColladaLoader answers an empty scene for an empty text and nothing for a failure") {
    // An empty text gives an object holding an empty scene, so the caller always has something to
    // add to its own scene.
    REQUIRE(ColladaLoader::outcomeFor(true, false, false) == Outcome::EmptyScene);
    // A text that fails to parse gives nothing at all.
    REQUIRE(ColladaLoader::outcomeFor(false, true, false) == Outcome::Nothing);
    // So the two failures differ, which a caller testing only for emptiness would miss.
    REQUIRE(ColladaLoader::outcomeFor(true, false, false) !=
            ColladaLoader::outcomeFor(false, true, false));
    // A text that parses gives the composed result.
    REQUIRE(ColladaLoader::outcomeFor(false, false, true) == Outcome::Composed);
    REQUIRE(std::string(ColladaLoader::kTypeName) == "ColladaLoader");
}

TEST_CASE("ColladaLoader never hands an empty text to its parser") {
    // The length is checked first, so the parser does not run at all in that case.
    REQUIRE_FALSE(ColladaLoader::emptyTextReachesTheParser());
    // And the file is read as text rather than bytes.
    REQUIRE(std::string(ColladaLoader::kResponseType) == "text");
}

TEST_CASE("ColladaLoader takes its own path unless that is the empty string") {
    // The loader's own path wins when it is a value.
    REQUIRE(ColladaLoader::basePath("models/", "http://example/a/") == "models/");
    // Only the empty string is replaced by the URL's directory.
    REQUIRE(ColladaLoader::basePath("", "http://example/a/") == "http://example/a/");
    REQUIRE(ColladaLoader::basePath("", "http://example/a/") != "");
    // A single character is already a value, so it is kept.
    REQUIRE(ColladaLoader::basePath("/", "http://example/a/") == "/");
}

TEST_CASE("ColladaLoader's TGA guard can never fail") {
    // The name it tests is an imported module and therefore always a value, so the branch is
    // taken every time and the test changes nothing.
    REQUIRE(ColladaLoader::tgaGuardIsAlwaysTrue());
}