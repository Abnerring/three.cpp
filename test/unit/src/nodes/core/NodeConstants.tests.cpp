#include "nodes/core/NodeConstants.h"

#include "catch.hpp"

TEST_CASE("node constant enums carry the upstream string values") {
    REQUIRE(NodeShaderStage::VERTEX == "vertex");
    REQUIRE(NodeShaderStage::FRAGMENT == "fragment");
    REQUIRE(NodeUpdateType::NONE == "none");
    REQUIRE(NodeUpdateType::FRAME == "frame");
    REQUIRE(NodeUpdateType::RENDER == "render");
    REQUIRE(NodeUpdateType::OBJECT == "object");
    REQUIRE(NodeType::BOOLEAN == "bool");
    REQUIRE(NodeType::INTEGER == "int");
    REQUIRE(NodeType::FLOAT == "float");
    REQUIRE(NodeType::VECTOR2 == "vec2");
    REQUIRE(NodeType::VECTOR3 == "vec3");
    REQUIRE(NodeType::VECTOR4 == "vec4");
    REQUIRE(NodeType::MATRIX2 == "mat2");
    REQUIRE(NodeType::MATRIX3 == "mat3");
    REQUIRE(NodeType::MATRIX4 == "mat4");
    REQUIRE(NodeAccess::READ_ONLY == "readOnly");
    REQUIRE(NodeAccess::WRITE_ONLY == "writeOnly");
    REQUIRE(NodeAccess::READ_WRITE == "readWrite");
}

TEST_CASE("node constant arrays keep the upstream order") {
    REQUIRE(defaultShaderStages.size() == 2);
    REQUIRE(defaultShaderStages[0] == "fragment");
    REQUIRE(defaultShaderStages[1] == "vertex");
    REQUIRE(defaultBuildStages.size() == 3);
    REQUIRE(defaultBuildStages[0] == "setup");
    REQUIRE(defaultBuildStages[1] == "analyze");
    REQUIRE(defaultBuildStages[2] == "generate");
    // shaderStages extends defaultShaderStages with compute, in that order.
    REQUIRE(shaderStages.size() == 3);
    REQUIRE(shaderStages[0] == "fragment");
    REQUIRE(shaderStages[1] == "vertex");
    REQUIRE(shaderStages[2] == "compute");
    REQUIRE(vectorComponents.size() == 4);
    REQUIRE(vectorComponents[3] == "w");
}
