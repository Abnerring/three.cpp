#include "nodes/core/NodeParser.h"

std::string NodeParser::abstractWarning() {
    return "Abstract function.";
}

bool NodeParser::returnsNothing() {
    // No value is produced, and no exception is raised.
    return true;
}

bool NodeParser::throwsOnMissingImplementation() {
    return false;
}
