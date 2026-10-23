#pragma once
#include <string>

// Port of the rules in src/nodes/core/BypassNode.js at the pinned revision. The node runs one
// thing and answers with another, so that a call which returns nothing can stand where a value
// is required.
class BypassNode {
  public:
    static constexpr bool isBypassNode = true;
    static constexpr const char* kTypeName = "BypassNode";

    // The type the node reports, which is the output node's. The call's own type is never asked
    // for, since the call is made for its effect rather than for its answer.
    static std::string nodeType(const std::string& outputType);

    // The type the call is built with. It is forced to the empty type, so a call that would
    // otherwise return a value has that value discarded rather than converted or stored.
    static constexpr const char* kCallBuildType = "void";

    // Whether the call's snippet is added to the flow. An empty snippet is not, so a call whose
    // text comes out empty produces no line at all rather than a blank one.
    static bool addsLine(const std::string& snippet);

    // The order the two are built in: the call first, so that its text is emitted before the
    // output it is meant to happen beside.
    static bool buildsCallFirst();

    // Whether the node may be cached. It may not.
    static bool isCacheable();
};
