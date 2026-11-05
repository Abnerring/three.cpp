#pragma once
#include "renderers/webgl/WebGLTextures.h"
#include "renderers/webgl/WebGLProperties.h"
#include "textures/Texture.h"
#include "textures/DataTexture.h"
#include "textures/DataArrayTexture.h"
#include "textures/CubeTexture.h"
#include "textures/CompressedTexture.h"
#include "textures/DepthTexture.h"
#include "core/RenderTarget.h"
#include "core/JSGlobal.h"
#include "core/NativeArray.h"
#include "core/JSSet.h"
#include "core/JSAsyncFunction.h"
#include "constants.h"
#include <cmath>

namespace WebGLTexturesFixtures {
using Args=std::vector<JSValue>;
inline JSValue object(){return JSValue::object();}
inline JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
inline void push(JSValue a,const JSValue& v){a.set(JSValue(a.length()),v);}
inline JSValue list(const Args& a={}){auto out=JSValue::array();for(const auto& v:a)push(out,v);return out;}
inline JSValue call(const JSValue& self,const char* key,const Args& a={}){return self.get(key).call(self,a);}
inline double identity(const JSValue& a,const JSValue& v){for(double i=0;i<a.length();++i)if(a.get(JSValue(i)).sameValue(v))return i;return -1;}
inline JSValue encode(const JSValue& c,const JSValue& v){
    if(v.isUndefined()){auto r=object();r.set("$undefined",true);return r;}
    if(v.type()==JSValue::Type::Number&&!std::isfinite(v.number())){auto r=object();r.set("$number",std::isnan(v.number())?"NaN":v.number()<0?"-Infinity":"Infinity");return r;}
    if(!v.isObject())return v;
    for(const auto& [key,values]:std::initializer_list<std::pair<const char*,const char*>>{{"texture","textures"},{"source","sources"},{"target","targets"},{"handle","handles"},{"image","images"}}){const auto index=identity(c.get(values),v);if(index>=0){auto r=object();r.set(key,index);return r;}}
    if(v.isHostObject("three.AnimationArray")){auto r=object();r.set("$typed",v.get("constructor").get("name"));auto values=list();for(double i=0;i<v.get("length").toNumber();++i)push(values,v.get(JSValue(i)));r.set("values",values);return r;}
    auto r=v.type()==JSValue::Type::Array?list():object();for(const auto& key:v.keys())if(!v.get(key).isCallable())r.set(key,encode(c,v.get(key)));return r;
}
inline JSValue command(const JSValue&,const Args& args,const Args& captures){
    auto c=captures[0];const auto name=captures[1].toString();auto event=list({name});for(const auto& a:args)push(event,encode(c,a));push(c.get("events"),event);
    if(c.get("request").get("variant").strictEquals("glThrow")&&!c.get("thrown").toBoolean()&&name=="state.texSubImage2D"){c.set("thrown",true);throw JSThrownValue("gpu-command");}
    if(name.starts_with("gl.create")){auto h=object();h.set("resource",name.substr(9));h.set("ordinal",c.get("handles").length());push(c.get("handles"),h);return h;}
    if(name=="state.pixelStorei"){auto unpack=c.get("unpack");unpack.set(arg(args,0),arg(args,1));return {};}
    if(name=="state.getParameter"){const auto v=c.get("unpack").get(arg(args,0));return v.isUndefined()?JSValue(0):v;}
    if(name=="state.bindFramebuffer")return true;
    return {};
}
inline JSValue callback(const JSValue& c,const std::string& name){return JSValue::function(command,{c,name});}
inline JSValue makeContext(const JSValue& request,const JSValue& enums){
    auto c=object();c.set("request",request);for(const auto* key:{"events","textures","sources","images","targets","handles"})c.set(key,list());c.set("unpack",object());c.set("thrown",false);
    c.set("properties",WebGLProperties().object());auto info=object(),memory=object(),render=object();memory.set("textures",0);render.set("frame",0);info.set("memory",memory);info.set("render",render);c.set("info",info);
    auto gl=object();for(const auto& key:enums.keys())gl.set(key,enums.get(key));auto canvas=object();canvas.set("width",8);canvas.set("height",8);gl.set("canvas",canvas);
    for(const auto* name:{"createTexture","deleteTexture","createFramebuffer","deleteFramebuffer","createRenderbuffer","deleteRenderbuffer","bindRenderbuffer","framebufferRenderbuffer","framebufferTexture2D","renderbufferStorage","renderbufferStorageMultisample","generateMipmap","blitFramebuffer","invalidateFramebuffer","texParameterf","texParameteri","texElementImage2D"})gl.set(name,callback(c,std::string("gl.")+name));
    c.set("gl",gl);auto state=object();for(const auto* name:{"activeTexture","bindTexture","unbindTexture","bindFramebuffer","pixelStorei","getParameter","texImage2D","texImage3D","texStorage2D","texStorage3D","texSubImage2D","texSubImage3D","compressedTexImage2D","compressedTexImage3D","compressedTexSubImage2D","compressedTexSubImage3D"})state.set(name,callback(c,std::string("state.")+name));c.set("state",state);
    auto extensions=object();for(const auto* key:{"has","get"})extensions.set(key,JSValue::function(+[](const JSValue&,const Args& a,const Args& cap)->JSValue{push(cap[0].get("events"),list({"extensions."+cap[1].toString(),arg(a,0)}));return cap[1].strictEquals("has")?JSValue(false):JSValue(nullptr);},{c,key}));c.set("extensions",extensions);
    auto caps=object();caps.set("maxTextures",request.get("maxTextures").isUndefined()?JSValue(16):request.get("maxTextures"));caps.set("maxTextureSize",1024);caps.set("maxCubemapSize",1024);caps.set("maxSamples",4);caps.set("getMaxAnisotropy",JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{push(cap[0].get("events"),list({"maxAnisotropy"}));return 16;},{c}));c.set("caps",caps);
    auto utils=object();utils.set("convert",JSValue::function(+[](const JSValue&,const Args& a,const Args& cap)->JSValue{
        const auto value=arg(a,0);push(cap[0].get("events"),list({"convert",encode(cap[0],value)}));
        for(const auto& [key,gl]:std::initializer_list<std::pair<const char*,const char*>>{{"UnsignedByteType","UNSIGNED_BYTE"},{"UnsignedShortType","UNSIGNED_SHORT"},{"UnsignedIntType","UNSIGNED_INT"},{"FloatType","FLOAT"},{"HalfFloatType","HALF_FLOAT"},{"ByteType","BYTE"},{"ShortType","SHORT"},{"IntType","INT"},{"RGBAFormat","RGBA"},{"RGBFormat","RGB"},{"RedFormat","RED"},{"RGFormat","RG"},{"RedIntegerFormat","RED_INTEGER"},{"RGIntegerFormat","RG_INTEGER"},{"RGBAIntegerFormat","RGBA_INTEGER"},{"DepthFormat","DEPTH_COMPONENT"},{"DepthStencilFormat","DEPTH_STENCIL"},{"UnsignedInt248Type","DEPTH24_STENCIL8"}})if(nativeConstantsBinding(key).strictEquals(value))return cap[1].get(gl);
        if(value.strictEquals(RGB_S3TC_DXT1_Format))return 33776;return value;
    },{c,enums}));c.set("utils",utils);
    c.set("manager",nativeWebGLTexturesConstructor().construct({gl,extensions,state,c.get("properties"),caps,utils,info}));return c;
}
inline JSValue enroll(const JSValue& c,const JSValue& texture){
    if(identity(c.get("textures"),texture)<0)push(c.get("textures"),texture);const auto source=texture.get("source");if(identity(c.get("sources"),source)<0)push(c.get("sources"),source);const auto image=texture.get("image");if(image.isObject()&&identity(c.get("images"),image)<0)push(c.get("images"),image);return texture;
}
inline JSValue bytes(double count,bool sequence=true){auto value=nativeGlobalBinding("Uint8Array").construct({count});if(sequence)for(double i=0;i<count;++i)value.set(JSValue(i),i);return value;}
inline JSValue makeTexture(const JSValue& c,const std::string& name){
    const auto data=bytes(64);JSValue t;
    if(name=="data")t=nativeDataTextureConstructor().construct({data,4,4});
    else if(name=="array")t=nativeDataArrayTextureConstructor().construct({data,2,2,4});
    else if(name=="depth")t=nativeDepthTextureConstructor().construct({4,4});
    else if(name=="compressed"||name=="compressedArray"){
        auto a=object(),b=object();a.set("width",4);a.set("height",4);a.set("data",data);b.set("width",2);b.set("height",2);b.set("data",bytes(8,false));t=nativeCompressedTextureConstructor().construct({list({a,b}),4,4,RGB_S3TC_DXT1_Format});
        if(name=="compressedArray"){t.set("isCompressedArrayTexture",true);auto image=t.get("image");image.set("depth",2);t.set("wrapR",ClampToEdgeWrapping);t.set("layerUpdates",nativeSetConstructor().construct());t.set("clearLayerUpdates",JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{call(self.get("layerUpdates"),"clear");return {};}));}
    }else if(name=="cube"||name=="cubeData"){
        auto faces=list();for(int i=0;i<6;++i){JSValue face;if(name=="cubeData")face=nativeDataTextureConstructor().construct({data,4,4});else{face=object();face.set("width",4);face.set("height",4);face.set("data",data);}push(faces,face);if(name=="cubeData")enroll(c,face);else push(c.get("images"),face);}t=nativeCubeTextureConstructor().construct({faces});
    }else{
        auto image=object();image.set("width",4);image.set("height",4);image.set("data",data);t=nativeTextureConstructor().construct({image});
        if(name=="data3d"){t.set("isData3DTexture",true);image.set("depth",4);t.set("wrapR",ClampToEdgeWrapping);t.set("magFilter",NearestFilter);t.set("minFilter",NearestFilter);t.set("generateMipmaps",false);t.set("flipY",false);t.set("unpackAlignment",1);}
        if(name=="video"){t.set("isVideoTexture",true);t.set("generateMipmaps",false);t.set("update",JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{push(cap[0].get("events"),list({"video.update"}));return {};},{c}));}
        if(name=="external"){t.set("isExternalTexture",true);auto resource=object();resource.set("external",true);t.set("sourceTexture",resource);push(c.get("handles"),resource);}
    }
    enroll(c,t);t.set("needsUpdate",true);return t;
}
inline JSValue snapshot(const JSValue& c){
    auto out=object(),properties=list(),sources=list(),ranges=list(),versions=list();const auto manager=c.get("properties");
    for(double i=0;i<c.get("textures").length();++i){const auto t=c.get("textures").get(JSValue(i));push(properties,encode(c,call(manager,"get",{t})));push(ranges,encode(c,t.get("updateRanges")));push(versions,list({t.get("version"),t.get("source").get("version"),t.get("source").get("dataReady")}));}
    for(double i=0;i<c.get("sources").length();++i)push(sources,encode(c,call(manager,"get",{c.get("sources").get(JSValue(i))})));
    out.set("properties",properties);out.set("sources",sources);out.set("memory",encode(c,c.get("info").get("memory")));out.set("ranges",ranges);out.set("versions",versions);out.set("units",call(c.get("manager"),"getTextureUnits"));return out;
}
inline JSValue currentError(){const auto value=nativeExceptionValue(std::current_exception());auto out=object();if(value.isObject()){out.set("name",value.get("name"));}else out.set("raw",value);return out;}
template<class Function> JSValue safe(const JSValue& c,Function&& function){auto row=object();try{row.set("value",encode(c,function()));}catch(...){row.set("error",currentError());}row.set("state",snapshot(c));return row;}
inline JSValue upload(const JSValue& c,const JSValue& t,JSValue slot=1){const char* name=t.get("isCubeTexture").toBoolean()?"setTextureCube":t.get("isDataArrayTexture").toBoolean()||t.get("isCompressedArrayTexture").toBoolean()?"setTexture2DArray":t.get("isData3DTexture").toBoolean()?"setTexture3D":"setTexture2D";return call(c.get("manager"),name,{t,slot});}
inline JSValue run(const JSValue& request,const JSValue& enums){
    if(request.get("kind").strictEquals("metadata")){auto out=object();const auto fn=nativeWebGLTexturesConstructor();out.set("name",fn.get("name"));out.set("length",fn.get("length"));out.set("source",call(fn,"toString"));return out;}
    const auto c=makeContext(request,enums),manager=c.get("manager"),variant=request.get("variant");auto rows=list();
    if(request.get("kind").strictEquals("units")){for(int i=0;i<4;++i)push(rows,call(manager,"allocateTextureUnit"));call(manager,"setTextureUnits",{"3"});push(rows,call(manager,"allocateTextureUnit"));call(manager,"resetTextureUnits");push(rows,call(manager,"getTextureUnits"));}
    else if(request.get("kind").strictEquals("upload")){
        auto t=makeTexture(c,request.get("class").toString());if(variant.strictEquals("notReady")){auto source=t.get("source");source.set("dataReady",false);}
        if(variant.strictEquals("layers")){call(t.get("layerUpdates"),"add",{0});call(t.get("layerUpdates"),"add",{1});}if(variant.strictEquals("halfFloat"))t.set("type",HalfFloatType);if(variant.strictEquals("srgb"))t.set("colorSpace",SRGBColorSpace);
        if(variant.strictEquals("ranges"))for(const auto& [start,count]:std::initializer_list<std::pair<int,int>>{{12,4},{0,4},{3,5},{32,8},{24,8},{16,8}})call(t,"addUpdateRange",{start,count});
        auto once=object();once.set("once",false);t.set("onUpdate",JSValue::function(+[](const JSValue& self,const Args& a,const Args& cap)->JSValue{
            push(cap[0].get("events"),list({"onUpdate",arg(a,0).sameValue(cap[1]),self.sameValue(cap[1])}));auto state=cap[3];const auto variant=cap[2];
            if(variant.strictEquals("onUpdateThrow")&&!state.get("once").toBoolean()){state.set("once",true);throw JSThrownValue("onUpdate");}
            if(variant.strictEquals("onUpdateReenter")&&!state.get("once").toBoolean()){state.set("once",true);auto texture=cap[1],source=texture.get("source");source.set("needsUpdate",true);texture.set("needsUpdate",true);upload(cap[0],texture,2);}return {};
        },{c,t,variant,once}));
        push(rows,safe(c,[&]{return upload(c,t);}));push(rows,safe(c,[&]{return upload(c,t);}));
        if(variant.strictEquals("sourceOnly")){auto source=t.get("source");source.set("needsUpdate",true);}if(variant.strictEquals("textureOnly"))t.set("version",t.get("version").toNumber()+1);
        if(variant.strictEquals("notReady")){auto source=t.get("source");source.set("dataReady",true);t.set("needsUpdate",true);}if(variant.strictEquals("samplerChange")){t.set("wrapS",RepeatWrapping);t.set("needsUpdate",true);}if(variant.strictEquals("dispose"))call(t,"dispose");
        push(rows,safe(c,[&]{return upload(c,t);}));auto render=c.get("info").get("render");render.set("frame",render.get("frame").toNumber()+1);push(rows,safe(c,[&]{return upload(c,t);}));push(rows,safe(c,[&]{return call(t,"dispose");}));push(rows,safe(c,[&]{return call(manager,"dispose");}));
    }else if(request.get("kind").strictEquals("shared")){
        auto a=makeTexture(c,"data"),b=makeTexture(c,"data");if(request.get("shared").toBoolean())b.set("source",a.get("source"));if(variant.strictEquals("cacheDiff"))b.set("wrapS",RepeatWrapping);auto once=object();once.set("once",false);
        a.set("onUpdate",JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{push(cap[0].get("events"),list({"a.onUpdate"}));auto state=cap[3];if(cap[2].strictEquals("uploadThrow")&&!state.get("once").toBoolean()){state.set("once",true);throw JSThrownValue("first");}if(cap[2].strictEquals("onUpdateDispose"))call(cap[1],"dispose");return {};},{c,a,variant,once}));
        push(rows,safe(c,[&]{return upload(c,a);}));push(rows,safe(c,[&]{return upload(c,b);}));if(variant.strictEquals("sourceChange")){b.set("source",makeTexture(c,"data").get("source"));b.set("needsUpdate",true);}if(variant.strictEquals("disposeFirst"))call(a,"dispose");if(variant.strictEquals("disposeSecond"))call(b,"dispose");push(rows,safe(c,[&]{return upload(c,b);}));push(rows,safe(c,[&]{return upload(c,a);}));push(rows,safe(c,[&]{return call(manager,"dispose");}));
    }else if(request.get("kind").strictEquals("target")){
        auto options=object();options.set("count",request.get("count"));options.set("depthBuffer",!request.get("depth").strictEquals("none"));options.set("stencilBuffer",request.get("stencil").isUndefined()?JSValue(false):request.get("stencil"));options.set("samples",request.get("samples"));options.set("generateMipmaps",request.get("mipmaps").isUndefined()?JSValue(false):request.get("mipmaps"));auto target=nativeRenderTargetConstructor().construct({4,4,options});
        if(request.get("cube").toBoolean()){target.set("isWebGLCubeRenderTarget",true);auto images=list();for(int i=0;i<6;++i){auto image=object();image.set("width",4);image.set("height",4);push(images,image);}auto texture=nativeCubeTextureConstructor().construct({images});texture.set("isRenderTargetTexture",true);texture.set("generateMipmaps",false);target.set("texture",texture);}
        if(request.get("depth").strictEquals("texture")){auto depth=nativeDepthTextureConstructor().construct({4,4});if(request.get("cube").toBoolean()){depth.set("isCubeTexture",true);depth.set("isCubeDepthTexture",true);depth.set("mapping",CubeReflectionMapping);auto image=object();image.set("width",4);image.set("height",4);image.set("depth",1);depth.set("image",list({image,image,image,image,image,image}));}target.set("depthTexture",depth);}push(c.get("targets"),target);for(double i=0;i<target.get("textures").length();++i)enroll(c,target.get("textures").get(JSValue(i)));if(target.get("depthTexture").toBoolean())enroll(c,target.get("depthTexture"));
        push(rows,safe(c,[&]{return call(manager,"setupRenderTarget",{target});}));if(request.get("rebind").toBoolean())push(rows,safe(c,[&]{return call(manager,"rebindTextures",{target,target.get("texture"),target.get("depthTexture")});}));push(rows,safe(c,[&]{return call(manager,"updateRenderTargetMipmap",{target});}));push(rows,safe(c,[&]{return call(manager,"updateMultisampleRenderTarget",{target});}));push(rows,safe(c,[&]{return call(manager,"setupDepthRenderbuffer",{target});}));push(rows,safe(c,[&]{return call(target,"dispose");}));push(rows,safe(c,[&]{return call(manager,"dispose");}));
    }
    auto out=object();out.set("rows",rows);out.set("events",c.get("events"));return out;
}
}
