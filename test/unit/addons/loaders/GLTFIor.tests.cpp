#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <vector>

TEST_CASE("GLTFLoader starts an index of refraction at glass") {
    // The default is one and a half, not one.
    REQUIRE(GLTFLoader::indexOfRefraction(false, 2.0) == 1.5);
    // A written value is taken.
    REQUIRE(GLTFLoader::indexOfRefraction(true, 1.33) == Approx(1.33).epsilon(1e-12));
    // A written zero is kept, because the source tests against undefined rather than falsiness.
    REQUIRE(GLTFLoader::indexOfRefraction(true, 0.0) == 0.0);
    // The default is not one, which would mean no bending at all.
    REQUIRE(GLTFLoader::indexOfRefraction(false, 0.0) != 1.0);
}

TEST_CASE("GLTFLoader starts a specular intensity at one") {
    // Fully specular by default.
    REQUIRE(GLTFLoader::specularIntensity(false, 5.0) == 1.0);
    // A written value is taken, including zero.
    REQUIRE(GLTFLoader::specularIntensity(true, 0.5) == 0.5);
    REQUIRE(GLTFLoader::specularIntensity(true, 0.0) == 0.0);
}

TEST_CASE("GLTFLoader starts a specular colour white") {
    // White by default.
    const auto white = GLTFLoader::specularColour(false, {});
    REQUIRE(white[0] == 1.0);
    REQUIRE(white[1] == 1.0);
    REQUIRE(white[2] == 1.0);
    // As written otherwise, with no conversion.
    const auto tinted = GLTFLoader::specularColour(true, {0.5, 0.25, 0.125});
    REQUIRE(tinted[0] == 0.5);
    REQUIRE(tinted[2] == 0.125);
    // Too few numbers falls back to white rather than reading past the end.
    const auto short_ = GLTFLoader::specularColour(true, {0.5});
    REQUIRE(short_[0] == 1.0);
}