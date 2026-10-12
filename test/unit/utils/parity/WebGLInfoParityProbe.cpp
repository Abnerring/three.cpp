#include "WebGLClosureConstructibility.h"
#include "renderers/webgl/WebGLInfo.h"
#include <cmath>
#include <iostream>
#include <limits>
namespace {
using Args = std::vector<JSValue>; using O = JsonValue::Object; using A = JsonValue::Array;
const std::vector<int> modes{4, 1, 3, 2, 0};
JSValue glObject() {
    auto gl = JSValue::object(); gl.set("TRIANGLES",4); gl.set("LINES",1);
    gl.set("LINE_STRIP",3); gl.set("LINE_LOOP",2); gl.set("POINTS",0); return gl;
}
JSValue append(const JSValue& array, const Args& items) { return array.get("push").call(array, items); }
JSValue tuple(const Args& values) { auto a=JSValue::array(); append(a,values); return a; }
JsonValue encode(const JSValue& v) {
    switch(v.type()) {
    case JSValue::Type::Undefined:return "undefined";
    case JSValue::Type::Function:return "function";
    case JSValue::Type::Null:return nullptr;
    case JSValue::Type::Boolean:return v.toBoolean();
    case JSValue::Type::String:return v.string();
    case JSValue::Type::Number: {
        const auto n=v.number(); if(std::isnan(n))return "NaN";
        if(std::isinf(n))return n<0?"-Infinity":"Infinity";
        if(n==0&&std::signbit(n))return "-0"; return n;
    }
    case JSValue::Type::Array: {
        A a;for(std::uint32_t i=0;i<v.length();++i)a.push_back(encode(v.get(std::to_string(i))));return a;
    }
    default: {
        O o;for(const auto& key:v.keys())o[key]=encode(v.get(key));return o;
    }
    }
}
std::vector<JSValue> numbers() {
    return {0,-0.0,1,3,5.5,-2,std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),"6","bad",nullptr,JSValue(),true,false};
}
JSValue convertible(JSValue trace,const char* name,JSValue result,bool throws=false,JSValue mutate={}) {
    auto value=JSValue::object(); value.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {
        append(c[0],{tuple({c[1],args[0]})});
        if(!c[4].isUndefined()) {auto r=c[4];r.set("calls",100);r.set("triangles",100);r.set("lines",100);r.set("points",100);}
        if(c[3].toBoolean())throw JSRangeError("conversion");return c[2];
    },{trace,name,result,throws,mutate}));return value;
}
void console(JSValue trace,int mode=0) {
    WebGLInfo::setConsoleFunction(JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue {
        append(c[0],{tuple({"console",self.isUndefined(),args[0],args[1],args[2]})});
        if(c[1].toBoolean())throw JSRangeError("console");return {};
    },{trace,mode!=0}));
}
template<class F> JsonValue invoke(F&& f) {
    try {return encode(f());} catch(const JSRangeError&){return O{{"error","RangeError"}};}
    catch(const JSTypeError&){return O{{"error","TypeError"}};}
    catch(const std::exception&){return O{{"error","Error"}};}
}
JsonValue state(const WebGLInfo& info,const JSValue& captured) {
    return O{{"keys",encode(tuple([&]{Args a;for(const auto& k:info.object().keys())a.emplace_back(k);return a;}()))},
        {"memory",encode(info.memory.value())},{"render",encode(info.render.value())},{"captured",encode(captured)},
        {"programs",encode(info.programs.value())},{"autoReset",encode(info.autoReset.value())}};
}
void emit(JsonValue v){std::cout<<v.dump()<<'\n';}
JsonValue numericCase(int mode,std::size_t c,std::size_t i) {
    auto trace=JSValue::array();console(trace);WebGLInfo info(glObject());const auto r=info.render.value();const auto values=numbers();
    auto render=r;render.set("frame",7);info.memory.set("textures",3);
    const auto result=invoke([&]{return info.update(values[c],mode,values[i]);});
    const auto updated=state(info,r);const auto reset=invoke([&]{return info.reset();});
    return O{{"kind","numeric"},{"mode",mode},{"count",double(c)},{"instances",double(i)},
        {"result",result},{"updated",updated},{"reset",reset},{"after",state(info,r)},{"trace",encode(trace)}};
}
JsonValue counterCase(int mode,int index) {
    auto trace=JSValue::array();console(trace);WebGLInfo info(glObject());auto r=info.render.value();
    const auto values=numbers();r.set("calls",values[index]);
    for(const auto* field:{"triangles","lines","points"})r.set(field,values[index]);
    const auto result=invoke([&]{return info.update(6,mode,2);});
    return O{{"kind","counter"},{"mode",mode},{"index",index},{"result",result},{"state",state(info,r)},{"trace",encode(trace)}};
}
JsonValue coercionCase(int mode,bool reentry) {
    auto trace=JSValue::array();console(trace);WebGLInfo info(glObject());auto r=info.render.value();
    const auto field=mode==4?"triangles":mode==0?"points":"lines";
    r.set("calls",convertible(trace,"calls","2"));
    r.set(field,convertible(trace,"previous","sum:"));
    const auto result=invoke([&]{return info.update(convertible(trace,"count",6,false,reentry?r:JSValue()),mode,convertible(trace,"instances",2));});
    return O{{"kind","coercion"},{"mode",mode},{"reentry",reentry},{"result",result},{"state",state(info,r)},{"trace",encode(trace)}};
}
JsonValue glCase(int index) {
    auto trace=JSValue::array();console(trace);auto gl=glObject(),mode=JSValue::object();mode.set("tag","mode");
    const std::vector<std::string> names{"TRIANGLES","LINES","LINE_STRIP","LINE_LOOP","POINTS"};
    for(std::size_t i=0;i<names.size();++i) {
        JSPropertyDescriptor d;d.configurable=true;
        d.getter=JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue {
            append(c[0],{tuple({"gl",c[1],self.sameValue(c[2])})});
            return c[3].number()==c[4].number()?c[5]:JSValue("other");
        },{trace,names[i],gl,double(i),index,mode});gl.defineProperty(names[i],d);
    }
    WebGLInfo info(gl);const auto r=info.render.value();
    const auto result=invoke([&]{return info.update(6,mode,2);});
    return O{{"kind","gl"},{"index",index},{"result",result},{"state",state(info,r)},{"trace",encode(trace)}};
}
JsonValue closureCase() {
    auto trace=JSValue::array();console(trace);WebGLInfo info(glObject());auto r=info.render.value();r.set("frame",17);
    info.render=nullptr;info.memory="memory";info.programs=tuple({"program"});info.autoReset="manual";
    const auto update=invoke([&]{return info.object().get("update").call(nullptr,{9,4,2});});
    const auto updated=state(info,r),reset=invoke([&]{return info.reset();});
    WebGLInfo copy(info);copy.programs=18;
    return O{{"kind","closure"},{"update",update},{"updated",updated},{"reset",reset},
        {"alias",copy.object().sameValue(info.object())},{"after",state(info,r)},{"trace",encode(trace)}};
}
JsonValue resetCase(int index) {
    auto trace=JSValue::array();console(trace);WebGLInfo info(glObject());auto r=info.render.value();r.set("frame",13);
    const std::vector<std::string> names{"calls","triangles","points","lines"};
    for(std::size_t i=0;i<names.size();++i) {
        JSPropertyDescriptor d;d.configurable=true;
        d.setter=JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {
            append(c[0],{tuple({"set",c[1],args[0]})});
            if(c[2].number()==c[3].number())throw JSTypeError("reset");return {};
        },{trace,names[i],double(i),index});r.defineProperty(names[i],d);
    }
    const auto result=invoke([&]{return info.reset();});
    return O{{"kind","reset"},{"index",index},{"result",result},{"state",state(info,r)},{"trace",encode(trace)}};
}
JsonValue exceptionCase(int index) {
    auto trace=JSValue::array();console(trace,index==7?1:0);auto gl=glObject();
    if(index==0)gl={};if(index==1)gl=nullptr;if(index==2)gl=JSValue::object();
    if(index==3) {JSPropertyDescriptor d;d.getter=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue {throw JSRangeError("gl");});gl.defineProperty("TRIANGLES",d);}
    WebGLInfo info(gl);auto r=info.render.value();JSValue count=6,mode=4,instances=2;
    if(index==2)mode={};
    if(index==4)count=convertible(trace,"count",6,true);
    if(index==5)instances=convertible(trace,"instances",2,true);
    if(index==6||index==7||index==8)mode="4";
    if(index==8)WebGLInfo::setConsoleFunction(1);
    if(index==9)r.set("calls",convertible(trace,"calls",0,true));
    if(index==10)r.set("triangles",convertible(trace,"previous",0,true));
    if(index==11) {mode=JSValue::object();mode.set("isStackTrace",true);mode.set("getError",JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue {throw JSRangeError("stackTrace");}));WebGLInfo::setConsoleFunction(nullptr);}
    if(index==12) {mode=JSValue::object();JSPropertyDescriptor d;d.getter=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue {throw JSRangeError("stackTrace");});mode.defineProperty("isStackTrace",d);WebGLInfo::setConsoleFunction(false);}
    const auto result=invoke([&]{return info.update(count,mode,instances);});
    return O{{"kind","exception"},{"index",index},{"result",result},{"state",state(info,r)},{"trace",encode(trace)}};
}
JsonValue prototypeCase(int scenario) {
    auto trace=JSValue::array(),prototype=JSValue::object().getPrototype(),gl=glObject();
    console(trace);std::vector<std::pair<std::string,std::optional<JSPropertyDescriptor>>> saved;
    struct Restore {
        JSValue prototype;decltype(saved)& descriptors;
        ~Restore(){for(const auto& [key,d]:descriptors)if(d)prototype.defineProperty(key,*d);else prototype.erase(key);}
    } restore{prototype,saved};
    for(const auto* key:{"geometries","textures","frame","calls","triangles","points","lines",
                        "memory","render","programs","autoReset","reset","update"}) {
        saved.emplace_back(key,prototype.getOwnPropertyDescriptor(key));JSPropertyDescriptor d;d.configurable=true;
        if(scenario==1){d.value=77;d.writable=false;}
        else {
            d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {append(c[0],{tuple({"get",c[1]})});return 77;},{trace,key});
            d.setter=JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {append(c[0],{tuple({"set",c[1],args[0]})});return {};},{trace,key});
        }
        prototype.defineProperty(key,d);
    }
    JsonValue initial,updated,after,deleted;A descriptors,results;
    const auto result=invoke([&]()->JSValue {
        WebGLInfo info(gl);auto r=info.render.value();initial=state(info,r);
        for(const auto& object:{info.object(),info.memory.value(),r}) {
            for(const auto& key:object.keys()) {
                const auto d=object.getOwnPropertyDescriptor(key);
                descriptors.push_back(A{key,d->value.has_value(),d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false),
                    d->getter.has_value(),d->setter.has_value(),object.getPrototype().sameValue(prototype)});
            }
        }
        results.push_back(invoke([&]{return info.update(6,4,2);}));updated=state(info,r);
        results.push_back(invoke([&]{return info.reset();}));info.programs=9;after=state(info,r);
        r.erase("triangles");results.push_back(invoke([&]{return info.update(6,4,1);}));deleted=state(info,r);
        return {};
    });
    return O{{"kind","prototype"},{"scenario",scenario},{"result",result},{"initial",initial},{"descriptors",descriptors},{"results",results},
        {"updated",updated},{"after",after},{"deleted",deleted},{"trace",encode(trace)}};
}
}
int main() {
    try {
        for(const auto mode:modes)for(std::size_t c=0;c<numbers().size();++c)for(std::size_t i=0;i<numbers().size();++i)emit(numericCase(mode,c,i));
        for(const auto mode:modes)for(int i=0;i<15;++i)emit(counterCase(mode,i));
        for(const auto mode:modes)for(const auto reentry:{false,true})emit(coercionCase(mode,reentry));
        for(int i=0;i<6;++i)emit(glCase(i));emit(closureCase());
        for(int i=0;i<5;++i)emit(resetCase(i));for(int i=0;i<13;++i)emit(exceptionCase(i));
        for(int i=0;i<2;++i)emit(prototypeCase(i));
        WebGLInfo::setConsoleFunction(nullptr);
        for(const auto& value:WebGLClosureParity::cases("info")) {std::cout<<value.dump()<<'\n';}
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
