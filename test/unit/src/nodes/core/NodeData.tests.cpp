#include "nodes/core/NodeCode.h"
#include "nodes/core/NodeFunctionInput.h"

#include "catch.hpp"

TEST_CASE("NodeCode stores the name, type and shader source") {
    NodeCode code("myCode", "vec3");
    REQUIRE(NodeCode::isNodeCode);
    REQUIRE(code.name == "myCode");
    REQUIRE(code.type == "vec3");
    REQUIRE(code.code.empty());

    NodeCode filled("other", "float", "return 1.0;");
    REQUIRE(filled.code == "return 1.0;");
}

TEST_CASE("NodeFunctionInput defaults match the upstream signature") {
    NodeFunctionInput input("vec3", "position");
    REQUIRE(NodeFunctionInput::isNodeFunctionInput);
    REQUIRE(input.type == "vec3");
    REQUIRE(input.name == "position");
    // Upstream uses null for count, which is an empty optional here.
    REQUIRE_FALSE(input.count.has_value());
    REQUIRE(input.qualifier.empty());
    REQUIRE_FALSE(input.isConst);

    NodeFunctionInput array("float", "weights", 4, "inout", true);
    REQUIRE(array.count.has_value());
    REQUIRE(array.count.value() == 4);
    REQUIRE(array.qualifier == "inout");
    REQUIRE(array.isConst);
}
