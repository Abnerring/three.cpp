#include "nodes/core/StackNode.h"
#include "nodes/core/Node.h"
#include "catch.hpp"

TEST_CASE("StackNode inherits real Node state and forwards its output type") {
    StackNode stack;
    auto self = stack.object();
    REQUIRE(self.get("isNode").strictEquals(true)); REQUIRE(self.get("isStackNode").strictEquals(true));
    REQUIRE(self.get("getElementType").call(self, {nullptr}).strictEquals("void"));
    auto output = JSValue::object();
    output.set("getElementType", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue { return "float"; }));
    stack.outputNode = output;
    REQUIRE(self.get("getElementType").call(self, {nullptr}).strictEquals("float"));
    REQUIRE_FALSE(self.get("isCacheable").call(self).toBoolean());
}
TEST_CASE("StackNode splices additions relative to the active node") {
    StackNode stack; Node first, second, third;
    auto self = stack.object(); stack.addToStack(first.object()); self.set("_currentNode", first.object());
    stack.addToStack(second.object()); stack.addToStack(third.object());
    REQUIRE(stack.nodes.get("0").sameValue(first.object()));
    REQUIRE(stack.nodes.get("1").sameValue(second.object()));
    REQUIRE(stack.nodes.get("2").sameValue(third.object()));
    stack.addToStackBefore(third.object());
    REQUIRE(stack.nodes.get("0").sameValue(third.object()));
    REQUIRE(stack.nodes.get("length").strictEquals(4));
}
TEST_CASE("StackNode preserves active state when child build throws") {
    StackNode stack, previous;
    Node child; child.object().set("build", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue { throw JSTypeError("child"); }));
    stack.addToStack(child.object()); auto builder = JSValue::object(); builder.set("buildStage", "setup");
    builder.set("setActiveStack", JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue { return {}; }));
    nativeSetCurrentStack(previous.object());
    REQUIRE_THROWS_AS(stack.build(builder), JSTypeError);
    REQUIRE(nativeGetCurrentStack().sameValue(stack.object()));
    REQUIRE(stack.object().get("_currentNode").sameValue(child.object()));
    nativeSetCurrentStack(nullptr);
}
