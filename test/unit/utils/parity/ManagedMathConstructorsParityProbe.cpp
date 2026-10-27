#include "animation/AnimationUtils.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/Color.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "core/JSPromise.h"
#include "core/JSRegExp.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,unsigned i){return i<a.size()?a[i]:JSValue{};}
void data(JSValue o,const JSValue& k,JSValue v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue array(const Args& a={}){auto o=JSValue::array();for(const auto& v:a)data(o,std::to_string(o.length()),v);return o;}
JSValue names(const JSValue& o){Args a;for(const auto& k:o.ownPropertyNames())a.push_back(k);return array(a);}
JSValue decode(const JSValue& v){
    if(!v.isObject())return v;const auto t=v.get("tag");if(t.strictEquals("undefined"))return {};if(t.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();if(t.strictEquals("Infinity"))return std::numeric_limits<double>::infinity();if(t.strictEquals("-Infinity"))return -std::numeric_limits<double>::infinity();if(t.strictEquals("-0"))return -0.0;
    if(v.type()==JSValue::Type::Array){auto o=JSValue::array();for(unsigned i=0;i<v.length();++i)data(o,std::to_string(i),decode(v.get(std::to_string(i))));return o;}
    return v;
}
JSValue normal(const JSValue& v){
    if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";if(v.isSymbol())return "#symbol";
    if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "#NaN";if(std::isinf(n))return n>0?"#Infinity":"#-Infinity";if(n==0&&std::signbit(n))return "#-0";}
    if(!v.isObject())return v;if(v.get("fixtureTag").toBoolean())return "@"+v.get("fixtureTag").toString();
    auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();if(v.type()==JSValue::Type::Array)for(unsigned i=0;i<v.length();++i)if(!v.hasOwn(std::to_string(i)))data(out,std::to_string(i),"#hole");
    for(const auto& k:v.keys())data(out,k,normal(v.get(k)));return out;
}
void event(JSValue s,const Args& a){auto e=s.get("events");e.get("push").call(e,{array(a)});}
JSValue constructor(const JSValue& name){if(name.strictEquals("Matrix3"))return nativeMatrix3Constructor();if(name.strictEquals("Matrix4"))return nativeMatrix4Constructor();if(name.strictEquals("Color"))return nativeColorConstructor();if(name.strictEquals("Vector2"))return nativeVector2Constructor();if(name.strictEquals("Vector3"))return nativeVector3Constructor();return nativeVector4Constructor();}
Args values(const JSValue& v){Args a;for(unsigned i=0;i<v.length();++i)a.push_back(decode(v.get(std::to_string(i))));return a;}
JSValue error(const std::exception& e){if(dynamic_cast<const JSTypeError*>(&e))return "#TypeError";if(dynamic_cast<const JSRangeError*>(&e))return "#RangeError";return "#Error";}
void console(JSValue s){ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{Args v{"console"};for(const auto& x:a)v.push_back(normal(x));event(c[0],v);return {};},{s},"console",0));}
JSValue basic(const JSValue& r){
    auto out=JSValue::object(),s=JSValue::object();s.set("events",array());console(s);const auto C=constructor(r.get("class"));const auto args=values(r.get("args"));auto v=C.construct(args);
    out.set("result",normal(v));out.set("own",names(v));out.set("prototype",v.getPrototype().sameValue(C.get("prototype")));out.set("metadata",array({C.get("name"),C.get("length"),v.get("constructor").sameValue(C)}));
    try{C.call({},args);out.set("call","ok");}catch(const std::exception& e){out.set("call",error(e));}
    out.set("stable",constructor(r.get("class")).sameValue(C));out.set("events",s.get("events"));ThreeUtils::setConsoleFunction({});return out;
}
JSValue newTarget(const JSValue& r){
    auto s=JSValue::object();s.set("events",array());s.set("request",r);auto C=constructor(r.get("class")),prototype=JSValue::object(C.get("prototype"));auto target=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Target",0,JSValue::ConstructorKind::Callable);target.set("prototype",prototype);
    const auto fields=r.get("class").strictEquals("Color")?Args{"isColor","r","g","b"}:Args{"elements"};
    for(const auto& field:fields){JSPropertyDescriptor d;d.configurable=true;d.setter=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{auto state=c[0];state.set("instance",self);event(state,{"write",c[1],normal(arg(a,0))});if(state.get("request").get("throwWrite").strictEquals(c[1]))throw JSTypeError("write");data(self,c[1],arg(a,0));return {};},{s,field},"set",1);prototype.defineProperty(field,d);}
    JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"getSet",names(self)});if(c[0].get("request").get("throwGet").toBoolean())throw JSTypeError("get");return JSValue::function(+[](const JSValue& self,const Args& a,const Args& cap)->JSValue{event(cap[0],{"callSet",normal(array(a)),self.sameValue(cap[0].get("instance"))});if(cap[0].get("request").get("throwCall").toBoolean())throw JSTypeError("call");if(cap[0].get("request").get("replacement").toBoolean()){auto o=JSValue::object();o.set("fixtureTag","replacement");return o;}return 7;},{c[0]},"set",0);},{s},"get set",0);prototype.defineProperty("set",d);
    auto out=JSValue::object();try{const auto v=C.construct(values(r.get("args")),target);out.set("result",normal(v));out.set("prototype",v.getPrototype().sameValue(prototype));}catch(const std::exception& e){out.set("result",error(e));}
    const auto instance=s.get("instance");out.set("partial",instance.isUndefined()?JSValue("#undefined"):normal(instance));out.set("events",s.get("events"));return out;
}
JSValue observed(JSValue s,const char* label,JSValue storage){
    auto handler=JSValue::object();
    data(handler,"get",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{c[1].toString()+".get",normal(arg(a,1))});if(c[0].get("request").get("throwRead").strictEquals(arg(a,1)))throw JSTypeError("read");return arg(a,0).get(arg(a,1));},{s,label},"get",3));
    data(handler,"set",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{c[1].toString()+".set",normal(arg(a,1)),normal(arg(a,2))});if(c[0].get("request").get("throwWrite").strictEquals(arg(a,1)))throw JSTypeError("write");auto target=arg(a,0);target.set(arg(a,1),arg(a,2));return true;},{s,label},"set",4));
    return JSValue::proxy(storage,handler);
}
JSValue matrixMethod(const JSValue& r){
    auto s=JSValue::object();s.set("events",array());s.set("request",r);const auto C=constructor(r.get("class"));const unsigned count=r.get("class").strictEquals("Matrix3")?9:16;
    auto targetValues=JSValue::object(),sourceValues=JSValue::object();for(unsigned i=0;i<count+5;++i){data(targetValues,std::to_string(i),i+1);data(sourceValues,std::to_string(i),i+1);data(sourceValues,std::to_string(i)+"0",i+101);}if(r.has("different"))sourceValues.set(r.get("different"),false);
    const auto target=observed(s,"target",targetValues),source=observed(s,"source",sourceValues);s.set("target",target);s.set("source",source);
    auto self=JSValue::object(),other=JSValue::object();for(const auto& pair:std::initializer_list<std::pair<JSValue,const char*>>{{self,"target"},{other,"source"}}){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{c[1].toString()+".elements"});return c[0].get(c[1]);},{s,pair.second},"get elements",0);auto object=pair.first;object.defineProperty("elements",d);}
    auto offset=JSValue::object();offset.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{"offset",arg(a,0)});return decode(c[0].get("request").get("offset"));},{s},"primitive",1));
    const auto method=r.get("method");Args args;
    if(method.strictEquals("set"))for(unsigned i=0;i<count;++i)args.push_back(i%3==0?JSValue(false):i%3==1?JSValue(std::to_string(i)):JSValue{});
    else if(method.strictEquals("copy")||method.strictEquals("equals"))args={other};else args={method.strictEquals("toArray")?target:source,offset};
    auto out=JSValue::object();try{const auto v=C.get("prototype").get(method).call(self,args);out.set("result",v.sameValue(self)?JSValue("@self"):v.sameValue(target)?JSValue("@target"):normal(v));}catch(const std::exception& e){out.set("result",error(e));}
    out.set("remaining",normal(targetValues));out.set("events",s.get("events"));return out;
}
JSValue clone(const JSValue& r){
    auto s=JSValue::object();s.set("events",array());const auto C=constructor(r.get("class"));auto self=JSValue::object();s.set("self",self);const auto replacement=JSValue::object();auto returned=replacement;returned.set("fixtureTag","cloneResult");s.set("returned",returned);
    auto fixture=JSValue::object();JSPropertyDescriptor fd;fd.configurable=true;fd.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"getFromArray"});return JSValue::function(+[](const JSValue&,const Args& a,const Args& cap)->JSValue{event(cap[0],{"fromArray",normal(arg(a,0))});return cap[0].get("returned");},{c[0]},"fromArray",1);},{s},"get fromArray",0);fixture.defineProperty("fromArray",fd);s.set("fixture",fixture);
    auto target=JSValue::builtinConstructor(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{"construct",normal(array(a))});return c[0].get("fixture");},{s},"Fixture",0);s.set("constructor",target);
    JSPropertyDescriptor cd;cd.configurable=true;cd.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"getConstructor"});return c[0].get("constructor");},{s},"get constructor",0);self.defineProperty("constructor",cd);
    for(const auto* key:{"elements","r","g","b"}){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[1]});return c[1].strictEquals("elements")?array({1,2,3}):JSValue(c[1].toString());},{s,key},"get",0);self.defineProperty(key,d);}
    const auto prototype=C.get("prototype");const auto original=prototype.getOwnPropertyDescriptor("fromArray");auto p=prototype;
    if(r.get("class").strictEquals("Matrix4"))p.defineProperty("fromArray",fd);
    auto out=JSValue::object();try{out.set("result",normal(C.get("prototype").get("clone").call(self)));}catch(const std::exception& e){out.set("result",error(e));}
    if(r.get("class").strictEquals("Matrix4"))p.defineProperty("fromArray",*original);
    out.set("events",s.get("events"));return out;
}
JSValue colorMethod(const JSValue& r){
    auto s=JSValue::object();s.set("events",array());s.set("request",r);ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{Args v{"console"};for(const auto& x:a)v.push_back(normal(x));event(c[0],v);return {};},{s},"console",0));
    auto value=nativeColorConstructor().construct();const auto args=values(r.get("args"));auto out=JSValue::object();const auto fn=value.get(r.get("method"));try{const auto v=fn.call(value,args);out.set("identity",v.sameValue(value));out.set("result",normal(v));}catch(const std::exception& e){out.set("result",error(e));}out.set("remaining",normal(value));out.set("events",s.get("events"));ThreeUtils::setConsoleFunction({});return out;
}
JSValue capture(JSValue s,JSValue key){
    auto object=JSValue::object();object.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{"primitive",c[1],arg(a,0)});if(c[0].get("request").get("throwPrimitive").strictEquals(c[1]))throw JSTypeError("primitive");return decode(c[0].get("request").get("values").get(c[1]));},{s,key},"primitive",1));return object;
}
JSValue styleExec(const JSValue& r){
    auto s=JSValue::object();s.set("events",array());s.set("request",r);s.set("position",0);console(s);auto self=nativeColorConstructor().construct();
    for(const auto* key:{"setRGB","setHSL","setHex"}){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"method.get",c[1]});return JSValue::function(+[](const JSValue&,const Args& a,const Args& cap)->JSValue{event(cap[0],{"method.call",cap[1],normal(array(a))});auto result=JSValue::object();result.set("fixtureTag","methodResult");return result;},{c[0],c[1]},c[1].toString(),0);},{s,key},"get",0);self.defineProperty(key,d);}
    auto p=nativeRegExpPrototype();const auto original=p.getOwnPropertyDescriptor("exec").value();auto d=original;d.value=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{
        auto state=c[0];const auto i=state.get("position").number();state.set("position",i+1);event(state,{"exec",i,normal(arg(a,0))});const auto mode=state.get("request").get("mode");
        if(i==0){if(mode.strictEquals("hex3")||mode.strictEquals("hex6"))return nullptr;auto model=JSValue::object();model.set("1",mode.strictEquals("hsl")?JSValue("hsl"):JSValue("rgb"));model.set("2","components");return model;}
        if(i==1&&mode.strictEquals("percent"))return nullptr;
        auto match=JSValue::object();if(mode.strictEquals("hex3")||mode.strictEquals("hex6")){
            auto hex=capture(state,"0");hex.set("length",mode.strictEquals("hex3")?3:6);JSPropertyDescriptor charAt;charAt.configurable=true;charAt.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{event(cap[0],{"charAt.get"});return JSValue::function(+[](const JSValue&,const Args& a,const Args& cap)->JSValue{event(cap[0],{"charAt.call",arg(a,0)});return capture(cap[0],arg(a,0).toString());},{cap[0]},"charAt",1);},{state},"get charAt",0);hex.defineProperty("charAt",charAt);match.set("1",hex);return match;
        }
        for(const auto* key:{"1","2","3","4"}){JSPropertyDescriptor item;item.configurable=true;item.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{event(cap[0],{"capture.get",cap[1]});if(cap[0].get("request").get("throwCapture").strictEquals(cap[1]))throw JSTypeError("capture");return capture(cap[0],cap[1]);},{state,key},"get",0);match.defineProperty(key,item);}return match;
    },{s},"exec",1);p.defineProperty("exec",d);auto out=JSValue::object();try{out.set("result",normal(self.get("setStyle").call(self, {"fixture"})));}catch(const std::exception& e){out.set("result",error(e));}p.defineProperty("exec",original);out.set("events",s.get("events"));ThreeUtils::setConsoleFunction({});return out;
}
JSValue colorArray(const JSValue& r){
    auto s=JSValue::object();s.set("events",array());s.set("request",r);auto fields=JSValue::object(),items=JSValue::object();fields.set("r",false);fields.set("g","raw");fields.set("b",JSValue{});for(unsigned i=0;i<5;++i){data(items,std::to_string(i),i+10);data(items,"0"+std::to_string(i),i+20);}
    const auto self=observed(s,"color",fields),input=observed(s,"array",items);auto offset=JSValue::object();offset.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{"offset",arg(a,0)});if(c[0].get("request").get("throwOffset").toBoolean())throw JSTypeError("offset");return decode(c[0].get("request").get("offset"));},{s},"primitive",1));
    auto out=JSValue::object();try{const auto result=nativeColorConstructor().get("prototype").get(r.get("method")).call(self,{input,offset});out.set("result",result.sameValue(self)?JSValue("@self"):result.sameValue(input)?JSValue("@array"):normal(result));}catch(const std::exception& e){out.set("result",error(e));}out.set("fields",normal(fields));out.set("items",normal(items));out.set("events",s.get("events"));return out;
}
}
int main(){std::string line;while(std::getline(std::cin,line)){try{const auto r=Animation_detail::parseJSON(line),kind=r.get("kind");JSValue out;if(kind.strictEquals("basic"))out=basic(r);else if(kind.strictEquals("newTarget"))out=newTarget(r);else if(kind.strictEquals("matrixMethod"))out=matrixMethod(r);else if(kind.strictEquals("clone"))out=clone(r);else if(kind.strictEquals("styleExec"))out=styleExec(r);else if(kind.strictEquals("colorArray"))out=colorArray(r);else out=colorMethod(r);std::cout<<out.toJSON()->dump()<<'\n';}catch(const std::exception& e){auto out=JSValue::object();out.set("probeError",e.what());std::cout<<out.toJSON()->dump()<<'\n';}}}
