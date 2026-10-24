#include "renderers/common/nodes/NodeUniform.h"
#include "renderers/common/Uniform.h"
#include "core/JSModuleObject.h"

JSValue nativeNodeUniforms(){
    using Args=std::vector<JSValue>;
    static thread_local JSWeakValue weak;static thread_local bool initializing=false;
    if(const auto module=weak.lock();module&&module->isObject())return *module;
    JSModuleInitializationScope scope(initializing,"renderer NodeUniform module");auto module=JSValue::object();const auto uniforms=nativeRendererUniforms();
    for(const auto* kind:{"Number","Vector2","Vector3","Vector4","Color","Matrix2","Matrix3","Matrix4"}){
        const auto name=std::string(kind)+"NodeUniform";const auto base=uniforms.get(std::string(kind)+"Uniform");auto state=JSValue::object(nullptr);
        auto ctor=JSValue::builtinConstructor([](const JSValue& target,const Args& args,const Args& c)->JSValue{
            const auto node=args.empty()?JSValue{}:args[0];
            // GetSuperConstructor precedes the argument property reads.
            const auto base=c[0].get("constructor").getPrototype();const auto name=node.get("name");const auto value=node.get("value");
            auto self=base.construct({name,value},target);self.set("nodeUniform",node);return self;
        },{state},name,1);
        state.set("constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));
        for(const auto* method:{"getValue","getType"}){JSPropertyDescriptor d;d.value=JSValue::function([](const JSValue& receiver,const Args&,const Args& c)->JSValue{
            return receiver.get("nodeUniform").get(c[0].strictEquals("getValue")?"value":"type");
        },{method},method,0);d.writable=d.configurable=true;prototype.defineProperty(method,d);}
        module.set(name,ctor);
    }
    module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
