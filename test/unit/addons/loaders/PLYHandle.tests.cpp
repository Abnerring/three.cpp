#include "loaders/PLYLoader.h"

#include "catch.hpp"

#include <vector>

TEST_CASE("PLYLoader triangulates faces by the specification rule") {
    PLYLoader::Buffer buffer;
    PLYLoader::handleFace(buffer, {0, 1, 2});
    REQUIRE(buffer.indices.size() == 3);
    REQUIRE(buffer.indices[0] == 0);
    REQUIRE(buffer.indices[2] == 2);

    PLYLoader::Buffer quad;
    // A quad becomes 0,1,2 followed by 0,2,3 rather than 1,2,3.
    PLYLoader::handleFace(quad, {4, 5, 6, 7});
    REQUIRE(quad.indices.size() == 6);
    REQUIRE(quad.indices[0] == 4);
    REQUIRE(quad.indices[1] == 5);
    REQUIRE(quad.indices[2] == 6);
    REQUIRE(quad.indices[3] == 4);
    REQUIRE(quad.indices[4] == 6);
    REQUIRE(quad.indices[5] == 7);
}

TEST_CASE("PLYLoader ignores faces that are neither triangles nor quads") {
    PLYLoader::Buffer buffer;
    PLYLoader::handleFace(buffer, {0, 1});
    REQUIRE(buffer.indices.empty());
    PLYLoader::handleFace(buffer, {0, 1, 2, 3, 4});
    REQUIRE(buffer.indices.empty());
}

TEST_CASE("PLYLoader appends vertex streams in the descriptor order") {
    PLYLoader::AttributeDescriptor descriptor;
    descriptor.positionUsage = true;
    descriptor.positionNames[0] = "x";
    descriptor.positionNames[1] = "y";
    descriptor.positionNames[2] = "z";
    descriptor.uvUsage = true;
    descriptor.uvNames[0] = "s";
    descriptor.uvNames[1] = "t";

    PLYLoader::ElementRecord element;
    element["x"] = 1.0;
    element["y"] = 2.0;
    element["z"] = 3.0;
    element["s"] = 0.25;
    element["t"] = 0.75;

    PLYLoader::Buffer buffer;
    PLYLoader::handleVertex(buffer, element, descriptor);
    REQUIRE(buffer.vertices.size() == 3);
    REQUIRE(buffer.vertices[0] == 1.0);
    REQUIRE(buffer.vertices[2] == 3.0);
    REQUIRE(buffer.uvs.size() == 2);
    REQUIRE(buffer.uvs[1] == 0.75);
    // A stream the descriptor does not enable stays empty.
    REQUIRE(buffer.normals.empty());
    REQUIRE(buffer.colors.empty());
}

TEST_CASE("PLYLoader rounds integer colours back to their storage width") {
    PLYLoader::AttributeDescriptor descriptor;
    descriptor.colorUsage = true;
    descriptor.colorNames[0] = "red";
    descriptor.colorNames[1] = "green";
    descriptor.colorNames[2] = "blue";
    descriptor.colorType = "uchar";

    PLYLoader::ElementRecord element;
    element["red"] = 255.0;
    element["green"] = 128.0;
    element["blue"] = 0.0;

    PLYLoader::Buffer buffer;
    PLYLoader::handleVertex(buffer, element, descriptor);
    REQUIRE(buffer.colors.size() == 3);
    // The value is scaled to one, converted out of sRGB and rounded back to 0 to 255, so the
    // extremes survive and the middle value stays in range.
    REQUIRE(buffer.colors[0] == Approx(255.0).margin(1.0));
    REQUIRE(buffer.colors[2] == 0.0);
    REQUIRE(buffer.colors[1] > 0.0);
    REQUIRE(buffer.colors[1] < 255.0);
}
