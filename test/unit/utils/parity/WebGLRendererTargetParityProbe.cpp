#include "renderers/WebGLRendererScene.h"
#include "renderers/WebGLRenderTarget.h"
#include "renderers/webgl/WebGLProperties.h"
#include "core/JSPromise.h"
#include "core/RenderTarget.h"
#include "core/EventDispatcher.h"
#include "core/JSGlobal.h"
#include "textures/DepthTexture.h"
#include "textures/Texture.h"
#include "math/Vector4.h"
#include <iostream>
#include <string>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, unsigned i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue object(std::initializer_list<std::pair<const char*, JSValue>> values = {}) {
    auto result=JSValue::object(); for(const auto& [key,value]:values) result.set(key,value); return result;
}
JSValue array(const Args& values = {}) { auto result=JSValue::array();for(const auto& value:values)result.set(JSValue(result.length()),value);return result; }
void push(JSValue target,JSValue value){target.set(JSValue(target.length()),value);}
JSValue call(JSValue self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
JSValue clean(JSValue value){
    if(value.isUndefined())return "undefined";if(value.isNull()||!value.isObject())return value;
    if(value.get("tag").type()==JSValue::Type::String)return value.get("tag");
    if(value.get("isVector4").toBoolean())return call(value,"toArray");
    if(value.type()==JSValue::Type::Array){auto result=array();for(double i=0;i<value.length();++i)push(result,clean(value.get(JSValue(i))));return result;}
    return "object";
}
JSValue property(JSValue h,JSValue value){return call(h.get("properties"),"get",{value});}
JSValue service(const JSValue&,const Args& args,const Args& captures){
    auto h=captures[0];const auto name=captures[1].string();auto row=array({name});for(const auto& value:args)push(row,clean(value));push(h.get("trace"),row);
    const auto mode=h.get("mode").toString();auto target=arg(args,0);
    if(name=="textures.setupRenderTarget"){
        auto p=property(h,target);p.set("__webglFramebuffer",h.get("framebuffer"));p.set("__webglMultisampledFramebuffer",object({{"tag","multisampled"}}));
        if(mode=="setupThrow")throw JSThrownValue("setup-stop");
        if(mode=="setupReentry"&&!h.get("reentered").toBoolean()){h.set("reentered",true);call(h.get("renderer"),"setRenderTarget",{nullptr,2,0});}
    }
    if(name=="state.bindFramebuffer"){
        if(mode=="bindThrow")throw JSThrownValue("bind-stop");
        if(mode=="bindReentry"&&!h.get("reentered").toBoolean()){h.set("reentered",true);call(h.get("renderer"),"setRenderTarget",{h.get("nested"),3,0});}
        return mode!="alreadyBound";
    }
    if(name=="state.drawBuffers"&&mode=="drawBuffersThrow")throw JSThrownValue("draw-buffers-stop");
    if(name=="textures.useMultisampledRTT")return mode=="multisampleRTT";
    return {};
}
void method(JSValue h,JSValue receiver,const char* name,const std::string& label,unsigned arity=0){receiver.set(name,JSValue::function(service,{h,label},name,arity));}
JSValue harness(const char* mode){
    auto h=object({{"mode",mode},{"trace",array()},{"reentered",false}}),renderer=object(),state=object(),textures=object(),gl=object({{"FRAMEBUFFER",36160},{"COLOR_ATTACHMENT0",36064},{"TEXTURE_CUBE_MAP_POSITIVE_X",34069},{"TEXTURE_2D",3553}});
    WebGLProperties properties;h.set("properties",properties.object());h.set("renderer",renderer);
    auto canvas=object({{"width",20},{"height",12},{"style",object()}}),xr=object({{"isPresenting",false}});
    auto bindings=object({{"_this",renderer},{"properties",properties.object()},{"state",state},{"textures",textures},{"_gl",gl},{"canvas",canvas},{"xr",xr},{"_pixelRatio",1.5},{"_scratchFramebuffer",object({{"tag","scratch"}})}});
    for(const auto* name:{"bindFramebuffer","drawBuffers","viewport","scissor","setScissorTest"})method(h,state,name,std::string("state.")+name);
    for(const auto* name:{"setupRenderTarget","rebindTextures","setupDepthRenderbuffer","useMultisampledRTT"})method(h,textures,name,std::string("textures.")+name);
    for(const auto* name:{"framebufferTexture2D","framebufferTextureLayer"})method(h,gl,name,std::string("gl.")+name);
    auto pipeline=nativeWebGLRendererScenePipeline(bindings);h.set("pipeline",pipeline);h.set("canvas",canvas);h.set("xr",xr);
    for(const auto* name:{"getActiveCubeFace","getActiveMipmapLevel","getRenderTarget","setRenderTargetTextures","setRenderTargetFramebuffer","setRenderTarget","getPixelRatio","setPixelRatio","getSize","setSize","getDrawingBufferSize","setDrawingBufferSize","getCurrentViewport","getViewport","setViewport","getScissor","setScissor","getScissorTest","setScissorTest","setOpaqueSort","setTransparentSort"})renderer.set(name,pipeline.get(name));
    auto target=nativeWebGLRenderTargetConstructor().construct({8,6});target.set("tag","target");call(target.get("viewport"),"set",{1,2,3,4});call(target.get("scissor"),"set",{2,1,4,3});target.set("scissorTest",true);h.set("target",target);
    auto nested=nativeWebGLRenderTargetConstructor().construct({4,3});nested.set("tag","nested");h.set("nested",nested);property(h,nested).set("__webglFramebuffer",object({{"tag","nested-framebuffer"}}));
    h.set("framebuffer",object({{"tag","framebuffer"}}));return h;
}
JSValue cell(JSValue h,const char* name){return h.get("pipeline").getInternalSlot("WebGLRendererScene.get").call({}, {name});}
JSValue snapshot(JSValue h){return object({{"target",clean(call(h.get("renderer"),"getRenderTarget"))},{"face",call(h.get("renderer"),"getActiveCubeFace")},{"mip",call(h.get("renderer"),"getActiveMipmapLevel")},{"viewport",clean(cell(h,"_currentViewport"))},{"scissor",clean(cell(h,"_currentScissor"))},{"scissorTest",cell(h,"_currentScissorTest")},{"material",cell(h,"_currentMaterialId")}});}
JSValue run(const char* label){
    auto h=harness(label),target=h.get("target"),renderer=h.get("renderer"),p=property(h,target);const std::string mode=label;double face=0,mip=0;
    if(mode!="fresh"&&mode!="setupThrow"&&mode!="setupReentry")p.set("__webglFramebuffer",h.get("framebuffer"));
    if(mode=="externalDefault")call(renderer,"setRenderTargetFramebuffer",{target});
    if(mode=="externalStored")call(renderer,"setRenderTargetFramebuffer",{target,object({{"tag","external-framebuffer"}})});
    if(mode=="externalTextures"){auto depth=object({{"renderTarget",nullptr}});target.set("depthTexture",depth);call(renderer,"setRenderTargetTextures",{target,object({{"tag","external-color"}}),object({{"tag","external-depth"}})});}
    if(mode=="depthSwap"||mode=="badDepth"){auto depth=object({{"renderTarget",nullptr},{"image",object({{"width",mode=="badDepth"?7:8},{"height",6}})}});target.set("depthTexture",depth);property(h,depth);}
    if(mode=="existing")p.set("__boundDepthTexture",nullptr);
    if(mode=="cube"||mode=="cubeMip"){target.set("isWebGLCubeRenderTarget",true);face=2;auto faces=array();for(unsigned i=0;i<6;++i){auto frame=object({{"tag","cube-"+std::to_string(i)}});push(faces,mode=="cubeMip"?array({frame,object({{"tag","cube-mip-"+std::to_string(i)}})}):frame);}p.set("__webglFramebuffer",faces);property(h,target.get("texture")).set("__webglTexture",object({{"tag","cube-texture"}}));if(mode=="cubeMip")mip=1;}
    if(mode=="array"||mode=="volume"||mode=="compressedArray"){target.get("texture").set(mode=="array"?"isDataArrayTexture":mode=="volume"?"isData3DTexture":"isCompressedArrayTexture",true);face=3;auto second=nativeWebGLRenderTargetConstructor().construct().get("texture");auto values=target.get("textures");values.set("1",second);for(unsigned i=0;i<2;++i)property(h,values.get(JSValue(i))).set("__webglTexture",object({{"tag","layer-"+std::to_string(i)}}));}
    if(mode=="multisample"||mode=="multisampleRTT"){target.set("samples",4);p.set("__webglMultisampledFramebuffer",object({{"tag","multisampled"}}));}
    if(mode=="mip"){mip=1;p.set("__webglFramebuffer",array({object({{"tag","mip0"}}),object({{"tag","mip1"}})}));property(h,target.get("texture")).set("__webglTexture",object({{"tag","mip-texture"}}));}
    auto error=JSValue(nullptr);try{call(renderer,"setRenderTarget",{mode=="null"?JSValue(nullptr):target,face,mip});}catch(const JSThrownValue& e){error=e.value.isObject()?e.value.get("message"):e.value;}catch(const std::exception& e){error=e.what();}
    return object({{"mode",label},{"trace",h.get("trace")},{"error",error},{"state",snapshot(h)}});
}
JSValue metadata(JSValue constructor,JSValue parent){
    auto result=object({{"name",constructor.get("name")},{"length",constructor.get("length")},{"source",constructor.nativeFunctionSource()},{"parent",constructor.getPrototype().sameValue(parent)}}),methods=object();
    const auto prototype=constructor.get("prototype");for(const auto& key:prototype.ownPropertyNames()){
        if(key=="constructor")continue;const auto d=*prototype.getOwnPropertyDescriptor(key);auto row=object({{"enumerable",d.enumerable.value_or(false)},{"configurable",d.configurable.value_or(false)}});
        if(d.value){row.set("name",d.value->get("name"));row.set("length",d.value->get("length"));row.set("source",d.value->nativeFunctionSource());row.set("writable",d.writable.value_or(false));}
        if(d.getter&&d.getter->isCallable()){row.set("getName",d.getter->get("name"));row.set("getSource",d.getter->nativeFunctionSource());}
        if(d.setter&&d.setter->isCallable()){row.set("setName",d.setter->get("name"));row.set("setSource",d.setter->nativeFunctionSource());}methods.set(key,row);
    }result.set("methods",methods);return result;
}

void getter(JSValue target,const char* name,JSValue::NativeFunction fn,const Args& captures){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(fn,captures);target.defineProperty(name,d);}
JSValue failure(){try{throw;}catch(const JSThrownValue& e){return e.value.isObject()?e.value.get("name"):e.value;}catch(const JSTypeError&){return "TypeError";}catch(const std::exception& e){return e.what();}}
JSValue targetState(JSValue target){
    auto attachments=array();const auto values=target.get("textures");for(double i=0;i<values.length();++i){const auto texture=values.get(JSValue(i)),image=texture.get("image");push(attachments,object({{"size",array({image.get("width"),image.get("height"),image.get("depth")})},{"owner",texture.get("renderTarget").sameValue(target)},{"flags",array({texture.get("isRenderTargetTexture"),texture.get("isArrayTexture"),texture.get("isData3DTexture").strictEquals(true)})},{"version",texture.get("version")},{"sourceVersion",texture.get("source").get("version")},{"options",array({texture.get("minFilter"),texture.get("magFilter"),texture.get("format"),texture.get("type"),texture.get("flipY"),texture.get("generateMipmaps"),texture.get("mipmapsAutoUpdate"),texture.get("colorSpace"),texture.get("internalFormat")})}}));}
    return object({{"size",array({target.get("width"),target.get("height"),target.get("depth")})},{"viewport",call(target.get("viewport"),"toArray")},{"scissor",call(target.get("scissor"),"toArray")},{"flags",array({target.get("scissorTest"),target.get("depthBuffer"),target.get("stencilBuffer"),target.get("resolveColorBuffer"),target.get("resolveDepthBuffer"),target.get("resolveStencilBuffer"),target.get("storeMultisampledColorBuffer"),target.get("storeMultisampledDepthBuffer"),target.get("storeMultisampledStencilBuffer"),target.get("multiview"),target.get("useArrayDepthTexture")})},{"samples",target.get("samples")},{"attachments",attachments},{"depthNull",target.get("depthTexture").isNull()},{"sharedSource",values.length()>1&&values.get("0").get("source").sameValue(values.get("1").get("source"))},{"sharedImage",values.length()>1&&values.get("0").get("image").sameValue(values.get("1").get("image"))}});
}
JSValue dynamicConstruction(JSValue constructor,const Args& fields,const char* fail){
    auto data=object({{"events",array()},{"values",object()},{"fail",fail}}),options=object({{"count",2},{"depth",3},{"samples",4}});
    auto newTarget=JSValue::constructor(+[](const JSValue& self,const Args&,const Args&){return self;},{},"Derived",0,JSValue::ConstructorKind::Callable);newTarget.set("prototype",JSValue::object(constructor.get("prototype")));
    auto parent=JSValue::builtinConstructor(+[](const JSValue& nt,const Args& args,const Args& c)->JSValue{
        auto data=c[0];push(data.get("events"),"super");data.set("newTarget",nt.sameValue(c[1]));auto argv=array();for(const auto& v:args)push(argv,clean(v));data.set("args",argv);data.set("optionsIdentity",args.size()>2&&args[2].sameValue(c[3]));
        if(data.get("fail").strictEquals("super"))throw JSThrownValue("super-stop");
        auto self=JSValue::object(c[2].get("prototype"));data.set("instance",self);
        const auto fields=c[4];for(double i=0;i<fields.length();++i){const auto name=fields.get(JSValue(i));JSPropertyDescriptor d;d.configurable=true;
            d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c){return c[0].get("values").get(c[1]);},{data,name});
            d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{auto data=c[0];push(data.get("events"),array({"set",c[1],clean(arg(a,0))}));data.get("values").set(c[1],arg(a,0));if(data.get("fail").sameValue(c[1]))throw JSThrownValue("setter-stop");return {};},{data,name});self.defineProperty(name,d);
        }return self;
    },{data,newTarget,constructor,options,array(fields)},"DynamicParent",0);
    const auto saved=constructor.getPrototype();constructor.setPrototype(parent);auto error=JSValue(nullptr);bool returned=false;
    try{returned=constructor.construct({17,23,options},newTarget).sameValue(data.get("instance"));}catch(...){error=failure();}constructor.setPrototype(saved);
    return object({{"fail",fail},{"events",data.get("events")},{"newTarget",data.get("newTarget")},{"args",data.get("args")},{"optionsIdentity",data.get("optionsIdentity")},{"error",error},{"returnsParent",returned}});
}
JSValue primitiveAllocation(JSValue constructor,bool primitive){
    auto globals=nativeGlobalObject(),intrinsic=JSValue::object().getPrototype();const auto original=globals.get("Object");const auto old=intrinsic.getOwnPropertyDescriptor("_setTextureOptions");JSPropertyDescriptor d;d.value=nativeRenderTargetConstructor().get("prototype").get("_setTextureOptions");d.writable=d.configurable=true;intrinsic.defineProperty("_setTextureOptions",d);
    auto nt=JSValue::constructor(+[](const JSValue& self,const Args&,const Args&){return self;},{},"NewTarget",0,JSValue::ConstructorKind::Callable);auto chosen=JSValue::object(nullptr);nt.set("prototype",primitive?JSValue(17):chosen);
    if(!primitive)chosen.set("_setTextureOptions",*d.value);
    auto poison=JSValue::object(nullptr);globals.set("Object",object({{"prototype",poison},{"assign",original.get("assign")},{"keys",original.get("keys")},{"defineProperty",original.get("defineProperty")}}));JSValue value;auto error=JSValue(nullptr);
    try{value=constructor.construct({7,9},nt);}catch(...){error=failure();}
    globals.set("Object",original);if(old)intrinsic.defineProperty("_setTextureOptions",*old);else intrinsic.erase("_setTextureOptions");
    auto result=object({{"primitive",primitive},{"error",error},{"intrinsic",value.isObject()&&value.getPrototype().sameValue(intrinsic)},{"chosen",value.isObject()&&value.getPrototype().sameValue(chosen)},{"poison",value.isObject()&&value.getPrototype().sameValue(poison)}});
    if(value.isObject())result.set("initial",targetState(value));return result;
}
JSValue copyGetters(JSValue constructor,const char* fail){
    auto source=constructor.construct({4,5,object({{"count",2},{"depth",2},{"samples",7}})}),destination=constructor.construct({9,10}),events=array(),enabled=object({{"value",true}});auto prototype=nativeRenderTargetConstructor().get("prototype");
    for(const auto* name:{"width","height","depth","scissor","scissorTest","viewport","textures","depthBuffer","stencilBuffer","resolveColorBuffer","resolveDepthBuffer","resolveStencilBuffer","storeMultisampledColorBuffer","storeMultisampledDepthBuffer","storeMultisampledStencilBuffer","depthTexture","samples","multiview","useArrayDepthTexture"}){
        const auto value=source.get(name);getter(source,name,+[](const JSValue&,const Args&,const Args& c)->JSValue{if(c[3].get("value").toBoolean()){push(c[0],c[1]);if(c[1].sameValue(c[4]))throw JSThrownValue("getter-stop");}return c[2];},{events,name,value,enabled,fail});
    }
    auto error=JSValue(nullptr);bool self=false;try{self=prototype.get("copy").call(destination,{source}).sameValue(destination);}catch(...){error=failure();}enabled.set("value",false);
    return object({{"fail",fail},{"events",events},{"error",error},{"returnsSelf",self},{"destination",targetState(destination)},{"source",targetState(source)}});
}
JSValue copyAlias(JSValue constructor,const char* label){
    const std::string mode=label;auto source=constructor.construct({4,5,object({{"count",2},{"depth",2}})}),destination=constructor.construct({9,10,object({{"count",3}})}),events=array();source.set("samples",7);
    if(mode=="self")destination=source;if(mode=="sharedArray")destination.set("textures",source.get("textures"));auto originalArray=destination.get("textures"),sourceArray=source.get("textures"),first=source.get("texture");const auto baseClone=first.get("clone");
    auto data=object({{"source",source},{"destination",destination},{"events",events},{"first",first},{"mode",label},{"baseClone",baseClone}});
    first.set("clone",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
        auto data=c[0],source=data.get("source"),destination=data.get("destination");const auto mode=data.get("mode").string();push(data.get("events"),array({"clone",self.sameValue(data.get("first")),source.get("textures").get("length"),destination.get("textures").get("length")}));
        if(mode=="replaceSource")source.get("textures").set("0",nativeTextureConstructor().construct({object({{"width",13},{"height",14},{"depth",15}})}));
        if(mode=="replaceDestination")destination.set("textures",array({nativeTextureConstructor().construct()}));
        if(mode=="returnAlias")return self;
        if(mode=="throw"){destination.set("width",31);source.set("samples",23);throw JSThrownValue("clone-stop");}
        return data.get("baseClone").call(self);
    },{data}));
    auto error=JSValue(nullptr);bool result=false;try{result=call(destination,"copy",{source}).sameValue(destination);}catch(...){error=failure();}
    auto aliases=array();for(double i=0;i<destination.get("textures").length();++i){const auto item=destination.get("textures").get(JSValue(i));push(aliases,object({{"first",item.sameValue(first)},{"sourceTexture",item.sameValue(source.get("textures").get(JSValue(i)))},{"sourceData",item.get("source").sameValue(first.get("source"))},{"firstImage",item.get("image").sameValue(first.get("image"))}}));}
    return object({{"mode",label},{"events",events},{"error",error},{"returnsSelf",result},{"destination",targetState(destination)},{"source",targetState(source)},{"aliases",aliases},{"originalArrayRetained",originalArray.sameValue(destination.get("textures"))},{"sourceArrayRetained",sourceArray.sameValue(source.get("textures"))},{"oldLength",originalArray.get("length")}});
}
JSValue cloneIdentity(JSValue constructor,const char* mode){
    auto source=constructor.construct({6,7,object({{"count",2},{"depth",3},{"samples",4},{"multiview",true},{"useArrayDepthTexture",true}})}),depth=nativeDepthTextureConstructor().construct({6,7});
    auto owner=object();if(std::string(mode)=="external")depth.set("renderTarget",owner);source.set("depthTexture",depth);call(source.get("viewport"),"set",{1,2,3,4});call(source.get("scissor"),"set",{2,1,4,3});
    auto events=array();call(source,"addEventListener",{"dispose",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"source disposed");return {};},{events})});const auto copy=call(source,"clone");
    const auto copiedDepth=copy.get("depthTexture"),first=source.get("texture"),other=copy.get("texture");
    auto result=object({{"mode",mode},{"prototype",copy.getPrototype().sameValue(constructor.get("prototype"))},{"constructor",copy.get("constructor").sameValue(constructor)},{"state",targetState(copy)},{"distinctTexture",!other.sameValue(first)},{"distinctSource",!other.get("source").sameValue(first.get("source"))},{"distinctImage",!other.get("image").sameValue(first.get("image"))},{"sourceOwner",first.get("renderTarget").sameValue(source)},{"distinctDepth",!copiedDepth.sameValue(depth)},{"sourceDepthOwner",depth.get("renderTarget").sameValue(source)},{"copyDepthOwner",copiedDepth.get("renderTarget").sameValue(copy)},{"externalDepthOwner",copiedDepth.get("renderTarget").sameValue(owner)}});
    call(copy,"dispose");result.set("cloneListenerCopied",events.length()!=0);call(source,"dispose");result.set("events",events);return result;
}
JSValue dynamicClone(JSValue constructor,bool fail){
    auto source=constructor.construct(),events=array(),replacement=object(),data=object({{"events",events},{"source",source},{"replacement",replacement},{"fail",fail}});
    getter(replacement,"copy",+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0].get("events"),"get copy");return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0].get("events"),array({"call copy",self.sameValue(c[0].get("replacement")),arg(a,0).sameValue(c[0].get("source"))}));if(c[0].get("fail").toBoolean())throw JSThrownValue("copy-stop");return 17;},c);},{data});
    auto custom=JSValue::builtinConstructor(+[](const JSValue& nt,const Args& a,const Args& c)->JSValue{push(c[0].get("events"),array({"construct",a.size(),nt.sameValue(c[0].get("constructor"))}));return c[0].get("replacement");},{data},"Replacement",0);data.set("constructor",custom);
    getter(source,"constructor",+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0].get("events"),"get constructor");return c[0].get("constructor");},{data});auto error=JSValue(nullptr),value=JSValue{};
    try{value=call(source,"clone");}catch(...){error=failure();}return object({{"fail",fail},{"events",events},{"error",error},{"value",clean(value)}});
}
JSValue lifecycle(JSValue constructor,const char* label){
    const std::string mode=label;auto target=constructor.construct({4,5}),first=target.get("texture"),image=first.get("image"),source=first.get("source"),events=array(),data=object({{"target",target},{"events",events},{"done",false},{"mode",label}});
    auto second=JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{auto data=c[0];push(data.get("events"),array({"second",self.sameValue(data.get("target")),arg(args,0).get("target").sameValue(self)}));return {};},{data});data.set("second",second);
    auto third=JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{push(c[0].get("events"),array({"third",self.sameValue(c[0].get("target")),arg(args,0).get("target").sameValue(self)}));return {};},{data});data.set("third",third);
    auto listener=JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
        auto data=c[0];const auto event=arg(args,0);data.set("event",event);const auto mode=data.get("mode").string();push(data.get("events"),array({"first",self.sameValue(data.get("target")),event.get("target").sameValue(self),array({self.get("width"),self.get("height"),self.get("depth")}),call(self.get("viewport"),"toArray"),call(self.get("scissor"),"toArray")}));
        if(mode=="throw")throw JSThrownValue("dispose-stop");
        if(mode=="mutateListeners"){call(self,"removeEventListener",{"dispose",data.get("second")});call(self,"addEventListener",{"dispose",data.get("third")});}
        if(mode=="reenter"&&!data.get("done").toBoolean()){data.set("done",true);call(self,"setSize",{11,12,2});}
        return {};
    },{data});call(target,"addEventListener",{"dispose",listener});call(target,"addEventListener",{"dispose",second});auto error=JSValue(nullptr);
    try{call(target,"setSize",{8,9,3});if(mode=="mutateListeners")call(target,"dispose");}catch(...){error=failure();}
    return object({{"mode",label},{"events",events},{"error",error},{"state",targetState(target)},{"eventTargetNull",data.get("event").get("target").isNull()},{"textureSame",target.get("texture").sameValue(first)},{"imageSame",target.get("texture").get("image").sameValue(image)},{"sourceSame",target.get("texture").get("source").sameValue(source)}});
}
JSValue classCases(){
    auto result=object(),construction=object(),allocation=object(),getters=object(),alias=object(),clones=object(),dynamic=object(),life=object(),defaults=object();
    const Args fields={"isRenderTarget","width","height","depth","scissor","scissorTest","viewport","textures","depthBuffer","stencilBuffer","resolveColorBuffer","resolveDepthBuffer","resolveStencilBuffer","storeMultisampledColorBuffer","storeMultisampledDepthBuffer","storeMultisampledStencilBuffer","_depthTexture","depthTexture","samples","multiview","useArrayDepthTexture"};
    for(const auto& constructor:Args{nativeRenderTargetConstructor(),nativeWebGLRenderTargetConstructor()}){
        const auto name=constructor.get("name").string();const auto names=name=="RenderTarget"?fields:Args{"isWebGLRenderTarget"};auto rows=array();push(rows,dynamicConstruction(constructor,names,""));push(rows,dynamicConstruction(constructor,names,"super"));for(const auto& field:names)push(rows,dynamicConstruction(constructor,names,field.string().c_str()));construction.set(name,rows);
        allocation.set(name,array({primitiveAllocation(constructor,true),primitiveAllocation(constructor,false)}));
        auto gs=array();for(const auto* fail:{"","width","textures","depthTexture","samples"})push(gs,copyGetters(constructor,fail));getters.set(name,gs);
        auto as=array();for(const auto* mode:{"normal","self","sharedArray","replaceSource","replaceDestination","returnAlias","throw"})push(as,copyAlias(constructor,mode));alias.set(name,as);
        clones.set(name,array({cloneIdentity(constructor,"owned"),cloneIdentity(constructor,"external")}));dynamic.set(name,array({dynamicClone(constructor,false),dynamicClone(constructor,true)}));
        auto ls=array();for(const auto* mode:{"normal","throw","mutateListeners","reenter"})push(ls,lifecycle(constructor,mode));life.set(name,ls);
        defaults.set(name,array({targetState(constructor.construct()),targetState(constructor.construct({JSValue{},7,object({{"count",2},{"depth",3},{"samples",4},{"stencilBuffer",true},{"resolveColorBuffer",false},{"resolveDepthBuffer",false},{"resolveStencilBuffer",false},{"storeMultisampledColorBuffer",false},{"storeMultisampledDepthBuffer",false},{"storeMultisampledStencilBuffer",false},{"multiview",true},{"useArrayDepthTexture",true}})}))}));
    }
    result.set("construction",construction);result.set("allocation",allocation);result.set("copyGetters",getters);result.set("copyAlias",alias);result.set("clones",clones);result.set("dynamicClone",dynamic);result.set("lifecycle",life);result.set("defaults",defaults);return result;
}

