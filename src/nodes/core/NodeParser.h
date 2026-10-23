#pragma once
#include <string>

// Port of the base parser in src/nodes/core/NodeParser.js at the pinned revision. The class
// exists to be derived from, one parser per shading language, and its only method does nothing
// but complain.
class NodeParser {
  public:
    // The sentence the unimplemented method prints.
    static std::string abstractWarning();

    // What parsing a source that no derived parser handled gives back. The method warns and
    // returns nothing rather than throwing, so a parser that was never written leaves its caller
    // with an empty answer instead of an error. This is the same shape as the other places in
    // this layer that report and carry on.
    static bool returnsNothing();

    // Whether the unimplemented method throws, which it does not.
    static bool throwsOnMissingImplementation();
};
