#include "nodes/accessors/Instance.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/EventNode.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/accessors/BufferAttributeNode.h"
#include "nodes/accessors/BufferNode.h"
#include "nodes/accessors/StorageBufferNode.h"
#include "nodes/core/IndexNode.h"
#include "core/InstancedInterleavedBuffer.h"
#include "core/InstancedBufferAttribute.h"
#include "core/JSGlobal.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "constants.h"
#include <algorithm>
namespace {
using namespace UtilityNodeRuntime;
#include "nodes/accessors/InstanceSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue maximum(const JSValue& object,const char* key){const auto math=nativeGlobalBinding("Math");const auto max=invoke(math,"max");return max({object.get(key),1});}
JSValue matrixNode(const JSValue& imports,const JSValue& caches,const JSValue& builder,const JSValue& matrix){
    JSValue result;const auto count=maximum(matrix,"count");const auto storage=matrix.get("isStorageInstancedBufferAttribute").strictEquals(true);
    if(storage){const auto node=imports.get("storage").call({}, {matrix,"mat4",count});return invoke(node,"element")({imports.get("instanceIndex")});}
    const auto bytes=count.toNumber()*16*4;if(JSValue(bytes).toNumber()<=invoke(builder,"getUniformBufferLimit")().toNumber()){
        const auto node=imports.get("buffer").call({}, {matrix.get("array"),"mat4",count});return invoke(node,"element")({imports.get("instanceIndex")});
    }
    auto interleaved=invoke(caches.get("matrix"),"get")({matrix});if(!interleaved.toBoolean()){interleaved=imports.get("InstancedInterleavedBuffer").construct({matrix.get("array"),16,1});invoke(caches.get("matrix"),"set")({matrix,interleaved});}
    const auto factory=imports.get(matrix.get("usage").strictEquals(imports.get("DynamicDrawUsage"))?"instancedDynamicBufferAttribute":"instancedBufferAttribute");Args vectors;for(const auto offset:{0,4,8,12})vectors.push_back(factory.call({}, {interleaved,"vec4",16,offset}));return imports.get("mat4").call({},vectors);
}
JSValue previousNode(const JSValue& imports,const JSValue& caches,const JSValue& mesh,const JSValue& matrix,const JSValue& builder){
    auto data=invoke(caches.get("previous"),"get")({mesh});if(data.isUndefined()){
        const auto previous=invoke(matrix,"clone")();data=JSValue::object();own(data,"previousInstanceMatrix",previous);own(data,"node",matrixNode(imports,caches,builder,previous));invoke(caches.get("previous"),"set")({mesh,data});
    }return data.get("node");
}
JSValue beforeFrame(const JSValue&,const Args&,const Args& c){
    const auto interleavedMatrix=c[0],matrices=c[1],colors=c[2],interleavedColor=c[3];
    if(!interleavedMatrix.isNull()&&!interleavedMatrix.get("version").strictEquals(matrices.get("version"))){
        invoke(interleavedMatrix,"clearUpdateRanges")();const auto ranges=interleavedMatrix.get("updateRanges");const auto push=invoke(ranges,"push");Args values;each(matrices.get("updateRanges"),[&](const JSValue& value){values.push_back(value);});push(values);invoke(matrices,"clearUpdateRanges")();auto matrix=interleavedMatrix;matrix.set("version",matrices.get("version"));
    }
    if(colors.toBoolean()&&!interleavedColor.isNull()&&!interleavedColor.get("version").strictEquals(colors.get("version"))){
        invoke(interleavedColor,"clearUpdateRanges")();const auto ranges=interleavedColor.get("updateRanges");const auto push=invoke(ranges,"push");Args values;each(colors.get("updateRanges"),[&](const JSValue& value){values.push_back(value);});push(values);invoke(colors,"clearUpdateRanges")();auto color=interleavedColor;color.set("version",colors.get("version"));
    }return {};
}
JSValue afterObject(const JSValue&,const Args& a,const Args& c){
    const auto object=arg(a,0).get("object"),matrices=c[0],caches=c[1];const auto previous=invoke(caches.get("previous"),"get")({object}).get("previousInstanceMatrix");const auto array=previous.get("array");const auto set=invoke(array,"set");set({matrices.get("array")});auto destination=previous;destination.set("version",matrices.get("version"));const auto interleaved=invoke(caches.get("matrix"),"get")({previous});if(!interleaved.isUndefined()){auto buffer=interleaved;buffer.set("version",matrices.get("version"));}return {};
}
JSValue instanceBody(const JSValue&,const Args& a,const Args& c){
    const auto inputs=arg(a,0),builder=arg(a,1),imports=c[0],caches=c[1];auto iterator=getJSIterator(inputs);JSValue matrices,colors;try{if(const auto value=stepJSIterator(iterator))matrices=*value;if(const auto value=stepJSIterator(iterator))colors=*value;closeJSIterator(iterator);}catch(...){closeJSIterator(iterator,std::current_exception());}if(colors.isUndefined())colors=nullptr;
    const auto storageMatrix=matrices.get("isStorageInstancedBufferAttribute").strictEquals(true);const bool storageColor=colors.toBoolean()&&colors.get("isStorageInstancedBufferAttribute").strictEquals(true);
    const auto matrix=matrixNode(imports,caches,builder,matrices);JSValue interleavedMatrix=nullptr;
    if(!storageMatrix){const auto bytes=maximum(matrices,"count").toNumber()*16*4;if(bytes>invoke(builder,"getUniformBufferLimit")().toNumber())interleavedMatrix=invoke(caches.get("matrix"),"get")({matrices});}
    JSValue colorNode=nullptr,interleavedColor=nullptr;
    if(colors.toBoolean()){
        if(storageColor){const auto factory=imports.get("storage");const auto count=maximum(colors,"count");colorNode=invoke(factory.call({}, {colors,"vec3",count}),"element")({imports.get("instanceIndex")});}
        else{auto attribute=invoke(caches.get("color"),"get")({colors});if(!attribute.toBoolean()){attribute=imports.get("InstancedBufferAttribute").construct({colors.get("array"),3});invoke(caches.get("color"),"set")({colors,attribute});}interleavedColor=attribute;const auto factory=imports.get(colors.get("usage").strictEquals(imports.get("DynamicDrawUsage"))?"instancedDynamicBufferAttribute":"instancedBufferAttribute");const auto vec=imports.get("vec3");colorNode=vec.call({}, {factory.call({}, {attribute,"vec3",3,0})});}
    }
    if(!interleavedMatrix.isNull()||!interleavedColor.isNull()){auto callback=JSValue::function(beforeFrame,{interleavedMatrix,matrices,colors,interleavedColor},"",0);callback.setFunctionSourceText(InstanceSource::arrow1);imports.get("OnBeforeFrameUpdate").call({}, {callback});}
    const auto position=invoke(matrix,"mul")({imports.get("positionLocal")}).get("xyz");invoke(imports.get("positionLocal"),"assign")({position});
    if(invoke(builder,"needsPreviousData")().toBoolean()){
        const auto mesh=builder.get("object");auto callback=JSValue::function(afterObject,{matrices,caches},"",1);callback.setFunctionSourceText(InstanceSource::arrow2);imports.get("OnAfterObjectUpdate").call({}, {callback});const auto previous=previousNode(imports,caches,mesh,matrices,builder);const auto assign=invoke(imports.get("positionPrevious"),"assign");assign({invoke(previous,"mul")({imports.get("positionPrevious")}).get("xyz")});
    }
    if(invoke(builder,"hasGeometryAttribute")({"normal"}).toBoolean()){const auto normal=imports.get("transformNormal").call({}, {imports.get("normalLocal"),matrix});invoke(imports.get("normalLocal"),"assign")({normal});}
    if(!colorNode.isNull())invoke(imports.get("instanceColor"),"assign")({colorNode});return {};
}
}
JSValue nativeInstanceModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto m=weak.lock();m&&m->isObject())return *m;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Instance module");
    const auto tsl=nativeTSLBase(),event=nativeEventNodeModule(),normal=nativeNormalModule(),position=nativePositionModule(),property=nativePropertyNodeModule(),attribute=nativeBufferAttributeNodeModule(),buffer=nativeBufferNodeModule(),storage=nativeStorageBufferNodeModule(),index=nativeIndexNodeModule(),interleaved=nativeInstancedInterleavedBufferModule(),instanced=nativeInstancedBufferAttributeModule();
    auto imports=JSValue::object(nullptr);for(const auto* n:{"vec3","mat4","Fn"})own(imports,n,nativeTSLBaseBinding(n));for(const auto* n:{"OnAfterObjectUpdate","OnBeforeFrameUpdate"})own(imports,n,nativeEventNodeBinding(n));for(const auto* n:{"normalLocal","transformNormal"})own(imports,n,nativeNormalBinding(n));for(const auto* n:{"positionLocal","positionPrevious"})own(imports,n,nativePositionBinding(n));own(imports,"varyingProperty",nativePropertyNodeBinding("varyingProperty"));for(const auto* n:{"instancedBufferAttribute","instancedDynamicBufferAttribute"})own(imports,n,nativeBufferAttributeNodeBinding(n));own(imports,"buffer",nativeBufferNodeBinding("buffer"));own(imports,"storage",nativeStorageBufferNodeBinding("storage"));own(imports,"instanceIndex",nativeIndexNodeBinding("instanceIndex"));own(imports,"InstancedInterleavedBuffer",nativeInstancedInterleavedBufferConstructor());own(imports,"InstancedBufferAttribute",nativeInstancedBufferAttributeConstructor());own(imports,"DynamicDrawUsage",nativeConstantsBinding("DynamicDrawUsage"));
    auto caches=JSValue::object(nullptr);for(const auto* n:{"matrix","color","previous"})own(caches,n,nativeGlobalBinding("WeakMap").construct());auto bindings=JSValue::object(nullptr);const auto color=imports.get("varyingProperty").call({}, {"vec3","vInstanceColor"});own(imports,"instanceColor",color);own(bindings,"instanceColor",color);
    auto body=JSValue::function(instanceBody,{imports,caches},"",2);body.setFunctionSourceText(InstanceSource::arrow0);const auto instance=imports.get("Fn").call({}, {body,"void"});own(imports,"instance",instance);own(bindings,"instance",instance);
    auto meshBody=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{auto iterator=getJSIterator(arg(a,0));JSValue mesh;try{if(const auto value=stepJSIterator(iterator))mesh=*value;closeJSIterator(iterator);}catch(...){closeJSIterator(iterator,std::current_exception());}const auto matrix=mesh.get("instanceMatrix"),color=mesh.get("instanceColor");c[0].call({}, {matrix,color});return {};},{instance},"",1);meshBody.setFunctionSourceText(InstanceSource::arrow3);own(bindings,"instancedMesh",imports.get("Fn").call({}, {meshBody,"void"}));
    auto module=JSValue::object();for(const auto* n:{"instance","instanceColor","instancedMesh"})own(module,n,bindings.get(n));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeInstanceBinding(const std::string& name){const auto keeper=nativeInstanceModule();return lexical().lock()->get(name);}
