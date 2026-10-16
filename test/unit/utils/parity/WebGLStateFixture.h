#pragma once
#include "renderers/webgl/WebGLState.h"
#include "core/NativeObject.h"
#include "core/NativeArray.h"
#include "core/JSWeakMap.h"
#include "core/JSIterator.h"
#include "core/JSRegExp.h"
#include "math/Vector4.h"
#include "math/Vector2.h"
#include "math/Quaternion.h"
#include "core/Object3D.h"
#include "math/Euler.h"
#include "math/Vector3.h"
#include "utils.h"
#include <cmath>
#include <limits>
#include <iostream>

namespace StateFixture {
using Args = std::vector<JSValue>;
using A = JsonValue::Array;
using O = JsonValue::Object;
inline void property(JSValue object, const char* name, const JSValue& value) {
    JSPropertyDescriptor d; d.value=value; d.writable=d.enumerable=d.configurable=true; object.defineProperty(name,d);
}
inline JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
inline JSValue array(const Args& args){auto result=JSValue::array();for(std::size_t i=0;i<args.size();++i)property(result,std::to_string(i).c_str(),args[i]);return result;}
inline JSValue call(const JSValue& self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
inline JsonValue encode(const JSValue& value){
    if(value.isUndefined())return "undefined";if(value.isNull())return nullptr;
    if(value.type()==JSValue::Type::Boolean)return value.toBoolean();
    if(value.type()==JSValue::Type::String)return value.string();
    if(value.type()==JSValue::Type::Number){const auto n=value.number();if(std::isnan(n))return "NaN";if(std::isinf(n))return n<0?"-Infinity":"Infinity";if(n==0&&std::signbit(n))return "-0";return n;}
    if(value.type()==JSValue::Type::Symbol)return "symbol";
    if(value.isCallable())return "function";
    if(value.hasOwn("tag"))return O{{"ref",value.get("tag").toString()}};
    if(value.type()==JSValue::Type::Array){A out;for(unsigned i=0;i<value.length();++i)out.push_back(encode(value.get(std::to_string(i))));return out;}
    if(value.isHostObject("three.AnimationArray")){A out;const auto n=value.get("length").toNumber();for(unsigned i=0;i<n;++i)out.push_back(encode(value.get(std::to_string(i))));return O{{"typed","Uint8Array"},{"data",out}};}
    O out;for(const auto& key:value.keys()){const auto d=value.getOwnPropertyDescriptor(key);out[key]=d&&d->value?encode(*d->value):JsonValue("accessor");}return out;
}
inline void log(JSValue state,const Args& args){call(state.get("log"),"push",{JSValue::fromJSON(encode(array(args)))});}
inline JSValue decode(const JSValue& state,const JSValue& value);
inline JSValue execute(JSValue state,const JSValue& operation);
inline void actions(JSValue state,const JSValue& values){if(values.isUndefined())return;for(unsigned i=0;i<values.length();++i)execute(state,values.get(std::to_string(i)));}
inline void trigger(JSValue state,const std::string& name){
    if(state.get("trigger").strictEquals(name)&&!state.get("acted").toBoolean()){
        state.set("acted",true);actions(state,state.get("action"));
    }
}
inline JSValue command(const JSValue& self,const Args& args,const Args& captures){
    auto state=captures[0];const auto name=captures[1].string(),surface=captures[2].string();
    Args row{name,self.sameValue(state.get(surface))};for(const auto& value:args)row.push_back(JSValue::fromJSON(encode(value)));log(state,row);
    trigger(state,name);
    if(state.get("fail").strictEquals(name))throw state.get("thrown");
    if(name=="getParameter"){
        const auto key=arg(args,0);if(key.strictEquals(state.get("gl").get("MAX_COMBINED_TEXTURE_IMAGE_UNITS")))return state.get("max");
        if(key.strictEquals(state.get("gl").get("VERSION")))return state.get("version");
        if(key.strictEquals(state.get("gl").get("SCISSOR_BOX")))return state.get("scissor");
        if(key.strictEquals(state.get("gl").get("VIEWPORT")))return state.get("viewport");
        return std::string("parameter:")+key.toString();
    }
    if(name=="createTexture"){const auto n=state.get("next").toNumber()+1;state.set("next",n);auto value=JSValue::object();property(value,"tag","texture:"+JSValue(n).toString());return value;}
    if(name=="get")return state.get("extension");
    if(name=="getUniformBlockIndex")return state.get("blockIndex");
    return {};
}
inline JSValue console(const JSValue& self,const Args& args,const Args& captures){
    auto state=captures[0];Args row{"console",self.isUndefined()};for(const auto& value:args)row.push_back(JSValue::fromJSON(encode(value)));log(state,row);trigger(state,"console");
    if(state.get("fail").strictEquals("console"))throw state.get("thrown");return {};
}
inline JSValue primitive(const JSValue&,const Args& args,const Args& captures){
    auto state=captures[0];const auto spec=captures[1];log(state,{"coerce",spec.get("name"),arg(args,0)});
    actions(state,spec.get("action"));if(spec.get("throw").toBoolean())throw JSRangeError("coercion");return decode(state,spec.get("value"));
}
inline JSValue decode(const JSValue& state,const JSValue& value){
    if(!value.isObject())return value;
    if(value.hasOwn("$ref"))return state.get("objects").get(value.get("$ref"));
    if(value.hasOwn("$")){
        const auto name=value.get("$").toString();if(name=="undefined")return {};if(name=="NaN")return std::numeric_limits<double>::quiet_NaN();
        if(name=="Infinity")return std::numeric_limits<double>::infinity();if(name=="-Infinity")return -std::numeric_limits<double>::infinity();if(name=="-0")return -0.0;
        if(name=="symbol")return state.get("symbol");
    }
    if(value.hasOwn("$coerce")){
        const auto spec=value.get("$coerce");auto result=JSValue::object();property(result,"tag",spec.get("name"));
        result.setPrimitiveConverter(JSValue::function(primitive,{state,spec}));return result;
    }
    if(value.hasOwn("$nativeObject3D")){auto result=nativeObject(std::make_shared<Object3D>());property(result,"tag","Object3D");return result;}
    if(value.hasOwn("$nativeQuaternion")){auto q=std::make_shared<Quaternion>();if(value.get("$nativeQuaternion").toBoolean())q->_onChange([weak=JSWeakValue(state)](){if(auto live=weak.lock())log(*live,{"onChange","Quaternion"});});auto wrapped=nativeQuaternion(q);property(wrapped,"tag","Quaternion");return wrapped;}
    if(value.hasOwn("$nativeEuler")){auto q=std::make_shared<Euler>();if(value.get("$nativeEuler").toBoolean())q->_onChange([weak=JSWeakValue(state)](){if(auto live=weak.lock())log(*live,{"onChange","Euler"});});auto wrapped=nativeEuler(q);property(wrapped,"tag","Euler");return wrapped;}
    if(value.hasOwn("$vector")||value.hasOwn("$color")){
        const bool color=value.hasOwn("$color");auto result=JSValue::object(color?nativeColorPrototype():state.get("vectorPrototype"));
        const auto values=value.get(color?"$color":"$vector");if(color)property(result,"isColor",true);
        const auto fields=color?"rgb":"xyzw";
        for(unsigned i=0;fields[i];++i)property(result,std::string(1,fields[i]).c_str(),decode(state,values.get(std::to_string(i))));return result;
    }
    auto out=value.type()==JSValue::Type::Array?JSValue::array():JSValue::object();
    for(const auto& key:value.keys())property(out,key.c_str(),decode(state,value.get(key)));return out;
}
inline JSValue getPath(JSValue state,const JSValue& path){
    if(path.type()==JSValue::Type::String)return state.get("objects").get(path);
    auto result=state.get("objects").get(path.get("0"));for(unsigned i=1;i<path.length();++i)result=result.get(path.get(std::to_string(i)));return result;
}
inline JSValue getter(const JSValue&,const Args&,const Args& captures){
    auto state=captures[0],frame=captures[1];log(state,{"getter",frame.get("label"),frame.get("key")});
    actions(state,frame.get("action"));if(frame.get("throw").toBoolean())throw JSRangeError("getter");return frame.get("value");
}
inline JSValue setter(const JSValue&,const Args& args,const Args& captures){
    auto state=captures[0],frame=captures[1];log(state,{"setter",frame.get("label"),frame.get("key"),arg(args,0)});actions(state,frame.get("setAction"));if(frame.get("setThrow").toBoolean())throw JSRangeError("setter");frame.set("value",arg(args,0));return {};
}
inline JSValue overridden(const JSValue& self,const Args& args,const Args& captures){
    auto state=captures[0];const auto frame=captures[1],spec=frame.get("spec");
    Args row{"override",spec.get("label"),JSValue::fromJSON(encode(self))};for(const auto& value:args)row.push_back(JSValue::fromJSON(encode(value)));log(state,row);
    actions(state,spec.get("action"));if(spec.get("throw").toBoolean())throw JSRangeError("override");
    if(spec.get("forward").toBoolean())return frame.get("original").call(self,args);
    if(spec.get("returnThis").toBoolean())return self;
    return decode(state,spec.get("result"));
}
inline JSValue execute(JSValue state,const JSValue& operation){
    const auto kind=operation.get("0").string();
    if(kind=="save"){state.get("objects").set(operation.get("1"),getPath(state,operation.get("2")));return {};}
    if(kind=="nativeRegister"){
        const auto target=getPath(state,operation.get("1"));const auto label=operation.get("2").toString();
        const auto spec=operation.get("3").toJSON().value_or(JsonValue::Object{});
        auto callback=[weak=JSWeakValue(state),label,spec,count=0u]() mutable {
            if(auto current=weak.lock()) {
                log(*current,{"nativeCallback",label,++count});const auto action=JSValue::fromJSON(spec);
                actions(*current,action.get("action"));if(action.get("throw").toBoolean())throw JSRangeError("native callback");
            }
        };
        if(target.isHostObject("three.Quaternion"))std::static_pointer_cast<Quaternion>(target.hostPayload("three.Quaternion"))->_onChange(std::move(callback));
        else std::static_pointer_cast<Euler>(target.hostPayload("three.Euler"))->_onChange(std::move(callback));
        return {};
    }
    if(kind=="nativeSet"){
        const auto target=getPath(state,operation.get("1"));const auto value=decode(state,operation.get("2")).toNumber();
        if(target.isHostObject("three.Quaternion"))std::static_pointer_cast<Quaternion>(target.hostPayload("three.Quaternion"))->x=value;
        else std::static_pointer_cast<Euler>(target.hostPayload("three.Euler"))->x=value;
        return {};
    }
    if(kind=="callbackFlags"){
        auto target=getPath(state,operation.get("1"));const auto spec=operation.get("2");JSPropertyDescriptor d;
        if(spec.hasOwn("writable"))d.writable=spec.get("writable").toBoolean();
        if(spec.hasOwn("configurable"))d.configurable=spec.get("configurable").toBoolean();
        if(spec.hasOwn("enumerable"))d.enumerable=spec.get("enumerable").toBoolean();
        target.defineProperty("_onChangeCallback",d);return {};
    }
    if(kind=="call"||kind=="construct"||kind=="borrow"){
        const auto path=operation.get("1"),self=getPath(state,path);const auto method=operation.get("2");
        const auto fn=self.get(method);Args args;const auto values=operation.get("3");for(unsigned i=0;i<values.length();++i)args.push_back(decode(state,values.get(std::to_string(i))));
        if(kind=="construct")return fn.construct(args);
        return fn.call(kind=="borrow"?decode(state,operation.get("4")):self,args);
    }
    if(kind=="descriptor"){
        const auto d=getPath(state,operation.get("1")).getOwnPropertyDescriptor(operation.get("2"));
        if(!d)return JSValue::fromJSON(O{{"own",false}});
        return JSValue::fromJSON(O{{"own",true},{"writable",d->writable.value_or(false)},{"enumerable",d->enumerable.value_or(false)},{"configurable",d->configurable.value_or(false)},{"value",d->value?encode(*d->value):JsonValue("accessor")}});
    }
    if(kind=="getKey")return getPath(state,operation.get("1")).get(decode(state,operation.get("2")));
    if(kind=="setKey"){auto target=getPath(state,operation.get("1"));target.set(decode(state,operation.get("2")),decode(state,operation.get("3")));}
    else if(kind=="set"){auto target=getPath(state,operation.get("1"));target.set(operation.get("2"),decode(state,operation.get("3")));}
    else if(kind=="define"){auto target=getPath(state,operation.get("1"));property(target,operation.get("2").toString().c_str(),decode(state,operation.get("3")));}
    else if(kind=="delete"){auto target=getPath(state,operation.get("1"));target.erase(operation.get("2"));}
    else if(kind=="object"){auto value=decode(state,operation.get("2"));state.get("objects").set(operation.get("1"),value);}
    else if(kind=="fail")state.set("fail",operation.get("1"));
    else if(kind=="trigger"){state.set("trigger",operation.get("1"));state.set("action",operation.get("2"));state.set("acted",false);}
    else if(kind=="watch"){
        auto target=getPath(state,operation.get("1")),frame=JSValue::object(nullptr);const auto key=operation.get("2"),spec=operation.get("3");
        property(frame,"key",key);property(frame,"value",target.get(key));property(frame,"label",spec.get("label").isUndefined()?operation.get("1"):spec.get("label"));
        property(frame,"action",spec.get("action"));property(frame,"throw",spec.get("throw"));property(frame,"setAction",spec.get("setAction"));property(frame,"setThrow",spec.get("setThrow"));
        JSPropertyDescriptor d;d.getter=JSValue::function(getter,{state,frame});d.setter=JSValue::function(setter,{state,frame});d.enumerable=d.configurable=true;target.defineProperty(key,d);
    } else if(kind=="gc")JSValue::object().collectGarbage();
    else if(kind=="function"){
        auto target=getPath(state,operation.get("1")),frame=JSValue::object(nullptr);const auto key=operation.get("2"),spec=operation.get("3");
        property(frame,"original",target.get(key));property(frame,"spec",spec);
        property(target,key.toString().c_str(),JSValue::function(overridden,{state,frame}));
    }
    else if(kind=="clear")state.get("log").set("length",0);
    else if(kind=="metadata"){
        const auto self=getPath(state,operation.get("1")),fn=self.get(operation.get("2"));
        A own,flags;for(const auto& name:fn.ownPropertyNames())own.push_back(name);
        const auto descriptor=fn.getOwnPropertyDescriptor("prototype");
        if(descriptor)flags=A{descriptor->writable.value_or(false),descriptor->enumerable.value_or(false),descriptor->configurable.value_or(false)};
        return JSValue::fromJSON(O{{"name",encode(fn.get("name"))},{"length",encode(fn.get("length"))},{"own",own},{"prototypeFlags",flags},
                                  {"source",fn.nativeFunctionSource()},{"constructible",fn.isConstructible()}});
    }
    return {};
}
template<class F> JsonValue outcome(F action){
    try{return encode(action());}catch(const JSValue& e){return O{{"thrown",encode(e)}};}
    catch(const JSTypeError&){return O{{"error","TypeError"}};}catch(const JSRangeError&){return O{{"error","RangeError"}};}
    catch(const std::exception& e){return O{{"error","Error"},{"message",e.what()}};}
}
struct Fixture {
    JSValue state=JSValue::object(nullptr),gl=JSValue::object(),extensions=JSValue::object(),extension=JSValue::object(),manager;
    Fixture(const JSValue& config){
        property(state,"log",JSValue::array());property(state,"gl",gl);property(state,"extensions",extensions);property(state,"extension",extension);
        property(state,"next",0);property(state,"max",8);property(state,"version","WebGL 2.0");
        property(state,"scissor",array({0,0,640,480}));property(state,"viewport",array({0,0,640,480}));property(state,"blockIndex",7);
        property(state,"fail","");property(state,"trigger","");property(state,"acted",false);property(state,"action",JSValue::array());
        auto thrown=JSValue::object();property(thrown,"tag","thrown");property(state,"thrown",thrown);property(state,"symbol",JSValue::symbol("slot"));
        auto objects=JSValue::object(nullptr);property(state,"objects",objects);
        for(const auto& [name,value]:std::initializer_list<std::pair<const char*,JSValue>>{{"gl",gl},{"extensions",extensions},{"extension",extension},{"state",state}})property(objects,name,value);
        for(const auto& key:config.keys())if(key!="before")state.set(key,decode(state,config.get(key)));
        for(const auto* name:{"getParameter","createTexture","bindTexture","texParameteri","texImage2D","texImage3D","enable","disable","colorMask","clearColor","depthMask","depthFunc",
            "clearDepth","stencilMask","stencilFunc","stencilOp","clearStencil","frontFace","cullFace","blendEquation","blendFuncSeparate","blendFunc","blendEquationSeparate","blendColor",
            "lineWidth","polygonOffset","activeTexture","bindFramebuffer","useProgram","drawBuffers","pixelStorei","scissor","viewport","getUniformBlockIndex","uniformBlockBinding","bindBufferBase",
            "compressedTexImage2D","compressedTexImage3D","texStorage2D","texStorage3D","texSubImage2D","texSubImage3D","compressedTexSubImage2D","compressedTexSubImage3D"})
            property(gl,name,JSValue::function(command,{state,name,"gl"}));
        property(extensions,"get",JSValue::function(command,{state,"get","extensions"}));property(extension,"clipControlEXT",JSValue::function(command,{state,"clipControlEXT","extension"}));
        for(const auto& [name,value]:std::initializer_list<std::pair<const char*,int>>{
            {"MAX_COMBINED_TEXTURE_IMAGE_UNITS",35661},{"VERSION",7938},{"SCISSOR_BOX",3088},{"VIEWPORT",2978},
            {"TEXTURE_2D",3553},{"TEXTURE_CUBE_MAP",34067},{"TEXTURE_CUBE_MAP_POSITIVE_X",34069},{"TEXTURE_2D_ARRAY",35866},{"TEXTURE_3D",32879},
            {"TEXTURE_MIN_FILTER",10241},{"TEXTURE_MAG_FILTER",10240},{"NEAREST",9728},{"RGBA",6408},{"UNSIGNED_BYTE",5121},{"TEXTURE0",33984},
            {"DEPTH_TEST",2929},{"CULL_FACE",2884},{"BLEND",3042},{"STENCIL_TEST",2960},{"POLYGON_OFFSET_FILL",32823},{"SCISSOR_TEST",3089},{"SAMPLE_ALPHA_TO_COVERAGE",32926},
            {"NEVER",512},{"ALWAYS",519},{"LESS",513},{"LEQUAL",515},{"EQUAL",514},{"GEQUAL",518},{"GREATER",516},{"NOTEQUAL",517},
            {"CW",2304},{"CCW",2305},{"BACK",1029},{"FRONT",1028},{"FRONT_AND_BACK",1032},{"FRAMEBUFFER",36160},{"DRAW_FRAMEBUFFER",36009},{"READ_FRAMEBUFFER",36008},{"COLOR_ATTACHMENT0",36064},
            {"FUNC_ADD",32774},{"FUNC_SUBTRACT",32778},{"FUNC_REVERSE_SUBTRACT",32779},{"MIN",32775},{"MAX",32776},
            {"ZERO",0},{"ONE",1},{"SRC_COLOR",768},{"SRC_ALPHA",770},{"SRC_ALPHA_SATURATE",776},{"DST_COLOR",774},{"DST_ALPHA",772},
            {"ONE_MINUS_SRC_COLOR",769},{"ONE_MINUS_SRC_ALPHA",771},{"ONE_MINUS_DST_COLOR",775},{"ONE_MINUS_DST_ALPHA",773},
            {"CONSTANT_COLOR",32769},{"ONE_MINUS_CONSTANT_COLOR",32770},{"CONSTANT_ALPHA",32771},{"ONE_MINUS_CONSTANT_ALPHA",32772},
            {"UNIFORM_BUFFER",35345},{"KEEP",7680},{"PACK_ALIGNMENT",3333},{"UNPACK_ALIGNMENT",3317},{"UNPACK_FLIP_Y_WEBGL",37440},
            {"UNPACK_PREMULTIPLY_ALPHA_WEBGL",37441},{"UNPACK_COLORSPACE_CONVERSION_WEBGL",37443},{"BROWSER_DEFAULT_WEBGL",37444},
            {"PACK_ROW_LENGTH",3330},{"PACK_SKIP_PIXELS",3332},{"PACK_SKIP_ROWS",3331},{"UNPACK_ROW_LENGTH",3314},{"UNPACK_IMAGE_HEIGHT",32878},
            {"UNPACK_SKIP_PIXELS",3316},{"UNPACK_SKIP_ROWS",3315},{"UNPACK_SKIP_IMAGES",32877}})property(gl,name,value);
        property(extension,"LOWER_LEFT_EXT",36001);property(extension,"ZERO_TO_ONE_EXT",37727);property(extension,"NEGATIVE_ONE_TO_ONE_EXT",37726);
        auto canvas=JSValue::object();property(canvas,"width",640);property(canvas,"height",480);property(gl,"canvas",canvas);property(objects,"canvas",canvas);
        const auto vp=nativeVector(std::make_shared<Vector4>()).getPrototype();property(state,"vectorPrototype",vp);property(objects,"vectorPrototype",vp);property(objects,"colorPrototype",nativeColorPrototype());
        property(objects,"quaternionPrototype",nativeQuaternion(std::make_shared<Quaternion>()).getPrototype());
        property(objects,"eulerPrototype",nativeEuler(std::make_shared<Euler>()).getPrototype());
        property(objects,"vector2Prototype",nativeVector(std::make_shared<Vector2>()).getPrototype());
        property(objects,"vector3Prototype",nativeVector(std::make_shared<Vector3>()).getPrototype());
        property(objects,"weakMapPrototype",nativeWeakMapConstructor().get("prototype"));
        property(objects,"stringPrototype",nativeStringPrototype());property(objects,"regexpPrototype",nativeRegExpPrototype());
        property(objects,"reversed",ThreeUtils::ReversedDepthFuncs.object());
        ThreeUtils::setConsoleFunction(JSValue::function(console,{state}));
        actions(state,config.get("before"));
        try { manager=nativeWebGLStateConstructor().call({}, {gl,extensions}); }
        catch(const std::exception& e){std::cerr<<"constructor: "<<e.what()<<" trace="<<encode(state.get("log")).dump()<<'\n';ThreeUtils::setConsoleFunction(nullptr);throw;}
        property(objects,"manager",manager);
        property(objects,"constructorHolder",JSValue::object());property(objects.get("constructorHolder"),"ctor",nativeWebGLStateConstructor());
    }
    ~Fixture(){ThreeUtils::setConsoleFunction(nullptr);}
};
inline JsonValue scenario(const JSValue& input){
    struct ConsoleCleanup { ~ConsoleCleanup(){ThreeUtils::setConsoleFunction(nullptr);} } cleanup;
    A results;JsonValue trace;
    try{
        Fixture f(input.get("config"));const auto ops=input.get("ops");
        for(unsigned i=0;i<ops.length();++i)results.push_back(outcome([&]{return execute(f.state,ops.get(std::to_string(i)));}));
        trace=encode(f.state.get("log"));
    }catch(const JSValue& e){results.push_back(O{{"constructorThrown",encode(e)}});}
    catch(const JSTypeError&){results.push_back(O{{"constructorError","TypeError"}});}
    catch(const JSRangeError&){results.push_back(O{{"constructorError","RangeError"}});}
    catch(const std::exception& e){results.push_back(O{{"constructorError","Error"},{"message",e.what()}});}
    return O{{"name",input.get("name").toString()},{"results",results},{"trace",trace}};
}
} // namespace StateFixture
