#include "core/UnicodeCase.h"
#include "core/JSString.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "animation/AnimationUtils.h"
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
namespace {
using Args=std::vector<JSValue>;
JSValue array(const Args& values={}){auto out=JSValue::array();for(const auto& value:values)out.get("push").call(out,{value});return out;}
std::u16string text(const JSValue& values){std::u16string out;for(std::uint32_t i=0;i<values.length();++i)out.push_back(static_cast<char16_t>(values.get(std::to_string(i)).toNumber()));return out;}
JSValue encoded(std::u16string_view value){auto out=array();for(const auto unit:value)out.get("push").call(out,{unit});return out;}
void event(const JSValue& state,const Args& args){auto events=state.get("events");events.get("push").call(events,{array(args)});}
JSValue decode(const JSValue& spec,const JSValue& state,const std::string& role){
    const auto kind=spec.get("kind");
    if(kind.strictEquals("undefined"))return {};if(kind.strictEquals("null"))return nullptr;
    if(kind.strictEquals("number"))return spec.get("value");if(kind.strictEquals("boolean"))return spec.get("value");
    if(kind.strictEquals("nan"))return std::numeric_limits<double>::quiet_NaN();
    if(kind.strictEquals("infinity"))return std::numeric_limits<double>::infinity();
    if(kind.strictEquals("negativeInfinity"))return -std::numeric_limits<double>::infinity();
    if(kind.strictEquals("negativeZero"))return -0.0;
    if(kind.strictEquals("symbol"))return JSValue::symbol("value");
    if(kind.strictEquals("string"))return jsUTF8(text(spec.get("units")));
    if(kind.strictEquals("boxed"))return createNativeStringObject(jsUTF8(text(spec.get("units"))));
    auto object=JSValue::object();if(!kind.strictEquals("object"))return object;
    const auto result=decode(spec.get("result"),state,role+".result");
    const auto converter=JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{
        event(c[0],{"convert",c[1],args.empty()?JSValue{}:args[0],self.sameValue(c[4])});
        if(c[3].get("throwConvert").toBoolean())throw JSTypeError("conversion");return c[2];
    },{state,role,result,spec,object});
    JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{
        event(c[0],{"get",c[1]});if(c[3].get("throwGet").toBoolean())throw JSTypeError("get converter");return c[2];
    },{state,role,converter,spec});object.defineProperty(JSValue::wellKnownSymbol("toPrimitive"),d);return object;
}
int methods(){
    const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);auto output=array();
    for(std::uint32_t i=0;i<requests.length();++i){const auto request=requests.get(std::to_string(i));const auto name=request.get("method");const auto prototype=nativeStringPrototype();const auto method=prototype.get(name);auto result=JSValue::object();
        if(request.get("kind").strictEquals("metadata")){
            const auto descriptor=prototype.getOwnPropertyDescriptor(name).value();result.set("name",method.get("name"));result.set("length",method.get("length"));result.set("constructible",method.isConstructible());result.set("prototypeOwn",method.hasOwn("prototype"));result.set("writable",descriptor.writable.value_or(false));result.set("enumerable",descriptor.enumerable.value_or(false));result.set("configurable",descriptor.configurable.value_or(false));
        }else{
            auto state=JSValue::object();state.set("events",array());const auto receiver=decode(request.get("receiver"),state,"receiver");Args args;if(request.has("position"))args.push_back(decode(request.get("position"),state,"position"));
            try{result.set("result",encoded(jsUTF16(method.call(receiver,args).toString())));}catch(const JSTypeError&){result.set("result","#TypeError");}catch(const JSRangeError&){result.set("result","#RangeError");}result.set("events",state.get("events"));
        }
        output.get("push").call(output,{result});
    }
    std::cout<<output.toJSON()->dump();return 0;
}
std::u16string pointText(unsigned p){std::u16string value;if(p<=0xffff)value.push_back(static_cast<char16_t>(p));else{const auto n=p-0x10000;value.push_back(static_cast<char16_t>(0xd800+(n>>10)));value.push_back(static_cast<char16_t>(0xdc00+(n&1023)));}return value;}
void emit(const std::u16string& text) {
    bool first=true;
    for (auto unit:text) {if (!first) std::cout << ',';first=false;std::cout << unsigned(unit);}
    std::cout << '\n';
}
}
int main(int argc, char** argv) {
    const auto mode=argc>1?std::string(argv[1]):std::string{};
    if(mode=="--methods")return methods();
    const auto convert=mode.rfind("--lower",0)==0?unicodeToLowerCase:unicodeToUpperCase;
    std::cout << std::hex;
    if(mode=="--lower-context-all"){
        for(unsigned p=0;p<=0x10ffff;++p){const auto value=pointText(p);for(const auto& source:{value+u"\u03a3",u"A"+value+u"\u03a3",u"A\u03a3"+value,u"A\u03a3"+value+u"A"})emit(convert(source));}
    } else if (argc>1&&mode!="--lower") {
        for (unsigned p=0;p<=0x10ffff;++p) {
            emit(convert(pointText(p)));
        }
    } else {
        std::string line;
        while(std::getline(std::cin,line)) {
            std::istringstream input(line);input >> std::hex;
            std::u16string value;unsigned unit;
            while(input>>unit) {if(unit>0xffff)return 2;value.push_back(static_cast<char16_t>(unit));}
            emit(convert(value));
        }
    }
}
