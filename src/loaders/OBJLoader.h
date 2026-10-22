#pragma once
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
};
