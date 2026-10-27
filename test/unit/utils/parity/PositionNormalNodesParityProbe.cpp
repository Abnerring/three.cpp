#include "animation/AnimationUtils.h"
#include "nodes/accessors/Position.h"
#include "nodes/accessors/Normal.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/StackNode.h"
#include "nodes/tsl/TSLBase.h"
#include "core/JSIterator.h"
#include "core/NativeArray.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue call(const JSValue& o,const char* k,const Args& a={}){return o.get(k).call(o,a);}
JSValue array(const Args& a={}){auto o=JSValue::array();for(const auto& v:a)call(o,"push",{v});return o;}
class Graph {
    double baseline_;bool relative_;Args seen_;JSValue objects_=array();
    JSValue encode(const JSValue& v){
        if(v.isUndefined())return array({"undefined"});if(v.isCallable())return array({"function"});if(v.isSymbol())return array({"symbol"});
        if(v.type()==JSValue::Type::Number){if(std::isnan(v.number()))return array({"number","NaN"});if(!std::isfinite(v.number()))return array({"number",v.number()>0?"Infinity":"-Infinity"});if(v.number()==0&&std::signbit(v.number()))return array({"number","-0"});}if(!v.isObject())return v;
        for(std::size_t i=0;i<seen_.size();++i)if(v.sameValue(seen_[i]))return array({"ref",i});const auto index=seen_.size();seen_.push_back(v);auto record=JSValue::object(),props=array();record.set("kind",v.type()==JSValue::Type::Array?"array":"object");record.set("properties",props);call(objects_,"push",{record});if(v.type()==JSValue::Type::Array)record.set("length",v.length());const bool node=v.get("isNode").strictEquals(true);if(node)record.set("nodeType",v.get("type"));
        for(const auto& k:v.keys()){const auto value=v.get(k);call(props,"push",{array({k,relative_&&node&&k=="id"&&value.toNumber()>baseline_?array({"integrationId",value.toNumber()-baseline_}):encode(value)})});}return array({"ref",index});
    }
  public:Graph(double baseline=0,bool relative=false):baseline_(baseline),relative_(relative){}JSValue operator()(const JSValue& v){auto o=JSValue::object();o.set("root",encode(v));o.set("objects",objects_);return o;}
};
JSValue error(std::exception_ptr p){try{std::rethrow_exception(p);}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception& e){return std::string("#NativeError:")+e.what();}}
JSValue decode(const JSValue& v){if(!v.isObject())return v;if(v.get("tag").strictEquals("undefined"))return {};if(v.get("tag").strictEquals("symbol"))return JSValue::symbol("fixture");if(v.get("tag").strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();return v;}
JSValue state(const JSValue& r){auto s=JSValue::object();s.set("request",r);s.set("events",array());s.set("active",true);return s;}
void event(const JSValue& s,const Args& a){if(s.get("active").toBoolean())call(s.get("events"),"push",{array(a)});}
void checkpoint(const JSValue& s,const std::string& label){event(s,{label});if(s.get("active").toBoolean()&&s.get("request").get("throwAt").strictEquals(label))throw JSTypeError(label);}
void observe(const JSValue& s,JSValue o,const char* key,const std::string& label){auto slot=JSValue::object();slot.set("value",o.get(key));JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{checkpoint(c[0],"get:"+c[2].string());return c[1].get("value");},{s,slot,label});d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{checkpoint(c[0],"set:"+c[2].string());auto v=c[1];v.set("value",arg(a,0));return {};},{s,slot,label});o.defineProperty(key,d);}
JSValue binding(const std::string& name){const auto p=nativePositionModule();return p.has(name)?nativePositionBinding(name):nativeNormalBinding(name);}
JSValue callback(JSValue n){for(int i=0;i<16;++i){const auto shader=n.get("shaderNode");if(!shader.isUndefined())return shader.get("jsFunc");n=n.get("node");}throw std::logic_error("Export has no shader callback");}
JSValue metadata(const JSValue& r){auto out=JSValue::object(),functions=array();const auto m=r.get("normal").toBoolean()?nativeNormalModule():nativePositionModule();out.set("exports",m);for(const auto& k:m.keys()){const auto v=m.get(k);if(v.isCallable()){auto f=JSValue::object();f.set("name",k);f.set("functionName",v.get("name"));f.set("length",v.get("length"));f.set("constructible",v.isConstructible());f.set("shaderNode",v.get("shaderNode"));call(functions,"push",{f});}else if(v.get("isVarNode").toBoolean()&&!JSValue(k).strictEquals("normalFlat")&&!JSValue(k).strictEquals("normalWorld")){auto f=JSValue::object();f.set("name",k);f.set("callbackLength",callback(v).get("length"));call(functions,"push",{f});}}out.set("functions",functions);return Graph()(out);}
JSValue vec(const Args& a={1,2,3}){return nativeTSLBaseBinding("vec3").call({},a);}
JSValue mat(){return nativeTSLBaseBinding("mat3").call();}
JSValue method(const JSValue& s,const std::string& name,const JSValue& receiver,const JSValue& value){return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{checkpoint(c[0],c[1].string());event(c[0],{"this:"+c[1].string(),self.sameValue(c[2])});if(c[1].strictEquals("hasAttribute"))event(c[0],{"attribute",arg(a,0)});if(c[1].strictEquals("getSubBuildProperty"))event(c[0],{"property",arg(a,0)});return c[3];},{s,name,receiver,value},name.c_str(),0);}
JSValue fixture(const JSValue& s){
 const auto r=s.get("request");auto b=JSValue::object(),ctx=JSValue::object(),camera=JSValue::object(),geometry=JSValue::object(),material=JSValue::object();
 b.set("shaderStage",r.has("stage")?decode(r.get("stage")):JSValue("fragment"));b.set("subBuildFn",r.has("subBuildFn")?decode(r.get("subBuildFn")):JSValue("OTHER"));
 camera.set("isOrthographicCamera",decode(r.get("orthographic")));material.set("vertexNode",decode(r.get("vertexNode")));material.set("side",r.has("side")?r.get("side"):JSValue(0));
 ctx.set("clipSpace",nativeTSLBaseBinding("vec4").call({}, {1,2,3,4}));ctx.set("modelNormalViewMatrix",r.get("matrixContext").strictEquals("node")?mat():decode(r.get("matrixContext")));
 for(const auto* name:{"setupPositionView","setupNormal","setupClearcoatNormal"})ctx.set(name,method(s,name,ctx,vec()));
 geometry.set("hasAttribute",method(s,"hasAttribute",geometry,r.has("hasAttribute")?decode(r.get("hasAttribute")):JSValue(true)));
 b.set("isFlatShading",method(s,"isFlatShading",b,decode(r.get("flat"))));b.set("getSubBuildProperty",method(s,"getSubBuildProperty",b,r.has("property")?decode(r.get("property")):JSValue("v_positionWorld_sub")));
 b.set("camera",camera);b.set("material",material);b.set("geometry",geometry);b.set("context",ctx);
 if(r.get("observed").toBoolean()){
  for(const auto* k:{"shaderStage","subBuildFn","camera","material","geometry","context","isFlatShading","getSubBuildProperty"})observe(s,b,k,std::string("builder.")+k);
  for(const auto* k:{"clipSpace","modelNormalViewMatrix","setupPositionView","setupNormal","setupClearcoatNormal"})observe(s,ctx,k,std::string("context.")+k);
  observe(s,camera,"isOrthographicCamera","camera.isOrthographicCamera");observe(s,material,"vertexNode","material.vertexNode");observe(s,geometry,"hasAttribute","geometry.hasAttribute");
 }
 if(r.has("subBuildSequence")){auto index=JSValue::object();index.set("i",0);JSPropertyDescriptor d;d.enumerable=d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{checkpoint(c[0],"get:builder.subBuildFn");auto index=c[1];const auto i=index.get("i");index.set("i",i.toNumber()+1);return decode(c[0].get("request").get("subBuildSequence").get(i));},{s,index});b.defineProperty("subBuildFn",d);}
 return b;
}
JSValue iterable(const JSValue& s,const JSValue& values){auto input=JSValue::object();input.set(JSValue::wellKnownSymbol("iterator"),JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
 checkpoint(c[0],"iterator");auto it=JSValue::object(),index=JSValue::object();index.set("i",0);
 it.set("next",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{checkpoint(c[0],"next");auto index=c[2];const auto i=index.get("i").toNumber();index.set("i",i+1);auto result=JSValue::object();result.set("value",c[1].get(JSValue(i)));result.set("done",i>=c[1].length());observe(c[0],result,"value","iterator.value");observe(c[0],result,"done","iterator.done");return result;},{c[0],c[1],index}));
 it.set("return",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{checkpoint(c[0],"return");return c[0].get("request").get("returnPrimitive").toBoolean()?JSValue(7):JSValue::object();},{c[0]}));observe(c[0],it,"next","iterator.next");observe(c[0],it,"return","iterator.return");return it;
 },{s,values}));return input;}
