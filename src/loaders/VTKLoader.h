#pragma once
#include <string>
#include <vector>

// Port of the text scanning in examples/jsm/loaders/VTKLoader.js at the pinned revision. The
// text form of the format is a sequence of keywords and numbers, so the reader starts by
// splitting a line into the values it holds. The binary form is compressed and needs a
// decompressor the port does not carry, so only the text form is portable.
class VTKLoader {
  public:
    // The values on one line, split on whitespace. A token that does not begin with a number
    // reads as NaN, which is what the upstream conversion gives when it meets one.
    static std::vector<double> parseFloats(const std::string& line);

    // How many characters at the front of a line form a word rather than a number. A word is
    // anything that is not a digit, a dot, whitespace or a minus sign.
    static std::size_t wordLength(const std::string& line);

    // The number of nodes a connectivity line declares and the node numbers that follow it.
    struct Connectivity {
        int numVertices = 0;
        std::vector<int> indices;
    };

    // Reads a connectivity line, which begins with a count and then lists that many nodes.
    // Returns nothing when the line does not begin with a count at all.
    static bool parseConnectivity(const std::string& line, Connectivity& out);

    // Splits a polygon into triangles around its first node. A polygon of fewer than three
    // nodes is dropped rather than producing anything.
    static std::vector<int> triangulateFan(const std::vector<int>& indices, int numVertices);

    // Whether a line holds only numbers, which is how a continuation line of a POINTS section
    // is told from a keyword.
    static bool isNumberLine(const std::string& line);
};
