#include "loaders/AMFLoader.h"

#include "loaders/XmlDocument.h"

#include "catch.hpp"

#include <string>

namespace {
const char* kAmf =
    "<amf unit=\"millimeter\">"
    "  <object id=\"1\">"
    "    <mesh>"
    "      <vertices>"
    "        <vertex><coordinates><x>0</x><y>0</y><z>0</z></coordinates>"
    "          <normal><nx>0</nx><ny>0</ny><nz>1</nz></normal></vertex>"
    "        <vertex><coordinates><x>1</x><y>2</y><z>3</z></coordinates></vertex>"
    "      </vertices>"
    "      <volume materialid=\"9\">"
    "        <metadata type=\"name\">Body</metadata>"
    "        <triangle><v1>0</v1><v2>1</v2><v3>2</v3></triangle>"
    "        <triangle><v1>2</v1><v2>1</v2><v3>0</v3></triangle>"
    "      </volume>"
    "    </mesh>"
    "  </object>"
    "</amf>";
}  // namespace

TEST_CASE("AMFLoader reads the vertices of a mesh") {
    const auto root = XmlDocument::parse(kAmf);
    const auto mesh = root->child("object")->child("mesh");
    const auto vertices = AMFLoader::parseMeshVertices(*mesh->child("vertices"));
    // Two vertices, so six components.
    REQUIRE(vertices.positions.size() == 6);
    REQUIRE(vertices.positions[0] == 0.0);
    REQUIRE(vertices.positions[3] == 1.0);
    REQUIRE(vertices.positions[4] == 2.0);
    REQUIRE(vertices.positions[5] == 3.0);
    // Only the first vertex carried a normal.
    REQUIRE(vertices.normals.size() == 3);
    REQUIRE(vertices.normals[2] == 1.0);
}

TEST_CASE("AMFLoader reads a volume with its name and triangles") {
    const auto root = XmlDocument::parse(kAmf);
    const auto mesh = root->child("object")->child("mesh");
    const auto volume = AMFLoader::parseMeshVolume(*mesh->child("volume"));
    REQUIRE(volume.name == "Body");
    REQUIRE(volume.materialId == "9");
    REQUIRE(volume.triangles.size() == 6);
    REQUIRE(volume.triangles[0] == 0);
    REQUIRE(volume.triangles[5] == 0);
}

TEST_CASE("AMFLoader leaves the name empty without a name metadata") {
    const auto root = XmlDocument::parse(
        "<amf><object><mesh><volume><triangle><v1>1</v1><v2>2</v2><v3>3</v3></triangle>"
        "<metadata type=\"other\">ignored</metadata></volume></mesh></object></amf>");
    const auto mesh = root->child("object")->child("mesh");
    const auto volume = AMFLoader::parseMeshVolume(*mesh->child("volume"));
    REQUIRE(volume.name.empty());
    // A volume without the attribute has no material id either.
    REQUIRE(volume.materialId.empty());
    REQUIRE(volume.triangles.size() == 3);
}

TEST_CASE("AMFLoader handles a mesh without vertices") {
    const auto root = XmlDocument::parse("<amf><object><mesh><vertices/></mesh></object></amf>");
    const auto mesh = root->child("object")->child("mesh");
    const auto vertices = AMFLoader::parseMeshVertices(*mesh->child("vertices"));
    REQUIRE(vertices.positions.empty());
    REQUIRE(vertices.normals.empty());
}
