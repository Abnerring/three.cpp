#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <array>
#include <string>

TEST_CASE("GLTFLoader renames a texture transform field") {
    // The coordinate becomes the channel and the scale becomes the repeat.
    REQUIRE(GLTFLoader::textureTransformField("texCoord") == "channel");
    REQUIRE(GLTFLoader::textureTransformField("scale") == "repeat");
    // The offset and the rotation keep their names.
    REQUIRE(GLTFLoader::textureTransformField("offset") == "offset");
    REQUIRE(GLTFLoader::textureTransformField("rotation") == "rotation");
    // Anything else names nothing.
    REQUIRE(GLTFLoader::textureTransformField("matrix").empty());
    REQUIRE(GLTFLoader::textureTransformField("").empty());
}

TEST_CASE("GLTFLoader leaves a texture alone when a transform asks nothing") {
    // No coordinate, no offset, no rotation, no scale: the texture need not be copied.
    REQUIRE_FALSE(GLTFLoader::textureTransformApplies(false, 0, 0, false, false, false));
    // A coordinate that matches the texture's own channel asks nothing either.
    REQUIRE_FALSE(GLTFLoader::textureTransformApplies(true, 1, 1, false, false, false));
}

TEST_CASE("GLTFLoader copies a texture when a transform asks something") {
    // A different channel is a change.
    REQUIRE(GLTFLoader::textureTransformApplies(true, 1, 0, false, false, false));
    // As is any one of the other three.
    REQUIRE(GLTFLoader::textureTransformApplies(false, 0, 0, true, false, false));
    REQUIRE(GLTFLoader::textureTransformApplies(false, 0, 0, false, true, false));
    REQUIRE(GLTFLoader::textureTransformApplies(false, 0, 0, false, false, true));
}

TEST_CASE("GLTFLoader writes the transform matrix in glTF order") {
    // No scale, no offset, no rotation is the identity.
    const auto identity = GLTFLoader::textureTransformMatrix({1, 1}, {0, 0}, 0.0);
    REQUIRE(identity.a == 1.0);
    REQUIRE(identity.b == 0.0);
    REQUIRE(identity.c == 0.0);
    REQUIRE(identity.d == 1.0);
    REQUIRE(identity.e == 0.0);
    REQUIRE(identity.f == 0.0);

    // A quarter turn: the x axis lands on the negative y and the y axis on the positive x.
    const double quarter = 3.14159265358979323846 / 2.0;
    const auto turned = GLTFLoader::textureTransformMatrix({1, 1}, {0, 0}, quarter);
    REQUIRE(turned.a == Approx(0.0).margin(1e-12));
    REQUIRE(turned.b == Approx(-1.0).epsilon(1e-12));
    REQUIRE(turned.c == Approx(1.0).epsilon(1e-12));
    REQUIRE(turned.d == Approx(0.0).margin(1e-12));

    // The two axes take their own scale factors, and the offset is the translation.
    const auto scaled = GLTFLoader::textureTransformMatrix({2, 3}, {4, 5}, 0.0);
    REQUIRE(scaled.a == 2.0);
    REQUIRE(scaled.d == 3.0);
    REQUIRE(scaled.e == 4.0);
    REQUIRE(scaled.f == 5.0);
}

TEST_CASE("GLTFLoader scales the two axes apart") {
    // A quarter turn with different scales shows the factors do not cross over.
    const double quarter = 3.14159265358979323846 / 2.0;
    const auto matrix = GLTFLoader::textureTransformMatrix({2, 3}, {0, 0}, quarter);
    REQUIRE(matrix.a == Approx(0.0).margin(1e-12));
    // The second column's first number takes the x factor, not the y one.
    REQUIRE(matrix.b == Approx(-2.0).epsilon(1e-12));
    REQUIRE(matrix.c == Approx(3.0).epsilon(1e-12));
    REQUIRE(matrix.d == Approx(0.0).margin(1e-12));
}