JSValue sizing(){
    auto h=harness("sizing"),r=h.get("renderer"),trace=h.get("trace"),v=nativeVector4Constructor().construct();
    call(r,"setViewport",{1.2,2.4,6.3,4.9});call(r,"setScissor",{nativeVector4Constructor().construct({0.4,1.1,3.2,2.5})});call(r,"setScissorTest",{1});
    const auto first=snapshot(h);call(r,"setRenderTarget",{nullptr});const auto screen=snapshot(h);call(r,"setRenderTarget",{h.get("target")});
    call(r,"setViewport",{1.2,2.4,6.3,4.9});const auto target=snapshot(h);call(r,"setPixelRatio",{2});call(r,"setSize",{9.5,7.25,false});call(r,"setDrawingBufferSize",{11.25,5.5,1.25});
    auto viewport=call(r,"getViewport",{v});auto view=clean(viewport);auto scissor=clean(call(r,"getScissor",{v}));
    return object({{"trace",trace},{"first",first},{"screen",screen},{"target",target},{"final",snapshot(h)},{"viewport",view},{"scissor",scissor},{"scissorTest",call(r,"getScissorTest")},{"pixelRatio",call(r,"getPixelRatio")},{"canvas",object({{"width",h.get("canvas").get("width")},{"height",h.get("canvas").get("height")},{"style",h.get("canvas").get("style")}})}});
}
}
int main(){try{auto result=object(),rows=array();for(const auto* mode:{"fresh","null","existing","externalDefault","externalStored","externalTextures","depthSwap","badDepth","cube","cubeMip","array","volume","compressedArray","multisample","multisampleRTT","mip","setupThrow","setupReentry","bindThrow","bindReentry","drawBuffersThrow","alreadyBound"})push(rows,run(mode));result.set("switching",rows);result.set("sizing",sizing());result.set("metadata",object({{"RenderTarget",metadata(nativeRenderTargetConstructor(),EventDispatcher::runtimeConstructor())},{"WebGLRenderTarget",metadata(nativeWebGLRenderTargetConstructor(),nativeRenderTargetConstructor())}}));result.set("classes",classCases());std::cout<<result.toJSON()->dump()<<'\n';return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
