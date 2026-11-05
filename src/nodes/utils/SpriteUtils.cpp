#include "nodes/utils/SpriteUtils.h"

#include "nodes/accessors/Camera.h"
#include "nodes/accessors/ModelNode.h"
#include "nodes/accessors/Position.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// defined( value ) in TSL: the value is neither undefined nor null.
bool defined(const JSValue& value) { return !value.isUndefined() && !value.isNull(); }

void setComponent(const JSValue& node, int row, int column, const JSValue& value) {
    // The source writes through the matrix index chain: worldMatrix[ row ][ column ] = value.
    auto rowNode = node.get(JSValue(static_cast<double>(row)));
    rowNode.set(JSValue(static_cast<double>(column)), value);
}

void setRow(const JSValue& node, int row, const JSValue& x, const JSValue& y, const JSValue& z) {
    setComponent(node, row, 0, x);
    setComponent(node, row, 1, y);
    setComponent(node, row, 2, z);
}

// export const billboarding = Fn( ( { position = null, horizontal = true, vertical = false, horizontalRotation = false } ) => { ... } )
JSValue billboarding(const JSValue&, const Args& args, const Args&) {
    const auto options = arg(args, 0, JSValue::object(nullptr));
    const auto positionOption = options.get("position");
    const auto position = positionOption.isUndefined() ? JSValue(nullptr) : positionOption;
    auto horizontal = options.get("horizontal");
    if (horizontal.isUndefined()) horizontal = JSValue(true);
    auto vertical = options.get("vertical");
    if (vertical.isUndefined()) vertical = JSValue(false);
    auto horizontalRotation = options.get("horizontalRotation");
    if (horizontalRotation.isUndefined()) horizontalRotation = JSValue(false);

    const auto modelWorldMatrix = nativeModelNodeBinding("modelWorldMatrix");
    const auto cameraViewMatrix = nativeCameraBinding("cameraViewMatrix");
    const auto cameraProjectionMatrix = nativeCameraBinding("cameraProjectionMatrix");
    const auto cameraPosition = nativeCameraBinding("cameraPosition");
    const auto positionGeometry = nativePositionBinding("positionGeometry");
    const auto positionWorld = nativePositionBinding("positionWorld");

    JSValue center;
    if (!position.isNull()) {
        center = invoke(nativeTSLCoreBinding("nodeObject"), "call")({JSValue::object(nullptr), position});
    } else {
        const auto scaled =
            invoke(modelWorldMatrix, "mul")({nativeTSLBaseBinding("vec4").call({}, {positionGeometry, JSValue(0.0)})});
        center = invoke(positionWorld, "sub")({scaled.get("xyz")});
    }

    const auto worldMatrix = invoke(modelWorldMatrix, "toVar")({});
    setRow(worldMatrix, 3, center.get("x"), center.get("y"), center.get("z"));

    auto modelViewMatrix = invoke(cameraViewMatrix, "mul")({worldMatrix});
    const auto scaleX = invoke(modelWorldMatrix.get(JSValue(0.0)), "length")({});
    const auto scaleY = invoke(modelWorldMatrix.get(JSValue(1.0)), "length")({});
    const auto scaleZ = invoke(modelWorldMatrix.get(JSValue(2.0)), "length")({});

    JSValue right, up, forward;
    if (defined(horizontalRotation)) {
        const auto worldPosition = worldMatrix.get(JSValue(3.0)).get("xyz");
        const auto look = invoke(cameraPosition, "sub")({worldPosition});
        const auto lookXZ = invoke(
            nativeTSLBaseBinding("vec3").call({}, {look.get("x"), JSValue(0.0), look.get("z")}),
            "normalize")({});
        const auto rightW = nativeTSLBaseBinding("vec3").call(
            {}, {lookXZ.get("z"), JSValue(0.0), invoke(lookXZ.get("x"), "negate")()});
        right = invoke(
            invoke(cameraViewMatrix, "mul")({nativeTSLBaseBinding("vec4").call({}, {rightW, JSValue(0.0)})})
                .get("xyz"),
            "mul")({scaleX});
        up = invoke(cameraViewMatrix.get(JSValue(1.0)).get("xyz"), "mul")({scaleY});
        forward = invoke(
            invoke(cameraViewMatrix, "mul")({nativeTSLBaseBinding("vec4").call({}, {lookXZ, JSValue(0.0)})})
                .get("xyz"),
            "mul")({scaleZ});
    } else {
        if (defined(horizontal)) {
            right = nativeTSLBaseBinding("vec3").call({}, {scaleX, JSValue(0.0), JSValue(0.0)});
        }
        if (defined(vertical)) {
            up = nativeTSLBaseBinding("vec3").call({}, {JSValue(0.0), scaleY, JSValue(0.0)});
        }
        forward = nativeTSLBaseBinding("vec3").call({}, {JSValue(0.0), JSValue(0.0), JSValue(1.0)});
    }

    if (right.toBoolean() && !right.isUndefined()) {
        setRow(modelViewMatrix, 0, right.get("x"), right.get("y"), right.get("z"));
    }
    if (up.toBoolean() && !up.isUndefined()) {
        setRow(modelViewMatrix, 1, up.get("x"), up.get("y"), up.get("z"));
    }
    if (forward.toBoolean() && !forward.isUndefined()) {
        setRow(modelViewMatrix, 2, forward.get("x"), forward.get("y"), forward.get("z"));
    }

    const auto projected = invoke(cameraProjectionMatrix, "mul")({modelViewMatrix});
    return invoke(projected, "mul")({positionGeometry});
}
}  // namespace

JSValue nativeSpriteUtilsModule() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "SpriteUtils module");
    nativeCameraModule();
    nativeModelNodeModule();
    nativePositionModule();
    nativeTSLBase();
    nativeTSLCore();

    auto bindings = JSValue::object(nullptr);
    // Fn(...) returns a rest parameter wrapper, so the exported helper has length zero.
    auto billboardingFn = JSValue::function(billboarding, {}, "billboarding", 0);
    own(bindings, "billboarding", billboardingFn);
    return publish(bindings, lexical(), weak);
}

JSValue nativeSpriteUtilsBinding(const std::string& name) {
    const auto keeper = nativeSpriteUtilsModule();
    (void) keeper;
    return lexical().lock()->get(name);
}
