#include "nodes/core/NodeBuilderAssembly.h"

NodeBuilderAssembly::Filled NodeBuilderAssembly::arrayPlace(bool hasValues, bool valueIsPresent) {
    // Only whether a value is there.
    if (hasValues && valueIsPresent) return Filled::Built;
    return Filled::Constant;
}

NodeBuilderAssembly::Filled NodeBuilderAssembly::structPlace(bool hasValues, bool memberHasValue,
                                                             bool memberValueIsNode) {
    // Three questions, and only the third passing means the value is built.
    if (hasValues && memberHasValue && memberValueIsNode) return Filled::Built;
    return Filled::Constant;
}

bool NodeBuilderAssembly::bothAskTheSame() {
    // The list asks one question and the structure three.
    return false;
}

std::string NodeBuilderAssembly::joined(int placeCount) {
    // A separator after every place but the last.
    std::string out;
    for (int i = 0; i < placeCount; ++i) {
        if (i > 0) out += ", ";
        out += "x";
    }
    return out;
}

bool NodeBuilderAssembly::memberTypeShadowsStructureType() {
    return true;
}
