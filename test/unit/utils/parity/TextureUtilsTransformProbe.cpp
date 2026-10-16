#include "TextureUtilsFixture.h"
#include <algorithm>
using namespace TextureUtilsTest;
namespace {
void step(const JSValue& state, const std::string& event) {
    const auto trace = state.get("trace");
    log(trace, {event});
    const auto count = state.get("steps").toNumber() + 1;
    auto mutableState = state;
    mutableState.set("steps", count);
    const auto spec = state.get("spec");
    if (spec.get("gcAt").strictEquals(count)) state.collectGarbage();
    if (spec.get("swapAt").strictEquals(count)) {
        const auto storage = state.get("storage");
        auto texture = storage.get("texture");
        texture.set("repeat", state.get("objects").get("newRepeat"));
        texture.set("offset", state.get("objects").get("newOffset"));
    }
    if (spec.get("throwAt").strictEquals(count)) throw JSRangeError("fixture step");
    if (spec.get("reenterAt").strictEquals(count) && !state.get("reentered").toBoolean()) {
        mutableState.set("reentered",true);
        const auto texture=state.get("objects").get("texture"),aspect=spec.get("reenterAspect");
        if(spec.get("op").strictEquals("contain"))contain(texture,aspect);
        else if(spec.get("op").strictEquals("cover"))cover(texture,aspect);
        else fill(texture);
    }
}
JSValue watched(const JSValue& state, const std::string& name, const JSValue& initial) {
    if (!initial.isObject()) return initial;
    auto target = JSValue::object(), backing = JSValue::object();
    auto storage = state.get("storage"), objects = state.get("objects");
    property(storage, name, backing);
    property(objects, name, target);
    for (const auto& key : initial.keys()) {
        property(backing, key, initial.get(key));
        JSPropertyDescriptor descriptor;
        descriptor.configurable = descriptor.enumerable = true;
        descriptor.getter = JSValue::function([](const JSValue&, const Args&, const Args& captures) {
            step(captures[0], "get " + captures[1].string() + "." + captures[2].string());
            return captures[0].get("storage").get(captures[1].string()).get(captures[2].string());
        }, {state, name, key});
        const auto readonly = state.get("spec").get("readonly");
        if (!readonly.strictEquals(name + "." + key)) {
            descriptor.setter = JSValue::function([](const JSValue&, const Args& args, const Args& captures) {
                step(captures[0], "set " + captures[1].string() + "." + captures[2].string());
                auto object = captures[0].get("storage").get(captures[1].string());
                object.set(captures[2].string(), argument(args, 0));
                return JSValue{};
            }, {state, name, key});
        }
        target.defineProperty(key, descriptor);
    }
    return target;
}
JSValue nativeFunction(const std::string& name) {
    if (name == "contain") return nativeTextureContain();
    if (name == "cover") return nativeTextureCover();
    if (name == "fill") return nativeTextureFill();
    return nativeTextureByteLength();
}
JsonValue transformScenario(const JSValue& spec) {
    auto state = JSValue::object();
    property(state,"trace",JSValue::array()); property(state,"steps",0);
    property(state,"spec",spec); property(state,"storage",JSValue::object()); property(state,"objects",JSValue::object());
    const auto trace = state.get("trace");
    auto initial = decode(spec.get("texture"),trace);
    JSValue texture = initial;
    if (spec.get("watch").toBoolean() && initial.isObject()) {
        for (const auto* name : {"image","repeat","offset"}) {
            if (initial.hasOwn(name)) initial.set(name, watched(state,name,initial.get(name)));
        }
        auto spare = JSValue::object(); property(spare,"x",17);property(spare,"y",19);
        watched(state,"newRepeat",spare);watched(state,"newOffset",spare);
        texture = watched(state,"texture",initial);
    }
    auto aspect = decode(spec.get("aspect"),trace);
    if (spec.hasOwn("aspectValues")) {
        aspect = JSValue::object();
        property(state,"aspectIndex",0);
        JSPropertyDescriptor d;d.configurable=d.enumerable=true;
        d.getter=JSValue::function([](const JSValue&,const Args&,const Args& captures) {
            step(captures[0],"get aspect conversion");
            return JSValue::function([](const JSValue&,const Args& args,const Args& c) {
                step(c[0],"convert aspect " + argument(args,0).toString());
                const auto values=c[0].get("spec").get("aspectValues");
                const auto index=c[0].get("aspectIndex").toNumber();
                auto state=c[0];state.set("aspectIndex",index+1);
                return decode(values.get(JSValue(std::min(index,values.get("length").toNumber()-1)).toString()),c[0].get("trace"));
            },captures);
        },{state});
        aspect.defineProperty(JSValue::wellKnownSymbol("toPrimitive"),d);
    }
    JsonValue result;
    const auto op=spec.get("op").string(),entry=spec.get("entry").toString();
    try {
        JSValue value;
        if(entry=="class") value=nativeTextureUtils().get(op).call(nullptr,{texture,aspect});
        else if(entry=="construct") value=nativeFunction(op).construct({texture,aspect});
        else if(entry=="direct") value=op=="contain"?contain(texture,aspect):op=="cover"?cover(texture,aspect):fill(texture);
        else value=nativeFunction(op).call(nullptr,{texture,aspect});
        result=O{{"same",value.sameValue(texture)}};
    } catch(const JSTypeError&){result=O{{"error","TypeError"}};}
      catch(const JSRangeError&){result=O{{"error","RangeError"}};}
      catch(const std::exception& error){result=O{{"error","Error"},{"message",error.what()}};}
    O values;
    if(spec.get("watch").toBoolean() && initial.isObject()) {
        for(const auto* name:{"repeat","offset","newRepeat","newOffset"}) {
            if(state.get("storage").hasOwn(name)) values[name]=encode(state.get("storage").get(name));
        }
    } else if(texture.isObject()) {
        for(const auto* name:{"repeat","offset"})values[name]=encode(texture.get(name));
    }
    return O{{"name",spec.get("name").toString()},{"result",result},{"values",values},{"trace",encode(trace)}};
}
JsonValue functionInfo(const JSValue& function) {
    A names,descriptors;
    for(const auto& name:function.ownPropertyNames()) {
        names.push_back(name);const auto d=function.getOwnPropertyDescriptor(name);
        descriptors.push_back(A{d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)});
    }
    return O{{"name",function.get("name").toString()},{"length",function.get("length").toNumber()},
        {"source",function.get("toString").call(function).toString()},{"own",names},{"descriptors",descriptors},{"constructible",function.isConstructible()}};
}
JsonValue metadata() {
    O functions;
    const auto ctor=nativeTextureUtils();
    for(const auto* name:{"contain","cover","fill","getByteLength"}) {
        functions[name]=functionInfo(nativeFunction(name));
        functions[std::string("static_")+name]=functionInfo(ctor.get(name));
    }
    functions["TextureUtils"]=functionInfo(ctor);
    const auto instance=ctor.construct(),prototype=ctor.get("prototype");
    A keys;for(const auto& key:prototype.ownPropertyNames())keys.push_back(key);
    const auto d=prototype.getOwnPropertyDescriptor("constructor");
    bool callable=true;try{ctor.call();}catch(const JSTypeError&){callable=false;}
    auto other=JSValue::constructor([](const JSValue&,const Args&,const Args&){return JSValue{};});
    const auto result=ctor.construct({},other);
    A instanceNames;for(const auto& name:instance.ownPropertyNames())instanceNames.push_back(name);
    return O{{"functions",functions},{"instanceOwn",instanceNames},
        {"instancePrototype",instance.getPrototype().sameValue(prototype)},{"prototypeOwn",keys},
        {"constructorDescriptor",A{d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)}},
        {"prototypeConstructor",prototype.get("constructor").sameValue(ctor)},{"callable",callable},
        {"newTarget",result.getPrototype().sameValue(other.get("prototype"))},
        {"separate",!nativeTextureContain().sameValue(ctor.get("contain"))},
        {"shared",nativeTextureUtils().sameValue(ctor)&&nativeTextureContain().sameValue(nativeTextureContain())}};
}
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    try {
        auto keeper=JSValue::object();
        std::ifstream input(argv[1],std::ios::binary);std::string line;
        while(std::getline(input,line)) {
            const auto spec=Animation_detail::parseJSON(line);
            const auto result=spec.get("mode").strictEquals("metadata")?metadata():transformScenario(spec);
            std::cout<<result.dump()<<'\n';JSValue::nativeSafepoint();
        }
        return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
