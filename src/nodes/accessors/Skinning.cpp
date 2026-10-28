// Native algorithms from the pinned Skinning.js. Source slices are metadata only.
#include "nodes/accessors/Skinning.h"
#include "nodes/accessors/StorageBufferNode.h"
#include "nodes/accessors/BufferNode.h"
#include "nodes/accessors/ReferenceNode.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/accessors/TextureSizeNode.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/accessors/Tangent.h"
#include "nodes/core/AttributeNode.h"
#include "nodes/core/IndexNode.h"
#include "nodes/utils/EventNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
#include "core/JSGlobal.h"
#include "core/InstancedBufferAttribute.h"
#include "textures/DataTexture.h"
#include "constants.h"
namespace {
using namespace UtilityNodeRuntime;
#include "SkinningSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue boneTextureMatrices(const JSValue& texture,const JSValue& imports){
    auto result=JSValue::object();auto element=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
        const auto texture=c[0],table=c[1];
        const auto intFactory=table.get("int"),sizeNode=table.get("textureSize").call({}, {texture}).get("x");
        const auto size=invoke(intFactory.call({}, {sizeNode}),"toConst")();
        const auto j=invoke(invoke(table.get("int").call({}, {arg(args,0)}),"mul")({4}),"toConst")();
        const auto y=invoke(invoke(j,"div")({size}),"toConst")();
        const auto subtract=invoke(j,"sub");const auto offset=invoke(y,"mul")({size});const auto x=invoke(subtract({offset}),"toConst")();
        const auto matrix=table.get("mat4");Args columns;
        {const auto load=invoke(texture,"load");columns.push_back(load({table.get("ivec2").call({}, {x,y})}));}
        for(int i=1;i<=3;++i){const auto load=invoke(texture,"load");const auto vec=table.get("ivec2");const auto next=invoke(x,"add")({i});columns.push_back(load({vec.call({}, {next,y})}));}
        return matrix.call({},columns);
    },{texture,imports},"element",1);element.setFunctionSourceText(SkinningSource::callback_0);own(result,"element",element);return result;
}
JSValue boneMatricesNode(const JSValue& builder,const JSValue& skeleton,const JSValue& imports){
    const auto bytes=skeleton.get("bones").get("length").toNumber()*16*4;const auto limit=invoke(builder,"getUniformBufferLimit")();
    if(bytes<=limit.toNumber()){const auto reference=imports.get("referenceBuffer");return reference.call({}, {"skeleton.boneMatrices","mat4",skeleton.get("bones").get("length")});}
    if(skeleton.get("boneTexture").isNull())invoke(skeleton,"computeBoneTexture")();
    const auto texture=imports.get("texture").call({}, {skeleton.get("boneTexture")});
    auto callback=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
        const auto object=arg(args,0).get("object"),skeleton=object.get("skeleton");if(skeleton.get("boneTexture").isNull())invoke(skeleton,"computeBoneTexture")();auto texture=c[0];texture.set("value",skeleton.get("boneTexture"));return {};
    },{texture},"",1);callback.setFunctionSourceText(SkinningSource::callback_1);imports.get("OnObjectUpdate").call({}, {callback});return boneTextureMatrices(texture,imports);
}
JSValue skinnedPosition(const JSValue& matrices,const JSValue& position,const JSValue& bind,const JSValue& inverse,const JSValue& index,const JSValue& weight,const JSValue& imports){
    Args bones;for(const auto* component:{"x","y","z","w"}){const auto element=invoke(matrices,"element");bones.push_back(element({index.get(component)}));}
    const auto vertex=invoke(bind,"mul")({position}),sum=imports.get("add");Args weighted;
    unsigned i=0;for(const auto* component:{"x","y","z","w"}){const auto multiply=invoke(bones[i++],"mul");const auto scaled=multiply({weight.get(component)});weighted.push_back(invoke(scaled,"mul")({vertex}));}
    const auto skinned=sum.call({},weighted);return invoke(inverse,"mul")({skinned}).get("xyz");
}
JSValue skinnedNormalAndTangent(const JSValue& matrices,const JSValue& normal,const JSValue& tangent,const JSValue& bind,const JSValue& inverse,const JSValue& index,const JSValue& weight,const JSValue& imports){
    Args bones;for(const auto* component:{"x","y","z","w"}){const auto element=invoke(matrices,"element");bones.push_back(element({index.get(component)}));}
    const auto sum=imports.get("add");Args weighted;unsigned i=0;for(const auto* component:{"x","y","z","w"})weighted.push_back(invoke(weight.get(component),"mul")({bones[i++]}));
    auto matrix=sum.call({},weighted);matrix=invoke(invoke(inverse,"mul")({matrix}),"mul")({bind});
    const auto matrix3=imports.get("mat3").call({}, {matrix});const auto skinNormal=invoke(matrix3,"mul")({normal}),skinTangent=invoke(matrix3,"mul")({tangent});
    auto result=JSValue::object();own(result,"skinNormal",skinNormal);own(result,"skinTangent",skinTangent);return result;
}
JSValue previousPosition(const JSValue& builder,const JSValue& mesh,const JSValue& bind,const JSValue& inverse,const JSValue& index,const JSValue& weight,const JSValue& imports){
    const auto skeleton=mesh.get("skeleton"),cache=imports.get("previous");auto data=invoke(cache,"get")({skeleton});
    if(data.isUndefined()){
        invoke(skeleton,"update")();const auto bytes=skeleton.get("bones").get("length").toNumber()*16*4;const auto limit=invoke(builder,"getUniformBufferLimit")();
        if(bytes<=limit.toNumber()){
            const auto type=nativeGlobalBinding("Float32Array"),values=type.construct({skeleton.get("boneMatrices")});data=JSValue::object();own(data,"previousBoneMatrices",values);own(data,"previousBoneTexture",nullptr);
            const auto buffer=imports.get("buffer");own(data,"node",buffer.call({}, {values,"mat4",skeleton.get("bones").get("length")}));
        }else{
            if(skeleton.get("boneTexture").isNull())invoke(skeleton,"computeBoneTexture")();
            const auto image=skeleton.get("boneTexture").get("image"),width=image.get("width"),height=image.get("height");
            const auto type=nativeGlobalBinding("Float32Array"),values=type.construct({skeleton.get("boneMatrices")});
            const auto textureClass=imports.get("DataTexture");auto previousTexture=textureClass.construct({values,width,height,imports.get("RGBAFormat"),imports.get("FloatType")});previousTexture.set("needsUpdate",true);
            data=JSValue::object();own(data,"previousBoneMatrices",values);own(data,"previousBoneTexture",previousTexture);own(data,"node",boneTextureMatrices(imports.get("texture").call({}, {previousTexture}),imports));
        }
        invoke(cache,"set")({skeleton,data});
    }
    return skinnedPosition(data.get("node"),imports.get("positionPrevious"),bind,inverse,index,weight,imports);
}
void updateSkeleton(const JSValue& skeleton,const JSValue& frameId,const JSValue& imports){
    const auto cache=imports.get("updated");if(!invoke(cache,"get")({skeleton}).strictEquals(frameId)){
        invoke(cache,"set")({skeleton,frameId});const auto data=invoke(imports.get("previous"),"get")({skeleton});
        if(!data.isUndefined()){
            const auto set=invoke(data.get("previousBoneMatrices"),"set");set({skeleton.get("boneMatrices")});
            if(!data.get("previousBoneTexture").isNull()){auto texture=data.get("previousBoneTexture");texture.set("needsUpdate",true);}
        }
        invoke(skeleton,"update")();
    }
}
JSValue deform(const JSValue&,const Args& args,const Args& c){
    const auto table=c[0];const bool compute=c[1].toBoolean();auto iterator=getJSIterator(arg(args,0));const auto first=stepJSIterator(iterator);const auto mesh=first?*first:JSValue{};auto toPosition=JSValue(nullptr);
    if(compute&&!iterator.done){const auto second=stepJSIterator(iterator);if(second&&!second->isUndefined())toPosition=*second;}if(!iterator.done)closeJSIterator(iterator);
    const auto builder=arg(args,1);JSValue position,index,weight,bind,inverse,matrices;
    if(!compute){
        const auto attribute=table.get("attribute");index=attribute.call({}, {"skinIndex","uvec4"});weight=attribute.call({}, {"skinWeight","vec4"});
        bind=table.get("reference").call({}, {"bindMatrix","mat4"});inverse=table.get("reference").call({}, {"bindMatrixInverse","mat4"});matrices=boneMatricesNode(builder,mesh.get("skeleton"),table);position=table.get("positionLocal");
        auto callback=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{const auto frame=arg(args,0),object=frame.get("object"),frameId=frame.get("frameId"),skeleton=object.get("skeleton");updateSkeleton(skeleton,frameId,c[0]);return {};},{table},"",1);callback.setFunctionSourceText(SkinningSource::callback_3);table.get("OnObjectUpdate").call({}, {callback});
    }else{
        auto input=[&](const char* name,const char* type,int size,bool integers)->JSValue{
            const auto storage=table.get("storage"),ctor=table.get("InstancedBufferAttribute");JSValue values;
            if(integers){const auto arrayType=nativeGlobalBinding("Uint32Array");values=arrayType.construct({invoke(mesh.get("geometry"),"getAttribute")({name}).get("array")});}
            else values=invoke(mesh.get("geometry"),"getAttribute")({name}).get("array");
            const auto node=storage.call({}, {ctor.construct({values,size}),type});const auto read=invoke(invoke(node,"setPBO")({true}),"toReadOnly")();
            const auto element=invoke(read,"element");return invoke(element({table.get("instanceIndex")}),"toVar")();
        };
        position=input("position","vec3",3,false);index=input("skinIndex","uvec4",4,true);weight=input("skinWeight","vec4",4,false);
        const auto uniform=table.get("uniform");bind=uniform.call({}, {mesh.get("bindMatrix"),"mat4"});inverse=uniform.call({}, {mesh.get("bindMatrixInverse"),"mat4"});
        const auto buffer=table.get("buffer"),boneMatrices=mesh.get("skeleton").get("boneMatrices"),length=mesh.get("skeleton").get("bones").get("length");matrices=buffer.call({}, {boneMatrices,"mat4",length});
        const auto skeleton=mesh.get("skeleton");auto callback=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{const auto frameId=arg(args,0).get("frameId");updateSkeleton(c[0],frameId,c[1]);return {};},{skeleton,table},"",1);callback.setFunctionSourceText(SkinningSource::callback_5);table.get("OnObjectUpdate").call({}, {callback});
    }
    if(invoke(builder,"needsPreviousData")().toBoolean()){
        const auto previous=previousPosition(builder,mesh,bind,inverse,index,weight,table);invoke(table.get("positionPrevious"),"assign")({previous});
    }
    const auto result=skinnedPosition(matrices,position,bind,inverse,index,weight,table);
    if(compute){if(!toPosition.isNull())invoke(toPosition,"assign")({result});}else invoke(table.get("positionLocal"),"assign")({result});
    if(invoke(builder,"hasGeometryAttribute")({"normal"}).toBoolean()){
        const auto values=skinnedNormalAndTangent(matrices,table.get("normalLocal"),table.get("tangentLocal"),bind,inverse,index,weight,table),normal=values.get("skinNormal"),tangent=values.get("skinTangent");
        invoke(table.get("normalLocal"),"assign")({normal});if(invoke(builder,"hasGeometryAttribute")({"tangent"}).toBoolean())invoke(table.get("tangentLocal"),"assign")({tangent});
    }
    return compute?result:JSValue{};
}
}
JSValue nativeSkinningModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Skinning module");
    const auto tsl=nativeTSLBase(),attribute=nativeAttributeNodeModule(),events=nativeEventNodeModule(),normal=nativeNormalModule(),position=nativePositionModule(),tangent=nativeTangentModule(),reference=nativeReferenceNodeModule(),buffer=nativeBufferNodeModule(),storage=nativeStorageBufferNodeModule(),texture=nativeTextureNodeModule(),size=nativeTextureSizeNodeModule(),index=nativeIndexNodeModule(),instanced=nativeInstancedBufferAttributeModule(),dataTexture=nativeDataTextureModule(),constants=nativeConstantsModule();
    auto table=JSValue::object(nullptr),bindings=JSValue::object(nullptr);for(const auto* name:{"Fn","add","uniform","int","ivec2","mat3","mat4"})own(table,name,nativeTSLBaseBinding(name));
    own(table,"attribute",nativeAttributeNodeBinding("attribute"));own(table,"OnObjectUpdate",nativeEventNodeBinding("OnObjectUpdate"));own(table,"normalLocal",nativeNormalBinding("normalLocal"));own(table,"positionLocal",nativePositionBinding("positionLocal"));own(table,"positionPrevious",nativePositionBinding("positionPrevious"));own(table,"tangentLocal",nativeTangentBinding("tangentLocal"));
    own(table,"reference",nativeReferenceNodeBinding("reference"));own(table,"referenceBuffer",nativeReferenceNodeBinding("referenceBuffer"));own(table,"buffer",nativeBufferNodeBinding("buffer"));own(table,"storage",nativeStorageBufferNodeBinding("storage"));own(table,"texture",nativeTextureNodeBinding("texture"));own(table,"textureSize",nativeTextureSizeNodeBinding("textureSize"));own(table,"instanceIndex",nativeIndexNodeBinding("instanceIndex"));own(table,"InstancedBufferAttribute",nativeInstancedBufferAttributeBinding("InstancedBufferAttribute"));own(table,"DataTexture",nativeDataTextureBinding("DataTexture"));own(table,"RGBAFormat",nativeConstantsBinding("RGBAFormat"));own(table,"FloatType",nativeConstantsBinding("FloatType"));
    own(table,"updated",nativeGlobalBinding("WeakMap").construct());own(table,"previous",nativeGlobalBinding("WeakMap").construct());
    auto standard=JSValue::function(deform,{table,false},"",2);standard.setFunctionSourceText(SkinningSource::callback_2);own(bindings,"skinning",table.get("Fn").call({}, {standard,"void"}));
    auto compute=JSValue::function(deform,{table,true},"",2);compute.setFunctionSourceText(SkinningSource::callback_4);own(bindings,"computeSkinning",table.get("Fn").call({}, {compute}));
    auto module=JSValue::object();for(const auto* name:{"computeSkinning","skinning"})own(module,name,bindings.get(name));bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeSkinningBinding(const std::string& name){const auto keeper=nativeSkinningModule();return lexical().lock()->get(name);}
