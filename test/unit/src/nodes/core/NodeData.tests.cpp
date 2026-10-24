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
    REQUIRE(NodeFunctionInput::constructor().get("isNodeFunctionInput").strictEquals(true));
    REQUIRE(input.type.strictEquals("vec3"));
    REQUIRE(input.name.strictEquals("position"));
    REQUIRE(input.count.isNull());
    REQUIRE(input.qualifier.strictEquals(""));
    REQUIRE_FALSE(input.isConst.toBoolean());

    NodeFunctionInput array("float", "weights", 4, "inout", true);
    REQUIRE(array.count.strictEquals(4));
    REQUIRE(array.qualifier.strictEquals("inout"));
    REQUIRE(array.isConst.toBoolean());
}
