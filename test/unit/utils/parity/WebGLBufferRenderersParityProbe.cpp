#include "WebGLClosureConstructibility.h"
#include "renderers/webgl/WebGLBufferRenderer.h"
#include "renderers/webgl/WebGLIndexedBufferRenderer.h"
#include <cmath>
#include <iostream>
#include <limits>
namespace {
using Args=std::vector<JSValue>;using O=JsonValue::Object;using A=JsonValue::Array;
JSValue array(const Args& values={}){auto a=JSValue::array();a.get("push").call(a,values);return a;}
void push(const JSValue& trace,const Args& values){trace.get("push").call(trace,{array(values)});}
JSValue token(const JSValue& v){
    if(!v.isObject())return v;auto r=JSValue::object();auto tag=v.get("tag");r.set("ref",tag.isUndefined()?JSValue("object"):tag);return r;
}
JsonValue encode(const JSValue& v){
    if(v.isUndefined())return "undefined";if(v.isNull())return nullptr;
    if(v.type()==JSValue::Type::Boolean)return v.toBoolean();if(v.type()==JSValue::Type::String)return v.string();
    if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "NaN";if(std::isinf(n))return n<0?"-Infinity":"Infinity";if(n==0&&std::signbit(n))return "-0";return n;}
    if(v.type()==JSValue::Type::Array){A a;for(std::uint32_t i=0;i<v.length();++i)a.push_back(encode(v.get(std::to_string(i))));return a;}
    O o;for(const auto& key:v.keys())o[key]=encode(v.get(key));return o;
}
JSValue call(const JSValue& r,const char* name,const Args& args={}){return r.get(name).call(r,args);}
JSValue index(JSValue type=5123,JSValue bytes=2){auto r=JSValue::object();r.set("type",type);r.set("bytesPerElement",bytes);return r;}
std::vector<JSValue> numbers(){return {0,-0.0,2,2.5,-1,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),"3","bad",nullptr,JSValue(),true,false};}
template<class F> JsonValue invoke(F&& fn){try{return encode(fn());}catch(const JSRangeError&){return O{{"error","RangeError"}};}catch(const JSTypeError&){return O{{"error","TypeError"}};}catch(const std::exception&){return O{{"error","Error"}};}}
struct Harness {
    JSValue trace=JSValue::array(),gl=JSValue::object(),extension=JSValue::object(),extensions=JSValue::object(),info=JSValue::object(),control=JSValue::object(),renderer;
    JSValue record(const char* name,JSValue receiver){return JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue {
        Args log{c[1],self.sameValue(c[2])};for(const auto& arg:args)log.push_back(token(arg));push(c[0],log);return {};
    },{trace,name,receiver});}
    explicit Harness(bool indexed){
        for(const auto* name:{"drawArrays","drawArraysInstanced","drawElements","drawElementsInstanced"})gl.set(name,record(name,gl));
        for(const auto* name:{"multiDrawArraysWEBGL","multiDrawElementsWEBGL"})extension.set(name,record(name,extension));
        extensions.set("get",JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue {
            push(c[0],{"get",self.sameValue(c[1]),args[0]});return c[2];
        },{trace,extensions,extension}));info.set("update",record("info",info));
        renderer=indexed?WebGLIndexedBufferRenderer(gl,extensions,info).object():WebGLBufferRenderer(gl,extensions,info).object();
        control.set("renderer",renderer);call(renderer,"setMode",{4});if(indexed)call(renderer,"setIndex",{index()});
    }
    JSValue convertible(const char* name,JSValue value,bool throws=false,int mutate=0){
        auto r=JSValue::object();r.set("tag",name);r.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {
            push(c[0],{"convert",c[1],args[0]});const auto renderer=c[4].get("renderer");
            if(c[5].toNumber()!=0){call(renderer,"setMode",{1});call(renderer,"setIndex",{index(99,8)});}
            if(c[3].toBoolean())throw JSRangeError("convert");return c[2];
        },{trace,name,value,throws,control,mutate}));return r;
    }
    void getter(JSValue target,const char* property,JSValue value,int mutate=0,bool throws=false){
        JSPropertyDescriptor d;d.configurable=true;d.enumerable=true;
        d.getter=JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue {
            push(c[0],{"getter",c[1],self.sameValue(c[2])});
            if(c[4].toNumber()!=0)call(c[3].get("renderer"),"setMode",{c[4]});
            if(c[5].toBoolean())throw JSRangeError("getter");return c[6];
        },{trace,property,target,control,mutate,throws,value});target.defineProperty(property,d);
    }
};
JsonValue numericCase(bool indexed,int operation,int s,int c,int instances){
    Harness h(indexed);const auto values=numbers();
    const auto result=invoke([&]{return operation==0?call(h.renderer,"render",{values[s],values[c]}):call(h.renderer,"renderInstances",{values[s],values[c],values[instances]});});
    return O{{"kind","numeric"},{"indexed",indexed},{"operation",operation},{"start",s},{"count",c},{"instances",instances},{"result",result},{"trace",encode(h.trace)}};
}
JsonValue multiCase(bool indexed,int values,int draws){
    Harness h(indexed);auto starts=array({0,3,9}),counts=array();starts.set("tag","starts");counts.set("tag","counts");
    const std::vector<Args> fixtures{{1,2,3},{"1","2","3"},{2,JSValue(),4},{-0.0,-0.0,-0.0},{std::numeric_limits<double>::quiet_NaN(),3,4},{std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),4}};
    counts.get("push").call(counts,fixtures[values]);const Args drawCounts{0,-0.0,1,2.5,"0",false,nullptr,JSValue(),std::numeric_limits<double>::quiet_NaN(),-2};
    const auto result=invoke([&]{return call(h.renderer,"renderMultiDraw",{starts,counts,drawCounts[draws]});});
    return O{{"kind","multi"},{"indexed",indexed},{"values",values},{"draws",draws},{"result",result},{"trace",encode(h.trace)}};
}
JsonValue edgeCase(bool indexed,int scenario){
    Harness h(indexed);auto starts=array({0,3,9}),counts=array({1,2,3});starts.set("tag","starts");counts.set("tag","counts");
    JSValue start=3,count=6,instances=2,drawCount=3;A results;const auto command=indexed?"drawElements":"drawArrays",multi=indexed?"multiDrawElementsWEBGL":"multiDrawArraysWEBGL";
    auto run=[&](const char* method,const Args& args){results.push_back(invoke([&]{return call(h.renderer,method,args);}));};
    if(scenario==0){call(h.renderer,"setMode",{JSValue()});if(indexed)call(h.renderer,"setIndex",{JSValue::object()});run("render",{});}
    if(scenario==1){if(indexed){auto value=index();call(h.renderer,"setIndex",{value});value.set("type",99);value.set("bytesPerElement",8);}run("render",{3,6});}
    if(scenario==2){if(indexed){auto value=index(99,8);h.getter(value,"bytesPerElement",8,0,true);run("setIndex",{value});}run("render",{3,6});}
    if(scenario==3){if(indexed){auto value=index(99,8);h.getter(value,"type",99,0,true);run("setIndex",{value});}run("render",{3,6});}
    if(scenario==4){if(indexed){auto bytes=h.convertible("bytes",2);call(h.renderer,"setIndex",{index(99,bytes)});run("render",{3,6});}else run("render",{h.convertible("start",3),h.convertible("count",6)});}
    if(scenario==5){start=h.convertible("start",3,false,indexed?1:0);run("render",{start,6});run("render",{3,6});}
    if(scenario==6){if(indexed)call(h.renderer,"setIndex",{index(5123,h.convertible("bytes",2,false,1))});run("renderInstances",{h.convertible("start",3),6,2});}
    if(scenario==7){h.getter(h.gl,command,h.gl.get(command),3);run("render",{h.convertible("start",3),6});}
    if(scenario==8){h.gl.set(command,JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"gpu-throw"});throw JSRangeError("gpu");},{h.trace}));run("render",{3,6});}
    if(scenario==9){h.gl.set(command,JSValue());run("render",{h.convertible("start",3),6});}
    if(scenario==10){h.getter(h.gl,command,{},0,true);run("render",{h.convertible("start",3),6});}
    if(scenario==11){h.getter(h.info,"update",h.info.get("update"),3);run("render",{3,6});}
    if(scenario==12){h.info.set("update",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"info-throw"});throw JSRangeError("info");},{h.trace}));run("render",{3,6});}
    if(scenario==13){h.info.set("update",h.record("replacement-info",h.info));run("render",{3,6});}
    if(scenario==14){auto r=indexed?WebGLIndexedBufferRenderer().object():WebGLBufferRenderer().object();results.push_back(invoke([&]{return call(r,"renderInstances",{h.convertible("start",3),h.convertible("count",6),-0.0});}));}
    if(scenario==15){run("renderInstances",{3,6,h.convertible("instances",0)});}
    if(scenario==16){const auto original=h.extensions.get("get");h.getter(h.extensions,"get",JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue {
        push(c[0],{"extension-callback"});call(c[1].get("renderer"),"setMode",{2});return c[2].call(self,args);
    },{h.trace,h.control,original}),1);h.getter(h.extension,multi,h.extension.get(multi),3);run("renderMultiDraw",{starts,counts,3});}
    if(scenario==17){h.extensions.set("get",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"get-null"});return nullptr;},{h.trace}));run("renderMultiDraw",{starts,counts,3});}
    if(scenario==18){h.extension.set(multi,JSValue());run("renderMultiDraw",{starts,counts,3});}
    if(scenario==19){h.extension.set(multi,JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"multi-throw"});throw JSRangeError("multi");},{h.trace}));run("renderMultiDraw",{starts,counts,3});}
    if(scenario==20){for(int i=0;i<3;++i)h.getter(counts,std::to_string(i).c_str(),i+1);run("renderMultiDraw",{starts,counts,3});}
    if(scenario==21){drawCount=JSValue::object();drawCount.set("tag","drawCount");drawCount.set("calls",0);drawCount.setPrimitiveConverter(JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue {
        auto value=self;const auto calls=value.get("calls").toNumber();value.set("calls",calls+1);push(c[0],{"condition",args[0],calls});return calls<2?3:1;
    },{h.trace}));run("renderMultiDraw",{starts,counts,drawCount});}
    if(scenario==22){drawCount=JSValue::object();drawCount.set("tag","drawCount");drawCount.set("limit",3);drawCount.setPrimitiveConverter(JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue {
        push(c[0],{"condition",args[0],self.get("limit")});return self.get("limit");
    },{h.trace}));JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {auto limit=c[0];limit.set("limit",1);push(c[1],{"count-0"});return 5;},{drawCount,h.trace});counts.defineProperty("0",d);run("renderMultiDraw",{starts,counts,drawCount});}
    if(scenario==23)run("renderMultiDraw",{starts,"123",3});
    if(scenario==24)run("renderMultiDraw",{starts,nullptr,1});
    if(scenario==25)run("renderMultiDraw",{starts,JSValue(),std::numeric_limits<double>::quiet_NaN()});
    if(scenario==26){counts.erase("1");run("renderMultiDraw",{starts,counts,3});}
    if(scenario==27){h.extension.set(multi,JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {auto values=c[0];values.set("0",10);call(c[1].get("renderer"),"setMode",{1});push(c[2],{"multi-mutate"});return {};},{counts,h.control,h.trace}));run("renderMultiDraw",{starts,counts,3});}
    if(scenario==28){counts.set("1",h.convertible("count-1",2,true));run("renderMultiDraw",{starts,counts,3});}
    if(scenario==29)run("renderMultiDraw",{starts,counts,"0"});
    if(scenario==30){h.renderer.set("render",JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue {push(c[0],{"replacement-render",self.sameValue(c[1])});return 42;},{h.trace,h.renderer}));run("render",{3,6});}
    if(scenario==31){auto mode=JSValue::object();mode.set("tag","mode");call(h.renderer,"setMode",{mode});run("render",{3,6});}
    return O{{"kind","edge"},{"indexed",indexed},{"scenario",scenario},{"results",results},{"trace",encode(h.trace)}};
}
JsonValue prototypeCase(bool indexed,int scenario){
    Harness h(indexed);auto pollution=JSValue::array(),prototype=JSValue::object().getPrototype();
    auto indexValue=JSValue::object(nullptr);indexValue.set("type",5125);indexValue.set("bytesPerElement",4);
    std::vector<std::pair<std::string,std::optional<JSPropertyDescriptor>>> saved;
    struct Restore {
        JSValue prototype;decltype(saved)& descriptors;
        ~Restore(){for(const auto& [key,descriptor]:descriptors)if(descriptor)prototype.defineProperty(key,*descriptor);else prototype.erase(key);}
    } restore{prototype,saved};
    for(const auto* key:{"mode","type","bytesPerElement"}) {
        saved.emplace_back(key,prototype.getOwnPropertyDescriptor(key));JSPropertyDescriptor d;d.configurable=true;
        if(scenario==1){d.value=77;d.writable=false;}
        else {
            d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"get",c[1]});return 77;},{pollution,key});
            d.setter=JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {push(c[0],{"set",c[1],args[0]});return {};},{pollution,key});
        }
        prototype.defineProperty(key,d);
    }
    A results;const auto result=invoke([&]()->JSValue {
        h.renderer=indexed?WebGLIndexedBufferRenderer(h.gl,h.extensions,h.info).object():WebGLBufferRenderer(h.gl,h.extensions,h.info).object();
        auto run=[&](const char* name,const Args& args){results.push_back(invoke([&]{return call(h.renderer,name,args);}));};
        run("render",{3,6});run("setMode",{4});if(indexed)run("setIndex",{indexValue});
        run("render",{3,6});run("setMode",{1});run("renderInstances",{5,8,2});
        auto starts=array({0,3}),counts=array({6,2});starts.set("tag","starts");counts.set("tag","counts");
        run("renderMultiDraw",{starts,counts,2});return {};
    });
    return O{{"kind","prototype"},{"indexed",indexed},{"scenario",scenario},{"result",result},{"results",results},{"trace",encode(h.trace)},{"pollution",encode(pollution)}};
}
void emit(JsonValue v){std::cout<<v.dump()<<'\n';}
}
int main(){
    try{auto anchor=JSValue::object();
        for(const bool indexed:{false,true}){
            for(int s=0;s<14;++s)for(int c=0;c<14;++c){emit(numericCase(indexed,0,s,c,0));anchor.collectGarbage();}
            for(int s=0;s<4;++s)for(int c=0;c<6;++c)for(int i=0;i<14;++i){emit(numericCase(indexed,1,s,c,i));anchor.collectGarbage();}
            for(int v=0;v<6;++v)for(int d=0;d<10;++d){emit(multiCase(indexed,v,d));anchor.collectGarbage();}
            for(int i=0;i<32;++i){emit(edgeCase(indexed,i));anchor.collectGarbage();}
            for(int i=0;i<2;++i){emit(prototypeCase(indexed,i));anchor.collectGarbage();}
        }
        for(const auto& value:WebGLClosureParity::cases("buffer")) {std::cout<<value.dump()<<'\n';}
        for(const auto& value:WebGLClosureParity::cases("indexed")) {std::cout<<value.dump()<<'\n';}
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
