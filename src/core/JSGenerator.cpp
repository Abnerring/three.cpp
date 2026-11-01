#include "core/JSGenerator.h"
#include "core/JSFunction.h"
#include "core/JSIterator.h"

namespace {
using Args = std::vector<JSValue>;
enum class State { SuspendedStart, SuspendedYield, Executing, Completed };
struct FunctionData { JSGeneratorBody body; JSGeneratorInitializer initialize; };
struct GeneratorData { State state = State::SuspendedStart; JSGeneratorBody body; };
constexpr auto generatorTag = "runtime.Generator";
constexpr auto functionTag = "runtime.GeneratorFunctionData";
constexpr const char* contextSlots[] = {
    "Generator.function", "Generator.receiver", "Generator.arguments",
    "Generator.captures", "Generator.locals"
};
JSPropertyDescriptor data(JSValue value, bool writable, bool configurable = true,
                          bool enumerable = false) {
    JSPropertyDescriptor d; d.value=std::move(value); d.writable=writable;
    d.enumerable=enumerable; d.configurable=configurable; return d;
}
JSValue list(const Args& values) {
    auto array=JSValue::array();
    for (std::size_t i=0;i<values.size();++i)
        array.defineProperty(std::to_string(i),data(values[i],true,true,true));
    return array;
}
Args values(const JSValue& array) {
    Args out; out.reserve(array.length());
    for (std::uint32_t i=0;i<array.length();++i) out.push_back(array.get(std::to_string(i)));
    return out;
}
JSValue iteratorResult(const JSValue& value, bool done) {
    auto result=JSValue::object();
    result.defineProperty("value",data(value,true,true,true));
    result.defineProperty("done",data(done,true,true,true)); return result;
}
void complete(JSValue generator, const std::shared_ptr<GeneratorData>& state) {
    state->state=State::Completed;
    for (const auto key:contextSlots) generator.setInternalSlot(key,{});
}
JSValue resume(const JSValue& self,const Args& args,const Args& captures) {
    return resumeNativeGenerator(self,{static_cast<JSGeneratorCompletionKind>(static_cast<unsigned>(captures[0].number())),args.empty()?JSValue{}:args[0]});
}
JSValue invoke(const JSValue& receiver,const Args& args,const Args& captures) {
    const auto holder=captures[0];
    const auto metadata=std::static_pointer_cast<FunctionData>(holder.hostPayload(functionTag));
    Args environment(captures.begin()+1,captures.end());
    auto locals=JSValue::object(nullptr);
    if (metadata->initialize) metadata->initialize(locals,receiver,args,environment);
    const auto function=holder.getInternalSlot("GeneratorFunction.function");
    auto prototype=function.get("prototype");
    if (!prototype.isObject()) prototype=nativeGeneratorPrototype();
    auto payload=std::make_shared<GeneratorData>(); payload->body=metadata->body;
    auto generator=JSValue::hostObject(payload,payload.get(),generatorTag);
    generator.setPrototype(prototype);
    generator.setInternalSlot(contextSlots[0],function);
    generator.setInternalSlot(contextSlots[1],receiver);
    generator.setInternalSlot(contextSlots[2],list(args));
    generator.setInternalSlot(contextSlots[3],list(environment));
    generator.setInternalSlot(contextSlots[4],locals); return generator;
}
JSGeneratorStep empty(const JSValue&,const JSGeneratorCompletion& completion,
                      const JSValue&,const Args&,const Args&) {
    if (completion.kind==JSGeneratorCompletionKind::Throw) return JSGeneratorStep::abrupt(completion.value);
    return JSGeneratorStep::complete(completion.kind==JSGeneratorCompletionKind::Return?completion.value:JSValue{});
}
JSValue dynamicConstructor(const JSValue&,const JSValue& newTarget,const Args& args,const Args&) {
    if (!args.empty()) throw JSGeneratorDynamicSourceUnsupported();
    const auto target=newTarget.isUndefined()?nativeGeneratorFunctionConstructor():newTarget;
    const auto prototype=target.getPrototypeFromConstructor(nativeGeneratorFunctionPrototype());
    auto function=createNativeGeneratorFunction(empty,{},"anonymous",0,"function* anonymous(\n) {\n\n}");
    function.setPrototype(prototype); return function;
}
}
JSGeneratorStep JSGeneratorStep::yield(JSValue value) { return {Kind::Yield,std::move(value)}; }
JSGeneratorStep JSGeneratorStep::complete(JSValue value) { return {Kind::Return,std::move(value)}; }
JSGeneratorStep JSGeneratorStep::abrupt(JSValue value) { return {Kind::Throw,std::move(value)}; }
bool isNativeGeneratorObject(const JSValue& value) {
    static_cast<void>(JSWeakValue(value)); return value.isHostObject(generatorTag);
}
JSValue nativeGeneratorFunctionConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto value=weak.lock();value&&value->isConstructible()) return *value;
    const auto functionConstructor=nativeFunctionConstructor();
    auto functionPrototype=JSValue::object(functionConstructor.get("prototype"));
    auto generatorPrototype=JSValue::object(nativeIteratorPrototype());
    functionPrototype.defineProperty("prototype",data(generatorPrototype,false));
    auto constructor=JSValue::callableBuiltinConstructor(dynamicConstructor,{},"GeneratorFunction",1,functionPrototype);
    constructor.setPrototype(functionConstructor);
    functionPrototype.defineProperty("constructor",data(constructor,false));
    functionPrototype.defineProperty(JSValue::wellKnownSymbol("toStringTag"),data("GeneratorFunction",false));
    generatorPrototype.defineProperty("constructor",data(functionPrototype,false));
    const char* names[]={"next","return","throw"};
    for (unsigned i=0;i<3;++i)
        generatorPrototype.defineProperty(names[i],data(JSValue::function(resume,{i},names[i],1),true));
    generatorPrototype.defineProperty(JSValue::wellKnownSymbol("toStringTag"),data("Generator",false));
    constructor.setInternalSlot("Generator.prototype",generatorPrototype);
    constructor.retainForHeapLifetime(); weak=JSWeakValue(constructor); return constructor;
}
JSValue nativeGeneratorFunctionPrototype() { return nativeGeneratorFunctionConstructor().get("prototype"); }
JSValue nativeGeneratorPrototype() { return nativeGeneratorFunctionConstructor().getInternalSlot("Generator.prototype"); }
JSValue createNativeGeneratorFunction(JSGeneratorBody body,const Args& captures,
    const std::string& name,std::uint32_t arity,const std::string& source,
    JSGeneratorInitializer initialize) {
    if (!body) throw JSTypeError("A native generator requires a body");
    auto payload=std::make_shared<FunctionData>(FunctionData{body,initialize});
    auto holder=JSValue::hostObject(payload,payload.get(),functionTag);
    Args environment{holder}; environment.insert(environment.end(),captures.begin(),captures.end());
    auto function=JSValue::function(invoke,environment,name,arity);
    function.setPrototype(nativeGeneratorFunctionPrototype());
    function.defineProperty("prototype",data(JSValue::object(nativeGeneratorPrototype()),true,false));
    if (!source.empty()) function.setFunctionSourceText(source);
    holder.setInternalSlot("GeneratorFunction.function",function); return function;
}
JSValue resumeNativeGenerator(const JSValue& generator,JSGeneratorCompletion completion) {
    if (!isNativeGeneratorObject(generator)) throw JSTypeError("Generator method called on incompatible receiver");
    const auto payload=std::static_pointer_cast<GeneratorData>(generator.hostPayload(generatorTag));
    if (payload->state==State::Executing) throw JSTypeError("Generator is already running");
    if (payload->state==State::SuspendedStart&&completion.kind!=JSGeneratorCompletionKind::Normal)
        complete(generator,payload);
    if (payload->state==State::Completed) {
        if (completion.kind==JSGeneratorCompletionKind::Throw) throw JSGeneratorThrown(completion.value);
        return iteratorResult(completion.kind==JSGeneratorCompletionKind::Return?completion.value:JSValue{},true);
    }
    if (payload->state==State::SuspendedStart) completion.value={};
    const auto locals=generator.getInternalSlot(contextSlots[4]);
    const auto receiver=generator.getInternalSlot(contextSlots[1]);
    const auto args=values(generator.getInternalSlot(contextSlots[2]));
    const auto captures=values(generator.getInternalSlot(contextSlots[3]));
    payload->state=State::Executing;
    try {
        const auto step=payload->body(locals,completion,receiver,args,captures);
        if (step.kind==JSGeneratorStep::Kind::Yield) {
            payload->state=State::SuspendedYield; return iteratorResult(step.value,false);
        }
        complete(generator,payload);
        if (step.kind==JSGeneratorStep::Kind::Throw) throw JSGeneratorThrown(step.value);
        return iteratorResult(step.value,true);
    } catch (...) {
        complete(generator,payload); throw;
    }
}
