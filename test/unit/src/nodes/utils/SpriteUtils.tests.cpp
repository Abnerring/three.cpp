#include "nodes/utils/SpriteUtils.h"

#include "nodes/core/Node.h"

#include "catch.hpp"

TEST_CASE("SpriteUtils exposes the billboarding helper") {
    const auto module = nativeSpriteUtilsModule();
    const auto billboarding = module.get("billboarding");
    REQUIRE(billboarding.isCallable());
    // Fn returns a rest parameter wrapper, so its length is zero.
    REQUIRE(billboarding.get("length").strictEquals(0));
}

TEST_CASE("SpriteUtils billboarding builds the default branch") {
    const auto billboarding = nativeSpriteUtilsBinding("billboarding");
    // No options at all: horizontal is on and vertical is off.
    JSValue result;
    REQUIRE_NOTHROW(result = billboarding.call({}, {}));
    REQUIRE(result.get("isNode").strictEquals(true));

    // Vertical instead of horizontal exercises the other optional row.
    auto verticalOptions = JSValue::object(nullptr);
    verticalOptions.set("horizontal", false);
    verticalOptions.set("vertical", true);
    JSValue vertical;
    REQUIRE_NOTHROW(vertical = billboarding.call({}, {verticalOptions}));
    REQUIRE(vertical.get("isNode").strictEquals(true));
}

TEST_CASE("SpriteUtils billboarding builds the horizontal rotation branch") {
    const auto billboarding = nativeSpriteUtilsBinding("billboarding");
    auto options = JSValue::object(nullptr);
    options.set("horizontalRotation", true);
    JSValue result;
    REQUIRE_NOTHROW(result = billboarding.call({}, {options}));
    REQUIRE(result.get("isNode").strictEquals(true));
}
