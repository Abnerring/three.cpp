#include "materials/nodes/PointsNodeMaterial.h"

#include "math/Vector2.h"
#include "materials/PointsMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "materials/nodes/NodeMaterialSources.inc"
#include "materials/nodes/SpriteNodeMaterial.h"
#include "nodes/accessors/MaterialNode.h"
#include "nodes/accessors/ModelNode.h"
#include "nodes/accessors/Position.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/display/ScreenNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/RotateNode.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _defaultValues = new PointsMaterial();  created once and reused.
JSWeakValue& defaults() { static thread_local JSWeakValue weak; return weak; }
// const _size = new Vector2();  shared by the render update callback below.
JSValue& sizeKeeper() { static thread_local JSValue value; return value; }
// const scale = uniform( 1 ).setGroup( renderGroup ).onRenderUpdate( ... )
JSValue& scaleKeeper() { static thread_local JSValue value; return value; }

JSValue defaultValues() {
    if (const auto value = defaults().lock(); value && value->toBoolean()) return *value;
    const auto created = nativePointsMaterialConstructor().construct();
    defaults() = JSWeakValue(created);
    return created;
}

JSValue vec3Node(const JSValue& value) {
    return nativeTSLBaseBinding("vec3").call({}, {value});
}
JSValue vec4Node(const JSValue& x, const JSValue& y, const JSValue& z) {
    return nativeTSLBaseBinding("vec4").call({}, {x, y, z});
}

// const scale = uniform( 1 ).setGroup( renderGroup ).onRenderUpdate( function ( { renderer } ) { const size = renderer.getSize( _size ); this.value = 0.5 * size.y; } );
JSValue scaleUniform() {
    if (!scaleKeeper().toBoolean()) {
        if (!sizeKeeper().toBoolean()) sizeKeeper() = nativeVector2Constructor().construct();
        auto uniform = nativeUniformFactory().call({}, {JSValue(1)});
        uniform = invoke(uniform, "setGroup")({nativeRenderGroup()});
        auto callback = JSValue::function(
            +[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                const auto update = arg(args, 0, nullptr);
                const auto size = invoke(update.get("renderer"), "getSize")({captures[0]});
                auto node = self;
                node.set("value", JSValue(0.5 * size.get("y").toNumber()));
                return size;
            },
            {sizeKeeper()}, "", 1);
        scaleKeeper() = invoke(uniform, "onRenderUpdate")({callback});
    }
    return scaleKeeper();
}

// setupPositionView() { const { positionNode } = this; return modelViewMatrix.mul( vec3( positionNode || positionLocal ) ).xyz; }
JSValue setupPositionView(const JSValue& self, const Args&, const Args&) {
    const auto positionNode = self.get("positionNode");
    const auto position = nativeModelNodeBinding("modelViewMatrix");
    const auto viewPosition = invoke(position, "mul")({
        vec3Node(positionNode.toBoolean() ? positionNode : nativePositionBinding("positionLocal"))});
    return viewPosition.get("xyz");
}

// setupVertexSprite( builder ) { ... }
JSValue setupVertexSprite(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto camera = builder.get("camera");
    const auto rotationNode = self.get("rotationNode");
    const auto scaleNode = self.get("scaleNode");
    const auto sizeNode = self.get("sizeNode");
    const auto sizeAttenuation = self.get("sizeAttenuation");

    // let mvp = super.setupVertex( builder )
    const auto basePrototype = nativeSpriteNodeMaterialBinding("default").get("prototype");
    auto mvp = basePrototype.get("setupVertex").call(self, {builder});

    if (!builder.get("material").get("isNodeMaterial").strictEquals(true)) return mvp;

    // let pointSize = sizeNode !== null ? vec2( sizeNode ) : materialPointSize
    auto pointSize = sizeNode.toBoolean()
        ? nativeTSLBaseBinding("vec2").call({}, {sizeNode})
        : nativeMaterialNodeBinding("materialPointSize");
    pointSize = invoke(pointSize, "mul")({nativeScreenNodeBinding("screenDPR")});
    if (camera.get("isPerspectiveCamera").toBoolean() && sizeAttenuation.strictEquals(true)) {
        // pointSize = pointSize.mul( scale.div( positionView.z.negate() ) )
        const auto divisor =
            invoke(scaleUniform(), "div")({invoke(nativePositionBinding("positionView").get("z"), "negate")()});
        pointSize = invoke(pointSize, "mul")({divisor});
    }
    if (scaleNode.toBoolean() && scaleNode.get("isNode").strictEquals(true)) {
        pointSize = invoke(pointSize, "mul")({nativeTSLBaseBinding("vec2").call({}, {scaleNode})});
    }

    auto offset = nativePositionBinding("positionGeometry").get("xy");
    if (rotationNode.toBoolean() && rotationNode.get("isNode").strictEquals(true)) {
        const auto rotation = nativeTSLBaseBinding("float").call({}, {rotationNode});
        offset = nativeRotateNodeBinding("rotate").call({}, {offset, rotation});
    }
    // offset = offset.mul( pointSize ).div( viewportSize.div( 2 ) ).mul( mvp.w )
    offset = invoke(offset, "mul")({pointSize});
    offset = invoke(offset, "div")({invoke(nativeScreenNodeBinding("viewportSize"), "div")({JSValue(2.0)})});
    offset = invoke(offset, "mul")({mvp.get("w")});
    // mvp = mvp.add( vec4( offset, 0, 0 ) )  — three arguments, because offset already fills xy.
    return invoke(mvp, "add")({vec4Node(offset, JSValue(0.0), JSValue(0.0))});
}

