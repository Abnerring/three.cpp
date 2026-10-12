#include "WebGLClosureConstructibility.h"
#include "renderers/webgl/WebGLProperties.h"
#include "core/JSWeakMap.h"
#include <cmath>
#include <iostream>
#include <limits>
namespace {
using Args=std::vector<JSValue>;using O=JsonValue::Object;using A=JsonValue::Array;
JSValue array(const Args& values={}){auto result=JSValue::array();result.get("push").call(result,values);return result;}
void append(const JSValue& trace,const Args& values){trace.get("push").call(trace,{array(values)});}
JSValue call(const JSValue& object,const char* name,const Args& args={}){return object.get(name).call(object,args);}
JsonValue encode(const JSValue& value){
    if(value.isUndefined())return "undefined";if(value.isNull())return nullptr;
    if(value.type()==JSValue::Type::Boolean)return value.toBoolean();if(value.type()==JSValue::Type::String)return value.string();
    if(value.type()==JSValue::Type::Function)return "function";
    if(value.type()==JSValue::Type::Number){auto n=value.number();if(std::isnan(n))return "NaN";if(std::isinf(n))return n<0?"-Infinity":"Infinity";if(n==0&&std::signbit(n))return "-0";return n;}
    if(value.type()==JSValue::Type::Array){A result;for(std::uint32_t i=0;i<value.length();++i)result.push_back(encode(value.get(std::to_string(i))));return result;}
    O result;for(const auto& key:value.keys()){const auto descriptor=value.getOwnPropertyDescriptor(key);result[key]=descriptor->value?encode(*descriptor->value):JsonValue("accessor");}return result;
}
template<class F>JsonValue invoke(F fn){try{return encode(fn());}catch(const JSRangeError&){return O{{"error","RangeError"}};}catch(const JSTypeError&){return O{{"error","TypeError"}};}catch(const std::exception&){return O{{"error","Error"}};}}
Args scalars(){return {{},nullptr,false,true,0,-0.0,1.5,-2,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),"","3","x"};}
struct Restore {
    JSValue prototype;std::vector<std::pair<std::string,std::optional<JSPropertyDescriptor>>> saved;
    explicit Restore(JSValue p):prototype(p){}
    void remember(const std::string& key){saved.emplace_back(key,prototype.getOwnPropertyDescriptor(key));}
    ~Restore(){for(const auto& [key,d]:saved)if(d)prototype.defineProperty(key,*d);else prototype.erase(key);}
};
JSValue key(int type){if(type==1)return JSValue::array();if(type==2)return JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return {};});return JSValue::object();}
JSValue converter(JSValue trace,JSValue result,int action=0,JSValue properties={},JSValue object={}){
    auto value=JSValue::object();value.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{
        append(c[0],{"convert",args[0]});const auto action=c[2].toNumber();
        if(action==1)throw JSRangeError("conversion");if(action==2)return JSValue::object();
        if(action==3)call(c[3],"dispose");if(action==4)call(c[3],"remove",{c[4]});return c[1];
    },{trace,result,action,properties,object}));return value;
}
JsonValue basicCase(int type){
    WebGLProperties p;auto object=key(type),other=key(type);A results;results.push_back(encode(p.has(object)));
    auto first=p.get(object),second=p.get(other);results.push_back(first.sameValue(p.get(object)));results.push_back(!first.sameValue(second));
    results.push_back(first.getPrototype().sameValue(JSValue::object().getPrototype()));
    auto value=JSValue::object();results.push_back(invoke([&]{return p.update(object,"value",value);}));results.push_back(first.get("value").sameValue(value));
    results.push_back(invoke([&]{return p.remove(object);}));results.push_back(encode(p.has(object)));auto third=p.get(object);results.push_back(!third.sameValue(first));
    results.push_back(invoke([&]{return p.dispose();}));results.push_back(encode(p.has(object)));first.set("after",42);results.push_back(encode(first.get("after")));
    auto detached=p.object().get("get");results.push_back(detached.call(nullptr,{object}).sameValue(p.get(object)));
    p.object().get("dispose").call(false);results.push_back(encode(p.has(object)));return O{{"kind","basic"},{"type",type},{"results",results}};
}
JsonValue primitiveCase(int index){WebGLProperties p;auto value=scalars()[index];return O{{"kind","primitive"},{"index",index},
    {"has",invoke([&]{return p.has(value);})},{"get",invoke([&]{return p.get(value);})},{"remove",invoke([&]{return p.remove(value);})},
    {"update",invoke([&]{return p.update(value,"x",42);})}};}
