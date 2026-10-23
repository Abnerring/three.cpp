#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("GLTFLoader assigns an object extras") {
    // An object merges into the user data.
    REQUIRE(GLTFLoader::extrasDisposition(true, "object") ==
            GLTFLoader::ExtrasDisposition::Assign);
    // No extras at all is not the same as an empty object: nothing happens either way, but the
    // first is decided before the type is looked at.
    REQUIRE(GLTFLoader::extrasDisposition(false, "object") ==
            GLTFLoader::ExtrasDisposition::None);
}

TEST_CASE("GLTFLoader warns about a primitive extras") {
    // The four types that are not objects all warn.
    REQUIRE(GLTFLoader::extrasDisposition(true, "number") ==
            GLTFLoader::ExtrasDisposition::Warn);
    REQUIRE(GLTFLoader::extrasDisposition(true, "string") ==
            GLTFLoader::ExtrasDisposition::Warn);
    REQUIRE(GLTFLoader::extrasDisposition(true, "boolean") ==
            GLTFLoader::ExtrasDisposition::Warn);
    REQUIRE(GLTFLoader::extrasDisposition(true, "undefined") ==
            GLTFLoader::ExtrasDisposition::Warn);
}

TEST_CASE("GLTFLoader merges through the two type traps") {
    // A null value reports its type as an object, so it merges rather than warning; merging
    // nothing is a no-op rather than an error.
    REQUIRE(GLTFLoader::extrasDisposition(true, "object") ==
            GLTFLoader::ExtrasDisposition::Assign);
    // An array reports the same type and therefore merges too, which copies its indices as
    // numbered keys rather than warning about it.
    REQUIRE(GLTFLoader::extrasDisposition(true, "object") ==
            GLTFLoader::ExtrasDisposition::Assign);
}

TEST_CASE("GLTFLoader builds the extras warning") {
    // The message names the loader and the value it dropped.
    REQUIRE(GLTFLoader::extrasWarning("5") ==
            "THREE.GLTFLoader: Ignoring primitive type .extras, 5");
    REQUIRE(GLTFLoader::extrasWarning("true") ==
            "THREE.GLTFLoader: Ignoring primitive type .extras, true");
    // An empty value still produces the whole message around it.
    REQUIRE(GLTFLoader::extrasWarning("") ==
            "THREE.GLTFLoader: Ignoring primitive type .extras, ");
}