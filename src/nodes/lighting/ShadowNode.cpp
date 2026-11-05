#include "nodes/lighting/ShadowNode.h"
#include "constants.h"

#include "core/JSWeakMap.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/lighting/ShadowBaseNode.h"
#include "materials/nodes/NodeMaterial.h"
#include "nodes/accessors/Lights.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/accessors/ReferenceNode.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/ContextNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/display/ViewportDepthNode.h"
#include "nodes/lighting/ShadowFilterNode.h"
#include "nodes/tsl/TSLBase.h"
#include "renderers/common/QuadMesh.h"
#include "renderers/common/RendererUtils.h"
#include "nodes/accessors/CubeTextureNode.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/display/ScreenNode.h"
#include "nodes/math/MathNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/EquirectUV.h"
#include "nodes/utils/LoopNode.h"
#include "textures/DepthTexture.h"

#include <stdexcept>
#include <string>

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// Forward declarations for the VSM pass graphs (defined at the bottom of this translation unit).
JSValue makeVsmFn(bool vertical);
JSWeakValue& quadMeshCache() { static thread_local JSWeakValue weak; return weak; }

// const _quadMesh = new QuadMesh(); (module level singleton, shared by both VSM passes)
JSValue sharedQuadMesh() {
    if (const auto cached = quadMeshCache().lock(); cached && cached->toBoolean()) return *cached;
    const auto mesh = nativeQuadMeshConstructor().construct({JSValue(nullptr)});
    quadMeshCache() = JSWeakValue(mesh);
    return mesh;
}

// const _shadowFilterLib = [ BasicShadowFilter, PCFShadowFilter, null, VSMShadowFilter ]
JSValue shadowFilterLibrary() {
    auto library = JSValue::array(4);
    library.set(JSValue(0.0), nativeShadowFilterNodeBinding("BasicShadowFilter"));
    library.set(JSValue(1.0), nativeShadowFilterNodeBinding("PCFShadowFilter"));
    library.set(JSValue(2.0), JSValue(nullptr));
    library.set(JSValue(3.0), nativeShadowFilterNodeBinding("VSMShadowFilter"));
    return library;
}

// getShadowFilterFn( type ) { return _shadowFilterLib[ type ]; }
JSValue getShadowFilterFn(const JSValue&, const Args& args, const Args&) {
    return shadowFilterLibrary().get(arg(args, 0).toNumber());
}

[[noreturn]] JSValue notPorted(const char* name) {
    throw std::runtime_error(std::string("ShadowNode: ") + name + " is not ported yet");
}

// dispose() { this._reset(); super.dispose(); }
JSValue disposeMethod(const JSValue& rawSelf, const Args&, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto reset = self.get("_reset");
    if (reset.toBoolean()) reset.call(self, {});
    const auto basePrototype = nativeShadowBaseNodeConstructor().get("prototype");
    const auto baseDispose = basePrototype.get("dispose");
    if (baseDispose.toBoolean()) baseDispose.call(self, {});
    return {};
}

void disposeCache(JSValue& self, const char* name) {
    JSValue slot = self.get(name);
    if (!slot.toBoolean()) return;
    const auto dispose = slot.get("dispose");
    if (dispose.toBoolean()) dispose.call(slot, {});
    self.set(name, JSValue(nullptr));
}

// _reset() { ...dispose the cached shadow maps and VSM materials... }
JSValue resetMethod(const JSValue& rawSelf, const Args&, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    self.set("_currentShadowType", JSValue(nullptr));
    if (const auto disposeShadowMaterial = self.get("disposeShadowMaterial");
        disposeShadowMaterial.toBoolean()) {
        disposeShadowMaterial.call(self, {});
    }
    disposeCache(self, "shadowMap");
    disposeCache(self, "vsmShadowMapVertical");
    disposeCache(self, "vsmShadowMapHorizontal");
    disposeCache(self, "vsmMaterialVertical");
    disposeCache(self, "vsmMaterialHorizontal");
    return {};
}

