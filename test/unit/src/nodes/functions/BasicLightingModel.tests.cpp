#include "nodes/functions/BasicLightingModel.h"

#include "catch.hpp"

TEST_CASE("BasicLightingModel exports the pinned default class") {
    const auto module = nativeBasicLightingModelModule();
    const auto constructor = nativeBasicLightingModel();
    REQUIRE(module.get("default").sameValue(constructor));
    REQUIRE(constructor.get("name").strictEquals("BasicLightingModel"));
    REQUIRE(constructor.get("length").toNumber() == 0);
}

TEST_CASE("BasicLightingModel preserves LightingModel inheritance and method metadata") {
    const auto constructor = nativeBasicLightingModel();
    const auto prototype = constructor.get("prototype");
    REQUIRE(prototype.getPrototype().get("constructor").get("name").strictEquals("LightingModel"));
    REQUIRE(prototype.get("indirect").get("length").toNumber() == 1);
    REQUIRE(prototype.get("finish").get("length").toNumber() == 1);
    REQUIRE(prototype.get("indirect").nativeFunctionSource().find("indirect( {") == 0);
    REQUIRE(prototype.get("finish").nativeFunctionSource().find("finish( builder )") == 0);
}

TEST_CASE("BasicLightingModel instances inherit both concrete hooks") {
    const auto instance = nativeBasicLightingModel().construct();
    REQUIRE(instance.get("indirect").isCallable());
    REQUIRE(instance.get("finish").isCallable());
    REQUIRE(instance.get("start").isCallable());
}
