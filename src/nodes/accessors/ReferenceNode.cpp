// Native managed port of pinned three.js ReferenceNode.js.
#include "nodes/accessors/ReferenceNode.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/accessors/CubeTextureNode.h"
#include "nodes/accessors/BufferNode.h"
#include "nodes/accessors/UniformArrayNode.h"
#include "nodes/accessors/ReferenceElementNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "core/JSGlobal.h"
#include "utils.h"
#include <algorithm>
#include <string_view>
namespace {
using namespace UtilityNodeRuntime;
#include "ReferenceNodeSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0],imports=captures[2];
    if(name.strictEquals("isCacheable"))return false;
    if(name.strictEquals("element")){
        const auto ctor=imports.get("ReferenceElementNode"),index=imports.get("nodeObject").call({}, {arg(args,0)});return ctor.construct({self,index});
    }
    if(name.strictEquals("setGroup")){self.set("group",arg(args,0));return self;}
    if(name.strictEquals("setName")){self.set("name",arg(args,0));return self;}
    if(name.strictEquals("label")){
        ThreeUtils::warn({"TSL: \"label()\" has been deprecated. Use \"setName()\" instead."});return invoke(self,"setName")({arg(args,0)});
    }
    if(name.strictEquals("setNodeType")){
        const auto uniformType=arg(args,0);auto node=JSValue(nullptr);
        if(!self.get("count").isNull()){
            const auto buffer=imports.get("buffer"),count=self.get("count");node=buffer.call({}, {nullptr,uniformType,count});
        }else{
            const auto isArray=invoke(nativeGlobalBinding("Array"),"isArray");const auto value=invoke(self,"getValueFromReference")();
            if(isArray({value}).toBoolean()){
                node=imports.get("uniformArray").call({}, {nullptr,uniformType});node.set("updateType",imports.get("NodeUpdateType").get("OBJECT"));
            }else if(uniformType.strictEquals("texture"))node=imports.get("texture").call({}, {nullptr});
            else if(uniformType.strictEquals("cubeTexture"))node=imports.get("cubeTexture").call({}, {nullptr});
            else node=imports.get("uniform").call({}, {nullptr,uniformType});
        }
        if(!self.get("group").isNull()){const auto set=invoke(node,"setGroup");set({self.get("group")});}
        if(!self.get("name").isNull()){const auto set=invoke(node,"setName");set({self.get("name")});}
        self.set("node",node);return {};
    }
    if(name.strictEquals("generateNodeType")){
        const auto builder=arg(args,0);if(self.get("node").isNull()){invoke(self,"updateReference")({builder});invoke(self,"updateValue")();}
        return invoke(self.get("node"),"getNodeType")({builder});
    }
    if(name.strictEquals("getValueFromReference")){
        const auto object=args.empty()||args[0].isUndefined()?self.get("reference"):args[0],properties=self.get("properties");auto value=object.get(properties.get("0"));
        for(double i=1;less(i,properties.get("length"));++i)value=value.get(properties.get(JSValue(i).toString()));return value;
    }
    if(name.strictEquals("updateReference")){
        self.set("reference",!self.get("object").isNull()?self.get("object"):arg(args,0).get("object"));return self.get("reference");
    }
    if(name.strictEquals("setup")){invoke(self,"updateValue")();return self.get("node");}
    if(name.strictEquals("update")){invoke(self,"updateValue")();return {};}
    if(name.strictEquals("updateValue")){
        if(self.get("node").isNull()){const auto set=invoke(self,"setNodeType");set({self.get("uniformType")});}
        const auto value=invoke(self,"getValueFromReference")();const auto isArray=invoke(nativeGlobalBinding("Array"),"isArray");
        if(isArray({value}).toBoolean()){auto node=self.get("node");node.set("array",value);}else{auto node=self.get("node");node.set("value",value);}return {};
    }
    throw std::logic_error("Unknown ReferenceNode method");
}
}
JSValue nativeReferenceNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ReferenceNode module");
    const auto node=nativeNodeConstructor(),types=nativeNodeConstantsBinding("NodeUpdateType"),uniform=nativeUniformNodeConstructor(),uniformFactory=nativeUniformFactory();
    const auto texture=nativeTextureNodeModule(),cube=nativeCubeTextureNodeModule(),buffer=nativeBufferNodeModule(),tsl=nativeTSLBase(),uniformArray=nativeUniformArrayNodeModule(),element=nativeReferenceElementNodeModule();
    auto imports=JSValue::object(nullptr),holder=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
    own(imports,"NodeUpdateType",types);own(imports,"uniform",uniformFactory);own(imports,"texture",nativeTextureNodeBinding("texture"));own(imports,"cubeTexture",nativeCubeTextureNodeBinding("cubeTexture"));
    own(imports,"buffer",nativeBufferNodeBinding("buffer"));own(imports,"nodeObject",nativeTSLBaseBinding("nodeObject"));own(imports,"uniformArray",nativeUniformArrayNodeBinding("uniformArray"));own(imports,"ReferenceElementNode",nativeReferenceElementNodeBinding("default"));
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& captures)->JSValue{
        const auto holder=captures[0],imports=captures[1];auto self=holder.get("constructor").getPrototype().construct({},target);const auto property=arg(args,0),object=arg(args,2,nullptr);
        self.set("property",property);self.set("uniformType",arg(args,1));self.set("object",object);self.set("count",arg(args,3,nullptr));
        self.set("properties",invoke(property,"split")({"."}));self.set("reference",object);self.set("node",nullptr);self.set("group",nullptr);self.set("name",nullptr);self.set("updateType",imports.get("NodeUpdateType").get("OBJECT"));return self;
    },{holder,imports},"ReferenceNode",2);holder.set("constructor",ctor);ctor.setPrototype(node);auto p=ctor.get("prototype");p.setPrototype(node.get("prototype"));ctor.setFunctionSourceText(ReferenceNodeSource::ReferenceNode);
    JSPropertyDescriptor type;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "ReferenceNode";},{},"get type",0);type.getter->setFunctionSourceText(ReferenceNodeSource::get_type);type.configurable=true;ctor.defineProperty("type",type);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"isCacheable",0},{"element",1},{"setGroup",1},{"setName",1},{"label",1},{"setNodeType",1},{"generateNodeType",1},{"getValueFromReference",0},{"updateReference",1},{"setup",0},{"update",0},{"updateValue",0}}){JSPropertyDescriptor d;d.value=JSValue::function(method,{name,p,imports},name,arity);d.value->setFunctionSourceText(ReferenceNodeSource::get(name));d.writable=d.configurable=true;p.defineProperty(name,d);}
    own(bindings,"default",ctor);for(const auto* name:{"reference","referenceBuffer"}){auto f=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{
        if(captures[1].strictEquals("reference"))return captures[0].construct({arg(args,0),arg(args,1),arg(args,2)});
        return captures[0].construct({arg(args,0),arg(args,1),arg(args,3),arg(args,2)});
    },{ctor,name},name,std::string_view(name)=="reference"?3:4);f.setFunctionSourceText(ReferenceNodeSource::get(name));own(bindings,name,f);}
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& key:names)own(module,key.c_str(),bindings.get(key));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeReferenceNodeBinding(const std::string& name){const auto keeper=nativeReferenceNodeModule();return lexical().lock()->get(name);}
JSValue nativeReferenceNodeConstructor(){return nativeReferenceNodeBinding("default");}