// setupShadowFilter( builder, { filterFn, depthTexture, shadowCoord, shadow, depthLayer } )
// const frustumTest = ...; const shadowNode = filterFn( {...} ); return frustumTest.select( shadowNode, float( 1 ) )
JSValue setupShadowFilter(const JSValue&, const Args& args, const Args&) {
    const auto configuration = arg(args, 1);
    const auto filterFn = configuration.get("filterFn");
    const auto shadowCoord = configuration.get("shadowCoord");

    const auto x = shadowCoord.get("x");
    const auto y = shadowCoord.get("y");
    const auto z = shadowCoord.get("z");
    auto test = x.get("greaterThanEqual").call(x, {JSValue(0.0)});
    test = test.get("and").call(test, {x.get("lessThanEqual").call(x, {JSValue(1.0)})});
    test = test.get("and")
               .call(test, {y.get("greaterThanEqual").call(y, {JSValue(0.0)})});
    test = test.get("and").call(test, {y.get("lessThanEqual").call(y, {JSValue(1.0)})});
    test = test.get("and")
               .call(test, {z.get("lessThanEqual").call(z, {JSValue(1.0)})});

    auto filterConfiguration = JSValue::object(nullptr);
    filterConfiguration.set("depthTexture", configuration.get("depthTexture"));
    filterConfiguration.set("shadowCoord", shadowCoord);
    filterConfiguration.set("shadow", configuration.get("shadow"));
    filterConfiguration.set("depthLayer", configuration.get("depthLayer"));
    const auto shadowNode = filterFn.call({}, {filterConfiguration});
    return test.get("select").call(test, {shadowNode, nativeTSLBaseBinding("float").call({}, {JSValue(1.0)})});
}
// const bias = shadow.biasNode || reference( 'bias', 'float', shadow ).setGroup( renderGroup )
JSValue setupShadowCoord(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0);
    const auto shadowPosition = arg(args, 1);
    const auto shadow = self.get("shadow");
    const auto renderer = builder.get("renderer");
    const auto group = nativeUniformGroupNodeBinding("renderGroup");
    const auto referenceFn = nativeReferenceNodeBinding("reference");

    auto bias = shadow.get("biasNode");
    if (!bias.toBoolean()) {
        auto referenceNode =
            referenceFn.call({}, {JSValue("bias"), JSValue("float"), shadow});
        const auto setGroup = referenceNode.get("setGroup");
        bias = setGroup.call(referenceNode, {group});
    }

    auto shadowCoord = shadowPosition;
    JSValue coordZ;
    const auto camera = shadow.get("camera");
    const bool isOrthographic = camera.get("isOrthographicCamera").toBoolean();
    const bool logarithmicDepthBuffer = renderer.get("logarithmicDepthBuffer").toBoolean();
    if (isOrthographic || !logarithmicDepthBuffer) {
        const auto xyz = shadowCoord.get("xyz");
        const auto div = xyz.get("div");
        shadowCoord = div.call(xyz, {shadowCoord.get("w")});
        coordZ = shadowCoord.get("z");
    } else {
        const auto w = shadowCoord.get("w");
        const auto xy = shadowCoord.get("xy");
        const auto div = xy.get("div");
        shadowCoord = div.call(xy, {w});
        auto nearReference =
            referenceFn.call({}, {JSValue("near"), JSValue("float"), camera});
        nearReference = nearReference.get("setGroup").call(nearReference, {group});
        auto farReference = referenceFn.call({}, {JSValue("far"), JSValue("float"), camera});
        farReference = farReference.get("setGroup").call(farReference, {group});
        const auto negate = w.get("negate");
        const auto negatedW = negate.call(w, {});
        coordZ = nativeViewportDepthNodeBinding("viewZToLogarithmicDepth")
                     .call({}, {negatedW, nearReference, farReference});
    }

    const auto x = shadowCoord.get("x");
    const auto y = shadowCoord.get("y");
    const auto oneMinus = y.get("oneMinus");
    const auto yFlipped = oneMinus.call(y, {});
    JSValue z;
    if (renderer.get("reversedDepthBuffer").toBoolean()) {
        const auto sub = coordZ.get("sub");
        z = sub.call(coordZ, {bias});
    } else {
        const auto add = coordZ.get("add");
        z = add.call(coordZ, {bias});
    }
    return nativeTSLBaseBinding("vec3").call({}, {x, yFlipped, z});
}
// const depthTexture = new DepthTexture( shadow.mapSize.width, shadow.mapSize.height ) ...
JSValue setupRenderTarget(const JSValue&, const Args& args, const Args&) {
    const auto shadow = arg(args, 0);
    const auto builder = arg(args, 1);
    const auto mapSize = shadow.get("mapSize");
    const auto width = mapSize.get("width");
    const auto height = mapSize.get("height");
    const auto renderer = builder.get("renderer");

    auto depthTexture = nativeDepthTextureConstructor().construct({width, height});
    depthTexture.set("name", JSValue("ShadowDepthTexture"));
    const auto compareFunction = renderer.get("reversedDepthBuffer").toBoolean()
                                     ? nativeNodeConstantsBinding("GreaterEqualCompare")
                                     : nativeNodeConstantsBinding("LessEqualCompare");
    depthTexture.set("compareFunction", compareFunction);

    const auto createRenderTarget = builder.get("createRenderTarget");
    auto shadowMap = createRenderTarget.call(builder, {width, height});
    auto texture = shadowMap.get("texture");
    texture.set("name", JSValue("ShadowMap"));
    texture.set("type", shadow.get("mapType"));
    shadowMap.set("depthTexture", depthTexture);

    auto result = JSValue::object(nullptr);
    result.set("shadowMap", shadowMap);
    result.set("depthTexture", depthTexture);
    return result;
}
// setupShadow( builder ) { ... }
// Segment 1 only: build the render target and depth texture, configure the filters from the renderer
// compatibility and shadow map type, then refresh the shadow camera projection. Segments 2 to 4 (the VSM
// passes, the shadow coordinate branch and the filter/output graph) still raise until they land.
JSValue setupShadow(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto builder = arg(args, 0);
    const auto renderer = builder.get("renderer");
    const auto camera = builder.get("camera");
    const auto shadow = self.get("shadow");

    const auto renderTargets = self.get("setupRenderTarget").call(self, {shadow, builder});
    auto depthTexture = renderTargets.get("depthTexture");
    auto shadowMap = renderTargets.get("shadowMap");
    const auto shadowMapType = renderer.get("shadowMap").get("type");
    const auto compatibility = nativeConstantsModule().get("Compatibility");
    const auto hasTextureCompare = renderer.get("hasCompatibility")
                                       .call(renderer, {compatibility.get("TEXTURE_COMPARE")});
    const auto linearFilter = nativeNodeConstantsBinding("LinearFilter");
    const auto nearestFilter = nativeNodeConstantsBinding("NearestFilter");
    if (shadowMapType.strictEquals(nativeNodeConstantsBinding("PCFShadowMap")) &&
        hasTextureCompare.toBoolean()) {
        depthTexture.set("minFilter", linearFilter);
        depthTexture.set("magFilter", linearFilter);
    } else {
        depthTexture.set("minFilter", nearestFilter);
        depthTexture.set("magFilter", nearestFilter);
    }

    auto shadowCamera = shadow.get("camera");
    shadowCamera.set("coordinateSystem", camera.get("coordinateSystem"));
    shadowCamera.set("_reversedDepth", renderer.get("reversedDepthBuffer"));
    shadowCamera.get("updateProjectionMatrix").call(shadowCamera, {});

    // Segment 2: the VSM blur pass materials (L384-L448).
    if (renderer.get("shadowMap").get("type").strictEquals(nativeNodeConstantsBinding("VSMShadowMap")) &&
        !shadow.get("isPointLightShadow").toBoolean()) {
        depthTexture.set("compareFunction", JSValue(nullptr));
        const auto mapWidth = shadow.get("mapSize").get("width");
        const auto mapHeight = shadow.get("mapSize").get("height");
        const auto shadowMapDepth = shadowMap.get("depth");
        const auto createRenderTarget = builder.get("createRenderTarget");
        auto formatOptions = JSValue::object(nullptr);
        formatOptions.set("format", nativeNodeConstantsBinding("RGFormat"));
        auto depthOptions = JSValue::object(nullptr);
        depthOptions.set("format", nativeNodeConstantsBinding("RGFormat"));
        depthOptions.set("depth", shadowMapDepth);
        JSValue vsmVertical;
        JSValue vsmHorizontal;
        if (shadowMapDepth.toNumber() > 1) {
            // Reuse the per shadow map array target so a layered shadow map shares one target.
            vsmVertical = shadowMap.get("_vsmShadowMapVertical");
            if (!vsmVertical.toBoolean()) {
                vsmVertical = createRenderTarget.call(builder, {mapWidth, mapHeight, depthOptions});
                vsmVertical.get("texture").set("name", JSValue("VSMVertical"));
                shadowMap.set("_vsmShadowMapVertical", vsmVertical);
            }
            vsmHorizontal = shadowMap.get("_vsmShadowMapHorizontal");
            if (!vsmHorizontal.toBoolean()) {
                vsmHorizontal = createRenderTarget.call(builder, {mapWidth, mapHeight, depthOptions});
                vsmHorizontal.get("texture").set("name", JSValue("VSMHorizontal"));
                shadowMap.set("_vsmShadowMapHorizontal", vsmHorizontal);
            }
        } else {
            vsmVertical = createRenderTarget.call(builder, {mapWidth, mapHeight, formatOptions});
            vsmHorizontal = createRenderTarget.call(builder, {mapWidth, mapHeight, formatOptions});
        }
        self.set("vsmShadowMapVertical", vsmVertical);
        self.set("vsmShadowMapHorizontal", vsmHorizontal);

        auto shadowPassVertical = nativeTextureNodeBinding("texture").call({}, {depthTexture});
        auto shadowPassHorizontal =
            nativeTextureNodeBinding("texture").call({}, {vsmVertical.get("texture")});
        if (depthTexture.get("isArrayTexture").toBoolean()) {
            shadowPassVertical =
                invoke(shadowPassVertical, "depth")({self.get("depthLayer")});
            shadowPassHorizontal =
                invoke(shadowPassHorizontal, "depth")({self.get("depthLayer")});
        }

        const auto group = nativeUniformGroupNodeBinding("renderGroup");
        const auto referenceFn = nativeReferenceNodeBinding("reference");
        const auto makeReference = [&](const char* propertyName, const char* propertyType) {
            auto referenceNode =
                referenceFn.call({}, {JSValue(propertyName), JSValue(propertyType), shadow});
            return invoke(referenceNode, "setGroup")({group});
        };
        const auto samples = makeReference("blurSamples", "float");
        const auto radius = makeReference("radius", "float");
        const auto size = makeReference("mapSize", "vec2");
        const auto sharedContext =
            nativeContextNodeBinding("context").call({}, {builder.get("getSharedContext").call(builder, {})});

        auto materialVertical = self.get("vsmMaterialVertical");
        if (!materialVertical.toBoolean()) {
            materialVertical = nativeNodeMaterialConstructor().construct();
            self.set("vsmMaterialVertical", materialVertical);
        }
        materialVertical.set("contextNode", sharedContext);
        materialVertical.set("fragmentNode", makeVsmFn(true));
        materialVertical.set("name", JSValue("VSMVertical"));

        auto materialHorizontal = self.get("vsmMaterialHorizontal");
        if (!materialHorizontal.toBoolean()) {
            materialHorizontal = nativeNodeMaterialConstructor().construct();
            self.set("vsmMaterialHorizontal", materialHorizontal);
        }
        materialHorizontal.set("contextNode", sharedContext);
        materialHorizontal.set("fragmentNode", makeVsmFn(false));
        materialHorizontal.set("name", JSValue("VSMHorizontal"));
    }
    // Segment 3: the shadow coordinate branch (L452-L476).
    const auto light = self.get("light");
    const auto group = nativeUniformGroupNodeBinding("renderGroup");
    const auto referenceFn = nativeReferenceNodeBinding("reference");
    const auto makeReference = [&](const char* propertyName, const char* propertyType) {
        auto referenceNode =
            referenceFn.call({}, {JSValue(propertyName), JSValue(propertyType), shadow});
        return referenceNode.get("setGroup").call(referenceNode, {group});
    };
    const auto shadowIntensity = makeReference("intensity", "float");
    const auto normalBias = makeReference("normalBias", "float");
    const auto shadowMatrix = nativeLightsBinding("lightShadowMatrix").call({}, {light});
    const auto normalWorld = nativeNormalBinding("normalWorld");
    const auto shadowNormalBias = normalWorld.get("mul").call(normalWorld, {normalBias});

    const auto material = builder.get("material");
    const auto builderContext = builder.get("context");
    const bool receivedShadowPosition =
        material.isObject() && material.get("receivedShadowPositionNode").toBoolean();
    const bool contextShadowPosition =
        builderContext.isObject() && builderContext.get("shadowPositionWorld").toBoolean();
    auto shadowPosition = JSValue{};
    if (!renderer.get("highPrecision").toBoolean() || receivedShadowPosition ||
        contextShadowPosition) {
        const auto shadowPositionWorld = nativeShadowBaseNodeBinding("shadowPositionWorld");
        const auto biased =
            shadowPositionWorld.get("add").call(shadowPositionWorld, {shadowNormalBias});
        shadowPosition = shadowMatrix.get("mul").call(shadowMatrix, {biased});
    } else {
        const auto uniformFactory = nativeUniformFactory();
        auto modelMatrix = uniformFactory.call({}, {JSValue("mat4")});
        const auto onObjectUpdate = modelMatrix.get("onObjectUpdate");
        auto updateCallback = JSValue::function(
            +[](const JSValue&, const std::vector<JSValue>& callbackArgs,
                const std::vector<JSValue>& captures) -> JSValue {
                const auto configuration = callbackArgs.at(0);
                auto selfUniform = const_cast<JSValue&>(callbackArgs.at(1));
                const auto object = configuration.get("object");
                const auto matrix = captures.at(0);
                auto value = selfUniform.get("value");
                value.get("multiplyMatrices")
                    .call(value, {matrix.get("value"), object.get("matrixWorld")});
                return value;
            },
            {shadowMatrix});
        modelMatrix = onObjectUpdate.call(modelMatrix, {updateCallback});
        const auto positionLocal = nativePositionBinding("positionLocal");
        const auto localPosition = modelMatrix.get("mul").call(modelMatrix, {positionLocal});
        const auto biasVector = nativeTSLBaseBinding("vec4").call({}, {shadowNormalBias, JSValue(0.0)});
        const auto biasContribution = shadowMatrix.get("mul").call(shadowMatrix, {biasVector});
        shadowPosition = localPosition.get("add").call(localPosition, {biasContribution});
    }
    self.set("_shadowIntensity", shadowIntensity);
    const auto shadowCoord = self.get("setupShadowCoord").call(self, {builder, shadowPosition});

    // Segment 4: resolve the filter, sample the shadow colour when transmitted and mix the output.
    // The pinned toInspector() debug hooks are omitted here (inspector only, no rendering effect).
    const bool isVsmShadow =
        renderer.get("shadowMap").get("type").strictEquals(nativeNodeConstantsBinding("VSMShadowMap")) &&
        !shadow.get("isPointLightShadow").toBoolean();
    auto shadowDepthTexture = depthTexture;
    if (isVsmShadow) {
        const auto vsmMap = self.get("vsmShadowMapVertical");
        if (vsmMap.toBoolean()) shadowDepthTexture = vsmMap.get("texture");
    }

    auto filterFn = shadow.get("filterNode");
    if (!filterFn.toBoolean()) {
        filterFn = self.get("getShadowFilterFn").call(self, {renderer.get("shadowMap").get("type")});
    }
    if (!filterFn.toBoolean()) {
        throw std::runtime_error("THREE.WebGPURenderer: Shadow map type not supported yet.");
    }

    auto filterConfiguration = JSValue::object(nullptr);
    filterConfiguration.set("filterFn", filterFn);
    filterConfiguration.set("shadowTexture", shadowMap.get("texture"));
    filterConfiguration.set("depthTexture", shadowDepthTexture);
    filterConfiguration.set("shadow", shadow);
    filterConfiguration.set("shadowCoord", shadowCoord);
    filterConfiguration.set("depthLayer", self.get("depthLayer"));
    const auto shadowNode = self.get("setupShadowFilter").call(self, {builder, filterConfiguration});

    auto shadowColor = JSValue{};
    if (renderer.get("shadowMap").get("transmitted").toBoolean()) {
        auto mapTexture = shadowMap.get("texture");
        if (mapTexture.get("isCubeTexture").toBoolean()) {
            shadowColor = nativeCubeTextureNodeBinding("cubeTexture")
                              .call({}, {mapTexture, shadowCoord.get("xyz")});
        } else {
            shadowColor =
                nativeTextureNodeBinding("texture").call({}, {mapTexture, shadowCoord});
            if (depthTexture.get("isArrayTexture").toBoolean()) {
                shadowColor = shadowColor.get("depth").call(shadowColor, {self.get("depthLayer")});
            }
        }
    }

    const auto mixFn = nativeMathNodeBinding("mix");
    const auto intensity = self.get("_shadowIntensity");
    JSValue shadowOutput;
    if (shadowColor.toBoolean()) {
        const auto rgb = shadowNode.get("rgb");
        const auto colored = rgb.get("mix").call(rgb, {shadowColor, JSValue(1.0)});
        const auto weight = intensity.get("mul").call(intensity, {shadowColor.get("a")});
        auto mixed = mixFn.call({}, {JSValue(1.0), colored, weight});
        shadowOutput = mixed.get("toVar").call(mixed, {});
    } else {
        auto mixed = mixFn.call({}, {JSValue(1.0), shadowNode, intensity});
        shadowOutput = mixed.get("toVar").call(mixed, {});
    }

    self.set("shadowMap", shadowMap);
    const_cast<JSValue&>(shadow).set("map", shadowMap);
    return shadowOutput;
}
// setup( builder ) { check the renderer shadow state, reuse the cached node and run the material hook. }
// The pinned wrapper runs the body inside Fn(...)(); the body is executed directly here, which has the
// same observable result because the call happens immediately. setupShadow is still a raising member.
JSValue setup(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto builder = arg(args, 0);
    const auto renderer = builder.get("renderer");
    const auto shadowMap = renderer.get("shadowMap");
    // if ( builder.renderer.shadowMap.enabled === false ) return;
    if (shadowMap.get("enabled").strictEquals(JSValue(false))) return {};

    const auto currentShadowType = shadowMap.get("type");
    if (!self.get("_currentShadowType").strictEquals(currentShadowType)) {
        self.get("_reset").call(self, {});
        self.set("_node", JSValue(nullptr));
    }

    auto node = self.get("_node");
    self.get("setupShadowPosition").call(self, {builder});
    if (!node.toBoolean()) {
        node = self.get("setupShadow").call(self, {builder});
        self.set("_node", node);
        self.set("_currentShadowType", currentShadowType);
    }

    const auto material = builder.get("material");
    if (material.isObject()) {
        const auto receivedShadowNode = material.get("receivedShadowNode");
        if (receivedShadowNode.toBoolean()) node = receivedShadowNode.call(material, {node});
    }
    return node;
}
// renderShadow( frame ) { shadow.updateMatrices( light ); shadowMap.setSize(...); renderer.render( scene, shadow.camera ); }
JSValue renderShadow(const JSValue& self, const Args& args, const Args&) {
    const auto frame = arg(args, 0);
    const auto shadow = self.get("shadow");
    const auto shadowMap = self.get("shadowMap");
    const auto light = self.get("light");
    const auto renderer = frame.get("renderer");
    const auto scene = frame.get("scene");

    const auto updateMatrices = shadow.get("updateMatrices");
    updateMatrices.call(shadow, {light});
    const auto mapSize = shadow.get("mapSize");
    const auto setSize = shadowMap.get("setSize");
    setSize.call(shadowMap, {mapSize.get("width"), mapSize.get("height"), shadowMap.get("depth")});

    const auto currentSceneName = scene.get("name");
    const auto lightName = light.get("name");
    std::string lightLabel;
    if (lightName.toBoolean()) {
        lightLabel = lightName.toString();
    } else {
        lightLabel = "ID: " + std::to_string(static_cast<long long>(light.get("id").toNumber()));
    }
    JSValue& sceneRef = const_cast<JSValue&>(scene);
    sceneRef.set("name", JSValue("Shadow Map [ " + lightLabel + " ]"));
    renderer.get("render").call(renderer, {sceneRef, shadow.get("camera")});
    sceneRef.set("name", currentSceneName);
    return {};
}

