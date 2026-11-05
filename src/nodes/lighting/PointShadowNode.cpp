#include "nodes/lighting/PointShadowNode.h"

#include "constants.h"
#include "core/JSWeakMap.h"
#include "core/JSGlobal.h"
#include "core/NativeObject.h"
#include "nodes/accessors/CubeTextureNode.h"
#include "nodes/accessors/ReferenceNode.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/display/ScreenNode.h"
#include "nodes/display/ViewportDepthNode.h"
#include "nodes/math/MathNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/PostProcessingUtils.h"
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

// Forward declaration: the sampled point filter graph is defined below the filter lookup.
JSValue pointShadowFilterGraph();

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
    return pointShadowFilterGraph();
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

// The Fn convention: the inputs arrive as one iterable whose single entry is the destructured object.
JSValue inputObject(const Args& args) {
    auto iterator = getJSIterator(arg(args, 0));
    JSValue inputs;
    if (!iterator.done) {
        const auto value = stepJSIterator(iterator);
        if (value) inputs = *value;
    }
    if (!iterator.done) closeJSIterator(iterator);
    return inputs;
}

JSValue floatNode(double value) { return nativeTSLBaseBinding("float").call({}, {JSValue(value)}); }

JSValue makeFn(JSValue::NativeFunction body) {
    auto callback = JSValue::function(body, {}, "", 0);
    return nativeTSLCoreBinding("Fn").call({}, {callback});
}

// PointShadowFilter = Fn( ( { depthTexture, bd3D, dp, shadow } ) => { ... 5 Vogel disk samples ... } )
JSValue pointShadowFilterBody(const JSValue&, const Args& args, const Args&) {
    const auto inputs = inputObject(args);
    const auto depthTexture = inputs.get("depthTexture");
    const auto bd3D = inputs.get("bd3D");
    const auto dp = inputs.get("dp");
    const auto shadow = inputs.get("shadow");

    const auto group = nativeUniformGroupNodeBinding("renderGroup");
    const auto makeReference = [&](const char* propertyName, const char* propertyType) {
        auto referenceNode =
            nativeReferenceNodeBinding("reference").call({}, {JSValue(propertyName), JSValue(propertyType), shadow});
        return invoke(referenceNode, "setGroup")({group});
    };
    const auto radius = makeReference("radius", "float");
    const auto mapSize = makeReference("mapSize", "vec2");
    const auto texelSize = invoke(radius, "div")({mapSize.get("x")});

    const auto absDir = invoke(bd3D, "abs")();
    const auto axis = invoke(invoke(absDir.get("x"), "greaterThan")({absDir.get("z")}), "select")(
        {nativeTSLBaseBinding("vec3").call({}, {JSValue(0.0), JSValue(1.0), JSValue(0.0)}),
         nativeTSLBaseBinding("vec3").call({}, {JSValue(1.0), JSValue(0.0), JSValue(0.0)})});
    const auto tangent =
        nativeMathNodeBinding("normalize")
            .call({}, {nativeMathNodeBinding("cross").call({}, {bd3D, axis})});
    const auto bitangent = nativeMathNodeBinding("cross").call({}, {bd3D, tangent});
    const auto phi = invoke(
        nativePostProcessingUtilsBinding("interleavedGradientNoise")
            .call({}, {nativeScreenNodeBinding("screenCoordinate").get("xy")}),
        "mul")({JSValue(6.28318530718)});

    const auto add = nativeOperatorNodeBinding("add");
    JSValue sum;
    for (int sample = 0; sample < 5; ++sample) {
        const auto sampleOffset =
            nativePostProcessingUtilsBinding("vogelDiskSample")
                .call({}, {JSValue(static_cast<double>(sample)), JSValue(5.0), phi});
        const auto offset = invoke(
            invoke(invoke(sampleOffset.get("x"), "mul")({tangent}), "add")({invoke(sampleOffset.get("y"), "mul")({bitangent})}),
            "mul")({texelSize});
        const auto sampleDirection = invoke(bd3D, "add")({offset});
        const auto sampled =
            nativeCubeTextureNodeBinding("cubeTexture").call({}, {depthTexture, sampleDirection});
        const auto term = invoke(sampled, "compare")({dp});
        sum = sample == 0 ? term : add.call({}, {sum, term});
    }
    return invoke(sum, "mul")({floatNode(1.0 / 5.0)});
}

