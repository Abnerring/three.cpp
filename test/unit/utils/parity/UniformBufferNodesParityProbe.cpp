#include "animation/AnimationUtils.h"
#include "nodes/accessors/BufferNode.h"
#include "nodes/accessors/UniformArrayNode.h"
#include "nodes/accessors/BuiltinNode.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"
#include "core/NativeArray.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <iterator>
#include <optional>
namespace {
using Args=std::vector<JSValue>;
bool relative=false;double baseline=0;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue array(const Args& values={}){auto out=JSValue::array();for(std::size_t i=0;i<values.size();++i)own(out,std::to_string(i),values[i]);return out;}
JSValue call(const JSValue& o,const char* k,const Args& a={}){return o.get(k).call(o,a);}
void push(const JSValue& o,const JSValue& v){call(o,"push",{v});}
Args values(const JSValue& o){Args out;for(unsigned i=0;i<o.length();++i)out.push_back(o.get(std::to_string(i)));return out;}
JSValue normal(const JSValue& v,Args* seen=nullptr){
 if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";if(v.isSymbol())return "#symbol";
 if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "#NaN";if(!std::isfinite(n))return n>0?"#Infinity":"#-Infinity";if(n==0&&std::signbit(n))return "#-0";}
 if(!v.isObject())return v;if(v.get("_tag").toBoolean())return "@"+v.get("_tag").toString();Args local;if(!seen)seen=&local;
 for(std::size_t i=0;i<seen->size();++i)if(v.sameValue((*seen)[i]))return array({"#ref",static_cast<double>(i)});const auto index=static_cast<double>(seen->size());seen->push_back(v);
 auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();for(const auto& k:v.keys()){const auto field=v.get(k);own(out,k,relative&&k=="id"&&v.get("isNode").strictEquals(true)?(field.toNumber()>baseline?array({"#local",field.toNumber()-baseline}):array({"#existing",index})):normal(field,seen));}return out;
}
template<class F>JSValue attempt(F body){try{return normal(body());}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception&){return "#Error";}}
void event(const JSValue& s,const Args& a){push(s.get("events"),array(a));}
void observe(const JSValue& s,JSValue o,const char* key,const std::string& label){auto cell=JSValue::object();cell.set("value",o.get(key));JSPropertyDescriptor d;d.configurable=d.enumerable=true;
 d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[2]});if(c[0].get("r").get("throwAt").sameValue(c[2]))throw JSTypeError("get");return c[1].get("value");},{s,cell,label});
 d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{"set",c[2],normal(arg(a,0))});if(c[0].get("r").get("throwAt").strictEquals("set."+c[2].toString()))throw JSTypeError("set");auto cell=c[1];cell.set("value",arg(a,0));return {};},{s,cell,label});o.defineProperty(key,d);
}
JSValue primitive(const JSValue& s,const JSValue& label,const JSValue& value){auto out=JSValue::object();out.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{"primitive",c[1],arg(a,0)});if(c[0].get("r").get("throwAt").strictEquals("primitive."+c[1].toString()))throw JSTypeError("primitive");return c[2];},{s,label,value}));return out;}
void tick(const JSValue& s,const std::string& key){event(s,{key});if(s.get("r").get("throwAt").strictEquals(key))throw JSTypeError(key);}
JSValue state(const JSValue& r){auto s=JSValue::object();s.set("r",r);s.set("events",array());return s;}
JSValue decode(const JSValue& v,const JSValue& s){
 if(!v.isObject())return v;if(v.type()==JSValue::Type::Array){auto out=array();for(const auto& item:values(v))push(out,decode(item,s));return out;}
 const auto tag=v.get("tag");if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("NaN"))return std::nan("");if(tag.strictEquals("Infinity"))return INFINITY;if(tag.strictEquals("-0"))return -0.0;if(tag.strictEquals("symbol"))return JSValue::symbol("fixture");
 if(tag.strictEquals("primitive"))return primitive(s,v.get("name"),v.get("value"));
 if(tag.strictEquals("type")){
  auto out=primitive(s,"type",v.get("text"));auto cell=JSValue::object();cell.set("index",0);
  JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{tick(c[0],"type.charAt.get");return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"type.charAt.call");event(c[0],{"charAt.argument",normal(arg(a,0))});auto cell=c[2];const auto i=cell.get("index").toNumber();cell.set("index",i+1);const auto seq=c[1].get("sequence");return seq.isUndefined()?JSValue("f"):seq.get(JSValue(i));},c);},{s,v,cell});out.defineProperty("charAt",d);return out;
 }
 auto out=JSValue::object();for(const auto& k:v.keys())own(out,k,decode(v.get(k),s));return out;
}
JSValue module(const JSValue& name){return name.strictEquals("Buffer")?nativeBufferNodeModule():name.strictEquals("Builtin")?nativeBuiltinNodeModule():nativeUniformArrayNodeModule();}
JSValue describe(const JSValue& o){auto out=array();for(const auto& k:o.ownPropertyNames()){const auto d=o.getOwnPropertyDescriptor(k).value();const auto v=d.value.value_or(JSValue{});push(out,array({k,relative&&k=="id"&&o.get("isNode").strictEquals(true)?array({"#local",v.toNumber()-baseline}):normal(v),v.isCallable()?v.get("name"):JSValue(nullptr),v.isCallable()?v.get("length"):JSValue(nullptr),v.isConstructible(),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false),d.getter&&d.getter->isCallable()?d.getter->get("name"):JSValue(nullptr)}));}return out;}
JSValue metadata(const JSValue& r){const auto m=module(r.get("class")),ctor=m.get("default");auto n=r.get("class").strictEquals("UniformArray")?ctor.construct({array({1})}):ctor.construct({"TEST","float",3});auto out=JSValue::object();own(out,"module",describe(m));own(out,"statics",describe(ctor));own(out,"prototype",describe(ctor.get("prototype")));own(out,"instance",describe(n));own(out,"type",n.get("type"));if(r.get("class").strictEquals("UniformArray")){auto e=call(n,"element",{0});own(out,"elementStatics",describe(e.get("constructor")));own(out,"elementPrototype",describe(e.getPrototype()));own(out,"element",normal(e));}return out;}
JSValue factory(const JSValue& r){auto s=state(r),out=JSValue::object();const auto old=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{Args row{"console"};for(const auto& x:a)row.push_back(normal(x));event(c[0],row);return {};},{s}));try{
 Args a;for(const auto& x:values(r.get("args")))a.push_back(decode(x,s));const auto m=module(r.get("class")),f=m.get(r.get("name"));own(out,"result",attempt([&]{return r.get("construct").toBoolean()?f.construct(a):f.call(m,a);}));own(out,"allocationDelta",nativeNodeConstructor().construct().get("id").toNumber()-baseline);own(out,"events",s.get("events"));ThreeUtils::setConsoleFunction(old);return out;
 }catch(...){ThreeUtils::setConsoleFunction(old);throw;}}
