#include "nodes/utils/Discard.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/code/ExpressionNode.h"
#include "nodes/math/ConditionalNode.h"
#include "nodes/tsl/TSLCore.h"

Discard::Form Discard::form(bool hasCondition, bool conditionIsTruthy) {
    // A truth test, not a test for presence, so a condition that is there but falsy takes the
    // same branch as one that is missing.
    if (hasCondition && conditionIsTruthy) return Form::Conditional;
    return Form::Always;
}

std::string Discard::expression() {
    return "discard";
}

std::string Discard::alwaysExpression() {
    // The same statement word for word, since the difference between the forms is the condition
    // wrapped around it rather than the statement itself.
    return "discard";
}

std::string Discard::returnExpression() {
    return "return";
}

namespace { JSWeakValue& discardLexical(){static thread_local JSWeakValue weak;return weak;} }
JSValue nativeDiscardModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Discard exports");
    const auto conditional=nativeConditionalNodeModule(),expression=nativeExpressionNodeModule(),tsl=nativeTSLCore();
    const auto select=nativeConditionalNodeBinding("select"),expr=nativeExpressionNodeBinding("expression");auto bindings=JSValue::object();
    own(bindings,"Discard",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
        const auto condition=arg(args,0);JSValue node;
        if(condition.toBoolean()){const auto discard=c[1].call({}, {"discard"});node=c[0].call({}, {condition,discard});}
        else node=c[1].call({}, {"discard"});return invoke(node,"toStack")();
    },{select,expr},"Discard",1));
    own(bindings,"Return",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return invoke(c[0].call({}, {"return"}),"toStack")();},{expr},"Return",0));
    nativeTSLCoreBinding("addMethodChaining").call({}, {"discard",bindings.get("Discard")});auto module=JSValue::object();
    for(const auto& key:bindings.ownPropertyNames())own(module,key.c_str(),bindings.get(key));bindings.retainForHeapLifetime();discardLexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeDiscardBinding(const std::string& name){const auto keeper=nativeDiscardModule();return discardLexical().lock()->get(name);}
