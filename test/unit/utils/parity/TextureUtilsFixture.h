#pragma once
#include "extras/TextureUtils.h"
#include "core/NativeArray.h"
#include "animation/AnimationUtils.h"
#include <fstream>
#include <iostream>
#include <limits>
#include <cmath>
namespace TextureUtilsTest {
using Args=std::vector<JSValue>;using O=JsonValue::Object;using A=JsonValue::Array;
void property(JSValue target,const JSValue& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;target.defineProperty(key,d);}
JSValue argument(const Args& args,std::size_t n){return n<args.size()?args[n]:JSValue{};}
JsonValue encode(const JSValue& value){
 if(value.isUndefined())return "undefined";if(value.isNull())return nullptr;
 if(value.type()==JSValue::Type::String)return value.string();if(value.type()==JSValue::Type::Boolean)return value.toBoolean();
 if(value.type()==JSValue::Type::Number){const auto n=value.number();if(std::isnan(n))return "NaN";if(std::isinf(n))return n<0?"-Infinity":"Infinity";if(n==0&&std::signbit(n))return "-0";return n;}
 if(value.type()==JSValue::Type::Symbol)return "symbol";
 if(value.isCallable())return "function";
 if(value.type()==JSValue::Type::Array){A out;for(unsigned i=0;i<value.length();++i)out.push_back(encode(value.get(std::to_string(i))));return out;}
 O out;for(const auto& key:value.keys())out[key]=encode(value.get(key));return out;
}
void log(JSValue trace,const Args& row){auto values=JSValue::array();for(std::size_t i=0;i<row.size();++i)property(values,std::to_string(i),row[i]);trace.get("push").call(trace,{values});}
JSValue decode(const JSValue& value,const JSValue& trace);
JSValue convert(const JSValue&,const Args& args,const Args& captures){
 const auto trace=captures[0],spec=captures[1];log(trace,{spec.get("label"),"call",argument(args,0)});
 if(spec.get("gc").toBoolean())JSValue::object().collectGarbage();
 if(spec.get("throw").toBoolean())throw JSRangeError("conversion");
 return decode(spec.get("value"),trace);
}
JSValue methodGetter(const JSValue&,const Args&,const Args& captures){
 const auto trace=captures[0],spec=captures[1];log(trace,{spec.get("label"),"get"});
 if(spec.get("getThrow").toBoolean())throw JSRangeError("conversion getter");
 if(spec.hasOwn("methodValue"))return decode(spec.get("methodValue"),trace);
 return JSValue::function(convert,{trace,spec});
}
JSValue decode(const JSValue& value,const JSValue& trace){
 if(!value.isObject())return value;
 if(value.hasOwn("$")) {const auto name=value.get("$").string();if(name=="undefined")return {};if(name=="NaN")return std::numeric_limits<double>::quiet_NaN();if(name=="Infinity")return std::numeric_limits<double>::infinity();if(name=="-Infinity")return -std::numeric_limits<double>::infinity();if(name=="-0")return -0.;if(name=="symbol")return JSValue::symbol("offset");}
 if(value.hasOwn("$convert")){
  auto out=JSValue::object();JSPropertyDescriptor d;d.getter=JSValue::function(methodGetter,{trace,value.get("$convert")});d.configurable=d.enumerable=true;out.defineProperty(JSValue::wellKnownSymbol("toPrimitive"),d);return out;
 }
 if(value.hasOwn("$ordinary")){
  auto out=JSValue::object();const auto spec=value.get("$ordinary");for(const auto& key:spec.keys()){JSPropertyDescriptor d;d.getter=JSValue::function(methodGetter,{trace,spec.get(key)});d.configurable=d.enumerable=true;out.defineProperty(key,d);}return out;
 }
 auto out=value.type()==JSValue::Type::Array?JSValue::array():JSValue::object();for(const auto& key:value.keys())property(out,key,decode(value.get(key),trace));return out;
}
}
