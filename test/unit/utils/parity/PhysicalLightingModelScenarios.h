#pragma once
#include "nodes/functions/PhysicalLightingModel.h"
#include "nodes/core/LightingModel.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/StackNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "core/JSPromise.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace PhysicalLightingScenarios {
using namespace LightingNodeRuntime;
inline JSValue object() { return JSValue::object(nullptr); }
inline JSValue call(const JSValue& value,const char* key,const Args& args={}) { return value.get(key).call(value,args); }
inline JSValue array(const Args& values={}) { auto out=JSValue::array();for(const auto& value:values)call(out,"push",{value});return out; }
inline void push(const JSValue& out,const JSValue& value) { call(out,"push",{value}); }
inline JSValue node(const char* type,const Args& args={}) { return nativeTSLBaseBinding(type).call({},args); }
inline JSValue variable(const char* name,const char* type="vec3") { return call(node(type),"toVar",{name}); }
inline JSValue invokeModel(const char* method,const JSValue& model,const Args& args={}) { return nativePhysicalLightingModelConstructor().get("prototype").get(method).call(model,args); }
inline JSValue helperBody(const char* name) { const auto value=nativePhysicalLightingModelHelper(name);return value.get("isFn").toBoolean()?value.get("shaderNode").get("jsFunc"):value; }
inline JSValue attempt(const std::function<JSValue()>& fn) {
    try { return fn(); } catch(const JSThrownValue& error) { return array({"throw",error.value}); }
    catch(const JSTypeError&) { return array({"error","TypeError"}); }
    catch(const std::exception& error) { return array({"native-error",error.what()}); }
}
inline const std::vector<std::string>& tagNames() {
    static const std::vector<std::string> names={"diffuseColor","diffuseContribution","diffuseRoughness","specularColor","specularColorBlended","specularF90","roughness","metalness","clearcoat","clearcoatRoughness","sheen","sheenRoughness","iridescence","iridescenceIOR","iridescenceThickness","ior","thickness","transmission","attenuationDistance","attenuationColor","dispersion","retroreflectivity","normalView","clearcoatNormalView","normalWorld","positionViewDirection","positionView","positionWorld","cameraPosition","cameraProjectionMatrix","cameraViewMatrix","cameraViewport","modelWorldMatrix","screenSize","viewportBackSideTexture","viewportFrontSideTexture","clearcoatF0","clearcoatF90","XYZ_TO_REC709"};return names;
}
inline const std::vector<std::string>& helperNames() {
    static const std::vector<std::string> names={"getVolumeTransmissionRay","applyIorToRoughness","getTransmissionSample","volumeAttenuation","getIBLVolumeRefraction","Fresnel0ToIor","IorToFresnel0","evalSensitivity","evalIridescence","IBLSheenBRDF"};return names;
}
class Graph {
    Args seen_;JSValue objects_=array();std::vector<std::pair<JSValue,std::string>> tags_;
    JSValue encode(const JSValue& value) {
        if(value.isUndefined())return array({"undefined"});
        if(value.isCallable())return array({"function"});
        if(value.isSymbol())return array({"symbol"});
        if(value.type()==JSValue::Type::Number){const auto n=value.toNumber();if(std::isnan(n))return array({"number","NaN"});if(!std::isfinite(n))return array({"number",n>0?"Infinity":"-Infinity"});if(n==0&&std::signbit(n))return array({"number","-0"});}
        if(!value.isObject())return value;
        for(const auto& [tagged,name]:tags_)if(value.sameValue(tagged))return array({"import",name});
        for(unsigned i=0;i<seen_.size();++i)if(value.sameValue(seen_[i]))return array({"ref",i});
        const auto index=seen_.size();seen_.push_back(value);auto record=object(),properties=array();record.set("kind",value.type()==JSValue::Type::Array?"array":"object");record.set("properties",properties);push(objects_,record);
        if(value.type()==JSValue::Type::Array)record.set("length",value.length());
        if(value.get("isNode").strictEquals(true))record.set("nodeType",value.get("type"));
        auto keys=value.keys();std::sort(keys.begin(),keys.end());
        for(const auto& key:keys){if(key=="id"||key=="uuid"||key=="stackTrace"||key=="_cacheKey"||key=="_cacheKeyVersion")continue;push(properties,array({key,encode(value.get(key))}));}
        return array({"ref",static_cast<double>(index)});
    }
public:
    Graph() {
        for(const auto& name:tagNames()) { const auto value=nativePhysicalLightingModelHelper(name);if(value.isObject())tags_.push_back({value,name}); }
        for(const auto& name:helperNames()) { const auto value=nativePhysicalLightingModelHelper(name);if(value.isObject()&&value.get("isFn").toBoolean())tags_.push_back({value.get("shaderNode"),"helper:"+name}); }
        for(const auto* name:{"BRDF_Lambert","BRDF_EON","EON_DirectionalAlbedo","BRDF_GGX","DFGLUT","EnvironmentBRDF","F_Schlick","Schlick_to_F0","BRDF_Sheen","LTC_Evaluate","LTC_Uv","textureBicubicLevel"}) { const auto value=nativePhysicalLightingModelHelper(name);if(value.isObject()&&value.get("isFn").toBoolean())tags_.push_back({value.get("shaderNode"),std::string("shader:")+name}); }
        for(const auto* name:{"viewportBackSideTexture","viewportFrontSideTexture"}) { const auto texture=nativePhysicalLightingModelHelper(name).get("value");if(texture.isObject())tags_.push_back({texture,std::string(name)+":value"}); }
    }
    JSValue operator()(const JSValue& value) {auto out=object();out.set("root",encode(value));out.set("objects",objects_);return out;}
};
inline JSValue functionMetadata(const JSValue& value) {auto out=object();out.set("name",value.get("name"));out.set("length",value.get("length"));out.set("source",call(value,"toString"));return out;}
inline JSValue metadata() {
    const auto ctor=nativePhysicalLightingModelConstructor(),proto=ctor.get("prototype");auto out=functionMetadata(ctor),methods=object(),keys=array(),helpers=object();
    out.set("baseSame",ctor.getPrototype().sameValue(nativeLightingModelConstructor()));out.set("prototypeBaseSame",proto.getPrototype().sameValue(nativeLightingModelConstructor().get("prototype")));
    for(const auto& key:proto.ownPropertyNames())push(keys,key);out.set("prototypeKeys",keys);
    for(const auto* name:{"start","computeMultiscattering","direct","directRectArea","indirect","indirectDiffuse","indirectSpecular","ambientOcclusion","finish"})methods.set(name,functionMetadata(proto.get(name)));out.set("methods",methods);
    for(const auto& name:helperNames()){const auto helper=nativePhysicalLightingModelHelper(name);auto row=object();if(helper.get("isFn").toBoolean()){const auto shader=helper.get("shaderNode");row.set("callback",functionMetadata(shader.get("jsFunc")));row.set("nodeType",shader.get("nodeType").isUndefined()?JSValue(nullptr):shader.get("nodeType"));row.set("inputs",shader.get("layout").isNull()?JSValue(nullptr):shader.get("layout").get("inputs"));}else row.set("callback",functionMetadata(helper));helpers.set(name,row);}out.set("helpers",helpers);return out;
}
inline JSValue construction() {
    auto out=array();const auto custom=object();
    for(const auto& args:std::vector<Args>{{},{JSValue{},nullptr,0,1,"",custom,false,true},{true,true,true,true,true,true,true,true}}){const auto model=nativePhysicalLightingModelConstructor().construct(args);auto row=object(),keys=array();for(const auto& key:model.keys())push(keys,key);row.set("keys",keys);auto values=array();for(const auto* key:{"clearcoat","sheen","iridescence","anisotropy","transmission","dispersion","retroreflection","diffuseRoughness"}){const auto value=model.get(key);push(values,value.sameValue(custom)?JSValue("custom-identity"):value);}row.set("values",values);auto caches=array();for(const auto* key:{"clearcoatRadiance","clearcoatSpecularDirect","clearcoatSpecularIndirect","sheenSpecularDirect","sheenSpecularIndirect","iridescenceFresnel","iridescenceF0Dielectric","iridescenceF0Metallic","dfg","eonDirectionalAlbedo","multiScatteringCompensation","singleScatteringDielectric","multiScatteringDielectric"})push(caches,model.get(key).isNull());row.set("nullCaches",caches);push(out,row);}return out;
}
struct StackScope {
    JSValue old=nativeGetCurrentStack(),stack=nativeStackNodeConstructor().construct();
    StackScope(){nativeSetCurrentStack(stack);}~StackScope(){nativeSetCurrentStack(old);}
};
inline JSValue context() {
    auto out=object(),reflected=object();for(const auto* name:{"directDiffuse","directSpecular","indirectDiffuse","indirectSpecular"})reflected.set(name,variable(name));out.set("reflectedLight",reflected);
    for(const auto* name:{"irradiance","radiance","iblIrradiance","outgoingLight"})out.set(name,variable(name));out.set("ambientOcclusion",node("float",{.75}));return out;
}
inline JSValue builder(const JSValue& ctx,const JSValue& events) {
    auto out=object(),lights=object();out.set("context",ctx);out.set("lightsNode",lights);
    lights.set("getLightNodes",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"getLightNodes");return c[1];},{events,array({"light-fixture"})}));
    lights.set("setupLights",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{push(c[0],array({"setupLights",arg(args,0).sameValue(c[1]),arg(args,1)}));return {};},{events,out}));return out;
}
inline Args flags(unsigned mask){Args values;for(unsigned i=0;i<8;++i)values.push_back(bool(mask&(1u<<i)));return values;}
inline JSValue state(const JSValue& model){auto out=object();for(const auto& key:model.keys()){const auto v=model.get(key);if(!v.isCallable())out.set(key,v);}return out;}
inline JSValue ready(const Args& featureFlags,const JSValue& b,const JSValue& events){auto model=nativePhysicalLightingModelConstructor().construct(featureFlags);model.set("indirect",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"indirect");return {};},{events}));invokeModel("start",model,{b});model.erase("indirect");return model;}
inline JSValue lighting(unsigned mask,const std::string& method) {
    StackScope scope;const auto events=array(),ctx=context(),b=builder(ctx,events);auto model=ready(flags(mask),b,events),out=object();JSValue result;
    if(method=="start")result={};else if(method=="direct"){auto data=object();data.set("lightDirection",node("vec3",{.2,.3,.8}));data.set("lightColor",node("vec3",{2.,3.,4.}));data.set("reflectedLight",ctx.get("reflectedLight"));result=invokeModel("direct",model,{data,b});}
    else if(method=="directRectArea"){auto data=object();data.set("lightColor",node("vec3",{2.,3.,4.}));data.set("lightPosition",node("vec3",{0.,1.,2.}));data.set("halfWidth",node("vec3",{1.,0.,0.}));data.set("halfHeight",node("vec3",{0.,1.,0.}));data.set("reflectedLight",ctx.get("reflectedLight"));for(const auto* key:{"ltc_1","ltc_2"}){auto sampler=object();sampler.set("sample",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{push(c[0],array({c[1],arg(args,0)}));return node("vec4",{.2,.4,.6,.8});},{events,key}));data.set(key,sampler);}result=invokeModel("directRectArea",model,{data,b});}
    else result=invokeModel(method.c_str(),model,{b});
    out.set("returnUndefined",result.isUndefined());out.set("model",state(model));out.set("context",ctx);out.set("events",events);out.set("stack",scope.stack.get("nodes"));return Graph()(out);
}
inline JSValue multiscattering(unsigned variant) {
    StackScope scope;auto model=nativePhysicalLightingModelConstructor().construct(),out=object();model.set("dfg",node("vec2",{.45,.12}));const auto single=variable("single"),multi=variable("multi"),f0=node("vec3",{.04,.1,.3}),f90=node("float",{1.});Args values{single,multi,f90,f0};if(variant==1)values.push_back(nullptr);if(variant==2)values.push_back(0);if(variant==3)values.push_back(node("vec3",{.5,.2,.1}));out.set("returnUndefined",invokeModel("computeMultiscattering",model,values).isUndefined());out.set("stack",scope.stack.get("nodes"));out.set("single",single);out.set("multi",multi);return Graph()(out);
}
inline JSValue indirectOrder(const std::string& throwAt,bool reenter=false) {
    auto self=object(),events=array(),control=object();control.set("entered",false);const auto b=object();
    for(const auto* key:{"indirectDiffuse","indirectSpecular","ambientOcclusion"})self.set(key,JSValue::function(+[](const JSValue& receiver,const Args& args,const Args& c)->JSValue{push(c[0],array({c[1],receiver.sameValue(c[2]),arg(args,0).sameValue(c[3])}));if(c[4].strictEquals(c[1]))throw JSThrownValue{c[1]};if(c[5].toBoolean()&&c[1].strictEquals("indirectDiffuse")&&!c[6].get("entered").toBoolean()){auto control=c[6];control.set("entered",true);invokeModel("indirect",c[2],{c[3]});}return {};},{events,key,self,b,throwAt,reenter,control}));auto out=object();const auto result=attempt([&]{return invokeModel("indirect",self,{b});});out.set("result",result.isUndefined()?JSValue("undefined"):result);out.set("events",events);return out;
}
inline JSValue helper(const std::string& name,unsigned variant=0) {
    StackScope scope;JSValue inputs;Args args;auto out=object();
    const auto f=node("float",{.5}),ior=node("float",{1.5}),n=node("vec3",{0.,0.,1.}),v=node("vec3",{.1,.2,1.}),color=node("vec3",{.2,.4,.8});
    if(name=="applyIorToRoughness")inputs=array({f,ior});
    if(name=="getVolumeTransmissionRay")inputs=array({n,v,f,ior,node("mat4")});
    if(name=="volumeAttenuation")inputs=array({f,color,node("float",{variant==0?0.:2.})});
    if(name=="IBLSheenBRDF"){inputs=object();inputs.set("normal",n);inputs.set("viewDir",v);inputs.set("roughness",f);}
    if(name=="evalIridescence"){inputs=object();inputs.set("outsideIOR",node("float",{1.}));inputs.set("eta2",ior);inputs.set("cosTheta1",f);inputs.set("thinFilmThickness",node("float",{300.}));inputs.set("baseF0",color);}
    if(name=="getTransmissionSample"){inputs=array({node("vec2",{.25,.75}),f,ior});auto b=object(),material=object();material.set("side",variant==0?0:1);b.set("material",material);args={inputs,b};}
    if(name=="getIBLVolumeRefraction")inputs=array({n,v,f,color,color,node("float",{1.}),node("vec3"),node("mat4"),node("mat4"),node("mat4"),ior,f,color,node("float",{2.}),variant==0?JSValue(nullptr):node("float",{.7})});
    if(args.empty())args={inputs};if(name=="Fresnel0ToIor")args={color};if(name=="IorToFresnel0")args={ior,node("float",{1.})};if(name=="evalSensitivity")args={node("float",{500.}),node("vec3",{0.,1.,2.})};
    const auto result=helperBody(name.c_str()).call({},args);out.set("result",result);out.set("stack",scope.stack.get("nodes"));
    // Execute real deferred loop callbacks once with an integer node. The loop's
    // original bounds remain in the graph; this validates the emitted loop body.
    auto expansions=array(),ifBodies=array();Args seen;std::function<void(const JSValue&)> walk=[&](const JSValue& value){if(!value.isObject()||value.isCallable())return;for(const auto& old:seen)if(old.sameValue(value))return;seen.push_back(value);if(value.get("type").strictEquals("ConditionalNode")){for(const auto* branch:{"ifNode","elseNode"}){const auto candidate=value.get(branch);if(candidate.isObject()&&candidate.get("jsFunc").isCallable()){auto row=object();row.set("branch",branch);row.set("result",candidate.get("jsFunc").call({},{}));push(ifBodies,row);}}}if(value.get("type").strictEquals("LoopNode")){const auto params=value.get("params"),fn=params.get(JSValue(params.length()-1)),body=fn.get("isFn").toBoolean()?fn.get("shaderNode").get("jsFunc"):fn;auto vars=object();const auto name=params.get("0").get("name");vars.set(name.isUndefined()?"i":name.toString(),node("int",{1}));auto row=object();row.set("result",body.call(params,{vars}));row.set("inputs",vars);push(expansions,row);}for(const auto& key:value.keys())if(key!="stackTrace")walk(value.get(key));};walk(scope.stack.get("nodes"));out.set("loopBodies",expansions);out.set("ifBodies",ifBodies);return Graph()(out);
}
inline JSValue numericBuilder(const char* name) {
    StackScope scope;auto inputs=name==std::string("applyIorToRoughness")?array({node("float",{.35}),node("float",{1.4})}):object();if(name==std::string("IBLSheenBRDF")){inputs.set("normal",node("vec3",{0.,0.,1.}));inputs.set("viewDir",node("vec3",{.1,.2,.9}));inputs.set("roughness",node("float",{.4}));}
    const auto result=helperBody(name).call({}, {inputs});auto renderer=object(),debug=object(),diagnostics=object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);renderer.set("coordinateSystem",2001);auto b=nativeNodeBuilderConstructor().construct({nullptr,renderer,nullptr});b.set("shaderStage","fragment");auto steps=array();for(const auto* stage:{"setup","analyze","generate"}){b.set("buildStage",stage);push(steps,attempt([&]{const auto value=call(result,"build",{b,"float"});return value.isObject()&&value.get("isNode").toBoolean()?JSValue("node"):value;}));}auto out=object();out.set("steps",steps);out.set("flow",b.get("flow"));return Graph()(out);
}
inline void observe(JSValue target,const char* key,const std::string& label,const JSValue& events,const std::string& throwAt) {
    auto storage=object();storage.set("value",target.get(key));
    JSPropertyDescriptor descriptor;descriptor.enumerable=descriptor.configurable=true;
    descriptor.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{const auto label="get:"+c[0].toString();push(c[1],label);if(c[2].strictEquals(label))throw JSThrownValue(label);return c[3].get("value");},{label,events,throwAt,storage});
    descriptor.setter=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{const auto label="set:"+c[0].toString();push(c[1],label);if(c[2].strictEquals(label))throw JSThrownValue(label);auto storage=c[3];storage.set("value",arg(args,0));return {};},{label,events,throwAt,storage});
    target.defineProperty(key,descriptor);
}
inline JSValue observed(const std::string& method,const std::string& throwAt) {
    StackScope scope;auto model=nativePhysicalLightingModelConstructor().construct(),events=array(),ctx=context(),b=builder(ctx,events),data=object();
    model.set("indirect",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"call:indirect");if(c[1].strictEquals("call:indirect"))throw JSThrownValue("call:indirect");return {};},{events,throwAt}));
    if(method=="direct") {
        model.set("multiScatteringCompensation",node("vec3",{1.}));data.set("lightDirection",node("vec3",{.2,.3,.8}));data.set("lightColor",node("vec3",{2.,3.,4.}));data.set("reflectedLight",ctx.get("reflectedLight"));
        for(const auto* key:{"lightDirection","lightColor","reflectedLight"})observe(data,key,std::string("data.")+key,events,throwAt);
        for(const auto* key:{"sheen","clearcoat","iridescenceFresnel","iridescence","anisotropy","retroreflection","diffuseRoughness","multiScatteringCompensation"})observe(model,key,std::string("self.")+key,events,throwAt);
    } else {
        for(const auto* key:{"clearcoat","sheen","iridescence","transmission","dfg","singleScatteringDielectric","multiScatteringDielectric","iridescenceF0Dielectric","computeMultiscattering","diffuseRoughness"})observe(model,key,std::string("self.")+key,events,throwAt);
        const auto lights=b.get("lightsNode");observe(b,"lightsNode","builder.lightsNode",events,throwAt);
        for(const auto* key:{"getLightNodes","setupLights"})observe(lights,key,std::string("lights.")+key,events,throwAt);
        // This read only creates the fixture; the observed execution starts now.
        events.set("length",0);
    }
    auto out=object();const auto result=attempt([&]{return invokeModel(method.c_str(),model,method=="direct"?Args{data,b}:Args{b});});out.set("result",result.isUndefined()?JSValue("undefined"):result);out.set("events",events);out.set("stack",scope.stack.get("nodes"));return Graph()(out);
}
inline const std::vector<std::string>& arrayHelperNames() {
    static const std::vector<std::string> names={"getVolumeTransmissionRay","applyIorToRoughness","getTransmissionSample","volumeAttenuation","getIBLVolumeRefraction"};return names;
}
inline const std::vector<std::string>& iteratorModes() {
    static const std::vector<std::string> names={"empty","one","exact","extra","nextThrow","doneThrow","valueThrow","returnGetThrow","returnThrow","returnPrimitive"};return names;
}
inline JSValue iterableHelper(const std::string& name,const std::string& mode) {
    StackScope scope;auto values=array(),events=array(),control=object();control.set("index",0);
    const auto f=node("float",{.5}),ior=node("float",{1.5}),n=node("vec3",{0.,0.,1.}),v=node("vec3",{.1,.2,1.}),color=node("vec3",{.2,.4,.8});
    if(name=="applyIorToRoughness")values=array({f,ior});
    if(name=="getVolumeTransmissionRay")values=array({n,v,f,ior,node("mat4")});
    if(name=="getTransmissionSample")values=array({node("vec2",{.25,.75}),f,ior});
    if(name=="volumeAttenuation")values=array({f,color,node("float",{2.})});
    if(name=="getIBLVolumeRefraction")values=array({n,v,f,color,color,node("float",{1.}),node("vec3"),node("mat4"),node("mat4"),node("mat4"),ior,f,color,node("float",{2.}),nullptr});
    auto iterator=object(),iterable=object();const auto length=mode=="empty"?0u:mode=="one"?1u:mode=="extra"?values.length()+1:values.length();
    auto next=JSValue::function(+[](const JSValue& receiver,const Args&,const Args& c)->JSValue {
        const auto i=c[0].get("index").toNumber();auto control=c[0];control.set("index",i+1);push(c[1],array({"next",i,receiver.sameValue(c[2])}));
        if(c[3].strictEquals("nextThrow"))throw JSThrownValue("next");
        auto result=object();JSPropertyDescriptor done;done.configurable=done.enumerable=true;done.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],array({"done",c[1]}));if(c[2].strictEquals("doneThrow"))throw JSThrownValue("done");return c[1].toNumber()>=c[3].toNumber();},{c[1],i,c[3],c[4]});result.defineProperty("done",done);
        JSPropertyDescriptor value;value.configurable=value.enumerable=true;value.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],array({"value",c[1]}));if(c[2].strictEquals("valueThrow"))throw JSThrownValue("value");return c[3].get(c[1]);},{c[1],i,c[3],c[5]});result.defineProperty("value",value);return result;
    },{control,events,iterator,mode,length,values});
    JSPropertyDescriptor nextDescriptor;nextDescriptor.configurable=true;nextDescriptor.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:next");return c[1];},{events,next});iterator.defineProperty("next",nextDescriptor);
    JSPropertyDescriptor close;close.configurable=true;close.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:return");if(c[1].strictEquals("returnGetThrow"))throw JSThrownValue("return:get");return JSValue::function(+[](const JSValue& receiver,const Args&,const Args& c)->JSValue{push(c[0],array({"return",receiver.sameValue(c[2])}));if(c[1].strictEquals("returnThrow"))throw JSThrownValue("return:call");return c[1].strictEquals("returnPrimitive")?JSValue(5):object();},{c[0],c[1],c[2]});},{events,mode,iterator});iterator.defineProperty("return",close);
    JSPropertyDescriptor start;start.configurable=true;start.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:iterator");return JSValue::function(+[](const JSValue& receiver,const Args&,const Args& c)->JSValue{push(c[0],array({"call:iterator",receiver.sameValue(c[1])}));return c[2];},{c[0],c[1],c[2]});},{events,iterable,iterator});iterable.defineProperty(JSValue::wellKnownSymbol("iterator"),start);
    auto b=object(),material=object();material.set("side",0);b.set("material",material);auto out=object();JSValue graphResult;
    const auto result=attempt([&]{graphResult=helperBody(name.c_str()).call({}, {iterable,b});return JSValue("success");});
    out.set("result",result);out.set("events",events);if(result.strictEquals("success")){auto graph=object();graph.set("result",graphResult);graph.set("stack",scope.stack.get("nodes"));out.set("graph",Graph()(graph));}return out;
}
inline JSValue all() {
    auto out=object();out.set("metadata",metadata());out.set("construction",construction());auto cases=object();
    for(const auto mask:{0u,1u,2u,4u,16u,48u,128u,255u})cases.set("start:"+std::to_string(mask),attempt([&]{return lighting(mask,"start");}));
    for(const auto mask:{0u,255u})for(const auto* method:{"direct","directRectArea","indirectDiffuse","indirectSpecular","ambientOcclusion","finish","indirect"})cases.set(std::string(method)+":"+std::to_string(mask),attempt([&]{return lighting(mask,method);}));
    for(unsigned i=0;i<4;++i)cases.set("multiscattering:"+std::to_string(i),attempt([&]{return multiscattering(i);}));
    for(const auto& name:helperNames()){cases.set("helper:"+name,attempt([&]{return helper(name);}));if(name=="volumeAttenuation"||name=="getTransmissionSample"||name=="getIBLVolumeRefraction")cases.set("helper:"+name+":alternate",attempt([&]{return helper(name,1);}));}
    for(const auto* key:{"","indirectDiffuse","indirectSpecular","ambientOcclusion"})cases.set(std::string("order:")+key,indirectOrder(key));cases.set("order:reenter",indirectOrder("",true));
    for(const auto* key:{"","get:data.lightDirection","get:data.lightColor","get:data.reflectedLight","get:self.sheen","get:self.iridescence","get:self.multiScatteringCompensation"})cases.set(std::string("observed:direct:")+key,observed("direct",key));
    for(const auto* key:{"","get:self.clearcoat","set:self.dfg","get:self.computeMultiscattering","get:builder.lightsNode","get:lights.getLightNodes","call:indirect"})cases.set(std::string("observed:start:")+key,observed("start",key));
    for(const auto& name:arrayHelperNames())for(const auto& mode:iteratorModes())cases.set("iterator:"+name+":"+mode,iterableHelper(name,mode));
    cases.set("builder:roughness",numericBuilder("applyIorToRoughness"));cases.set("builder:sheen",numericBuilder("IBLSheenBRDF"));out.set("cases",cases);return out;
}
} // namespace PhysicalLightingScenarios
