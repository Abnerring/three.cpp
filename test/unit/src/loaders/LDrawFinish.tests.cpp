#include "loaders/LDrawLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("LDrawLoader reads a surface finish") {
    // The five words the format defines.
    REQUIRE(LDrawLoader::finishTypeFromToken("CHROME") == LDrawLoader::FinishType::Chrome);
    REQUIRE(LDrawLoader::finishTypeFromToken("PEARLESCENT") == LDrawLoader::FinishType::Pearlescent);
    REQUIRE(LDrawLoader::finishTypeFromToken("RUBBER") == LDrawLoader::FinishType::Rubber);
    REQUIRE(LDrawLoader::finishTypeFromToken("MATTE_METALLIC") ==
            LDrawLoader::FinishType::MatteMetallic);
    REQUIRE(LDrawLoader::finishTypeFromToken("METAL") == LDrawLoader::FinishType::Metal);
}

TEST_CASE("LDrawLoader ignores the case of a finish word") {
    // The comparison is on the upper cased word.
    REQUIRE(LDrawLoader::finishTypeFromToken("chrome") == LDrawLoader::FinishType::Chrome);
    REQUIRE(LDrawLoader::finishTypeFromToken("Chrome") == LDrawLoader::FinishType::Chrome);
    REQUIRE(LDrawLoader::finishTypeFromToken("mEtAl") == LDrawLoader::FinishType::Metal);
}

TEST_CASE("LDrawLoader gives an unknown word the plain finish") {
    // A colour with no finish word arrives as an empty string.
    REQUIRE(LDrawLoader::finishTypeFromToken("") == LDrawLoader::FinishType::Default);
    // A word that names nothing is the same rather than an error.
    REQUIRE(LDrawLoader::finishTypeFromToken("GLITTER") == LDrawLoader::FinishType::Default);
    // The near miss of a real word is not accepted.
    REQUIRE(LDrawLoader::finishTypeFromToken("MATTE") == LDrawLoader::FinishType::Default);
    REQUIRE(LDrawLoader::finishTypeFromToken("MATTE METALLIC") == LDrawLoader::FinishType::Default);
}

TEST_CASE("LDrawLoader keeps the finishes apart") {
    // Every real word gives a different value, so none of them is an alias of another.
    const auto chrome = LDrawLoader::finishTypeFromToken("CHROME");
    const auto pearl = LDrawLoader::finishTypeFromToken("PEARLESCENT");
    const auto rubber = LDrawLoader::finishTypeFromToken("RUBBER");
    const auto matte = LDrawLoader::finishTypeFromToken("MATTE_METALLIC");
    const auto metal = LDrawLoader::finishTypeFromToken("METAL");
    REQUIRE_FALSE(chrome == pearl);
    REQUIRE_FALSE(pearl == rubber);
    REQUIRE_FALSE(rubber == matte);
    REQUIRE_FALSE(matte == metal);
    REQUIRE_FALSE(metal == chrome);
    // And none of them is the plain finish.
    REQUIRE_FALSE(chrome == LDrawLoader::FinishType::Default);
}
