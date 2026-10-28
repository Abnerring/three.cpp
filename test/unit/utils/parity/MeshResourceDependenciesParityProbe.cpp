#include "animation/AnimationUtils.h"
#include "core/JSGlobal.h"
#include "core/NativeObject.h"
#include "core/BufferGeometry.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include <cmath>
#include <iostream>
#include <limits>
namespace {
using Args=std::vector<JSValue>;
JSValue at(const Args& a,unsigned i){return i<a.size()?a[i]:JSValue{};}
JSValue arr(const Args& a={}){auto result=JSValue::array();for(const auto& v:a)result.get("push").call(result,{v});return result;}
void log(JSValue s,const Args& a){const auto events=s.get("events");events.get("push").call(events,{arr(a)});}
void tick(JSValue s,const std::string& label){log(s,{label});if(s.get("request").get("throwAt").strictEquals(label))throw JSTypeError(label);}
JSValue norm(const JSValue& v){if(v.isUndefined())return "#undefined";if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "#NaN";if(std::isinf(n))return n>0?"#Infinity":"#-Infinity";if(n==0&&std::signbit(n))return "#-0";}if(!v.isObject())return v;if(v.type()==JSValue::Type::Array){auto result=arr();for(unsigned i=0;i<v.length();++i)result.get("push").call(result,{norm(v.get(std::to_string(i)))});return result;}auto result=JSValue::object();for(const auto& key:v.keys())result.set(key,norm(v.get(key)));return result;}
JSValue decode(const JSValue& v){if(!v.isObject())return v;const auto tag=v.get("tag");if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();if(tag.strictEquals("Infinity"))return std::numeric_limits<double>::infinity();if(tag.strictEquals("-Infinity"))return -std::numeric_limits<double>::infinity();if(tag.strictEquals("-0"))return -0.0;return v;}
JSValue status(std::exception_ptr e){try{std::rethrow_exception(e);}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception& e){return std::string("#Error:")+e.what();}}
JSValue state(JSValue r){auto s=JSValue::object();s.set("request",r);s.set("events",arr());return s;}
JSValue math(JSValue r){
    auto s=state(r),out=JSValue::object();Args args;const auto raw=r.get("args");
    for(unsigned i=0;i<raw.length();++i){const auto v=raw.get(std::to_string(i));if(v.isObject()&&v.hasOwn("coerce")){auto input=JSValue::object();input.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"convert:"+c[1].toString());log(c[0],{"hint",at(a,0)});return decode(c[2]);},{s,i,v.get("coerce")},"primitive",1));args.push_back(input);}else args.push_back(decode(v));}
    const auto object=nativeGlobalBinding("Math"),function=object.get(r.get("name"));out.set("metadata",arr({function.get("name"),function.get("length"),function.isConstructible()}));
    try{const auto result=function.call(object,args);out.set("result",r.get("name").strictEquals("random")?JSValue(result.number()>=0&&result.number()<1):norm(result));}catch(...){out.set("result",status(std::current_exception()));}out.set("events",s.get("events"));return out;
}
JSValue vectorFrom(JSValue r){
    auto s=state(r),out=JSValue::object(),self=JSValue::object(),attribute=JSValue::object(),fields=JSValue::object();const auto count=static_cast<unsigned>(r.get("count").number());
    auto index=JSValue::object();index.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{tick(c[0],"index.convert");return -0.5;},{s},"primitive",1));
    for(unsigned i=0;i<count;++i){const std::string component(1,"xyzw"[i]),getter=std::string("get")+"XYZW"[i];fields.set(component,"initial");JSPropertyDescriptor d;d.configurable=d.enumerable=true;
        d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0].get(c[1]);},{fields,component});
        d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"set:"+c[2].string());auto fields=c[1];fields.set(c[2],at(a,0));return {};},{s,fields,component});self.defineProperty(component,d);
        JSPropertyDescriptor method;method.configurable=true;method.getter=JSValue::function(+[](const JSValue& receiver,const Args&,const Args& c)->JSValue{
            const auto key=c[1].string();tick(c[0],"get:"+key);return JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue{tick(c[0],"call:"+c[1].string());log(c[0],{"receiver",receiver.sameValue(c[3]),"index",at(a,0).sameValue(c[4])});return decode(c[2]);},{c[0],c[1],c[2],receiver,c[3]},key,1);
        },{s,getter,r.get("values").get(std::to_string(i)),index});attribute.defineProperty(getter,method);
    }
    const auto constructor=count==2?nativeVector2Constructor():count==3?nativeVector3Constructor():nativeVector4Constructor();const auto fn=constructor.get("prototype").get("fromBufferAttribute");
    out.set("metadata",arr({fn.get("name"),fn.get("length"),fn.isConstructible()}));try{out.set("result",fn.call(self,{attribute,index}).sameValue(self)?JSValue("@self"):JSValue("@other"));}catch(...){out.set("result",status(std::current_exception()));}out.set("fields",norm(fields));out.set("events",s.get("events"));return out;
}
JSValue view(JSValue r){
    const auto constructor=nativeGlobalBinding(r.get("name").string());auto value=constructor.construct({arr({1,2,3})});const auto mode=r.get("mode");
    if(mode.strictEquals("proxy"))value=JSValue::proxy(value,JSValue::object());if(mode.strictEquals("fake")){value=JSValue::object();value.set("isTypedArray",true);}if(mode.strictEquals("buffer"))value=nativeGlobalBinding("ArrayBuffer").construct({8});if(mode.strictEquals("null"))value=nullptr;if(mode.strictEquals("array"))value=arr();
    const auto object=nativeGlobalBinding("ArrayBuffer"),method=object.get("isView");auto out=JSValue::object();out.set("result",method.call(object,{value}));out.set("metadata",arr({method.get("name"),method.get("length"),method.isConstructible()}));out.set("identity",constructor.sameValue(nativeGlobalBinding(r.get("name").string())));return out;
}
JSValue vectorMethod(JSValue r){
    auto out=JSValue::object();const auto count=r.get("count").number();const auto ctor=count==2?nativeVector2Constructor():nativeVector3Constructor();auto value=ctor.construct({decode(r.get("x")),decode(r.get("y")),decode(r.get("z"))});auto matrix=JSValue::object();matrix.set("elements",r.get("elements"));const auto name=r.get("name");
    const Args args=name.strictEquals("normalize")?Args{}:name.strictEquals("divideScalar")?Args{decode(r.get("scalar"))}:Args{matrix};try{out.set("result",value.get(name).call(value,args).sameValue(value));}catch(...){out.set("result",status(std::current_exception()));}out.set("fields",norm(value));return out;
}
JSValue run(JSValue r){const auto kind=r.get("kind");if(kind.strictEquals("math"))return math(r);if(kind.strictEquals("from"))return vectorFrom(r);if(kind.strictEquals("view"))return view(r);return vectorMethod(r);}
}
int main(){std::string line;while(std::getline(std::cin,line)){try{std::cout<<run(Animation_detail::parseJSON(line)).toJSON()->dump()<<'\n';}catch(const std::exception& e){auto out=JSValue::object();out.set("probeError",e.what());std::cout<<out.toJSON()->dump()<<'\n';}}}
