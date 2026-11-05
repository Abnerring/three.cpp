// Native translation of pinned three.js Line2NodeMaterial.js (11738a).
#include "materials/nodes/Line2NodeMaterial.h"
#include "materials/nodes/Line2NodeMaterialSources.inc"
#include "materials/nodes/NodeMaterial.h"
#include "materials/LineDashedMaterial.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/core/AttributeNode.h"
#include "nodes/accessors/Camera.h"
#include "nodes/accessors/MaterialNode.h"
#include "nodes/accessors/ModelNode.h"
#include "nodes/accessors/Position.h"
#include "nodes/accessors/UV.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/display/ScreenNode.h"
#include "nodes/display/ViewportTextureNode.h"
#include "constants.h"
#include "utils.h"

namespace {
using namespace LightingNodeRuntime;
using namespace Line2NodeMaterialSource;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
JSValue withSource(JSValue value, const char* source) { value.setFunctionSourceText(source); return value; }

JSValue get_type(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    return JSValue("Line2NodeMaterial");
    return JSValue{};
}

JSValue setupDiffuseColor(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    auto builder = arg(args, 0);
    captures.at(1).getPrototype().get("setupDiffuseColor", self).call(self, {builder});
    invoke(module.get("diffuseColor").get("a"), "mulAssign")({module.get("alphaLine")});
    if ((JSValue(self.get("vertexColors").strictEquals(JSValue(true))).toBoolean() ? invoke(builder.get("geometry"), "hasAttribute")({JSValue("instanceColorStart")}) : JSValue(false)).toBoolean()) {
        auto instanceColorStart = module.get("attribute").call({}, {JSValue("instanceColorStart")});
        auto instanceColorEnd = module.get("attribute").call({}, {JSValue("instanceColorEnd")});
        auto instanceColor = invoke(invoke(module.get("positionGeometry").get("y"), "lessThan")({JSValue(0.5)}), "select")({instanceColorStart, instanceColorEnd});
        invoke(module.get("diffuseColor").get("rgb"), "mulAssign")({instanceColor});
    }
    if (self.get("transparent").toBoolean()) {
        invoke(module.get("diffuseColor").get("rgb"), "assign")({invoke(invoke(module.get("diffuseColor").get("rgb"), "mul")({module.get("diffuseColor").get("a")}), "add")({invoke(module.get("viewportOpaqueMipTexture").call({}, {}).get("rgb"), "mul")({invoke(module.get("diffuseColor").get("a"), "oneMinus")({})})})});
    }
    return JSValue{};
}

JSValue setupPosition(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    auto builder = arg(args, 0);
    auto localPosition = invoke(invoke(invoke(module.get("modelWorldMatrixInverse"), "mul")({module.get("cameraWorldMatrix")}), "mul")({module.get("cameraProjectionMatrixInverse")}), "mul")({module.get("mvpLine")});
    invoke(module.get("positionLocal"), "assign")({invoke(localPosition.get("xyz"), "div")({localPosition.get("w")})});
    if (invoke(builder, "needsPreviousData")({}).toBoolean()) {
        invoke(module.get("positionPrevious"), "assign")({module.get("positionLocal")});
    }
    return captures.at(1).getPrototype().get("setupPosition", self).call(self, {builder});
    return JSValue{};
}

JSValue get_lineColorNode(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    return self.get("colorNode");
    return JSValue{};
}

JSValue set_lineColorNode(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    auto value = arg(args, 0);
    module.get("warnOnce").call({}, {JSValue("Line2NodeMaterial: \"lineColorNode\" has been deprecated. Use \"colorNode\" instead.")});
    self.set("colorNode", value);
    return JSValue{};
}

JSValue get_worldUnits(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    return self.get("_useWorldUnits");
    return JSValue{};
}

JSValue set_worldUnits(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    auto value = arg(args, 0);
    if (JSValue(!self.get("_useWorldUnits").strictEquals(value)).toBoolean()) {
        self.set("_useWorldUnits", value);
        self.set("needsUpdate", JSValue(true));
    }
    return JSValue{};
}

JSValue get_dashed(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    return self.get("_useDash");
    return JSValue{};
}

JSValue set_dashed(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    auto value = arg(args, 0);
    if (JSValue(!self.get("_useDash").strictEquals(value)).toBoolean()) {
        self.set("_useDash", value);
        self.set("needsUpdate", JSValue(true));
    }
    return JSValue{};
}

JSValue get_alphaToCoverage(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    return self.get("_useAlphaToCoverage");
    return JSValue{};
}

JSValue set_alphaToCoverage(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    auto module = captures.at(0);
    auto value = arg(args, 0);
    if (JSValue(!self.get("_useAlphaToCoverage").strictEquals(value)).toBoolean()) {
        self.set("_useAlphaToCoverage", value);
        self.set("needsUpdate", JSValue(true));
    }
    return JSValue{};
}

} // namespace