// pointShadowFilter = Fn( ( { filterFn, depthTexture, shadowCoord, shadow }, builder ) => { ... } )
JSValue pointShadowFilterGraphBody(const JSValue&, const Args& args, const Args&) {
    const auto inputs = inputObject(args);
    const auto builder = args.size() > 1 ? args.at(1) : JSValue{};
    const auto renderer = builder.isObject() ? builder.get("renderer") : JSValue{};
    const auto filterFn = inputs.get("filterFn");
    const auto depthTexture = inputs.get("depthTexture");
    const auto shadowCoord = inputs.get("shadowCoord");
    const auto shadow = inputs.get("shadow");

    const auto shadowPosition = invoke(shadowCoord.get("xyz"), "toConst")();
    const auto shadowPositionAbs = invoke(shadowPosition, "abs")();
    const auto viewZ = invoke(
        invoke(shadowPositionAbs.get("x"), "max")({shadowPositionAbs.get("y")}),
        "max")({shadowPositionAbs.get("z")});

    const auto group = nativeUniformGroupNodeBinding("renderGroup");
    auto nearUniform = invoke(nativeUniformFactory().call({}, {JSValue("float")}), "setGroup")({group});
    auto farUniform = invoke(nativeUniformFactory().call({}, {JSValue("float")}), "setGroup")({group});
    const auto nearCallback = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            return captures.at(0).get("camera").get("near");
        },
        {shadow});
    const auto farCallback = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            return captures.at(0).get("camera").get("far");
        },
        {shadow});
    nearUniform = invoke(nearUniform, "onRenderUpdate")({nearCallback});
    farUniform = invoke(farUniform, "onRenderUpdate")({farCallback});
    auto bias = nativeReferenceNodeBinding("reference").call({}, {JSValue("bias"), JSValue("float"), shadow});
    bias = invoke(bias, "setGroup")({group});

    auto result = invoke(floatNode(1.0), "toVar")();
    const auto negatedViewZ = invoke(viewZ, "negate")();
    JSValue dp;
    if (renderer.get("reversedDepthBuffer").toBoolean()) {
        dp = nativeViewportDepthNodeBinding("viewZToReversedPerspectiveDepth")
                 .call({}, {negatedViewZ, nearUniform, farUniform});
        invoke(dp, "subAssign")({bias});
    } else if (renderer.get("logarithmicDepthBuffer").toBoolean()) {
        dp = nativeViewportDepthNodeBinding("viewZToLogarithmicDepth")
                 .call({}, {negatedViewZ, nearUniform, farUniform});
        invoke(dp, "addAssign")({bias});
    } else {
        dp = nativeViewportDepthNodeBinding("viewZToPerspectiveDepth")
                 .call({}, {negatedViewZ, nearUniform, farUniform});
        invoke(dp, "addAssign")({bias});
    }

    const auto bd3D = nativeMathNodeBinding("normalize").call({}, {shadowPosition});

    // If( viewZ.sub( far ).lessThanEqual( 0.0 ).and( viewZ.sub( near ).greaterThanEqual( 0.0 ) ) ) {
    //     result.assign( filterFn( { depthTexture, bd3D, dp, shadow } ) );
    // }
    const auto upperBound = invoke(invoke(viewZ, "sub")({farUniform}), "lessThanEqual")({JSValue(0.0)});
    const auto lowerBound =
        invoke(invoke(viewZ, "sub")({nearUniform}), "greaterThanEqual")({JSValue(0.0)});
    const auto inRange = invoke(upperBound, "and")({lowerBound});
    auto ifBody = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            const auto target = captures.at(0);
            auto filterConfiguration = JSValue::object(nullptr);
            filterConfiguration.set("depthTexture", captures.at(1));
            filterConfiguration.set("bd3D", captures.at(2));
            filterConfiguration.set("dp", captures.at(3));
            filterConfiguration.set("shadow", captures.at(4));
            const auto filtered = captures.at(5).call({}, {filterConfiguration});
            invoke(target, "assign")({filtered});
            return JSValue{};
        },
        {result, depthTexture, bd3D, dp, shadow, filterFn});
    nativeTSLBaseBinding("If").call({}, {inRange}).call({}, {ifBody});
    return result;
}

JSValue pointShadowFilterFn() { return makeFn(pointShadowFilterGraphBody); }
JSValue pointShadowFilterGraph() { return makeFn(pointShadowFilterBody); }

// setupShadowFilter( builder, { filterFn, depthTexture, shadowCoord, shadow } ) {
//     return pointShadowFilter( { filterFn, depthTexture, shadowCoord, shadow } );
// }
JSValue setupShadowFilter(const JSValue&, const Args& args, const Args&) {
    const auto builder = arg(args, 0);
    const auto configuration = arg(args, 1);
    auto inputs = JSValue::object(nullptr);
    inputs.set("filterFn", configuration.get("filterFn"));
    inputs.set("depthTexture", configuration.get("depthTexture"));
    inputs.set("shadowCoord", configuration.get("shadowCoord"));
    inputs.set("shadow", configuration.get("shadow"));
    return pointShadowFilterFn().call({}, {inputs, builder});
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
    auto shadow = self.get("shadow");
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

    auto lightPositionWorld = nativeVector3Constructor().construct();
    auto lookTarget = nativeVector3Constructor().construct();
    auto projectionScreenMatrix = nativeMatrix4Constructor().construct();
    auto cameraPosition = camera.get("position");
    auto cameraUp = camera.get("up");

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

        // Typed vector and matrix methods answer the mutated value; it has to be written back into the
        // owning managed object because the receiver arrives as a value copy.
        const auto lightMatrixWorld = light.get("matrixWorld");
        lightPositionWorld = lightPositionWorld.get("setFromMatrixPosition").call(lightPositionWorld, {lightMatrixWorld});
        cameraPosition = cameraPosition.get("copy").call(cameraPosition, {lightPositionWorld});
        camera.set("position", cameraPosition);
        lookTarget = lookTarget.get("copy").call(lookTarget, {cameraPosition});
        lookTarget = lookTarget.get("add").call(lookTarget, {cubeDirections.get(face)});
        cameraUp = cameraUp.get("copy").call(cameraUp, {cubeUps.get(face)});
        camera.set("up", cameraUp);
        camera.get("lookAt").call(camera, {lookTarget});
        camera.get("updateMatrixWorld").call(camera, {});

        shadowMatrix = shadowMatrix.get("makeTranslation").call(
            shadowMatrix, {JSValue(-lightPositionWorld.get("x").toNumber()),
                           JSValue(-lightPositionWorld.get("y").toNumber()),
                           JSValue(-lightPositionWorld.get("z").toNumber())});
        shadow.set("matrix", shadowMatrix);
        projectionScreenMatrix = projectionScreenMatrix.get("multiplyMatrices").call(
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