// vsmPass( renderer ) { set the two VSM targets and render the shared quad mesh with each material. }
JSValue vsmPass(const JSValue& self, const Args& args, const Args&) {
    const auto renderer = arg(args, 0);
    const auto shadow = self.get("shadow");
    const auto shadowMap = self.get("shadowMap");
    const auto depth = shadowMap.get("depth");
    const auto mapSize = shadow.get("mapSize");
    const auto vertical = self.get("vsmShadowMapVertical");
    const auto horizontal = self.get("vsmShadowMapHorizontal");
    const auto setSizeVertical = vertical.get("setSize");
    setSizeVertical.call(vertical, {mapSize.get("width"), mapSize.get("height"), depth});
    const auto setSizeHorizontal = horizontal.get("setSize");
    setSizeHorizontal.call(horizontal, {mapSize.get("width"), mapSize.get("height"), depth});

    auto quadMesh = sharedQuadMesh();
    const auto setRenderTarget = renderer.get("setRenderTarget");
    setRenderTarget.call(renderer, {vertical});
    quadMesh.set("material", self.get("vsmMaterialVertical"));
    quadMesh.get("render").call(quadMesh, {renderer});
    setRenderTarget.call(renderer, {horizontal});
    quadMesh.set("material", self.get("vsmMaterialHorizontal"));
    quadMesh.get("render").call(quadMesh, {renderer});
    return {};
}
// module level renderer state cache, like the pinned `let _rendererState;`
JSWeakValue& rendererStateCache() { static thread_local JSWeakValue weak; return weak; }

