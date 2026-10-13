#pragma once
#include "renderers/webgl/WebGLUtils.h"
#include "math/ColorManagement.h"
#include "WebGLUtilsTables.h"
#include <cmath>
#include <iostream>
#include <limits>
namespace WebGLUtilsParity {
using Args=std::vector<JSValue>;using A=JsonValue::Array;using O=JsonValue::Object;
inline JSValue dict(){return JSValue::object(nullptr);}
inline JSValue list(const Args& a){auto r=JSValue::array();for(std::size_t i=0;i<a.size();++i)r.set(std::to_string(i),a[i]);return r;}
inline void push(JSValue trace,const Args& a){trace.set(std::to_string(trace.length()),list(a));}
inline JsonValue encode(const JSValue& v){if(v.isSymbol())return O{{"symbol",v.symbolDescription()?JsonValue(*v.symbolDescription()):JsonValue(nullptr)}};if(v.isUndefined())return "undefined";if(v.isNull())return nullptr;if(v.isCallable())return "function";if(v.type()==JSValue::Type::Boolean)return v.toBoolean();if(v.type()==JSValue::Type::String)return v.string();if(v.type()==JSValue::Type::Number){auto n=v.number();if(std::isnan(n))return "NaN";if(!std::isfinite(n))return n<0?"-Infinity":"Infinity";if(n==0&&std::signbit(n))return "-0";return n;}if(v.type()==JSValue::Type::Array){A a;for(std::uint32_t i=0;i<v.length();++i)a.push_back(encode(v.get(std::to_string(i))));return a;}O o;for(auto& k:v.keys())o[k]=encode(v.get(k));return o;}
template<class F> JsonValue invoke(F&& f){try{return encode(f());}catch(const JSRangeError&){return O{{"error","RangeError"}};}catch(const JSTypeError&){return O{{"error","TypeError"}};}catch(const std::exception&){return O{{"error","Error"}};}}
inline void getter(JSValue o,const JSValue& key,JSValue fn){JSPropertyDescriptor d;d.getter=fn;d.enumerable=true;d.configurable=true;o.defineProperty(key,d);}
inline void data(JSValue o,const JSValue& key,JSValue value){JSPropertyDescriptor d;d.value=value;d.writable=true;d.enumerable=true;d.configurable=true;o.defineProperty(key,d);}
inline Args values(){auto object=JSValue::object();object.set("tag","value");return {{},nullptr,false,true,0,-0.0,1,-1,"","srgb","linear",std::numeric_limits<double>::quiet_NaN(),object,JSValue::symbol("value"),JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};})};}
inline A names(const std::vector<std::string>& input){A out;for(auto& v:input)out.emplace_back(v);return out;}
inline JsonValue descriptor(const JSValue& o,const char* k){auto d=o.getOwnPropertyDescriptor(k);if(!d)return nullptr;return A{d->value.has_value(),d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false),d->getter.has_value(),d->setter.has_value()};}
inline JsonValue metadata(const JSValue& f){auto p=f.get("prototype");return O{{"name",encode(f.get("name"))},{"length",encode(f.get("length"))},{"names",names(f.ownPropertyNames())},{"nameFlags",descriptor(f,"name")},{"lengthFlags",descriptor(f,"length")},{"prototypeFlags",descriptor(f,"prototype")},{"prototypeNames",p.isObject()?JsonValue(names(p.ownPropertyNames())):JsonValue(nullptr)},{"constructorFlags",p.isObject()?descriptor(p,"constructor"):JsonValue(nullptr)},{"constructorIdentity",p.isObject()&&p.get("constructor").sameValue(f)}};}
inline JSValue conversion(JSValue trace,const char* label,JSValue result,bool throws=false){auto object=JSValue::object();object.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{push(c[0],{"coerce",c[1],a[0]});if(c[3].toBoolean())throw JSRangeError("coerce");return c[2];},{trace,label,result,throws}));return object;}
struct Restore {JSValue object;std::vector<std::pair<JSValue,std::optional<JSPropertyDescriptor>>> saved;explicit Restore(JSValue o):object(o){}void save(JSValue key){saved.emplace_back(key,object.getOwnPropertyDescriptor(key));}~Restore(){for(auto i=saved.rbegin();i!=saved.rend();++i)if(i->second)object.defineProperty(i->first,*i->second);else object.erase(i->first);}};
inline void fail(JSValue state,JSValue point){if(state.get("throw").strictEquals(point))throw JSRangeError("fixture");}
struct Fixture {
 JSValue trace=JSValue::array(),cm=nativeColorManagement(),state=dict(),gl=JSValue::object(),extension=JSValue::object(),extensions=JSValue::object();
 Restore restore{cm};
 Fixture(){restore.save("getTransfer");auto original=cm.get("getTransfer");auto transfer=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],{"transfer",self.sameValue(c[1]),a[0]});fail(c[2],"transfer");if(c[2].hasOwn("transfer"))return c[2].get("transfer");return c[3].call(self,a);},{trace,cm,state,original});getter(cm,"getTransfer",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],{"transfer-get",self.sameValue(c[1])});fail(c[2],"transfer-get");return c[2].get("badTransfer").toBoolean()?JSValue(17):c[3];},{trace,cm,state,transfer}));
  for(auto& key:glNames)field(gl,key,"gl");for(auto& key:extensionNames)field(extension,key,"extension-field");
  auto get=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],{"extension",self.sameValue(c[1]),a[0]});fail(c[2],"extension");return c[2].hasOwn("extension")?c[2].get("extension"):c[3];},{trace,extensions,state,extension});getter(extensions,"get",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],{"extension-get",self.sameValue(c[1])});fail(c[2],"extension-get");return c[2].get("badExtension").toBoolean()?JSValue(17):c[3];},{trace,extensions,state,get}));
 }
 void field(JSValue object,const std::string& key,const char* label){getter(object,key,JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],{c[1],c[2],self.sameValue(c[3])});fail(c[4],c[1]);if(c[4].hasOwn("return"))return c[4].get("return");return c[2];},{trace,label,key,object,state}));}
 JSValue make(){return nativeWebGLUtilsConstructor().call(nullptr,{gl,extensions});}
};
inline JSValue call(JSValue object,const Args& args){return object.get("convert").call(object,args);}
inline const Args colorSpaces{NoColorSpace,LinearSRGBColorSpace,SRGBColorSpace};
}
