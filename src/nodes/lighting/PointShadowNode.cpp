#include "nodes/lighting/PointShadowNode.h"

#include "constants.h"
#include "core/JSWeakMap.h"
#include "core/JSGlobal.h"
#include "core/NativeObject.h"
#include "nodes/accessors/CubeTextureNode.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/lighting/ShadowNode.h"
#include "math/Color.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "textures/CubeDepthTexture.h"

#include <array>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

[[noreturn]] JSValue notPorted(const char* name) {
    throw std::runtime_error(std::string("PointShadowNode: ") + name + " is not ported yet");
}

// getShadowFilterFn( type ) { return type === BasicShadowMap ? BasicPointShadowFilter : PointShadowFilter; }
// Stage A resolves the basic variant through the cube texture compare and leaves the sampled variant raising.
JSValue getShadowFilterFn(const JSValue&, const Args& args, const Args&) {
    const auto type = arg(args, 0).toNumber();
    if (type == static_cast<double>(BasicShadowMap)) {
        return JSValue::function(
            +[](const JSValue&, const Args& callbackArgs, const Args&) -> JSValue {
                const auto inputs = callbackArgs.at(0);
                const auto depthTexture = inputs.get("depthTexture");
                const auto bd3D = inputs.get("bd3D");
                const auto dp = inputs.get("dp");
                const auto sampled =
                    nativeCubeTextureNodeBinding("cubeTexture").call({}, {depthTexture, bd3D});
                return invoke(sampled, "compare")({dp});
            });
    }
    return notPorted("getShadowFilterFn (PointShadowFilter)");
}

// setupShadowCoord( builder, shadowPosition ) { return shadowPosition; }
JSValue setupShadowCoord(const JSValue&, const Args& args, const Args&) { return arg(args, 1); }

// setupRenderTarget( shadow, builder ) { new CubeDepthTexture( width ) + createCubeRenderTarget( width ) }
JSValue setupRenderTarget(const JSValue&, const Args& args, const Args&) {
    const auto shadow = arg(args, 0);
    const auto builder = arg(args, 1);
    const auto mapSize = shadow.get("mapSize");
    const auto renderer = builder.get("renderer");

    auto depthTexture = nativeTexture(
        std::make_shared<CubeDepthTexture>(mapSize.get("width").toNumber()));
    depthTexture.set("name", JSValue("PointShadowDepthTexture"));
    const auto compareFunction = renderer.get("reversedDepthBuffer").toBoolean()
                                     ? JSValue(static_cast<double>(GreaterEqualCompare))
                                     : JSValue(static_cast<double>(LessEqualCompare));
    depthTexture.set("compareFunction", compareFunction);

    auto shadowMap = builder.get("createCubeRenderTarget").call(builder, {mapSize.get("width")});
    auto texture = shadowMap.get("texture");
    texture.set("name", JSValue("PointShadowMap"));
    shadowMap.set("depthTexture", depthTexture);

    auto result = JSValue::object(nullptr);
    result.set("shadowMap", shadowMap);
    result.set("depthTexture", depthTexture);
    return result;
}

JSValue setupShadowFilter(const JSValue&, const Args&, const Args&) {
    return notPorted("setupShadowFilter");
}
// Module level cube face tables, mirroring the pinned WebGPU/WebGL direction and up arrays.
JSWeakValue& cubeTablesCache() { static thread_local JSWeakValue weak; return weak; }

JSValue vector3(double x, double y, double z) {
    return nativeVector3Constructor().construct({JSValue(x), JSValue(y), JSValue(z)});
}