JSValue builder(const JSValue& s){auto b=JSValue::object(),renderer=JSValue::object(),backend=JSValue::object();b.set("_tag","builder");backend.set("isWebGLBackend",decode(s.get("r").get("webgl"),s));renderer.set("backend",backend);b.set("renderer",renderer);auto properties=JSValue::object();
 b.set("getTypeLength",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"builder.getTypeLength.call");event(c[0],{"typeLength",normal(arg(a,0))});if(c[0].get("r").getOwnPropertyDescriptor("paddedLength"))return decode(c[0].get("r").get("paddedLength"),c[0]);return arg(a,0).strictEquals("mat4")?16:4;},{s}));
 b.set("getNodeProperties",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{tick(c[0],"builder.getNodeProperties.call");return c[1];},{s,properties}));
 b.set("format",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"builder.format.call");event(c[0],{"format",normal(array(a))});return "FORMATTED";},{s}));
 b.set("isVector",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{"isVector",normal(arg(a,0))});return c[0].get("r").get("vectorIndex");},{s}));
 b.set("isInteger",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{"isInteger",normal(arg(a,0))});return true;},{s}));
 if(s.get("r").get("observed").toBoolean()){observe(s,b,"renderer","builder.renderer");observe(s,renderer,"backend","renderer.backend");observe(s,backend,"isWebGLBackend","backend.isWebGLBackend");for(const auto* k:{"getTypeLength","getNodeProperties","format"})observe(s,b,k,std::string("builder.")+k);}
 return b;
}
void watchedArray(const JSValue& s,JSValue a){
 if(!a.isObject())return;const auto length=a.get("length").toNumber();
 // Array length itself cannot be redefined; wrap it in a genuine array-like object.
 for(double i=0;i<length;++i){auto item=a.get(JSValue(i));if(item.isObject()&&item.type()!=JSValue::Type::Array){for(const auto& k:item.keys())observe(s,item,k.c_str(),"item"+std::to_string(static_cast<int>(i))+"."+k);}}
}
JSValue arrayLike(const JSValue& s,const JSValue& original){auto a=JSValue::object();const auto length=original.get("length");a.set("length",length);for(double i=0;i<length.toNumber();++i)a.set(JSValue(i),original.get(JSValue(i)));observe(s,a,"length","array.length");for(double i=0;i<length.toNumber();++i)observe(s,a,std::to_string(static_cast<int>(i)).c_str(),"array."+std::to_string(static_cast<int>(i)));return a;}
JSValue typedState(const JSValue& n){auto out=JSValue::object();for(const auto* k:{"value","bufferCount","bufferType","updateType"})own(out,k,attempt([&]{return n.get(k);}));own(out,"valueConstructor",attempt([&]{const auto v=n.get("value");return v.isObject()?v.get("constructor").get("name"):JSValue(nullptr);}));return out;}
JSValue protocol(const JSValue& r){auto s=state(r),b=builder(s),out=JSValue::object();auto n=nativeUniformArrayNodeConstructor().construct({array({1}),"float"});n.set("_tag","self");s.set("node",n);
 const auto input=r.getOwnPropertyDescriptor("array")?decode(r.get("array"),s):array({1,2});if(r.get("observed").toBoolean())watchedArray(s,input);n.set("array",r.get("observed").toBoolean()&&input.isObject()&&input.get("length").type()==JSValue::Type::Number?arrayLike(s,input):input);n.set("elementType",decode(r.get("type"),s));
 const auto method=r.get("method");if(method.strictEquals("updateBuffer")||method.strictEquals("update")){
  auto v=r.getOwnPropertyDescriptor("target")?decode(r.get("target"),s):JSValue::array(40);if(v.isObject())for(unsigned i=0;i<40;++i)if(!v.hasOwn(std::to_string(i)))v.set(std::to_string(i),99);n.set("value",v);
 }
 if(r.get("observed").toBoolean())for(const auto* k:{"array","value","elementType","bufferCount","bufferType","updateBuffer","getPaddedType"})observe(s,n,k,std::string("self.")+k);
 own(out,"result",attempt([&]{return call(n,method.toString().c_str(),{b});}));
 // Getter diagnostics belong to the method trace, before observation of its final state.
 own(out,"events",normal(s.get("events")));own(out,"state",typedState(n));own(out,"allocationDelta",nativeNodeConstructor().construct().get("id").toNumber()-baseline);return out;
}
JSValue bufferProtocol(const JSValue& r){auto s=state(r),n=nativeBufferNodeConstructor().construct({nullptr,"vec3",2}),out=JSValue::object();n.set("_tag","self");auto ranges=array({17});n.set("updateRanges",ranges);n.set("getNodeType",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"getNodeType.call");return arg(a,0);},{s}));
 if(r.get("customPush").toBoolean())ranges.set("push",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{tick(c[0],"push.call");event(c[0],{"push",self.sameValue(c[1]),normal(arg(a,0))});return "IGNORED";},{s,ranges}));
 if(r.get("observed").toBoolean()){observe(s,n,"updateRanges","self.updateRanges");observe(s,n,"getNodeType","self.getNodeType");observe(s,ranges,"push","ranges.push");}
 own(out,"result",attempt([&]{return call(n,r.get("method").toString().c_str(),{"START","COUNT"});}));own(out,"events",normal(s.get("events")));own(out,"ranges",attempt([&]{return ranges;}));return out;
}
JSValue callbackProtocol(const JSValue& r){auto s=state(r),b=builder(s),n=nativeUniformArrayNodeConstructor().construct({array({1,2}),"float"}),out=JSValue::object();n.set("_tag","self");s.set("node",n);call(n,"setup",{b});s.set("events",array());
 auto callback=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{auto s=c[0],n=s.get("node");tick(s,"callback.call");event(s,{"callback",self.sameValue(n),normal(arg(a,0)),arg(a,1).sameValue(n)});if(s.get("r").get("gc").toBoolean())s.collectGarbage();n.get("array").set("0",7);if(s.get("r").get("reenter").toBoolean()&&!s.get("entered").toBoolean()){s.set("entered",true);call(n,"update",{"INNER"});}return s.get("r").get("returnValue");},{s},"userCallback",2);
 if(r.get("customBind").toBoolean())callback.set("bind",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{tick(c[0],"customBind.call");return c[1];},{s,callback}));
 if(r.get("observed").toBoolean()){observe(s,callback,"bind","callback.bind");for(const auto* k:{"updateType","update","updateBuffer"})observe(s,n,k,std::string("self.")+k);}
 own(out,"registration",attempt([&]{return call(n,r.get("registration").toString().c_str(),{callback,"object"});}));own(out,"first",attempt([&]{return call(n,"update",{"FRAME"});}));own(out,"second",attempt([&]{return call(n,"update",{"NEXT"});}));own(out,"events",normal(s.get("events")));own(out,"state",typedState(n));return out;
}
JSValue elementProtocol(const JSValue& r){auto s=state(r),b=builder(s),n=nativeUniformArrayNodeConstructor().construct({array({1}),"vec3"}),out=JSValue::object();n.set("_tag","uniform");auto index=nativeNodeConstructor().construct({"int"});index.set("_tag","index");auto e=call(n,"element",{index});e.set("_tag","element");
 n.set("build",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{tick(c[0],"uniform.build.call");return "UNIFORM";},{s}));index.set("getNodeType",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{tick(c[0],"index.getNodeType.call");return "int";},{s}));index.set("build",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"index.build.call");event(c[0],{"index.output",normal(arg(a,1))});return 3;},{s}));e.set("getNodeType",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{tick(c[0],"element.getNodeType.call");return "vec3";},{s}));
 if(r.get("observed").toBoolean()){for(const auto* k:{"node","indexNode","getNodeType"})observe(s,e,k,std::string("element.")+k);for(const auto* k:{"build","getPaddedType"})observe(s,n,k,std::string("uniform.")+k);for(const auto* k:{"build","getNodeType"})observe(s,index,k,std::string("index.")+k);}
 own(out,"result",attempt([&]{return call(e,"generate",{b});}));own(out,"events",s.get("events"));return out;
}
JSValue integration(const JSValue& r){auto renderer=JSValue::object(),backend=JSValue::object();renderer.set("backend",backend);renderer.set("coordinateSystem",2001);if(r.get("configured").toBoolean()){auto debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);}auto b=nativeNodeBuilderConstructor().construct({nullptr,renderer,nullptr});b.set("shaderStage",r.get("stage"));auto n=r.get("class").strictEquals("Builtin")?nativeBuiltinNodeBinding("builtin").call({}, {"gl_ClipDistance"}):nativeUniformArrayNodeBinding("uniformArray").call({}, {array({1,2}),"float"});if(r.get("element").toBoolean())n=call(n,"element",{0});auto results=array();for(const auto* stage:{"setup","analyze","generate"}){b.set("buildStage",stage);push(results,attempt([&]{const auto v=call(n,"build",{b,"float"});return v.isObject()&&v.get("isNode").strictEquals(true)?JSValue("#"+v.get("type").toString()):v;}));}auto out=JSValue::object();own(out,"results",results);own(out,"flow",normal(b.get("flow")));return out;}
JsonValue freshGC(){JsonValue::Array checks;for(int pass=0;pass<3;++pass){if(JSValue::hasActiveHeap())throw std::logic_error("previous realm retained");{
 const auto m=nativeUniformArrayNodeModule(),f=nativeUniformArrayNodeBinding("uniformArray"),builtin=nativeBuiltinNodeBinding("builtin");auto a=f.call({}, {array({1,2}),"float"});a.collectGarbage();if(!f.sameValue(nativeUniformArrayNodeBinding("uniformArray"))||!builtin.sameValue(nativeBuiltinNodeBinding("builtin")))throw std::logic_error("lexical binding lost");
 auto receiver=JSValue::object(),backend=JSValue::object();receiver.set("backend",backend);auto b=JSValue::object();b.set("renderer",receiver);b.set("getTypeLength",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return 4;}));b.set("getNodeProperties",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return JSValue::object();}));call(a,"setup",{b});a.collectGarbage();if(!a.get("value").get("0").strictEquals(1))throw std::logic_error("buffer lost");
 std::optional<UniformArrayNode> owner(std::in_place,array({3}),"float");UniformArrayNode copy(*owner);owner.reset();owner.emplace(array({9}),"int");copy.elementType="vec3";if(!copy.object().get("elementType").strictEquals("vec3")||!owner->object().get("elementType").strictEquals("int"))throw std::logic_error("wrapper owner");
 }if(JSValue::hasActiveHeap())throw std::logic_error("realm leak");checks.emplace_back(pass);}
 return JsonValue(checks);}
}
int main(int argc,char** argv){try{if(argc>1&&std::string(argv[1])=="--fresh-gc"){std::cout<<freshGC().dump();return 0;}const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input),buffer=nativeBufferNodeModule(),uniform=nativeUniformArrayNodeModule(),builtin=nativeBuiltinNodeModule(),node=nativeNodeConstructor();auto out=array();for(unsigned i=0;i<requests.length();++i){const auto r=requests.get(std::to_string(i)),kind=r.get("kind");relative=r.get("relativeNodeIds").toBoolean();baseline=node.construct().get("id").toNumber();push(out,kind.strictEquals("metadata")?metadata(r):kind.strictEquals("factory")?factory(r):kind.strictEquals("buffer")?bufferProtocol(r):kind.strictEquals("callback")?callbackProtocol(r):kind.strictEquals("element")?elementProtocol(r):kind.strictEquals("integration")?integration(r):protocol(r));}std::cout<<out.toJSON()->dump();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