JSValue nativeLine2NodeMaterialModule() {
    using namespace LightingNodeRuntime;
    using namespace Line2NodeMaterialSource;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "Line2NodeMaterial module");
    auto module = JSValue::object(nullptr);
    own(module, "NodeMaterial", nativeNodeMaterialConstructor());
    own(module, "dashSize", nativePropertyNodeBinding("dashSize"));
    own(module, "diffuseColor", nativePropertyNodeBinding("diffuseColor"));
    own(module, "gapSize", nativePropertyNodeBinding("gapSize"));
    own(module, "varyingProperty", nativePropertyNodeBinding("varyingProperty"));
    own(module, "attribute", nativeAttributeNodeBinding("attribute"));
    own(module, "cameraProjectionMatrix", nativeCameraBinding("cameraProjectionMatrix"));
    own(module, "cameraProjectionMatrixInverse", nativeCameraBinding("cameraProjectionMatrixInverse"));
    own(module, "cameraWorldMatrix", nativeCameraBinding("cameraWorldMatrix"));
    own(module, "materialLineScale", nativeMaterialNodeBinding("materialLineScale"));
    own(module, "materialLineDashSize", nativeMaterialNodeBinding("materialLineDashSize"));
    own(module, "materialLineGapSize", nativeMaterialNodeBinding("materialLineGapSize"));
    own(module, "materialLineDashOffset", nativeMaterialNodeBinding("materialLineDashOffset"));
    own(module, "materialLineWidth", nativeMaterialNodeBinding("materialLineWidth"));
    own(module, "modelViewMatrix", nativeModelNodeBinding("modelViewMatrix"));
    own(module, "modelWorldMatrixInverse", nativeModelNodeBinding("modelWorldMatrixInverse"));
    own(module, "positionGeometry", nativePositionBinding("positionGeometry"));
    own(module, "positionLocal", nativePositionBinding("positionLocal"));
    own(module, "positionPrevious", nativePositionBinding("positionPrevious"));
    own(module, "mix", nativeMathNodeBinding("mix"));
    own(module, "smoothstep", nativeMathNodeBinding("smoothstep"));
    own(module, "Fn", nativeTSLBaseBinding("Fn"));
    own(module, "float", nativeTSLBaseBinding("float"));
    own(module, "vec2", nativeTSLBaseBinding("vec2"));
    own(module, "vec3", nativeTSLBaseBinding("vec3"));
    own(module, "vec4", nativeTSLBaseBinding("vec4"));
    own(module, "If", nativeTSLBaseBinding("If"));
    own(module, "uv", nativeUVBinding("uv"));
    own(module, "screenDPR", nativeScreenNodeBinding("screenDPR"));
    own(module, "viewport", nativeScreenNodeBinding("viewport"));
    own(module, "viewportOpaqueMipTexture", nativeViewportTextureNodeBinding("viewportOpaqueMipTexture"));
    own(module, "LineDashedMaterial", nativeLineDashedMaterialConstructor());
    own(module, "NoBlending", JSValue(NoBlending));
    own(module, "warnOnce", ThreeUtils::nativeWarnOnceFunction());
    own(module, "_defaultValues", module.get("LineDashedMaterial").construct({}));
    own(module, "worldStart", module.get("varyingProperty").call({}, {JSValue("vec3"), JSValue("worldStart")}));
    own(module, "worldEnd", module.get("varyingProperty").call({}, {JSValue("vec3"), JSValue("worldEnd")}));
    own(module, "lineDistance", module.get("varyingProperty").call({}, {JSValue("float"), JSValue("lineDistance")}));
    own(module, "worldPos", module.get("varyingProperty").call({}, {JSValue("vec4"), JSValue("worldPos")}));
    own(module, "trimSegmentAlpha", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        const auto destructured1 = arg(args, 0);
        auto start = destructured1.get("start");
        auto end = destructured1.get("end");
        auto a = invoke(invoke(module.get("cameraProjectionMatrix"), "element")({JSValue(2.0)}), "element")({JSValue(2.0)});
        auto b = invoke(invoke(module.get("cameraProjectionMatrix"), "element")({JSValue(3.0)}), "element")({JSValue(2.0)});
        auto nearEstimate = invoke(invoke(a, "greaterThan")({JSValue(0.0)}), "select")({invoke(invoke(b, "negate")({}), "div")({invoke(a, "add")({JSValue(1.0)})}), invoke(invoke(b, "mul")({JSValue(-JSValue(0.5).toNumber())}), "div")({a})});
        return invoke(invoke(nearEstimate, "sub")({start.get("z")}), "div")({invoke(end.get("z"), "sub")({start.get("z")})});
        return JSValue{};
    }, {module}, "", 1), callback1Source), ([&]() { auto object1 = JSValue::object(); own(object1, "start", JSValue("vec4")); own(object1, "end", JSValue("vec4")); own(object1, "return", JSValue("float")); return object1; }())}));
    own(module, "closestLineToLine", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        const auto destructured1 = arg(args, 0);
        auto p1 = destructured1.get("p1");
        auto p2 = destructured1.get("p2");
        auto p3 = destructured1.get("p3");
        auto p4 = destructured1.get("p4");
        auto p13 = invoke(p1, "sub")({p3});
        auto p43 = invoke(p4, "sub")({p3});
        auto p21 = invoke(p2, "sub")({p1});
        auto d1343 = invoke(p13, "dot")({p43});
        auto d4321 = invoke(p43, "dot")({p21});
        auto d1321 = invoke(p13, "dot")({p21});
        auto d4343 = invoke(p43, "dot")({p43});
        auto d2121 = invoke(p21, "dot")({p21});
        auto denom = invoke(invoke(d2121, "mul")({d4343}), "sub")({invoke(d4321, "mul")({d4321})});
        auto numer = invoke(invoke(d1343, "mul")({d4321}), "sub")({invoke(d1321, "mul")({d4343})});
        auto mua = invoke(invoke(numer, "div")({denom}), "clamp")({});
        auto mub = invoke(invoke(invoke(d1343, "add")({invoke(d4321, "mul")({mua})}), "div")({d4343}), "clamp")({});
        return module.get("vec2").call({}, {mua, mub});
        return JSValue{};
    }, {module}, "", 1), callback2Source), ([&]() { auto object1 = JSValue::object(); own(object1, "p1", JSValue("vec3")); own(object1, "p2", JSValue("vec3")); own(object1, "p3", JSValue("vec3")); own(object1, "p4", JSValue("vec3")); own(object1, "return", JSValue("vec2")); return object1; }())}));
    own(module, "mvpLine", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        const auto destructured1 = arg(args, 0);
        auto material = destructured1.get("material");
        auto useDash = material.get("_useDash");
        auto useWorldUnits = material.get("_useWorldUnits");
        auto instanceStart = module.get("attribute").call({}, {JSValue("instanceStart")});
        auto instanceEnd = module.get("attribute").call({}, {JSValue("instanceEnd")});
        auto start = invoke(module.get("vec4").call({}, {invoke(module.get("modelViewMatrix"), "mul")({module.get("vec4").call({}, {instanceStart, JSValue(1.0)})})}), "toVar")({JSValue("start")});
        auto end = invoke(module.get("vec4").call({}, {invoke(module.get("modelViewMatrix"), "mul")({module.get("vec4").call({}, {instanceEnd, JSValue(1.0)})})}), "toVar")({JSValue("end")});
        JSValue distanceStart;
        JSValue distanceEnd;
        if (useDash.toBoolean()) {
            distanceStart = invoke(module.get("float").call({}, {module.get("attribute").call({}, {JSValue("instanceDistanceStart")})}), "toVar")({JSValue("distanceStart")});
            distanceEnd = invoke(module.get("float").call({}, {module.get("attribute").call({}, {JSValue("instanceDistanceEnd")})}), "toVar")({JSValue("distanceEnd")});
        }
        if (useWorldUnits.toBoolean()) {
            invoke(module.get("worldStart"), "assign")({start.get("xyz")});
            invoke(module.get("worldEnd"), "assign")({end.get("xyz")});
        }
        auto aspect = invoke(module.get("viewport").get("z"), "div")({module.get("viewport").get("w")});
        auto perspective = invoke(invoke(invoke(module.get("cameraProjectionMatrix"), "element")({JSValue(2.0)}), "element")({JSValue(3.0)}), "equal")({JSValue(-JSValue(1.0).toNumber())});
        module.get("If").call({}, {perspective, withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
            auto module = captures.at(0);
            auto useDash = captures.at(1);
            auto start = captures.at(2);
            auto end = captures.at(3);
            auto distanceStart = captures.at(4);
            auto distanceEnd = captures.at(5);
            invoke(module.get("If").call({}, {invoke(invoke(start.get("z"), "lessThan")({JSValue(0.0)}), "and")({invoke(end.get("z"), "greaterThan")({JSValue(0.0)})}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto useDash = captures.at(1);
                auto start = captures.at(2);
                auto end = captures.at(3);
                auto distanceStart = captures.at(4);
                auto distanceEnd = captures.at(5);
                auto alpha = module.get("trimSegmentAlpha").call({}, {([&]() { auto object1 = JSValue::object(); own(object1, "start", start); own(object1, "end", end); return object1; }())});
                invoke(end, "assign")({module.get("vec4").call({}, {module.get("mix").call({}, {start.get("xyz"), end.get("xyz"), alpha}), end.get("w")})});
                if (useDash.toBoolean()) {
                    invoke(distanceEnd, "assign")({module.get("mix").call({}, {distanceStart, distanceEnd, alpha})});
                }
                return JSValue{};
            }, {module, useDash, start, end, distanceStart, distanceEnd}, "", 0), callback5Source)}), "ElseIf")({invoke(invoke(end.get("z"), "lessThan")({JSValue(0.0)}), "and")({invoke(start.get("z"), "greaterThanEqual")({JSValue(0.0)})}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto useDash = captures.at(1);
                auto start = captures.at(2);
                auto end = captures.at(3);
                auto distanceStart = captures.at(4);
                auto distanceEnd = captures.at(5);
                auto alpha = module.get("trimSegmentAlpha").call({}, {([&]() { auto object1 = JSValue::object(); own(object1, "start", end); own(object1, "end", start); return object1; }())});
                invoke(start, "assign")({module.get("vec4").call({}, {module.get("mix").call({}, {end.get("xyz"), start.get("xyz"), alpha}), start.get("w")})});
                if (useDash.toBoolean()) {
                    invoke(distanceStart, "assign")({module.get("mix").call({}, {distanceEnd, distanceStart, alpha})});
                }
                return JSValue{};
            }, {module, useDash, start, end, distanceStart, distanceEnd}, "", 0), callback6Source)});
            return JSValue{};
        }, {module, useDash, start, end, distanceStart, distanceEnd}, "", 0), callback4Source)});
        if (useDash.toBoolean()) {
            auto dashScaleNode = (material.get("dashScaleNode").toBoolean() ? module.get("float").call({}, {material.get("dashScaleNode")}) : module.get("materialLineScale"));
            auto offsetNode = (material.get("offsetNode").toBoolean() ? module.get("float").call({}, {material.get("offsetNode")}) : module.get("materialLineDashOffset"));
            auto lineDist = invoke(invoke(module.get("positionGeometry").get("y"), "lessThan")({JSValue(0.5)}), "select")({invoke(dashScaleNode, "mul")({distanceStart}), invoke(dashScaleNode, "mul")({distanceEnd})});
            lineDist = invoke(lineDist, "add")({offsetNode});
            invoke(module.get("lineDistance"), "assign")({lineDist});
        }
        auto clipStart = invoke(module.get("cameraProjectionMatrix"), "mul")({start});
        auto clipEnd = invoke(module.get("cameraProjectionMatrix"), "mul")({end});
        auto ndcStart = invoke(clipStart.get("xyz"), "div")({clipStart.get("w")});
        auto ndcEnd = invoke(clipEnd.get("xyz"), "div")({clipEnd.get("w")});
        auto dir = invoke(invoke(ndcEnd.get("xy"), "sub")({ndcStart.get("xy")}), "toVar")({});
        invoke(dir.get("x"), "assign")({invoke(dir.get("x"), "mul")({aspect})});
        invoke(dir, "assign")({invoke(dir, "normalize")({})});
        auto clip = invoke(module.get("vec4").call({}, {}), "toVar")({});
        if (useWorldUnits.toBoolean()) {
            auto worldDir = invoke(invoke(end.get("xyz"), "sub")({start.get("xyz")}), "normalize")({});
            auto tmpFwd = invoke(perspective, "select")({invoke(module.get("mix").call({}, {start.get("xyz"), end.get("xyz"), JSValue(0.5)}), "normalize")({}), module.get("vec3").call({}, {JSValue(0.0), JSValue(0.0), JSValue(-JSValue(1.0).toNumber())})});
            auto worldUp = invoke(invoke(worldDir, "cross")({tmpFwd}), "normalize")({});
            auto worldFwd = invoke(worldDir, "cross")({worldUp});
            invoke(module.get("worldPos"), "assign")({invoke(invoke(module.get("positionGeometry").get("y"), "lessThan")({JSValue(0.5)}), "select")({start, end})});
            auto hw = invoke(module.get("materialLineWidth"), "mul")({JSValue(0.5)});
            invoke(module.get("worldPos"), "addAssign")({module.get("vec4").call({}, {invoke(invoke(module.get("positionGeometry").get("x"), "lessThan")({JSValue(0.0)}), "select")({invoke(worldUp, "mul")({hw}), invoke(invoke(worldUp, "mul")({hw}), "negate")({})}), JSValue(0.0)})});
            if (JSValue(!useDash.toBoolean()).toBoolean()) {
                invoke(module.get("worldPos"), "addAssign")({module.get("vec4").call({}, {invoke(invoke(module.get("positionGeometry").get("y"), "lessThan")({JSValue(0.5)}), "select")({invoke(invoke(worldDir, "mul")({hw}), "negate")({}), invoke(worldDir, "mul")({hw})}), JSValue(0.0)})});
                invoke(module.get("worldPos"), "addAssign")({module.get("vec4").call({}, {invoke(worldFwd, "mul")({hw}), JSValue(0.0)})});
                module.get("If").call({}, {invoke(invoke(module.get("positionGeometry").get("y"), "greaterThan")({JSValue(1.0)}), "or")({invoke(module.get("positionGeometry").get("y"), "lessThan")({JSValue(0.0)})}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                    auto module = captures.at(0);
                    auto worldFwd = captures.at(1);
                    auto hw = captures.at(2);
                    invoke(module.get("worldPos"), "subAssign")({module.get("vec4").call({}, {invoke(invoke(worldFwd, "mul")({JSValue(2.0)}), "mul")({hw}), JSValue(0.0)})});
                    return JSValue{};
                }, {module, worldFwd, hw}, "", 0), callback7Source)});
            }
            invoke(clip, "assign")({invoke(module.get("cameraProjectionMatrix"), "mul")({module.get("worldPos")})});
            auto clipPose = invoke(module.get("vec3").call({}, {}), "toVar")({});
            invoke(clipPose, "assign")({invoke(invoke(module.get("positionGeometry").get("y"), "lessThan")({JSValue(0.5)}), "select")({ndcStart, ndcEnd})});
            invoke(clip.get("z"), "assign")({invoke(clipPose.get("z"), "mul")({clip.get("w")})});
        } else {
            auto offset = invoke(module.get("vec2").call({}, {dir.get("y"), invoke(dir.get("x"), "negate")({})}), "toVar")({JSValue("offset")});
            invoke(dir.get("x"), "assign")({invoke(dir.get("x"), "div")({aspect})});
            invoke(offset.get("x"), "assign")({invoke(offset.get("x"), "div")({aspect})});
            invoke(offset, "assign")({invoke(invoke(module.get("positionGeometry").get("x"), "lessThan")({JSValue(0.0)}), "select")({invoke(offset, "negate")({}), offset})});
            invoke(module.get("If").call({}, {invoke(module.get("positionGeometry").get("y"), "lessThan")({JSValue(0.0)}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto dir = captures.at(1);
                auto offset = captures.at(2);
                invoke(offset, "assign")({invoke(offset, "sub")({dir})});
                return JSValue{};
            }, {module, dir, offset}, "", 0), callback8Source)}), "ElseIf")({invoke(module.get("positionGeometry").get("y"), "greaterThan")({JSValue(1.0)}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto dir = captures.at(1);
                auto offset = captures.at(2);
                invoke(offset, "assign")({invoke(offset, "add")({dir})});
                return JSValue{};
            }, {module, dir, offset}, "", 0), callback9Source)});
            invoke(offset, "assign")({invoke(offset, "mul")({module.get("materialLineWidth")})});
            invoke(offset, "assign")({invoke(offset, "div")({invoke(module.get("viewport").get("w"), "div")({module.get("screenDPR")})})});
            invoke(clip, "assign")({invoke(invoke(module.get("positionGeometry").get("y"), "lessThan")({JSValue(0.5)}), "select")({clipStart, clipEnd})});
            invoke(offset, "assign")({invoke(offset, "mul")({clip.get("w")})});
            invoke(clip, "assign")({invoke(clip, "add")({module.get("vec4").call({}, {offset, JSValue(0.0), JSValue(0.0)})})});
        }
        return clip;
        return JSValue{};
    }, {module}, "", 1), callback3Source)}).call({}, {}));
    own(module, "alphaLine", module.get("Fn").call({}, {withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(0);
        const auto destructured1 = arg(args, 0);
        auto material = destructured1.get("material");
        auto renderer = destructured1.get("renderer");
        auto useAlphaToCoverage = material.get("_useAlphaToCoverage");
        auto useDash = material.get("_useDash");
        auto useWorldUnits = material.get("_useWorldUnits");
        auto vUv = module.get("uv").call({}, {});
        if (useDash.toBoolean()) {
            auto dashSizeNode = (material.get("dashSizeNode").toBoolean() ? module.get("float").call({}, {material.get("dashSizeNode")}) : module.get("materialLineDashSize"));
            auto gapSizeNode = (material.get("gapSizeNode").toBoolean() ? module.get("float").call({}, {material.get("gapSizeNode")}) : module.get("materialLineGapSize"));
            invoke(module.get("dashSize"), "assign")({dashSizeNode});
            invoke(module.get("gapSize"), "assign")({gapSizeNode});
            invoke(invoke(invoke(vUv.get("y"), "lessThan")({JSValue(-JSValue(1.0).toNumber())}), "or")({invoke(vUv.get("y"), "greaterThan")({JSValue(1.0)})}), "discard")({});
            invoke(invoke(invoke(module.get("lineDistance"), "mod")({invoke(module.get("dashSize"), "add")({module.get("gapSize")})}), "greaterThan")({module.get("dashSize")}), "discard")({});
        }
        auto alpha = invoke(module.get("float").call({}, {JSValue(1.0)}), "toVar")({JSValue("alpha")});
        if (useWorldUnits.toBoolean()) {
            auto len = invoke(module.get("float").call({}, {}), "toVar")({});
            auto orthographic = invoke(invoke(invoke(invoke(module.get("cameraProjectionMatrix"), "element")({JSValue(2.0)}), "element")({JSValue(3.0)}), "notEqual")({JSValue(-JSValue(1.0).toNumber())}), "toConst")({});
            invoke(module.get("If").call({}, {orthographic, withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto len = captures.at(1);
                auto lineDir = invoke(module.get("worldEnd").get("xy"), "sub")({module.get("worldStart").get("xy")});
                auto t = invoke(invoke(invoke(invoke(module.get("worldPos").get("xy"), "sub")({module.get("worldStart").get("xy")}), "dot")({lineDir}), "div")({invoke(lineDir, "dot")({lineDir})}), "clamp")({});
                invoke(len, "assign")({invoke(invoke(invoke(module.get("worldStart").get("xy"), "add")({invoke(lineDir, "mul")({t})}), "sub")({module.get("worldPos").get("xy")}), "length")({})});
                return JSValue{};
            }, {module, len}, "", 0), callback11Source)}), "Else")({withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                auto module = captures.at(0);
                auto len = captures.at(1);
                auto rayEnd = invoke(invoke(module.get("worldPos").get("xyz"), "normalize")({}), "mul")({JSValue(100000.0)});
                auto lineDir = invoke(module.get("worldEnd"), "sub")({module.get("worldStart")});
                auto params = module.get("closestLineToLine").call({}, {([&]() { auto object1 = JSValue::object(); own(object1, "p1", module.get("worldStart")); own(object1, "p2", module.get("worldEnd")); own(object1, "p3", module.get("vec3").call({}, {JSValue(0.0), JSValue(0.0), JSValue(0.0)})); own(object1, "p4", rayEnd); return object1; }())});
                auto p1 = invoke(module.get("worldStart"), "add")({invoke(lineDir, "mul")({params.get("x")})});
                auto p2 = invoke(rayEnd, "mul")({params.get("y")});
                auto delta = invoke(p1, "sub")({p2});
                invoke(len, "assign")({invoke(delta, "length")({})});
                return JSValue{};
            }, {module, len}, "", 0), callback12Source)});
            auto norm = invoke(len, "div")({module.get("materialLineWidth")});
            if (JSValue(!useDash.toBoolean()).toBoolean()) {
                if ((useAlphaToCoverage.toBoolean() ? JSValue(greater(renderer.get("currentSamples"), JSValue(0.0))) : JSValue(false)).toBoolean()) {
                    auto dnorm = invoke(norm, "fwidth")({});
                    invoke(alpha, "assign")({invoke(module.get("smoothstep").call({}, {invoke(invoke(dnorm, "negate")({}), "add")({JSValue(0.5)}), invoke(dnorm, "add")({JSValue(0.5)}), norm}), "oneMinus")({})});
                } else {
                    invoke(invoke(norm, "greaterThan")({JSValue(0.5)}), "discard")({});
                }
            }
        } else {
            if ((useAlphaToCoverage.toBoolean() ? JSValue(greater(renderer.get("currentSamples"), JSValue(0.0))) : JSValue(false)).toBoolean()) {
                auto a = vUv.get("x");
                auto b = invoke(invoke(vUv.get("y"), "greaterThan")({JSValue(0.0)}), "select")({invoke(vUv.get("y"), "sub")({JSValue(1.0)}), invoke(vUv.get("y"), "add")({JSValue(1.0)})});
                auto len2 = invoke(invoke(a, "mul")({a}), "add")({invoke(b, "mul")({b})});
                auto dlen = invoke(module.get("float").call({}, {invoke(len2, "fwidth")({})}), "toVar")({JSValue("dlen")});
                module.get("If").call({}, {invoke(invoke(vUv.get("y"), "abs")({}), "greaterThan")({JSValue(1.0)}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                    auto module = captures.at(0);
                    auto alpha = captures.at(1);
                    auto len2 = captures.at(2);
                    auto dlen = captures.at(3);
                    invoke(alpha, "assign")({invoke(module.get("smoothstep").call({}, {invoke(dlen, "oneMinus")({}), invoke(dlen, "add")({JSValue(1.0)}), len2}), "oneMinus")({})});
                    return JSValue{};
                }, {module, alpha, len2, dlen}, "", 0), callback13Source)});
            } else {
                module.get("If").call({}, {invoke(invoke(vUv.get("y"), "abs")({}), "greaterThan")({JSValue(1.0)}), withSource(JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                    auto module = captures.at(0);
                    auto vUv = captures.at(1);
                    auto a = vUv.get("x");
                    auto b = invoke(invoke(vUv.get("y"), "greaterThan")({JSValue(0.0)}), "select")({invoke(vUv.get("y"), "sub")({JSValue(1.0)}), invoke(vUv.get("y"), "add")({JSValue(1.0)})});
                    auto len2 = invoke(invoke(a, "mul")({a}), "add")({invoke(b, "mul")({b})});
                    invoke(invoke(len2, "greaterThan")({JSValue(1.0)}), "discard")({});
                    return JSValue{};
                }, {module, vUv}, "", 0), callback14Source)});
            }
        }
        return alpha;
        return JSValue{};
    }, {module}, "", 1), callback10Source)}).call({}, {}));
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(+[](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        auto module = captures.at(1);
        auto argument1 = arg(args, 0);
        if (argument1.isUndefined()) argument1 = ([&]() { auto object2 = JSValue::object();  return object2; }());
        auto parameters = argument1;
        auto self = superConstruct(newTarget, captures);
        self.set("isLine2NodeMaterial", JSValue(true));
        invoke(self, "setDefaultValues")({module.get("_defaultValues")});
        self.set("vertexColors", parameters.get("vertexColors"));
        self.set("dashOffset", JSValue(0.0));
        self.set("offsetNode", JSValue(nullptr));
        self.set("dashScaleNode", JSValue(nullptr));
        self.set("dashSizeNode", JSValue(nullptr));
        self.set("gapSizeNode", JSValue(nullptr));
        self.set("blending", module.get("NoBlending"));
        self.set("_useDash", parameters.get("dashed"));
        self.set("_useAlphaToCoverage", JSValue(true));
        self.set("_useWorldUnits", JSValue(false));
        invoke(self, "setValues")({parameters});
        return self;
    }, {holder, module}, "Line2NodeMaterial", 0);
    holder.set("constructor", ctor);
    const auto base = module.get("NodeMaterial");
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    ctor.setFunctionSourceText(classSource);
    method(prototype, "setupDiffuseColor", 1, setupDiffuseColor, {module, prototype}, setupDiffuseColorSource);
    method(prototype, "setupPosition", 1, setupPosition, {module, prototype}, setupPositionSource);
    { JSPropertyDescriptor d; d.configurable = true;
      d.getter = withSource(JSValue::function(get_type, {module, prototype}, "get type", 0), get_typeSource);
      ctor.defineProperty("type", d); }
    { JSPropertyDescriptor d; d.configurable = true;
      d.getter = withSource(JSValue::function(get_lineColorNode, {module, prototype}, "get lineColorNode", 0), get_lineColorNodeSource);
      d.setter = withSource(JSValue::function(set_lineColorNode, {module, prototype}, "set lineColorNode", 1), set_lineColorNodeSource);
      prototype.defineProperty("lineColorNode", d); }
    { JSPropertyDescriptor d; d.configurable = true;
      d.getter = withSource(JSValue::function(get_worldUnits, {module, prototype}, "get worldUnits", 0), get_worldUnitsSource);
      d.setter = withSource(JSValue::function(set_worldUnits, {module, prototype}, "set worldUnits", 1), set_worldUnitsSource);
      prototype.defineProperty("worldUnits", d); }
    { JSPropertyDescriptor d; d.configurable = true;
      d.getter = withSource(JSValue::function(get_dashed, {module, prototype}, "get dashed", 0), get_dashedSource);
      d.setter = withSource(JSValue::function(set_dashed, {module, prototype}, "set dashed", 1), set_dashedSource);
      prototype.defineProperty("dashed", d); }
    { JSPropertyDescriptor d; d.configurable = true;
      d.getter = withSource(JSValue::function(get_alphaToCoverage, {module, prototype}, "get alphaToCoverage", 0), get_alphaToCoverageSource);
      d.setter = withSource(JSValue::function(set_alphaToCoverage, {module, prototype}, "set alphaToCoverage", 1), set_alphaToCoverageSource);
      prototype.defineProperty("alphaToCoverage", d); }
    own(module, "default", ctor);
    module.retainForHeapLifetime(); lexical() = JSWeakValue(module);
    auto exports = JSValue::object(); own(exports, "default", ctor);
    exports.retainForHeapLifetime(); weak = JSWeakValue(exports); return exports;
}
JSValue nativeLine2NodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeLine2NodeMaterialModule(); (void)keeper;
    return name == "default" ? lexical().lock()->get(name) : JSValue{};
}
JSValue nativeLine2NodeMaterialConstructor() { return nativeLine2NodeMaterialBinding("default"); }
JSValue nativeLine2NodeMaterialHelper(const std::string& name) {
    const auto keeper = nativeLine2NodeMaterialModule(); (void)keeper;
    return lexical().lock()->get(name);
}
