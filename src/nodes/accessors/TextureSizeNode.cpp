#include "nodes/accessors/TextureSizeNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& self,const Args& args,const Args& c){
    if(c[0].strictEquals("isCacheable"))return false;const auto builder=arg(args,0);
    const auto textureProperty=invoke(self.get("textureNode"),"build")({builder,"property"});
    const auto level=self.get("levelNode").isNull()?JSValue("0"):invoke(self.get("levelNode"),"build")({builder,"int"});
    const auto size=invoke(builder,"generateTextureSize");const auto value=self.get("textureNode").get("value");const auto snippet=size({value,textureProperty,level});
    const auto format=invoke(builder,"format");const auto type=invoke(self,"getNodeType")({builder});return format({snippet,type});
}
}
JSValue nativeTextureSizeNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"TextureSizeNode module");const auto base=nativeNodeConstructor(),tsl=nativeTSLBase();
    const auto ctor=makeConstructor("TextureSizeNode",1,+[](const JSValue& target,const Args& args,const Args& c)->JSValue{
        auto self=superConstruct(target,c,{"uvec2"});self.set("isTextureSizeNode",true);self.set("textureNode",arg(args,0));self.set("levelNode",arg(args,1,nullptr));return self;
    },{{"isCacheable",0},{"generate",1}},method);
    const auto proxy=nativeTSLBaseBinding("nodeProxy").call({}, {ctor});const auto textureSize=invoke(proxy,"setParameterLength")({1,2});
    auto bindings=JSValue::object(nullptr);own(bindings,"default",ctor);own(bindings,"textureSize",textureSize);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);
    auto module=JSValue::object();own(module,"default",ctor);own(module,"textureSize",textureSize);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeTextureSizeNodeBinding(const std::string& name){const auto keeper=nativeTextureSizeNodeModule();return lexical().lock()->get(name);}
JSValue nativeTextureSizeNodeConstructor(){return nativeTextureSizeNodeBinding("default");}
TextureSizeNode::TextureSizeNode(const JSValue& texture,const JSValue& level):object_(constructor().construct({texture,level})){}
JSValue TextureSizeNode::generate(const JSValue& builder)const{return object_.get("generate").call(object_,{builder});}
