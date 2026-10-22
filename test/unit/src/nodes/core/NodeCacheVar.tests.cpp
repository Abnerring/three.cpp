#include "nodes/core/NodeCache.h"
#include "nodes/core/NodeVar.h"

#include "catch.hpp"

#include <memory>

TEST_CASE("NodeCache hands out increasing ids and starts empty") {
    NodeCache first;
    NodeCache second;
    REQUIRE(second.id() == first.id() + 1);
    REQUIRE_FALSE(first.parent());

    int key = 0;
    REQUIRE(first.getData(&key).isUndefined());
}

TEST_CASE("NodeCache falls back to its parent and prefers its own data") {
    auto parent = std::make_shared<NodeCache>();
    NodeCache child(parent);

    int a = 0;
    int b = 0;
    parent->setData(&a, JSValue(1.0));
    REQUIRE(child.getData(&a).toNumber() == 1);
    REQUIRE(child.getData(&b).isUndefined());

    // A value set on the child shadows the parent for that node only.
    child.setData(&a, JSValue(2.0));
    REQUIRE(child.getData(&a).toNumber() == 2);
    REQUIRE(parent->getData(&a).toNumber() == 1);
}

TEST_CASE("NodeVar defaults match the upstream signature") {
    NodeVar basic("vColor", "vec3");
    REQUIRE(NodeVar::isNodeVar);
    REQUIRE(basic.name == "vColor");
    REQUIRE(basic.type == "vec3");
    REQUIRE_FALSE(basic.readOnly);
    REQUIRE_FALSE(basic.local);
    REQUIRE_FALSE(basic.count.has_value());

    NodeVar array("weights", "float", true, 4, true);
    REQUIRE(array.readOnly);
    REQUIRE(array.local);
    REQUIRE(array.count.has_value());
    REQUIRE(array.count.value() == 4);
}
