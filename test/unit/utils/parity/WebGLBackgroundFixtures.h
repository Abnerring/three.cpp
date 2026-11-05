#pragma once
#include "renderers/webgl/WebGLBackground.h"
#include "textures/Texture.h"
#include "textures/CubeTexture.h"
#include "math/Color.h"
#include "math/ColorManagement.h"
#include "math/Euler.h"
#include "math/Matrix4.h"
#include "core/JSAsyncFunction.h"
#include <cmath>

namespace WebGLBackgroundFixtures {
using Args=std::vector<JSValue>;
inline JSValue object(){return JSValue::object();}
inline JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
inline void push(JSValue array,const JSValue& value){array.set(JSValue(array.length()),value);}
inline JSValue list(const Args& a={}){auto out=JSValue::array();for(const auto& v:a)push(out,v);return out;}
inline JSValue call(const JSValue& self,const char* key,const Args& args={}){return self.get(key).call(self,args);}
inline void getter(JSValue target,const char* key,JSValue fn){JSPropertyDescriptor d;d.getter=fn;d.configurable=true;target.defineProperty(key,d);}
inline JSValue clean(const JSValue& v){
    if(v.isUndefined()){auto out=object();out.set("$undefined",true);return out;}
    if(v.type()==JSValue::Type::Number&&(!std::isfinite(v.number())||(v.number()==0&&std::signbit(v.number())))){auto out=object();out.set("$number",std::isnan(v.number())?"NaN":v.number()==0?"-0":v.number()<0?"-Infinity":"Infinity");return out;}
    if(!v.isObject())return v;auto out=v.type()==JSValue::Type::Array?list():object();for(const auto& key:v.keys())out.set(key,clean(v.get(key)));return out;
}
inline JSValue error(){const auto value=nativeExceptionValue(std::current_exception());auto out=object();if(value.isObject())out.set("name",value.get("name"));else out.set("raw",value);return out;}
inline JSValue arrayOf(const JSValue& value){auto out=list();for(double i=0;i<value.get("length").toNumber();++i)push(out,value.get(JSValue(i)));return out;}
inline JSValue fnMetadata(const JSValue& fn){auto out=object();out.set("name",fn.get("name"));out.set("length",fn.get("length"));out.set("source",call(fn,"toString"));out.set("constructible",fn.hasOwn("prototype"));return out;}
inline JSValue metadata(){
    const auto ctor=nativeWebGLBackgroundConstructor(),value=ctor.construct();auto out=object(),keys=list(),methods=list();out.set("factory",fnMetadata(ctor));
    for(const auto& key:value.keys()){push(keys,key);auto row=fnMetadata(value.get(key));row.set("key",key);const auto d=value.getOwnPropertyDescriptor(key);row.set("flags",list({d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)}));push(methods,row);}
    out.set("keys",keys);out.set("methods",methods);return out;
}
inline JSValue texture(const JSValue& settings){
    auto value=settings.get("kind").strictEquals("cube")?nativeCubeTextureConstructor().construct():nativeTextureConstructor().construct();
    value.set("colorSpace",settings.get("colorSpace").isUndefined()?JSValue("srgb"):settings.get("colorSpace"));value.set("isRenderTargetTexture",settings.get("renderTarget").isUndefined()?JSValue(false):settings.get("renderTarget"));value.set("matrixAutoUpdate",settings.get("matrixAutoUpdate").isUndefined()?JSValue(true):settings.get("matrixAutoUpdate"));call(value.get("offset"),"set",{0.2,0.3});call(value.get("repeat"),"set",{2,3});value.set("rotation",0.4);return value;
}
inline double identity(const JSValue& values,const JSValue& value){for(double i=0;i<values.length();++i)if(values.get(JSValue(i)).sameValue(value))return i;return -1;}
inline void log(JSValue context,const std::string& name,const Args& args={}){Args values{name};values.insert(values.end(),args.begin(),args.end());push(context.get("events"),list(values));}
inline JSValue service(const JSValue&,const Args& args,const Args& captures){
    auto c=captures[0];const auto name=captures[1].toString();
    if(name=="xr"){log(c,"xr");if(c.get("xrThrow").toBoolean()){c.set("xrThrow",false);throw JSThrownValue("xr");}return c.get("mode");}
    if(name=="target"){log(c,"target");return c.get("target");}
    if(name=="clear"){log(c,"clear",args);if(c.get("clearThrow").toBoolean()){c.set("clearThrow",false);throw JSThrownValue("clear");}return {};}
    if(name=="setClear"||name=="color.setMask"||name=="depth.setTest"||name=="depth.setMask"){log(c,name,args);return {};}
    if(name=="env"){
        log(c,"env",{identity(c.get("textures"),arg(args,0)),arg(args,1)});if(c.get("envThrow").toBoolean()){c.set("envThrow",false);throw JSThrownValue("env");}
        const auto mode=c.get("request").get("envResult");return mode.strictEquals("null")?JSValue(nullptr):mode.strictEquals("undefined")?JSValue{}:mode.strictEquals("alternate")?c.get("textures").get("1"):mode.strictEquals("first")?c.get("textures").get("0"):arg(args,0);
    }
    if(name=="objects"){
        const auto mesh=arg(args,0);push(c.get("meshes"),mesh);const auto index=c.get("meshes").length()-1;log(c,"objects",{index,mesh.get("material").get("name")});
        for(const auto* type:{"geometry","material"})call(mesh.get(type),"addEventListener",{"dispose",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"dispose:"+c[1].toString(),{c[2]});if(c[1].strictEquals("geometry")&&c[0].get("disposeThrow").toBoolean()){auto ctx=c[0];ctx.set("disposeThrow",false);throw JSThrownValue("dispose");}return {};},{c,type,index})});
        if(c.get("updateThrow").toBoolean()){c.set("updateThrow",false);throw JSThrownValue("objects");}return mesh.get("geometry");
    }
    if(name=="unshift"){
        const auto mesh=arg(args,0);log(c,"unshift",{identity(c.get("meshes"),mesh),arg(args,1).sameValue(mesh.get("geometry")),arg(args,2).sameValue(mesh.get("material")),arg(args,3),arg(args,4),arg(args,5)});push(c.get("draws"),mesh);return {};
    }
    throw std::logic_error("Unknown background test service");
}
inline JSValue callback(JSValue c,const char* name){return JSValue::function(service,{c,name});}
inline JSValue context(const JSValue& request,JSValue events){
    auto c=object();c.set("request",request);c.set("events",events);c.set("meshes",list());c.set("draws",list());auto textures=list();
    if(request.get("textures").isUndefined()){auto a=object(),b=object();a.set("kind","plane");a.set("colorSpace","srgb");b.set("kind","cube");b.set("colorSpace","srgb-linear");push(textures,texture(a));push(textures,texture(b));}
    else for(double i=0;i<request.get("textures").length();++i)push(textures,texture(request.get("textures").get(JSValue(i))));c.set("textures",textures);
    JSValue target=nullptr;if(request.get("target").strictEquals("target"))target=object();else if(request.get("target").strictEquals("xr")){target=object();target.set("isXRRenderTarget",true);auto t=object();t.set("colorSpace","srgb");target.set("texture",t);}c.set("target",target);c.set("mode",request.get("mode"));
    auto renderer=object(),xr=object();renderer.set("autoClear",request.hasOwn("autoClear")?request.get("autoClear"):JSValue(true));for(const auto* key:{"autoClearColor","autoClearDepth","autoClearStencil"})renderer.set(key,true);renderer.set("outputColorSpace",request.get("outputColorSpace").isUndefined()?JSValue("srgb"):request.get("outputColorSpace"));renderer.set("toneMapping",0);xr.set("getEnvironmentBlendMode",callback(c,"xr"));renderer.set("xr",xr);renderer.set("getRenderTarget",callback(c,"target"));renderer.set("clear",callback(c,"clear"));c.set("renderer",renderer);
    auto state=object(),buffers=object(),color=object(),depth=object();color.set("setClear",callback(c,"setClear"));color.set("setMask",callback(c,"color.setMask"));depth.set("setTest",callback(c,"depth.setTest"));depth.set("setMask",callback(c,"depth.setMask"));buffers.set("color",color);buffers.set("depth",depth);state.set("buffers",buffers);c.set("state",state);
    auto environments=object(),objects=object(),renderList=object();environments.set("get",callback(c,"env"));objects.set("update",callback(c,"objects"));renderList.set("unshift",callback(c,"unshift"));c.set("environments",environments);c.set("objects",objects);c.set("renderList",renderList);
    auto scene=object();scene.set("isScene",request.hasOwn("isScene")?request.get("isScene"):JSValue(true));scene.set("background",nullptr);scene.set("backgroundBlurriness",0);scene.set("backgroundIntensity",1);scene.set("backgroundRotation",nativeEulerConstructor().construct());c.set("scene",scene);
    c.set("factory",nativeWebGLBackgroundConstructor().construct({renderer,environments,state,objects,request.get("alpha"),request.get("premultiplied")}));return c;
}
inline JSValue meshSnapshot(const JSValue& c,const JSValue& mesh){
    const auto g=mesh.get("geometry"),m=mesh.get("material"),u=m.get("uniforms");const auto cube=m.get("name").strictEquals("BackgroundCubeMaterial");const auto key=cube?"envMap":"map";const auto d=m.getOwnPropertyDescriptor(key);auto result=object(),geometry=object(),material=object(),attributes=list();
    result.set("identity",identity(c.get("meshes"),mesh));geometry.set("type",g.get("type"));for(const auto& a:g.get("attributes").keys())push(attributes,a);geometry.set("attributes",attributes);geometry.set("position",arrayOf(g.get("attributes").get("position").get("array")));geometry.set("index",arrayOf(g.get("index").get("array")));result.set("geometry",geometry);
    for(const auto* k:{"name","type","side","depthTest","depthWrite","fog","allowOverride","toneMapped","version","vertexShader","fragmentShader"})material.set(k,m.get(k));auto uniformKeys=list();for(const auto& k:u.keys())push(uniformKeys,k);material.set("uniformKeys",uniformKeys);result.set("material",material);result.set("layers",mesh.get("layers").get("mask"));result.set("texture",identity(c.get("textures"),m.get(key)));result.set("intensity",u.get("backgroundIntensity").get("value"));auto getterInfo=fnMetadata(*d->getter);getterInfo.set("flags",list({d->enumerable.value_or(false),d->configurable.value_or(false)}));result.set("getter",getterInfo);result.set("matrixWorld",call(mesh.get("matrixWorld"),"toArray"));
    if(cube){result.set("rotation",call(u.get("backgroundRotation").get("value"),"toArray"));result.set("blurriness",u.get("backgroundBlurriness").get("value"));result.set("beforeRender",fnMetadata(mesh.get("onBeforeRender")));}else result.set("uv",call(u.get("uvTransform").get("value"),"toArray"));return result;
}
inline JSValue action(const JSValue& c,const JSValue& a){
    auto scene=c.get("scene");if(a.hasOwn("texture"))scene.set("background",c.get("textures").get(a.get("texture")));
    if(a.get("background").strictEquals("color"))scene.set("background",nativeColorConstructor().construct({0.1,0.4,0.7}));else if(a.get("background").strictEquals("none"))scene.set("background",nullptr);else if(a.get("background").strictEquals("texture"))scene.set("background",c.get("textures").get("0"));
    if(a.hasOwn("intensity"))scene.set("backgroundIntensity",a.get("intensity"));if(a.hasOwn("blurriness"))scene.set("backgroundBlurriness",a.get("blurriness"));if(a.hasOwn("rotation")){const auto r=a.get("rotation");call(scene.get("backgroundRotation"),"set",{r.get("0"),r.get("1"),r.get("2")});}if(a.get("bumpVersion").toBoolean()){auto bg=scene.get("background");bg.set("version",bg.get("version").toNumber()+1);}if(a.hasOwn("toneMapping")){auto renderer=c.get("renderer");renderer.set("toneMapping",a.get("toneMapping"));}
    const auto op=a.get("op"),factory=c.get("factory");JSValue result;
    if(op.strictEquals("setColor"))result=call(factory,"setClearColor",{a.get("color"),a.get("alpha")});else if(op.strictEquals("setAlpha"))result=call(factory,"setClearAlpha",{a.get("alpha")});else if(op.strictEquals("render"))result=call(factory,"render",{scene});else if(op.strictEquals("dispose"))result=call(factory,"dispose");else result=call(factory,"addToRenderList",{c.get("renderList"),scene});
    const auto draws=c.get("draws");if(op.strictEquals("add")&&draws.length()>0){const auto mesh=draws.get(JSValue(draws.length()-1));if(mesh.get("material").get("name").strictEquals("BackgroundCubeMaterial")){auto camera=object();camera.set("matrixWorld",call(nativeMatrix4Constructor().construct(),"makeTranslation",{3,4,5}));call(mesh,"onBeforeRender",{c.get("renderer"),scene,camera});}}
    auto out=object(),meshes=list(),indices=list();out.set("undefined",result.isUndefined());out.set("color",list({call(factory,"getClearColor").get("r"),call(factory,"getClearColor").get("g"),call(factory,"getClearColor").get("b")}));out.set("alpha",call(factory,"getClearAlpha"));
    for(double i=0;i<c.get("meshes").length();++i)push(meshes,meshSnapshot(c,c.get("meshes").get(JSValue(i))));for(double i=0;i<draws.length();++i)push(indices,identity(c.get("meshes"),draws.get(JSValue(i))));out.set("meshes",meshes);out.set("draws",indices);return out;
}
inline JSValue actionRecord(const char* op,int textureIndex=-1){auto a=object();a.set("op",op);if(textureIndex>=0)a.set("texture",textureIndex);return a;}
inline JSValue observable(const JSValue& request,JSValue events){
    const auto c=context(request,events),variant=request.get("variant");auto rows=list();const auto attempt=[&](JSValue a){auto row=object();try{row.set("value",action(c,a));}catch(...){row.set("error",error());}push(rows,row);};
    if(variant.strictEquals("updateThrow")||variant.strictEquals("envThrow")||variant.strictEquals("clearThrow")||variant.strictEquals("xrThrow")||variant.strictEquals("disposeThrow")){
        auto ctx=c;ctx.set(variant,true);const auto render=variant.strictEquals("clearThrow")||variant.strictEquals("xrThrow");
        if(variant.strictEquals("disposeThrow")){attempt(actionRecord("add",1));attempt(actionRecord("add",0));attempt(actionRecord("dispose"));attempt(actionRecord("dispose"));attempt(actionRecord("add",1));}
        else{attempt(actionRecord(render?"render":"add",variant.strictEquals("updateThrow")?1:0));attempt(actionRecord(render?"render":"add",variant.strictEquals("updateThrow")?1:0));}
    }else if(variant.strictEquals("cacheGetters")){
        auto counter=object();counter.set("version",0);getter(c.get("textures").get("1"),"version",JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{push(cap[0],"get:version");auto state=cap[1];const auto value=state.get("version");state.set("version",value.toNumber()+1);return value;},{events,counter}));getter(c.get("renderer"),"toneMapping",JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{push(cap[0],"get:toneMapping");return 0;},{events}));for(int i=0;i<3;++i)attempt(actionRecord("add",1));
    }else if(variant.strictEquals("rgbReentry")){
        const auto other=context(object(),events);call(other.get("factory"),"setClearColor",{0x123456,0.25});events.set("length",0);auto color=call(c.get("factory"),"getClearColor");color.set("getRGB",JSValue::function(+[](const JSValue&,const Args& a,const Args& cap)->JSValue{push(cap[0],"outer:getRGB");auto target=arg(a,0);target.set("r",1);target.set("g",2);target.set("b",3);call(cap[1].get("factory"),"render",{cap[1].get("scene")});return target;},{events,other}));attempt(actionRecord("render"));
    }else if(variant.strictEquals("uniformAlias")){
        auto token=object();token.set("raw",true);auto scene=c.get("scene");scene.set("backgroundIntensity",token);scene.set("backgroundBlurriness",token);attempt(actionRecord("add",1));auto row=object();const auto uniforms=c.get("meshes").get("0").get("material").get("uniforms");row.set("intensityAlias",uniforms.get("backgroundIntensity").get("value").sameValue(token));row.set("blurrinessAlias",uniforms.get("backgroundBlurriness").get("value").sameValue(token));push(rows,row);
    }else if(variant.strictEquals("mapGetter")){
        attempt(actionRecord("add",0));auto material=c.get("meshes").get("0").get("material"),uniform=material.get("uniforms").get("t2D");uniform.set("value",c.get("textures").get("1"));auto row=object();row.set("alias",material.get("map").sameValue(c.get("textures").get("1")));push(rows,row);
    }else if(variant.strictEquals("beforeRenderReceiver")){
        attempt(actionRecord("add",1));const auto mesh=c.get("meshes").get("0");auto receiver=object(),camera=object();receiver.set("matrixWorld",nativeMatrix4Constructor().construct());camera.set("matrixWorld",call(nativeMatrix4Constructor().construct(),"makeTranslation",{8,9,10}));mesh.get("onBeforeRender").call(receiver,{c.get("renderer"),c.get("scene"),camera});auto row=object();row.set("receiver",call(receiver.get("matrixWorld"),"toArray"));row.set("mesh",call(mesh.get("matrixWorld"),"toArray"));push(rows,row);
    }else throw std::logic_error("Unknown background observable");return rows;
}
inline JSValue colorRGB(const JSValue& request,JSValue events){
    const auto variant=request.get("variant");auto self=nativeColorConstructor().construct({0.1,0.4,0.7}),target=object(),stored=object(),management=nativeColorManagement();const auto original=management.getOwnPropertyDescriptor("workingToColorSpace");
    if(variant.strictEquals("metadata"))return fnMetadata(nativeColorConstructor().get("prototype").get("getRGB"));
    for(const auto* key:{"r","g","b"}){
        JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{return cap[0].get(cap[1]);},{stored,key});
        d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& cap)->JSValue{
            push(cap[0],list({"set:"+cap[1].toString(),arg(a,0)}));auto values=cap[2];values.set(cap[1],arg(a,0));
            if(cap[3].strictEquals("setterThrow")&&cap[1].strictEquals("g"))throw JSThrownValue("setter");
            if(cap[3].strictEquals("setterReentry")&&cap[1].strictEquals("r"))call(nativeColorConstructor().construct({0.8,0.2,0.3}),"getRGB",{object()});return {};
        },{events,key,stored,variant});d.enumerable=true;target.defineProperty(key,d);
        if(variant.strictEquals("getterOrder")){self.erase(key);getter(self,key,JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{push(cap[0],"get:"+cap[1].toString());return cap[1].strictEquals("r")?JSValue(0.1):cap[1].strictEquals("g")?JSValue(0.4):JSValue(0.7);},{events,key}));}
    }
    if(variant.strictEquals("returnIgnored")||variant.strictEquals("convertReentry")||variant.strictEquals("convertThrow"))management.set("workingToColorSpace",JSValue::function(+[](const JSValue&,const Args& a,const Args& cap)->JSValue{
        push(cap[0],list({"convert",arg(a,1)}));if(cap[1].strictEquals("convertThrow"))throw JSThrownValue("convert");
        if(cap[1].strictEquals("convertReentry")){auto management=nativeColorManagement();management.set("workingToColorSpace",cap[2]);call(nativeColorConstructor().construct({0.6,0.8,0.2}),"getHex");}else{auto color=arg(a,0);color.set("r",0.2);color.set("g",0.3);color.set("b",0.4);}auto result=object();result.set("r",9);result.set("g",9);result.set("b",9);return result;
    },{events,variant,*original->value}));
    try{const auto output=call(self,"getRGB",{variant.strictEquals("targetNull")?JSValue(nullptr):target,request.get("colorSpace")});management.defineProperty("workingToColorSpace",*original);auto result=object();result.set("same",output.sameValue(target));result.set("stored",stored);return result;}catch(...){management.defineProperty("workingToColorSpace",*original);throw;}
}
inline JSValue run(const JSValue& request){
    if(request.get("op").strictEquals("metadata"))return metadata();auto events=list(),out=object();try{JSValue value;if(request.get("op").strictEquals("observable"))value=observable(request,events);else if(request.get("op").strictEquals("colorRGB"))value=colorRGB(request,events);else{const auto c=context(request,events);value=list();for(double i=0;i<request.get("actions").length();++i)push(value,action(c,request.get("actions").get(JSValue(i))));}out.set("value",clean(value));out.set("events",clean(events));}catch(...){out.set("error",error());out.set("events",clean(events));}return out;
}
}
