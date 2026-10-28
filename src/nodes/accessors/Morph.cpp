// Ported from pinned three.js src/nodes/accessors/Morph.js; see THREEJS-LICENSE.
#include "nodes/accessors/Morph.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/utils/LoopNode.h"
#include "nodes/utils/EventNode.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/accessors/Position.h"
#include "nodes/accessors/Normal.h"
#include "nodes/core/IndexNode.h"
#include "textures/DataArrayTexture.h"
#include "math/Vector2.h"
#include "math/Vector4.h"
#include "constants.h"
#include "nodes/accessors/UniformArrayNode.h"
#include "core/JSGlobal.h"
#include "nodes/accessors/MorphSources.inc"

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue fallbackArray(const JSValue& value){return value.toBoolean()?value:JSValue::array();}
JSValue morphAttribute(const JSValue& geometry){
    auto value=geometry.get("morphAttributes").get("position");if(value.toBoolean())return value;
    value=geometry.get("morphAttributes").get("normal");if(value.toBoolean())return value;
    return geometry.get("morphAttributes").get("color");
}
JSValue getMorphBody(const JSValue&,const Args& a,const Args& c){
    const auto inputs=arg(a,0),imports=c[0];
    const auto bufferMap=inputs.get("bufferMap"),influence=inputs.get("influence"),stride=inputs.get("stride"),width=inputs.get("width"),depth=inputs.get("depth"),offset=inputs.get("offset");
    const auto texelIndex=invoke(invoke(imports.get("int").call({}, {imports.get("vertexIndex")}),"mul")({stride}),"add")({offset});
    const auto y=invoke(texelIndex,"div")({width});const auto subtract=invoke(texelIndex,"sub");const auto x=subtract({invoke(y,"mul")({width})});
    const auto textureLoad=imports.get("textureLoad");const auto uv=imports.get("ivec2").call({}, {x,y});const auto bufferAttrib=invoke(textureLoad.call({}, {bufferMap,uv}),"depth")({depth}).get("xyz");
    return invoke(bufferAttrib,"mul")({influence});
}
JSValue entryBody(const JSValue&,const Args& a,const Args& c){
    const auto geometry=arg(a,0),imports=c[0],textures=c[1],vector=c[2];
    const bool hasPosition=!geometry.get("morphAttributes").get("position").isUndefined();
    const bool hasNormal=!geometry.get("morphAttributes").get("normal").isUndefined();
    const bool hasColor=!geometry.get("morphAttributes").get("color").isUndefined();
    const auto attribute=morphAttribute(geometry),count=attribute.isUndefined()?JSValue(0):attribute.get("length");
    auto entry=invoke(textures,"get")({geometry});
    if(entry.isUndefined()||!entry.get("count").strictEquals(count)){
        if(!entry.isUndefined())invoke(entry.get("texture"),"dispose")();
        const auto targets=fallbackArray(geometry.get("morphAttributes").get("position"));
        const auto normals=fallbackArray(geometry.get("morphAttributes").get("normal"));
        const auto colors=fallbackArray(geometry.get("morphAttributes").get("color"));
        int vertexDataCount=0;if(hasPosition)vertexDataCount=1;if(hasNormal)vertexDataCount=2;if(hasColor)vertexDataCount=3;
        double width=geometry.get("attributes").get("position").get("count").toNumber()*vertexDataCount;JSValue height=1;
        if(width>4096){const auto math=nativeGlobalBinding("Math");const auto ceil=invoke(math,"ceil");height=ceil({width/4096});width=4096;}
        const auto bufferConstructor=nativeGlobalBinding("Float32Array");const auto bufferArea=width*height.toNumber()*4;auto buffer=bufferConstructor.construct({bufferArea*count.toNumber()});
        auto texture=imports.get("DataArrayTexture").construct({buffer,width,height,count});texture.set("type",imports.get("FloatType"));texture.set("needsUpdate",true);
        const int dataStride=vertexDataCount*4;
        for(double i=0;less(i,count);++i){
            const auto key=JSValue(i).toString();const auto target=targets.get(key),normal=normals.get(key),color=colors.get(key);const auto offset=width*height.toNumber()*4*i;
            for(double j=0;less(j,target.get("count"));++j){
                const auto stride=j*dataStride;
                if(hasPosition){
                    invoke(vector,"fromBufferAttribute")({target,j});
                    buffer.set(JSValue(offset+stride+0).toString(),vector.get("x"));buffer.set(JSValue(offset+stride+1).toString(),vector.get("y"));buffer.set(JSValue(offset+stride+2).toString(),vector.get("z"));buffer.set(JSValue(offset+stride+3).toString(),0);
                }
                if(hasNormal){
                    invoke(vector,"fromBufferAttribute")({normal,j});
                    buffer.set(JSValue(offset+stride+4).toString(),vector.get("x"));buffer.set(JSValue(offset+stride+5).toString(),vector.get("y"));buffer.set(JSValue(offset+stride+6).toString(),vector.get("z"));buffer.set(JSValue(offset+stride+7).toString(),0);
                }
                if(hasColor){
                    invoke(vector,"fromBufferAttribute")({color,j});
                    buffer.set(JSValue(offset+stride+8).toString(),vector.get("x"));buffer.set(JSValue(offset+stride+9).toString(),vector.get("y"));buffer.set(JSValue(offset+stride+10).toString(),vector.get("z"));buffer.set(JSValue(offset+stride+11).toString(),color.get("itemSize").strictEquals(4)?vector.get("w"):JSValue(1));
                }
            }
        }
        entry=JSValue::object();own(entry,"count",count);own(entry,"texture",texture);own(entry,"stride",vertexDataCount);own(entry,"size",imports.get("Vector2").construct({width,height}));
        invoke(textures,"set")({geometry,entry});
        auto holder=JSValue::object(nullptr);
        auto dispose=JSValue::constructor(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            invoke(c[0],"dispose")();invoke(c[1],"delete")({c[2]});const auto remove=invoke(c[2],"removeEventListener");remove({"dispose",c[3].get("disposeTexture")});return {};
        },{texture,textures,geometry,holder},"disposeTexture",0,JSValue::ConstructorKind::Callable);
        dispose.setFunctionSourceText(MorphSource::disposeTextureSource);holder.set("disposeTexture",dispose);invoke(geometry,"addEventListener")({"dispose",dispose});
    }
    return entry;
}
JSValue ifBody(const JSValue&,const Args&,const Args& c){
    const auto imports=c[0],getMorph=c[1],bufferMap=c[2],influence=c[3],stride=c[4],width=c[5],index=c[6];
    for(unsigned component=0;component<2;++component){
        if(!c[7+component].strictEquals(true))continue;
        const auto append=invoke(imports.get(component==0?"positionLocal":"normalLocal"),"addAssign");
        auto params=JSValue::object();own(params,"bufferMap",bufferMap);own(params,"influence",influence);own(params,"stride",stride);own(params,"width",width);own(params,"depth",index);own(params,"offset",imports.get("int").call({}, {component}));
        append({getMorph.call({}, {params})});
    }return {};
}
JSValue loopBody(const JSValue&,const Args& a,const Args& c){
    const auto index=arg(a,0).get("i"),imports=c[0],mesh=c[1],influences=c[2];
    const auto influence=invoke(imports.get("float").call({}, {0}),"toVar")();
    if(greater(mesh.get("count"),1)&&(!mesh.get("morphTexture").isNull()&&!mesh.get("morphTexture").isUndefined())){
        const auto assign=invoke(influence,"assign");const auto textureLoad=imports.get("textureLoad");const auto texture=mesh.get("morphTexture"),ivec2=imports.get("ivec2");
        const auto x=invoke(imports.get("int").call({}, {index}),"add")({1}),y=imports.get("int").call({}, {imports.get("instanceIndex")});
        assign({textureLoad.call({}, {texture,ivec2.call({}, {x,y})}).get("r")});
    }else{
        const auto assign=invoke(influence,"assign");assign({invoke(invoke(influences,"element")({index}),"toVar")()});
    }
    const auto If=imports.get("If"),condition=invoke(influence,"notEqual")({0});
    auto callback=JSValue::function(ifBody,{imports,c[3],c[4],influence,c[5],c[6],index,c[7],c[8]},"",0);callback.setFunctionSourceText(MorphSource::ifSource);If.call({}, {condition,callback});return {};
}
JSValue updateBody(const JSValue&,const Args& a,const Args& c){
    const auto object=arg(a,0).get("object"),data=c[0];auto base=data.get("base"),influences=data.get("influences");
    if(object.get("geometry").get("morphTargetsRelative").toBoolean())base.set("value",1);
    else{
        const auto reduce=invoke(object.get("morphTargetInfluences"),"reduce");auto callback=JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue{return add(arg(a,0),arg(a,1));},{},"",2);callback.setFunctionSourceText(MorphSource::reduceSource);
        base.set("value",1-reduce({callback,0}).toNumber());
    }
    if(influences.toBoolean()){influences.set("array",object.get("morphTargetInfluences"));invoke(influences,"update")();}return {};
}
JSValue referenceBody(const JSValue&,const Args& a,const Args& c){
    const auto mesh=TSLFunctionRuntime::first(arg(a,0)),imports=c[0],cache=c[1];const auto geometry=mesh.get("geometry");
    const bool hasPosition=!geometry.get("morphAttributes").get("position").isUndefined();
    const auto hasNormal=invoke(geometry,"hasAttribute")({"normal"});const auto normal=hasNormal.toBoolean()?JSValue(!geometry.get("morphAttributes").get("normal").isUndefined()):hasNormal;
    const auto attribute=morphAttribute(geometry),count=attribute.isUndefined()?JSValue(0):attribute.get("length");if(count.strictEquals(0))return {};
    auto data=invoke(cache,"get")({mesh});
    if(data.isUndefined()||!data.get("count").strictEquals(count)){
        data=JSValue::object();own(data,"base",imports.get("uniform").call({}, {1}));
        own(data,"influences",mesh.get("morphTargetInfluences").toBoolean()?imports.get("uniformArray").call({}, {mesh.get("morphTargetInfluences"),"float"}):JSValue(nullptr));own(data,"count",count);invoke(cache,"set")({mesh,data});
    }
    const auto base=data.get("base"),influences=data.get("influences");const auto entry=c[2].call({}, {geometry}),bufferMap=entry.get("texture"),stride=entry.get("stride"),size=entry.get("size");
    if(hasPosition)invoke(imports.get("positionLocal"),"mulAssign")({base});if(normal.strictEquals(true))invoke(imports.get("normalLocal"),"mulAssign")({base});
    const auto width=imports.get("int").call({}, {size.get("width")});
    const auto Loop=imports.get("Loop");auto loop=JSValue::function(loopBody,{imports,mesh,influences,c[3],bufferMap,stride,width,hasPosition,normal},"",1);loop.setFunctionSourceText(MorphSource::loopSource);Loop.call({}, {count,loop});
    const auto OnObjectUpdate=imports.get("OnObjectUpdate");auto update=JSValue::function(updateBody,{data},"",1);update.setFunctionSourceText(MorphSource::updateSource);OnObjectUpdate.call({}, {update});return {};
}
}

