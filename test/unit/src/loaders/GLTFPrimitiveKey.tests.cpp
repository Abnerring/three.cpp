#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

namespace {
const std::vector<GLTFLoader::Attribute> kPN{{"POSITION", 0}, {"NORMAL", 1}};
}  // namespace

TEST_CASE("GLTFLoader keys a plain primitive") {
    // The index accessor, the attributes and the mode, in that order.
    const auto key = GLTFLoader::createPrimitiveKey(false, 0, 0, {}, 5, kPN, 4, {});
    REQUIRE(key == "5:NORMAL:1;POSITION:0;:4");
}

TEST_CASE("GLTFLoader keys a draco primitive") {
    // A compressed primitive is keyed by the extension's own numbers.
    const auto key = GLTFLoader::createPrimitiveKey(true, 7, 8, kPN, 5, kPN, 4, {});
    REQUIRE(key == "draco:7:8:NORMAL:1;POSITION:0;");
    // The ordinary fields are not used in this form.
    const auto other = GLTFLoader::createPrimitiveKey(true, 7, 8, kPN, 99, kPN, 1, {});
    REQUIRE(key == other);
}

TEST_CASE("GLTFLoader adds a morph target to the key") {
    const std::vector<std::vector<GLTFLoader::Attribute>> one{{{ "POSITION", 9 }}};
    const auto key = GLTFLoader::createPrimitiveKey(false, 0, 0, {}, 5, kPN, 4, one);
    REQUIRE(key == "5:NORMAL:1;POSITION:0;:4:POSITION:9;");
    // Two targets add two pieces.
    const std::vector<std::vector<GLTFLoader::Attribute>> two{{{ "POSITION", 9 }},
                                                             {{"POSITION", 10}}};
    const auto both = GLTFLoader::createPrimitiveKey(false, 0, 0, {}, 5, kPN, 4, two);
    REQUIRE(both == "5:NORMAL:1;POSITION:0;:4:POSITION:9;:POSITION:10;");
}

TEST_CASE("GLTFLoader separates primitives that differ") {
    // Two primitives with the same shape but different index accessors.
    const auto a = GLTFLoader::createPrimitiveKey(false, 0, 0, {}, 5, kPN, 4, {});
    const auto b = GLTFLoader::createPrimitiveKey(false, 0, 0, {}, 6, kPN, 4, {});
    REQUIRE(a != b);
    // Two that differ only in a morph target.
    const std::vector<std::vector<GLTFLoader::Attribute>> target{{{ "POSITION", 9 }}};
    const auto withTarget = GLTFLoader::createPrimitiveKey(false, 0, 0, {}, 5, kPN, 4, target);
    REQUIRE(a != withTarget);
    // Two that differ only in the mode.
    const auto asPoints = GLTFLoader::createPrimitiveKey(false, 0, 0, {}, 5, kPN, 0, {});
    REQUIRE(a != asPoints);
}

TEST_CASE("GLTFLoader keys no targets as no suffix") {
    // An empty target list adds nothing at all.
    const auto none = GLTFLoader::createPrimitiveKey(false, 0, 0, {}, 5, kPN, 4, {});
    REQUIRE(none.back() == '4');
    REQUIRE(none.find("::") == std::string::npos);
}