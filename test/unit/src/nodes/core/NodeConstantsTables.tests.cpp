#include "nodes/core/NodeConstantsTables.h"

#include "catch.hpp"

#include <string>

TEST_CASE("NodeConstantsTables lists the fragment stage before the vertex one") {
    // The default order is fragment then vertex, not the other way round and not alphabetical,
    // so anything walking the list reaches the fragment stage first.
    const auto& stages = NodeConstantsTables::defaultShaderStages();
    REQUIRE(stages.size() == 2);
    REQUIRE(stages[0] == "fragment");
    REQUIRE(stages[1] == "vertex");
    REQUIRE(stages[0] != "vertex");
    REQUIRE(stages[0] != stages[1]);
}

TEST_CASE("NodeConstantsTables appends the compute stage to the two defaults") {
    // The fuller list is the two defaults with compute at the end, so its first two entries are
    // the defaults in the same order.
    const auto& full = NodeConstantsTables::shaderStages();
    REQUIRE(full.size() == 3);
    REQUIRE(full[0] == "fragment");
    REQUIRE(full[1] == "vertex");
    REQUIRE(full[2] == "compute");
    // The compute stage is last rather than first, which is what appending means.
    REQUIRE(full[2] != full[0]);
    REQUIRE(full[0] == NodeConstantsTables::defaultShaderStages()[0]);
}

TEST_CASE("NodeConstantsTables lists its build stages and components in order") {
    const auto& builds = NodeConstantsTables::defaultBuildStages();
    REQUIRE(builds.size() == 3);
    REQUIRE(builds[0] == "setup");
    REQUIRE(builds[1] == "analyze");
    REQUIRE(builds[2] == "generate");
    const auto& components = NodeConstantsTables::vectorComponents();
    REQUIRE(components.size() == 4);
    REQUIRE(components[0] == "x");
    REQUIRE(components[3] == "w");
}

TEST_CASE("NodeConstantsTables' no update kind is a word, not a number") {
    // The kinds are strings, so the one meaning no update is a value that a truth test passes.
    REQUIRE(std::string(NodeConstantsTables::kUpdateNone) == "none");
    REQUIRE(NodeConstantsTables::passesTruthTest(NodeConstantsTables::kUpdateNone));
    // Only a comparison against the word tells it apart, which a truth test cannot do.
    REQUIRE(NodeConstantsTables::meansNoUpdate(NodeConstantsTables::kUpdateNone));
    REQUIRE_FALSE(NodeConstantsTables::meansNoUpdate(NodeConstantsTables::kUpdateFrame));
    // The four kinds are four different words, so none is another's spelling.
    REQUIRE(std::string(NodeConstantsTables::kUpdateFrame) != NodeConstantsTables::kUpdateNone);
    REQUIRE(std::string(NodeConstantsTables::kUpdateRender) != NodeConstantsTables::kUpdateObject);
    // The other three also pass a truth test, so passing one does not distinguish any of them.
    REQUIRE(NodeConstantsTables::passesTruthTest(NodeConstantsTables::kUpdateFrame));
    REQUIRE(NodeConstantsTables::passesTruthTest(NodeConstantsTables::kUpdateNone) ==
            NodeConstantsTables::passesTruthTest(NodeConstantsTables::kUpdateFrame));
}

TEST_CASE("NodeConstantsTables spells two types shorter than their names") {
    // A boolean is called a bool and an integer an int, so the name a caller writes is not the
    // name of the kind.
    REQUIRE(std::string(NodeConstantsTables::kBooleanType) == "bool");
    REQUIRE(std::string(NodeConstantsTables::kBooleanType) != "boolean");
    REQUIRE(std::string(NodeConstantsTables::kIntegerType) == "int");
    REQUIRE(std::string(NodeConstantsTables::kIntegerType) != "integer");
    // The access kinds use camel case where the type names do not.
    REQUIRE(std::string(NodeConstantsTables::kAccessReadOnly) == "readOnly");
    REQUIRE(std::string(NodeConstantsTables::kAccessReadWrite) == "readWrite");
}