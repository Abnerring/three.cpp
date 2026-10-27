#include "nodes/accessors/BuiltinNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("BuiltinNode gives its type as a float whatever it names") {
    // The type is fixed, so a node naming a four part variable still reports a float. That the
    // type carries no information is the property worth checking.
    REQUIRE(std::string(BuiltinNode::kNodeType) == "float");
    const BuiltinNode vec4Name("gl_ClipDistance");
    REQUIRE(std::string(BuiltinNode::kNodeType) == "float");
    // The name is kept as given, separately from the type.
    REQUIRE(vec4Name.name() == "gl_ClipDistance");
}

TEST_CASE("BuiltinNode hands its name back untouched") {
    // The generated text is the name exactly, with no prefix and no checking against a list.
    const BuiltinNode known("gl_FragDepth");
    REQUIRE(known.generate() == "gl_FragDepth");
    // A name the shader would not know is passed through rather than refused, which is what
    // having no list means.
    const BuiltinNode unknown("not_a_real_builtin");
    REQUIRE(unknown.generate() == "not_a_real_builtin");
    // An empty name gives an empty answer rather than being replaced by anything.
    const BuiltinNode empty("");
    REQUIRE(empty.generate().empty());
}

TEST_CASE("BuiltinNode is never cacheable") {
    // As with the point coordinate node, the refusal is a plain no.
    REQUIRE_FALSE(BuiltinNode::isCacheable());
    REQUIRE(BuiltinNode::isBuiltinNode);
}

TEST_CASE("BuiltinNode's name and its text are the same thing") {
    // Since the text is the name, the two never differ, which a sweep shows rather than a
    // remark.
    const char* names[] = {"gl_Position", "gl_FragDepth", "x", ""};
    for (const char* name : names) {
        const BuiltinNode node(name);
        REQUIRE(node.generate() == node.name());
    }
}