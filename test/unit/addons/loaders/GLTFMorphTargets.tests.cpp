#include "loaders/GLTFLoader.h"

#include "catch.hpp"

TEST_CASE("GLTFLoader treats a false valued extension as absent") {
    // A value that is not true counts as absent even though the key exists.
    REQUIRE_FALSE(GLTFLoader::materialExtensionPresent(true, false));
    // A true value counts as present.
    REQUIRE(GLTFLoader::materialExtensionPresent(true, true));
    // No extensions object carries nothing whatever the value would be.
    REQUIRE_FALSE(GLTFLoader::materialExtensionPresent(false, true));
}

TEST_CASE("GLTFLoader applies target names only when they line up") {
    // An array of exactly the influence count applies.
    REQUIRE(GLTFLoader::targetNamesApply(true, 3, 3));
    // Something that is not an array does not.
    REQUIRE_FALSE(GLTFLoader::targetNamesApply(false, 3, 3));
    // A length that does not match does not either, in either direction.
    REQUIRE_FALSE(GLTFLoader::targetNamesApply(true, 2, 3));
    REQUIRE_FALSE(GLTFLoader::targetNamesApply(true, 4, 3));
    // An empty pair does line up.
    REQUIRE(GLTFLoader::targetNamesApply(true, 0, 0));
}

TEST_CASE("GLTFLoader writes the weights that fit") {
    // A short list fills the first few and leaves the rest.
    REQUIRE(GLTFLoader::weightsWritten(3, 5) == 3);
    // A long list is cut at the influence count.
    REQUIRE(GLTFLoader::weightsWritten(5, 3) == 3);
    // Equal lists write them all.
    REQUIRE(GLTFLoader::weightsWritten(4, 4) == 4);
    // No influences means nothing is written.
    REQUIRE(GLTFLoader::weightsWritten(4, 0) == 0);
}