JSValue cubeFaceTables() {
    if (const auto cached = cubeTablesCache().lock(); cached && cached->toBoolean()) return *cached;
    auto tables = JSValue::object(nullptr);
    auto makeList = [](std::initializer_list<std::array<double, 3>> values) {
        auto list = JSValue::array(6);
        std::size_t index = 0;
        for (const auto& value : values) {
            list.set(index++, vector3(value[0], value[1], value[2]));
        }
        return list;
    };
    tables.set("directionsWebGPU", makeList({{-1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}));
    tables.set("upsWebGPU", makeList({{0, 1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {0, 1, 0}, {0, 1, 0}}));
    tables.set("directionsWebGL", makeList({{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}));
    tables.set("upsWebGL", makeList({{0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}}));
    cubeTablesCache() = JSWeakValue(tables);
    return tables;
}

// renderShadow( frame ) { render the six cube faces of the point light shadow }
JSValue renderShadow(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto frame = arg(args, 0);
    const auto shadow = self.get("shadow");
    auto shadowMap = self.get("shadowMap");
    auto light = self.get("light");
    auto renderer = frame.get("renderer");
    const auto scene = frame.get("scene");
    auto camera = shadow.get("camera");
    auto shadowMatrix = shadow.get("matrix");

    const bool isWebGPU =
        renderer.get("coordinateSystem").toNumber() == static_cast<double>(WebGPUCoordinateSystem);
    const auto tables = cubeFaceTables();
    const auto cubeDirections = tables.get(isWebGPU ? "directionsWebGPU" : "directionsWebGL");
    const auto cubeUps = tables.get(isWebGPU ? "upsWebGPU" : "upsWebGL");

    const auto mapSize = shadow.get("mapSize");
    shadowMap.get("setSize").call(shadowMap, {mapSize.get("width"), mapSize.get("width")});

    auto clearColor = nativeColorConstructor().construct();
    const auto previousAutoClear = renderer.get("autoClear");
    const auto previousClearColor = renderer.get("getClearColor").call(renderer, {clearColor});
    const auto previousClearAlpha = renderer.get("getClearAlpha").call(renderer, {});
    renderer.set("autoClear", false);
    renderer.get("setClearColor").call(renderer,
                                      {shadow.get("clearColor"), shadow.get("clearAlpha")});

    const auto lightPositionWorld = nativeVector3Constructor().construct();
    const auto lookTarget = nativeVector3Constructor().construct();
    const auto projectionScreenMatrix = nativeMatrix4Constructor().construct();
    const auto cameraPosition = camera.get("position");
    const auto cameraUp = camera.get("up");

    for (int face = 0; face < 6; ++face) {
        renderer.get("setRenderTarget").call(renderer, {shadowMap, JSValue(static_cast<double>(face))});
        renderer.get("clear").call(renderer, {});

        const auto lightDistance = light.get("distance");
        const auto cameraFar = camera.get("far");
        const auto far = lightDistance.toBoolean() ? lightDistance : cameraFar;
        if (!far.strictEquals(cameraFar)) {
            camera.set("far", far);
            camera.get("updateProjectionMatrix").call(camera, {});
        }

        const auto lightMatrixWorld = light.get("matrixWorld");
        lightPositionWorld.get("setFromMatrixPosition").call(lightPositionWorld, {lightMatrixWorld});
        cameraPosition.get("copy").call(cameraPosition, {lightPositionWorld});
        lookTarget.get("copy").call(lookTarget, {cameraPosition});
        lookTarget.get("add").call(lookTarget, {cubeDirections.get(face)});
        cameraUp.get("copy").call(cameraUp, {cubeUps.get(face)});
        camera.get("lookAt").call(camera, {lookTarget});
        camera.get("updateMatrixWorld").call(camera, {});

        shadowMatrix.get("makeTranslation").call(
            shadowMatrix, {JSValue(-lightPositionWorld.get("x").toNumber()),
                           JSValue(-lightPositionWorld.get("y").toNumber()),
                           JSValue(-lightPositionWorld.get("z").toNumber())});
        projectionScreenMatrix.get("multiplyMatrices").call(
            projectionScreenMatrix,
            {camera.get("projectionMatrix"), camera.get("matrixWorldInverse")});
        shadow.get("_frustum")
            .get("setFromProjectionMatrix")
            .call(shadow.get("_frustum"),
                  {projectionScreenMatrix, camera.get("coordinateSystem"),
                   camera.get("reversedDepth")});

        const auto currentSceneName = scene.get("name");
        std::string lightLabel;
        const auto lightName = light.get("name");
        if (lightName.toBoolean()) {
            lightLabel = lightName.toString();
        } else {
            lightLabel = "ID: " + std::to_string(static_cast<long long>(light.get("id").toNumber()));
        }
        JSValue& sceneRef = const_cast<JSValue&>(scene);
        sceneRef.set("name",
                     JSValue("Point Light Shadow [ " + lightLabel + " ] - Face " +
                             std::to_string(face + 1)));
        renderer.get("render").call(renderer, {sceneRef, camera});
        sceneRef.set("name", currentSceneName);
    }

    renderer.set("autoClear", previousAutoClear);
    renderer.get("setClearColor").call(renderer, {previousClearColor, previousClearAlpha});
    return {};
}
}  // namespace

JSValue nativePointShadowNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "PointShadowNode module");
    nativeShadowNodeModule();

    const auto base = nativeShadowNodeConstructor();
    JSValue bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "PointShadowNode", 2, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            const auto light = arg(args, 0, nullptr);
            auto shadow = arg(args, 1, JSValue{});
            auto self = superConstruct(target, captures, {light, shadow});
            self.set("isPointShadowNode", true);
            return self;
        },
        {}, "", "");

    const auto prototype = ctor.get("prototype");
    method(prototype, "getShadowFilterFn", 1, getShadowFilterFn, {}, "");
    method(prototype, "setupShadowCoord", 2, setupShadowCoord, {}, "");
    method(prototype, "setupRenderTarget", 2, setupRenderTarget, {}, "");
    method(prototype, "setupShadowFilter", 2, setupShadowFilter, {}, "");
    method(prototype, "renderShadow", 1, renderShadow, {}, "");
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativePointShadowNodeBinding(const std::string& name) {
    const auto keeper = nativePointShadowNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativePointShadowNodeConstructor() { return nativePointShadowNodeBinding("default"); }