JSValue console(const JSValue& s){return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],Args{"console",array(a)});checkpoint(c[0],"console");return {};},{s});}
JSValue runCallback(const JSValue& r){
 auto s=state(r),out=JSValue::object();const auto before=nativeNodeConstructor().construct(),b=fixture(s),previous=nativeGetCurrentStack(),stack=nativeStackNodeConstructor().construct();nativeSetCurrentStack(stack);const auto oldConsole=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(console(s));
 try{const auto name=r.get("name"),fn=callback(binding(name.string()));JSValue result;
  if(name.strictEquals("transformNormal")||name.strictEquals("transformNormalToView")){
   auto values=array();const auto mode=r.get("input");if(!mode.strictEquals("empty")){call(values,"push",{vec()});if(!mode.strictEquals("single"))call(values,"push",{mode.strictEquals("nullMatrix")?JSValue(nullptr):mode.strictEquals("undefinedMatrix")?JSValue{}:mat()});if(mode.strictEquals("extra"))call(values,"push",{vec()});}
   const auto input=mode.strictEquals("null")?JSValue(nullptr):mode.strictEquals("undefined")?JSValue{}:mode.strictEquals("number")?JSValue(1):r.get("iterator").toBoolean()?iterable(s,values):values;
   result=fn.call({}, {input,r.get("nullBuilder").toBoolean()?JSValue(nullptr):b});
  }else result=fn.call({}, {r.get("nullBuilder").toBoolean()?JSValue(nullptr):b});
  out.set("result",result);
  if(r.get("repeat").toBoolean())out.set("again",fn.call({}, {b}));
 }catch(...){out.set("error",error(std::current_exception()));}
 s.set("active",false);nativeSetCurrentStack(previous);ThreeUtils::setConsoleFunction(oldConsole);out.set("events",s.get("events"));out.set("stackNodes",stack.get("nodes"));out.set("allocations",nativeNodeConstructor().construct().get("id").toNumber()-before.get("id").toNumber()-1);return Graph()(out);
}
JSValue factory(const JSValue& r){auto out=JSValue::object();const auto before=nativeNodeConstructor().construct(),n=vec(),matrix=mat();try{const auto name=r.get("name").string();auto params=Args{n};if(r.get("matrix").toBoolean())params.push_back(matrix);out.set("result",r.get("chaining").toBoolean()?call(n,"transformNormal",r.get("matrix").toBoolean()?Args{matrix}:Args{}):binding(name).call({},params));}catch(...){out.set("error",error(std::current_exception()));}out.set("allocations",nativeNodeConstructor().construct().get("id").toNumber()-before.get("id").toNumber()-1);return Graph()(out);}
double integrationBaseline=0;bool integrationStarted=false;
JSValue integration(const JSValue& r){
 const auto ctor=nativeNodeBuilderConstructor();const bool relative=r.get("relativeIntegrationNodeIds").toBoolean();if(relative&&!integrationStarted){integrationBaseline=nativeNodeConstructor().construct().get("id").toNumber();integrationStarted=true;}
 auto s=state(r),out=JSValue::object(),results=array(),f=fixture(s),attr=JSValue::object(),geometry=JSValue::object(),renderer=JSValue::object(),debug=JSValue::object(),diagnostics=JSValue::object(),object=JSValue::object();
 attr.set("array",NativeArray_detail::constructor(6).construct({array({1,2,3,4,5,6})}));attr.set("itemSize",3);attr.set("normalized",false);geometry.set("getAttribute",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0].get("missingAttribute").toBoolean()?JSValue{}:c[1];},{r,attr}));geometry.set("hasAttribute",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return !c[0].get("missingAttribute").toBoolean();},{r}));
 diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);renderer.set("coordinateSystem",2000);object.set("geometry",geometry);object.set("material",f.get("material"));auto b=ctor.construct({object,renderer,nullptr});b.set("shaderStage",r.get("stage"));b.set("camera",f.get("camera"));b.set("subBuildFn",r.get("subBuildFn"));auto ctx=b.get("context"),provided=f.get("context");for(const auto& k:provided.keys())ctx.set(k,provided.get(k));b.set("isFlatShading",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return decode(c[0].get("flat"));},{r}));
 const auto old=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(console(s));try{const auto n=binding(r.get("name").string());for(const auto* stage:{"setup","analyze","generate"}){b.set("buildStage",stage);call(results,"push",{call(n,"build",{b,r.get("output")})});}out.set("results",results);out.set("flow",b.get("flow"));out.set("uniforms",b.get("uniforms"));out.set("attributes",b.get("attributes"));out.set("varyings",b.get("varyings"));}catch(...){out.set("error",error(std::current_exception()));}s.set("active",false);ThreeUtils::setConsoleFunction(old);out.set("events",s.get("events"));return Graph(integrationBaseline,relative)(out);
}
JSValue fresh(const std::string& name){auto out=JSValue::object();out.set("exports",name=="Position"?nativePositionModule():nativeNormalModule());out.set("nextNode",nativeNodeConstructor().construct());out.set("transformNormalChain",vec().get("transformNormal").isCallable());return Graph()(out);}
void lifetime(){for(const auto* name:{"Position","Normal"}){JSWeakValue weak;{const auto value=std::string_view(name)=="Position"?nativePositionBinding("positionView"):nativeNormalBinding("normalView");weak=JSWeakValue(value);value.collectGarbage();if(!weak.lock()->isObject())throw std::logic_error("binding lost");auto m=std::string_view(name)=="Position"?nativePositionModule():nativeNormalModule();m.set(std::string_view(name)=="Position"?"positionView":"normalView",nullptr);if(!(std::string_view(name)=="Position"?nativePositionBinding("positionView"):nativeNormalBinding("normalView")).sameValue(value))throw std::logic_error("facade leaked into lexical");}JSValue::nativeSafepoint();if(!weak.expired())throw std::logic_error("module heap retained");}std::cout<<"Position/Normal empty-realm bindings released: 2\n";}
}
int main(int argc,char** argv){try{if(argc==2&&std::string_view(argv[1])=="--lifetime"){lifetime();return 0;}if(argc==3&&std::string_view(argv[1])=="--fresh"){std::cout<<fresh(argv[2]).toJSON()->dump();return 0;}const std::string input((std::istreambuf_iterator<char>(std::cin)),{});const auto requests=Animation_detail::parseJSON(input),position=nativePositionModule(),normal=nativeNormalModule();auto out=array();for(unsigned i=0;i<requests.length();++i){const auto r=requests.get(std::to_string(i)),kind=r.get("kind");call(out,"push",{kind.strictEquals("metadata")?metadata(r):kind.strictEquals("callback")?runCallback(r):kind.strictEquals("factory")?factory(r):integration(r)});}std::cout<<out.toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
