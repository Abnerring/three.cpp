#include "nodes/accessors/MaterialNode.h"
#include "core/JSGlobal.h"
#include "math/Vector2.h"
#include "core/JSString.h"
#include "core/JSStringEncoding.h"
#include "core/JSIterator.h"
#include "core/JSMap.h"
#include "core/JSPromise.h"
#include "nodes/core/Node.h"
#include "nodes/core/UniformNode.h"
#include "nodes/accessors/Normal.h"
#include "nodes/display/NormalMapNode.h"
#include "nodes/display/BumpMapNode.h"
#include "constants.h"
#include "animation/AnimationUtils.h"
#include "nodes/accessors/ReferenceNode.h"
#include "nodes/accessors/MaterialReferenceNode.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/accessors/CubeTextureNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/UniformGroupNode.h"
#include "textures/Texture.h"
#include "textures/CubeTexture.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <iterator>
namespace {
using Args=std::vector<JSValue>;
bool relative=false;double baseline=0;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
JSValue call(const JSValue& self,const std::string& key,const Args& args={}){const auto f=self.get(key);return f.call(self,args);}
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue array(const Args& a={}){auto v=JSValue::array();for(std::size_t i=0;i<a.size();++i)own(v,std::to_string(i),a[i]);return v;}
void push(const JSValue& a,const JSValue& v){call(a,"push",{v});}
Args values(const JSValue& a){Args result;for(unsigned i=0;i<a.length();++i)result.push_back(a.get(std::to_string(i)));return result;}
JSValue normal(const JSValue& v,Args* seen=nullptr){
 if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";if(v.isSymbol())return "#symbol";
 if(v.type()==JSValue::Type::Number){const auto x=v.toNumber();if(std::isnan(x))return "#NaN";if(!std::isfinite(x))return x>0?"#Infinity":"#-Infinity";if(x==0&&std::signbit(x))return "#-0";}
 if(!v.isObject())return v;if(v.get("_tag").toBoolean())return "@"+v.get("_tag").toString();Args local;if(!seen)seen=&local;for(std::size_t i=0;i<seen->size();++i)if(v.sameValue((*seen)[i]))return array({"#ref",double(i)});const auto index=double(seen->size());seen->push_back(v);
 auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();for(const auto& k:v.keys()){const auto x=v.get(k);own(out,k,relative&&k=="id"&&v.get("isNode").strictEquals(true)?(x.toNumber()>baseline?array({"#local",x.toNumber()-baseline}):array({"#existing",index})):normal(x,seen));}return out;
}
template<class F>JSValue attempt(F f){try{return normal(f());}catch(const JSThrownValue& e){auto o=JSValue::object();own(o,"error",e.value.get("name"));return o;}catch(const JSReferenceError&){auto o=JSValue::object();own(o,"error","ReferenceError");return o;}catch(const JSTypeError&){auto o=JSValue::object();own(o,"error","TypeError");return o;}catch(const JSRangeError&){auto o=JSValue::object();own(o,"error","RangeError");return o;}catch(const std::exception&){auto o=JSValue::object();own(o,"error","Error");return o;}}
JSValue ctor(const JSValue& r){return r.get("material").toBoolean()?nativeMaterialReferenceNodeConstructor():nativeReferenceNodeConstructor();}
JSValue state(const JSValue& r){auto s=JSValue::object();s.set("r",r);s.set("events",array());s.set("active",true);return s;}
void tick(JSValue s,const std::string& label,const Args& args={}){if(!s.get("active").toBoolean())return;Args row{label};for(const auto& v:args)row.push_back(normal(v));push(s.get("events"),array(row));if(s.get("r").get("throwAt").strictEquals(label))throw JSTypeError(label);}
void observe(JSValue s,JSValue o,const std::string& key,const std::string& label){const auto active=s.get("active");s.set("active",false);auto cell=JSValue::object();cell.set("value",o.get(key));s.set("active",active);JSPropertyDescriptor d;d.configurable=d.enumerable=true;
 d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{auto s=c[0];const auto label=c[2].toString();tick(s,"get "+label);if(s.get("active").toBoolean()&&s.get("r").get("reenter").strictEquals("get "+label)&&!s.get("entered").toBoolean()){s.set("entered",true);tick(s,"reenter",{call(s.get("node"),"isCacheable")});}return c[1].get("value");},{s,cell,label});
 d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"set "+c[2].toString(),{arg(a,0)});auto cell=c[1];cell.set("value",arg(a,0));return {};},{s,cell,label});o.defineProperty(key,d);
}
JSValue decode(const JSValue& v,const JSValue& s){
 if(!v.isObject())return v;if(v.type()==JSValue::Type::Array){auto o=array();for(const auto& x:values(v))push(o,decode(x,s));return o;}const auto tag=v.get("tag");
 if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("NaN"))return std::nan("");if(tag.strictEquals("-0"))return -0.0;if(tag.strictEquals("symbol"))return JSValue::symbol("fixture");
 if(tag.strictEquals("node")){auto node=nativeTSLBaseBinding("float").call({}, {.5});node.set("_tag","input");return node;}
 if(tag.strictEquals("texture")||tag.strictEquals("cube")){auto t=tag.strictEquals("cube")?nativeCubeTextureConstructor().construct():nativeTextureConstructor().construct();t.set("uuid","TEX");t.set("_tag",tag);return t;}
 if(tag.strictEquals("primitive")){auto o=JSValue::object();o.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"coerce",{arg(a,0)});return c[1];},{s,v.get("value")}));return o;}
 auto o=JSValue::object();for(const auto& k:v.keys())own(o,k,decode(v.get(k),s));return o;
}
JSValue describe(const JSValue& o){auto result=array();for(const auto& k:o.ownPropertyNames()){const auto d=*o.getOwnPropertyDescriptor(k);const auto v=d.value.value_or(JSValue{});push(result,array({k,normal(v),v.isCallable()?v.get("name"):JSValue(nullptr),v.isCallable()?v.get("length"):JSValue(nullptr),v.isCallable()&&v.isConstructible(),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false),d.getter&&!d.getter->isUndefined()?d.getter->get("name"):JSValue(nullptr),d.setter&&!d.setter->isUndefined()?d.setter->get("name"):JSValue(nullptr)}));}return result;}
template<class F>JSValue capture(JSValue s,F f){const auto prior=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"console",a);return {};},{s}));try{auto o=f();ThreeUtils::setConsoleFunction(prior);return o;}catch(...){ThreeUtils::setConsoleFunction(prior);throw;}}
JSValue referenceCallback,renderCallback;
JSValue mapImport(const std::string& mode){
 nativeNodeConstructor();nativeReferenceNodeModule();nativeMaterialReferenceNodeModule();nativeNormalModule();nativeTSLBase();nativeUniformNodeConstructor();nativeUniformFactory();nativeNormalMapNodeModule();nativeBumpMapNodeModule();nativeVector2Constructor();nativeConstantsModule();
 auto globals=nativeGlobalObject(),events=array(),cell=JSValue::object();const auto prior=globals.getOwnPropertyDescriptor("Map");const auto intrinsic=nativeMapConstructor();auto handler=JSValue::object();
 handler.set("construct",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{
  push(c[0],array({"construct Map",double(arg(a,1).length()),arg(a,2).sameValue(c[1].get("replacement"))}));if(c[2].strictEquals("constructThrow"))throw JSTypeError("construct Map");const auto cache=arg(a,0).construct(values(arg(a,1)),arg(a,0));
  for(const auto* key:{"get","set"}){const auto original=cache.get(key);auto object=cache;object.set(key,JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],array({"cache."+c[1].toString(),arg(a,0)}));return c[2].call(self,a);},{c[0],key,original}));}return cache;
 },{events,cell,mode}));const auto replacement=JSValue::proxy(intrinsic,handler);cell.set("replacement",replacement);
 if(mode=="missing")globals.erase("Map");else{JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],array({"get global.Map"}));if(c[1].strictEquals("getterThrow"))throw JSTypeError("global.Map");return c[1].strictEquals("nonconstructor")?JSValue::object():c[2];},{events,mode,replacement});globals.defineProperty("Map",d);}
 auto out=JSValue::object();own(out,"result",attempt([&]()->JSValue{nativeMaterialNodeModule();auto n=nativeMaterialNodeConstructor().construct({"fresh-map-import"});const auto a=call(n,"getCache",{"fresh-import-key","float"}),b=call(n,"getCache",{"fresh-import-key","vec3"});auto result=JSValue::object();own(result,"same",a.sameValue(b));own(result,"type",a.get("uniformType"));return result;}));if(prior)globals.defineProperty("Map",*prior);else globals.erase("Map");own(out,"events",events);return out;
}
void initializeMaterial(){
 auto p=nativeUniformNodeConstructor().get("prototype"),captured=JSValue::object();const auto oldReference=p.getOwnPropertyDescriptor("onReference"),oldRender=p.getOwnPropertyDescriptor("onRenderUpdate");const auto reference=p.get("onReference"),render=p.get("onRenderUpdate");
 for(const auto* name:{"onReference","onRenderUpdate"}){JSPropertyDescriptor d;d.writable=d.configurable=true;d.value=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{auto cell=captures[0];cell.set(captures[1].toString(),arg(args,0));return captures[2].call(self,{arg(args,0)});},{captured,name,std::string(name)=="onReference"?reference:render});p.defineProperty(name,d);}
 const auto restore=[&]{if(oldReference)p.defineProperty("onReference",*oldReference);else p.erase("onReference");if(oldRender)p.defineProperty("onRenderUpdate",*oldRender);else p.erase("onRenderUpdate");};try{nativeMaterialNodeModule();restore();}catch(...){restore();throw;}referenceCallback=captured.get("onReference");renderCallback=captured.get("onRenderUpdate");
}
JSValue metadata(){const auto c=nativeMaterialNodeConstructor(),m=nativeMaterialNodeModule();auto out=JSValue::object(),keys=array();for(const auto& k:m.keys())push(keys,k);own(out,"keys",keys);own(out,"constructor",describe(c));own(out,"prototype",describe(c.get("prototype")));return out;}
JSValue source(const JSValue& r){const auto c=nativeMaterialNodeConstructor();const auto name=r.get("name").toString();const auto f=name=="class"?c:name=="type"?*c.getOwnPropertyDescriptor("type")->getter:name=="referenceCallback"?referenceCallback:name=="renderCallback"?renderCallback:c.get("prototype").get(name);auto o=JSValue::object();own(o,"source",call(f,"toString"));own(o,"name",f.get("name"));own(o,"length",f.get("length"));own(o,"constructible",f.isConstructible());return o;}
JSValue construction(const JSValue& r){auto s=state(r),o=JSValue::object();own(o,"result",attempt([&]{return nativeMaterialNodeConstructor().construct({decode(r.get("scope"),s)});}));return o;}
JSValue cacheCase(const JSValue& r){auto s=state(r),a=nativeMaterialNodeConstructor().construct({"cache"}),b=nativeMaterialNodeConstructor().construct({"other"}),results=array();const auto key=decode(r.get("key"),s);auto first=JSValue{};for(const auto& row:values(r.get("calls"))){const auto who=row.get("other").toBoolean()?b:a;push(results,attempt([&]{const auto value=call(who,row.get("method").toString(),row.has("type")?Args{key,decode(row.get("type"),s)}:Args{key});if(first.isUndefined())first=value;auto out=JSValue::object();own(out,"node",normal(value));own(out,"sameAsFirst",value.sameValue(first));return out;}));}auto out=JSValue::object();own(out,"results",results);own(out,"events",s.get("events"));return out;}
JSValue setupCase(const JSValue& r){
 auto s=state(r),n=nativeMaterialNodeConstructor().construct({r.get("scope")}),material=decode(r.get("material"),s),context=JSValue::object(),builder=JSValue::object();context.set("material",material);builder.set("context",context);n.set("_tag","self");s.set("node",n);
 n.set("getNodeType",JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{tick(captures[0],"call getNodeType",{arg(args,0).get("_tag")});return captures[1];},{s,r.has("outputType")?r.get("outputType"):JSValue("float")}));builder.set("_tag","builder");
 for(const auto* name:{"getCache","getFloat","getColor","getTexture"}){const auto f=n.get(name);n.set(name,JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{tick(captures[0],"call "+captures[1].toString(),args);return captures[2].call(self,args);},{s,name,f}));}
 if(r.get("observed").toBoolean()){s.set("active",false);for(const auto* name:{"scope","getCache","getFloat","getColor","getTexture","getNodeType"})observe(s,n,name,std::string("self.")+name);observe(s,context,"material","context.material");observe(s,builder,"context","builder.context");for(const auto& name:material.keys())observe(s,material,name,"material."+name);s.set("active",true);}
 auto out=JSValue::object();own(out,"result",attempt([&]{return call(n,"setup",{builder});}));s.set("active",false);own(out,"events",s.get("events"));return out;
}
JSValue callbackCase(const JSValue& r){
 auto s=state(r),material=decode(r.get("material"),s),frame=JSValue::object();frame.set("material",material);auto vector=nativeVector2Constructor().construct(),self=JSValue::object();self.set("value",vector);auto globals=nativeGlobalObject();const auto oldMath=globals.getOwnPropertyDescriptor("Math");const auto math=nativeGlobalBinding("Math");
 if(r.get("mathProtocol").toBoolean()){
  auto proxyMath=JSValue::object();for(const auto* name:{"cos","sin"}){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{tick(c[0],"get Math."+c[1].toString());return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"call Math."+c[1].toString(),a);return c[2].call(c[3],a);},c);},{s,name,math.get(name),math});proxyMath.defineProperty(name,d);}JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{tick(c[0],"get global.Math");return c[1];},{s,proxyMath});globals.defineProperty("Math",d);
 }
 if(r.get("observed").toBoolean()){for(const auto* name:{"anisotropy","anisotropyRotation"})observe(s,material,name,std::string("material.")+name);observe(s,frame,"material","frame.material");observe(s,self,"value","self.value");observe(s,vector,"set","vector.set");}
 auto out=JSValue::object();const auto key=r.get("which").toString();own(out,"result",attempt([&]{return (key=="reference"?referenceCallback:renderCallback).call(self,{frame});}));if(oldMath)globals.defineProperty("Math",*oldMath);else globals.erase("Math");s.set("active",false);own(out,"value",normal(vector));own(out,"events",s.get("events"));return out;
}
JSValue mathCase(const JSValue& r){auto s=state(r),out=JSValue::object();const auto math=nativeGlobalBinding("Math"),f=math.get(r.get("name").toString());if(r.get("metadata").toBoolean()){own(out,"name",f.get("name"));own(out,"length",f.get("length"));own(out,"constructible",f.isConstructible());own(out,"prototype",normal(f.get("prototype")));own(out,"tag",call(JSValue::intrinsicObjectConstructor().get("prototype").get("toString"),"call",{math}));const auto desc=math.getOwnPropertyDescriptor(r.get("name").toString()).value();own(out,"flags",array({desc.writable.value_or(false),desc.enumerable.value_or(false),desc.configurable.value_or(false)}));return out;}own(out,"result",attempt([&]{return f.call({}, {decode(r.get("value"),s)});}));own(out,"events",s.get("events"));return out;}
JSValue units(const JSValue& text){auto result=array();for(const auto unit:jsUTF16(text.toString()))push(result,double(unit));return result;}
JSValue stringCase(const JSValue& r){
 auto s=state(r),out=JSValue::object();const auto c=nativeGlobalBinding("String"),from=c.get("fromCharCode");Args args;for(const auto& value:values(r.get("args")))args.push_back(decode(value,s));
 if(r.get("mode").strictEquals("metadata")){own(out,"name",c.get("name"));own(out,"length",c.get("length"));own(out,"constructible",c.isConstructible());own(out,"fromName",from.get("name"));own(out,"fromLength",from.get("length"));own(out,"fromConstructible",from.isConstructible());own(out,"fromPrototype",normal(from.get("prototype")));own(out,"sharedPrototype",c.get("prototype").sameValue(nativeStringPrototype()));own(out,"sharedConstructor",c.get("prototype").get("constructor").sameValue(c));const auto d=c.getOwnPropertyDescriptor("prototype").value(),f=c.getOwnPropertyDescriptor("fromCharCode").value();own(out,"prototypeFlags",array({d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)}));own(out,"fromFlags",array({f.writable.value_or(false),f.enumerable.value_or(false),f.configurable.value_or(false)}));return out;}
 own(out,"result",attempt([&]()->JSValue{
  if(r.get("mode").strictEquals("from"))return units(from.call(c,args));
  if(r.get("mode").strictEquals("call"))return units(c.call({},args));
  auto prototype=JSValue::object();JSValue target=c;
  if(r.get("customTarget").toBoolean()){
   const auto base=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Target",0,JSValue::ConstructorKind::Callable);auto handler=JSValue::object();handler.set("get",JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{if(arg(args,1).strictEquals("prototype")){tick(captures[0],"get target.prototype");return captures[1].toBoolean()?JSValue(3):captures[2];}return arg(args,0).get(arg(args,1),arg(args,2));},{s,r.get("badPrototype"),prototype}));target=JSValue::proxy(base,handler);
  }
  const auto value=c.construct(args,target);auto answer=JSValue::object();own(answer,"units",units(nativeStringValue(value)));own(answer,"length",value.get("length"));own(answer,"prototypeMatches",value.getPrototype().sameValue(r.get("customTarget").toBoolean()&&!r.get("badPrototype").toBoolean()?prototype:c.get("prototype")));return answer;
 }));own(out,"events",s.get("events"));return out;
}
JSValue integration(const JSValue& r){auto s=state(r),material=decode(r.get("material"),s),object=JSValue::object(),renderer=JSValue::object();object.set("material",material);object.set("geometry",JSValue::object());renderer.set("coordinateSystem",2001);renderer.set("outputColorSpace","");if(r.get("configured").toBoolean()){auto debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);}auto builder=nativeNodeBuilderConstructor().construct({object,renderer,nullptr});builder.set("shaderStage","fragment");auto context=builder.get("context");context.set("material",material);if(r.get("configured").toBoolean())context.set("getUV",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0].call({}, {0,0,1});},{nativeTSLBaseBinding("vec3")}));auto n=nativeMaterialNodeConstructor().construct({r.get("scope")}),results=array();if(r.has("nodeType"))n.set("nodeType",r.get("nodeType"));for(const auto* stage:{"setup","analyze","generate"}){builder.set("buildStage",stage);push(results,attempt([&]{return call(n,"build",{builder,r.get("output")});}));}auto out=JSValue::object();own(out,"results",results);own(out,"flow",normal(builder.get("flow")));return out;}
JSValue run(const JSValue& r){const auto kind=r.get("kind");if(kind.strictEquals("metadata"))return metadata();if(kind.strictEquals("source"))return source(r);if(kind.strictEquals("construct"))return construction(r);if(kind.strictEquals("cache"))return cacheCase(r);if(kind.strictEquals("callback"))return callbackCase(r);if(kind.strictEquals("math"))return mathCase(r);if(kind.strictEquals("string"))return stringCase(r);if(kind.strictEquals("integration"))return integration(r);if(kind.strictEquals("exports")){auto o=JSValue::object();for(const auto& k:nativeMaterialNodeModule().keys())if(k!="default")own(o,k,normal(nativeMaterialNodeBinding(k)));return o;}return setupCase(r);}
JsonValue fresh(){JsonValue::Array result;for(int i=0;i<3;++i){if(JSValue::hasActiveHeap())throw std::logic_error("existing heap");{auto m=nativeMaterialNodeModule();const auto c=nativeMaterialNodeConstructor();auto a=c.construct({"color"}),b=c.construct({"opacity"});const auto one=call(a,"getCache",{"fresh","float"}),two=call(b,"getCache",{"fresh","vec3"});m.collectGarbage();if(!one.sameValue(two)||!one.get("uniformType").strictEquals("float"))throw std::logic_error("cache identity/type");auto facade=m;facade.set("materialColor",nullptr);if(nativeMaterialNodeBinding("materialColor").isNull())throw std::logic_error("binding lost");MaterialNode x("color");auto y=x;y=x;const auto math=nativeGlobalBinding("Math");if(!call(math,"sin",{0}).strictEquals(0))throw std::logic_error("Math lost");}if(JSValue::hasActiveHeap())throw std::logic_error("heap leaked");result.emplace_back(i);}return result;}
}
int main(int argc,char** argv){try{if(argc>1&&std::string(argv[1])=="--fresh-gc"){std::cout<<fresh().dump();return 0;}if(argc>2&&std::string(argv[1])=="--map-import"){std::cout<<mapImport(argv[2]).toJSON()->dump();return 0;}const auto ref=nativeReferenceNodeModule(),mr=nativeMaterialReferenceNodeModule();initializeMaterial();const auto builder=nativeNodeBuilderConstructor();const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);auto result=array();for(const auto& r:values(requests)){relative=r.get("relativeNodeIds").toBoolean();baseline=nativeNodeConstructor().construct().get("id").toNumber();push(result,run(r));}std::cout<<result.toJSON()->dump();referenceCallback={};renderCallback={};}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
