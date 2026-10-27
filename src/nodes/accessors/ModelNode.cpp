// Ported from pinned three.js ModelNode.js; see THREEJS-LICENSE.
#include "nodes/accessors/ModelNode.h"
#include "nodes/accessors/Object3DNode.h"
#include "nodes/accessors/BuiltinNode.h"
#include "nodes/accessors/Camera.h"
#include "nodes/accessors/UniformArrayNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include <algorithm>

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSWeakValue& importCache(){static thread_local JSWeakValue weak;return weak;}
JSValue imports(){
    if(const auto value=importCache().lock();value&&value->isObject())return *value;
    const auto object=nativeObject3DNodeModule(),tsl=nativeTSLBase();const auto uniform=nativeUniformFactory(),matrix4=nativeMatrix4Constructor();
    const auto builtin=nativeBuiltinNodeModule(),camera=nativeCameraModule(),array=nativeUniformArrayNodeModule(),matrix3=nativeMatrix3Constructor();auto table=JSValue::object(nullptr);
    own(table,"Object3DNode",nativeObject3DNodeBinding("default"));own(table,"uniform",uniform);own(table,"Matrix3",matrix3);own(table,"Matrix4",matrix4);
    own(table,"builtin",nativeBuiltinNodeBinding("builtin"));own(table,"cameraIndex",nativeCameraBinding("cameraIndex"));own(table,"cameraViewMatrix",nativeCameraBinding("cameraViewMatrix"));own(table,"uniformArray",nativeUniformArrayNodeBinding("uniformArray"));
    own(table,"Fn",nativeTSLBaseBinding("Fn"));own(table,"nodeImmutable",nativeTSLBaseBinding("nodeImmutable"));own(table,"modelViewMatrix",matrix4.construct());
    table.retainForHeapLifetime();importCache()=JSWeakValue(table);return table;
}
JSValue updateCallback(const JSValue&,const Args& args,const Args& c){
    const auto kind=c[0],table=c[1],frame=arg(args,0),object=frame.get("object");
    if(kind.strictEquals("normal"))return invoke(arg(args,1).get("value"),"getNormalMatrix")({object.get("matrixWorld")});
    if(kind.strictEquals("inverse")){const auto copy=invoke(arg(args,1).get("value"),"copy")({object.get("matrixWorld")});return invoke(copy,"invert")();}
    const auto camera=frame.get("camera");
    if(kind.strictEquals("viewArray")||kind.strictEquals("normalArray")){
        const auto subCameras=camera.get("cameras"),array=arg(args,1).get("array");const auto length=subCameras.get("length");
        for(double i=0;less(JSValue(i),length);++i){
            if(kind.strictEquals("viewArray"))invoke(array.get(JSValue(i)),"multiplyMatrices")({subCameras.get(JSValue(i)).get("matrixWorldInverse"),object.get("matrixWorld")});
            else{invoke(table.get("modelViewMatrix"),"multiplyMatrices")({subCameras.get(JSValue(i)).get("matrixWorldInverse"),object.get("matrixWorld")});invoke(array.get(JSValue(i)),"getNormalMatrix")({table.get("modelViewMatrix")});}
        }return {};
    }
    if(kind.strictEquals("view"))return invoke(object.get("modelViewMatrix"),"multiplyMatrices")({camera.get("matrixWorldInverse"),object.get("matrixWorld")});
    if(!c[2].strictEquals(true))invoke(object.get("modelViewMatrix"),"multiplyMatrices")({camera.get("matrixWorldInverse"),object.get("matrixWorld")});
    return invoke(object.get("normalMatrix"),"getNormalMatrix")({object.get("modelViewMatrix")});
}
JSValue callback(const char* kind,const JSValue& table,const JSValue& precision={}){return JSValue::function(updateCallback,{kind,table,precision},"",std::string(kind)=="normal"||std::string(kind)=="inverse"||std::string(kind).ends_with("Array")?2:1);}
JSValue modelFn(const JSValue&,const Args& args,const Args& c){
    const auto kind=c[0],table=c[1],bindings=c[2],builder=arg(args,0);
    if(kind.strictEquals("modelViewMatrix")){const auto custom=builder.get("context").get("modelViewMatrix");return custom.toBoolean()?custom:bindings.get("mediumpModelViewMatrix");}
    JSValue precision;
    if(kind.strictEquals("highpModelViewMatrix")){auto context=builder.get("context");context.set("isHighPrecisionModelViewMatrix",true);}
    else precision=builder.get("context").get("isHighPrecisionModelViewMatrix");
    const auto camera=builder.get("camera");const bool normal=kind.strictEquals("highpModelNormalViewMatrix");
    if(camera.get("isArrayCamera").toBoolean()&&greater(camera.get("cameras").get("length"),0)){
        auto matrices=JSValue::array();for(double i=0;less(JSValue(i),camera.get("cameras").get("length"));++i)invoke(matrices,"push")({table.get(normal?"Matrix3":"Matrix4").construct()});
        const auto array=invoke(table.get("uniformArray").call({}, {matrices}),"onObjectUpdate")({callback(normal?"normalArray":"viewArray",table)});
        const auto element=invoke(array,"element");const auto index=camera.get("isMultiViewCamera").toBoolean()?table.get("builtin").call({}, {"gl_ViewID_OVR"}):table.get("cameraIndex");return element({index});
    }
    return invoke(table.get("uniform").call({}, {normal?"mat3":"mat4"}),"onObjectUpdate")({callback(normal?"normalView":"view",table,precision)});
}
JSValue fnVariable(const char* name,const JSValue& table,const JSValue& bindings){
    const auto body=JSValue::function(modelFn,{name,table,bindings},"",1);const auto fn=invoke(table.get("Fn").call({}, {body}),"once")();return invoke(fn.call(),"toVar")({name});
}
}
JSValue nativeModelNodeConstructor(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto ctor=weak.lock();ctor&&ctor->isConstructible())return *ctor;
    const auto table=imports(),base=table.get("Object3DNode");auto holder=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& c)->JSValue{return superConstruct(target,c,{arg(args,0)});},{holder},"ModelNode",1);holder.set("constructor",ctor);ctor.setPrototype(base);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "ModelNode";},{},"get type",0);type.configurable=true;ctor.defineProperty("type",type);
    JSPropertyDescriptor update;update.value=JSValue::function(+[](const JSValue& receiver,const Args& args,const Args& c)->JSValue{auto self=receiver;self.set("object3d",arg(args,0).get("object"));superMethod(self,"update",c,{arg(args,0)});return {};},{"update",p},"update",1);update.writable=update.configurable=true;p.defineProperty("update",update);
    ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;
}
JSValue nativeModelNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"ModelNode exports");
    const auto ctor=nativeModelNodeConstructor(),table=imports();auto bindings=JSValue::object(nullptr);own(bindings,"default",ctor);
    for(const auto& [name,key]:std::initializer_list<std::pair<const char*,const char*>>{{"modelDirection","DIRECTION"},{"modelWorldMatrix","WORLD_MATRIX"},{"modelPosition","POSITION"},{"modelScale","SCALE"},{"modelViewPosition","VIEW_POSITION"},{"modelRadius","RADIUS"}})own(bindings,name,table.get("nodeImmutable").call({}, {ctor,ctor.get(key)}));
    own(bindings,"modelNormalMatrix",invoke(table.get("uniform").call({}, {table.get("Matrix3").construct()}),"onObjectUpdate")({callback("normal",table)}));
    own(bindings,"modelWorldMatrixInverse",invoke(table.get("uniform").call({}, {table.get("Matrix4").construct()}),"onObjectUpdate")({callback("inverse",table)}));
    own(bindings,"modelViewMatrix",fnVariable("modelViewMatrix",table,bindings));
    own(bindings,"mediumpModelViewMatrix",invoke(table.get("cameraViewMatrix"),"mul")({bindings.get("modelWorldMatrix")}));
    own(bindings,"highpModelViewMatrix",fnVariable("highpModelViewMatrix",table,bindings));
    own(bindings,"highpModelNormalViewMatrix",fnVariable("highpModelNormalViewMatrix",table,bindings));
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeModelNodeBinding(const std::string& name){const auto keeper=nativeModelNodeModule();return lexical().lock()->get(name);}
const std::vector<std::string>& ModelNode::scopes(){return Object3DNode::scopes();}
bool ModelNode::isScope(const std::string& name){const auto& values=scopes();return std::find(values.begin(),values.end(),name)!=values.end();}
