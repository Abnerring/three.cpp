#include "loaders/AMFLoader.h"

#include "loaders/XmlDocument.h"

#include "catch.hpp"

#include <cstdint>
#include <string>

namespace {
const char* kAmf =
    "<amf unit=\"inch\">"
    "  <object id=\"1\">"
    "    <mesh>"
    "      <vertices>"
    "        <vertex><coordinates><x>0</x><y>0</y><z>0</z></coordinates></vertex>"
    "        <vertex><coordinates><x>1</x><y>2</y><z>3</z></coordinates></vertex>"
    "      </vertices>"
    "      <volume><triangle><v1>0</v1><v2>1</v2><v3>0</v3></triangle></volume>"
    "      <volume><triangle><v1>1</v1><v2>0</v2><v3>1</v3></triangle></volume>"
    "    </mesh>"
    "  </object>"
    "</amf>";
}  // namespace

TEST_CASE("AMFLoader applies the unit scale to the positions") {
    const auto root = XmlDocument::parse(kAmf);
    const auto meshes = AMFLoader::parseMeshes(*root, AMFLoader::unitScale("inch"));
    REQUIRE(meshes.size() == 1);
    // An inch is 25.4 millimetres, so the second vertex becomes 25.4 50.8 76.2.
    REQUIRE(meshes[0].vertices.positions[3] == Approx(25.4).epsilon(1e-9));
    REQUIRE(meshes[0].vertices.positions[4] == Approx(50.8).epsilon(1e-9));
    REQUIRE(meshes[0].vertices.positions[5] == Approx(76.2).epsilon(1e-9));
}

TEST_CASE("AMFLoader shares one vertex list across the volumes") {
    const auto root = XmlDocument::parse(kAmf);
    const auto meshes = AMFLoader::parseMeshes(*root, 1.0);
    REQUIRE(meshes[0].volumes.size() == 2);
    // The vertices belong to the mesh, so both volumes index the same list.
    REQUIRE(meshes[0].vertices.positions.size() == 6);
    REQUIRE(meshes[0].volumes[0].triangles.size() == 3);
    REQUIRE(meshes[0].volumes[1].triangles[0] == 1);
}

TEST_CASE("AMFLoader reads one mesh per mesh element") {
    const auto root = XmlDocument::parse(
        "<amf><object><mesh><vertices/></mesh><mesh><vertices/></mesh></object></amf>");
    REQUIRE(AMFLoader::parseMeshes(*root, 1.0).size() == 2);
}

TEST_CASE("AMFLoader reports a compressed document") {
    const std::vector<std::uint8_t> zip{0x50, 0x4b, 0x03, 0x04};
    REQUIRE_THROWS_WITH(AMFLoader::requirePlainXml(zip),
                        "THREE.AMFLoader: Compressed AMF is not supported");
    // Plain XML passes without a complaint.
    const std::vector<std::uint8_t> xml{'<', 'a', 'm', 'f', '>'};
    REQUIRE_NOTHROW(AMFLoader::requirePlainXml(xml));
}
