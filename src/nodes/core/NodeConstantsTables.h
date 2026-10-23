#pragma once
#include <string>
#include <vector>

// Port of the tables in src/nodes/core/constants.js at the pinned revision. Two of them hold a
// surprise worth writing down: the update kinds are strings rather than numbers, so the one that
// means no update is still a value that a truth test treats as one; and the default shader stages
// list the fragment stage before the vertex one.
class NodeConstantsTables {
  public:
    // The shader stages, in the order the module lists them. The fragment stage comes first, not
    // the vertex one, so anything that walks the default list reaches the fragment stage first.
    static const std::vector<std::string>& defaultShaderStages();

    // The same with the compute stage added at the end, which is how the fuller list is built.
    static const std::vector<std::string>& shaderStages();

    // The build stages, in order.
    static const std::vector<std::string>& defaultBuildStages();

    // The component names.
    static const std::vector<std::string>& vectorComponents();

    // The update kinds, which are strings. The one that means no update is the word "none"
    // rather than a number, so a node whose update kind is that word still has a value that a
    // truth test would pass; only a comparison against the word itself tells it apart.
    static constexpr const char* kUpdateNone = "none";
    static constexpr const char* kUpdateFrame = "frame";
    static constexpr const char* kUpdateRender = "render";
    static constexpr const char* kUpdateObject = "object";

    // Whether a node with a given update kind would pass a truth test. The none kind does,
    // which is the trap: it is a string like the others, not a falsy number.
    static bool passesTruthTest(const std::string& updateKind);

    // Whether an update kind means no update, which needs a comparison against the word.
    static bool meansNoUpdate(const std::string& updateKind);

    // The access kinds and the type names the module lists, two of which are not spelled the way
    // their names suggest: a boolean is called a bool and an integer an int.
    static constexpr const char* kBooleanType = "bool";
    static constexpr const char* kIntegerType = "int";
    static constexpr const char* kAccessReadOnly = "readOnly";
    static constexpr const char* kAccessReadWrite = "readWrite";
};
