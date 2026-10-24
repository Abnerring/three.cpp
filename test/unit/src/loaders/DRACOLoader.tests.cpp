#include "loaders/DRACOLoader.h"

#include "catch.hpp"

#include <string>

using DecoderPaths = DRACOLoader::DecoderPaths;

TEST_CASE("DRACOLoader maps three attribute names by capitals and one by hand") {
    // Three follow the rule that the decoder name is the key in capitals.
    REQUIRE(DRACOLoader::decoderAttributeID("position") == "POSITION");
    REQUIRE(DRACOLoader::decoderAttributeID("normal") == "NORMAL");
    REQUIRE(DRACOLoader::decoderAttributeID("color") == "COLOR");
    // The fourth does not: the coordinate attribute is called TEX_COORD rather than UV, so a
    // reader who assumed the rule everywhere would ask for a name the decoder does not have.
    REQUIRE(DRACOLoader::decoderAttributeID("uv") == "TEX_COORD");
    REQUIRE(DRACOLoader::decoderAttributeID("uv") != "UV");
    // A name outside the four has none at all.
    REQUIRE(DRACOLoader::decoderAttributeID("tangent").empty());
    REQUIRE(std::string(DRACOLoader::kTypeName) == "DRACOLoader");
}

TEST_CASE("DRACOLoader reads all four attributes as floats") {
    // Including the colour, which might have been expected to be bytes.
    for (const char* name : {"position", "normal", "color", "uv"}) {
        REQUIRE(DRACOLoader::decoderAttributeType(name) == "Float32Array");
    }
    REQUIRE(DRACOLoader::decoderAttributeType("color") != "Uint8Array");
    REQUIRE(DRACOLoader::decoderAttributeType("tangent").empty());
}

TEST_CASE("DRACOLoader numbers its first task one") {
    // The count of workers is four and the first task is numbered one, so a task numbered nought
    // does not exist and a caller counting from nought would be off by one.
    REQUIRE(DRACOLoader::kWorkerLimit == 4);
    REQUIRE(DRACOLoader::kFirstTaskId == 1);
    REQUIRE(DRACOLoader::kFirstTaskId != 0);
}

TEST_CASE("DRACOLoader's object form of the decoder path disables the fallback") {
    // The string form resolves all three paths.
    const DecoderPaths viaString = DRACOLoader::pathsForStringForm();
    REQUIRE(viaString.hasJs);
    REQUIRE(viaString.hasWasm);
    REQUIRE(viaString.hasDependencyJs);
    // The object form takes the two it names and sets the plain JavaScript decoder to nothing,
    // so the fallback is gone.
    const DecoderPaths viaObject = DRACOLoader::pathsForObjectForm();
    REQUIRE(viaObject.hasJs);
    REQUIRE(viaObject.hasWasm);
    REQUIRE_FALSE(viaObject.hasDependencyJs);
    REQUIRE_FALSE(DRACOLoader::objectFormKeepsFallback());
    // The two forms differ in exactly that one path.
    REQUIRE(viaString.hasDependencyJs != viaObject.hasDependencyJs);
    REQUIRE(viaString.hasJs == viaObject.hasJs);
}