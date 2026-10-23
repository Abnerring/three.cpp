#pragma once
#include <string>
#include <vector>

// Port of the rules in src/nodes/core/IndexNode.js at the pinned revision. The node reads one of
// six indices the shader is given, and three of its rules are worth writing down.
class IndexNode {
  public:
    static constexpr bool isIndexNode = true;
    static constexpr const char* kTypeName = "IndexNode";

    // The type the node reports. An index counts, so it is an unsigned integer rather than one of
    // the value types.
    static constexpr const char* kNodeType = "uint";

    // The six scopes, in the order the source declares them.
    static std::vector<std::string> scopes();

    // The builder call each scope goes through. A scope the node does not know is not answered
    // with a default: the source throws, which is unlike every other place in this layer taken so
    // far, where a missing or unknown thing is reported and the work carries on.
    static std::string builderCall(const std::string& scope);

    // Whether a scope is one the node knows, which is the same question as whether the call above
    // would answer rather than throw.
    static bool isKnown(const std::string& scope);

    // The sentence thrown for an unknown scope, with the scope in it.
    static std::string unknownScopeMessage(const std::string& scope);

    // Which stages read the index directly. The vertex stage and the compute one both do, so the
    // list is longer than the name of the first would suggest; every other stage takes it through
    // a varying instead.
    static bool readsDirectly(const std::string& shaderStage);
};
