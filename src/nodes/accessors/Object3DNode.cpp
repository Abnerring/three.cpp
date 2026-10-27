// Ported from pinned three.js Object3DNode.js; see THREEJS-LICENSE.
#include "nodes/accessors/Object3DNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "math/Vector3.h"
#include "math/Sphere.h"
#include <algorithm>

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue scopeType(const JSValue& scope,const JSValue& ctor){
    if(scope.strictEquals(ctor.get("WORLD_MATRIX")))return "mat4";
    if(scope.strictEquals(ctor.get("POSITION"))||scope.strictEquals(ctor.get("VIEW_POSITION"))||scope.strictEquals(ctor.get("DIRECTION"))||scope.strictEquals(ctor.get("SCALE")))return "vec3";
    if(scope.strictEquals(ctor.get("RADIUS")))return "float";return {};
}
JSValue method(const JSValue& receiver,const Args& args,const Args& c){
    auto self=receiver;const auto name=c[0],imports=c[2],ctor=imports.get("constructor");
    if(name.strictEquals("isCacheable"))return false;
    if(name.strictEquals("generateNodeType"))return scopeType(self.get("scope"),ctor);
    if(name.strictEquals("update")){
        const auto object=self.get("object3d"),uniform=self.get("uniformNode"),scope=self.get("scope");auto mutableUniform=uniform;
        const auto initializeVector=[&](){const auto value=uniform.get("value");mutableUniform.set("value",value.toBoolean()?value:imports.get("Vector3").construct());};
        if(scope.strictEquals(ctor.get("WORLD_MATRIX")))mutableUniform.set("value",object.get("matrixWorld"));
        else if(scope.strictEquals(ctor.get("POSITION"))){initializeVector();invoke(uniform.get("value"),"setFromMatrixPosition")({object.get("matrixWorld")});}
        else if(scope.strictEquals(ctor.get("SCALE"))){initializeVector();invoke(uniform.get("value"),"setFromMatrixScale")({object.get("matrixWorld")});}
        else if(scope.strictEquals(ctor.get("DIRECTION"))){initializeVector();invoke(object,"getWorldDirection")({uniform.get("value")});}
        else if(scope.strictEquals(ctor.get("VIEW_POSITION"))){const auto camera=arg(args,0).get("camera");initializeVector();invoke(uniform.get("value"),"setFromMatrixPosition")({object.get("matrixWorld")});invoke(uniform.get("value"),"applyMatrix4")({camera.get("matrixWorldInverse")});}
        else if(scope.strictEquals(ctor.get("RADIUS"))){const auto geometry=arg(args,0).get("object").get("geometry");if(geometry.get("boundingSphere").isNull())invoke(geometry,"computeBoundingSphere")();const auto sphere=invoke(imports.get("sphere"),"copy")({geometry.get("boundingSphere")});invoke(sphere,"applyMatrix4")({object.get("matrixWorld")});mutableUniform.set("value",imports.get("sphere").get("radius"));}
        return {};
    }
    if(name.strictEquals("generate")){
        const auto type=scopeType(self.get("scope"),ctor);if(!type.isUndefined()){auto uniform=self.get("uniformNode");uniform.set("nodeType",type);}return invoke(self.get("uniformNode"),"build")({arg(args,0)});
    }
    if(name.strictEquals("serialize")||name.strictEquals("deserialize")){superMethod(self,name.string().c_str(),c,{arg(args,0)});if(name.strictEquals("serialize")){auto data=arg(args,0);data.set("scope",self.get("scope"));}else self.set("scope",arg(args,0).get("scope"));return {};}
    throw std::logic_error("Unknown Object3DNode method");
}
}
JSValue nativeObject3DNodeConstructor(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto ctor=weak.lock();ctor&&ctor->isConstructible())return *ctor;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Object3DNode");
    const auto base=nativeNodeConstructor(),uniform=nativeUniformNodeConstructor(),tsl=nativeTSLBase();auto imports=JSValue::object(nullptr),holder=JSValue::object(nullptr);
    own(imports,"UniformNode",uniform);own(imports,"Vector3",nativeVector3Constructor());own(imports,"sphere",nativeSphereConstructor().construct());
    own(imports,"NodeUpdateType",nativeNodeConstantsBinding("NodeUpdateType"));
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& c)->JSValue{
        auto self=superConstruct(target,c);const auto imports=c[0].get("imports");self.set("scope",arg(args,0));self.set("object3d",arg(args,1,nullptr));self.set("updateType",imports.get("NodeUpdateType").get("OBJECT"));self.set("uniformNode",imports.get("UniformNode").construct({nullptr}));return self;
    },{holder},"Object3DNode",1);holder.set("constructor",ctor);holder.set("imports",imports);own(imports,"constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "Object3DNode";},{},"get type",0);type.configurable=true;ctor.defineProperty("type",type);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"isCacheable",0},{"generateNodeType",0},{"update",1},{"generate",1},{"serialize",1},{"deserialize",1}}){JSPropertyDescriptor d;d.value=JSValue::function(method,{name,prototype,imports},name,arity);d.writable=d.configurable=true;prototype.defineProperty(name,d);}
    for(const auto& [key,value]:std::initializer_list<std::pair<const char*,const char*>>{{"WORLD_MATRIX","worldMatrix"},{"POSITION","position"},{"SCALE","scale"},{"VIEW_POSITION","viewPosition"},{"DIRECTION","direction"},{"RADIUS","radius"}})own(ctor,key,value);
    ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;
}
JSValue nativeObject3DNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Object3DNode exports");
    const auto ctor=nativeObject3DNodeConstructor(),proxy=nativeTSLBaseBinding("nodeProxy");auto bindings=JSValue::object(nullptr);own(bindings,"default",ctor);
    for(const auto& [name,key]:std::initializer_list<std::pair<const char*,const char*>>{{"objectDirection","DIRECTION"},{"objectWorldMatrix","WORLD_MATRIX"},{"objectPosition","POSITION"},{"objectScale","SCALE"},{"objectViewPosition","VIEW_POSITION"},{"objectRadius","RADIUS"}})own(bindings,name,invoke(proxy.call({}, {ctor,ctor.get(key)}),"setParameterLength")({1}));
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeObject3DNodeBinding(const std::string& name){const auto keeper=nativeObject3DNodeModule();return lexical().lock()->get(name);}
const std::vector<std::string>& Object3DNode::scopes(){static const std::vector<std::string> names{"position","viewPosition","direction","scale","worldMatrix","radius"};return names;}
std::string Object3DNode::nodeType(const std::string& scope){if(scope=="worldMatrix")return "mat4";if(scope=="radius")return "float";if(scope=="position"||scope=="viewPosition"||scope=="direction"||scope=="scale")return "vec3";return {};}