JSValue nativeMorphModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Morph module");
    const auto tsl=nativeTSLBase(),loop=nativeLoopNodeModule(),event=nativeEventNodeModule(),texture=nativeTextureNodeModule(),position=nativePositionModule(),normal=nativeNormalModule(),index=nativeIndexNodeModule();
    const auto dataArray=nativeDataArrayTextureModule(),vector2=nativeVector2Constructor(),vector4=nativeVector4Constructor(),constants=nativeConstantsModule(),uniformArray=nativeUniformArrayNodeModule();
    auto imports=JSValue::object(nullptr);for(const auto* name:{"float","Fn","ivec2","int","If","uniform"})own(imports,name,nativeTSLBaseBinding(name));
    own(imports,"Loop",nativeLoopNodeBinding("Loop"));own(imports,"OnObjectUpdate",nativeEventNodeBinding("OnObjectUpdate"));own(imports,"textureLoad",nativeTextureNodeBinding("textureLoad"));
    own(imports,"positionLocal",nativePositionBinding("positionLocal"));own(imports,"normalLocal",nativeNormalBinding("normalLocal"));own(imports,"instanceIndex",nativeIndexNodeBinding("instanceIndex"));own(imports,"vertexIndex",nativeIndexNodeBinding("vertexIndex"));
    own(imports,"DataArrayTexture",nativeDataArrayTextureBinding("DataArrayTexture"));own(imports,"Vector2",vector2);own(imports,"FloatType",nativeConstantsBinding("FloatType"));own(imports,"uniformArray",nativeUniformArrayNodeBinding("uniformArray"));
    const auto textures=nativeGlobalBinding("WeakMap").construct(),vector=vector4.construct(),influences=nativeGlobalBinding("WeakMap").construct();
    auto getMorphCallback=JSValue::function(getMorphBody,{imports},"",1);getMorphCallback.setFunctionSourceText(MorphSource::getMorphSource);const auto getMorph=imports.get("Fn").call({}, {getMorphCallback});
    auto getEntry=JSValue::constructor(entryBody,{imports,textures,vector},"getEntry",1,JSValue::ConstructorKind::Callable);getEntry.setFunctionSourceText(MorphSource::getEntrySource);
    auto reference=JSValue::function(referenceBody,{imports,influences,getEntry,getMorph},"",1);reference.setFunctionSourceText(MorphSource::morphReferenceSource);
    const auto morphReference=imports.get("Fn").call({}, {reference,"void"});auto bindings=JSValue::object(nullptr);own(bindings,"morphReference",morphReference);
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);auto module=JSValue::object();own(module,"morphReference",morphReference);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeMorphBinding(const std::string& name){const auto keeper=nativeMorphModule();return lexical().lock()->get(name);}
