#include "materials/nodes/SpriteNodeMaterial.h"

#include "core/JSValue.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
JSValue builder(bool perspective, const JSValue& object) {
    auto camera = JSValue::object(nullptr);
    camera.set("isPerspectiveCamera", perspective);
    auto value = JSValue::object(nullptr);
    value.set("object", object);
    value.set("camera", camera);
    return value;
}
}  // namespace

int main() {
    try {
        const auto ctor = nativeSpriteNodeMaterialConstructor();
        const auto material = ctor.construct();

        const auto versionBefore = material.get("version").toNumber();
        auto handle = material;
        handle.set("sizeAttenuation", true);
        const auto versionAfterSame = material.get("version").toNumber();
        handle.set("sizeAttenuation", false);
        const auto versionAfterChange = material.get("version").toNumber();

        const auto freshMaterial = ctor.construct();
        auto emptyObject = JSValue::object(nullptr);
        bool simpleBuilt = true;
        std::string simpleFailure;
        JSValue simpleResult;
        try {
            simpleResult = freshMaterial.get("setupPositionView").call(freshMaterial, {builder(false, emptyObject)});
        } catch (const std::exception& error) {
            simpleBuilt = false;
            simpleFailure = error.what();
        }

        auto perspectiveMaterial = ctor.construct();
        auto perspectiveHandle = perspectiveMaterial;
        perspectiveHandle.set("sizeAttenuation", false);
        bool perspectiveBuilt = true;
        try {
            perspectiveMaterial.get("setupPositionView").call(perspectiveMaterial, {builder(true, emptyObject)});
        } catch (...) {
            perspectiveBuilt = false;
        }

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("type", material.get("type"));
        output.set("isSpriteNodeMaterial", material.get("isSpriteNodeMaterial"));
        output.set("isNodeMaterial", material.get("isNodeMaterial"));
        output.set("useSizeAttenuation", material.get("_useSizeAttenuation"));
        output.set("positionNodeIsNull", material.get("positionNode").isNull());
        output.set("rotationNodeIsNull", material.get("rotationNode").isNull());
        output.set("scaleNodeIsNull", material.get("scaleNode").isNull());
        output.set("transparent", material.get("transparent"));
        output.set("setupPositionViewLength", material.get("setupPositionView").get("length"));
        output.set("versionBefore", versionBefore);
        output.set("versionAfterSame", versionAfterSame);
        output.set("versionAfterChange", versionAfterChange);
        output.set("sizeAttenuationAfterChange", material.get("sizeAttenuation"));
        output.set("simpleBuilt", simpleBuilt);
        if (!simpleBuilt) output.set("simpleFailure", JSValue(simpleFailure));
        output.set("simpleResultIsNode", simpleBuilt ? simpleResult.get("isNode") : JSValue(false));
        output.set("perspectiveBuilt", perspectiveBuilt);
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
