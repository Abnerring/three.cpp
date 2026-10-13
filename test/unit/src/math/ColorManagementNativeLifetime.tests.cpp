#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "math/ColorManagement.h"
#include "math/Color.h"
TEST_CASE("Typed ColorManagement writes require a keeper and retain one authority") {
    REQUIRE_FALSE(JSValue::hasActiveHeap());
    REQUIRE_THROWS_AS(ColorManagement::enabled=false,JSTypeError);
    REQUIRE_FALSE(JSValue::hasActiveHeap());
    REQUIRE_THROWS_AS(ColorManagement::workingColorSpace="srgb",JSTypeError);
    REQUIRE_FALSE(JSValue::hasActiveHeap());
    JSWeakValue oldModule;
    {
        ColorManagement::Context context;auto cm=context.object();oldModule=JSWeakValue(cm);
        REQUIRE(JSValue::hasActiveHeap());ColorManagement::enabled=false;ColorManagement::workingColorSpace="srgb";
        REQUIRE_FALSE(bool(ColorManagement::enabled));REQUIRE(std::string(ColorManagement::workingColorSpace)=="srgb");
        REQUIRE(cm.get("enabled").sameValue(false));REQUIRE(cm.get("workingColorSpace").sameValue("srgb"));
        cm.set("enabled",true);REQUIRE(bool(ColorManagement::enabled));Color color(.5,.25,.75);ColorManagement::convert(color,"srgb","srgb-linear");REQUIRE(color.r==Approx(SRGBToLinear(.5)));
        const auto previous=ColorManagement::workingColorSpace;ColorManagement::workingColorSpace="srgb-linear";
        REQUIRE(previous.value().sameValue("srgb"));ColorManagement::workingColorSpace=previous;REQUIRE(cm.get("workingColorSpace").sameValue("srgb"));
    }
    REQUIRE_FALSE(JSValue::hasActiveHeap());REQUIRE_FALSE(oldModule.lock().has_value());
    {
        ColorManagement::Context next;REQUIRE(next.object().get("enabled").sameValue(true));REQUIRE(next.object().get("workingColorSpace").sameValue("srgb-linear"));
    }
    REQUIRE_FALSE(JSValue::hasActiveHeap());
    {
        const auto previous=ColorManagement::enabled; // Existing save/restore syntax is also an explicit snapshot keeper.
        REQUIRE(JSValue::hasActiveHeap());ColorManagement::enabled=false;REQUIRE_FALSE(bool(ColorManagement::enabled));
        ColorManagement::enabled=previous;REQUIRE(bool(ColorManagement::enabled));
    }
    REQUIRE_FALSE(JSValue::hasActiveHeap());
}
