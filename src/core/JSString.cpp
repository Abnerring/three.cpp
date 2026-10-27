#include "core/JSString.h"
#include "core/JSIterator.h"
#include "core/JSRegExp.h"
#include "core/JSStringEncoding.h"
#include "core/UnicodeCase.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

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
    if(jsIsRegExp(search))throw JSTypeError("String.prototype.startsWith does not accept a regular expression");
    const auto needle=search.toString();
    const auto position=argument(args,1).toNumber();
    const auto stringUnits=units(text),searchUnits=units(needle);
    const double integer=std::isnan(position)||position==0?0:std::trunc(position);
    const auto start=static_cast<std::size_t>(std::min(std::max(integer,0.0),double(stringUnits.size())));
    if(searchUnits.size()>stringUnits.size()-start)return false;
    return std::equal(searchUnits.begin(),searchUnits.end(),stringUnits.begin()+start);
}
JSValue charCodeAt(const JSValue& self,const Args& args,const Args&) {
    if(self.isNull()||self.isUndefined())throw JSTypeError("String method called on null or undefined");
    const auto text=jsUTF16(self.toString());
    const auto position=argument(args,0).toNumber();
    const auto index=std::isnan(position)||position==0?0:std::trunc(position);
    if(index<0||index>=double(text.size()))return std::numeric_limits<double>::quiet_NaN();
    return text[static_cast<std::size_t>(index)];
}
JSValue charAt(const JSValue& self,const Args& args,const Args&) {
    if(self.isNull()||self.isUndefined())throw JSTypeError("String method called on null or undefined");
    const auto text=jsUTF16(self.toString());
    const auto position=argument(args,0).toNumber();
    const auto index=std::isnan(position)||position==0?0:std::trunc(position);
    if(index<0||index>=double(text.size()))return "";
    return jsUTF8(std::u16string_view(text).substr(static_cast<std::size_t>(index),1));
}
JSValue slice(const JSValue& self,const Args& args,const Args&) {
    if(self.isNull()||self.isUndefined())throw JSTypeError("String method called on null or undefined");
    const auto text=jsUTF16(self.toString());
    const double length=double(text.size());
    const auto relative=[&](const JSValue& value) {
        const auto n=value.toNumber();
        const auto integer=std::isnan(n)||n==0?0:std::trunc(n);
        return integer<0?std::max(length+integer,0.0):std::min(integer,length);
    };
    const auto start=relative(argument(args,0));
    const auto endValue=argument(args,1);
    const auto end=endValue.isUndefined()?length:relative(endValue);
    return jsUTF8(std::u16string_view(text).substr(static_cast<std::size_t>(start),
        static_cast<std::size_t>(std::max(end-start,0.0))));
}
JSValue getMethod(const JSValue& object,const char* symbol) {
    const auto method=object.get(JSValue::wellKnownSymbol(symbol));
    if(method.isNull()||method.isUndefined())return {};
    if(!method.isCallable())throw JSTypeError("String protocol method is not callable");
    return method;
}
JSValue match(const JSValue& self,const Args& args,const Args&) {
    if(self.isNull()||self.isUndefined())throw JSTypeError("String method called on null or undefined");
    const auto pattern=argument(args,0);
    if(!pattern.isNull()&&!pattern.isUndefined()) {
        const auto method=getMethod(pattern,"match");
        if(!method.isUndefined())return method.call(pattern,{self});
    }
    const auto text=self.toString();
    const auto regexp=nativeRegExpConstructor().construct({pattern});
    const auto method=regexp.get(JSValue::wellKnownSymbol("match"));
    return method.call(regexp,{text});
}
JSValue substring(const JSValue& self,const Args& args,const Args&) {
    if(self.isNull()||self.isUndefined())throw JSTypeError("String method called on null or undefined");
    const auto text=jsUTF16(self.toString());
    const auto length=double(text.size());
    const auto clamp=[&](const JSValue& value) {
        const auto number=value.toNumber();
        const auto integer=std::isnan(number)||number==0?0:std::trunc(number);
        return std::min(std::max(integer,0.0),length);
    };
    const auto start=clamp(argument(args,0));
    const auto endValue=argument(args,1);
    const auto end=endValue.isUndefined()?length:clamp(endValue);
    const auto from=std::min(start,end),to=std::max(start,end);
    return jsUTF8(std::u16string_view(text).substr(static_cast<std::size_t>(from),static_cast<std::size_t>(to-from)));
}
void coercible(const JSValue& self) {
    if(self.isNull()||self.isUndefined())throw JSTypeError("String method called on null or undefined");
}
JSValue indexOf(const JSValue& self,const Args& args,const Args&) {
    coercible(self);
    const auto text=jsUTF16(self.toString());
    const auto needle=jsUTF16(argument(args,0).toString());
    const auto position=argument(args,1).toNumber();
    const auto integer=std::isnan(position)||position==0?0:std::trunc(position);
    const auto start=static_cast<std::size_t>(std::min(std::max(integer,0.0),double(text.size())));
    const auto found=text.find(needle,start);
    return found==std::u16string::npos?JSValue(-1):JSValue(found);
}
JSValue replace(const JSValue& self,const Args& args,const Args&) {
    coercible(self);const auto search=argument(args,0),replacement=argument(args,1);
    if(!search.isNull()&&!search.isUndefined()) {
        const auto method=getMethod(search,"replace");
        if(!method.isUndefined())return method.call(search,{self,replacement});
    }
    const auto text=jsUTF16(self.toString());const auto needle=jsUTF16(search.toString());
    const bool functional=replacement.isCallable();const auto substitution=functional?std::u16string{}:jsUTF16(replacement.toString());
    const auto position=text.find(needle);if(position==std::u16string::npos)return jsUTF8(text);
    const auto added=functional?jsUTF16(replacement.call({},{jsUTF8(needle),position,jsUTF8(text)}).toString()):jsGetSubstitution(needle,text,position,{},JSValue{},substitution);
    return jsUTF8(text.substr(0,position)+added+text.substr(position+needle.size()));
}
std::uint32_t uint32(const JSValue& value) {
    const auto n=value.toNumber();if(!std::isfinite(n)||n==0)return 0;
    auto result=std::fmod(std::trunc(n),4294967296.0);if(result<0)result+=4294967296.0;
    return static_cast<std::uint32_t>(result);
}
JSValue split(const JSValue& self,const Args& args,const Args&) {
    coercible(self);const auto separator=argument(args,0),limitValue=argument(args,1);
    if(!separator.isNull()&&!separator.isUndefined()) {
        const auto method=getMethod(separator,"split");if(!method.isUndefined())return method.call(separator,{self,limitValue});
    }
    const auto text=jsUTF16(self.toString());auto out=JSValue::array();
    const auto limit=limitValue.isUndefined()?UINT32_MAX:uint32(limitValue);
    // Separator conversion occurs even with a zero limit.
    const auto needle=jsUTF16(separator.toString());if(!limit)return out;
    std::uint32_t count=0;
    auto append=[&](std::u16string_view value){out.defineProperty(std::to_string(count++),data(jsUTF8(value),true,true,true));return count==limit;};
    if(separator.isUndefined()){append(text);return out;}
    if(text.empty()){if(!needle.empty())append(text);return out;}
    std::size_t previous=0,position=0;
    if(needle.empty()) {for(const auto c:text)if(append(std::u16string_view(&c,1)))break;return out;}
    while((position=text.find(needle,previous))!=std::u16string::npos) {
        if(append(std::u16string_view(text).substr(previous,position-previous)))return out;
        previous=position+needle.size();
    }
    append(std::u16string_view(text).substr(previous));return out;
}
JSValue trim(const JSValue& self,const Args&,const Args&) {
    coercible(self);const auto text=jsUTF16(self.toString());std::size_t start=0,end=text.size();
    while(start<end&&jsWhitespace(text[start]))++start;while(end>start&&jsWhitespace(text[end-1]))--end;
    return jsUTF8(std::u16string_view(text).substr(start,end-start));
}
JSValue toUpperCase(const JSValue& self,const Args&,const Args&) {
    coercible(self);return jsUTF8(unicodeToUpperCase(jsUTF16(self.toString())));
}
JSValue toLowerCase(const JSValue& self,const Args&,const Args&) {
    coercible(self);return jsUTF8(unicodeToLowerCase(jsUTF16(self.toString())));
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
    const auto install=[&](const char* name,JSValue::NativeFunction callback,unsigned arity){
        auto method=JSValue::function(callback,{},name,arity);method.retainForHeapLifetime();
        prototype.defineProperty(name,data(method,true,false,true));
    };
    install("indexOf",indexOf,1);install("replace",replace,2);install("split",split,2);install("trim",trim,0);install("toUpperCase",toUpperCase,0);
    install("charCodeAt",charCodeAt,1);install("charAt",charAt,1);install("toLowerCase",toLowerCase,0);install("slice",slice,2);
    install("match",match,1);install("substring",substring,2);
    return prototype;
}
JSValue createNativeStringObject(const JSValue& text){
    static_cast<void>(JSWeakValue(text));auto result=allocate(text);result.setPrototype(nativeStringPrototype());return result;
}
