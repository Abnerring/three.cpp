#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("GLTFLoader builds an attributes key") {
    // The names are sorted, so the order they are given in does not matter.
    const std::vector<GLTFLoader::Attribute> attributes{{"POSITION", 0}, {"NORMAL", 1}};
    REQUIRE(GLTFLoader::createAttributesKey(attributes) == "NORMAL:1;POSITION:0;");
    // The same two in the other order give the same key.
    const std::vector<GLTFLoader::Attribute> reversed{{"NORMAL", 1}, {"POSITION", 0}};
    REQUIRE(GLTFLoader::createAttributesKey(reversed) ==
            GLTFLoader::createAttributesKey(attributes));
    // An empty set gives nothing at all.
    REQUIRE(GLTFLoader::createAttributesKey({}).empty());
    // A single attribute has no separator before it.
    REQUIRE(GLTFLoader::createAttributesKey({{"TEXCOORD_0", 3}}) == "TEXCOORD_0:3;");
}

TEST_CASE("GLTFLoader scales a normalised accessor") {
    // One over the largest value each type holds.
    REQUIRE(GLTFLoader::normalizedComponentScale("int8") == Approx(1.0 / 127.0).epsilon(1e-15));
    REQUIRE(GLTFLoader::normalizedComponentScale("uint8") == Approx(1.0 / 255.0).epsilon(1e-15));
    REQUIRE(GLTFLoader::normalizedComponentScale("int16") ==
            Approx(1.0 / 32767.0).epsilon(1e-15));
    REQUIRE(GLTFLoader::normalizedComponentScale("uint16") ==
            Approx(1.0 / 65535.0).epsilon(1e-15));
    // The name is compared without regard to case.
    REQUIRE(GLTFLoader::normalizedComponentScale("INT8") == Approx(1.0 / 127.0).epsilon(1e-15));
    // A type that cannot be normalised is refused.
    REQUIRE_THROWS_WITH(GLTFLoader::normalizedComponentScale("float32"),
                        "THREE.GLTFLoader: Unsupported normalized accessor component type.");
}

TEST_CASE("GLTFLoader reads a mime type from an image reference") {
    // The extension decides.
    REQUIRE(GLTFLoader::imageMimeType("image.jpg") == "image/jpeg");
    REQUIRE(GLTFLoader::imageMimeType("image.jpeg") == "image/jpeg");
    REQUIRE(GLTFLoader::imageMimeType("image.webp") == "image/webp");
    REQUIRE(GLTFLoader::imageMimeType("image.ktx2") == "image/ktx2");
    // The case does not matter.
    REQUIRE(GLTFLoader::imageMimeType("IMAGE.JPG") == "image/jpeg");
    // Anything else is a png.
    REQUIRE(GLTFLoader::imageMimeType("image.png") == "image/png");
    REQUIRE(GLTFLoader::imageMimeType("image") == "image/png");
    REQUIRE(GLTFLoader::imageMimeType("") == "image/png");
}

TEST_CASE("GLTFLoader reads a mime type through a query string") {
    // A query after the extension does not hide it.
    REQUIRE(GLTFLoader::imageMimeType("image.jpg?v=2") == "image/jpeg");
    REQUIRE(GLTFLoader::imageMimeType("image.webp?") == "image/webp");
}

TEST_CASE("GLTFLoader reads a mime type from a data uri") {
    // A data uri names its own type before any extension could.
    REQUIRE(GLTFLoader::imageMimeType("data:image/jpeg;base64,AAAA") == "image/jpeg");
    REQUIRE(GLTFLoader::imageMimeType("data:image/webp;base64,AAAA") == "image/webp");
    REQUIRE(GLTFLoader::imageMimeType("data:image/ktx2;base64,AAAA") == "image/ktx2");
    // A data uri of another type falls through to the extension rule.
    REQUIRE(GLTFLoader::imageMimeType("data:image/png;base64,AAAA") == "image/png");
}

TEST_CASE("GLTFLoader names the binary header") {
    // The magic is four characters and the header twelve bytes.
    REQUIRE(std::string(GLTFLoader::kBinaryMagic) == "glTF");
    REQUIRE(GLTFLoader::kBinaryHeaderLength == 12);
    // The two chunk kinds are the words JSON and BIN read little endian.
    REQUIRE(GLTFLoader::kChunkJson == 0x4E4F534A);
    REQUIRE(GLTFLoader::kChunkBin == 0x004E4942);
}