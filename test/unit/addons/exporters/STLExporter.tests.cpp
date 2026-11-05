#include "exporters/STLExporter.h"

#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "geometries/BoxGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "math/Matrix4.h"
#include "objects/Mesh.h"

#include "catch.hpp"

#include <memory>
#include <stdexcept>

TEST_CASE("STLExporter writes the ascii facet of a triangle") {
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<BufferAttribute>(
                                           std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0}, 3));
    auto mesh = std::make_shared<Mesh>(geometry, std::make_shared<MeshBasicMaterial>());

    Object3D scene;
    scene.add(mesh);

    STLExporter exporter;
    const std::string ascii = exporter.parse(scene);
    REQUIRE(ascii ==
            "solid exported\n"
            "\tfacet normal 0 0 1\n"
            "\t\touter loop\n"
            "\t\t\tvertex 0 0 0\n"
            "\t\t\tvertex 1 0 0\n"
            "\t\t\tvertex 0 1 0\n"
            "\t\tendloop\n"
            "\tendfacet\n"
            "endsolid exported\n");
}

TEST_CASE("STLExporter walks the index and applies the world matrix") {
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<BufferAttribute>(
                                           std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0}, 3));
    geometry->setIndex(std::make_shared<BufferAttribute>(std::vector<unsigned int>{0, 1, 2}, 1));
    auto mesh = std::make_shared<Mesh>(geometry, std::make_shared<MeshBasicMaterial>());
    mesh->matrixWorld.makeTranslation(1, 2, 3);

    Object3D scene;
    scene.add(mesh);

    STLExporter exporter;
    const std::string ascii = exporter.parse(scene);
    REQUIRE(ascii.find("\t\t\tvertex 1 2 3\n") != std::string::npos);
    REQUIRE(ascii.find("\t\t\tvertex 2 2 3\n") != std::string::npos);
    REQUIRE(ascii.find("\t\t\tvertex 1 3 3\n") != std::string::npos);
}

TEST_CASE("STLExporter writes the binary layout for one triangle") {
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<BufferAttribute>(
                                           std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0}, 3));
    auto mesh = std::make_shared<Mesh>(geometry, std::make_shared<MeshBasicMaterial>());
    Object3D scene;
    scene.add(mesh);

    const std::string bytes = STLExporter().parse(scene, true);
    // 80 byte header + uint32 triangle count + one triangle of 12 float32 and 2 attribute bytes.
    REQUIRE(bytes.size() == 134);
    REQUIRE(bytes.substr(0, 80) == std::string(80, '\0'));
    REQUIRE(bytes.substr(80, 4) == std::string("\x01\x00\x00\x00", 4));
    // Normal (0, 0, 1) and the three vertices (0,0,0), (1,0,0), (0,1,0).
    REQUIRE(bytes.substr(84, 12) ==
            std::string("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x3F", 12));
    REQUIRE(bytes.substr(96, 12) == std::string(12, '\0'));
    REQUIRE(bytes.substr(108, 12) ==
            std::string("\x00\x00\x80\x3F\x00\x00\x00\x00\x00\x00\x00\x00", 12));
    REQUIRE(bytes.substr(120, 12) ==
            std::string("\x00\x00\x00\x00\x00\x00\x80\x3F\x00\x00\x00\x00", 12));
    REQUIRE(bytes.substr(132, 2) == std::string(2, '\0'));
}
