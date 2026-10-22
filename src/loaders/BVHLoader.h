#pragma once
#include <memory>
#include <string>
#include <vector>

// Port of the hierarchy half of examples/jsm/loaders/BVHLoader.js at the pinned revision. A
// BVH file is a joint hierarchy followed by per frame channel data; this slice reads the
// hierarchy into nodes and a flat list in document order, which is what the motion pass and
// the skeleton builder both work from.
class BVHLoader {
  public:
    struct Node {
        std::string name;
        // ROOT, JOINT or ENDSITE.
        std::string type;
        double offset[3] = {0, 0, 0};
        // Channel names in declaration order. An end site has none.
        std::vector<std::string> channels;
        std::vector<std::shared_ptr<Node>> children;
    };

    struct Hierarchy {
        std::shared_ptr<Node> root;
        // Every node in the order the file declared them, parents before children.
        std::vector<std::shared_ptr<Node>> list;
    };

    // Reads everything from HIERARCHY up to the closing brace of the root. Throws
    // std::runtime_error when a required token is missing or malformed.
    static Hierarchy parseHierarchy(const std::vector<std::string>& lines, std::size_t& cursor);

    // Splits the text into lines the way upstream does, trimming a trailing carriage return.
    static std::vector<std::string> splitLines(const std::string& text);
};
