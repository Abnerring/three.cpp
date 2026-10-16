#pragma once
#include "animation/AnimationUtils.h"
#include "core/JSPromise.h"
#include "renderers/webgl/WebGLPrograms.h"
#include "renderers/webgl/WebGLShaderCache.h"
#include "renderers/shaders/ShaderLib.h"
#include "renderers/shaders/UniformsUtils.h"
#include "math/ColorManagement.h"
#include "core/Layers.h"
#include "core/JSMap.h"
#include "core/JSSet.h"
#include "utils.h"
#include <cmath>

namespace ProgramsFixture {
using Args=std::vector<JSValue>;
inline JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue();}
inline void data(JSValue o,const std::string& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;o.defineProperty(key,d);}
inline JSValue list(const Args& a){auto r=JSValue::array();for(std::size_t i=0;i<a.size();++i)data(r,std::to_string(i),a[i]);return r;}
inline JSValue dict(std::initializer_list<std::pair<const char*,JSValue>> a){auto r=JSValue::object();for(const auto& [k,v]:a)data(r,k,v);return r;}
inline void append(JSValue a,const JSValue& v){data(a,std::to_string(a.length()),v);}
inline JSValue encode(const JSValue& v,int depth=0){
    if(v.isUndefined())return "@undefined";
    if(v.isSymbol())return "@symbol";
    if(v.type()==JSValue::Type::Number){if(std::isnan(v.number()))return "@NaN";if(!std::isfinite(v.number()))return v.number()>0?"@Infinity":"@-Infinity";if(v.number()==0&&std::signbit(v.number()))return "@-0";}
    if(v.isCallable())return "@function:"+v.get("name").toString()+":"+v.get("length").toString();
    if(!v.isObject())return v;
    if(v.hasOwn("fixtureHandle"))return v.get("fixtureHandle");
    if(depth>6)return "@object";
    auto r=v.type()==JSValue::Type::Array?JSValue::array():JSValue::object();
    for(const auto& k:v.keys())data(r,k,encode(v.get(k),depth+1));return r;
}
inline JSValue caught(){try{throw;}catch(const JSThrownValue& e){return e.value.get("name");}catch(const JSRangeError&){return "RangeError";}catch(const JSTypeError&){return "TypeError";}catch(const std::exception&){return "Error";}}
template<class F>JSValue attempt(F f){try{return encode(f());}catch(...){return list({"throw",caught()});}}
inline void event(const JSValue& s,const Args& a){
    auto row=JSValue::array();for(const auto& v:a)append(row,encode(v));auto trace=s.get("trace");append(trace,row);
    if(trace.length()==s.get("config").get("failAt").toNumber())throw JSRangeError("event");
}
inline JSValue call(const JSValue& receiver,const char* method,const Args& args={}){return receiver.get(method).call(receiver,args);}
inline void nestedParameters(const JSValue& state,const char* point){
    if(!state.get("config").get("nestedAt").strictEquals(point)||state.get("nestedDone").toBoolean())return;
    auto s=state;data(s,"nestedDone",true);Args args;const auto values=s.get("nestedArgs");
    for(std::uint32_t i=0;i<values.length();++i)args.push_back(values.get(std::to_string(i)));
    const auto parameters=call(s.get("module"),"getParameters",args);
    event(s,{"nested-parameters",parameters.get("mapUv"),parameters.get("vertexUv1s"),parameters.get("vertexUv2s")});
}
inline void patch(JSValue target,const JSValue& value){if(value.isObject())for(const auto& k:value.keys())data(target,k,value.get(k));}
struct Restore {
    struct Entry{JSValue object;std::string key;std::optional<JSPropertyDescriptor> descriptor;};
    std::vector<Entry> entries;
    void save(JSValue object,const std::string& key){entries.push_back({object,key,object.getOwnPropertyDescriptor(key)});}
    ~Restore(){for(auto it=entries.rbegin();it!=entries.rend();++it){if(it->descriptor)it->object.defineProperty(it->key,*it->descriptor);else it->object.erase(it->key);}}
};
inline void traced(JSValue object,const std::string& key,const std::string& label,JSValue state,JSValue sequence={}){
    const auto value=object.get(key);auto counter=dict({{"n",0}});JSPropertyDescriptor d;
    d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
        event(c[0],{"get",c[1],self.sameValue(c[2])});
        if(c[4].isUndefined())return c[3];auto n=c[5].get("n").number();auto counter=c[5];counter.set("n",n+1);return c[4].get(JSValue(n));
    },{state,label,object,value,sequence,counter});d.enumerable=d.configurable=true;object.defineProperty(key,d);
}
inline JSValue fixture(JSValue config){
    Restore restore;auto state=dict({{"config",config},{"trace",JSValue::array()}});
    auto material=dict({{"type","MeshBasicMaterial"},{"name","fixture"},{"precision",nullptr},{"wireframe",false},{"transparent",false},{"blending",1},{"alphaToCoverage",false},{"normalMapType",0},{"toneMapped",true},{"side",0},{"fog",true},{"sizeAttenuation",true},{"vertexShader","vertex source"},{"fragmentShader","fragment source"},{"uniforms",dict({{"x",dict({{"value",3}})}})}});
    data(material,"customProgramCacheKey",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"customKey",self.sameValue(c[1])});nestedParameters(c[0],"customKey");return c[0].get("config").hasOwn("customKey")?c[0].get("config").get("customKey"):JSValue("custom-key");},{state,material},"customProgramCacheKey",0));
    auto attributes=dict({{"position",dict({{"itemSize",3}})},{"normal",dict({{"itemSize",3}})}});
    auto geometry=dict({{"attributes",attributes},{"morphAttributes",JSValue::object()}});
    auto object=dict({{"geometry",geometry},{"instanceColor",nullptr},{"morphTexture",nullptr},{"_colorsTexture",nullptr}});
    auto scene=dict({{"fog",nullptr},{"environment",nullptr}}),lights=JSValue::object();
    for(const char* key:{"sun","directional","point","spot","spotLightMap","rectArea","hemi","sunShadowMap","directionalShadowMap","pointShadowMap","spotShadowMap"})data(lights,key,JSValue::array());
    data(lights,"numSpotLightShadowsWithMaps",0);data(lights,"numLightProbes",0);
    auto renderer=dict({{"outputColorSpace","srgb"},{"toneMapping",4},{"shadowMap",dict({{"enabled",false},{"type",1}})}});
    auto depth=JSValue::object();data(depth,"getReversed",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"getReversed",self.sameValue(c[1])});return c[0].get("config").hasOwn("reversed")?c[0].get("config").get("reversed"):JSValue(false);},{state,depth},"getReversed"));
    data(renderer,"state",dict({{"buffers",dict({{"depth",depth}})}}));
    data(renderer,"getRenderTarget",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"getRenderTarget",self.sameValue(c[1])});return c[0].get("config").hasOwn("target")?c[0].get("config").get("target"):JSValue(nullptr);},{state,renderer},"getRenderTarget"));
    auto environments=JSValue::object();data(environments,"get",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{event(c[0],{"environment",self.sameValue(c[1]),arg(a,0),arg(a,1)});return c[0].get("config").hasOwn("envMap")?c[0].get("config").get("envMap"):arg(a,0);},{state,environments},"get",2));
    auto extensions=JSValue::object();data(extensions,"has",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{event(c[0],{"extension",self.sameValue(c[1]),arg(a,0)});const auto map=c[0].get("config").get("extensions");return map.isObject()?map.get(arg(a,0)):JSValue(false);},{state,extensions},"has",1));
    auto capabilities=dict({{"precision","highp"},{"logarithmicDepthBuffer",false}});
    data(capabilities,"getMaxPrecision",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{event(c[0],{"precision",self.sameValue(c[1]),arg(a,0)});return c[0].get("config").hasOwn("maxPrecision")?c[0].get("config").get("maxPrecision"):arg(a,0);},{state,capabilities},"getMaxPrecision",1));
    auto binding=JSValue::object(),clipping=dict({{"numPlanes",0},{"numIntersection",0}});
    patch(material,config.get("material"));patch(attributes,config.get("attributes"));patch(geometry,config.get("geometry"));patch(object,config.get("object"));patch(scene,config.get("scene"));patch(lights,config.get("lights"));patch(renderer,config.get("renderer"));patch(capabilities,config.get("capabilities"));patch(clipping,config.get("clipping"));
    if(config.get("deleteMaterial").isObject())for(std::uint32_t i=0;i<config.get("deleteMaterial").length();++i)material.erase(config.get("deleteMaterial").get(std::to_string(i)).toString());
    const auto shaderLib=nativeShaderLib();
    for(const auto& key:shaderLib.keys()){
        restore.save(shaderLib,key);data(shaderLib,key,dict({{"vertexShader","vertex:"+key},{"fragmentShader","fragment:"+key},{"uniforms",dict({{"x",dict({{"value",7}})}})}}));
    }
    const auto oldConsole=ThreeUtils::getConsoleFunction();
    struct ConsoleRestore{JSValue value;~ConsoleRestore(){ThreeUtils::setConsoleFunction(value);}} consoleRestore{oldConsole};
    ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{Args values{"console"};values.insert(values.end(),a.begin(),a.end());event(c[0],values);return {};},{state}));
    const auto cm=nativeColorManagement();
    if(config.hasOwn("workingColorSpace")){restore.save(cm,"workingColorSpace");data(cm,"workingColorSpace",config.get("workingColorSpace"));}
    if(config.hasOwn("transferResult")){
        restore.save(cm,"getTransfer");auto target=cm;JSPropertyDescriptor d;d.enumerable=d.configurable=true;
        auto fn=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{event(c[0],{"transfer-call",self.sameValue(c[1]),arg(a,0)});return c[0].get("config").get("transferResult");},{state,cm},"getTransfer",1);
        d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"transfer-method",self.sameValue(c[1])});return c[2];},{state,cm,fn});target.defineProperty("getTransfer",d);
    }
    auto objects=dict({{"material",material},{"geometry",geometry},{"attributes",attributes},{"morph",geometry.get("morphAttributes")},{"object",object},{"scene",scene},{"lights",lights},{"renderer",renderer},{"capabilities",capabilities},{"clipping",clipping},{"environments",environments},{"extensions",extensions},{"depth",depth}});
    if(config.get("symbolFields").isObject())for(std::uint32_t i=0;i<config.get("symbolFields").length();++i){
        const auto pair=config.get("symbolFields").get(std::to_string(i));data(objects.get(pair.get("0")),pair.get("1").toString(),JSValue::symbol("fixture"));
    }
    if(config.get("symbolChannel").toBoolean())data(material,"map",dict({{"channel",JSValue::symbol("channel")}}));
    if(config.get("traceReads").toBoolean()){
        for(const auto& group:config.get("readKeys").keys()){
            auto target=objects.get(group);const auto keys=config.get("readKeys").get(group);
            for(std::uint32_t i=0;i<keys.length();++i){const auto key=keys.get(std::to_string(i)).toString();traced(target,key,group+"."+key,state);}
        }
    }
    if(config.get("sequences").isObject())for(const auto& group:config.get("sequences").keys()){
        const auto sequences=config.get("sequences").get(group);for(const auto& key:sequences.keys())traced(objects.get(group),key,group+"."+key,state,sequences.get(key));
    }
    if(config.get("conversion").isObject())for(const auto& group:config.get("conversion").keys()){
        const auto entries=config.get("conversion").get(group);for(const auto& key:entries.keys()){
            const auto spec=entries.get(key);auto value=JSValue::object();
            value.setPrimitiveConverter(JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{event(c[0],{"convert",c[1],arg(a,0),self.sameValue(c[2])});if(c[3].get("throws").toBoolean())throw JSRangeError("convert");return c[3].hasOwn("value")?c[3].get("value"):JSValue(1);},{state,group+"."+key,value,spec}));
            data(objects.get(group),key,value);
        }
    }
    auto results=JSValue::array();JSValue module;
    try{module=nativeWebGLProgramsConstructor().call({}, {renderer,environments,extensions,capabilities,binding,clipping});}
    catch(...){append(results,list({"throw",caught()}));return dict({{"results",results},{"trace",state.get("trace")}});}
    data(state,"module",module);
    if(config.hasOwn("nestedAt")){
        const auto nested=dict({{"type","MeshBasicMaterial"},{"name","nested"},{"precision",nullptr},{"toneMapped",false},{"map",dict({{"channel",2}})},{"wireframe",false},{"customProgramCacheKey",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "nested";},{},"customProgramCacheKey",0)}});
        data(state,"nestedArgs",list({nested,lights,JSValue::array(),scene,object,JSValue::array()}));
    }
    auto kind=config.get("kind");
    if(kind.strictEquals("setProtocol")||kind.strictEquals("layersProtocol")){
        auto proto=kind.strictEquals("setProtocol")?nativeSetConstructor().get("prototype"):nativeLayersConstructor().get("prototype");
        const auto methods=kind.strictEquals("setProtocol")?std::vector<std::string>{"add","has","clear"}:std::vector<std::string>{"disableAll","enable"};
        for(const auto& method:methods){
            restore.save(proto,method);const auto original=proto.get(method);JSPropertyDescriptor d;d.enumerable=false;d.configurable=true;
            const auto fn=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
                event(c[0],{"protocol-call",c[1],arg(a,0)});
                if(c[1].strictEquals("add"))nestedParameters(c[0],"add");
                const auto config=c[0].get("config");
                if(config.hasOwn("hasResult")&&c[1].strictEquals("has"))return config.get("hasResult");
                if(config.get("ignoreAdd").toBoolean()&&c[1].strictEquals("add"))return self;
                if(config.get("nestedKey").toBoolean()&&c[1].strictEquals("enable")&&!c[0].get("nestedDone").toBoolean()){
                    auto s=c[0];data(s,"nestedDone",true);event(s,{"nested-key",call(c[3],"getProgramCacheKey",{dict({{"shaderID","nested"},{"isRawShaderMaterial",false},{"fog",true},{"instancingColor",true}})})});
                }
                return c[2].call(self,a);
            },{state,method,original,module},method,method=="clear"||method=="disableAll"?0:1);
            d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"protocol-method",c[1]});return c[2];},{state,method,fn});proto.defineProperty(method,d);
        }
    }
    if(kind.strictEquals("lifecycle")){
        auto gl=JSValue::object(),counts=dict({{"program",0},{"shader",0}}),created=JSValue::array();
        data(gl,"VERTEX_SHADER",35633);data(gl,"FRAGMENT_SHADER",35632);
        for(const auto* method:{"createProgram","createShader","shaderSource","compileShader","attachShader","bindAttribLocation","linkProgram","deleteProgram"})
            data(gl,method,JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
                Args row{"gl",c[1],self.sameValue(c[2])};row.insert(row.end(),a.begin(),a.end());event(c[0],row);
                if(c[1].strictEquals("createProgram")||c[1].strictEquals("createShader")){
                    const auto kind=c[1].strictEquals("createProgram")?"program":"shader";auto counts=c[3];const auto next=counts.get(kind).number()+1;data(counts,kind,next);return dict({{"fixtureHandle",std::string(kind)+":"+JSValue(next).toString()}});
                }return {};
            },{state,method,gl,counts},method));
        data(renderer,"getContext",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"getContext",self.sameValue(c[1])});return c[2];},{state,renderer,gl},"getContext"));
        data(binding,"releaseStatesOfProgram",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{event(c[0],{"releaseBinding",self.sameValue(c[1]),arg(a,0).get("id")});return {};},{state,binding},"releaseStatesOfProgram",1));
        auto parameters=dict({{"isRawShaderMaterial",true},{"shaderType","Fixture"},{"shaderName","fixture"},{"defines",JSValue::object()},{"vertexShader","void main() { gl_Position = vec4( 0.0 ); }"},{"fragmentShader","void main() { gl_FragColor = vec4( 1.0 ); }"},{"rendererExtensionParallelShaderCompile",false}});
        auto programsByKey=JSValue::object();
        const auto actions=config.get("actions");
        for(std::uint32_t i=0;i<actions.length();++i){
            const auto action=actions.get(std::to_string(i)).toString();
            if(action=="recover")data(config,"failAt",0);
            append(results,attempt([&]()->JSValue{
                if(action=="dispose")return call(module,"dispose");
                if(action=="recover")return {};
                const auto key=action.substr(action.size()-1);
                if(action.starts_with("acquire")){
                    const auto p=call(module,"acquireProgram",{parameters,key});const auto old=programsByKey.get(key);data(programsByKey,key,p);
                    return list({p.get("id"),p.get("cacheKey"),p.get("usedTimes"),old.sameValue(p),p.get("program")});
                }
                return call(module,"releaseProgram",{programsByKey.get(key)});
            }));
            auto snapshot=JSValue::array();const auto programs=module.get("programs");for(std::uint32_t j=0;j<programs.length();++j){const auto p=programs.get(std::to_string(j));append(snapshot,list({p.get("id"),p.get("usedTimes"),p.get("program")}));}append(results,encode(snapshot));
        }
    }else if(kind.strictEquals("cacheProtocol")){
        auto program=dict({{"usedTimes",config.hasOwn("usedTimes")?config.get("usedTimes"):JSValue(1)},{"cacheKey","cache"}});
        data(program,"destroy",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"destroy",self.sameValue(c[1]),self.get("usedTimes")});return {};},{state,program},"destroy"));
        if(config.get("countConversion").toBoolean()){
            auto count=JSValue::object();count.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{"count-convert",arg(a,0)});return c[0].get("config").get("usedTimes");},{state}));data(program,"usedTimes",count);
        }
        if(config.get("countAccessor").toBoolean()){
            auto storage=dict({{"value",program.get("usedTimes")}});JSPropertyDescriptor d;d.enumerable=d.configurable=true;
            d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"count-get",self.sameValue(c[1])});return c[2].get("value");},{state,program,storage});
            d.setter=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{event(c[0],{"count-set",self.sameValue(c[1]),arg(a,0)});auto storage=c[2];data(storage,"value",arg(a,0));return {};},{state,program,storage});program.defineProperty("usedTimes",d);
        }
        auto mapProto=nativeMapConstructor().get("prototype");
        for(const char* method:{"get","delete"}){
            restore.save(mapProto,method);JSPropertyDescriptor d;d.enumerable=false;d.configurable=true;
            const auto fn=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
                event(c[0],{"map-call",c[1],JSMap::isCollection(self),arg(a,0)});
                if(c[1].strictEquals("get"))return c[2];return true;
            },{state,method,program},method,1);
            d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"map-method",c[1],JSMap::isCollection(self)});return c[2];},{state,method,fn});mapProto.defineProperty(method,d);
        }
        const auto operation=config.get("operation");
        if(operation.strictEquals("acquire"))append(results,attempt([&]{auto p=call(module,"acquireProgram",{JSValue::object(),"cache"});return list({p.sameValue(program),p.get("usedTimes")});}));
        else{
            auto array=module.get("programs");append(array,program);
            if(config.get("arrayAlias").toBoolean())data(module,"programs",JSValue::array());
            if(config.get("missingProgram").toBoolean())array.set("length",0);
            append(results,attempt([&]{return call(module,"releaseProgram",{program});}));
            append(results,attempt([&]{return program.get("usedTimes");}));
            append(results,array.length());
        }
    }else if(kind.strictEquals("arrayProtocol")){
        auto proto=JSValue::array().getPrototype();
        for(const char* method:{"push","join"}){
            restore.save(proto,method);const auto original=proto.get(method);JSPropertyDescriptor d;d.enumerable=false;d.configurable=true;
            const auto fn=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
                event(c[0],{"array-call",c[1],self.get("length"),arg(a,0)});
                if(c[0].get("config").get("replaceJoin").toBoolean()&&c[1].strictEquals("join"))return c[0].get("config").get("joinResult");
                return c[2].call(self,a);
            },{state,method,original},method,1);
            d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"array-method",c[1],self.get("length")});return c[2];},{state,method,fn});proto.defineProperty(method,d);
        }
        append(results,attempt([&]{return call(module,"getProgramCacheKey",{config.get("parameters")});}));
    }else if(kind.strictEquals("layersProtocol")){
        append(results,attempt([&]{return call(module,"getProgramCacheKey",{config.get("parameters")});}));
    }else if(kind.strictEquals("metadata")){
        for(const auto& key:module.keys()){
            const auto value=module.get(key);auto row=list({key,encode(value)});
            if(value.isCallable()){
                append(row,value.isConstructible());auto names=JSValue::array();for(const auto& n:value.ownPropertyNames())append(names,n);append(row,names);
                const auto d=value.getOwnPropertyDescriptor("prototype");append(row,list({d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false),value.get("prototype").get("constructor").sameValue(value)}));
            }append(results,row);
        }
    }else if(kind.strictEquals("key")){
        auto parameters=config.get("parameters");
        if(config.hasOwn("symbolParameter"))data(parameters,config.get("symbolParameter").toString(),JSValue::symbol("parameter"));
        if(config.get("definesPrototype").isObject())parameters.get("defines").setPrototype(config.get("definesPrototype"));
        if(config.get("mutateDefines").toBoolean()){
            auto defines=parameters.get("defines");JSPropertyDescriptor d;d.enumerable=d.configurable=true;
            d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
                event(c[0],{"define-get"});auto definitions=c[1];definitions.erase("b");data(definitions,"new",9);
                if(c[0].get("config").get("replaceDefines").toBoolean())data(c[2],"defines",dict({{"a",5},{"b",6},{"c",7},{"inherited",8}}));
                return 1;
            },{state,defines,parameters});defines.defineProperty("a",d);
        }
        if(config.get("traceReads").toBoolean()){
            const auto keys=config.get("parameterKeys");for(std::uint32_t i=0;i<keys.length();++i){const auto key=keys.get(std::to_string(i)).toString();traced(parameters,key,"parameters."+key,state);}
        }
        append(results,attempt([&]{return call(module,"getProgramCacheKey",{parameters});}));
    }else if(kind.strictEquals("construct")){
        const auto method=config.get("method").toString();const auto fn=module.get(method);
        auto marker=dict({{"marker",1}});
        auto target=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Target",0,JSValue::ConstructorKind::Callable);
        if(config.get("customTarget").toBoolean())target.set("prototype",marker);
        Args values;
        if(method=="getProgramCacheKey")values={config.get("parameters")};
        else if(method=="getUniforms")values={material};
        append(results,attempt([&]{
            const auto result=config.get("customTarget").toBoolean()?fn.construct(values,target):fn.construct(values);
            return dict({{"object",result.isObject()},{"defaultPrototype",result.getPrototype().sameValue(fn.get("prototype"))},{"customPrototype",result.getPrototype().sameValue(marker)},{"uniformAlias",result.sameValue(material.get("uniforms"))}});
        }));
    }else if(kind.strictEquals("uniforms")){
        append(results,attempt([&]{return call(module,"getUniforms",{material});}));
    }else{
        const auto repeat=config.hasOwn("repeat")?config.get("repeat").toNumber():1;
        for(int i=0;i<repeat;++i){
            if(i>0){data(config,"failAt",0);patch(material,config.get("recoveryMaterial"));}
            append(results,attempt([&]{
                auto p=call(module,"getParameters",{material,lights,config.hasOwn("shadows")?config.get("shadows"):JSValue::array(),scene,object,config.hasOwn("lightProbeGrids")?config.get("lightProbeGrids"):JSValue::array()});
                auto r=dict({{"parameters",encode(p)}});data(r,"key",call(module,"getProgramCacheKey",{p}));return r;
            }));
        }
    }
    return dict({{"results",results},{"trace",state.get("trace")}});
}
}
