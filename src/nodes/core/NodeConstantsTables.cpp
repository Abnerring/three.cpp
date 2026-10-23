#include "nodes/core/NodeConstantsTables.h"

const std::vector<std::string>& NodeConstantsTables::defaultShaderStages() {
    // The fragment stage first, then the vertex one.
    static const std::vector<std::string> names{"fragment", "vertex"};
    return names;
}

const std::vector<std::string>& NodeConstantsTables::shaderStages() {
    // The two defaults with compute appended.
    static const std::vector<std::string> names{"fragment", "vertex", "compute"};
    return names;
}

const std::vector<std::string>& NodeConstantsTables::defaultBuildStages() {
    static const std::vector<std::string> names{"setup", "analyze", "generate"};
    return names;
}

const std::vector<std::string>& NodeConstantsTables::vectorComponents() {
    static const std::vector<std::string> names{"x", "y", "z", "w"};
    return names;
}

bool NodeConstantsTables::passesTruthTest(const std::string& updateKind) {
    // A string is a value, so even the one meaning none passes.
    return !updateKind.empty();
}

bool NodeConstantsTables::meansNoUpdate(const std::string& updateKind) {
    // Only a comparison against the word tells it apart.
    return updateKind == kUpdateNone;
}
