#include "nodes/utils/MaxMipLevelNode.h"

#include "catch.hpp"

#include <cmath>
#include <string>

TEST_CASE("MaxMipLevel follows the larger dimension") {
    // A square texture's level is the logarithm of its side.
    REQUIRE(MaxMipLevel::level(16.0, 16.0) == Approx(4.0).epsilon(1e-12));
    REQUIRE(MaxMipLevel::level(256.0, 256.0) == Approx(8.0).epsilon(1e-12));
    REQUIRE(MaxMipLevel::level(1.0, 1.0) == Approx(0.0).epsilon(1e-12));
    // A wide thin texture takes its level from the width, not the height: sixteen by four has
    // four levels, not two. This is the part a reader is most likely to get wrong.
    REQUIRE(MaxMipLevel::level(16.0, 4.0) == Approx(4.0).epsilon(1e-12));
    REQUIRE(MaxMipLevel::level(16.0, 4.0) != Approx(2.0).epsilon(1e-12));
    // And a tall thin one from its height.
    REQUIRE(MaxMipLevel::level(4.0, 16.0) == Approx(4.0).epsilon(1e-12));
    // The order of the two arguments therefore does not matter.
    REQUIRE(MaxMipLevel::level(16.0, 4.0) == Approx(MaxMipLevel::level(4.0, 16.0)).epsilon(1e-12));
}

TEST_CASE("MaxMipLevel's usability test is not a truth test") {
    // The check is against the absence of a width, so a width of nought passes it; a truth test
    // would refuse that case.
    REQUIRE(MaxMipLevel::usable(true));
    REQUIRE_FALSE(MaxMipLevel::usable(false));
    // With a width of nought the level comes from the height rather than failing.
    REQUIRE(MaxMipLevel::levelWithZeroWidth(256.0) == Approx(8.0).epsilon(1e-12));
    REQUIRE(MaxMipLevel::level(0.0, 256.0) == Approx(8.0).epsilon(1e-12));
    // Both nought gives minus infinity rather than a finite answer, which is what the logarithm
    // of nought is.
    REQUIRE(std::isinf(MaxMipLevel::level(0.0, 0.0)));
    REQUIRE(MaxMipLevel::level(0.0, 0.0) < 0.0);
}

TEST_CASE("MaxMipLevel refreshes once a frame") {
    // Unlike the screen and viewport nodes, which refresh once a render, this one refreshes once
    // a frame.
    REQUIRE(std::string(MaxMipLevel::kUpdateType) == "FRAME");
    REQUIRE(std::string(MaxMipLevel::kUpdateType) != "RENDER");
    // And it starts at nought before any update has run.
    REQUIRE(MaxMipLevel::kInitialValue == 0.0);
    REQUIRE(std::string(MaxMipLevel::kTypeName) == "MaxMipLevelNode");
}

TEST_CASE("MaxMipLevel finds an image in one of three places") {
    using Source = MaxMipLevel::ImageSource;
    // A list of images with a first one that holds an image gives that image.
    REQUIRE(MaxMipLevel::imageSource(true, true, true) == Source::FirstImageOrItself);
    // A list whose first entry is not an image gives the entry itself.
    REQUIRE(MaxMipLevel::imageSource(true, false, false) == Source::FirstImage);
    // A first entry that is present but does not hold an image gives the entry itself.
    REQUIRE(MaxMipLevel::imageSource(true, true, false) == Source::FirstImage);
    // No list at all falls back to the texture's own image.
    REQUIRE(MaxMipLevel::imageSource(false, false, false) == Source::TextureImage);
    // The three sources are three different answers, so none is another by accident.
    REQUIRE(Source::FirstImage != Source::FirstImageOrItself);
    REQUIRE(Source::FirstImage != Source::TextureImage);
}
