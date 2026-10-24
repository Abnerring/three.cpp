#include "nodes/core/ConstNode.h"

bool ConstNode::isNumericType(const std::string& typeName) {
    // The source's pattern is /float|u?int/, which matches "float", "int" and "uint", and does so
    // wherever they appear rather than only at the start. A name holding one of them therefore
    // counts, which is a wider test than comparing whole names.
    if (typeName.find("float") != std::string::npos) return true;
    if (typeName.find("int") != std::string::npos) return true;
    if (typeName.find("uint") != std::string::npos) return true;
    return false;
}

bool ConstNode::writesDirectly(const std::string& typeName, const std::string& outputType) {
    // Both sides must be numeric; a numeric constant going to a vector is converted instead.
    return isNumericType(typeName) && isNumericType(outputType);
}

double ConstNode::numericValue(const std::string& typeName, double value) {
    // An integer loses its fraction towards zero. An unsigned one that is negative becomes zero
    // before the cut, so the two differ for every negative value.
    if (typeName == "int") return std::trunc(value);
    if (typeName == "uint") return value >= 0 ? std::trunc(value) : 0.0;
    // Any other type keeps the value as it is, since only the two named are handled here.
    return value;
}

bool ConstNode::isWeak(bool hasValue, bool valueIsTrue) {
    // A document that says nothing leaves the constant fixed, and one that says anything other
    // than exactly true leaves it fixed too.
    return hasValue && valueIsTrue;
}

#include "nodes/core/InputNode.h"
#include "core/JSRegExp.h"
#include "core/JSModuleObject.h"
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
struct Invocation{JSValue object,function;JSValue operator()(const Args& args={})const{return function.call(object,args);}};
Invocation invoke(const JSValue& object,const char* key){return {object,object.get(key)};}
JSValue superCall(const JSValue& self,const char* key,const Args& args){return nativeConstNodeConstructor().get("prototype").getPrototype().get(key,self).call(self,args);}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0].string();const auto builder=arg(args,0);
    if(name=="generateConst"){
        const auto callback=invoke(builder,"generateConst");const auto type=invoke(self,"getNodeType")({builder});return callback({type,self.get("value")});
    }
    if(name=="generate"){
        const auto type=invoke(self,"getNodeType")({builder}),output=arg(args,1),regexp=captures[1];
        if(invoke(regexp,"test")({type}).toBoolean()&&invoke(regexp,"test")({output}).toBoolean()){
            auto value=self.get("value");if(type.strictEquals("int"))value=std::trunc(value.toNumber());
            else if(type.strictEquals("uint")){const auto positive=value.toNumber()>=0;value=positive?JSValue(std::trunc(value.toNumber())):JSValue(0);}
            return invoke(builder,"generateConst")({output,value});
        }
        const auto callback=invoke(builder,"format");const auto constant=invoke(self,"generateConst")({builder});return callback({constant,type,output});
    }
    if(name=="serialize"){auto data=arg(args,0);superCall(self,"serialize",{data});data.set("isWeak",self.get("isWeak"));return {};}
    if(name=="deserialize"){const auto data=arg(args,0);superCall(self,"deserialize",{data});self.set("isWeak",data.get("isWeak").strictEquals(true));return {};}
    throw std::logic_error("Unknown ConstNode method");
}
}
JSValue nativeConstNodeConstructor(){
    static thread_local JSWeakValue weak;static thread_local bool initializing=false;
    if(const auto ctor=weak.lock();ctor&&ctor->isConstructible())return *ctor;
    JSModuleInitializationScope scope(initializing,"ConstNode");auto state=JSValue::object(nullptr);const auto base=nativeInputNodeConstructor(),regexp=createNativeRegExp("float|u?int");
    auto ctor=JSValue::builtinConstructor([](const JSValue& newTarget,const Args& args,const Args& c)->JSValue{
        const auto type=arg(args,1);auto self=c[0].get("constructor").getPrototype().construct({arg(args,0),type.isUndefined()?JSValue(nullptr):type},newTarget);self.set("isConstNode",true);self.set("isWeak",false);return self;
    },{state},"ConstNode",1);state.set("constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type;type.getter=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return "ConstNode";},{},"get type",0);type.configurable=true;ctor.defineProperty("type",type);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"generateConst",1},{"generate",2},{"serialize",1},{"deserialize",1}}){JSPropertyDescriptor d;d.value=JSValue::function(method,{name,regexp},name,arity);d.writable=d.configurable=true;prototype.defineProperty(name,d);}
    ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;
}
ConstNode::ConstNode(const JSValue& value,const JSValue& type):object_(constructor().construct({value,type})){}
