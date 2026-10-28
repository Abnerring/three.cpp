#pragma once
#include "animation/AnimationUtils.h"
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "core/JSPromise.h"
#include "core/NativeObject.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/StackNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <iterator>
#include <functional>
namespace StorageSkinningParity {
using Args=std::vector<JSValue>;
inline bool relative=false;
inline bool resourceUuids=false;
inline double baseline=0;
inline JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
inline JSValue call(const JSValue& self,const std::string& name,const Args& a={}){const auto f=self.get(name);return f.call(self,a);}
inline void own(JSValue o,const std::string& name,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(name,d);}
inline JSValue array(const Args& a={}){auto out=JSValue::array();for(std::size_t i=0;i<a.size();++i)own(out,std::to_string(i),a[i]);return out;}
inline void push(const JSValue& a,const JSValue& v){call(a,"push",{v});}
inline Args values(const JSValue& a){Args out;for(unsigned i=0;i<a.length();++i)out.push_back(a.get(std::to_string(i)));return out;}
inline JSValue normal(const JSValue& v,Args* seen=nullptr){
 if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";if(v.isSymbol())return "#symbol";
 if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "#NaN";if(!std::isfinite(n))return n>0?"#Infinity":"#-Infinity";if(n==0&&std::signbit(n))return "#-0";}
 if(!v.isObject())return v;if(v.get("fixtureTag").toBoolean())return "@"+v.get("fixtureTag").toString();
 Args local;if(!seen)seen=&local;for(std::size_t i=0;i<seen->size();++i)if(v.sameValue((*seen)[i]))return array({"#ref",double(i)});const auto index=double(seen->size());seen->push_back(v);
 auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();
 for(const auto& k:v.keys()){const auto field=v.get(k);if(relative&&k=="id"&&v.get("isNode").strictEquals(true))own(out,k,field.toNumber()>baseline?array({"#local",field.toNumber()-baseline}):array({"#existing",index}));else if(resourceUuids&&k=="uuid"&&(v.get("isTexture").strictEquals(true)||v.get("isTextureSource").strictEquals(true)))own(out,k,array({"#resourceUuid",index}));else own(out,k,normal(field,seen));}return out;
}
template<class F>JSValue attempt(F f){try{return normal(f());}catch(const JSThrownValue& e){auto out=JSValue::object();own(out,"error",e.value.get("name"));return out;}catch(const JSReferenceError&){auto out=JSValue::object();own(out,"error","ReferenceError");return out;}catch(const JSTypeError&){auto out=JSValue::object();own(out,"error","TypeError");return out;}catch(const JSRangeError&){auto out=JSValue::object();own(out,"error","RangeError");return out;}catch(const std::exception&){auto out=JSValue::object();own(out,"error","Error");return out;}}
inline JSValue state(const JSValue& r){auto s=JSValue::object();s.set("r",r);s.set("events",array());s.set("active",true);return s;}
inline void tick(JSValue s,const std::string& label,const Args& args={}){if(!s.get("active").toBoolean())return;Args row{label};for(const auto& a:args)row.push_back(normal(a));push(s.get("events"),array(row));if(s.get("r").get("throwAt").strictEquals(label))throw JSTypeError(label);}
inline void observe(JSValue s,JSValue o,const std::string& name,const std::string& label){const auto old=s.get("active");s.set("active",false);auto box=JSValue::object();box.set("value",o.get(name));s.set("active",old);JSPropertyDescriptor d;d.configurable=d.enumerable=true;
 d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{auto s=c[0];const auto label=c[2].toString();tick(s,"get "+label);if(s.get("active").toBoolean()&&s.get("r").get("reenter").strictEquals("get "+label)&&!s.get("entered").toBoolean()){s.set("entered",true);tick(s,"reenter",{call(s.get("node"),"getPBO")});}return c[1].get("value");},{s,box,label});
 d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"set "+c[2].toString(),{arg(a,0)});auto box=c[1];box.set("value",arg(a,0));return {};},{s,box,label});o.defineProperty(name,d);
}
inline JSValue decode(const JSValue& v,const JSValue& s){
 if(!v.isObject())return v;if(v.type()==JSValue::Type::Array){auto o=array();for(const auto& a:values(v))push(o,decode(a,s));return o;}
 const auto tag=v.get("tag");if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("NaN"))return std::nan("");if(tag.strictEquals("symbol"))return JSValue::symbol("fixture");
 if(tag.strictEquals("node")){auto n=nativeTSLBaseBinding(v.has("type")?v.get("type").toString():"float").call({}, {2});n.set("fixtureTag","input");return n;}
 if(tag.strictEquals("primitive")){auto n=JSValue::object();n.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"coerce "+c[2].toString(),{arg(a,0)});return c[1];},{s,v.get("value"),v.get("name")}));return n;}
 if(tag.strictEquals("typed"))return nativeGlobalBinding(v.has("type")?v.get("type").toString():"Float32Array").construct({decode(v.get("values"),s)});
 if(tag.strictEquals("constructor"))return nativeGlobalBinding(v.get("name").toString());
 auto n=JSValue::object();for(const auto& k:v.keys())own(n,k,decode(v.get(k),s));return n;
}
inline JSValue describe(const JSValue& o){auto result=array();for(const auto& key:o.ownPropertyNames()){const auto d=*o.getOwnPropertyDescriptor(key);const auto v=d.value.value_or(JSValue{});push(result,array({key,normal(v),v.isCallable()?v.get("name"):JSValue(nullptr),v.isCallable()?v.get("length"):JSValue(nullptr),v.isCallable()&&v.isConstructible(),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false),d.getter&&!d.getter->isUndefined()?d.getter->get("name"):JSValue(nullptr),d.setter&&!d.setter->isUndefined()?d.setter->get("name"):JSValue(nullptr)}));}return result;}
inline void restore(JSValue object,const std::string& name,const std::optional<JSPropertyDescriptor>& d){if(d)object.defineProperty(name,*d);else object.erase(name);}
inline JSValue functionSource(const JSValue& f){auto out=JSValue::object();own(out,"source",call(f,"toString"));own(out,"name",f.get("name"));own(out,"length",f.get("length"));own(out,"constructible",f.isConstructible());return out;}
inline JSValue builderObject(){auto material=JSValue::object(),object=JSValue::object(),renderer=JSValue::object();object.set("material",material);object.set("geometry",JSValue::object());renderer.set("coordinateSystem",2001);renderer.set("outputColorSpace","");auto debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);return nativeNodeBuilderConstructor().construct({object,renderer,nullptr});}
}
