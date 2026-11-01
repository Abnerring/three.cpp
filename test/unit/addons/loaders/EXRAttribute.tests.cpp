#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

TEST_CASE("EXRLoader reads a null terminated string") {
    // The null ends the string and is consumed with it.
    std::size_t offset = 0;
    const std::vector<std::uint8_t> bytes{'a', 'b', 0, 'c'};
    REQUIRE(EXRLoader::readNullTerminated(bytes, offset) == "ab");
    REQUIRE(offset == 3);
    // The next string begins where that one ended.
    REQUIRE(EXRLoader::readNullTerminated(bytes, offset) == "c");
    REQUIRE(offset == 4);
}

TEST_CASE("EXRLoader reads an empty null terminated string") {
    // A null in the first place is the empty string, which is how a header ends.
    std::size_t offset = 0;
    const std::vector<std::uint8_t> bytes{0, 'a', 0};
    REQUIRE(EXRLoader::readNullTerminated(bytes, offset).empty());
    REQUIRE(offset == 1);
    // An empty buffer reads nothing and leaves the offset alone.
    std::size_t at = 0;
    REQUIRE(EXRLoader::readNullTerminated({}, at).empty());
    REQUIRE(at == 0);
}

TEST_CASE("EXRLoader reads past the end when there is no null") {
    // A string with no terminator runs to the end of what is there.
    std::size_t offset = 0;
    const std::vector<std::uint8_t> bytes{'a', 'b', 'c'};
    REQUIRE(EXRLoader::readNullTerminated(bytes, offset) == "abc");
    REQUIRE(offset == 3);
}

TEST_CASE("EXRLoader needs one attribute for a part") {
    // A part that listed nothing comes back as nothing.
    REQUIRE_FALSE(EXRLoader::partHasAttributes(0));
    // One or more is a part, whatever became of them.
    REQUIRE(EXRLoader::partHasAttributes(1));
    REQUIRE(EXRLoader::partHasAttributes(5));
}

TEST_CASE("EXRLoader warns about an unknown attribute type") {
    // The type is quoted in the message.
    REQUIRE(EXRLoader::unknownAttributeWarning("box2i") ==
            "THREE.EXRLoader: Skipped unknown header attribute type 'box2i'.");
    REQUIRE(EXRLoader::unknownAttributeWarning("") ==
            "THREE.EXRLoader: Skipped unknown header attribute type ''.");
    // A type with a space keeps it.
    REQUIRE(EXRLoader::unknownAttributeWarning("odd type") ==
            "THREE.EXRLoader: Skipped unknown header attribute type 'odd type'.");
}