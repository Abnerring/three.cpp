#pragma once
#include <string>

// Port of the rules in src/nodes/core/AttributeNode.js at the pinned revision. The node reads a
// vertex attribute, and four of its rules are worth writing down.
class AttributeNode {
  public:
    static constexpr const char* kTypeName = "AttributeNode";

    // The type a missing attribute takes. It is a float, and a warning is printed, so a shader
    // that reads an attribute the geometry does not have still compiles and gives nought where
    // the attribute would have been; the same default the member node and the parameter node use.
    static constexpr const char* kMissingAttributeType = "float";

    static std::string nodeType(bool typeWasGiven, const std::string& givenType,
                                bool hasGeometryAttribute, const std::string& typeFromAttribute);

    // The sentence printed when the attribute is missing, which names it.
    static std::string missingAttributeWarning(const std::string& attributeName);

    // How the attribute is written in each stage. In the vertex stage it is the attribute
    // itself; in any other stage it is wrapped in a varying, so the same node produces code of
    // two different shapes depending on where it is used. A reader who took the attribute for a
    // value the fragment stage can read directly would miss that.
    enum class Form { Attribute, Varying };

    static Form formFor(const std::string& shaderStage);

    // Whether the hash is the attribute's name rather than an identifier. It is the name, so two
    // nodes naming the same attribute hash alike; the parameter node hashes its identifier, which
    // is the other reasonable choice and a different one.
    static bool hashIsTheName();

    // Whether a node is global by default, which it is, so it is not scoped to one place.
    static constexpr bool globalByDefault = true;

    static bool isCacheable();
};
