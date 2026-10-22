#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the container and unit handling in examples/jsm/loaders/AMFLoader.js at the pinned
// revision. AMF is an additive manufacturing format: either plain XML or a zip archive that
// holds one, with a unit on the root element that scales the millimetre coordinates. This
// slice covers the archive detection and the unit scale; reading the mesh is the layer above.
struct XmlNode;

class AMFLoader {
  public:
    // Whether the buffer starts with the zip signature, which upstream takes to mean the
    // document is packed and needs a decompressor to reach.
    static bool isZip(const std::vector<std::uint8_t>& buffer);

    // The millimetres one of the given unit equals. An unknown or missing unit is a
    // millimetre, which is what upstream falls back to.
    static double unitScale(const std::string& unit);

    // A volume: a name from a name metadata entry, an optional material id and the triangle
    // corner indices in document order.
    struct MeshVolume {
        std::string name;
        std::string materialId;
        std::vector<int> triangles;
    };

    // The vertices of a mesh, with normals kept separately since a file may give none.
    struct MeshVertices {
        std::vector<double> positions;
        std::vector<double> normals;
    };

    // Reads a volume element: the triangles and the metadata that names it.
    static MeshVolume parseMeshVolume(const XmlNode& node);

    // Reads a vertices element, taking x y z from each coordinates child and n x n y n z from
    // each normal child.
    static MeshVertices parseMeshVertices(const XmlNode& node);
};
