#include "core/JSValue.h"
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue empty(const JSValue&,const Args&,const Args&){return {};}
JsonValue strings(const std::vector<std::string>& values){JsonValue::Array result;for(const auto& value:values)result.emplace_back(value);return result;}
JsonValue descriptor(const JSValue& object,const std::string& key){
    const auto value=object.getOwnPropertyDescriptor(key);
    return JsonValue::Object{{"writable",*value->writable},{"enumerable",*value->enumerable},{"configurable",*value->configurable}};
}
std::string type(const JSValue& value){return value.isCallable()?"function":value.type()==JSValue::Type::Array?"array":value.isObject()?"object":value.isUndefined()?"undefined":value.isNull()?"null":value.type()==JSValue::Type::Number?"number":value.type()==JSValue::Type::String?"string":"boolean";}
JSValue returned(const std::string& kind){
    if(kind=="null")return nullptr;if(kind=="false")return false;if(kind=="number")return 12;
    if(kind=="nan")return std::numeric_limits<double>::quiet_NaN();if(kind=="negativeZero")return -0.0;
    if(kind=="string")return "return";if(kind=="array")return JSValue::array(2);
    if(kind=="function")return JSValue::function(empty);
    if(kind=="object"){auto object=JSValue::object();object.set("return",true);return object;}
    return {};
}
JsonValue metadata(bool callable,std::size_t arity){
    auto value=JSValue::constructor(empty,{},"Example",arity,callable?JSValue::ConstructorKind::Callable:JSValue::ConstructorKind::Class);
    return JsonValue::Object{{"kind","metadata"},{"callable",callable},{"arity",arity},
        {"names",strings(value.ownPropertyNames())},{"prototypeNames",strings(value.get("prototype").ownPropertyNames())},
        {"name",value.get("name").string()},{"length",value.get("length").number()},
        {"nameDescriptor",descriptor(value,"name")},{"lengthDescriptor",descriptor(value,"length")},
        {"prototypeDescriptor",descriptor(value,"prototype")},{"constructorDescriptor",descriptor(value.get("prototype"),"constructor")},
        {"backlink",value.get("prototype").get("constructor").sameValue(value)}};
}
JsonValue scenario(bool callable,const std::string& returnKind,const std::string& prototypeKind){
    auto state=JSValue::object();auto value=returned(returnKind);auto log=JSValue::array();
    auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args& captures){
        auto state=captures[0],log=captures[3],instance=self;
        state.set("receiver",self);log.set(std::to_string(log.length()),"body");
        instance.set("argument",args.empty()?JSValue():args[0]);instance.set("second",args.size()>1?args[1]:JSValue());
        const auto kind=captures[2].string();if(kind=="throw")throw JSRangeError("body");
        return kind=="self"?self:captures[1];
    },{state,value,returnKind,log},"Example",2,callable?JSValue::ConstructorKind::Callable:JSValue::ConstructorKind::Class);
    auto target=constructor;
    if(prototypeKind!="self"){
        target=JSValue::constructor([](const JSValue&,const Args&,const Args& captures){auto log=captures[0];log.set(std::to_string(log.length()),"target");return JSValue();},{log},"Target",0,JSValue::ConstructorKind::Callable);
        if(prototypeKind=="object")target.set("prototype",JSValue::object(nullptr));
        else if(prototypeKind=="null")target.set("prototype",nullptr);
        else if(prototypeKind=="number")target.set("prototype",17);
        else if(prototypeKind=="array")target.set("prototype",JSValue::array());
        else if(prototypeKind=="function")target.set("prototype",JSValue::function(empty));
    }
    auto expectedPrototype=target.get("prototype");if(!expectedPrototype.isObject())expectedPrototype=JSValue::object().getPrototype();
    JSValue result;std::string error;
    try{result=constructor.construct({7,"argument"},target);}catch(const JSRangeError&){error="RangeError";}catch(const JSTypeError&){error="TypeError";}
    const auto receiver=state.get("receiver");
    JsonValue out=JsonValue::Object{{"kind","construct"},{"callable",callable},{"returned",returnKind},{"prototype",prototypeKind},
        {"error",error},{"resultType",type(result)},{"sameReturned",value.isObject()&&result.sameValue(value)},
        {"sameReceiver",result.sameValue(receiver)},{"receiverPrototype",receiver.getPrototype().sameValue(expectedPrototype)},
        {"receiverKeys",strings(receiver.keys())},{"argument",receiver.get("argument").number()},
        {"second",receiver.get("second").string()},{"log",*log.toJSON()}};
    auto callReceiver=JSValue::object();error.clear();JSValue callResult;
    try{callResult=constructor.call(callReceiver,{3,"call"});}catch(const JSRangeError&){error="RangeError";}catch(const JSTypeError&){error="TypeError";}
    out["callError"]=error;out["callType"]=type(callResult);out["callKeys"]=strings(callReceiver.keys());
    return out;
}
JsonValue invalid(const std::string& targetKind,bool validBase){
    auto log=JSValue::array();auto base=validBase?JSValue::constructor(empty):JSValue::function(empty);
    auto target=targetKind=="function"?JSValue::function(empty):targetKind=="object"?JSValue::object():returned(targetKind);
    if(target.isObject()){
        JSPropertyDescriptor descriptor;descriptor.getter=JSValue::function([](const JSValue&,const Args&,const Args& captures){
            auto log=captures[0];log.set(std::to_string(log.length()),"prototype");throw JSRangeError("getter");return JSValue();
        },{log});target.defineProperty("prototype",descriptor);
    }
    std::string error;try{base.construct({},target);}catch(const JSRangeError&){error="RangeError";}catch(const JSTypeError&){error="TypeError";}
    return JsonValue::Object{{"kind","invalid"},{"target",targetKind},{"validBase",validBase},{"error",error},{"log",*log.toJSON()}};
}
JsonValue reentry(bool throws){
    auto state=JSValue::object(),log=JSValue::array();state.set("depth",0);
    auto constructor=JSValue::constructor([](const JSValue& self,const Args&,const Args& captures){
        auto state=captures[0],log=captures[1];auto depth=state.get("depth").number();
        log.set(std::to_string(log.length()),depth);state.set("escaped",self);
        if(depth==0){state.set("depth",1);state.get("constructor").construct();}
        self.collectGarbage();if(captures[2].toBoolean())throw JSRangeError("body");return JSValue();
    },{state,log,throws});state.set("constructor",constructor);
    std::string error;try{constructor.construct();}catch(const JSRangeError&){error="RangeError";}
    return JsonValue::Object{{"kind","reentry"},{"throws",throws},{"error",error},{"log",*log.toJSON()},{"prototype",state.get("escaped").getPrototype().sameValue(constructor.get("prototype"))}};
}
JsonValue intrinsic(bool array,const std::string& prototypeKind,int argumentMode){
    auto constructor=(array?JSValue::array():JSValue::object()).get("constructor");
    auto target=constructor;
    if(prototypeKind!="self"){
        target=JSValue::constructor(empty,{},"Target",0,JSValue::ConstructorKind::Callable);
        if(prototypeKind=="null")target.set("prototype",nullptr);
    }
    Args args;if(argumentMode==1)args={3};if(argumentMode==2)args={JSValue::object()};if(argumentMode==3)args={"a","b"};if(argumentMode==4)args={-1};
    JSValue result;std::string error;try{result=constructor.construct(args,target);}catch(const JSRangeError&){error="RangeError";}catch(const JSTypeError&){error="TypeError";}
    auto prototype=target.get("prototype");if(!prototype.isObject())prototype=(array?JSValue::array():JSValue::object()).getPrototype();
    return JsonValue::Object{{"kind","intrinsic"},{"array",array},{"prototype",prototypeKind},{"argumentMode",argumentMode},
        {"error",error},{"resultType",type(result)},{"prototypeMatches",result.isObject()&&result.getPrototype().sameValue(prototype)},
        {"sameArgument",!args.empty()&&result.sameValue(args[0])},{"length",result.type()==JSValue::Type::Array?JsonValue(result.length()):JsonValue(nullptr)},
        {"keys",result.isObject()?strings(result.keys()):JsonValue::Array{}}};
}
}
int main(){
    try{
        JsonValue::Array output;
        for(bool callable:{false,true})for(std::size_t arity:{std::size_t(0),std::size_t(2),std::size_t(4294967296ULL)})output.push_back(metadata(callable,arity));
        for(bool callable:{false,true})for(const auto* returned:{"undefined","null","false","number","nan","negativeZero","string","array","function","object","self","throw"})
            for(const auto* prototype:{"self","other","object","null","number","array","function"})output.push_back(scenario(callable,returned,prototype));
        for(bool validBase:{false,true})for(const auto* target:{"undefined","null","false","number","string","function","object"})output.push_back(invalid(target,validBase));
        for(bool throws:{false,true})output.push_back(reentry(throws));
        for(bool array:{false,true})for(const auto* prototype:{"self","other","null"})for(int argument=0;argument<5;++argument){
            // Existing primitive wrapper objects are outside this runtime's supported types.
            if(!array&&std::string(prototype)=="self"&&(argument==1||argument==3||argument==4))continue;
            output.push_back(intrinsic(array,prototype,argument));
        }
        std::cout<<JsonValue(output).dump()<<'\n';
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
