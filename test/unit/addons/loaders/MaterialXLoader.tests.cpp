#include "loaders/MaterialXLoader.h"

#include "catch.hpp"

#include <string>

using DisposeEffect = MaterialXLoader::DisposeEffect;

TEST_CASE("MaterialXLoader takes the URL's directory when it has no path") {
    // The loader's own path wins when it is a value.
    REQUIRE(MaterialXLoader::resourcePath("models/", "http://example/a/asset.mtlx") == "models/");
    // Without one the URL's directory is used, keeping its trailing slash so it can be joined to
    // a name.
    REQUIRE(MaterialXLoader::resourcePath("", "http://example/a/asset.mtlx") == "http://example/a/");
    REQUIRE(MaterialXLoader::resourcePath("", "http://example/a/asset.mtlx").back() == '/');
    // A URL with no slash at all gives nothing.
    REQUIRE(MaterialXLoader::resourcePath("", "asset.mtlx").empty());
    REQUIRE(std::string(MaterialXLoader::kTypeName) == "MaterialXLoader");
}

TEST_CASE("MaterialXLoader's path test is for a value rather than for the empty string") {
    // A single character is a value and is kept, as it is in the two loaders beside this one.
    REQUIRE(MaterialXLoader::resourcePath("/", "http://example/a/") == "/");
    // The two answers differ for a path that is present, which is the point of the test.
    REQUIRE(MaterialXLoader::resourcePath("/", "http://example/a/") !=
            MaterialXLoader::resourcePath("", "http://example/a/"));
}

TEST_CASE("MaterialXLoader forgets its disposer after running it") {
    // With one kept it runs and is forgotten.
    REQUIRE(MaterialXLoader::disposeEffect(true) == DisposeEffect::RanAndForgot);
    // With none there is nothing to do, so a second disposal is harmless.
    REQUIRE(MaterialXLoader::disposeEffect(false) == DisposeEffect::Nothing);
    REQUIRE(MaterialXLoader::disposeEffect(true) != MaterialXLoader::disposeEffect(false));
}

TEST_CASE("MaterialXLoader makes two manager calls on a failure") {
    // The item is started before the file is asked for, so the manager counts the work from the
    // call rather than from the arrival of the bytes.
    REQUIRE(MaterialXLoader::reportsStart());
    // A failure tells the manager the item failed and then that it ended: two calls, the failure
    // first. The collada and font loaders make only the first, so the counts differ between
    // formats for a reason that has nothing to do with the formats.
    REQUIRE(MaterialXLoader::managerCallsOnFailure() == 2);
    REQUIRE(MaterialXLoader::errorBeforeEnd());
    REQUIRE(std::string(MaterialXLoader::kResponseType) == "arraybuffer");
}