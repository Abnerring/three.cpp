// Native port of pinned three.js src/nodes/accessors/Batch.js.
#include "nodes/accessors/Batch.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/accessors/Tangent.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/accessors/TextureSizeNode.h"
#include "nodes/core/IndexNode.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/utils/EventNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
#include "textures/DataTexture.h"
#include "core/JSGlobal.h"

namespace {
using namespace UtilityNodeRuntime;
#include "BatchSources.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
Args pairInputs(const JSValue& value){
    auto iterator=getJSIterator(value);Args result;
    for(unsigned i=0;i<2;++i){const auto next=iterator.done?std::optional<JSValue>{}:stepJSIterator(iterator);result.push_back(next?*next:JSValue{});}
    if(!iterator.done)closeJSIterator(iterator);return result;
}
JSValue textureDimensions(const JSValue& value,const JSValue& table){
    const auto convert=table.get("int"),size=table.get("textureSize"),load=table.get("textureLoad");
    return invoke(convert.call({}, {size.call({}, {load.call({}, {value}),0}).get("x")}),"toConst")();
}
JSValue colorOrIndirect(const JSValue&,const Args& args,const Args& captures){
    const auto inputs=pairInputs(arg(args,0));const auto table=captures[0],value=inputs[0],id=inputs[1];
    const auto size=textureDimensions(value,table);JSValue x,y;
    if(captures[1].toBoolean()){
        const auto j=table.get("int").call({}, {id});x=invoke(invoke(j,"mod")({size}),"toConst")();y=invoke(invoke(j,"div")({size}),"toConst")();
    }else{
        const auto j=table.get("int").call({}, {id});x=invoke(invoke(j,"mod")({size}),"toConst")();
        const auto k=table.get("int").call({}, {id});y=invoke(invoke(k,"div")({size}),"toConst")();
    }
    const auto load=table.get("textureLoad"),coordinates=table.get("ivec2").call({}, {x,y});const auto result=load.call({}, {value,coordinates});
    return captures[1].toBoolean()?result:result.get("x");
}
JSValue createMatrix(const JSValue&,const Args& args,const Args& captures){
    const auto table=captures[0],texture=arg(args,0),id=arg(args,1),size=textureDimensions(texture,table);
    const auto j=invoke(invoke(invoke(table.get("float").call({}, {id}),"mul")({4}),"toInt")(),"toConst")();
    const auto x=invoke(invoke(j,"mod")({size}),"toConst")(),y=invoke(invoke(j,"div")({size}),"toConst")();
    const auto matrix=table.get("mat4");Args columns;
    for(unsigned i=0;i<4;++i){const auto load=table.get("textureLoad"),coordinateFactory=table.get("ivec2");const auto column=i?invoke(x,"add")({i}):x;columns.push_back(load.call({}, {texture,coordinateFactory.call({}, {column,y})}));}
    return matrix.call({},columns);
}
JSValue previousMatrix(const JSValue&,const Args& args,const Args& captures){
    const auto table=captures[0],mesh=arg(args,0),id=arg(args,1),cache=table.get("previous");auto data=invoke(cache,"get")({mesh});
    if(data.isUndefined()){
        const auto source=mesh.get("_matricesTexture"),image=source.get("image"),format=source.get("format"),type=source.get("type");
        const auto constructor=table.get("DataTexture"),values=invoke(image.get("data"),"slice")(),width=image.get("width"),height=image.get("height");
        const auto texture=constructor.construct({values,width,height,format,type});data=JSValue::object();own(data,"previousMatricesTexture",texture);own(data,"node",table.get("createMatrix").call({}, {texture,id}));invoke(cache,"set")({mesh,data});
    }
    return data.get("node");
}
JSValue batchBody(const JSValue&,const Args& args,const Args& captures){
    auto iterator=getJSIterator(arg(args,0));const auto input=stepJSIterator(iterator);const auto mesh=input?*input:JSValue{};if(!iterator.done)closeJSIterator(iterator);
    const auto builder=arg(args,1),table=captures[0];const auto id=invoke(builder,"getDrawIndex")().isNull()?table.get("instanceIndex"):table.get("drawIndex");
    const auto indirectFactory=table.get("getIndirectIndex"),texture=mesh.get("_indirectTexture"),converted=table.get("int").call({}, {id});const auto indirect=indirectFactory.call({}, {texture,converted});
    invoke(table.get("batchIndirectIndex"),"assign")({indirect});
    const auto create=table.get("createMatrix"),matrices=mesh.get("_matricesTexture");const auto matrix=create.call({}, {matrices,indirect});
    const auto colors=mesh.get("_colorsTexture");if(!colors.isNull()){const auto color=table.get("getBatchingColor").call({}, {colors,indirect});invoke(table.get("batchColor"),"assign")({color});}
    const auto bm=table.get("mat3").call({}, {matrix});
    const auto assignPosition=invoke(table.get("positionLocal"),"assign");assignPosition({invoke(matrix,"mul")({table.get("positionLocal")})});
    if(invoke(builder,"needsPreviousData")().toBoolean()){
        auto callback=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{
            const auto object=arg(args,0).get("object"),data=invoke(captures[0].get("previous"),"get")({object});
            const auto set=invoke(data.get("previousMatricesTexture").get("image").get("data"),"set");set({object.get("_matricesTexture").get("image").get("data")});
            auto texture=data.get("previousMatricesTexture");texture.set("needsUpdate",true);return {};
        },{table},"",1);callback.setFunctionSourceText(BatchSource::callback_3);table.get("OnAfterObjectUpdate").call({}, {callback});
        const auto previous=table.get("getPrevious").call({}, {mesh,indirect});const auto assign=invoke(table.get("positionPrevious"),"assign");assign({invoke(previous,"mul")({table.get("positionPrevious")}).get("xyz")});
    }
    const auto divide=invoke(table.get("normalLocal"),"div");const auto vector=table.get("vec3");Args squared;
    for(unsigned i=0;i<3;++i){const auto dot=invoke(bm.get(JSValue(i)),"dot");squared.push_back(dot({bm.get(JSValue(i))}));}
    const auto transformed=divide({vector.call({},squared)}),normal=invoke(bm,"mul")({transformed}).get("xyz");invoke(table.get("normalLocal"),"assign")({normal});
    if(invoke(builder,"hasGeometryAttribute")({"tangent"}).toBoolean())invoke(table.get("tangentLocal"),"mulAssign")({bm});
    auto callback=JSValue::function(+[](const JSValue&,const Args& args,const Args&)->JSValue{
        const auto frame=arg(args,0),object=frame.get("object"),camera=frame.get("camera"),material=frame.get("material");const auto update=invoke(object,"_updateDrawParameters");update({camera,object.get("geometry"),material});return {};
    },{},"",1);callback.setFunctionSourceText(BatchSource::callback_4);table.get("OnObjectUpdate").call({}, {callback});return {};
}
}
JSValue nativeBatchModule(){
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Batch module");
    const auto normal=nativeNormalModule(),position=nativePositionModule(),tsl=nativeTSLBase(),texture=nativeTextureNodeModule(),size=nativeTextureSizeNodeModule(),tangent=nativeTangentModule(),index=nativeIndexNodeModule(),property=nativePropertyNodeModule(),event=nativeEventNodeModule(),data=nativeDataTextureModule();
    auto table=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
    for(const auto* name:{"vec3","mat3","mat4","int","ivec2","float","Fn"})own(table,name,nativeTSLBaseBinding(name));
    own(table,"normalLocal",nativeNormalBinding("normalLocal"));for(const auto* name:{"positionLocal","positionPrevious"})own(table,name,nativePositionBinding(name));
    own(table,"textureLoad",nativeTextureNodeBinding("textureLoad"));own(table,"textureSize",nativeTextureSizeNodeBinding("textureSize"));own(table,"tangentLocal",nativeTangentBinding("tangentLocal"));
    for(const auto* name:{"instanceIndex","drawIndex"})own(table,name,nativeIndexNodeBinding(name));for(const auto* name:{"OnObjectUpdate","OnAfterObjectUpdate"})own(table,name,nativeEventNodeBinding(name));
    own(table,"DataTexture",nativeDataTextureBinding("DataTexture"));own(table,"previous",nativeGlobalBinding("WeakMap").construct());
    const auto fn=table.get("Fn");for(unsigned i=0;i<2;++i){auto callback=JSValue::function(colorOrIndirect,{table,i==0},"",1);callback.setFunctionSourceText(i==0?BatchSource::callback_0:BatchSource::callback_1);own(table,i==0?"getBatchingColor":"getIndirectIndex",fn.call({}, {callback}));}
    auto create=JSValue::constructor(createMatrix,{table},"createBatchingMatrixNode",2,JSValue::ConstructorKind::Callable);create.setFunctionSourceText(BatchSource::createBatchingMatrixNode);own(table,"createMatrix",create);
    auto previous=JSValue::constructor(previousMatrix,{table},"getPreviousNode",2,JSValue::ConstructorKind::Callable);previous.setFunctionSourceText(BatchSource::getPreviousNode);own(table,"getPrevious",previous);
    const auto varying=nativePropertyNodeBinding("varyingProperty");const auto color=varying.call({}, {"vec4","vBatchColor"}),indirect=varying.call({}, {"uint","vBatchIndirectId"});
    own(table,"batchColor",color);own(table,"batchIndirectIndex",indirect);own(bindings,"batchColor",color);own(bindings,"batchIndirectIndex",indirect);
    auto callback=JSValue::function(batchBody,{table},"",2);callback.setFunctionSourceText(BatchSource::callback_2);own(bindings,"batch",fn.call({}, {callback,"void"}));
    auto module=JSValue::object();for(const auto* name:{"batch","batchColor","batchIndirectIndex"})own(module,name,bindings.get(name));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeBatchBinding(const std::string& name){const auto keeper=nativeBatchModule();return lexical().lock()->get(name);}