// updateShadow( frame ) { save the renderer state, render the shadow map, restore it. }
JSValue updateShadow(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto frame = arg(args, 0);
    const auto shadowMap = self.get("shadowMap");
    const auto shadow = self.get("shadow");
    const auto renderer = frame.get("renderer");
    const auto scene = frame.get("scene");
    const auto camera = frame.get("camera");

    const auto shadowType = renderer.get("shadowMap").get("type");
    auto shadowCamera = shadow.get("camera");
    auto layers = shadowCamera.get("layers");
    const auto shadowCameraLayer = layers.get("mask").toNumber();
    // if ( ( shadow.camera.layers.mask & 0xFFFFFFFE ) === 0 ) shadow.camera.layers.mask = camera.layers.mask
    if ((static_cast<long long>(shadowCameraLayer) & 0xFFFFFFFEL) == 0) {
        layers.set("mask", camera.get("layers").get("mask"));
    }

    const auto currentRenderObjectFunction = renderer.get("getRenderObjectFunction").call(renderer, {});
    const auto resetState = nativeRendererUtilsBinding("resetRendererAndSceneState");
    auto rendererState = resetState.call({}, {renderer, scene, JSValue(nullptr)});
    rendererStateCache() = JSWeakValue(rendererState);

    JSValue& sceneRef = const_cast<JSValue&>(scene);
    sceneRef.set("overrideMaterial", self.get("getShadowMaterial").call(self, {}));
    renderer.get("setRenderObjectFunction")
        .call(renderer,
              {self.get("getShadowRenderObjectFunction").call(self, {renderer, shadow})});
    renderer.get("setClearColor").call(renderer, {JSValue(0x000000), JSValue(0)});
    renderer.get("setRenderTarget").call(renderer, {shadowMap});
    self.get("renderShadow").call(self, {frame});
    renderer.get("setRenderObjectFunction").call(renderer, {currentRenderObjectFunction});

    const auto vsmType = nativeNodeConstantsBinding("VSMShadowMap");
    if (shadowType.strictEquals(vsmType) && !shadow.get("isPointLightShadow").toBoolean()) {
        self.get("vsmPass").call(self, {renderer});
    }

    layers.set("mask", JSValue(shadowCameraLayer));
    nativeRendererUtilsBinding("restoreRendererAndSceneState")
        .call({}, {renderer, scene, rendererState});
    return {};
}
// updateBefore( frame ) { update the shadow map once per camera frame when needed. }
JSValue updateBefore(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto frame = arg(args, 0);
    const auto renderer = frame.get("renderer");
    // if ( frame.renderer._precompilationState !== null ) return;
    if (!renderer.get("_precompilationState").isNull()) return {};

    auto shadow = self.get("shadow");
    bool needsUpdate =
        shadow.get("needsUpdate").toBoolean() || shadow.get("autoUpdate").toBoolean();
    if (needsUpdate) {
        auto cameraFrameId = self.get("_cameraFrameId");
        const auto camera = frame.get("camera");
        const auto frameId = frame.get("frameId");
        const auto cached = cameraFrameId.get("get").call(cameraFrameId, {camera});
        if (cached.toBoolean() && cached.toNumber() == frameId.toNumber()) needsUpdate = false;
        cameraFrameId.get("set").call(cameraFrameId, {camera, frameId});
    }
    if (needsUpdate) {
        self.get("updateShadow").call(self, {frame});
        const_cast<JSValue&>(shadow).set("needsUpdate", false);
    }
    return {};
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
JSValue intNode(const JSValue& value) { return nativeTSLBaseBinding("int").call({}, {value}); }
JSValue vec2Node(const JSValue& x, const JSValue& y) { return nativeTSLBaseBinding("vec2").call({}, {x, y}); }

// Shared body for VSMPassVertical / VSMPassHorizontal: the captured boolean selects the swizzled offset and
// the sampled channel/variance terms, everything else is identical.
JSValue vsmPassBody(bool vertical, const JSValue& samples, const JSValue& radius, const JSValue& size,
                    const JSValue& shadowPass, const JSValue& depthLayer) {
    const auto one = floatNode(1);
    const auto condition = invoke(samples, "lessThanEqual")({one});
    const auto stride = invoke(floatNode(2), "div")({invoke(samples, "sub")({one})});
    const auto uvStride = invoke(condition, "select")({floatNode(0), stride});
    const auto uvStart = invoke(condition, "select")({floatNode(0), floatNode(-1)});
    auto mean = invoke(floatNode(0), "toVar")({JSValue(vertical ? "meanVertical" : "meanHorizontal")});
    auto squaredMean = invoke(floatNode(0), "toVar")(
        {JSValue(vertical ? "squareMeanVertical" : "squareMeanHorizontal")});

    auto loopConfiguration = JSValue::object(nullptr);
    loopConfiguration.set("start", intNode(JSValue(0.0)));
    loopConfiguration.set("end", intNode(samples));
    loopConfiguration.set("type", JSValue("int"));
    loopConfiguration.set("condition", JSValue("<"));

    auto loopBody = JSValue::function(
        +[](const JSValue&, const Args& loopArgs, const Args& captures) -> JSValue {
            const auto loopInputs = inputObject(loopArgs);
            const auto i = loopInputs.get("i");
            const bool isVertical = captures.at(0).toBoolean();
            const auto uvStride = captures.at(1);
            const auto uvStart = captures.at(2);
            const auto radius = captures.at(3);
            const auto size = captures.at(4);
            const auto shadowPass = captures.at(5);
            const auto mean = captures.at(6);
            const auto squaredMean = captures.at(7);
            const auto depthLayer = captures.at(8);

            const auto indexNode = nativeTSLBaseBinding("float").call({}, {i});
            const auto uvOffset = invoke(uvStart, "add")({invoke(indexNode, "mul")({uvStride})});
            const auto offset = vec2Node(isVertical ? JSValue(0.0) : uvOffset,
                                         isVertical ? uvOffset : JSValue(0.0));
            const auto scaled = invoke(offset, "mul")({radius});
            const auto screen = nativeScreenNodeBinding("screenCoordinate").get("xy");
            const auto uv = invoke(invoke(screen, "add")({scaled}), "div")({size});
            auto sampled = invoke(shadowPass, "sample")({uv});
            if (shadowPass.get("value").get("isArrayTexture").toBoolean()) {
                sampled = invoke(sampled, "depth")({depthLayer});
            }
            const auto sampledX = sampled.get("x");
            invoke(mean, "addAssign")({sampledX});
            if (isVertical) {
                invoke(squaredMean, "addAssign")({invoke(sampledX, "mul")({sampledX})});
            } else {
                const auto sampledY = sampled.get("y");
                const auto ySquared = invoke(sampledY, "mul")({sampledY});
                const auto xSquared = invoke(sampledX, "mul")({sampledX});
                invoke(squaredMean, "addAssign")(
                    {nativeOperatorNodeBinding("add").call({}, {ySquared, xSquared})});
            }
            return JSValue{};
        },
        {JSValue(vertical), uvStride, uvStart, radius, size, shadowPass, mean, squaredMean, depthLayer});
    nativeLoopNodeBinding("Loop").call({}, {loopConfiguration, loopBody});

    invoke(mean, "divAssign")({samples});
    invoke(squaredMean, "divAssign")({samples});
    const auto variance = invoke(squaredMean, "sub")({invoke(mean, "mul")({mean})});
    const auto deviation =
        nativeMathNodeBinding("sqrt").call({}, {invoke(variance, "max")({JSValue(0.0)})});
    return vec2Node(mean, deviation);
}

JSValue makeVsmFn(bool vertical) {
    auto callback = JSValue::function(
        +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            const auto inputs = inputObject(args);
            return vsmPassBody(captures.at(0).toBoolean(), inputs.get("samples"),
                               inputs.get("radius"), inputs.get("size"),
                               inputs.get("shadowPass"), inputs.get("depthLayer"));
        },
        {JSValue(vertical)});
    return nativeTSLCoreBinding("Fn").call({}, {callback});
}
}  // namespace

