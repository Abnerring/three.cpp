#include "renderers/webgl/WebGLAttributes.h"
#include "core/NativeArray.h"
#include "core/JSWeakMap.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args = std::vector<JSValue>; using O = JsonValue::Object; using A = JsonValue::Array;
void data(JSValue object, const std::string& key, JSValue value) {
    JSPropertyDescriptor d; d.value = value; d.writable = d.enumerable = d.configurable = true; object.defineProperty(key, d);
}
JSValue array(Args values) { auto result = JSValue::array(); for (std::size_t i = 0; i < values.size(); ++i) result.set(std::to_string(i), values[i]); return result; }
void push(JSValue values, JSValue value) { values.set(std::to_string(values.length()), value); }
JSValue call(JSValue self, const char* key, Args args = {}) { const auto f = self.get(key); return f.call(self, args); }
JsonValue encode(const JSValue& value) {
    if (value.isUndefined()) return "undefined"; if (value.isNull()) return nullptr;
    if (value.type() == JSValue::Type::Boolean) return value.toBoolean(); if (value.type() == JSValue::Type::String) return value.string();
    if (value.type() == JSValue::Type::Number) { auto n = value.number(); if (std::isnan(n)) return "NaN"; if (std::isinf(n)) return n < 0 ? "-Infinity" : "Infinity"; if (n == 0 && std::signbit(n)) return "-0"; return n; }
    if (value.isCallable()) return "function";
    if (value.isHostObject("three.AnimationArray")) {
        auto native = NativeArray_detail::unwrap(value); A result;
        for (std::size_t i = 0; i < native.size(); ++i) result.push_back(encode(native.get(double(i))));
        return O{{"typed", value.getHostSlot("kind").number()}, {"values", result}};
    }
    if (value.type() == JSValue::Type::Array) { A result; for (unsigned i = 0; i < value.length(); ++i) result.push_back(encode(value.get(std::to_string(i)))); return result; }
    O result; for (const auto& key : value.keys()) { const auto d = value.getOwnPropertyDescriptor(key); result[key] = d && d->value ? encode(*d->value) : JsonValue("accessor"); } return result;
}
JSValue frozen(JSValue value) { return JSValue::fromJSON(encode(value)); }
template<class F> JsonValue invoke(F&& fn) { try { return encode(fn()); } catch (const JSRangeError&) { return O{{"error", "RangeError"}}; } catch (const JSTypeError&) { return O{{"error", "TypeError"}}; } catch (const std::exception& e) { return O{{"error", "Error"}, {"message", e.what()}}; } }
JSValue typed(unsigned kind, unsigned length = 8) { auto values = JSValue::array(); for (unsigned i = 0; i < length; ++i) values.set(std::to_string(i), i + .25); return NativeArray_detail::constructor(kind).construct({values}); }
JSValue range(JSValue start, JSValue count) { auto r = JSValue::object(); data(r, "start", start); data(r, "count", count); return r; }
Args scalars() { return {{}, nullptr, false, true, 0, -0.0, -1, 1, 2, 10, "1", "2", "10", "", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), "\xF0\x9F\x98\x80", "\xEE\x80\x80"}; }
void log(JSValue state, Args values) { push(state.get("log"), array(std::move(values))); }
JSValue gpu(const JSValue& self, const Args& args, const Args& captures) {
    auto state = captures[0]; const auto name = captures[1].string(); const auto behavior = state.get("behavior").number();
    Args line{name, self.sameValue(state.get("gl"))}; for (const auto& arg : args) line.push_back(frozen(arg)); log(state, line);
    auto attribute = state.get("attribute");
    if (name == "createBuffer") { const auto n = state.get("nextBuffer").number() + 1; state.set("nextBuffer", n); return behavior == 12 ? JSValue(nullptr) : JSValue(n); }
    if (name == "bufferData" && behavior == 11) attribute.set("array", typed(3));
    if (name == "bufferSubData" && behavior == 7) throw JSRangeError("subdata");
    if (name == "bufferSubData" && behavior == 15) attribute.set("updateRanges", array({range(6,1)}));
    if (name == "deleteBuffer" && behavior == 10 && !state.get("busy").toBoolean()) {
        state.set("busy", true); attribute.set("version", attribute.get("version").toNumber() + 1);
        call(state.get("manager"), "update", {attribute, 99}); state.set("busy", false);
    }
    return {};
}
JSValue upload(const JSValue& self, const Args&, const Args& captures) {
    auto state = captures[0]; const auto behavior = state.get("behavior").number();
    log(state, {"onUpload", self.sameValue(state.get("attribute"))}); auto attribute = state.get("attribute");
    if (behavior == 1) attribute.set("version", attribute.get("version").toNumber() + 1);
    if (behavior == 2) attribute.set("array", typed(3));
    if (behavior == 3) state.get("original").setPrototype(NativeArray_detail::constructor(3).get("prototype"));
    if (behavior == 4) throw JSRangeError("upload");
    if (behavior == 5) call(state.get("manager"), "remove", {attribute});
    if ((behavior == 6 || behavior == 14) && !state.get("busy").toBoolean()) {
        state.set("busy", true); if (behavior == 14) attribute.set("version", attribute.get("version").toNumber() + 1);
        call(state.get("manager"), "update", {attribute, 77}); state.set("busy", false);
    }
    if (behavior == 16) attribute.set("isFloat16BufferAttribute", true);
    return {};
}
JSValue clear(const JSValue& self, const Args&, const Args& captures) {
    auto state = captures[0]; log(state, {"clear", self.sameValue(state.get("attribute"))});
    if (state.get("behavior").number() == 8) throw JSRangeError("clear");
    auto ranges = self.get("updateRanges"); ranges.set("length", 0);
    if (state.get("behavior").number() == 9) { auto attr = self; attr.set("version", attr.get("version").toNumber() + 1); }
    return {};
}
struct Fixture {
    JSValue state, attribute, gl, manager;
    Fixture(unsigned kind = 6, unsigned behavior = 0) : state(JSValue::object(nullptr)), attribute(JSValue::object()), gl(JSValue::object()) {
        data(state, "log", JSValue::array()); data(state, "gl", gl); data(state, "attribute", attribute); data(state, "behavior", behavior); data(state, "nextBuffer", 0); data(state, "busy", false);
        for (const char* name : {"createBuffer","bindBuffer","bufferData","bufferSubData","deleteBuffer"}) data(gl, name, JSValue::function(gpu, {state, name}, name, 0));
        unsigned i = 0; for (const char* name : {"FLOAT","HALF_FLOAT","UNSIGNED_SHORT","SHORT","UNSIGNED_INT","INT","BYTE","UNSIGNED_BYTE"}) data(gl, name, 100 + i++);
        auto values = typed(kind); data(attribute, "array", values); data(state, "original", values); data(attribute, "usage", 35044); data(attribute, "version", 0); data(attribute, "updateRanges", JSValue::array());
        data(attribute, "onUploadCallback", JSValue::function(upload, {state}, "", 0)); data(attribute, "clearUpdateRanges", JSValue::function(clear, {state}, "", 0));
        manager = WebGLAttributes(gl).object(); data(state, "manager", manager);
    }
    JSValue update() { return call(manager, "update", {attribute, 34962}); }
    JsonValue cache() { return encode(call(manager, "get", {attribute})); }
    JsonValue trace() { return encode(state.get("log")); }
};
JsonValue typedCase(unsigned kind, unsigned flag) {
    Fixture f(kind); if (flag) data(f.attribute, "isFloat16BufferAttribute", flag == 1 ? JSValue(true) : JSValue("half"));
    auto result = invoke([&] { return f.update(); }); return O{{"kind","typed"},{"type",kind},{"flag",flag},{"result",result},{"cache",f.cache()},{"trace",f.trace()}};
}
JsonValue versionCase(unsigned first, unsigned second, bool raw) {
    Fixture f; const auto values = scalars(); f.attribute.set("version", values[first]);
    if (raw) { data(f.attribute,"isGLBufferAttribute",true); data(f.attribute,"buffer",55); data(f.attribute,"type","kind"); data(f.attribute,"elementSize","4"); }
    const auto before = invoke([&] { return f.update(); }); const auto cache = call(f.manager,"get",{f.attribute});
    f.attribute.set("version",values[second]); if(raw)f.attribute.set("buffer",56);
    const auto after = invoke([&] { return f.update(); });
    return O{{"kind","version"},{"first",first},{"second",second},{"raw",raw},{"before",before},{"after",after},{"same",cache.sameValue(call(f.manager,"get",{f.attribute}))},{"cache",f.cache()},{"trace",f.trace()}};
}
JsonValue rangeCase(unsigned scenario) {
    Fixture f; f.update(); f.state.get("log").set("length",0); auto ranges = JSValue::array();
    switch(scenario) {
    case 0:break;
    case 1:ranges=array({range(6,1),range(1,2),range(3,2),range(7,1)});break;
    case 2:ranges=array({range(1,1),range(3,1)});break;
    case 3:ranges=array({range(0,1),range(3,1),range(6,1)});break;
    case 4:ranges=array({range("0","1"),range("3","2"),range("6","1")});break;
    case 5:ranges=array({range(-0.0,-0.0),range(0,0)});break;
    case 6:ranges=array({range(std::numeric_limits<double>::quiet_NaN(),2),range(1,2)});break;
    case 7:ranges=array({range(2,-1),range(0,1),range(5,2)});break;
    case 8:ranges=array({range(.5,.25),range(1.75,.5)});break;
    case 9:ranges=array({range(1,2),JSValue{},range(0,1)});break;
    case 10:ranges=JSValue::array(4);ranges.set("0",range(3,1));ranges.set("3",range(0,1));break;
    case 11:ranges=array({range(5,1),range(0,1)});ranges.preventExtensions();break;
    case 12:ranges=array({range(0,1),range(0,2),range(0,3)});break;
    default:for(unsigned i=0;i<40;++i)push(ranges,range((i*17+scenario)%71,i%3));break;
    }
    f.attribute.set("updateRanges",ranges);f.attribute.set("version",1);
    const auto result=invoke([&]{return f.update();});return O{{"kind","ranges"},{"scenario",scenario},{"result",result},{"ranges",encode(ranges)},{"cache",f.cache()},{"trace",f.trace()}};
}
JsonValue behaviorCase(unsigned behavior,bool update) {
    Fixture f(behavior==16?3:6,update?0:behavior);JsonValue initial="not-run";
    if(update){f.update();f.state.get("log").set("length",0);f.state.set("behavior",behavior);f.attribute.set("version",1);f.attribute.set("updateRanges",array({range(0,1),range(5,1)}));}
    auto result=invoke([&]{return f.update();});const auto cached=f.cache();
    JsonValue removed="not-run";if(behavior==10)removed=invoke([&]{return call(f.manager,"remove",{f.attribute});});
    return O{{"kind","behavior"},{"behavior",behavior},{"update",update},{"result",result},{"cached",cached},{"removed",removed},{"cache",f.cache()},{"version",encode(f.attribute.get("version"))},{"ranges",encode(f.attribute.get("updateRanges"))},{"trace",f.trace()}};
}
JsonValue interleavedCase(unsigned scenario) {
    Fixture f;auto a=JSValue::object(),b=JSValue::object();data(a,"isInterleavedBufferAttribute",true);data(a,"data",f.attribute);data(b,"isInterleavedBufferAttribute","yes");data(b,"data",f.attribute);
    const auto result=invoke([&]{return call(f.manager,"update",{a,34962});});const auto first=call(f.manager,"get",{a});const auto same=first.sameValue(call(f.manager,"get",{b}));
    if(scenario==1){f.attribute.set("version",1);call(f.manager,"update",{b,34962});}
    if(scenario==2){auto nested=JSValue::object();data(nested,"isInterleavedBufferAttribute",true);data(nested,"data",a);return O{{"kind","interleaved"},{"scenario",scenario},{"result",result},{"same",same},{"nested",invoke([&]{return call(f.manager,"update",{nested,34962});})},{"trace",f.trace()}};}
    const auto removed=invoke([&]{return call(f.manager,"remove",{b});});return O{{"kind","interleaved"},{"scenario",scenario},{"result",result},{"same",same},{"removed",removed},{"cache",f.cache()},{"trace",f.trace()}};
}
JsonValue resizeCase(unsigned scenario){Fixture f;f.update();f.state.get("log").set("length",0);f.attribute.set("version",1);
    if(scenario==0)f.attribute.set("array",typed(6,9));
    if(scenario==1)f.attribute.set("array",typed(3,16));
    if(scenario>=2){JSPropertyDescriptor d;d.value=scenario==2?JSValue("32"):JSValue(32);d.writable=d.configurable=true;f.state.get("original").defineProperty("byteLength",d);}
    auto result=invoke([&]{return f.update();});return O{{"kind","resize"},{"scenario",scenario},{"result",result},{"cache",f.cache()},{"trace",f.trace()}};
}
void watch(JSValue object,const char* key,JSValue state,const std::string& label){
    auto storage=JSValue::object(nullptr);data(storage,"value",object.get(key));JSPropertyDescriptor d;
    d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& cap)->JSValue{
        log(cap[0],{"get",cap[1],self.sameValue(cap[2])});return cap[3].get("value");
    },{state,label,object,storage});
    d.setter=JSValue::function(+[](const JSValue& self,const Args& args,const Args& cap)->JSValue{
        log(cap[0],{"set",cap[1],self.sameValue(cap[2]),frozen(args[0])});auto storage=cap[3];storage.set("value",args[0]);return {};
    },{state,label,object,storage});d.enumerable=d.configurable=true;object.defineProperty(key,d);
}
JsonValue orderCase(unsigned scenario){Fixture f(3);if(scenario){f.update();f.attribute.set("version",1);f.attribute.set("updateRanges",array({range(5,1),range(0,1),range(1,1)}));}
    if(scenario==2){auto cached=call(f.manager,"get",{f.attribute});for(const char* name:{"version","size","buffer"})watch(cached,name,f.state,std::string("cached.")+name);}
    for(const char* name:{"isInterleavedBufferAttribute","isGLBufferAttribute","array","usage","version","onUploadCallback","updateRanges","clearUpdateRanges","isFloat16BufferAttribute"})watch(f.attribute,name,f.state,std::string("attribute.")+name);
    for(const char* name:{"createBuffer","bindBuffer","bufferData","bufferSubData","deleteBuffer","UNSIGNED_SHORT","HALF_FLOAT"})watch(f.gl,name,f.state,std::string("gl.")+name);
    for(const char* name:{"byteLength","BYTES_PER_ELEMENT"})watch(f.state.get("original"),name,f.state,std::string("array.")+name);
    auto ranges=f.attribute.get("updateRanges");for(unsigned i=0;i<ranges.length();++i)for(const char* name:{"start","count"})watch(ranges.get(std::to_string(i)),name,f.state,"range"+std::to_string(i)+"."+name);
    f.state.get("log").set("length",0);const auto result=invoke([&]{return f.update();});const auto cached=f.cache();return O{{"kind","order"},{"scenario",scenario},{"result",result},{"cache",cached},{"trace",f.trace()}};
}
struct Restore { JSValue object;std::vector<std::pair<std::string,std::optional<JSPropertyDescriptor>>> saved;
    explicit Restore(JSValue value):object(value){}void save(const std::string& name){saved.emplace_back(name,object.getOwnPropertyDescriptor(name));}
    ~Restore(){for(const auto&[name,d]:saved)if(d)object.defineProperty(name,*d);else object.erase(name);}
};
JsonValue weakMethodCase(unsigned scenario){Fixture f;auto prototype=nativeWeakMapConstructor().get("prototype");Restore restore(prototype);auto registry=JSValue::array();
    for(const char* name:{"get","set","delete"}){restore.save(name);auto original=prototype.get(name);auto replacement=JSValue::function(+[](const JSValue& self,const Args& args,const Args& cap)->JSValue{
        const auto state=cap[0],name=cap[1],original=cap[2];const auto scenario=cap[3].number();log(state,{"weak-call",name,isNativeWeakMap(self)});
        if((scenario==1&&name.string()=="get")||(scenario==3&&name.string()=="set")||(scenario==5&&name.string()=="delete"))throw JSRangeError("weak-call");
        if(scenario==7&&name.string()=="get")return nullptr;if(scenario==8&&name.string()=="get")return false;
        return original.call(self,args);
    },{f.state,name,original,scenario});
    JSPropertyDescriptor d;d.configurable=true;
    if(scenario==0||scenario==2||scenario==4||scenario==6){d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& cap)->JSValue{
        log(cap[0],{"weak-get",cap[1],isNativeWeakMap(self)});const auto n=cap[3].number();
        if((n==2&&cap[1].string()=="set")||(n==4&&cap[1].string()=="get"))throw JSRangeError("weak-get");
        if(n==6&&cap[1].string()=="set")return 17;return cap[2];
    },{f.state,name,replacement,scenario});}else{d.value=replacement;d.writable=true;}prototype.defineProperty(name,d);}
    const auto result=invoke([&]{return f.update();});const auto removed=invoke([&]{return call(f.manager,"remove",{f.attribute});});
    return O{{"kind","weak-method"},{"scenario",scenario},{"result",result},{"removed",removed},{"trace",f.trace()}};
}
JsonValue pollutionCase(unsigned scenario){Fixture f;auto prototype=JSValue::object().getPrototype();Restore restore(prototype);
    for(const char*name:{"get","remove","update","buffer","type","bytesPerElement","version","size"}){restore.save(name);JSPropertyDescriptor d;d.configurable=true;
        if(scenario){d.value=77;d.writable=false;}else{d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&cap)->JSValue{log(cap[0],{"pollution-get",cap[1]});return 77;},{f.state,name});d.setter=JSValue::function(+[](const JSValue&,const Args&,const Args&cap)->JSValue{log(cap[0],{"pollution-set",cap[1]});return {};},{f.state,name});}prototype.defineProperty(name,d);}
    f.manager=WebGLAttributes(f.gl).object();f.state.set("manager",f.manager);const auto result=invoke([&]{return f.update();});const auto cached=f.cache();A descriptors;
    for(const auto&name:f.manager.keys()){const auto d=f.manager.getOwnPropertyDescriptor(name);descriptors.emplace_back(A{name,*d->writable,*d->enumerable,*d->configurable});}
    return O{{"kind","pollution"},{"scenario",scenario},{"result",result},{"cache",cached},{"descriptors",descriptors},{"trace",f.trace()}};
}
JSValue convertValue(JSValue state,const char*label,JSValue result,bool throws=false){auto value=JSValue::object();value.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args&args,const Args&cap)->JSValue{
    log(cap[0],{"convert",cap[1],args[0]});if(cap[3].toBoolean())throw JSRangeError("convert");return cap[2];
},{state,label,result,throws}));return value;}
JsonValue coercionCase(unsigned scenario){Fixture f;JsonValue initial="not-run";
    if(scenario<=3){f.attribute.set("version",convertValue(f.state,"old",scenario==3?JSValue("2"):JSValue(0),scenario==1));f.update();f.state.get("log").set("length",0);f.attribute.set("version",convertValue(f.state,"new",scenario==3?JSValue("10"):JSValue(1),scenario==2));}
    else if(scenario<=9){f.update();f.state.get("log").set("length",0);f.attribute.set("version",1);auto ranges=JSValue::array();
        auto first=range(convertValue(f.state,"a.start",scenario==9?JSValue("0"):JSValue(0),scenario==4),convertValue(f.state,"a.count",scenario==9?JSValue("1"):JSValue(1),scenario==6));
        auto second=range(convertValue(f.state,"b.start",scenario==9?JSValue("2"):JSValue(2),scenario==5),convertValue(f.state,"b.count",scenario==9?JSValue("1"):JSValue(1),scenario==7));
        ranges.set("0",first);ranges.set("1",second);f.attribute.set("updateRanges",ranges);
        if(scenario==8){JSPropertyDescriptor d;d.value=convertValue(f.state,"bytes",4);d.configurable=true;f.state.get("original").defineProperty("BYTES_PER_ELEMENT",d);}
    }else{auto unsupported=convertValue(f.state,"format","unsupported",scenario==11);data(unsupported,"byteLength",32);f.attribute.set("array",unsupported);}
    auto result=invoke([&]{return f.update();});return O{{"kind","coercion"},{"scenario",scenario},{"result",result},{"cache",f.cache()},{"trace",f.trace()}};
}
JsonValue functionMetadata(JSValue function){A names;for(const auto&name:function.ownPropertyNames())names.emplace_back(name);const auto pd=function.getOwnPropertyDescriptor("prototype");
    return O{{"name",encode(function.get("name"))},{"length",encode(function.get("length"))},{"constructible",function.isConstructible()},{"own",names},
        {"prototypeDescriptor",pd?JsonValue(A{pd->writable.value_or(false),pd->enumerable.value_or(false),pd->configurable.value_or(false)}):JsonValue(nullptr)},
        {"constructorSelf",pd&&function.get("prototype").get("constructor").sameValue(function)}};
}
JsonValue metadataCase(unsigned index){Fixture f;const char*names[]={"get","remove","update"};const auto function=f.manager.get(names[index]);const auto prototype=function.get("prototype");const auto constructor=prototype.getOwnPropertyDescriptor("constructor");
    return O{{"kind","metadata"},{"index",index},{"function",functionMetadata(function)},{"constructorDescriptor",A{*constructor->writable,*constructor->enumerable,*constructor->configurable}},{"prototypeParent",prototype.getPrototype().sameValue(JSValue::object().getPrototype())}};
}
JsonValue constructCase(unsigned index,unsigned mode){Fixture f;const char*names[]={"get","remove","update"};if(mode!=0)f.update();f.state.get("log").set("length",0);const auto cached=call(f.manager,"get",{f.attribute});
    const auto function=f.manager.get(names[index]);JSValue target=function,expectedPrototype=function.get("prototype");
    if(mode>=2){target=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Other",0,JSValue::ConstructorKind::Callable);expectedPrototype=mode==2?JSValue::object():JSValue::object().getPrototype();target.set("prototype",mode==2?expectedPrototype:JSValue(nullptr));}
    if(mode==4){auto data=JSValue::object();::data(data,"isInterleavedBufferAttribute",true);::data(data,"data",f.attribute);f.attribute=data;}
    JSValue result;const auto outcome=invoke([&]{result=function.construct({f.attribute,34962},target);return result;});
    return O{{"kind","construct"},{"index",index},{"mode",mode},{"outcome",outcome},{"returnedCached",cached.isObject()&&result.sameValue(cached)},
        {"newTargetPrototype",result.isObject()&&result.getPrototype().sameValue(expectedPrototype)},{"cache",f.cache()},{"trace",f.trace()}};
}
JsonValue arrowCase(){Fixture f;f.update();f.attribute.set("version",1);auto ranges=f.attribute.get("updateRanges");ranges.set("0",range(0,1));auto captured=JSValue::object();
    data(ranges,"sort",JSValue::function(+[](const JSValue&self,const Args&args,const Args&cap)->JSValue{auto captured=cap[0];captured.set("callback",args[0]);return self;},{captured}));
    f.update();return O{{"kind","arrow"},{"function",functionMetadata(captured.get("callback"))}};
}
void emit(JsonValue value){std::cout<<value.dump()<<'\n';JSValue::object().collectGarbage();}
}
int main(){
    for(unsigned kind=0;kind<9;++kind)for(unsigned flag=0;flag<3;++flag)emit(typedCase(kind,flag));
    for(bool raw:{false,true})for(unsigned first=0;first<scalars().size();++first)for(unsigned second=0;second<scalars().size();++second)emit(versionCase(first,second,raw));
    for(unsigned i=0;i<17;++i)emit(rangeCase(i));for(bool update:{false,true})for(unsigned i=0;i<17;++i)emit(behaviorCase(i,update));
    for(unsigned i=0;i<3;++i)emit(interleavedCase(i));for(unsigned i=0;i<4;++i)emit(resizeCase(i));
    for(unsigned i=0;i<3;++i)emit(orderCase(i));for(unsigned i=0;i<9;++i)emit(weakMethodCase(i));for(unsigned i=0;i<2;++i)emit(pollutionCase(i));
    for(unsigned i=0;i<12;++i)emit(coercionCase(i));
    for(unsigned i=0;i<3;++i)emit(metadataCase(i));for(unsigned i=0;i<3;++i)for(unsigned mode=0;mode<5;++mode)emit(constructCase(i,mode));emit(arrowCase());
}
