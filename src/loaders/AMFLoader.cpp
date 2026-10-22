#include "loaders/AMFLoader.h"

#include "loaders/XmlDocument.h"
#include <cctype>
#include <cstdlib>
#include <stdexcept>

bool AMFLoader::isZip(const std::vector<std::uint8_t>& buffer) {
    // The archive signature is the two characters PK.
    return buffer.size() >= 2 && buffer[0] == 0x50 && buffer[1] == 0x4b;
}

double AMFLoader::unitScale(const std::string& unit) {
    std::string lowered = unit;
    for (auto& c : lowered) c = char(std::tolower(static_cast<unsigned char>(c)));

    // Every scale is expressed in millimetres, which is the unit the coordinates use.
    if (lowered == "millimeter") return 1.0;
    if (lowered == "inch") return 25.4;
    if (lowered == "feet") return 304.8;
    if (lowered == "meter") return 1000.0;
    if (lowered == "micron") return 0.001;
    // An unknown unit leaves the coordinates as they are.
    return 1.0;
}

AMFLoader::MeshVolume AMFLoader::parseMeshVolume(const XmlNode& node) {
    MeshVolume volume;
    // The material is an attribute of the volume itself.
    volume.materialId = node.attributeOr("materialid");

    for (const auto& child : node.children) {
        if (!child) continue;
        if (child->name == "metadata") {
            // Only the entry of type name renames the volume.
            if (child->attributeOr("type") == "name") volume.name = child->text;
        } else if (child->name == "triangle") {
            const auto corner = [&child](const char* name) -> int {
                const auto found = child->child(name);
                return found ? std::atoi(found->text.c_str()) : 0;
            };
            volume.triangles.push_back(corner("v1"));
            volume.triangles.push_back(corner("v2"));
            volume.triangles.push_back(corner("v3"));
        }
    }
    return volume;
}

AMFLoader::MeshVertices AMFLoader::parseMeshVertices(const XmlNode& node) {
    MeshVertices vertices;
    const auto component = [](const XmlNode& parent, const char* name) -> double {
        const auto found = parent.child(name);
        return found ? std::strtod(found->text.c_str(), nullptr) : 0.0;
    };

    for (const auto& vertex : node.children) {
        if (!vertex || vertex->name != "vertex") continue;
        for (const auto& part : vertex->children) {
            if (!part) continue;
            if (part->name == "coordinates") {
                vertices.positions.push_back(component(*part, "x"));
                vertices.positions.push_back(component(*part, "y"));
                vertices.positions.push_back(component(*part, "z"));
            } else if (part->name == "normal") {
                vertices.normals.push_back(component(*part, "nx"));
                vertices.normals.push_back(component(*part, "ny"));
                vertices.normals.push_back(component(*part, "nz"));
            }
        }
    }
    return vertices;
}

void AMFLoader::requirePlainXml(const std::vector<std::uint8_t>& buffer) {
    if (isZip(buffer))
        throw std::runtime_error("THREE.AMFLoader: Compressed AMF is not supported");
}

std::vector<AMFLoader::Mesh> AMFLoader::parseMeshes(const XmlNode& root, double scale) {
    std::vector<Mesh> meshes;
    for (const auto& object : root.children) {
        if (!object || object->name != "object") continue;
        for (const auto& meshNode : object->children) {
            if (!meshNode || meshNode->name != "mesh") continue;

            Mesh mesh;
            if (const auto vertices = meshNode->child("vertices"))
                mesh.vertices = parseMeshVertices(*vertices);
            // The scale converts the file's unit to millimetres, applied to each component.
            for (auto& component : mesh.vertices.positions) component *= scale;

            for (const auto& volume : meshNode->children)
                if (volume && volume->name == "volume") mesh.volumes.push_back(parseMeshVolume(*volume));
            meshes.push_back(std::move(mesh));
        }
    }
    return meshes;
}