#include "materials/nodes/PointsNodeMaterial.h"

#include "core/JSValue.h"

#include <iostream>
#include <string>

int main() {
    try {
        const auto ctor = nativePointsNodeMaterialConstructor();
        const auto material = ctor.construct();

        const auto versionBefore = material.get("version").toNumber();
        auto handle = material;
        handle.set("alphaToCoverage", false);
        const auto versionAfterFirst = material.get("version").toNumber();
        handle.set("alphaToCoverage", false);
        const auto versionAfterSame = material.get("version").toNumber();
        handle.set("alphaToCoverage", true);
        const auto versionAfterChange = material.get("version").toNumber();

        bool positionBuilt = true;
        std::string positionFailure;
        JSValue position;
        try {
            position = material.get("setupPositionView").call(material, {});
        } catch (const std::exception& error) {
            positionBuilt = false;
            positionFailure = error.what();
        }

        // The two setupVertex branches are only recorded as built or raised: the base implementation needs
        // a full builder, so neither side is expected to run it against a stub.
        bool pointsBranchBuilt = true;
        try {
            auto pointsObject = JSValue::object(nullptr);
            pointsObject.set("isPoints", true);
            auto stubBuilder = JSValue::object(nullptr);
            stubBuilder.set("object", pointsObject);
            stubBuilder.set("material", JSValue::object(nullptr));
            material.get("setupVertex").call(material, {stubBuilder});
        } catch (...) {
            pointsBranchBuilt = false;
        }

        bool spriteBranchBuilt = true;
        try {
            auto spriteObject = JSValue::object(nullptr);
            spriteObject.set("isPoints", false);
            auto stubBuilder = JSValue::object(nullptr);
            stubBuilder.set("object", spriteObject);
            stubBuilder.set("material", JSValue::object(nullptr));
            material.get("setupVertex").call(material, {stubBuilder});
        } catch (...) {
            spriteBranchBuilt = false;
        }

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("type", material.get("type"));
        output.set("isPointsNodeMaterial", material.get("isPointsNodeMaterial"));
        output.set("isSpriteNodeMaterial", material.get("isSpriteNodeMaterial"));
        output.set("isNodeMaterial", material.get("isNodeMaterial"));
        output.set("sizeNodeIsNull", material.get("sizeNode").isNull());
        output.set("positionNodeIsNull", material.get("positionNode").isNull());
        output.set("size", material.get("size"));
        output.set("sizeAttenuation", material.get("sizeAttenuation"));
        output.set("setupPositionViewLength", material.get("setupPositionView").get("length"));
        output.set("setupVertexLength", material.get("setupVertex").get("length"));
        output.set("setupVertexSpriteLength", material.get("setupVertexSprite").get("length"));
        output.set("versionBefore", versionBefore);
        output.set("versionAfterFirst", versionAfterFirst);
        output.set("versionAfterSame", versionAfterSame);
        output.set("versionAfterChange", versionAfterChange);
        output.set("alphaToCoverageFlag", material.get("_useAlphaToCoverage"));
        output.set("positionBuilt", positionBuilt);
        if (!positionBuilt) output.set("positionFailure", JSValue(positionFailure));
        output.set("positionIsNode", positionBuilt ? position.get("isNode") : JSValue(false));
        output.set("pointsBranchBuilt", pointsBranchBuilt);
        output.set("spriteBranchBuilt", spriteBranchBuilt);
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
