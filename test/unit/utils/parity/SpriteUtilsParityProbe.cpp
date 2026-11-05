#include "nodes/utils/SpriteUtils.h"

#include "core/JSValue.h"

#include <iostream>
#include <string>

int main() {
    try {
        const auto billboarding = nativeSpriteUtilsBinding("billboarding");

        auto build = [&](const JSValue& options, JSValue& out, std::string& failure) -> bool {
            try {
                out = billboarding.call({}, {options});
                return true;
            } catch (const std::exception& error) {
                failure = error.what();
                return false;
            }
        };

        JSValue defaultResult, verticalResult, rotationResult, positionResult;
        std::string defaultFailure, verticalFailure, rotationFailure, positionFailure;
        const bool defaultBuilt = build(JSValue::object(nullptr), defaultResult, defaultFailure);

        auto verticalOptions = JSValue::object(nullptr);
        verticalOptions.set("horizontal", false);
        verticalOptions.set("vertical", true);
        const bool verticalBuilt = build(verticalOptions, verticalResult, verticalFailure);

        auto rotationOptions = JSValue::object(nullptr);
        rotationOptions.set("horizontalRotation", true);
        const bool rotationBuilt = build(rotationOptions, rotationResult, rotationFailure);

        // A position override goes through nodeObject, so it is only compared as built or raised.
        auto positionNode = JSValue::object(nullptr);
        positionNode.set("isNode", true);
        auto positionOptions = JSValue::object(nullptr);
        positionOptions.set("position", positionNode);
        const bool positionBuilt = build(positionOptions, positionResult, positionFailure);

        auto output = JSValue::object(nullptr);
        output.set("callable", billboarding.isCallable());
        output.set("isCallable", billboarding.isCallable());
        output.set("defaultBuilt", defaultBuilt);
        if (!defaultBuilt) output.set("defaultFailure", JSValue(defaultFailure));
        output.set("defaultIsNode", defaultBuilt ? defaultResult.get("isNode") : JSValue(false));
        output.set("defaultType", defaultBuilt ? defaultResult.get("type") : JSValue(""));
        output.set("verticalBuilt", verticalBuilt);
        if (!verticalBuilt) output.set("verticalFailure", JSValue(verticalFailure));
        output.set("verticalIsNode", verticalBuilt ? verticalResult.get("isNode") : JSValue(false));
        output.set("rotationBuilt", rotationBuilt);
        if (!rotationBuilt) output.set("rotationFailure", JSValue(rotationFailure));
        output.set("rotationIsNode", rotationBuilt ? rotationResult.get("isNode") : JSValue(false));
        output.set("positionBuilt", positionBuilt);
        if (!positionBuilt) output.set("positionFailure", JSValue(positionFailure));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
