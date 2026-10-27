#include "nodes/accessors/VertexColorNode.h"
#include "nodes/core/AttributeNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "math/Vector4.h"
namespace {
using namespace UtilityNodeRuntime;
#include "nodes/accessors/VertexColorNodeSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& receiver,const Args& args,const Args& c){
    auto self=receiver;const auto key=c[0];const auto builder=arg(args,0);
    if(key.strictEquals("getAttributeName")){const auto index=self.get("index");return add("color",greater(index,0)?index:JSValue(""));}
    if(key.strictEquals("generate")){
        const auto name=invoke(self,"getAttributeName")({builder});const auto attribute=invoke(builder,"hasGeometryAttribute")({name});
        if(attribute.strictEquals(true))return c[1].getPrototype().get("generate",self).call(self,{builder});
        const auto generate=invoke(builder,"generateConst");const auto type=self.get("nodeType");return generate({type,c[2].construct({1,1,1,1})});
    }
    c[1].getPrototype().get(key.toPropertyKey(),self).call(self,{builder});
    if(key.strictEquals("serialize")){auto data=builder;data.set("index",self.get("index"));}
    else self.set("index",builder.get("index"));return {};
}
}
JSValue nativeVertexColorNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto m=weak.lock();m&&m->isObject())return *m;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"VertexColorNode module");const auto base=nativeAttributeNodeConstructor(),vector=nativeVector4Constructor();auto holder=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& c)->JSValue{auto self=superConstruct(target,c,{nullptr,"vec4"});self.set("isVertexColorNode",true);self.set("index",arg(args,0));return self;},{holder},"VertexColorNode",1);holder.set("constructor",ctor);ctor.setPrototype(base);ctor.setFunctionSourceText(VertexColorNodeSource::classSource);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type;type.configurable=true;auto getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "VertexColorNode";},{},"get type",0);getter.setFunctionSourceText(VertexColorNodeSource::typeSource);type.getter=getter;ctor.defineProperty("type",type);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"getAttributeName",0},{"generate",1},{"serialize",1},{"deserialize",1}}){auto fn=JSValue::function(method,{name,p,vector},name,arity);const std::string key=name;fn.setFunctionSourceText(key=="getAttributeName"?VertexColorNodeSource::getAttributeNameSource:key=="generate"?VertexColorNodeSource::generateSource:key=="serialize"?VertexColorNodeSource::serializeSource:VertexColorNodeSource::deserializeSource);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;p.defineProperty(name,d);}
    auto factory=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{return c[0].construct({arg(a,0,0)});},{ctor},"vertexColor",0);factory.setFunctionSourceText(VertexColorNodeSource::arrow0);auto bindings=JSValue::object(nullptr),module=JSValue::object();own(bindings,"default",ctor);own(bindings,"vertexColor",factory);own(module,"default",ctor);own(module,"vertexColor",factory);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeVertexColorNodeBinding(const std::string& name){const auto keeper=nativeVertexColorNodeModule();return lexical().lock()->get(name);}
JSValue nativeVertexColorNodeConstructor(){return nativeVertexColorNodeBinding("default");}
