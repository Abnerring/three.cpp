#pragma once
#include "renderers/webgl/WebGLExtensions.h"
#include "renderers/webgl/WebGLCapabilities.h"
#include "renderers/webgl/WebGLInfo.h"
#include "core/JSIterator.h"
#include "core/JSString.h"
#include "core/JSSymbol.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>

namespace WebGLCapabilitiesParity {
using Args=std::vector<JSValue>; using A=JsonValue::Array; using O=JsonValue::Object;
inline JSValue list(const Args& a){auto r=JSValue::array();for(std::size_t i=0;i<a.size();++i)r.set(std::to_string(i),a[i]);return r;}
inline void push(JSValue trace,const Args& a){trace.set(std::to_string(trace.length()),list(a));}
inline JSValue dict(){return JSValue::object(nullptr);}
inline JsonValue encode(const JSValue& v){
 if(v.isSymbol())return O{{"symbol",v.symbolDescription()?JsonValue(*v.symbolDescription()):JsonValue(nullptr)}};
 if(v.isUndefined())return "undefined";if(v.isCallable())return "function";if(v.isNull())return nullptr;
 if(v.type()==JSValue::Type::Boolean)return v.toBoolean();if(v.type()==JSValue::Type::String)return v.string();
 if(v.type()==JSValue::Type::Number){auto n=v.number();if(std::isnan(n))return "NaN";if(!std::isfinite(n))return n<0?"-Infinity":"Infinity";if(n==0&&std::signbit(n))return "-0";return n;}
 if(v.type()==JSValue::Type::Array){A a;for(std::uint32_t i=0;i<v.length();++i)a.push_back(encode(v.get(std::to_string(i))));return a;}
 O o;for(auto& k:v.keys())o[k]=encode(v.get(k));return o;
}
template<class F> JsonValue invoke(F&& f){try{return encode(f());}catch(const JSRangeError&){return O{{"error","RangeError"}};}catch(const JSTypeError&){return O{{"error","TypeError"}};}catch(const std::exception&){return O{{"error","Error"}};}}
inline void getter(JSValue object,const std::string& key,JSValue fn){JSPropertyDescriptor d;d.getter=fn;d.enumerable=true;d.configurable=true;object.defineProperty(key,d);}
inline void data(JSValue object,const std::string& key,JSValue value){JSPropertyDescriptor d;d.value=value;d.writable=true;d.enumerable=true;d.configurable=true;object.defineProperty(key,d);}
inline void console(JSValue trace,bool throws=false){WebGLInfo::setConsoleFunction(JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{Args line{"console",self.isUndefined()};line.insert(line.end(),a.begin(),a.end());push(c[0],line);if(c[1].toBoolean())throw JSRangeError("console");return {};},{trace,throws}));}
inline JSValue conversion(JSValue trace,const char* name,JSValue value,bool throws=false){auto v=JSValue::object();v.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{push(c[0],{"convert",c[1],a[0]});if(c[3].toBoolean())throw JSRangeError("conversion");return c[2];},{trace,name,value,throws}));return v;}
inline Args values(){auto o=JSValue::object();o.set("tag","value");return {{},nullptr,false,true,0,-0.0,1,-1,"","true","highp","mediump","lowp",std::numeric_limits<double>::quiet_NaN(),o};}
inline A names(const std::vector<std::string>& v){A a;for(auto& x:v)a.emplace_back(x);return a;}
inline JsonValue descriptor(const JSValue& o,const std::string& k){auto d=o.getOwnPropertyDescriptor(k);if(!d)return nullptr;return A{d->value.has_value(),d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false),d->getter.has_value(),d->setter.has_value()};}
inline JsonValue metadata(const JSValue& f){auto p=f.get("prototype");return O{{"name",encode(f.get("name"))},{"length",encode(f.get("length"))},{"names",names(f.ownPropertyNames())},{"nameFlags",descriptor(f,"name")},{"lengthFlags",descriptor(f,"length")},{"prototypeFlags",descriptor(f,"prototype")},{"prototypeNames",p.isObject()?JsonValue(names(p.ownPropertyNames())):JsonValue(nullptr)},{"constructorFlags",p.isObject()?descriptor(p,"constructor"):JsonValue(nullptr)},{"constructorIdentity",p.isObject()&&p.get("constructor").sameValue(f)}};}
inline JSValue call(const JSValue& o,const char* method,const Args& args={}){return o.get(method).call(o,args);}
inline const std::vector<std::string> limits{"MAX_TEXTURE_IMAGE_UNITS","MAX_VERTEX_TEXTURE_IMAGE_UNITS","MAX_TEXTURE_SIZE","MAX_CUBE_MAP_TEXTURE_SIZE","MAX_VERTEX_ATTRIBS","MAX_VERTEX_UNIFORM_VECTORS","MAX_VARYING_VECTORS","MAX_FRAGMENT_UNIFORM_VECTORS","MAX_SAMPLES","SAMPLES"};
inline void fail(const JSValue& state,const std::string& point){if(state.get("throw").strictEquals(point))throw JSRangeError(point);}
struct Fixture {
 JSValue trace=JSValue::array(),state=dict(),gl=JSValue::object(),extensions=JSValue::object(),parameters=JSValue::object(),utils=JSValue::object();
 Fixture(){
  console(trace);auto p=dict();for(std::size_t i=0;i<limits.size();++i)p.set(limits[i],double(i+1));p.set("IMPLEMENTATION_COLOR_READ_FORMAT","converted");p.set("IMPLEMENTATION_COLOR_READ_TYPE","converted");p.set("aniso",16);state.set("parameter",p);
  auto shaders=dict();for(auto stage:{"VERTEX_SHADER","FRAGMENT_SHADER"})for(auto precision:{"HIGH_FLOAT","MEDIUM_FLOAT"})shaders.set(std::string(stage)+":"+precision,17);state.set("shader",shaders);
  state.set("has",dict());state.set("get",dict());auto ext=JSValue::object();ext.set("MAX_TEXTURE_MAX_ANISOTROPY_EXT","aniso");state.get("get").set("EXT_texture_filter_anisotropic",ext);state.set("convert","converted");state.set("input",dict());state.set("reads",dict());
  auto constants=limits;for(auto k:{"VERTEX_SHADER","FRAGMENT_SHADER","HIGH_FLOAT","MEDIUM_FLOAT","IMPLEMENTATION_COLOR_READ_FORMAT","IMPLEMENTATION_COLOR_READ_TYPE"})constants.emplace_back(k);
  for(auto& key:constants)getter(gl,key,JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],{"constant",c[1],self.sameValue(c[2])});fail(c[3],"constant:"+c[1].string());return c[1];},{trace,key,gl,state}));
  auto parameter=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],{"parameter",self.sameValue(c[1]),a[0]});fail(c[2],"parameter:"+a[0].toString());return c[2].get("parameter").get(a[0].toString());},{trace,gl,state});
  auto shader=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],{"shader",self.sameValue(c[1]),a[0],a[1]});auto key=a[0].toString()+":"+a[1].toString();fail(c[2],"shader:"+key);auto out=JSValue::object();getter(out,"precision",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],{"precision",c[1]});fail(c[2],"precision:"+c[1].string());return c[2].get("shader").get(c[1].string());},{c[0],key,c[2]}));return out;},{trace,gl,state});
  method(gl,"getParameter",parameter);method(gl,"getShaderPrecisionFormat",shader);
  for(auto key:{"has","get"}){auto f=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],{c[1],self.sameValue(c[2]),a[0]});fail(c[3],c[1].string()+":"+a[0].toString());auto table=c[3].get(c[1].string());return table.hasOwn(a[0].toString())?table.get(a[0].toString()):c[1].strictEquals("has")?JSValue(false):JSValue();},{trace,key,extensions,state});method(extensions,key,f);}
  method(utils,"convert",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],{"utils",self.sameValue(c[1]),a[0]});fail(c[2],"convert");return c[2].get("convert");},{trace,utils,state}));
  for(auto key:{"precision","logarithmicDepthBuffer","reversedDepthBuffer"})getter(parameters,key,JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{auto reads=c[3].get("reads");auto prior=reads.get(c[1].string());auto n=prior.isUndefined()?0:prior.number();reads.set(c[1].string(),n+1);push(c[0],{"input",c[1],n,self.sameValue(c[2])});fail(c[3],"input:"+c[1].string());auto value=c[3].get("input").get(c[1].string());return value.type()==JSValue::Type::Array?value.get(std::to_string(std::min(std::uint32_t(n),value.length()-1))):value;},{trace,key,parameters,state}));
 }
 void method(JSValue object,const char* name,JSValue fn){getter(object,name,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],{"method",c[1]});fail(c[3],"method:"+c[1].string());return c[2];},{trace,name,fn,state}));}
 JSValue make(){return nativeWebGLCapabilitiesConstructor().call(nullptr,{gl,extensions,parameters,utils});}
};
struct PrototypeRestore {JSValue object;std::vector<std::pair<std::string,std::optional<JSPropertyDescriptor>>> saved;explicit PrototypeRestore(JSValue o):object(o){}void save(const std::string& key){saved.emplace_back(key,object.getOwnPropertyDescriptor(key));}~PrototypeRestore(){for(auto i=saved.rbegin();i!=saved.rend();++i)if(i->second)object.defineProperty(i->first,*i->second);else object.erase(i->first);}};
inline JSValue extensionGL(JSValue trace,JSValue result){auto gl=JSValue::object();getter(gl,"getExtension",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],{"method","getExtension"});return c[1];},{trace,JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],{"extension",self.sameValue(c[1]),a.empty()?JSValue():a[0]});return c[2];},{trace,gl,result})}));return gl;}
inline JSValue extensions(JSValue gl){return nativeWebGLExtensionsConstructor().call(nullptr,{gl});}
}
