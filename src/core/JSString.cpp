#include "core/JSString.h"
#include "core/JSIterator.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
using Args=std::vector<JSValue>;
struct StringData { std::string value; };
JSValue argument(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue();}
JSPropertyDescriptor data(JSValue value,bool writable=false,bool enumerable=false,bool configurable=false){
    JSPropertyDescriptor d;d.value=std::move(value);d.writable=writable;d.enumerable=enumerable;d.configurable=configurable;return d;
}
std::vector<std::uint16_t> units(const std::string& value){
    // JSValue strings have already been validated and normalized as UTF-8/WTF-8.
    std::vector<std::uint16_t> result;
    for(std::size_t i=0;i<value.size();){
        const auto first=static_cast<unsigned char>(value[i++]);
        unsigned count=first<0x80?0:first<0xe0?1:first<0xf0?2:3;
        std::uint32_t point=count==0?first:first&(0x7f>>count);
        for(unsigned j=0;j<count;++j)point=(point<<6)|(static_cast<unsigned char>(value[i++])&0x3f);
        if(point<=0xffff)result.push_back(static_cast<std::uint16_t>(point));
        else{point-=0x10000;result.push_back(static_cast<std::uint16_t>(0xd800+(point>>10)));result.push_back(static_cast<std::uint16_t>(0xdc00+(point&0x3ff)));}
    }
    return result;
}
JSValue stringValue(const JSValue& self,const Args&,const Args&){return nativeStringValue(self);}
JSValue startsWith(const JSValue& self,const Args& args,const Args&){
    if(self.isNull()||self.isUndefined())throw JSTypeError("String.prototype.startsWith called on null or undefined");
    const auto text=self.toString();
    const auto search=argument(args,0);
    if(search.isObject()){
        const auto matcher=search.get(JSValue::wellKnownSymbol("match"));
        if(!matcher.isUndefined()&&matcher.toBoolean())throw JSTypeError("String.prototype.startsWith does not accept a regular expression");
        // No native RegExp objects exist yet. Supported objects without @@match
        // have no RegExpMatcher internal slot and therefore are not RegExp.
    }
    const auto needle=search.toString();
    const auto position=argument(args,1).toNumber();
    const auto stringUnits=units(text),searchUnits=units(needle);
    const double integer=std::isnan(position)||position==0?0:std::trunc(position);
    const auto start=static_cast<std::size_t>(std::min(std::max(integer,0.0),double(stringUnits.size())));
    if(searchUnits.size()>stringUnits.size()-start)return false;
    return std::equal(searchUnits.begin(),searchUnits.end(),stringUnits.begin()+start);
}
JSValue allocate(const JSValue& text){
    if(text.type()!=JSValue::Type::String)throw JSTypeError("StringData requires a primitive string");
    auto payload=std::make_shared<StringData>();payload->value=text.string();
    auto result=JSValue::hostObject(payload,payload.get(),"three.String");
    const auto length=static_cast<std::uint32_t>(text.get("length").number());
    for(std::uint32_t i=0;i<length;++i)result.defineProperty(std::to_string(i),data(text.get(std::to_string(i)),false,true));
    result.defineProperty("length",data(length));return result;
}
}
bool isNativeStringObject(const JSValue& value){static_cast<void>(JSWeakValue(value));return value.isHostObject("three.String");}
JSValue nativeStringValue(const JSValue& receiver){
    static_cast<void>(JSWeakValue(receiver));
    if(receiver.type()==JSValue::Type::String)return receiver;
    if(receiver.isHostObject("three.String"))return std::static_pointer_cast<StringData>(receiver.hostPayload("three.String"))->value;
    throw JSTypeError("String method called on incompatible receiver");
}
JSValue createNativeStringPrototype(){
    auto prototype=allocate("");
    prototype.defineProperty("toString",data(JSValue::function(stringValue,{},"toString",0),true,false,true));
    prototype.defineProperty("valueOf",data(JSValue::function(stringValue,{},"valueOf",0),true,false,true));
    prototype.defineProperty("startsWith",data(JSValue::function(startsWith,{},"startsWith",1),true,false,true));
    return prototype;
}
JSValue createNativeStringObject(const JSValue& text){
    static_cast<void>(JSWeakValue(text));auto result=allocate(text);result.setPrototype(nativeStringPrototype());return result;
}
