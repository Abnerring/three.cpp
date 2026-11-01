#include "loaders/PLYLoader.h"

#include "catch.hpp"

#include <string>

namespace {
const char* kHeader =
    "ply\n"
    "format ascii 1.0\n"
    "comment made by a test\n"
    "element vertex 3\n"
    "property float x\n"
    "property float y\n"
    "property float z\n"
    "element face 1\n"
    "property list uchar int vertex_indices\n"
    "obj_info hello\n"
    "end_header\n";
}

TEST_CASE("PLYLoader reads the format line and comments") {
    PLYLoader loader;
    const auto header = loader.parseHeader(kHeader);
    REQUIRE(header.format.has_value());
    REQUIRE(header.format.value() == "ascii");
    REQUIRE(header.version.has_value());
    REQUIRE(header.version.value() == "1.0");
    REQUIRE(header.comments.size() == 1);
    REQUIRE(header.comments[0] == "made by a test");
    REQUIRE(header.objInfo == "hello");
}

TEST_CASE("PLYLoader groups properties under their element") {
    PLYLoader loader;
    const auto header = loader.parseHeader(kHeader);
    REQUIRE(header.elements.size() == 2);
    REQUIRE(header.elements[0].name == "vertex");
    REQUIRE(header.elements[0].count == 3);
    REQUIRE(header.elements[0].properties.size() == 3);
    REQUIRE(header.elements[0].properties[0].name == "x");
    REQUIRE(header.elements[0].properties[0].type == "float");
    REQUIRE_FALSE(header.elements[0].properties[0].isList());

    REQUIRE(header.elements[1].name == "face");
    REQUIRE(header.elements[1].count == 1);
    const auto& list = header.elements[1].properties[0];
    REQUIRE(list.isList());
    REQUIRE(list.countType == "uchar");
    REQUIRE(list.itemType == "int");
    REQUIRE(list.name == "vertex_indices");
}

TEST_CASE("PLYLoader applies the property name mapping") {
    PLYLoader loader;
    loader.propertyNameMapping["vertex_indices"] = "vertex_index";
    const auto header = loader.parseHeader(kHeader);
    REQUIRE(header.elements[1].properties[0].name == "vertex_index");
    // Names that are not in the map are left alone.
    REQUIRE(header.elements[0].properties[0].name == "x");
}

TEST_CASE("PLYLoader rejects a header without the magic or the terminator") {
    PLYLoader loader;
    REQUIRE_THROWS_WITH(loader.parseHeader("format ascii 1.0\nend_header\n"),
                        "THREE.PLYLoader: Missing ply header");
    REQUIRE_THROWS_WITH(loader.parseHeader("ply\nformat ascii 1.0\n"),
                        "THREE.PLYLoader: Missing ply header terminator");
}
