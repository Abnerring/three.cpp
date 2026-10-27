#include "animation/AnimationUtils.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "math/Matrix3.h"
#include "math/ColorManagement.h"
#include <cmath>
#include <iostream>
#include <limits>
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue call(const JSValue& o,const char* k,const Args& a={}){return o.get(k).call(o,a);}
JSValue array(const Args& a={}){auto o=JSValue::array();for(std::size_t i=0;i<a.size();++i)own(o,std::to_string(i),a[i]);return o;}
JSValue normal(const JSValue& v){if(v.isUndefined())return "#undefined";if(v.isSymbol())return "#symbol";if(v.isCallable())return "#function";if(v.type()==JSValue::Type::Number){if(std::isnan(v.number()))return "#NaN";if(std::isinf(v.number()))return v.number()>0?"#Infinity":"#-Infinity";if(v.number()==0&&std::signbit(v.number()))return "#-0";}if(!v.isObject())return v;auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();for(const auto& k:v.keys())own(out,k,normal(v.get(k)));return out;}
void event(const JSValue& s,const Args& a){call(s.get("events"),"push",{array(a)});}
JSValue failure(std::exception_ptr p){try{std::rethrow_exception(p);}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception& e){auto o=JSValue::object();o.set("nativeError",e.what());return o;}}
JSValue decode(const JSValue& v,const JSValue& s){if(!v.isObject())return v;const auto tag=v.get("tag");if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();if(tag.strictEquals("Infinity"))return std::numeric_limits<double>::infinity();if(tag.strictEquals("-Infinity"))return -std::numeric_limits<double>::infinity();if(tag.strictEquals("-0"))return -0.0;if(tag.strictEquals("symbol"))return JSValue::symbol("fixture");
    if(tag.strictEquals("coerce")){auto o=JSValue::object();JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        event(c[0],{"coerce.get",c[1].get("label")});if(c[1].get("throw").strictEquals("get"))throw JSTypeError("get");return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{auto s=c[0];const auto count=s.get("coercions").toNumber()+1;s.set("coercions",count);event(s,{"coerce",c[1].get("label"),arg(a,0),count});if(c[1].get("throw").strictEquals("call")||s.get("request").get("throwCoerceAt").strictEquals(count))throw JSTypeError("coerce");return decode(c[1].get("value"),s);},c,"",1);
    },{s,v});o.defineProperty(JSValue::wellKnownSymbol("toPrimitive"),d);return o;}
    if(v.type()==JSValue::Type::Array){auto o=array();for(unsigned i=0;i<v.length();++i)call(o,"push",{decode(v.get(std::to_string(i)),s)});return o;}return v;
}
JSValue state(const JSValue& r){auto s=JSValue::object();s.set("request",r);s.set("events",array());s.set("coercions",0);return s;}
JSValue repeat(const JSValue& r){const auto s=state(r);auto out=JSValue::object();try{const auto self=decode(r.get("self"),s),count=decode(r.get("count"),s);const auto value=nativeStringPrototype().get("repeat").call(self,{count});const auto text=jsUTF16(value.string());auto units=array();for(const auto unit:text)call(units,"push",{static_cast<unsigned>(unit)});out.set("units",units);out.set("length",text.size());}catch(...){out.set("error",failure(std::current_exception()));}out.set("events",s.get("events"));return out;}
struct MatrixFixture {JSValue object,backing;};
MatrixFixture matrix(const JSValue& r,const JSValue& s,const char* label,const JSValue& supplied){auto object=nativeMatrix3Constructor().construct(),backing=array();for(unsigned i=0;i<9;++i){const auto v=supplied.get(std::to_string(i));JSValue value=decode(v,s);if(r.get("coerce").toBoolean()){auto enc=JSValue::object();enc.set("tag","coerce");enc.set("label",std::string(label)+"."+std::to_string(i));enc.set("value",v);value=decode(enc,s);}call(backing,"push",{value});}
    auto elements=backing;
    if(r.get("observed").toBoolean()){elements=JSValue::array(9);for(unsigned i=0;i<9;++i){JSPropertyDescriptor d;d.configurable=d.enumerable=true;const auto key=std::to_string(i),name=std::string(label)+"."+key;
        d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[2]});if(c[0].get("request").get("throwAt").strictEquals("get:"+c[2].toString()))throw JSTypeError("get");return c[1].get(c[3]);},{s,backing,name,key});
        d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{"set",c[2],normal(arg(a,0))});if(c[0].get("request").get("throwAt").strictEquals("set:"+c[2].toString()))throw JSTypeError("set");auto b=c[1];b.set(c[3],arg(a,0));return {};},{s,backing,name,key});elements.defineProperty(key,d);
    }JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"elements",c[2]});if(c[0].get("request").get("throwAt").strictEquals("elements:"+c[2].toString()))throw JSTypeError("elements");return c[1];},{s,elements,label});object.defineProperty("elements",d);
    }else object.set("elements",elements);return {object,backing};
}
JSValue matrixCase(const JSValue& r){const auto s=state(r);auto a=matrix(r,s,"A",r.get("a")),b=matrix(r,s,"B",r.get("b")),target=matrix(r,s,"T",r.get("t"));if(r.get("alias").strictEquals("a"))target=a;if(r.get("alias").strictEquals("b"))target=b;if(r.get("alias").strictEquals("both")){target=a;b=a;}
    if(r.get("delegate").toBoolean()){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"method",self.sameValue(c[1])});if(c[0].get("request").get("throwAt").strictEquals("method"))throw JSTypeError("method");return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{event(c[0],{"delegate",self.sameValue(c[1]),arg(a,0).sameValue(c[1]),arg(a,1).sameValue(c[1])});return c[0].get("request").get("returnSelf").toBoolean()?self:JSValue(17);},c);},{s,target.object});target.object.defineProperty("multiplyMatrices",d);}
    auto out=JSValue::object();try{const auto op=r.get("operation").string();const auto result=call(target.object,op.c_str(),op=="multiplyMatrices"?Args{a.object,b.object}:op=="multiply"?Args{b.object}:Args{a.object});out.set("sameSelf",result.sameValue(target.object));if(!result.sameValue(target.object))out.set("result",normal(result));}catch(...){out.set("error",failure(std::current_exception()));}
    out.set("target",normal(target.backing));out.set("a",normal(a.backing));out.set("b",normal(b.backing));out.set("events",s.get("events"));return out;
}
JSValue color(){const auto cm=nativeColorManagement(),spaces=cm.get("spaces"),linear=spaces.get("srgb-linear"),srgb=spaces.get("srgb"),proto=nativeMatrix3Constructor().get("prototype");auto out=JSValue::object();for(const auto* key:{"toXYZ","fromXYZ"}){const auto m=linear.get(key);auto item=JSValue::object();item.set("prototype",m.getPrototype().sameValue(proto));item.set("flag",m.get("isMatrix3"));item.set("sameAcrossSpaces",m.sameValue(srgb.get(key)));item.set("elements",normal(m.get("elements")));auto methods=array();for(const auto* name:{"multiply","premultiply","multiplyMatrices"})call(methods,"push",{m.get(name).sameValue(proto.get(name))});item.set("methods",methods);out.set(key,item);}const auto target=nativeMatrix3Constructor().construct();const auto value=call(cm,"_getMatrix",{target,"srgb-linear","srgb"});out.set("getMatrixSelf",value.sameValue(target));out.set("getMatrixElements",normal(target.get("elements")));return out;}
}
int main(){try{const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto cases=Animation_detail::parseJSON(input);auto out=array();for(unsigned i=0;i<cases.length();++i){const auto r=cases.get(std::to_string(i));call(out,"push",{r.get("kind").strictEquals("repeat")?repeat(r):r.get("kind").strictEquals("matrix")?matrixCase(r):color()});}std::cout<<out.toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