JsonValue scalarCase(int k,int v){WebGLProperties p;auto object=key(0),target=p.get(object);auto values=scalars();
    const auto result=invoke([&]{return p.update(object,values[k],values[v]);});return O{{"kind","scalar"},{"key",k},{"value",v},{"result",result},{"target",encode(target)}};}
JsonValue specialCase(int k,int v){WebGLProperties p;auto object=key(0),target=p.get(object);const std::vector<std::string> keys{"__proto__","constructor","toString","01","2","length"};
    const auto result=invoke([&]{return p.update(object,keys[k],scalars()[v]);});return O{{"kind","special"},{"key",k},{"value",v},{"result",result},{"target",encode(target)},
        {"nullPrototype",target.getPrototype().isNull()}};}
JsonValue coercionCase(int scenario){
    WebGLProperties p;auto object=key(0),trace=JSValue::array(),target=JSValue(),property=JSValue();
    if(scenario!=0)target=p.get(object);property=converter(trace,"x",scenario==2?1:scenario==3?2:scenario==9?3:scenario==10?4:0,p.object(),object);
    if(scenario>=4&&scenario<=8){property=JSValue::object();const auto toString=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{
        append(c[0],{"toString"});if(c[1].toNumber()==8)throw JSRangeError("toString");if(c[1].toNumber()==6)return JSValue::object();return "x";
    },{trace,scenario});JSPropertyDescriptor d;d.configurable=true;
        d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{append(c[0],{"get-toString"});if(c[1].toNumber()==7)throw JSRangeError("getter");return c[1].toNumber()==5?JSValue(17):c[2];},{trace,scenario,toString});property.defineProperty("toString",d);
        JSPropertyDescriptor valueOf;valueOf.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{append(c[0],{"get-valueOf"});return JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{append(c[0],{"valueOf"});return "x";},{c[0]});},{trace});property.defineProperty("valueOf",valueOf);
    }
    if(scenario==11)target.defineProperty("x",JSPropertyDescriptor{.value=1,.writable=false,.enumerable=true,.configurable=true});
    if(scenario==12){auto prototype=JSValue::object();JSPropertyDescriptor d;d.setter=JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{append(c[0],{"set",self.sameValue(c[1]),args[0]});return {};},{trace,target});d.configurable=true;prototype.defineProperty("x",d);target.setPrototype(prototype);}
    if(scenario==13||scenario==14){JSPropertyDescriptor d;d.enumerable=true;d.configurable=true;d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{append(c[0],{"unexpected-get"});return 1;},{trace});
        if(scenario==13)d.setter=JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{append(c[0],{"set",self.sameValue(c[1]),args[0]});return {};},{trace,target});target.defineProperty("x",d);}
    if(scenario==15)target.preventExtensions();
    Restore restore(nativeWeakMapConstructor().get("prototype"));
    if(scenario>=16&&scenario<=18){restore.remember("get");restore.prototype.defineProperty("get",JSPropertyDescriptor{.value=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{scenario==16?JSValue(nullptr):scenario==17?JSValue(17):JSValue(false)}),.writable=true,.configurable=true});}
    const auto result=invoke([&]{return p.update(object,property,42);});
    return O{{"kind","coercion"},{"scenario",scenario},{"result",result},{"target",encode(target)},{"has",encode(p.has(object))},{"trace",encode(trace)}};
}
int mapId(JSValue registry,const JSValue& map){for(std::uint32_t i=0;i<registry.length();++i)if(registry.get(std::to_string(i)).sameValue(map))return static_cast<int>(i+1);registry.get("push").call(registry,{map});return registry.length();}
JsonValue dynamicCase(int scenario){
    WebGLProperties p;auto object=key(0),trace=JSValue::array(),registry=JSValue::array(),target=p.get(object);target.set("before",1);
    auto prototype=nativeWeakMapConstructor().get("prototype");Restore restore(prototype);const std::vector<std::string> names{"get","set","delete","has"};
    if(scenario<=5||scenario>=16||scenario==13)p.remove(object);
    for(int method=0;method<4;++method){const auto& name=names[method];restore.remember(name);auto original=prototype.get(name);
        auto replacement=JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{
            const auto scenario=c[0].toNumber(),method=c[1].toNumber();append(c[2],{"call",c[3],mapId(c[4],self),isNativeWeakMap(self)});
            if((scenario==0&&method==0)||(scenario==2&&method==1)||(scenario==6&&method==2)||(scenario==8&&method==3)||(scenario==13&&method==0))call(c[5],"dispose");
            if((scenario==4&&method==0)||(scenario==10&&method==2))throw JSRangeError("method");
            if(scenario>=16&&method==0)return c[7];return c[6].call(self,args);
        },{scenario,method,trace,name,registry,p.object(),original,scenario==16?JSValue(nullptr):scenario==17?JSValue(false):scenario==18?JSValue(0):scenario==19?JSValue(""):scenario==20?JSValue::object():JSValue()});
        JSPropertyDescriptor d;d.writable=true;d.configurable=true;
        const bool getter=(scenario==1&&method==0)||(scenario==3&&method==1)||(scenario==5&&method==0)||(scenario==7&&method==2)||(scenario==9&&method==3)||(scenario==11&&method==2)||(scenario==12&&method==0)||(scenario==13&&method==1);
        if(getter){d.writable.reset();d.getter=JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{append(c[2],{"getter",c[3],mapId(c[4],self),isNativeWeakMap(self)});
            const auto scenario=c[0].toNumber();if(scenario==5||scenario==11)throw JSRangeError("getter");if(scenario==12)return 17;call(c[5],"dispose");return c[6];
        },{scenario,method,trace,name,registry,p.object(),replacement});}
        else d.value=scenario==14&&method==0?JSValue(17):replacement;
        prototype.defineProperty(name,d);
    }
    JsonValue result;
    if(scenario==6||scenario==7||scenario==10||scenario==11)result=invoke([&]{return p.remove(object);});
    else if(scenario==8||scenario==9)result=invoke([&]{return p.has(object);});
    else if(scenario==15)result=invoke([&]{return p.update(object,"after",42);});
    else result=invoke([&]{return p.get(object);});
    // Observe cache identity with saved intrinsic functions, avoiding extra logs.
    for(const auto& [name,d]:restore.saved)if(d)prototype.defineProperty(name,*d);
    return O{{"kind","dynamic"},{"scenario",scenario},{"result",result},{"old",encode(target)},{"has",encode(p.has(object))},{"trace",encode(trace)}};
}
JsonValue pollutionCase(int scenario){
    auto trace=JSValue::array(),prototype=JSValue::object().getPrototype();Restore restore(prototype);
    for(const auto* name:{"has","get","remove","update","dispose","properties"}){restore.remember(name);JSPropertyDescriptor d;d.configurable=true;
        if(scenario==1){d.value=77;d.writable=false;}else{d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{append(c[0],{"get",c[1]});return 77;},{trace,name});d.setter=JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{append(c[0],{"set",c[1],args[0]});return {};},{trace,name});}prototype.defineProperty(name,d);}
    JsonValue state;A descriptors;const auto result=invoke([&]()->JSValue{WebGLProperties p;for(const auto& name:p.object().keys()){const auto d=p.object().getOwnPropertyDescriptor(name);descriptors.push_back(A{name,d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)});}auto object=key(0),target=p.get(object);const auto update=invoke([&]{return p.update(object,"get",42);});state=O{{"update",update},{"target",encode(target)},{"has",encode(p.has(object))},{"plainPrototype",target.getPrototype().sameValue(prototype)}};return {};});
    return O{{"kind","pollution"},{"scenario",scenario},{"result",result},{"descriptors",descriptors},{"state",state},{"trace",encode(trace)}};
}
void emit(JsonValue value){std::cout<<value.dump()<<'\n';}
}
int main(){try{auto anchor=JSValue::object();auto run=[&](JsonValue value){emit(std::move(value));anchor.collectGarbage();};
    for(int i=0;i<3;++i)run(basicCase(i));for(int i=0;i<14;++i)run(primitiveCase(i));
    for(int k=0;k<14;++k)for(int v=0;v<14;++v)run(scalarCase(k,v));
    for(int k=0;k<6;++k)for(int v=0;v<14;++v)run(specialCase(k,v));
    for(int i=0;i<19;++i)run(coercionCase(i));for(int i=0;i<22;++i)run(dynamicCase(i));for(int i=0;i<2;++i)run(pollutionCase(i));

    for(const auto& value:WebGLClosureParity::cases("properties")) {std::cout<<value.dump()<<'\n';}
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
