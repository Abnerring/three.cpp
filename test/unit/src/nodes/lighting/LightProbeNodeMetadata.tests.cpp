#include "nodes/lighting/LightProbeNode.h"

#include "nodes/core/Node.h"

#include "catch.hpp"

// The existing LightProbeNode unit test and its differential compare the update behaviour: the
// scaled spherical harmonic coefficients. Neither looks at the constructor's Function.length, which
// is why this module kept an arity of 1 while the pinned source declares `constructor( light = null )`
// and therefore has length 0. This case guards exactly that metadata.
TEST_CASE("LightProbeNode keeps the pinned constructor metadata") {
    const auto module = nativeLightProbeNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.isCallable());
    REQUIRE(ctor.get("name").strictEquals("LightProbeNode"));
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("type").strictEquals("LightProbeNode"));
    // The two overridden methods keep their source arity.
    const auto proto = ctor.get("prototype");
    REQUIRE(proto.get("update").get("length").strictEquals(1));
    REQUIRE(proto.get("setup").get("length").strictEquals(1));
    REQUIRE(proto.get("update").get("name").strictEquals("update"));
    REQUIRE(proto.get("setup").get("name").strictEquals("setup"));
}

TEST_CASE("LightProbeNode constructor accepts the default light") {
    // Unlike the light accessors, this constructor does not key a weak map at construction time,
    // so it also has to work with no argument: the source declares light = null.
    const auto node = nativeLightProbeNodeConstructor().construct();
    REQUIRE(node.get("type").strictEquals("LightProbeNode"));
    REQUIRE(node.get("light").isNull());
    REQUIRE(node.get("lightProbe").get("array").get("length").strictEquals(9));
}
