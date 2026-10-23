#pragma once
#include <string>
#include <vector>

// Port of the type choice in src/nodes/utils/ConvertNode.js at the pinned revision. A node may be
// asked to convert to one of several types, written with bars between them, and the rule that
// picks one has two consequences that are easy to get wrong.
class ConvertNode {
  public:
    static constexpr const char* kTypeName = "ConvertNode";

    // How many parts a type has, which is what the choice compares: a three part vector counts
    // three, a float counts one.
    static int typeLength(const std::string& type);

    // The parts a type list was written with, in the order given.
    static std::vector<std::string> parts(const std::string& convertTo);

    // The type chosen from a list for a given requested type. The choice walks the list and
    // keeps the last part whose length equals the requested type's; but the first part is always
    // kept because there is nothing to compare against yet. Two consequences follow: a list none
    // of whose parts has the requested length answers with its first part rather than with
    // nothing, and a list with more than one match answers with the last match rather than the
    // first.
    static std::string choose(const std::string& requestType, const std::string& convertTo);
};