// setupVertex( builder ) { if ( builder.object.isPoints ) return super.setupVertex( builder ); else return this.setupVertexSprite( builder ); }
JSValue setupVertex(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto basePrototype = nativeSpriteNodeMaterialBinding("default").get("prototype");
    if (builder.get("object").get("isPoints").toBoolean()) {
        return basePrototype.get("setupVertex").call(self, {builder});
    }
    return invoke(self, "setupVertexSprite")({builder});
}

// get alphaToCoverage() { return this._useAlphaToCoverage; }
JSValue alphaToCoverageGetter(const JSValue& self, const Args&, const Args&) {
    return self.get("_useAlphaToCoverage");
}

// set alphaToCoverage( value ) { if ( this._useAlphaToCoverage !== value ) { ...; this.needsUpdate = true; } }
JSValue alphaToCoverageSetter(const JSValue& self, const Args& args, const Args&) {
    const auto value = arg(args, 0, nullptr);
    if (!self.get("_useAlphaToCoverage").strictEquals(value)) {
        auto selfRef = self;
        selfRef.set("_useAlphaToCoverage", value);
        selfRef.set("needsUpdate", true);
    }
    return {};
}
}  // namespace

JSValue nativePointsNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "PointsNodeMaterial module");
    nativeNodeMaterialModule();
    nativeSpriteNodeMaterialModule();
    nativeMaterialNodeModule();
    nativeModelNodeModule();
    nativePositionModule();
    nativeScreenNodeModule();
    nativeRotateNodeModule();
    nativeTSLBase();
    const auto base = nativeSpriteNodeMaterialConstructor();
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "PointsNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("sizeNode", nullptr);
            self.set("isPointsNodeMaterial", true);
            invoke(self, "setDefaultValues")({defaultValues()});
            invoke(self, "setValues")({arg(args, 0)});
            return self;
        },
        {}, PointsNodeMaterialSource::classSource, PointsNodeMaterialSource::typeSource);
    auto prototype = ctor.get("prototype");
    method(prototype, "setupPositionView", 0, setupPositionView, {},
           PointsNodeMaterialSource::setupPositionViewSource);
    method(prototype, "setupVertexSprite", 1, setupVertexSprite, {},
           PointsNodeMaterialSource::setupVertexSpriteSource);
    method(prototype, "setupVertex", 1, setupVertex, {}, PointsNodeMaterialSource::setupVertexSource);

    // get alphaToCoverage() / set alphaToCoverage( value )
    JSPropertyDescriptor accessor;
    accessor.configurable = true;
    accessor.getter = JSValue::function(alphaToCoverageGetter, {}, "get alphaToCoverage", 0);
    accessor.getter->setFunctionSourceText(PointsNodeMaterialSource::alphaToCoverageSource);
    accessor.setter = JSValue::function(alphaToCoverageSetter, {}, "set alphaToCoverage", 1);
    prototype.defineProperty("alphaToCoverage", accessor);

    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativePointsNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativePointsNodeMaterialModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativePointsNodeMaterialConstructor() {
    return nativePointsNodeMaterialBinding("default");
}
