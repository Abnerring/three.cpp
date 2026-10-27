#include "animation/AnimationUtils.h"
#include "core/JSValue.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "nodes/accessors/CubeTextureNode.h"
#include "nodes/accessors/ReflectVector.h"
#include "nodes/accessors/MaterialProperties.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/NodeFrame.h"
#include "nodes/core/UniformNode.h"
#include "nodes/tsl/TSLBase.h"
#include "textures/CubeTexture.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
using Args=std::vector<JSValue>;
bool relative=false,relativeResources=false,resourceOriginSet=false;double baseline=0,resourceOrigin=0;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue rec(std::initializer_list<std::pair<const char*,JSValue>> fields){auto o=JSValue::object();for(const auto& [k,v]:fields)own(o,k,v);return o;}
JSValue arr(const Args& a={}){auto o=JSValue::array();for(std::size_t i=0;i<a.size();++i)own(o,std::to_string(i),a[i]);return o;}
JSValue call(const JSValue& o,const char* k,const Args& a={}){return o.get(k).call(o,a);}
void push(const JSValue& o,const JSValue& v){call(o,"push",{v});}
JSValue normal(const JSValue& v,Args* seen=nullptr){
    if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";if(v.isSymbol())return "#symbol";
    if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "#NaN";if(!std::isfinite(n))return n>0?"#Infinity":"#-Infinity";if(n==0&&std::signbit(n))return "#-0";}
    if(!v.isObject())return v;if(v.get("_tag").toBoolean())return "@"+v.get("_tag").toString();
    Args local;if(!seen)seen=&local;for(std::size_t i=0;i<seen->size();++i)if(v.sameValue((*seen)[i]))return arr({"#ref",double(i)});const auto index=double(seen->size());seen->push_back(v);
    // Texture UUIDs are random; this is an explicit selected-field texture snapshot.
    // Node ID mode never changes Texture.id. Resource IDs require their own
    // explicit mode and use an actually allocated Texture as the import baseline.
    // Imported Texture IDs stay absolute; only later allocations are relative.
    if(v.get("isTexture").strictEquals(true)){auto o=JSValue::object();for(const auto* k:{"id","isTexture","isCubeTexture","isDepthTexture","mapping","flipY","version","isRenderTargetTexture"})own(o,k,relativeResources&&std::string(k)=="id"&&v.get(k).toNumber()>resourceOrigin?JSValue(v.get(k).toNumber()-resourceOrigin):normal(v.get(k),seen));return o;}
    auto o=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();for(const auto& k:v.keys()){const auto x=v.get(k);own(o,k,relative&&k=="id"&&v.get("isNode").strictEquals(true)?(x.toNumber()>baseline?arr({"#local",x.toNumber()-baseline}):arr({"#imported",index})):normal(x,seen));}return o;
}
template<class F>JSValue attempt(F f){try{return normal(f());}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception&){return "#Error";}}
void tick(const JSValue& s,const std::string& key){push(s.get("events"),key);if(s.get("r").get("throwAt").strictEquals(key))throw JSTypeError(key);}
JSValue state(const JSValue& r){return rec({{"r",r},{"events",arr()}});}
void observe(const JSValue& s,JSValue o,const char* key,const std::string& label){auto slot=rec({{"value",o.get(key)}});JSPropertyDescriptor d;d.enumerable=d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{tick(c[0],c[2].string());return c[1].get("value");},{s,slot,label},"get fixture",0);d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"set "+c[2].string());auto slot=c[1];slot.set("value",arg(a,0));return {};},{s,slot,label},"set fixture",1);o.defineProperty(key,d);}
JSValue decode(const JSValue& v,const JSValue& s){
    if(!v.isObject())return v;const auto tag=v.get("tag");if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("NaN"))return std::nan("");if(tag.strictEquals("Infinity"))return INFINITY;if(tag.strictEquals("-0"))return -0.;if(tag.strictEquals("symbol"))return JSValue::symbol("fixture");
    if(tag.strictEquals("node")){auto o=nativeNodeConstructor().construct({v.get("type").isUndefined()?JSValue("vec3"):v.get("type")});o.set("_tag",v.get("name").isUndefined()?JSValue("input"):v.get("name"));return o;}
    if(tag.strictEquals("cube"))return nativeCubeTextureConstructor().construct();
    if(tag.strictEquals("primitive")){auto o=JSValue::object();o.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"primitive."+c[1].get("name").string()+"."+arg(a,0).string());return c[1].get("value");},{s,v}));return o;}
    auto o=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();for(const auto& k:v.keys())own(o,k,decode(v.get(k),s));return o;
}
JSValue describe(const JSValue& o){auto a=arr();for(const auto& k:o.ownPropertyNames()){const auto d=o.getOwnPropertyDescriptor(k).value();const auto v=d.value.value_or(JSValue{});push(a,arr({k,v.isObject()&&!v.isCallable()?JSValue("#object"):normal(v),v.isCallable()?v.get("name"):JSValue(nullptr),v.isCallable()?v.get("length"):JSValue(nullptr),v.isConstructible(),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false),d.getter&&d.getter->isCallable()?d.getter->get("name"):JSValue(nullptr),d.setter&&d.setter->isCallable()?d.setter->get("name"):JSValue(nullptr)}));}return a;}
JSValue module(const JSValue& name){return name.strictEquals("Cube")?nativeCubeTextureNodeModule():name.strictEquals("Reflect")?nativeReflectVectorModule():nativeMaterialPropertiesModule();}
JSValue metadata(const JSValue& r){const auto m=module(r.get("module"));auto a=arr();for(const auto& k:m.keys()){const auto v=m.get(k);push(a,arr({k,v.isCallable()?"function":"object",v.isCallable()?v.get("name"):JSValue(nullptr),v.isCallable()?v.get("length"):JSValue(nullptr),v.isConstructible()}));}auto out=rec({{"exports",a}});if(r.get("module").strictEquals("Cube")){const auto ctor=nativeCubeTextureNodeConstructor();own(out,"constructor",describe(ctor));own(out,"prototype",describe(ctor.get("prototype")));own(out,"instance",normal(ctor.construct({nativeCubeTextureConstructor().construct()})));}else{auto nodes=JSValue::object();for(const auto& k:m.keys())own(nodes,k,normal(m.get(k)));own(out,"nodes",nodes);}return out;}
JSValue factory(const JSValue& r){auto s=state(r),out=JSValue::object();Args a;const auto input=r.get("args");for(unsigned i=0;i<input.length();++i)a.push_back(decode(input.get(JSValue(i)),s));const auto f=nativeCubeTextureNodeBinding(r.get("name").string());auto value=attempt([&]{return r.get("construct").toBoolean()?f.construct(a):f.call({},a);});own(out,"result",value);own(out,"allocationDelta",nativeNodeConstructor().construct().get("id").toNumber()-baseline);return out;}
JSValue clone(const JSValue& r){auto s=state(r);auto original=nativeCubeTextureNodeBinding("cubeTexture").call({}, {nativeCubeTextureConstructor().construct()});original.set("uvNode",nativeTSLBaseBinding("vec3").call({}, {1,2,3}));original.set("levelNode",nativeTSLBaseBinding("int").call({}, {2}));original.set("biasNode",nativeTSLBaseBinding("float").call({}, {.5}));Args a{original};for(const auto* k:{"uv","level","bias"})a.push_back(r.hasOwn(k)?decode(r.get(k),s):JSValue(nullptr));auto copied=nativeCubeTextureNodeBinding("cubeTexture").call({},a);return rec({{"copied",normal(copied)},{"reference",copied.get("referenceNode").sameValue(original)},{"sameValue",copied.get("value").sameValue(original.get("value"))},{"sameUV",copied.get("uvNode").sameValue(original.get("uvNode"))},{"sameLevel",copied.get("levelNode").sameValue(original.get("levelNode"))},{"sameBias",copied.get("biasNode").sameValue(original.get("biasNode"))},{"originalUpdateMatrix",original.get("updateMatrix")},{"copiedUpdateMatrix",copied.get("updateMatrix")}});}
JSValue method(const JSValue& r){
    auto s=state(r),texture=decode(r.get("texture"),s),self=rec({{"value",texture},{"sampler",decode(r.get("sampler"),s)},{"updateMatrix","sentinel"}}),renderer=rec({{"coordinateSystem",decode(r.get("coordinateSystem"),s)}}),builder=rec({{"renderer",renderer},{"_tag","builder"}}),uv=JSValue::object();
    if(r.get("realUV").toBoolean())uv=nativeTSLBaseBinding("vec3").call({}, {1,2,3});
    else{
        auto x=rec({{"negate",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{tick(c[0],"x.negate.call");return nativeTSLBaseBinding("float").call({}, {-1});},{s},"negate",0)}});
        own(uv,"x",x);own(uv,"yz",nativeTSLBaseBinding("vec2").call({}, {2,3}));
        own(uv,"build",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{tick(c[0],"uv.build.call");push(c[0].get("events"),arr({"build.arguments",arg(a,0).sameValue(c[1]),normal(arg(a,1)),self.sameValue(c[2])}));return "CUBE_UV";},{s,builder,uv},"build",2));
        if(r.get("observed").toBoolean()){observe(s,uv,"x","uv.x");observe(s,uv,"yz","uv.yz");observe(s,uv,"build","uv.build");observe(s,x,"negate","x.negate");}
    }
    if(r.get("observed").toBoolean()){
        observe(s,self,"value","self.value");observe(s,self,"sampler","self.sampler");observe(s,builder,"renderer","builder.renderer");observe(s,renderer,"coordinateSystem","renderer.coordinateSystem");
        if(texture.isObject())for(const auto* k:{"mapping","isRenderTargetTexture","isDepthTexture"})observe(s,texture,k,std::string("texture.")+k);
    }
    const auto old=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{tick(c[0],"console");push(c[0].get("events"),normal(arr(a)));return {};},{s}));
    auto result=attempt([&]{const auto name=r.get("method").string();const auto v=nativeCubeTextureNodeConstructor().get("prototype").get(name).call(self,{builder,uv});if(v.sameValue(nativeReflectVectorBinding("reflectVector")))return JSValue("#reflectVector");if(v.sameValue(nativeReflectVectorBinding("refractVector")))return JSValue("#refractVector");if(v.sameValue(uv))return JSValue("#uvIdentity");return v;});
    ThreeUtils::setConsoleFunction(old);return rec({{"result",result},{"events",s.get("events")},{"updateMatrix",self.get("updateMatrix")}});
}
JSValue frame(const JSValue& r,const JSValue& s){auto material=decode(r.get("material"),s),scene=decode(r.get("scene"),s);if(r.get("observed").toBoolean()){if(material.isObject())for(const auto* k:{"refractionRatio","envMap","envMapIntensity","envMapRotation"})observe(s,material,k,std::string("material.")+k);if(scene.isObject())for(const auto* k:{"environment","environmentNode","environmentIntensity","environmentRotation"})observe(s,scene,k,std::string("scene.")+k);}auto f=rec({{"material",material},{"scene",scene}});if(r.get("observed").toBoolean()){observe(s,f,"material","frame.material");observe(s,f,"scene","frame.scene");}return f;}
JSValue material(const JSValue& r){auto s=state(r),f=frame(r,s),node=nativeMaterialPropertiesBinding(r.get("name").string());JSValue initial=node.get("value");auto out=rec({{"reference",attempt([&]{return call(node,"updateReference",{f}).sameValue(f.get("material"));})}});own(out,"result",attempt([&]{if(r.get("viaFrame").toBoolean()){auto nf=NodeFrame().object();nf.set("material",f.get("material"));nf.set("scene",f.get("scene"));return call(nf,"updateNode",{node});}return call(node,"update",{f});}));own(out,"events",s.get("events"));own(out,"value",normal(node.get("value")));own(out,"initialIdentity",initial.sameValue(node.get("value")));return out;}
JSValue rotationSequence(const JSValue& r){auto out=arr();JSValue prior;const auto steps=r.get("steps");for(unsigned i=0;i<steps.length();++i){auto s=state(steps.get(JSValue(i))),f=frame(steps.get(JSValue(i)),s),n=nativeMaterialPropertiesBinding("materialEnvRotation");auto result=attempt([&]{return call(n,"update",{f});});auto value=n.get("value");push(out,rec({{"result",result},{"value",normal(value)},{"samePrior",i!=0&&prior.sameValue(value)},{"events",s.get("events")}}));prior=value;}return out;}
JSValue matrix(const JSValue& r){auto s=state(r),rotation=decode(r.get("rotation"),s),m=nativeMatrix4Constructor().construct();if(r.get("observed").toBoolean()&&rotation.isObject())for(const auto* k:{"x","y","z","order"})observe(s,rotation,k,std::string("rotation.")+k);auto out=rec({{"descriptor",describe(nativeMatrix4Constructor().get("prototype").get("makeRotationFromEuler"))}});own(out,"result",attempt([&]{return call(m,"makeRotationFromEuler",{rotation}).sameValue(m);}));own(out,"matrix",normal(m));own(out,"events",s.get("events"));return out;}
JSValue rotationProtocol(const JSValue& r){
    auto s=state(r),n=nativeMaterialPropertiesBinding("materialEnvRotation");
    call(n,"update",{rec({{"material",rec({{"envMapRotation",nullptr}})},{"scene",rec({{"environment",nullptr},{"environmentNode",nullptr}})}})});
    auto shared=n.get("value"),target=nativeMatrix4Constructor().construct();
    for(auto [o,prefix]:std::initializer_list<std::pair<JSValue,const char*>>{{shared,"shared"},{target,"target"}})for(const auto* key:{"makeRotationFromEuler","transpose","identity"}){
        const auto original=o.get(key);JSPropertyDescriptor d;d.configurable=true;
        d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            tick(c[0],c[3].string()+".get");
            return JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue{
                tick(c[0],c[3].string()+".call");push(c[0].get("events"),arr({"receiver",receiver.sameValue(c[1]),double(a.size())}));
                if(c[0].get("r").get("redirect").toBoolean()&&c[3].strictEquals("shared.makeRotationFromEuler"))return c[4];
                return c[2].call(receiver,a);
            },c,"fixture method",0);
        },{s,o,original,std::string(prefix)+"."+key,target},"get fixture method",0);o.defineProperty(key,d);
    }
    auto f=frame(r,s);const auto result=attempt([&]{return call(n,"update",{f});});
    for(auto o:{shared,target})for(const auto* key:{"makeRotationFromEuler","transpose","identity"})o.erase(key);
    return rec({{"result",result},{"value",normal(n.get("value"))},{"sharedIdentity",n.get("value").sameValue(shared)},{"target",normal(target)},{"events",s.get("events")}});
}
JSValue reentry(const JSValue& r){
    auto s=state(r),n=nativeMaterialPropertiesBinding("materialEnvRotation"),outer=frame(r,s),inner=frame(r.get("inner"),state(r.get("inner"))),guard=rec({{"once",true},{"nested",nullptr}});
    auto owner=outer.get(r.get("owner").string());const auto key=r.get("property").string();const auto value=owner.get(key);JSPropertyDescriptor d;d.enumerable=d.configurable=true;
    d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        tick(c[0],"reentry.get");auto guard=c[1];
        if(guard.get("once").toBoolean()){guard.set("once",false);tick(c[0],"reentry.enter");guard.set("nested",attempt([&]{return call(c[2],"update",{c[3]});}));tick(c[0],"reentry.exit");}
        return c[4];
    },{s,guard,n,inner,value},"get reentry",0);owner.defineProperty(key,d);
    const auto result=attempt([&]{return call(n,"update",{outer});});return rec({{"result",result},{"nested",guard.get("nested")},{"value",normal(n.get("value"))},{"events",s.get("events")}});
}
JSValue integration(const JSValue& r){
    auto renderer=rec({{"coordinateSystem",2001},{"backend",JSValue::object()},{"debug",rec({{"diagnostics",rec({{"keywords",false}})}})}}),geometry=JSValue::object(),material=rec({{"side",0},{"flatShading",false}}),object=rec({{"geometry",geometry},{"material",material}});
    auto attribute=rec({{"itemSize",3},{"normalized",false}});own(geometry,"hasAttribute",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return true;}));own(geometry,"getAttribute",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{attribute}));
    auto builder=nativeNodeBuilderConstructor().construct({object,renderer,nullptr});builder.set("shaderStage",r.get("stage"));builder.set("camera",rec({{"isOrthographicCamera",r.get("orthographic")},{"matrixWorld",nativeMatrix4Constructor().construct()}}));builder.set("material",material);
    auto context=builder.get("context");for(const auto* k:{"setupNormal","setupPositionView"})context.set(k,JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return nativeTSLBaseBinding("vec3").call({}, {0,0,1});}));
    auto node=r.get("name").strictEquals("cube")?nativeCubeTextureNodeBinding("cubeTexture").call({}, {nativeCubeTextureConstructor().construct(),nativeTSLBaseBinding("vec3").call({}, {1,2,3})}):nativeReflectVectorBinding(r.get("name").string());
    auto out=arr();for(const auto* stage:{"setup","analyze","generate"}){builder.set("buildStage",stage);push(out,attempt([&]{auto v=call(node,"build",{builder,"vec3"});return v.isObject()&&v.get("isNode").strictEquals(true)?JSValue("#"+v.get("type").toString()):v;}));}return rec({{"results",out},{"flow",normal(builder.get("flow"))}});
}
JSValue sourceMetadata(){auto out=JSValue::object(),functions=JSValue::object();const auto ctor=nativeCubeTextureNodeConstructor();for(const auto& name:nativeCubeTextureNodeModule().keys())own(functions,name,nativeCubeTextureNodeBinding(name).nativeFunctionSource());own(out,"exports",functions);own(out,"typeGetter",ctor.getOwnPropertyDescriptor("type")->getter->nativeFunctionSource());auto methods=JSValue::object();for(const auto* key:{"getInputType","getDefaultUV","setUpdateMatrix","setupUV","generateUV"})own(methods,key,ctor.get("prototype").get(key).nativeFunctionSource());own(out,"methods",methods);own(out,"matrix",nativeMatrix4Constructor().get("prototype").get("makeRotationFromEuler").nativeFunctionSource());auto callbacks=JSValue::object();for(const auto& key:nativeMaterialPropertiesModule().keys()){const auto n=nativeMaterialPropertiesBinding(key);own(callbacks,key,arr({n.get("updateReference").nativeFunctionSource(),n.get("updateReference").isConstructible(),n.get("update").nativeFunctionSource(),n.get("update").isConstructible()}));}own(out,"boundCallbacks",callbacks);return out;}
JSValue callbackSources(){auto uniform=nativeUniformNodeConstructor(),prototype=uniform.get("prototype"),calls=arr();for(const auto* key:{"onReference","onObjectUpdate"}){const auto original=prototype.get(key);own(prototype,key,JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{const auto fn=arg(a,0);push(c[0],rec({{"method",c[1]},{"name",fn.get("name")},{"length",fn.get("length")},{"constructible",fn.isConstructible()},{"descriptors",describe(fn)},{"source",fn.nativeFunctionSource()}}));return c[2].call(self,a);},{calls,key,original},"capture",1));}const auto module=nativeMaterialPropertiesModule();return calls;}
JSValue run(const JSValue& r){const auto k=r.get("kind");if(k.strictEquals("metadata"))return metadata(r);if(k.strictEquals("factory"))return factory(r);if(k.strictEquals("clone"))return clone(r);if(k.strictEquals("method"))return method(r);if(k.strictEquals("material"))return material(r);if(k.strictEquals("rotationSequence"))return rotationSequence(r);if(k.strictEquals("rotationProtocol"))return rotationProtocol(r);if(k.strictEquals("reentry"))return reentry(r);if(k.strictEquals("matrix"))return matrix(r);if(k.strictEquals("graph"))return normal(nativeReflectVectorBinding(r.get("name").string()));if(k.strictEquals("source"))return sourceMetadata();return integration(r);}
void lifetime(){for(int i=0;i<3;++i){JSWeakValue ctor,graph,uniform;{auto m=nativeCubeTextureNodeModule(),r=nativeReflectVectorModule(),p=nativeMaterialPropertiesModule();const auto c=nativeCubeTextureNodeConstructor(),g=nativeReflectVectorBinding("reflectVector"),u=nativeMaterialPropertiesBinding("materialEnvRotation");ctor=JSWeakValue(c);graph=JSWeakValue(g);uniform=JSWeakValue(u);auto n=c.construct({nativeCubeTextureConstructor().construct()});m.collectGarbage();if(!call(n,"getDefaultUV").sameValue(g))throw std::logic_error("cube/reflect graph lost");m.set("default",nullptr);r.set("reflectVector",nullptr);p.set("materialEnvRotation",nullptr);if(!nativeCubeTextureNodeConstructor().sameValue(c)||!nativeReflectVectorBinding("reflectVector").sameValue(g)||!nativeMaterialPropertiesBinding("materialEnvRotation").sameValue(u))throw std::logic_error("private module binding changed");}JSValue::nativeSafepoint();if(!ctor.expired()||!graph.expired()||!uniform.expired()||JSValue::hasActiveHeap())throw std::logic_error("retired module heap retained");}std::cout<<"3 fresh Cube/Reflect/MaterialProperties heap and private-binding GC lifetimes passed\n";}
}
int main(int argc,char** argv){try{if(argc>1&&std::string(argv[1])=="--lifetime"){lifetime();return 0;}if(argc>1&&std::string(argv[1])=="--callback-sources"){const auto out=callbackSources();std::cout<<out.toJSON()->dump();return 0;}const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input),c=nativeCubeTextureNodeModule(),r=nativeReflectVectorModule(),p=nativeMaterialPropertiesModule(),b=nativeNodeBuilderConstructor(),n=nativeNodeConstructor();auto out=arr();for(unsigned i=0;i<requests.length();++i){const auto request=requests.get(JSValue(i));relative=request.get("relativeNodeIds").toBoolean();relativeResources=request.get("relativeImportResourceIds").toBoolean();if(relativeResources&&!resourceOriginSet){resourceOrigin=nativeTextureConstructor().construct().get("id").number();resourceOriginSet=true;std::cerr<<"#TextureBaseline "<<resourceOrigin<<'\n';}baseline=n.construct().get("id").number();push(out,run(request));}std::cout<<out.toJSON()->dump();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
