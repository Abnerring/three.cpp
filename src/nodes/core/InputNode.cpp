#include "nodes/core/InputNode.h"

std::string InputNode::nodeType(bool typeWasGiven, const std::string& givenType,
                                const std::string& typeFromValue) {
    // The given type when there is one, the value's otherwise.
    if (typeWasGiven) return givenType;
    return typeFromValue;
}

std::string InputNode::inputType(bool derivedOverrides, const std::string& nodeTypeValue,
                                 const std::string& derivedInputType) {
    // By default the node type; a derived class may answer otherwise.
    if (derivedOverrides) return derivedInputType;
    return nodeTypeValue;
}

bool InputNode::serialisesArrayBufferAsText() {
    return true;
}

bool InputNode::deserialisesTextBackToArrayBuffer() {
    // The text is not an array, so it is taken as it stands rather than turned back.
    return false;
}

bool InputNode::reconstructsFromParts(bool deserialisedValueIsArray) {
    return deserialisedValueIsArray;
}

bool InputNode::precisionIsSetAfterDeserialising(bool hasStoredPrecision,
                                                 bool storedPrecisionTruthy) {
    // A short circuiting or, so a stored value that is falsy gives nothing.
    return hasStoredPrecision && storedPrecisionTruthy;
}

#include "nodes/core/Node.h"
#include "nodes/core/NodeUtils.h"
#include "core/JSIterator.h"
#include "core/JSModuleObject.h"
#include "utils.h"
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
struct Invocation{JSValue object,function;JSValue operator()(const Args& args={})const{return function.call(object,args);}};
Invocation invoke(const JSValue& object,const char* key){return {object,object.get(key)};}
JSValue superCall(const JSValue& self,const char* key,const Args& args){
    return nativeInputNodeConstructor().get("prototype").getPrototype().get(key,self).call(self,args);
}
JSValue utilsCall(const char* key,const Args& args){return nativeNodeUtilsBinding(key).call({},args);}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0].string();
    if(name=="isCacheable")return false;
    if(name=="generateNodeType"){
        if(self.get("nodeType").isNull())return utilsCall("getValueType",{self.get("value")});return self.get("nodeType");
    }
    if(name=="getInputType")return invoke(self,"getNodeType")({arg(args,0)});
    if(name=="setPrecision"){self.set("precision",arg(args,0));return self;}
    if(name=="serialize"){
        auto data=arg(args,0);superCall(self,"serialize",{data});data.set("value",self.get("value"));
        if(self.get("value").toBoolean()&&self.get("value").get("toArray").toBoolean())data.set("value",invoke(self.get("value"),"toArray")());
        data.set("valueType",utilsCall("getValueType",{self.get("value")}));data.set("nodeType",self.get("nodeType"));
        if(data.get("valueType").strictEquals("ArrayBuffer"))data.set("value",utilsCall("arrayBufferToBase64",{data.get("value")}));
        data.set("precision",self.get("precision"));return {};
    }
    if(name=="deserialize"){
        const auto data=arg(args,0);superCall(self,"deserialize",{data});self.set("nodeType",data.get("nodeType"));
        JSValue value;
        if(data.get("value").type()==JSValue::Type::Array){
            const auto function=nativeNodeUtilsBinding("getValueFromType");Args params{data.get("valueType")};
            auto iterator=getJSIterator(data.get("value"));while(const auto item=stepJSIterator(iterator))params.push_back(*item);value=function.call({},params);
        }else value=data.get("value");
        self.set("value",value);const auto precision=data.get("precision");self.set("precision",precision.toBoolean()?precision:JSValue(nullptr));
        if(self.get("value").toBoolean()&&self.get("value").get("fromArray").toBoolean()){
            const auto callback=invoke(self.get("value"),"fromArray");self.set("value",callback({data.get("value")}));
        }return {};
    }
    if(name=="generate"){ThreeUtils::warn({"Abstract function."});return {};}
    throw std::logic_error("Unknown InputNode method");
}
}
JSValue nativeInputNodeConstructor(){
    static thread_local JSWeakValue weak;static thread_local bool initializing=false;
    if(const auto ctor=weak.lock();ctor&&ctor->isConstructible())return *ctor;
    JSModuleInitializationScope scope(initializing,"InputNode");auto state=JSValue::object(nullptr);const auto base=nativeNodeConstructor();
    auto ctor=JSValue::builtinConstructor([](const JSValue& newTarget,const Args& args,const Args& captures)->JSValue{
        const auto type=arg(args,1);auto self=captures[0].get("constructor").getPrototype().construct({type.isUndefined()?JSValue(nullptr):type},newTarget);
        self.set("isInputNode",true);self.set("value",arg(args,0));self.set("precision",nullptr);return self;
    },{state},"InputNode",1);
    state.set("constructor",ctor);ctor.setPrototype(base);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type;type.getter=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return "InputNode";},{},"get type",0);type.configurable=true;ctor.defineProperty("type",type);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"isCacheable",0},{"generateNodeType",0},{"getInputType",1},{"setPrecision",1},{"serialize",1},{"deserialize",1},{"generate",0}}){
        JSPropertyDescriptor d;d.value=JSValue::function(method,{name},name,arity);d.writable=d.configurable=true;prototype.defineProperty(name,d);
    }ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;
}
InputNode::InputNode(const JSValue& value,const JSValue& type):object_(constructor().construct({value,type})){}
