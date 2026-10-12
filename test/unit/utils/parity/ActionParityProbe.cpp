#include "animation/AnimationAction.h"
#include "animation/AnimationUtils.h"
#include <iostream>
#include <cmath>

namespace {
using Args = std::vector<JSValue>;
void push(JSValue array, JSValue value) { array.set(std::to_string(array.length()), value); }
JSValue array(std::initializer_list<JSValue> values) { auto result = JSValue::array(); for (const auto& v : values) push(result, v); return result; }
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
void fn(JSValue object, const char* name, JSValue::NativeFunction callback, Args captures = {}) { object.set(name, JSValue::function(callback, captures, name)); }
JSValue decode(JSValue value) {
    if (!value.isObject()) return value;
    if (value.hasOwn("$number")) return value.get("$number").toNumber();
    if (value.hasOwn("$undefined")) return {};
    for (const auto& key : value.keys()) value.set(key, decode(value.get(key)));
    return value;
}
JsonValue encode(JSValue value) {
    if (value.isUndefined()) return "$undefined";
    if (value.type() == JSValue::Type::Number) {
        const auto n = value.number(); if (std::isnan(n)) return "$NaN";
        if (std::isinf(n)) return n < 0 ? "$-Infinity" : "$Infinity";
        if (n == 0 && std::signbit(n)) return "$-0"; return n;
    }
    if (!value.isObject()) return *value.toJSON();
    if (value.type() == JSValue::Type::Array) {
        JsonValue::Array result; for (std::uint32_t i = 0; i < value.length(); ++i) result.push_back(encode(value.get(std::to_string(i)))); return result;
    }
    JsonValue::Object result; for (const auto& key : value.keys()) { const auto child=value.get(key); if (!child.isCallable()) result[key] = encode(child); } return result;
}
JSValue call(JSValue receiver, const char* name, Args args = {}) { return receiver.get(name).call(receiver, args); }
JSValue coercion(JSValue value, JSValue label, JSValue trace) {
    auto object=JSValue::object();object.set("value",value);
    fn(object,"valueOf",[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],array({"coerce",c[1]}));return self.get("value");},{trace,label});
    return object;
}
void watch(JSValue object, JSValue trace, const std::vector<std::string>& keys) {
    auto storage=JSValue::object();
    for(const auto& key:keys){storage.set(key,object.get(key));JSPropertyDescriptor d;d.configurable=d.enumerable=true;
        d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],array({"get",c[2]}));return c[1].get(c[2].string());},{trace,storage,key});
        d.setter=JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{push(c[0],array({"set",c[2]}));auto s=c[1];s.set(c[2].string(),arg(args,0));return {};},{trace,storage,key});
        object.defineProperty(key,d);
    }
}
JSValue fixture(JSValue request, JSValue trace, JSValue state) {
    auto mixer = JSValue::object(); mixer.set("time", request.hasOwn("now") ? request.get("now") : JSValue(0));
    mixer.set("_root", JSValue::object()); mixer.set("active", false); mixer.set("next", 0);
    fn(mixer, "_activateAction", [](const JSValue& receiver, const Args&, const Args& c) -> JSValue {
        auto self = receiver; self.set("active", true); push(c[0], array({"activate"})); return {};
    }, {trace});
    fn(mixer, "_deactivateAction", [](const JSValue& receiver, const Args&, const Args& c) -> JSValue {
        auto self = receiver; self.set("active", false); push(c[0], array({"deactivate"})); return {};
    }, {trace});
    fn(mixer, "_isActiveAction", [](const JSValue& self, const Args&, const Args& c) -> JSValue {
        push(c[0], array({"isActive"})); return self.get("active");
    }, {trace});
    fn(mixer, "_lendControlInterpolant", [](const JSValue& receiver, const Args&, const Args& c) -> JSValue {
        auto self = receiver; const auto id = self.get("next"); self.set("next", id.toNumber() + 1);
        push(c[0], array({"lend", id})); auto result = JSValue::object(); result.set("id", id);
        result.set("parameterPositions", array({0,0})); result.set("sampleValues", array({0,0}));
        fn(result, "evaluate", [](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            push(c[0], array({"control", self.get("id"), arg(args,0)})); return array({c[1].get("controlValue")});
        }, {c[0],c[1]}); return result;
    }, {trace,state});
    fn(mixer, "_takeBackControlInterpolant", [](const JSValue&, const Args& args, const Args& c) -> JSValue {
        push(c[0], array({"takeBack", arg(args,0).get("id")})); return {};
    }, {trace});
    fn(mixer, "dispatchEvent", [](const JSValue&, const Args& args, const Args& c) -> JSValue {
        const auto event = arg(args,0), action = event.get("action");
        push(c[0], array({"event",event.get("type"),event.get("direction"),event.get("loopDelta"),action.get("time"),action.get("_loopCount"),action.get("enabled"),action.get("paused")}));
        const auto op = c[1].get("eventOp");
        if (op.strictEquals("reset")) call(action,"reset");
        if (op.strictEquals("stop")) call(action,"stop");
        if (op.strictEquals("change")) { auto a=action; a.set("weight",.25); a.set("time",99); a.set("enabled",true); }
        c[0].collectGarbage(); return {};
    }, {trace,state});
    return mixer;
}
JSValue clipFixture(JSValue duration, JSValue trace, int count) {
    auto clip = JSValue::object(), tracks = JSValue::array(); clip.set("duration",duration); clip.set("blendMode",2500);
    for(int i=0;i<count;++i){
        auto track=JSValue::object();
        fn(track,"createInterpolant",[](const JSValue&,const Args& args,const Args& c)->JSValue{
            push(c[0],array({"create",c[1],arg(args,0).isNull()}));auto interpolant=JSValue::object();
            fn(interpolant,"evaluate",[](const JSValue&,const Args& args,const Args& c)->JSValue{
                push(c[0],array({"evaluate",c[1],arg(args,0)}));return array({arg(args,0)});
            },c);return interpolant;
        },{trace,i});push(tracks,track);
    }
    clip.set("tracks",tracks);return clip;
}
void bindings(JSValue action,JSValue trace,int count){
    auto values=action.get("_propertyBindings");
    for(int i=0;i<count;++i){auto binding=JSValue::object();
        fn(binding,"accumulate",[](const JSValue&,const Args& a,const Args& c)->JSValue{push(c[0],array({"accumulate",c[1],arg(a,0),arg(a,1)}));return {};},{trace,i});
        fn(binding,"accumulateAdditive",[](const JSValue&,const Args& a,const Args& c)->JSValue{push(c[0],array({"additive",c[1],arg(a,0)}));return {};},{trace,i});
        values.set(std::to_string(i),binding);
    }
}
JsonValue snapshot(JSValue action,JSValue mixer){
    JsonValue::Object out;
    for(const char* key:{"blendMode","loop","_loopCount","_startTime","time","timeScale","_effectiveTimeScale","weight","_effectiveWeight","repetitions","paused","enabled","clampWhenFinished","zeroSlopeAtStart","zeroSlopeAtEnd","_restoreTimeScale","_cacheIndex","_byClipCacheIndex","_interpolantSettings"})out[key]=encode(action.get(key));
    for(const char* key:{"_timeScaleInterpolant","_weightInterpolant"}){
        const auto control=action.get(key);out[key]=control.isNull()?JsonValue(nullptr):JsonValue::Object{{"id",encode(control.get("id"))},{"times",encode(control.get("parameterPositions"))},{"values",encode(control.get("sampleValues"))}};
    }
    out["active"]=encode(mixer.get("active"));out["now"]=encode(mixer.get("time"));return out;
}
JsonValue run(JSValue request){
    auto trace=JSValue::array(),state=JSValue::object();state.set("controlValue",1);state.set("eventOp",request.get("eventOp"));
    auto mixer=fixture(request,trace,state);const auto count=request.hasOwn("count")?int(request.get("count").toNumber()):2;
    const auto duration=request.hasOwn("duration")?request.get("duration"):JSValue(2);
    auto clip=clipFixture(duration,trace,count);
    AnimationAction handle(mixer,clip,request.get("localRoot"),request.get("blendMode"));auto action=handle.object();bindings(action,trace,count);
    auto otherClip=clipFixture(4,trace,0);AnimationAction otherHandle(mixer,otherClip);auto other=otherHandle.object();
    const auto fields=request.get("fields");if(fields.isObject())for(const auto& key:fields.keys())action.set(key,fields.get(key));
    const auto converted=request.get("coerce");if(converted.isObject())for(const auto& key:converted.keys())action.set(key,coercion(converted.get(key),key,trace));
    if(request.get("watch").toBoolean()){
        watch(action,trace,{"enabled","paused","time","timeScale","weight","repetitions","_loopCount","_startTime","_mixer","_clip","_localRoot","_effectiveTimeScale","_effectiveWeight","_restoreTimeScale","_timeScaleInterpolant","_weightInterpolant","_interpolants","_propertyBindings","blendMode","loop","clampWhenFinished","zeroSlopeAtStart","zeroSlopeAtEnd","_interpolantSettings","warp","stopFading","stopWarping","reset","fadeIn","fadeOut","_setEndings","_updateTime","_updateTimeScale","_updateWeight","_scheduleFading"});
        watch(mixer,trace,{"time","_activateAction","_deactivateAction","_isActiveAction","_lendControlInterpolant","_takeBackControlInterpolant","dispatchEvent"});
        watch(action.get("_interpolantSettings"),trace,{"endingStart","endingEnd"});
    }
    JsonValue::Object out{{"initial",snapshot(action,mixer)}};JsonValue::Array steps;
    const auto operations=request.get("operations");
    for(std::uint32_t i=0;i<operations.length();++i){
        const auto op=operations.get(std::to_string(i));JsonValue result;
        try{
            if(op.hasOwn("set")){auto target=op.get("target").strictEquals("other")?other:op.get("target").strictEquals("mixer")?mixer:op.get("target").strictEquals("state")?state:action;const auto props=op.get("set");for(const auto& key:props.keys())target.set(key,props.get(key));result="$undefined";}
            else{Args args;const auto input=op.get("args");if(input.isObject())for(std::uint32_t j=0;j<input.length();++j){auto value=input.get(std::to_string(j));if(value.isObject()&&value.hasOwn("$coerce"))value=coercion(value.get("$coerce"),"arg"+std::to_string(j),trace);args.push_back(value.strictEquals("$other")?other:value.strictEquals("$self")?action:value);}
                const auto value=call(action,op.get("method").string().c_str(),args);
                result=value.strictEquals(action)?JsonValue("$self"):value.strictEquals(other)?JsonValue("$other"):value.strictEquals(mixer)?JsonValue("$mixer"):value.strictEquals(clip)?JsonValue("$clip"):value.strictEquals(mixer.get("_root"))?JsonValue("$root"):encode(value);
            }
        }catch(const JSTypeError&){result=JsonValue::Object{{"error","TypeError"}};}catch(const JSRangeError&){result=JsonValue::Object{{"error","RangeError"}};}
        steps.emplace_back(JsonValue::Object{{"result",result},{"state",snapshot(action,mixer)},{"other",snapshot(other,mixer)}});
    }
    out["steps"]=steps;out["trace"]=encode(trace);return out;
}
}
int main(){try{
    const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto cases=decode(Animation_detail::parseJSON(input));JsonValue::Array out;
    for(std::uint32_t i=0;i<cases.length();++i){try{out.push_back(run(cases.get(std::to_string(i))));}catch(const JSTypeError&){out.emplace_back(JsonValue::Object{{"error","TypeError"}});}catch(const JSRangeError&){out.emplace_back(JsonValue::Object{{"error","RangeError"}});}}
    std::cout<<JsonValue(out).dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