JSValue nativeShadowNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ShadowNode module");
    nativeShadowBaseNodeModule();
    nativeShadowFilterNodeModule();

    const auto base = nativeShadowBaseNodeConstructor();
    JSValue bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "ShadowNode", 2, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            const auto light = arg(args, 0, nullptr);
            auto self = superConstruct(target, captures, {light});
            auto shadow = arg(args, 1, JSValue{});
            if (!shadow.toBoolean()) shadow = light.isObject() ? light.get("shadow") : JSValue{};
            self.set("shadow", shadow);
            self.set("shadowMap", JSValue(nullptr));
            self.set("vsmShadowMapVertical", JSValue(nullptr));
            self.set("vsmShadowMapHorizontal", JSValue(nullptr));
            self.set("vsmMaterialVertical", JSValue(nullptr));
            self.set("vsmMaterialHorizontal", JSValue(nullptr));
            self.set("_node", JSValue(nullptr));
            self.set("_currentShadowType", JSValue(nullptr));
            self.set("_cameraFrameId", createNativeWeakMap());
            self.set("isShadowNode", true);
            self.set("depthLayer", JSValue(0));
            return self;
        },
        {}, "", "");

    const auto prototype = ctor.get("prototype");
    method(prototype, "getShadowFilterFn", 1, getShadowFilterFn, {}, "");
    method(prototype, "dispose", 0, disposeMethod, {}, "");
    method(prototype, "_reset", 0, resetMethod, {}, "");
    method(prototype, "setupShadowFilter", 2, setupShadowFilter, {}, "");
    method(prototype, "setupShadowCoord", 2, setupShadowCoord, {}, "");
    method(prototype, "setupRenderTarget", 2, setupRenderTarget, {}, "");
    method(prototype, "setupShadow", 1, setupShadow, {}, "");
    method(prototype, "setup", 1, setup, {}, "");
    method(prototype, "renderShadow", 1, renderShadow, {}, "");
    method(prototype, "updateShadow", 1, updateShadow, {}, "");
    method(prototype, "vsmPass", 1, vsmPass, {}, "");
    method(prototype, "updateBefore", 1, updateBefore, {}, "");
    own(bindings, "shadow", JSValue::function(
        +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            const auto ctor = captures.at(0);
            auto shadowArgument = arg(args, 1, JSValue{});
            return ctor.construct({arg(args, 0, nullptr), shadowArgument});
        },
        {ctor}));
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeShadowNodeBinding(const std::string& name) {
    const auto keeper = nativeShadowNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeShadowNodeConstructor() { return nativeShadowNodeBinding("default"); }

JSValue nativeShadowNodeVsmPassGraph(bool vertical) { return makeVsmFn(vertical); }
JSValue nativeShadowNodeQuadMesh() { return sharedQuadMesh(); }