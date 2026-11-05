#include "materials/nodes/MeshPhongNodeMaterial.h"

#include "materials/MeshPhongMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "materials/nodes/NodeMaterialSources.inc"
#include "nodes/accessors/MaterialNode.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/functions/PhongLightingModel.h"
#include "nodes/lighting/BasicEnvironmentNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _defaultValues = new MeshPhongMaterial();  created once and reused.
JSWeakValue& defaults() { static thread_local JSWeakValue weak; return weak; }

JSValue defaultValues() {
    if (const auto value = defaults().lock(); value && value->toBoolean()) return *value;
    const auto created = nativeMeshPhongMaterialConstructor().construct();
    defaults() = JSWeakValue(created);
    return created;
}

// setupEnvironment( builder ) { const envNode = super.setupEnvironment( builder ); return envNode ? new BasicEnvironmentNode( envNode ) : null; }
JSValue setupEnvironment(const JSValue& self, const Args& args, const Args&) {
    const auto basePrototype = nativeNodeMaterialBinding("default").get("prototype");
    const auto envNode = basePrototype.get("setupEnvironment").call(self, {arg(args, 0, nullptr)});
    if (!envNode.toBoolean()) return JSValue(nullptr);
    return nativeBasicEnvironmentNodeConstructor().construct({envNode});
}

// setupLightingModel( /*builder*/ ) { return new PhongLightingModel(); }
JSValue setupLightingModel(const JSValue&, const Args&, const Args&) {
    return nativePhongLightingModelConstructor().construct();
}

// setupVariants( /*builder*/ ) { const shininessNode = ( this.shininessNode ? float( this.shininessNode ) : materialShininess ).max( 1e-4 ); shininess.assign( shininessNode ); const specularNode = this.specularNode || materialSpecular; specularColor.assign( specularNode ); }
JSValue setupVariants(const JSValue& self, const Args&, const Args&) {
    auto shininessNode = self.get("shininessNode").toBoolean()
        ? nativeTSLBaseBinding("float").call({}, {self.get("shininessNode")})
        : nativeMaterialNodeBinding("materialShininess");
    // to prevent pow( 0.0, 0.0 )
    shininessNode = invoke(shininessNode, "max")({JSValue(1e-4)});
    invoke(nativePropertyNodeBinding("shininess"), "assign")({shininessNode});

    const auto specularNode = self.get("specularNode").toBoolean()
        ? self.get("specularNode")
        : nativeMaterialNodeBinding("materialSpecular");
    invoke(nativePropertyNodeBinding("specularColor"), "assign")({specularNode});
    return {};
}
}  // namespace

JSValue nativeMeshPhongNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "MeshPhongNodeMaterial module");
    nativeNodeMaterialModule();
    nativeBasicEnvironmentNodeModule();
    nativePhongLightingModelModule();
    nativeMaterialNodeModule();
    nativePropertyNodeModule();
    nativeTSLBase();
    const auto base = nativeNodeMaterialBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "MeshPhongNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("isMeshPhongNodeMaterial", true);
            self.set("lights", true);
            self.set("shininessNode", nullptr);
            self.set("specularNode", nullptr);
            invoke(self, "setDefaultValues")({defaultValues()});
            invoke(self, "setValues")({arg(args, 0)});
            return self;
        },
        {}, MeshPhongNodeMaterialSource::classSource, MeshPhongNodeMaterialSource::typeSource);
    auto prototype = ctor.get("prototype");
    method(prototype, "setupEnvironment", 1, setupEnvironment, {},
           MeshPhongNodeMaterialSource::setupEnvironmentSource);
    method(prototype, "setupLightingModel", 1, setupLightingModel, {},
           MeshPhongNodeMaterialSource::setupLightingModelSource);
    method(prototype, "setupVariants", 1, setupVariants, {},
           MeshPhongNodeMaterialSource::setupVariantsSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeMeshPhongNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeMeshPhongNodeMaterialModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeMeshPhongNodeMaterialConstructor() {
    return nativeMeshPhongNodeMaterialBinding("default");
}
