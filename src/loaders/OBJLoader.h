#pragma once
#include "loaders/OBJMaterialGroups.h"
#include <array>
#include <string>
#include <vector>

// Port of the geometry extraction layer of examples/jsm/loaders/OBJLoader.js at the pinned
// revision: line preprocessing, the vertex/normal/uv streams, the face fan triangulation and
// the three relative index parsers. Turning those into BufferGeometry objects, grouping by
// object and resolving materials are the layers above and are not part of this slice.
class OBJLoader {
  public:
    // Nine numbers per triangle: vertex, normal and uv indices for each of its three corners.
    // Resolved indices are already multiplied out as upstream does (vertex and normal by
    // three, uv by two), so they point into the flat arrays.
    using Triangle = std::array<int, 9>;

    struct Geometry {
        std::vector<double> vertices;  // three numbers per vertex
        std::vector<double> normals;   // three numbers per normal
        std::vector<double> uvs;       // two numbers per uv
        // Three numbers per vertex, undefined when the file had no vertex colours; stored as
        // zero with hasVertexColors telling the two apart.
        std::vector<double> colors;
        bool hasVertexColors = false;
        std::vector<Triangle> triangles;
    };

    // Relative index resolution for the three kinds, matching upstream's helpers.
    static int parseVertexIndex(const std::string& value, std::size_t length);
    static int parseNormalIndex(const std::string& value, std::size_t length);
    static int parseUVIndex(const std::string& value, std::size_t length);

    // Reads the text into flat arrays. Carriage returns are normalised, backslash line
    // continuations are joined, comments and blank lines are skipped, faces are fanned from
    // their first vertex, and a face corner with a missing index contributes zero.
    Geometry parseGeometry(const std::string& text) const;

    // Computes one face normal from three vertices and appends it three times, which is what
    // upstream does when a face carries no normal indices. The three indices a, b and c point
    // into vertices; the result is appended to out.
    static void addFaceNormal(const std::vector<double>& vertices, int a, int b, int c,
                              std::vector<double>& out);

    // One object as the file declares it, with the accumulated streams and the material runs.
    struct Object {
        std::string name;
        bool fromDeclaration = false;
        std::vector<double> vertices;
        std::vector<double> normals;
        std::vector<double> colors;
        std::vector<double> uvs;
        bool hasUVIndices = false;
        OBJMaterialGroups groups;
        std::vector<std::string> materialLibraries;
    };

    // Splits the text into objects the way upstream does: o and g start a new object, usemtl
    // opens a material run, mtllib records a library, faces fan into the current object. The
    // first object starts undeclared so the first o or g renames it instead of adding one.
    std::vector<Object> parseObjects(const std::string& text) const;
};
