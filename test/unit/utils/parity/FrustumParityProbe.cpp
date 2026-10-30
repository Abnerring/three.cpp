#include "animation/AnimationUtils.h"
#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include "math/Frustum.h"
#include "math/FrustumArray.h"
#include "math/Matrix4.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& values,unsigned index){return index<values.size()?values[index]:JSValue{};}
JSValue array(const Args& values={}){auto result=JSValue::array();for(const auto& value:values)result.get("push").call(result,{value});return result;}
void own(JSValue object,const JSValue& key,const JSValue& value){JSPropertyDescriptor descriptor;descriptor.value=value;descriptor.writable=descriptor.enumerable=descriptor.configurable=true;object.defineProperty(key,descriptor);}
void push(JSValue value,const JSValue& element){value.get("push").call(value,{element});}
JSValue ctor(const JSValue& request){return request.get("family").strictEquals("array")?nativeFrustumArrayConstructor():nativeFrustumConstructor();}
JSValue call(const JSValue& object,const std::string& key,const Args& args={}){const auto function=object.get(key);return function.call(object,args);}
JSValue normal(const JSValue& value,const JSValue& state={},unsigned depth=0){
    if(value.isUndefined())return "#undefined";if(value.isSymbol())return "#symbol";
    if(value.type()==JSValue::Type::Number){const auto number=value.number();if(std::isnan(number))return "#NaN";if(std::isinf(number))return number>0?"#Infinity":"#-Infinity";if(number==0&&std::signbit(number))return "#-0";}
    if(!value.isObject())return value;
    if(state.isObject()){const auto refs=state.get("refs");for(unsigned i=0;i<refs.length();++i){const auto item=refs.get(JSValue(i));if(item.get("0").sameValue(value))return "@"+item.get("1").string();}}
    if(value.isCallable())return "#function:"+value.get("name").toString();
    if(depth>10)return "#depth";
    if(value.type()==JSValue::Type::Array){auto result=array();for(unsigned i=0;i<value.length();++i)push(result,normal(value.get(JSValue(i)),state,depth+1));return result;}
    auto result=JSValue::object();for(const auto& key:value.keys())own(result,key,normal(value.get(key),state,depth+1));return result;
}
void ref(JSValue state,const JSValue& value,const std::string& name){push(state.get("refs"),array({value,name}));}
void event(JSValue state,const std::string& name,const Args& args={}){
    if(!state.get("active").toBoolean())return;auto row=array({name});for(const auto& value:args)push(row,normal(value,state));push(state.get("events"),row);
    if(state.get("request").get("throwAt").strictEquals(name))throw JSThrownValue(state.get("throwValue"));
    if(state.get("request").get("gcAt").strictEquals(name)){state.collectGarbage();push(state.get("events"),array({"gc"}));}
}
JSValue state(const JSValue& request){auto result=JSValue::object();own(result,"request",request);own(result,"events",array());own(result,"refs",array());own(result,"active",false);auto thrown=JSValue::object();own(thrown,"token","fixture-throw");own(result,"throwValue",thrown);ref(result,thrown,"throwValue");return result;}
JSValue decode(const JSValue& value,const JSValue& state){
    if(!value.isObject())return value;const auto tag=value.get("tag");
    if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();if(tag.strictEquals("Infinity"))return std::numeric_limits<double>::infinity();if(tag.strictEquals("-Infinity"))return -std::numeric_limits<double>::infinity();if(tag.strictEquals("-0"))return -0.0;if(tag.strictEquals("symbol"))return JSValue::symbol("fixture");
    if(tag.strictEquals("primitive")){auto object=JSValue::object();ref(state,object,"primitive:"+value.get("name").string());object.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{event(captures[0],"convert:"+captures[1].get("name").string(),{arg(args,0)});return decode(captures[1].get("value"),captures[0]);},{state,value},"primitive",1));return object;}
    auto object=value.type()==JSValue::Type::Array?JSValue::array(value.length()):JSValue::object();for(const auto& key:value.keys())own(object,key,decode(value.get(key),state));return object;
}
JSValue caught(const JSValue& state,std::exception_ptr exception){
    auto out=JSValue::object();try{std::rethrow_exception(exception);}catch(const JSThrownValue& exception){own(out,"value",normal(exception.value,state));own(out,"fixtureIdentity",exception.value.sameValue(state.get("throwValue")));if(exception.value.isObject()&&!exception.value.sameValue(state.get("throwValue"))){own(out,"name",exception.value.get("name"));own(out,"message",exception.value.get("message"));}}
    catch(const JSTypeError&){own(out,"name","TypeError");}catch(const JSReferenceError&){own(out,"name","ReferenceError");}catch(const JSRangeError&){own(out,"name","RangeError");}catch(const std::exception& exception){own(out,"nativeError",exception.what());}return out;
}
template<class F>JSValue attempt(const JSValue& state,F function){auto out=JSValue::object();try{own(out,"value",normal(function(),state));}catch(...){own(out,"throw",caught(state,std::current_exception()));}return out;}
void getter(JSValue state,JSValue object,const JSValue& key,const std::string& label,const JSValue& value){
    JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.enumerable=true;descriptor.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue{event(captures[0],"get:"+captures[1].string());return captures[2];},{state,label,value},"get "+label,0);object.defineProperty(key,descriptor);
}
void cell(JSValue state,JSValue object,const JSValue& key,const std::string& label,const JSValue& value){
    auto storage=JSValue::object();own(storage,"value",value);JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.enumerable=true;
    descriptor.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue{event(captures[0],"get:"+captures[1].string());return captures[2].get("value");},{state,label,storage},"get "+label,0);
    descriptor.setter=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{event(captures[0],"set:"+captures[1].string(),{arg(args,0)});auto storage=captures[2];storage.set("value",arg(args,0));return {};},{state,label,storage},"set "+label,1);object.defineProperty(key,descriptor);
}
void method(JSValue state,JSValue object,const std::string& name,const std::string& label,const JSValue& result,bool returnSelf=false){
    JSPropertyDescriptor descriptor;descriptor.configurable=true;descriptor.getter=JSValue::function(+[](const JSValue& receiver,const Args&,const Args& captures)->JSValue{
        event(captures[0],"get:"+captures[1].string());return JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue{
            event(captures[0],"call:"+captures[1].string(),{self.sameValue(captures[4]),array(args)});
            const auto request=captures[0].get("request");if(!captures[0].get("reentering").toBoolean()&&request.get("reenterAt").strictEquals("call:"+captures[1].string())){auto state=captures[0];state.set("reentering",true);try{const auto result=nativeFrustumConstructor().get("prototype").get("containsPoint").call(state.get("self"),{state.get("point")});event(state,"reenter:return",{result});}catch(...){state.set("reentering",false);throw;}state.set("reentering",false);}
            return captures[3].toBoolean()?self:captures[2];
        },{captures[0],captures[1],captures[2],captures[3],receiver},captures[5].string(),0);
    },{state,label,result,returnSelf,object,name},"get "+label,0);object.defineProperty(name,descriptor);
}
void observe(JSValue state,JSValue object,const std::string& key,const std::string& label){
    const auto value=object.get(key);getter(state,object,key,label,value);
}
JSValue vector(const JSValue& values,unsigned count=3){Args args;for(unsigned i=0;i<count;++i)args.push_back(values.get(JSValue(i)));return (count==2?nativeVector2Constructor():nativeVector3Constructor()).construct(args);}
JSValue matrix(const JSValue& values){auto result=nativeMatrix4Constructor().construct();if(values.isObject())result.set("elements",values);return result;}
JSValue planes(const JSValue& values){auto result=array();for(unsigned i=0;i<6;++i){const auto value=values.get(JSValue(i));push(result,nativePlaneConstructor().construct({vector(value),value.get("3")}));}return result;}
JSValue snapshot(const JSValue& value){return normal(value);}
JSValue describe(const JSValue& object){auto result=array();for(const auto& key:object.ownPropertyNames()){const auto d=object.getOwnPropertyDescriptor(key).value();const auto value=d.value.value_or(JSValue{});push(result,array({key,value.isObject()&&!value.isCallable()?JSValue("#object"):normal(value),value.isCallable()?value.get("name"):JSValue(nullptr),value.isCallable()?value.get("length"):JSValue(nullptr),value.isConstructible(),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)}));}return result;}
JSValue metadata(const JSValue& request){const auto constructor=ctor(request);return [&]{auto result=JSValue::object();own(result,"constructor",describe(constructor));own(result,"prototype",describe(constructor.get("prototype")));own(result,"constructorSource",constructor.nativeFunctionSource());auto methods=JSValue::object();for(const auto& key:constructor.get("prototype").ownPropertyNames())if(key!="constructor")own(methods,key,constructor.get("prototype").get(key).nativeFunctionSource());own(result,"methodSources",methods);return result;}();}
JSValue numeric(const JSValue& request){
    auto s=state(request),object=nativeFrustumConstructor().construct();if(request.hasOwn("planes"))object.set("planes",planes(decode(request.get("planes"),s)));
    auto out=JSValue::object();const auto name=request.get("method").string();Args args;
    if(name=="setFromProjectionMatrix")args={matrix(decode(request.get("elements"),s)),decode(request.get("coordinateSystem"),s),decode(request.get("reversedDepth"),s)};
    else if(name=="containsPoint")args={vector(decode(request.get("point"),s))};
    else if(name=="intersectsSphere")args={nativeSphereConstructor().construct({vector(decode(request.get("point"),s)),decode(request.get("radius"),s)})};
    else if(name=="intersectsBox"){auto box=JSValue::object();own(box,"min",vector(decode(request.get("min"),s)));own(box,"max",vector(decode(request.get("max"),s)));args={box};}
    else if(name=="intersectsObject"){auto input=JSValue::object(),sphere=nativeSphereConstructor().construct({vector(decode(request.get("point"),s)),decode(request.get("radius"),s)});own(input,"matrixWorld",matrix(decode(request.get("elements"),s)));if(request.get("geometry").toBoolean()){auto geometry=JSValue::object();own(geometry,"boundingSphere",sphere);own(input,"geometry",geometry);}else own(input,"boundingSphere",sphere);args={input};}
    else if(name=="intersectsSprite"){auto input=JSValue::object();own(input,"center",vector(decode(request.get("center"),s),2));own(input,"matrixWorld",matrix(decode(request.get("elements"),s)));args={input};}
    else if(name=="set"){const auto value=planes(decode(request.get("sourcePlanes"),s));for(unsigned i=0;i<6;++i)args.push_back(value.get(JSValue(i)));}
    else if(name=="copy"){auto source=nativeFrustumConstructor().construct();source.set("planes",planes(decode(request.get("sourcePlanes"),s)));args={source};}
    try{const auto result=call(object,name,args);own(out,"result",result.sameValue(object)?JSValue("@self"):snapshot(result));if(name=="clone")own(out,"cloneIndependent",!result.get("planes").sameValue(object.get("planes"))&&!result.get("planes").get("0").sameValue(object.get("planes").get("0")));}catch(...){own(out,"throw",caught(s,std::current_exception()));}own(out,"self",snapshot(object));return out;
}
JSValue arrayNumeric(const JSValue& request){
    const auto s=state(request);auto object=nativeFrustumArrayConstructor().construct(),out=JSValue::object(),passes=array();const auto raw=request.get("passes");JSValue first;
    for(unsigned pass=0;pass<raw.length();++pass){const auto count=raw.get(JSValue(pass)).toNumber();auto cameras=array();for(double i=0;i<count;++i){auto camera=JSValue::object();own(camera,"projectionMatrix",matrix(request.get("elements")));own(camera,"matrixWorldInverse",matrix({}));own(camera,"coordinateSystem",decode(request.get("coordinateSystem"),s));own(camera,"reversedDepth",decode(request.get("reversedDepth"),s));push(cameras,camera);}auto cameraArray=JSValue::object();own(cameraArray,"cameras",cameras);auto row=JSValue::object();try{own(row,"returnedSelf",call(object,"setFromArrayCamera",{cameraArray}).sameValue(object));}catch(...){own(row,"throw",caught(s,std::current_exception()));}if(first.isUndefined()&&object.get("_frustums").length())first=object.get("_frustums").get("0");own(row,"count",object.get("_count"));own(row,"poolLength",object.get("_frustums").get("length"));own(row,"firstReused",first.isUndefined()||object.get("_frustums").get("0").sameValue(first));push(passes,row);}
    own(out,"passes",passes);const auto methodName=request.get("method").string();
    if(methodName=="copy"||methodName=="clone"){
        auto target=methodName=="copy"?nativeFrustumArrayConstructor().construct():JSValue{};const auto result=methodName=="copy"?call(target,"copy",{object}):call(object,"clone");own(out,"result",snapshot(result));own(out,"independent",result.get("_count").strictEquals(0)||!result.get("_frustums").get("0").sameValue(object.get("_frustums").get("0")));
    }else if(methodName!="setFromArrayCamera"){
        auto value=methodName=="containsPoint"?vector(request.get("point")):methodName=="intersectsSphere"?nativeSphereConstructor().construct({vector(request.get("point")),decode(request.get("radius"),s)}):JSValue::object();
        if(methodName=="intersectsBox"){own(value,"min",vector(request.get("min")));own(value,"max",vector(request.get("max")));}
        if(methodName=="intersectsObject"){own(value,"boundingSphere",nativeSphereConstructor().construct({vector(request.get("point")),request.get("radius")}));own(value,"matrixWorld",matrix({}));}
        if(methodName=="intersectsSprite"){own(value,"center",vector(request.get("center"),2));own(value,"matrixWorld",matrix({}));}
        own(out,"result",call(object,methodName,{value}));
    }
    own(out,"self",snapshot(object));return out;
}
struct Restore {JSValue object;std::string key;std::optional<JSPropertyDescriptor> descriptor;};
void restore(const std::vector<Restore>& entries){for(auto i=entries.rbegin();i!=entries.rend();++i){auto object=i->object;if(i->descriptor)object.defineProperty(i->key,*i->descriptor);else object.erase(i->key);}}
JSValue observedVector(JSValue state,const std::string& name,const JSValue& values){auto result=JSValue::object();ref(state,result,name);for(unsigned i=0;i<3;++i)getter(state,result,std::string(1,"xyz"[i]),name+"."+"xyz"[i],decode(values.get(JSValue(i)),state));return result;}
void sequenceGetter(JSValue state,JSValue object,const std::string& key,const std::string& name,const JSValue& input){auto values=array();for(unsigned i=0;i<input.length();++i)push(values,decode(input.get(JSValue(i)),state));auto cell=JSValue::object();own(cell,"index",0);JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue{event(captures[0],"get:"+captures[1].string());auto cell=captures[3];const auto index=cell.get("index").number();cell.set("index",index+1);const auto values=captures[2];return values.get(JSValue(std::min(index,double(values.length()-1))));},{state,name,values,cell},"get "+name,0);d.setter=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{event(captures[0],"set:"+captures[1].string(),{arg(args,0)});return {};},{state,name},"set "+name,1);object.defineProperty(key,d);}
JSValue protocol(const JSValue& request){
    auto s=state(request),self=JSValue::object(),point=JSValue::object(),out=JSValue::object();ref(s,self,"self");ref(s,point,"point");own(s,"self",self);own(s,"point",point);const auto name=request.get("method").string();const bool isArray=request.get("family").strictEquals("array");Args args;std::vector<Restore> changes;
    auto pool=JSValue::object();ref(s,pool,isArray?"pool":"planes");
    if(!isArray){
        getter(s,self,"planes","self.planes",pool);
        for(unsigned i=0;i<6;++i){auto plane=JSValue::object();const auto label="p"+std::to_string(i);ref(s,plane,label);getter(s,pool,JSValue(i),"planes."+std::to_string(i),plane);method(s,plane,"copy",label+".copy",{},true);method(s,plane,"setComponents",label+".setComponents",{},true);method(s,plane,"normalize",label+".normalize",{},true);
            const auto distances=request.get("distances");method(s,plane,"distanceToPoint",label+".distanceToPoint",decode(distances.isObject()?distances.get(JSValue(i)):JSValue(1),s));
            const auto values=request.hasOwn("normal")?request.get("normal"):array({1,-1,0});getter(s,plane,"normal",label+".normal",observedVector(s,label+".normal",values));
        }
        if(name=="set")for(unsigned i=0;i<6;++i){auto value=JSValue::object();ref(s,value,"arg"+std::to_string(i));args.push_back(value);}
        else if(name=="copy"){auto source=JSValue::object(),sourcePlanes=JSValue::object();ref(s,source,"source");ref(s,sourcePlanes,"sourcePlanes");getter(s,source,"planes","source.planes",sourcePlanes);for(unsigned i=0;i<6;++i){auto value=JSValue::object();ref(s,value,"arg"+std::to_string(i));getter(s,sourcePlanes,JSValue(i),"sourcePlanes."+std::to_string(i),value);}args={source};}
        else if(name=="setFromProjectionMatrix"){
            auto value=JSValue::object(),elements=JSValue::object();ref(s,value,"matrix");ref(s,elements,"elements");getter(s,value,"elements","matrix.elements",elements);for(unsigned i=0;i<16;++i)getter(s,elements,JSValue(i),"elements."+std::to_string(i),decode(request.get("elements").get(JSValue(i)),s));args={value,decode(request.get("coordinateSystem"),s),decode(request.get("reversedDepth"),s)};
            if(request.hasOwn("errorMode")){
                auto global=nativeGlobalObject();changes.push_back({global,"Error",global.getOwnPropertyDescriptor("Error")});const auto mode=request.get("errorMode");
                if(mode.strictEquals("missing"))global.erase("Error");else{
                    auto replacement=mode.strictEquals("nonconstructible")?JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Error",1):JSValue::constructor(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{event(captures[0],"construct:Error",args);return captures[0].get("throwValue");},{s},"Error",1,JSValue::ConstructorKind::Callable);
                    getter(s,global,"Error","global.Error",replacement);
                }
            }
        }else if(name=="containsPoint")args={point};
        else if(name=="intersectsSphere"){auto sphere=JSValue::object();ref(s,sphere,"sphere");getter(s,sphere,"center","sphere.center",point);getter(s,sphere,"radius","sphere.radius",decode(request.get("radius"),s));args={sphere};}
        else if(name=="intersectsBox"){auto box=JSValue::object();ref(s,box,"box");getter(s,box,"min","box.min",observedVector(s,"box.min",request.get("min")));getter(s,box,"max","box.max",observedVector(s,"box.max",request.get("max")));args={box};}
        else if(name=="intersectsObject"||name=="intersectsSprite"){
            auto input=JSValue::object();ref(s,input,"input");getter(s,input,"matrixWorld","input.matrixWorld",matrix(request.get("matrix")));method(s,self,"intersectsSphere","self.intersectsSphere",decode(request.get("result"),s));
            if(name=="intersectsSprite")getter(s,input,"center","input.center",vector(request.get("center"),2));
            else{
                auto sphere=nativeSphereConstructor().construct({nativeVector3Constructor().construct({1,2,3}),2});ref(s,sphere,"inputSphere");
                auto holder=input;const auto branch=request.get("branch");if(branch.strictEquals("geometry")||branch.strictEquals("geometryNull")){getter(s,input,"boundingSphere","input.boundingSphere",{});holder=JSValue::object();ref(s,holder,"geometry");getter(s,input,"geometry","input.geometry",holder);}
                const auto null=branch.strictEquals("null")||branch.strictEquals("geometryNull");cell(s,holder,"boundingSphere",holder.sameValue(input)?"input.boundingSphere":"geometry.boundingSphere",null?JSValue(nullptr):sphere);
                own(holder,"computeBoundingSphere",JSValue::function(+[](const JSValue& self,const Args&,const Args& captures)->JSValue{event(captures[0],"call:computeBoundingSphere");auto value=self;value.set("boundingSphere",captures[1]);return {};},{s,sphere},"computeBoundingSphere",0));
            }
            const auto prototype=(name=="intersectsSprite"?nativeVector2Constructor():nativeSphereConstructor()).get("prototype");const auto key=name=="intersectsSprite"?"distanceTo":"copy";changes.push_back({prototype,key,prototype.getOwnPropertyDescriptor(key)});observe(s,prototype,key,std::string("prototype.")+key);
            const auto spherePrototype=nativeSphereConstructor().get("prototype");changes.push_back({spherePrototype,"applyMatrix4",spherePrototype.getOwnPropertyDescriptor("applyMatrix4")});observe(s,spherePrototype,"applyMatrix4","prototype.applyMatrix4");args={input};
        }else if(name=="clone"){
            auto target=JSValue::object();ref(s,target,"cloneTarget");method(s,target,"copy","cloneTarget.copy",request.get("result"));auto constructor=JSValue::constructor(+[](const JSValue&,const Args&,const Args& captures)->JSValue{event(captures[0],"call:constructor");return captures[1];},{s,target},"Custom",0,JSValue::ConstructorKind::Callable);getter(s,self,"constructor","self.constructor",constructor);
        }
    }else{
        getter(s,self,"_frustums","self._frustums",pool);cell(s,self,"coordinateSystem","self.coordinateSystem",2000);
        const auto counts=request.hasOwn("counts")?request.get("counts"):array({3});sequenceGetter(s,self,"_count","self._count",counts);
        for(unsigned i=0;i<3;++i){auto frustum=JSValue::object();const auto label="f"+std::to_string(i);ref(s,frustum,label);getter(s,pool,JSValue(i),"pool."+std::to_string(i),frustum);const auto results=request.get("results");for(const auto* key:{"intersectsObject","intersectsSprite","intersectsSphere","intersectsBox","containsPoint"})method(s,frustum,key,label+"."+key,decode(results.isObject()?results.get(JSValue(i)):JSValue{},s));method(s,frustum,"copy",label+".copy",{},true);method(s,frustum,"setFromProjectionMatrix",label+".setFromProjectionMatrix",{},true);}
        if(name=="copy"){
            auto source=JSValue::object(),sourcePool=JSValue::object();ref(s,source,"source");ref(s,sourcePool,"sourcePool");getter(s,source,"coordinateSystem","source.coordinateSystem",2001);getter(s,source,"_frustums","source._frustums",sourcePool);sequenceGetter(s,source,"_count","source._count",counts);for(unsigned i=0;i<3;++i){auto item=JSValue::object();ref(s,item,"source"+std::to_string(i));getter(s,sourcePool,JSValue(i),"sourcePool."+std::to_string(i),item);}args={source};
        }else if(name=="setFromArrayCamera"){
            auto input=JSValue::object(),cameras=JSValue::object();ref(s,input,"cameraArray");ref(s,cameras,"cameras");getter(s,input,"cameras","cameraArray.cameras",cameras);sequenceGetter(s,cameras,"length","cameras.length",request.hasOwn("lengths")?request.get("lengths"):array({2}));
            for(unsigned i=0;i<3;++i){auto camera=JSValue::object();const auto label="camera"+std::to_string(i);ref(s,camera,label);getter(s,cameras,JSValue(i),"cameras."+std::to_string(i),camera);getter(s,camera,"projectionMatrix",label+".projectionMatrix",matrix({}));getter(s,camera,"matrixWorldInverse",label+".matrixWorldInverse",matrix({}));getter(s,camera,"coordinateSystem",label+".coordinateSystem",2001);getter(s,camera,"reversedDepth",label+".reversedDepth",i%2==0);}
            const auto prototype=nativeMatrix4Constructor().get("prototype");changes.push_back({prototype,"multiplyMatrices",prototype.getOwnPropertyDescriptor("multiplyMatrices")});observe(s,prototype,"multiplyMatrices","prototype.multiplyMatrices");args={input};
        }else if(name=="clone"){
            self=JSValue::object();ref(s,self,"cloneSelf");getter(s,self,"constructor","mustNotRead.constructor",1);getter(s,self,"coordinateSystem","cloneSelf.coordinateSystem",2001);getter(s,self,"_frustums","cloneSelf._frustums",array());getter(s,self,"_count","cloneSelf._count",0);
        }else args={point};
    }
    own(s,"active",true);own(out,"result",attempt(s,[&]{return ctor(request).get("prototype").get(name).call(self,args);}));own(s,"active",false);restore(changes);own(out,"events",s.get("events"));return out;
}
JSValue construction(const JSValue& request){
    auto s=state(request),constructor=ctor(request),out=JSValue::object(),prototype=JSValue::object();Args args;const auto raw=request.get("args");for(unsigned i=0;i<raw.length();++i)args.push_back(decode(raw.get(JSValue(i)),s));
    JSValue target;const bool custom=request.get("newTarget").toBoolean();if(custom){
        for(const auto* key:request.get("family").strictEquals("array")?std::initializer_list<const char*>{"coordinateSystem","_frustums","_count"}:std::initializer_list<const char*>{"planes"}){JSPropertyDescriptor d;d.configurable=true;d.setter=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{event(captures[0],"set:target."+captures[1].string(),{arg(args,0)});return {};},{s,key},std::string("set ")+key,1);prototype.defineProperty(key,d);}
        auto original=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Target",0,JSValue::ConstructorKind::Callable),handler=JSValue::object();own(handler,"get",JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{const auto key=arg(args,1);if(key.strictEquals("prototype")){event(captures[0],"get:newTarget.prototype");return captures[1];}return arg(args,0).get(key,arg(args,2));},{s,prototype},"get",3));target=JSValue::proxy(original,handler);
    }
    own(s,"active",true);try{const auto result=request.get("call").toBoolean()?constructor.call(nullptr,args):custom?constructor.construct(args,target):constructor.construct(args);own(s,"active",false);own(out,"result",snapshot(result));if(custom)own(out,"prototypeIdentity",result.getPrototype().sameValue(prototype));}catch(...){own(s,"active",false);own(out,"throw",caught(s,std::current_exception()));}own(out,"events",s.get("events"));return out;
}
JSValue run(const JSValue& request){const auto kind=request.get("kind");if(kind.strictEquals("metadata"))return metadata(request);if(kind.strictEquals("numeric"))return numeric(request);if(kind.strictEquals("arrayNumeric"))return arrayNumeric(request);if(kind.strictEquals("construct"))return construction(request);return protocol(request);}
JsonValue fresh(){
    JsonValue::Array results;for(unsigned pass=0;pass<3;++pass){JSValue::nativeSafepoint();if(JSValue::hasActiveHeap())throw std::logic_error("prior realm retained");JSWeakValue weak;
        {auto module=nativeFrustumModule(),arrayModule=nativeFrustumArrayModule(),frustum=nativeFrustumConstructor().construct(),array=nativeFrustumArrayConstructor().construct();weak=JSWeakValue(frustum);const auto plane=frustum.get("planes").get("0");module.collectGarbage();if(!plane.get("normal").get("x").strictEquals(1))throw std::logic_error("lost default plane");auto clone=call(frustum,"clone");if(clone.get("planes").get("0").sameValue(plane))throw std::logic_error("aliased Frustum clone");const auto constructor=nativeFrustumConstructor();module.set("Frustum",nullptr);if(!nativeFrustumConstructor().sameValue(constructor))throw std::logic_error("facade modified lexical constructor");const auto arrayConstructor=nativeFrustumArrayConstructor();arrayModule.set("FrustumArray",nullptr);if(!nativeFrustumArrayConstructor().sameValue(arrayConstructor))throw std::logic_error("facade modified lexical array constructor");}
        JSValue::nativeSafepoint();if(!weak.expired()||JSValue::hasActiveHeap())throw std::logic_error("Frustum realm leaked");results.emplace_back(true);
    }return results;
}
JSValue freshImport(const std::string& family){
    const auto vector2=nativeVector2Constructor(),vector3=nativeVector3Constructor(),sphere=nativeSphereConstructor(),plane=nativePlaneConstructor(),matrix=nativeMatrix4Constructor();
    auto s=state(JSValue::object());
    for(const auto& pair:std::initializer_list<std::pair<JSValue,const char*>>{{vector2,"Vector2"},{vector3,"Vector3"},{matrix,"Matrix4"}}){auto prototype=pair.first.get("prototype");const auto key=std::string(pair.second)=="Matrix4"?"elements":"x";JSPropertyDescriptor d;d.configurable=true;d.setter=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue{event(captures[0],"set:"+captures[1].string(),{arg(args,0)});return {};},{s,std::string(pair.second)+"."+key},"setter",1);prototype.defineProperty(key,d);}
    own(s,"active",true);const auto module=family=="array"?nativeFrustumArrayModule():nativeFrustumModule();own(s,"active",false);auto out=JSValue::object(),exports=array();for(const auto& key:module.keys())push(exports,key);own(out,"events",s.get("events"));own(out,"exports",exports);return out;
}
}
int main(int argc,char** argv){
    if(argc>1&&std::string(argv[1])=="--fresh-gc"){try{std::cout<<fresh().dump()<<'\n';return 0;}catch(const std::exception& exception){std::cerr<<exception.what()<<'\n';return 1;}}
    if(argc>2&&std::string(argv[1])=="--fresh-import"){try{std::cout<<freshImport(argv[2]).toJSON()->dump()<<'\n';return 0;}catch(const std::exception& exception){std::cerr<<exception.what()<<'\n';return 1;}}
    std::string line;while(std::getline(std::cin,line)){try{const auto result=run(Animation_detail::parseJSON(line));std::cout<<result.toJSON()->dump()<<'\n';result.collectGarbage();}catch(const std::exception& exception){auto out=JSValue::object();own(out,"probeError",exception.what());std::cout<<out.toJSON()->dump()<<'\n';}}
}
