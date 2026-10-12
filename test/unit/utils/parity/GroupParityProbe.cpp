#include "animation/AnimationObjectGroup.h"
#include "animation/PropertyBinding.h"
#include "animation/AnimationUtils.h"
#include <iostream>
#include <sstream>

namespace {
using Args=std::vector<JSValue>;
void push(JSValue array,JSValue value){array.set(std::to_string(array.length()),value);}
JSValue obj(std::initializer_list<std::pair<const char*,JSValue>> entries){auto o=JSValue::object();for(const auto&[k,v]:entries)o.set(k,v);return o;}
JSPropertyDescriptor getter(JSValue function){JSPropertyDescriptor d;d.getter=function;d.enumerable=true;d.configurable=true;return d;}
JSValue list(std::initializer_list<JSValue> values){auto a=JSValue::array();for(const auto&v:values)push(a,v);return a;}
std::string error(const std::exception&e){return dynamic_cast<const JSTypeError*>(&e)?"TypeError":dynamic_cast<const JSRangeError*>(&e)?"RangeError":"Error";}
JsonValue encode(JSValue v){
    if(v.isUndefined())return "$undefined";if(v.isCallable())return "$function";
    if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "$NaN";if(std::isinf(n))return n<0?"$-Infinity":"$Infinity";if(n==0&&std::signbit(n))return "$-0";return n;}
    if(v.type()==JSValue::Type::Array){JsonValue::Array a;for(std::uint32_t i=0;i<v.length();++i)a.push_back(v.hasOwn(std::to_string(i))?encode(v.get(std::to_string(i))):JsonValue("$hole"));return a;}
    if(v.isObject()){JsonValue::Object o;for(const auto& k:v.keys())o[k]=encode(v.get(k));return o;}
    return v.toJSON().value();
}
JSValue root(JSValue uuid){return obj({{"uuid",uuid},{"name",uuid},{"value",1},{"other",2},{"third",3},{"children",JSValue::array()}});}
JsonValue bindingArray(JSValue a){
    if(a.isUndefined())return "$undefined";JsonValue::Array result;
    for(std::uint32_t i=0;i<a.length();++i){const auto k=std::to_string(i);if(!a.hasOwn(k)){result.emplace_back("$hole");continue;}const auto b=a.get(k);
        if(b.isUndefined()){result.emplace_back("$undefined");continue;}
        result.emplace_back(JsonValue::Object{{"path",encode(b.get("path"))},{"root",encode(b.get("rootNode").get("uuid"))},{"parsed",encode(b.get("parsedPath"))}});
    }return result;
}
JsonValue state(JSValue g){
    JsonValue::Array objects,bindings;
    const auto o=g.get("_objects"),b=g.get("_bindings");for(std::uint32_t i=0;i<o.length();++i)objects.push_back(encode(o.get(std::to_string(i)).get("uuid")));
    for(std::uint32_t i=0;i<b.length();++i)bindings.push_back(b.hasOwn(std::to_string(i))?bindingArray(b.get(std::to_string(i))):JsonValue("$hole"));
    const auto stats=g.get("stats");
    return JsonValue::Object{{"objects",objects},{"cached",encode(g.get("nCachedObjects_"))},{"indices",encode(g.get("_indicesByUUID"))},
        {"paths",encode(g.get("_paths"))},{"parsed",encode(g.get("_parsedPaths"))},{"bindings",bindings},{"pathIndices",encode(g.get("_bindingsIndicesByPath"))},
        {"stats",JsonValue::Array{encode(stats.get("objects").get("total")),encode(stats.get("objects").get("inUse")),encode(stats.get("bindingsPerObject"))}}};
}
JsonValue normal(JSValue request){
    std::vector<JSValue> roots;auto uuids=request.get("uuids");for(std::uint32_t i=0;i<uuids.length();++i)roots.push_back(root(uuids.get(std::to_string(i))));
    std::vector<JSValue> initial;auto initialIds=request.get("initial");for(std::uint32_t i=0;i<initialIds.length();++i)initial.push_back(roots.at(std::size_t(initialIds.get(std::to_string(i)).number())));
    AnimationObjectGroup group(initial);auto g=group.object();JsonValue::Array results{state(g)};std::vector<JSValue> subscriptions;
    const auto actions=request.get("actions");for(std::uint32_t i=0;i<actions.length();++i){auto action=actions.get(std::to_string(i));std::string failure;JSValue returned;
        try{const auto operation=action.get("op").toString();if(operation=="subscribe") {returned=group.subscribe_(action.get("path"),action.get("parsed"));subscriptions.push_back(returned);}
            else if(operation=="unsubscribe")returned=group.unsubscribe_(action.get("path"));
            else if(operation=="set") {auto binding=PropertyBinding::create(g,action.get("path"));binding->setValue(Float64Array({action.get("value").toNumber()}),0);}
            else {std::vector<JSValue> args;auto ids=action.get("ids");for(std::uint32_t j=0;j<ids.length();++j)args.push_back(roots.at(std::size_t(ids.get(std::to_string(j)).number())));
                if(operation=="add")returned=group.add(args);else if(operation=="remove")returned=group.remove(args);else returned=group.uncache(args);}
        }catch(const std::exception&e){failure=error(e);}
        JsonValue::Array retained,values;for(const auto& a:subscriptions)retained.push_back(bindingArray(a));for(const auto& r:roots)values.push_back(encode(r.get("value")));
        results.emplace_back(JsonValue::Object{{"error",failure},{"returnUndefined",returned.isUndefined()},{"state",state(g)},{"retained",retained},{"values",values}});
    }return results;
}
JsonValue callbacks(JSValue request){
    const auto name=request.get("name").toString();auto events=JSValue::array();AnimationObjectGroup group({root("a"),root("b")});auto g=group.object();
    if(name=="constructor"){
        auto prototype=JSValue::array().getPrototype();const auto old=*prototype.getOwnPropertyDescriptor("slice");
        auto replacement=JSValue::function([](const JSValue& receiver,const Args&a,const Args&c){push(c[0],list({"slice",receiver.get("length"),receiver.get("0").get("uuid")}));return c[1].call(receiver,a);},{events,*old.value});
        const auto originalCall=replacement.get("call");replacement.defineProperty("call",getter(JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],"getCall");return c[1];},{events,originalCall})));
        prototype.defineProperty("slice",getter(JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],"getSlice");return c[1];},{events,replacement})));
        AnimationObjectGroup created({root("x"),root("y")});prototype.defineProperty("slice",old);
        return JsonValue::Object{{"events",encode(events)},{"state",state(created.object())}};
    }
    if(name=="stats"){
        group.remove({g.get("_objects").get("0")});auto counts=g.get("stats").get("objects"),other=obj({{"total",20}});
        JsonValue::Array names;for(const auto& name:g.getPrototype().ownPropertyNames())names.emplace_back(name);
        return JsonValue::Array{encode(counts.getOwnPropertyDescriptor("total")->getter->call(other)),encode(counts.getOwnPropertyDescriptor("inUse")->getter->call(other)),
            encode(g.get("stats").getOwnPropertyDescriptor("bindingsPerObject")->getter->call(other)),g.get("add").sameValue(AnimationObjectGroup().object().get("add")),g.hasOwn("add"),names};
    }
    if(name=="uuidReenter"){
        auto a=root("c");a.defineProperty("uuid",getter(JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[1],"uuid");auto state=c[0];state.collectGarbage();state.set("_objects",JSValue::array());return JSValue("c");},{g,events})));
        group.add({a});return JsonValue::Object{{"events",encode(events)},{"state",state(g)}};
    }
    if(name=="fieldOrder"){
        for(const auto* k:{"_objects","_indicesByUUID","_paths","_parsedPaths","_bindings","nCachedObjects_"}){
            auto value=g.get(k);JSPropertyDescriptor d=getter(JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],c[1]);return c[2];},{events,k,value}));
            d.setter=JSValue::function([](const JSValue&,const Args&a,const Args&c){push(c[0],list({"set",c[1],a[0]}));return JSValue();},{events,k});g.defineProperty(k,d);
        }
        group.add({});group.remove({});group.uncache({});return encode(events);
    }
    if(name=="pushOrder"){
        auto objects=g.get("_objects"),paths=g.get("_paths"),parsed=g.get("_parsedPaths"),bindings=g.get("_bindings");
        for(auto pair:std::vector<std::pair<JSValue,const char*>>{{objects,"objects"},{paths,"paths"},{parsed,"parsed"},{bindings,"bindings"}}){
            const auto original=pair.first.get("push");pair.first.defineProperty("push",getter(JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],c[1]);return c[2];},{events,pair.second,original})));
        }
        group.subscribe_(".value");group.add({root("c")});return JsonValue::Object{{"events",encode(events)},{"state",state(g)}};
    }
    if(name=="keyOrder"){
        auto key=JSValue::object();key.set("toString",JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],"key");return JSValue(".value");},{events}));
        auto parsed=PropertyBinding::parseTrackName(".value");auto bindings=group.subscribe_(key,parsed);group.unsubscribe_(key);return JsonValue::Object{{"events",encode(events)},{"state",state(g)},{"retained",bindingArray(bindings)}};
    }
    if(name=="readonlyDelete"){
        auto indices=g.get("_indicesByUUID");JSPropertyDescriptor d;d.configurable=false;indices.defineProperty("a",d);std::string failure;
        try{group.uncache({g.get("_objects").get("0")});}catch(const std::exception&e){failure=error(e);}return JsonValue::Object{{"error",failure},{"state",state(g)}};
    }
    if(name=="uuidKey"){
        auto a=root(JSValue::object());auto key=a.get("uuid");key.set("toString",JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],"uuidKey");return JSValue("c");},{events}));
        group.add({a});group.remove({a});group.add({a});group.uncache({a});return JsonValue::Object{{"events",encode(events)},{"state",state(g)}};
    }
    if(name=="bindingPushOrder"){
        auto bindings=group.subscribe_(".value");const auto original=bindings.get("push");
        bindings.defineProperty("push",getter(JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],"binding.push");return c[1];},{events,original})));
        for(const auto* field:{"_paths","_parsedPaths"}){auto array=g.get(field);const auto value=array.get("0");array.defineProperty("0",getter(JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],c[1]);return c[2];},{events,field,value})));}
        group.add({root("c")});return encode(events);
    }
    if(name=="indexObject"){
        auto a=g.get("_objects").get("0");group.remove({a});
        auto index=JSValue::object();index.set("valueOf",JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],"number");return JSValue(0);},{events}));
        index.set("toString",JSValue::function([](const JSValue&,const Args&,const Args&c){push(c[0],"string");return JSValue("0");},{events}));
        auto indices=g.get("_indicesByUUID");indices.set("a",index);group.add({a});return JsonValue::Object{{"events",encode(events)},{"state",state(g)}};
    }
    throw JSTypeError("Unknown callback scenario");
}
}
int main(){
    const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);JsonValue::Array output;
    for(std::uint32_t i=0;i<requests.length();++i){auto r=requests.get(std::to_string(i));std::ostringstream messages;auto* before=std::cerr.rdbuf(messages.rdbuf());JsonValue value;
        try{value=r.get("kind").strictEquals("callback")?callbacks(r):normal(r);}catch(const std::exception&e){value=JsonValue::Object{{"error",error(e)}};}
        std::cerr.rdbuf(before);std::istringstream lines(messages.str());JsonValue::Array log;for(std::string line;std::getline(lines,line);)log.emplace_back(line);
        output.emplace_back(JsonValue::Object{{"result",value},{"messages",log}});requests.collectGarbage();
    }std::cout<<JsonValue(output).dump()<<'\n';
}

