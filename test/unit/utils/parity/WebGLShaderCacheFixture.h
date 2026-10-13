#pragma once
#include "renderers/webgl/WebGLShaderCache.h"
#include "core/JSIterator.h"
#include "core/JSMap.h"
#include "core/JSSet.h"
#include "core/JSSymbol.h"
#include "core/NativeObject.h"
#include "materials/ShaderMaterial.h"
#include "materials/RawShaderMaterial.h"
#include <cmath>
#include <cstdint>
#include <iostream>

namespace ShaderCacheFixture {
using Args = std::vector<JSValue>;
using O = JsonValue::Object;
using A = JsonValue::Array;
inline JSValue call(const JSValue& receiver, const char* name, const Args& args = {}) {
    return receiver.get(name).call(receiver, args);
}
inline void prop(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;
    object.defineProperty(key,d);
}
inline JSValue object(std::initializer_list<std::pair<const char*,JSValue>> values = {}) {
    auto result=JSValue::object();for(const auto& [key,value]:values)prop(result,key,value);return result;
}
inline void accessor(JSValue object,const char* key,const JSValue& get,const JSValue& set={}) {
    JSPropertyDescriptor d;d.getter=get;d.setter=set;d.enumerable=d.configurable=true;object.defineProperty(key,d);
}
inline void event(const JSValue& log,const char* message) {call(log,"push",{message});}
inline JsonValue encode(const JSValue& value) {
    if(value.isUndefined())return "undefined";if(value.isNull())return nullptr;
    if(value.isSymbol())return "symbol:"+value.symbolDescription().value_or("");
    if(value.isCallable())return "function";
    if(value.type()==JSValue::Type::Number){if(std::isnan(value.number()))return "NaN";if(value.number()==0&&std::signbit(value.number()))return "-0";return value.number();}
    if(value.type()==JSValue::Type::String)return value.string();if(value.type()==JSValue::Type::Boolean)return value.toBoolean();return "object";
}
template<class F> JsonValue outcome(F action) {
    try{return action();}catch(const JSTypeError&){return "TypeError";}catch(const JSRangeError&){return "RangeError";}
}
inline JsonValue descriptor(const JSValue& object,const std::string& key) {
    const auto d=object.getOwnPropertyDescriptor(key);if(!d)return nullptr;
    return O{{"writable",d->writable.value_or(false)},{"enumerable",d->enumerable.value_or(false)},{"configurable",d->configurable.value_or(false)}};
}
inline A names(const JSValue& object) {A result;for(const auto& name:object.ownPropertyNames())result.emplace_back(name);return result;}
inline A iterate(const JSValue& value) {A result;auto record=getJSIterator(value);while(const auto item=stepJSIterator(record))result.push_back(encode(*item));return result;}
inline void emit(const JsonValue& value) {std::cout<<value.dump()<<'\n';}
inline unsigned random(std::uint32_t& state) {state=state*1664525u+1013904223u;return state;}
inline unsigned locate(const JSValue& value,const Args& values) {for(unsigned i=0;i<values.size();++i)if(value.sameValue(values[i]))return i;return 999;}
inline JsonValue snapshot(const JSValue& manager,const Args& materials,const Args& codes,const Args& stages) {
    A shaders,sets,saved;
    auto shaderIterator=getJSIterator(call(manager.get("shaderCache"),"entries"));
    while(const auto entry=stepJSIterator(shaderIterator)){
        const auto stage=entry->get("1");shaders.emplace_back(A{locate(entry->get("0"),codes),encode(stage.get("id")),encode(stage.get("usedTimes"))});
    }
    auto materialIterator=getJSIterator(call(manager.get("materialCache"),"entries"));
    while(const auto entry=stepJSIterator(materialIterator)){
        A ids;auto record=getJSIterator(entry->get("1"));while(const auto stage=stepJSIterator(record))ids.push_back(encode(stage->get("id")));
        sets.emplace_back(A{locate(entry->get("0"),materials),ids});
    }
    for(const auto& stage:stages)saved.emplace_back(A{encode(stage.get("id")),locate(stage.get("code"),codes),encode(stage.get("usedTimes"))});
    return O{{"shaders",shaders},{"materials",sets},{"saved",saved}};
}
inline void metadata() {
    const auto constructor=nativeWebGLShaderCacheConstructor();auto manager=constructor.construct(),prototype=constructor.get("prototype");
    auto stage=call(manager,"_getShaderStage",{"metadata"}),stageConstructor=stage.get("constructor");
    A methods;
    for(const auto& name:prototype.ownPropertyNames()){
        const auto method=prototype.get(name);methods.emplace_back(O{{"key",name},{"name",encode(method.get("name"))},{"length",encode(method.get("length"))},{"descriptor",descriptor(prototype,name)},{"construct",outcome([&]()->JsonValue{method.construct();return "ok";})}});
    }
    emit(O{{"kind","metadata"},{"name",constructor.get("name").string()},{"length",constructor.get("length").number()},{"prototype",descriptor(constructor,"prototype")},{"own",names(manager)},{"classCall",outcome([&]()->JsonValue{constructor.call();return "ok";})},{"methods",methods},{"stageName",stageConstructor.get("name").string()},{"stageLength",stageConstructor.get("length").number()},{"stageOwn",names(stage)},{"stagePrototype",names(stageConstructor.get("prototype"))},{"stageId",encode(stage.get("id"))},{"stageClassCall",outcome([&]()->JsonValue{stageConstructor.call();return "ok";})},{"maps",isNativeMap(manager.get("shaderCache"))&&isNativeMap(manager.get("materialCache"))}});
}
inline void randomMutations() {
    for(unsigned seed=0;seed<48;++seed){
        std::uint32_t rng=seed+1;auto manager=WebGLShaderCache().object();
        Args codes{{},nullptr,"alpha","beta",0,-0.0,std::nan(""),3,JSValue::object(),JSSymbol::create("code")};
        Args materials;for(unsigned i=0;i<6;++i)materials.push_back(object({{"vertexShader",codes[i]},{"fragmentShader",codes[(i+2)%codes.size()]}}));
        Args stages;
        for(unsigned step=0;step<160;++step){
            const auto op=random(rng)%9;
            const auto m=random(rng)%materials.size(),c=random(rng)%codes.size(),d=random(rng)%codes.size();
            auto material=materials[m];JsonValue result=outcome([&]()->JsonValue{
                switch(op){
                case 0:{const auto stage=call(manager,"getVertexShaderStage",{material});stages.push_back(stage);return encode(stage.get("id"));}
                case 1:{const auto stage=call(manager,"getFragmentShaderStage",{material});stages.push_back(stage);return encode(stage.get("id"));}
                case 2:{const auto a=call(manager,"_getShaderStage",{codes[c]}),b=call(manager,"_getShaderStage",{codes[d]});stages.push_back(a);stages.push_back(b);return call(manager,"update",{material,a,b}).sameValue(manager);}
                case 3:return call(manager,"remove",{material}).sameValue(manager);
                case 4:material.set("vertexShader",codes[c]);material.set("fragmentShader",codes[d]);return "ok";
                case 5:return isNativeSet(call(manager,"_getShaderCacheForMaterial",{material}));
                case 6:return encode(call(manager,"dispose"));
                case 7:{const auto stage=call(manager,"_getShaderStage",{codes[c]});stages.push_back(stage);return encode(stage.get("id"));}
                case 8:{if(stages.empty())return "empty";auto stage=stages[random(rng)%stages.size()];stage.set("usedTimes",static_cast<int>(c%3)-1);return "ok";}
                }
                return "unreachable";
            });
            emit(O{{"kind","random"},{"seed",seed},{"step",step},{"operation",op},{"result",result},{"state",snapshot(manager,materials,codes,stages)}});
        }
    }
}
inline JSValue traceGetter(const JSValue& log,const char* label,const JSValue& result,unsigned error=0) {
    return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        call(c[0],"push",{c[1]});if(c[3].number()==1)throw JSRangeError("getter");if(c[3].number()==2)throw JSTypeError("getter");return c[2];
    },{log,label,result,error});
}
inline JSValue traceMethod(const JSValue& log,const char* label,const JSValue& result,unsigned error=0) {
    return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        call(c[0],"push",{c[1]});if(c[3].number()==1)throw JSRangeError("method");if(c[3].number()==2)throw JSTypeError("method");return c[2];
    },{log,label,result,error});
}
inline void updateProtocols() {
    for(unsigned has=0;has<8;++has)for(unsigned numeric=0;numeric<9;++numeric)for(unsigned reused=0;reused<2;++reused){
        auto log=JSValue::array(),stored=object(),shaders=object(),manager=WebGLShaderCache().object();
        Args hasResults{false,true,0,"",{},nullptr,std::nan(""),JSValue::object()};
        auto number=JSValue::object();number.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{call(c[0],"push",{"hint:"+args[0].toString()});return 3;},{log}));
        Args numbers{{},nullptr,false,3,"2",std::nan(""),-0.0,JSSymbol::create("count"),number};
        auto vertex=object(),fragment=object();
        for(const auto& pair:std::vector<std::pair<JSValue,const char*>>{{vertex,"vertex"},{fragment,"fragment"}}){
            const std::string label=pair.second;
            accessor(pair.first,"usedTimes",traceGetter(log,("get:"+label).c_str(),numbers[numeric]),JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
                call(c[0],"push",{c[1]});auto state=c[2];state.set(c[3].string(),args[0]);return {};
            },{log,"set:"+label,stored,label}));
        }
        accessor(shaders,"has",traceGetter(log,"get:has",traceMethod(log,"call:has",hasResults[has])));
        accessor(shaders,"add",traceGetter(log,"get:add",traceMethod(log,"call:add",{})));
        accessor(manager,"_getShaderCacheForMaterial",traceGetter(log,"get:material-helper",traceMethod(log,"call:material-helper",shaders)));
        const auto result=outcome([&]()->JsonValue{return call(manager,"update",{object(),vertex,reused?vertex:fragment}).sameValue(manager);});
        emit(O{{"kind","update-protocol"},{"has",has},{"numeric",numeric},{"reused",reused},{"result",result},{"log",*log.toJSON()},{"vertex",encode(stored.get("vertex"))},{"fragment",encode(stored.get("fragment"))}});
    }
}
inline void lookupProtocols() {
    for(unsigned which=0;which<2;++which)for(unsigned mode=0;mode<12;++mode){
        auto manager=WebGLShaderCache().object(),log=JSValue::array(),state=object(),material=object();
        const auto helper=[](const JSValue& log,const char* name,unsigned value){return JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{call(c[0],"push",{c[1]});call(c[0],"push",{"arg:"+args[0].toString()});return c[2];},{log,name,value});};
        const auto old=helper(log,"old-helper",17),replacement=helper(log,"new-helper",23);
        accessor(manager,"_getShaderStage",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            event(c[0],"get:helper");if(c[3].number()==1)throw JSRangeError("helper");if(c[3].number()==2)return 7;
            auto material=c[2];if(c[3].number()==3)prop(material,c[4].string().c_str(),"changed");return c[1];
        },{log,old,material,mode,which?"fragmentShader":"vertexShader"}));
        accessor(material,which?"fragmentShader":"vertexShader",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            event(c[0],"get:shader");if(c[3].number()==4)throw JSRangeError("shader");auto manager=c[1];
            if(c[3].number()==5)prop(manager,"_getShaderStage",c[2]);return "code";
        },{log,manager,replacement,mode}));
        JSValue receiver=material;
        if(mode>=6){const Args values{{},nullptr,false,3,"",JSSymbol::create("material")};receiver=values[mode-6];}
        const auto result=outcome([&]()->JsonValue{return encode(call(manager,which?"getFragmentShaderStage":"getVertexShaderStage",{receiver}));});
        emit(O{{"kind","lookup-protocol"},{"which",which},{"mode",mode},{"result",result},{"log",*log.toJSON()}});
    }
}
inline void removeProtocols() {
    for(unsigned mode=0;mode<25;++mode){
        auto log=JSValue::array(),state=object({{"next",0},{"count",1},{"reads",0}}),manager=WebGLShaderCache().object();
        auto stage=object({{"code","old-code"}}),iterator=object(),iterable=object(),materialMap=object(),shaderMap=object(),otherMap=object();
        const auto erase=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
            event(c[0],"call:shader-delete");call(c[0],"push",{"code:"+args[0].toString()});return false;
        },{log});
        accessor(otherMap,"delete",traceGetter(log,"get:other-delete",traceMethod(log,"call:other-delete",false)));
        accessor(shaderMap,"delete",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            event(c[0],"get:shader-delete");if(c[4].number()==16)throw JSRangeError("delete");
            if(c[4].number()==21){auto stage=c[2];prop(stage,"code","changed-code");}
            if(c[4].number()==22){auto manager=c[3];prop(manager,"materialCache",c[5]);}
            return c[1];
        },{log,erase,stage,manager,mode,otherMap}));
        accessor(materialMap,"get",traceGetter(log,"get:material-get",traceMethod(log,"call:material-get",iterable)));
        accessor(materialMap,"delete",traceGetter(log,"get:material-delete",traceMethod(log,"call:material-delete",false),mode==17?1:0));
        accessor(manager,"materialCache",traceGetter(log,"get:materialCache",materialMap));
        accessor(manager,"shaderCache",traceGetter(log,"get:shaderCache",shaderMap));
        JSPropertyDescriptor iteration;iteration.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            event(c[0],"get:iterator");if(c[2].number()==1)throw JSRangeError("iterator");
            return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],"call:iterator");return c[2].number()==2?JSValue(7):c[1];},c);
        },{log,iterator,mode});iterable.defineProperty(JSSymbol::iterator(),iteration);
        accessor(iterator,"next",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            event(c[0],"get:next");if(c[3].number()==3)throw JSRangeError("next-get");
            return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
                event(c[0],"call:next");if(c[3].number()==4)throw JSRangeError("next-call");if(c[3].number()==5)return 7;
                auto state=c[1];const auto cursor=state.get("next").number();state.set("next",cursor+1);auto result=JSValue::object();
                accessor(result,"done",traceGetter(c[0],"get:done",cursor>0,c[3].number()==6?1:0));
                accessor(result,"value",traceGetter(c[0],"get:value",c[2],c[3].number()==7?1:0));return result;
            },c);
        },{log,state,stage,mode}));
        accessor(iterator,"return",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            event(c[0],"get:return");if(c[1].number()==12)throw JSTypeError("return-get");if(c[1].number()==18)return nullptr;
            return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
                event(c[0],"call:return");if(c[1].number()==13)throw JSTypeError("return-call");return c[1].number()==14?JSValue(7):JSValue::object();
            },c);
        },{log,mode}));
        accessor(stage,"usedTimes",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            event(c[0],"get:count");auto state=c[1];const auto read=state.get("reads").number();state.set("reads",read+1);
            const auto mode=c[2].number();if(mode==8||mode==12||mode==13||mode==14||mode==18)throw JSRangeError("count");
            if(mode==9)return JSSymbol::create("count");if(mode==23&&read==1)return "0";
            if(mode==24&&read==1)return -0.0;return state.get("count");
        },{log,state,mode}),JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
            event(c[0],"set:count");if(c[2].number()==10)throw JSRangeError("count-set");auto state=c[1];state.set("count",args[0]);
            if(c[2].number()==20){auto manager=c[3];prop(manager,"shaderCache",c[4]);}return {};
        },{log,state,mode,manager,otherMap}));
        if(mode==15)accessor(stage,"code",traceGetter(log,"get:code",{},1));
        const auto result=outcome([&]()->JsonValue{return call(manager,"remove",{object()}).sameValue(manager);});
        emit(O{{"kind","remove-protocol"},{"mode",mode},{"result",result},{"log",*log.toJSON()},{"count",encode(state.get("count"))},{"next",encode(state.get("next"))},{"reads",encode(state.get("reads"))}});
    }
}
inline void cacheProtocols() {
    for(unsigned which=0;which<2;++which)for(unsigned mode=0;mode<11;++mode){
        auto manager=WebGLShaderCache().object(),log=JSValue::array(),cache=object(),other=object(),stored=object();
        const char* field=which?"shaderCache":"materialCache";
        const Args resultValues{{},nullptr,false,7,JSValue::object()};
        const auto resultValue=mode<5?resultValues[mode]:JSValue{};
        accessor(cache,"get",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            event(c[0],"get:get");if(c[5].number()==5)throw JSRangeError("get");if(c[5].number()==6){auto manager=c[1];prop(manager,c[2].string().c_str(),c[3]);}
            return JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],"call:get");call(c[0],"push",{"key:"+args[0].toString()});if(c[5].number()==7)throw JSRangeError("get-call");return c[4];},c);
        },{log,manager,field,other,resultValue,mode}));
        accessor(cache,"set",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
            event(c[0],"get:set");if(c[2].number()==8)throw JSRangeError("set");if(c[2].number()==9)return 7;
            return JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
                event(c[0],"call:set");auto stored=c[1];stored.set("key",args[0]);stored.set("value",args[1]);if(c[2].number()==10)throw JSRangeError("set-call");return false;
            },c);
        },{log,stored,mode}));
        accessor(manager,field,traceGetter(log,"get:cache",cache));
        const auto result=outcome([&]()->JsonValue{return encode(call(manager,which?"_getShaderStage":"_getShaderCacheForMaterial",{"code"}));});
        const auto value=stored.get("value");
        emit(O{{"kind","cache-protocol"},{"which",which},{"mode",mode},{"result",result},{"log",*log.toJSON()},{"stored",encode(value)},{"id",value.isObject()?encode(value.get("id")):JsonValue("none")},{"set",isNativeSet(value)}});
    }
}
inline void disposeProtocols() {
    for(unsigned mode=0;mode<8;++mode){
        auto manager=WebGLShaderCache().object(),log=JSValue::array(),shader=object(),material=object(),other=object();
        accessor(other,"clear",traceGetter(log,"get:other-clear",traceMethod(log,"call:other-clear",{})));
        accessor(shader,"clear",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            event(c[0],"get:shader-clear");if(c[3].number()==1)throw JSRangeError("shader-clear");if(c[3].number()==6){auto manager=c[1];prop(manager,"materialCache",c[2]);}
            return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
                event(c[0],"call:shader-clear");if(c[3].number()==2)throw JSRangeError("shader-clear-call");if(c[3].number()==7){auto manager=c[1];prop(manager,"materialCache",c[2]);}return {};
            },c);
        },{log,manager,other,mode}));
        accessor(material,"clear",traceGetter(log,"get:material-clear",traceMethod(log,"call:material-clear",{},mode==5?1:0),mode==4?1:0));
        accessor(manager,"shaderCache",traceGetter(log,"get:shaderCache",shader,mode==0?1:0));
        accessor(manager,"materialCache",traceGetter(log,"get:materialCache",material,mode==3?1:0));
        const auto result=outcome([&]()->JsonValue{return encode(call(manager,"dispose"));});
        emit(O{{"kind","dispose-protocol"},{"mode",mode},{"result",result},{"log",*log.toJSON()}});
    }
}
inline void stageConstructionProtocols() {
    auto manager=WebGLShaderCache().object(),exemplar=call(manager,"_getShaderStage",{"stage-prototype"});
    const auto constructor=exemplar.get("constructor");auto prototype=constructor.get("prototype");
    for(unsigned mode=0;mode<7;++mode){
        auto log=JSValue::array(),state=object({{"entered",false}}),cache=WebGLShaderCache().object();
        for(const char* name:{"id","code","usedTimes"})prototype.erase(name);
        accessor(prototype,"id",{},JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
            event(c[0],"set:id");call(c[0],"push",{args[0]});if(c[2].number()==1)throw JSRangeError("id");
            auto state=c[1];if(c[2].number()==2&&!state.get("entered").toBoolean()){state.set("entered",true);call(c[3],"_getShaderStage",{"nested-stage"});}
            if(c[2].number()==3){auto target=self;prop(target,"code","before-code");}auto target=self;prop(target,"id",args[0]);return {};
        },{log,state,mode,cache}));
        accessor(prototype,"code",{},JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
            event(c[0],"set:code");if(c[1].number()==4)throw JSRangeError("code");auto target=self;prop(target,"code",args[0]);return {};
        },{log,mode}));
        accessor(prototype,"usedTimes",{},JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
            event(c[0],"set:usedTimes");if(c[1].number()==5)throw JSRangeError("usedTimes");auto target=self;prop(target,"usedTimes",args[0]);return {};
        },{log,mode}));
        if(mode==6)prop(cache.get("shaderCache"),"set",traceMethod(log,"cache:set",{},1));
        const auto result=outcome([&]()->JsonValue{
            const auto stage=call(cache,"_getShaderStage",{"code"});return O{{"id",encode(stage.get("id"))},{"code",encode(stage.get("code"))},{"usedTimes",encode(stage.get("usedTimes"))}};
        });
        const auto size=cache.get("shaderCache").get("size").number();
        for(const char* name:{"id","code","usedTimes"})prototype.erase(name);
        if(mode==6){auto map=cache.get("shaderCache");map.erase("set");}
        const auto next=call(cache,"_getShaderStage",{"after"});
        emit(O{{"kind","stage-construction"},{"mode",mode},{"result",result},{"log",*log.toJSON()},{"sizeBefore",size},{"nextId",encode(next.get("id"))}});
    }
}
inline void constructorProtocols() {
    const auto constructor=nativeWebGLShaderCacheConstructor();
    for(unsigned mode=0;mode<8;++mode){
        auto log=JSValue::array(),stored=object({{"entered",false}}),prototype=JSValue::object(constructor.get("prototype"));
        const auto newTarget=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Derived",0,JSValue::ConstructorKind::Callable);
        accessor(prototype,"shaderCache",{},JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
            event(c[0],"set:shaderCache");call(c[0],"push",{isNativeMap(args[0])});if(c[2].number()==1)throw JSRangeError("shader");
            if(c[2].number()==2)nativeWebGLShaderCacheConstructor().get("prototype").get("dispose").call(self);
            auto target=self;if(c[2].number()==3)prop(target,"materialCache","before");prop(target,"shaderCache",args[0]);return {};
        },{log,stored,mode}));
        accessor(prototype,"materialCache",{},JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
            event(c[0],"set:materialCache");call(c[0],"push",{isNativeMap(args[0])});if(c[2].number()==4)throw JSRangeError("material");
            auto state=c[1];if(c[2].number()==5&&!state.get("entered").toBoolean()){state.set("entered",true);nativeWebGLShaderCacheConstructor().construct({},c[3]);}
            auto target=self;prop(target,"materialCache",args[0]);return {};
        },{log,stored,mode,newTarget}));
        auto writableTarget=newTarget;writableTarget.set("prototype",mode==6?JSValue(nullptr):mode==7?JSValue(3):prototype);
        const auto result=outcome([&]()->JsonValue{
            const auto value=constructor.construct({},newTarget);return O{{"own",names(value)},{"shader",isNativeMap(value.get("shaderCache"))},{"material",isNativeMap(value.get("materialCache"))},{"derived",value.getPrototype().sameValue(prototype)}};
        });
        emit(O{{"kind","constructor-protocol"},{"mode",mode},{"result",result},{"log",*log.toJSON()}});
    }
}
inline void reentryProtocols() {
    for(unsigned mode=0;mode<8;++mode){
        auto manager=WebGLShaderCache().object(),material=object(),log=JSValue::array(),state=object({{"entered",false},{"count",0}});
        auto vertex=call(manager,"_getShaderStage",{"vertex"}),fragment=call(manager,"_getShaderStage",{"fragment"});
        const auto shaders=call(manager,"_getShaderCacheForMaterial",{material});
        accessor(vertex,"usedTimes",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            event(c[0],"get:vertex");auto state=c[1];if(!state.get("entered").toBoolean()){
                state.set("entered",true);const auto mode=c[2].number();
                if(mode==0)call(c[3],"dispose");if(mode==1)call(c[4],"clear");if(mode==2)call(c[4],"add",{c[5]});if(mode==3)call(c[3],"remove",{c[6]});
                if(mode==4)call(c[3],"update",{c[6],c[7],c[5]});if(mode==5)throw JSRangeError("vertex");
            }return state.get("count");
        },{log,state,mode,manager,shaders,fragment,material,vertex}),JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
            event(c[0],"set:vertex");if(c[2].number()==6)throw JSRangeError("vertex-set");auto state=c[1];state.set("count",args[0]);if(c[2].number()==7)call(c[3],"dispose");return {};
        },{log,state,mode,manager}));
        const auto result=outcome([&]()->JsonValue{return call(manager,"update",{material,vertex,fragment}).sameValue(manager);});
        A ids;auto record=getJSIterator(shaders);while(const auto stage=stepJSIterator(record))ids.push_back(encode(stage->get("code")));
        emit(O{{"kind","reentry"},{"mode",mode},{"result",result},{"log",*log.toJSON()},{"vertex",encode(state.get("count"))},{"fragment",encode(fragment.get("usedTimes"))},{"shaderSize",manager.get("shaderCache").get("size").number()},{"materialSize",manager.get("materialCache").get("size").number()},{"setCodes",ids}});
    }
}
inline void nativeMaterials() {
    auto shader=std::make_shared<ShaderMaterial>();auto raw=std::make_shared<RawShaderMaterial>();
    auto material=nativeMaterial(shader),rawMaterial=nativeMaterial(raw),manager=WebGLShaderCache().object();
    const auto vertex=call(manager,"getVertexShaderStage",{material}),fragment=call(manager,"getFragmentShaderStage",{material});
    call(manager,"update",{material,vertex,fragment});call(manager,"update",{rawMaterial,vertex,fragment});
    emit(O{{"kind","native-material"},{"mode",0},{"vertexCode",vertex.get("code").string()},{"fragmentCode",fragment.get("code").string()},{"vertexId",vertex.get("id").number()},{"fragmentId",fragment.get("id").number()},{"rawSame",call(manager,"getVertexShaderStage",{rawMaterial}).sameValue(vertex)&&call(manager,"getFragmentShaderStage",{rawMaterial}).sameValue(fragment)},{"vertexCount",vertex.get("usedTimes").number()},{"fragmentCount",fragment.get("usedTimes").number()}});
    shader->vertexShader="vertex-b";material.set("fragmentShader","fragment-b");
    const auto v2=call(manager,"getVertexShaderStage",{material}),f2=call(manager,"getFragmentShaderStage",{material});call(manager,"update",{material,v2,f2});
    emit(O{{"kind","native-material"},{"mode",1},{"vertexCode",v2.get("code").string()},{"fragmentCode",f2.get("code").string()},{"vertexId",v2.get("id").number()},{"fragmentId",f2.get("id").number()},{"nativeVertex",shader->vertexShader},{"nativeFragment",shader->fragmentShader},{"identity",material.sameValue(nativeMaterial(shader))},{"vertexCount",v2.get("usedTimes").number()},{"fragmentCount",f2.get("usedTimes").number()}});
    call(manager,"remove",{material});call(manager,"remove",{rawMaterial});
    emit(O{{"kind","native-material"},{"mode",2},{"vertexCount",vertex.get("usedTimes").number()},{"fragmentCount",fragment.get("usedTimes").number()},{"newVertexCount",v2.get("usedTimes").number()},{"newFragmentCount",f2.get("usedTimes").number()},{"shaderSize",manager.get("shaderCache").get("size").number()},{"materialSize",manager.get("materialCache").get("size").number()}});
}
} // namespace ShaderCacheFixture
