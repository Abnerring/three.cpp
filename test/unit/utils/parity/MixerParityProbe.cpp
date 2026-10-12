#include "animation/AnimationMixer.h"
#include "animation/AnimationObjectGroup.h"
#include "animation/AnimationRuntime.h"
#include "animation/AnimationClip.h"
#include "core/NativeObject.h"
#include "objects/Mesh.h"
#include <iostream>
#include <cmath>

namespace {
using Args=std::vector<JSValue>;
void push(JSValue a,JSValue v){a.set(std::to_string(a.length()),v);}
JSValue at(JSValue a,std::size_t i){return a.get(std::to_string(i));}
JSValue call(JSValue v,const char* name,Args args={}){return v.get(name).call(v,args);}
JSValue decode(JSValue v){if(!v.isObject())return v;if(v.hasOwn("$undefined"))return {};if(v.hasOwn("$number"))return v.get("$number").toNumber();for(const auto& k:v.keys())v.set(k,decode(v.get(k)));return v;}
JsonValue encode(JSValue v){
    if(v.isUndefined())return "$undefined";
    if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "$NaN";if(std::isinf(n))return n<0?"$-Infinity":"$Infinity";if(n==0&&std::signbit(n))return "$-0";return n;}
    if(!v.isObject())return *v.toJSON();
    if(v.type()==JSValue::Type::Array){JsonValue::Array out;for(std::uint32_t i=0;i<v.length();++i)out.push_back(encode(at(v,i)));return out;}
    JsonValue::Object out;for(const auto& k:v.keys()){const auto child=v.get(k);if(!child.isCallable())out[k]=encode(child);}return out;
}
JSValue id(JSValue list,JSValue value){if(value.isNull()||value.isUndefined())return value;for(std::uint32_t i=0;i<list.length();++i)if(at(list,i).strictEquals(value))return i;push(list,value);return list.length()-1;}
JsonValue numericArray(JSValue a){JsonValue::Array out;for(std::size_t i=0,n=std::size_t(a.get("length").toNumber());i<n;++i)out.push_back(encode(at(a,i)));return out;}
JsonValue components(JSValue value,const std::vector<const char*>& keys){JsonValue::Array out;for(const auto* key:keys)out.push_back(encode(value.get(key)));return out;}
JsonValue prototypeMetadata(){
    JsonValue::Array classes;
    const auto functionPrototype=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return {};}).getPrototype();
    const auto objectPrototype=JSValue::object().getPrototype();
    for(const auto& p:{AnimationAction::prototype(),AnimationObjectGroup::prototype(),AnimationMixer::prototype()}){
        const auto constructor=p.get("constructor");JsonValue::Array keys,constructorKeys,properties,constructorProperties;
        for(const auto& key:p.ownPropertyNames()){
            keys.emplace_back(key);const auto d=*p.getOwnPropertyDescriptor(key);const auto fn=*d.value;
            properties.emplace_back(JsonValue::Object{{"key",key},{"writable",*d.writable},{"enumerable",*d.enumerable},{"configurable",*d.configurable},{"name",encode(fn.get("name"))},{"length",encode(fn.get("length"))}});
        }
        for(const auto& key:constructor.ownPropertyNames()){
            constructorKeys.emplace_back(key);const auto d=*constructor.getOwnPropertyDescriptor(key);
            constructorProperties.emplace_back(JsonValue::Object{{"key",key},{"writable",*d.writable},{"enumerable",*d.enumerable},{"configurable",*d.configurable}});
        }
        JsonValue error=nullptr;try{constructor.call(JSValue::object());}catch(const JSTypeError& e){error=JsonValue::Object{{"name","TypeError"},{"message",e.what()}};}
        classes.emplace_back(JsonValue::Object{{"name",encode(constructor.get("name"))},{"length",encode(constructor.get("length"))},{"keys",keys},{"constructorKeys",constructorKeys},{"properties",properties},{"constructorProperties",constructorProperties},
            {"prototypeIdentity",constructor.get("prototype").sameValue(p)},{"baseObject",p.getPrototype().sameValue(objectPrototype)},
            {"baseFunction",constructor.getPrototype().sameValue(functionPrototype)},
            {"constructorInheritsPrototypeConstructor",constructor.getPrototype().sameValue(p.getPrototype().get("constructor"))},{"call",error}});
    }
    return classes;
}
JSValue enumerationEntry(const char* name,bool bindings,JSValue callback){
    auto value=JSValue::object();value.set("name",name);
    if(bindings){value.set("restoreOriginalState",callback);return value;}
    auto byRoot=JSValue::object();byRoot.set("root",value);auto entry=JSValue::object();entry.set("actionByRoot",byRoot);return entry;
}
JsonValue runEnumeration(JSValue request){
    const bool bindings=request.get("forIn").strictEquals("bindings");auto root=JSValue::object();root.set("uuid","root");
    AnimationMixer mixer(root);auto prototype=JSValue::object(),table=JSValue::object(prototype),trace=JSValue::array(),callbacks=JSValue::object();
    const auto callback=JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{
        auto action=args.empty()?self:args[0];push(c[2],action.get("name"));
        if(!action.get("name").strictEquals("a"))return {};
        auto table=c[0],prototype=c[1];const auto mutation=c[3].get("mutation");const bool bindings=c[3].get("forIn").strictEquals("bindings");
        if(mutation.strictEquals("deleteOwn"))table.erase("b");
        else if(mutation.strictEquals("nonenumOwn")){JSPropertyDescriptor d;d.enumerable=false;table.defineProperty("b",d);}
        else if(mutation.strictEquals("nonenumProto")){JSPropertyDescriptor d;d.enumerable=false;prototype.defineProperty("c",d);}
        else if(mutation.strictEquals("deleteProto"))prototype.erase("c");
        else if(mutation.strictEquals("addProto"))prototype.set("z",enumerationEntry("z",bindings,c[4].get("fn")));
        else if(mutation.strictEquals("replaceProto")){auto p=JSValue::object();p.set("z",enumerationEntry("z",bindings,c[4].get("fn")));table.setPrototype(p);}
        else if(mutation.strictEquals("hideInherited")){JSPropertyDescriptor d;d.value=enumerationEntry("hidden",bindings,c[4].get("fn"));d.enumerable=false;d.configurable=true;table.defineProperty("c",d);}
        else if(mutation.strictEquals("readdOwn")){table.erase("b");table.set("b",enumerationEntry("readded",bindings,c[4].get("fn")));}
        return {};
    },{table,prototype,trace,request,callbacks});callbacks.set("fn",callback);
    JSPropertyDescriptor inherited;inherited.value=enumerationEntry("inherited",bindings,callback);inherited.enumerable=request.get("enumerable").toBoolean();inherited.configurable=true;inherited.writable=true;prototype.defineProperty("b",inherited);
    prototype.set("c",enumerationEntry("c",bindings,callback));table.set("a",enumerationEntry("a",bindings,callback));table.set("b",enumerationEntry("b",bindings,callback));
    if(request.hasOwn("value"))table=request.get("value");
    const auto noop=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return {};});
    if(bindings){auto byRoot=JSValue::object();byRoot.set("root",table);mixer.set("_bindingsByRootAndName",byRoot);mixer.set("_removeInactiveBinding",noop);mixer.set("_actionsByClip",JSValue::object(nullptr));}
    else{mixer.set("_actionsByClip",table);mixer.set("_deactivateAction",callback);mixer.set("_removeInactiveAction",noop);}
    auto objectPrototype=JSValue::object().getPrototype();
    if(request.get("prototypeGetter").toBoolean()){
        JSPropertyDescriptor d;d.enumerable=true;d.configurable=true;
        d.getter=JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{
            const char* type=self.type()==JSValue::Type::Number?"number":self.type()==JSValue::Type::Boolean?"boolean":self.type()==JSValue::Type::String?"string":"object";
            push(c[0],type);return c[1];
        },{trace,enumerationEntry("primitiveInherited",bindings,callback)});
        objectPrototype.defineProperty("enumInherited",d);
    }
    JsonValue error=nullptr;try{mixer.uncacheRoot(root);}catch(const JSTypeError&){error="TypeError";}catch(const JSRangeError&){error="RangeError";}
    if(request.get("prototypeGetter").toBoolean())objectPrototype.erase("enumInherited");
    return JsonValue::Object{{"trace",encode(trace)},{"error",error}};
}
JsonValue snapshot(JSValue mixer,JSValue roots,JSValue actions,JSValue bindingIds,JSValue group){
    JsonValue::Object out{{"time",encode(mixer.get("time"))},{"timeScale",encode(mixer.get("timeScale"))},{"accu",encode(mixer.get("_accuIndex"))},{"stats",encode(mixer.get("stats"))}};
    JsonValue::Array objects;
    for(std::uint32_t i=0;i<roots.length();++i){const auto root=at(roots,i);objects.emplace_back(JsonValue::Object{
        {"position",components(root.get("position"),{"x","y","z"})},{"quaternion",components(root.get("quaternion"),{"x","y","z","w"})},{"rotation",components(root.get("rotation"),{"x","y","z"})},
        {"visible",encode(root.get("visible"))},{"label",encode(root.get("label"))},{"matrixWorldNeedsUpdate",encode(root.get("matrixWorldNeedsUpdate"))},
        {"morph",numericArray(root.get("morphTargetInfluences"))},{"opacity",encode(root.get("material").get("opacity"))},
        {"color",components(root.get("material").get("color"),{"r","g","b"})},{"materialVersion",encode(root.get("material").get("version"))}});}
    out["objects"]=objects;JsonValue::Array actionStates;
    for(std::uint32_t i=0;i<actions.length();++i){const auto action=at(actions,i);JsonValue::Object entry;
        for(const char* k:{"time","timeScale","weight","enabled","paused","blendMode","loop","repetitions","_loopCount","_startTime","_effectiveTimeScale","_effectiveWeight","_cacheIndex","_byClipCacheIndex","_restoreTimeScale"})entry[k]=encode(action.get(k));
        const auto local=action.get("_localRoot");entry["localRoot"]=local.isNull()?JsonValue(nullptr):encode(local.get("uuid"));
        JsonValue::Array bindings,aliases;auto list=action.get("_propertyBindings"),interpolants=action.get("_interpolants");
        for(std::uint32_t j=0;j<list.length();++j){const auto b=at(list,j);bindings.push_back(encode(id(bindingIds,b)));aliases.emplace_back(at(interpolants,j).get("resultBuffer").sameValue(b.get("buffer")));}
        entry["bindings"]=bindings;entry["sharedResult"]=aliases;entry["settings"]=encode(action.get("_interpolantSettings"));actionStates.emplace_back(entry);
    }
    out["actions"]=actionStates;JsonValue::Array order;const auto active=mixer.get("_actions");for(std::uint32_t i=0;i<active.length();++i)order.push_back(encode(id(actions,at(active,i))));out["actionOrder"]=order;
    JsonValue::Array bindingStates;const auto bindings=mixer.get("_bindings");for(std::uint32_t i=0;i<bindings.length();++i){const auto b=at(bindings,i);bindingStates.emplace_back(JsonValue::Object{{"id",encode(id(bindingIds,b))},{"cache",encode(b.get("_cacheIndex"))},{"use",encode(b.get("useCount"))},{"refs",encode(b.get("referenceCount"))},{"buffer",numericArray(b.get("buffer"))}});}out["bindings"]=bindingStates;
    JsonValue::Array controls;const auto cs=mixer.get("_controlInterpolants");for(std::uint32_t i=0;i<cs.length();++i){const auto c=at(cs,i);controls.emplace_back(JsonValue::Object{{"cache",encode(c.get("__cacheIndex"))},{"times",numericArray(c.get("parameterPositions"))},{"values",numericArray(c.get("sampleValues"))},{"result",numericArray(c.get("resultBuffer"))},{"shared",c.get("resultBuffer").sameValue(at(cs,0).get("resultBuffer"))}});}out["controls"]=controls;
    if(!group.isUndefined()){out["groupStats"]=encode(group.get("stats"));JsonValue::Array order;const auto os=group.get("_objects");for(std::uint32_t i=0;i<os.length();++i)order.push_back(encode(at(os,i).get("uuid")));out["groupOrder"]=order;}
    return out;
}
JsonValue run(JSValue request){
    if(request.hasOwn("metadata"))return prototypeMetadata();
    if(request.hasOwn("forIn"))return runEnumeration(request);
    std::vector<std::shared_ptr<Mesh>> nativeRoots;auto roots=JSValue::array();
    for(int i=0;i<3;++i){auto root=std::make_shared<Mesh>();root->uuid="root"+std::to_string(i);root->name="root"+std::to_string(i);root->position.set(1,2,3);root->morphTargetInfluences=SharedArray<double>({.1,.2});root->morphTargetDictionary=std::make_shared<std::map<std::string,std::size_t>>(std::map<std::string,std::size_t>{{"smile",0},{"frown",1}});auto mat=std::dynamic_pointer_cast<MeshBasicMaterial>(root->material);mat->opacity=.7;mat->color.setRGB(.2,.4,.6);auto wrapped=nativeObject(root);wrapped.set("label","initial");push(roots,wrapped);nativeRoots.push_back(root);}
    if(request.get("hierarchy").toBoolean())nativeRoots[0]->add(nativeRoots[2]);
    auto clips=JSValue::array();std::vector<std::shared_ptr<AnimationClip>> nativeClips;
    const auto source=request.get("clips");for(std::uint32_t i=0;i<source.length();++i){auto clip=AnimationClip::parse(at(source,i));push(clips,nativeAnimationClip(clip));nativeClips.push_back(clip);}
    for(auto& root:nativeRoots)for(auto& clip:nativeClips)root->animations.push_back(clip);
    JSValue group;auto root=at(roots,0);if(request.get("group").toBoolean()){group=AnimationObjectGroup({at(roots,0),at(roots,1)}).object();group.set("uuid","group");root=group;}
    AnimationMixer handle(root);auto mixer=handle.object(),slots=JSValue::object(),actions=JSValue::array(),bindingIds=JSValue::array(),events=JSValue::array(),listenerState=JSValue::object();listenerState.set("fired",false);
    auto listener=JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{
        const auto event=args[0],action=event.get("action");auto e=JSValue::object();for(const char* key:{"type","direction","loopDelta"})e.set(key,event.get(key));e.set("action",id(c[0],action));e.set("time",action.get("time"));e.set("enabled",action.get("enabled"));e.set("target",event.get("target").sameValue(self));push(c[1],e);
        auto state=c[2];if(!state.get("fired").toBoolean()){state.set("fired",true);const auto op=c[3];
            if(op.strictEquals("stopSelf"))call(action,"stop");else if(op.strictEquals("stopAll"))call(self,"stopAllAction");
            else if(op.strictEquals("playOther"))call(c[4].get("b"),"play");else if(op.strictEquals("uncacheSelf"))call(self,"uncacheAction",{action.get("_clip"),action.get("_localRoot")});
            else if(op.strictEquals("update"))call(self,"update",{.125});
        }
        self.collectGarbage();return {};
    },{actions,events,listenerState,request.get("eventOp"),slots});
    handle.addEventListener("loop",listener);handle.addEventListener("finished",listener);
    JsonValue::Array states;const auto ops=request.get("operations");
    for(std::uint32_t i=0;i<ops.length();++i){const auto op=at(ops,i);const auto kind=op.get("op").toString();JsonValue result;
        try{
            auto clip=op.get("clip").type()==JSValue::Type::String?op.get("clip"):at(clips,std::size_t(op.get("clip").isUndefined()?0:op.get("clip").toNumber()));
            const auto local=op.get("root").isUndefined()?JSValue{}:at(roots,std::size_t(op.get("root").toNumber()));
            if(kind=="action"||kind=="existing"){
                const auto a=call(mixer,kind=="action"?"clipAction":"existingAction",{clip,local,op.get("blend")});
                if(op.hasOwn("slot"))slots.set(op.get("slot").toString(),a);result=encode(id(actions,a));
            }else if(kind=="actionMethod"){
                const auto a=slots.get(op.get("slot").toString());Args args;const auto input=op.get("args");if(input.isObject())for(std::uint32_t j=0;j<input.length();++j){const auto value=at(input,j);args.push_back(value.isObject()&&value.hasOwn("action")?slots.get(value.get("action").toString()):value);}
                const auto value=call(a,op.get("method").string().c_str(),args);result=value.isObject()?encode(id(actions,value)):encode(value);
            }else if(kind=="set"){
                auto target=op.hasOwn("slot")?slots.get(op.get("slot").toString()):mixer;const auto values=op.get("values");for(const auto& k:values.keys())target.set(k,values.get(k));result="$undefined";
            }else if(kind=="uncacheClip"){call(mixer,"uncacheClip",{clip});result="$undefined";}
            else if(kind=="uncacheAction"){call(mixer,"uncacheAction",{clip,local});result="$undefined";}
            else if(kind=="uncacheRoot"){call(mixer,"uncacheRoot",{local.isUndefined()?root:local});result="$undefined";}
            else if(kind=="groupAdd"||kind=="groupRemove"||kind=="groupUncache"){call(group,kind=="groupAdd"?"add":kind=="groupRemove"?"remove":"uncache",{local});result="$undefined";}
            else if(kind=="gc"){mixer.collectGarbage();result="$undefined";}
            else{const auto value=call(mixer,kind.c_str(),op.hasOwn("value")?Args{op.get("value")}:Args{});result=value.sameValue(mixer)?JsonValue("$mixer"):encode(value);}
        }catch(const JSTypeError&){result=JsonValue::Object{{"error","TypeError"}};}catch(const JSRangeError&){result=JsonValue::Object{{"error","RangeError"}};}
        states.emplace_back(JsonValue::Object{{"result",result},{"state",snapshot(mixer,roots,actions,bindingIds,group)}});
    }
    return JsonValue::Object{{"steps",states},{"events",encode(events)}};
}
}
int main(){try{const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto cases=decode(Animation_detail::parseJSON(input));JsonValue::Array out;
    for(std::uint32_t i=0;i<cases.length();++i){try{out.push_back(run(at(cases,i)));}catch(const JSTypeError&){out.emplace_back(JsonValue::Object{{"error","TypeError"}});}catch(const JSRangeError&){out.emplace_back(JsonValue::Object{{"error","RangeError"}});}}
    std::cout<<JsonValue(out).dump()<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
