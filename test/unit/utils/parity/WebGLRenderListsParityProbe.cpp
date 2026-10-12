#include "renderers/webgl/WebGLRenderLists.h"
#include "core/JSWeakMap.h"
#include <cmath>
#include <iostream>
#include <thread>
#include <exception>
namespace {
using Args=std::vector<JSValue>;using O=JsonValue::Object;using A=JsonValue::Array;
void append(JSValue a,Args values){a.get("push").call(a,values);}
JSValue tuple(Args values){auto a=JSValue::array();append(a,values);return a;}
JsonValue encode(const JSValue&v){switch(v.type()){
case JSValue::Type::Undefined:return "@undefined";case JSValue::Type::Null:return nullptr;case JSValue::Type::Boolean:return v.toBoolean();case JSValue::Type::String:return v.string();case JSValue::Type::Function:return "@function";
case JSValue::Type::Number:{auto n=v.number();if(std::isnan(n))return "@NaN";if(std::isinf(n))return n<0?"@-Infinity":"@Infinity";if(n==0&&std::signbit(n))return "@-0";return n;}
case JSValue::Type::Array:{A a;for(unsigned i=0;i<v.length();++i)a.push_back(encode(v.get(std::to_string(i))));return a;}
default:{O o;for(auto&k:v.keys())o[k]=encode(v.get(k));return o;}}}
std::string attempt(const auto& op){try{op();return "";}catch(const JSTypeError&){return "TypeError";}catch(const JSRangeError&){return "RangeError";}catch(...){return "Error";}}
JSValue object(int id,int order=0,int variant=0){auto o=JSValue::object();o.set("id",id);o.set("renderOrder",order);if(variant&2)o.set("isInstancedMesh",true);if(variant&1)o.set("isSkinnedMesh",true);return o;}
JSValue material(int id,JSValue transmission=0,JSValue transparent=false){auto m=JSValue::object();m.set("id",id);m.set("transmission",transmission);m.set("transparent",transparent);return m;}
JSValue camera(JSValue reversed=false){auto o=JSValue::object();o.set("reversedDepth",reversed);return o;}
JsonValue item(const JSValue&v){if(v.isUndefined()||v.isNull())return encode(v);O o;for(auto key:{"id","materialVariant","groupOrder","renderOrder","z","group"})o[key]=encode(v.get(key));o["object"]=encode(v.get("object").isNull()?JSValue(nullptr):v.get("object").get("id"));o["geometry"]=encode(v.get("geometry"));o["material"]=encode(v.get("material").isNull()?JSValue(nullptr):v.get("material").get("id"));return o;}
JsonValue items(JSValue array){A result;for(unsigned i=0;i<array.length();++i)result.push_back(item(array.get(std::to_string(i))));return result;}
JsonValue state(const WebGLRenderList&list){return O{{"opaque",items(list.opaque)},{"transmissive",items(list.transmissive)},{"transparent",items(list.transparent)}};}
JsonValue basic(int mode){WebGLRenderList list;const auto a=list.opaque.value(),t=list.transmissive.value(),r=list.transparent.value();auto cam=camera(mode==1?JSValue(true):mode==2?JSValue(1):JSValue(false));
 for(int i=0;i<10;++i){auto o=object(i,9-i,i%4);auto m=material(i%3,i%3==1?JSValue("0.5"):JSValue(0),i%3==2?JSValue(true):JSValue(1));if(i%4==0)list.unshift(o,i,m,i%2,i==0?-0.:double(i),i);else list.push(o,i,m,i%2,i==1?-0.:double(i),i,cam);}
 auto before=state(list);auto first=a.get("0"),unused=a.get("1");auto finish=attempt([&]{list.finish();});list.init();const auto empty=state(list);list.push(object(99),99,material(7),3,-0.,99,cam);const auto reused=list.opaque.get("0").strictEquals(first);list.finish();auto after=state(list);
 A names;for(auto&key:list.object().keys())names.emplace_back(key);O methods;for(auto key:{"init","push","unshift","finish","sort"}){auto f=list.object().get(key);methods[key]=O{{"name",f.get("name").string()},{"length",f.get("length").number()}};}
 return O{{"kind","basic"},{"mode",mode},{"before",before},{"finish",finish},{"empty",empty},{"reusedFirst",reused},{"arrays",A{a.strictEquals(list.opaque),t.strictEquals(list.transmissive),r.strictEquals(list.transparent)}},{"unused",item(unused)},{"after",after},{"names",names},{"methods",methods}};}
JSValue comparison(JSValue events,JSValue list,int mode){auto flag=JSValue::object();flag.set("first",true);return JSValue::function([](const JSValue& self,const Args&a,const Args&c)->JSValue{
 append(c[0],{tuple({a[0].get("id"),a[1].get("id"),self.isUndefined()})});auto flag=c[3];bool first=flag.get("first").toBoolean();flag.set("first",false);const int mode=int(c[2].toNumber());
 if(mode==3&&first)throw JSRangeError("compare");if(mode==4&&first)c[1].get("init").call(c[1]);
 if(mode==5&&first)c[1].get("push").call(c[1],{object(99),99,material(9),0,-100,99,camera()});
 if(mode==6&&first)c[1].get("sort").call(c[1]);if(mode==2)return NAN;
 return a[1].get("id").toNumber()-a[0].get("id").toNumber();},{events,list,mode,flag});}
JsonValue sorted(int count,int mode){WebGLRenderList list;auto cam=camera(mode%2==1);auto events=JSValue::array();
 for(int i=0;i<count;++i){auto m=material((i*7)%5,i%3==1?.5:0,i%3==2);list.push(object(i,(i*3)%4,i%4),i,m,(i*11)%3,mode==7&&i%5==0?JSValue(NAN):JSValue((i*13)%7),i,cam);}
 JSValue custom=mode==0||mode==7?JSValue():comparison(events,list.object(),mode);std::string err=attempt([&]{list.sort(custom,custom);});return O{{"kind","sort"},{"count",count},{"mode",mode},{"error",err},{"state",state(list)},{"events",encode(events)}};}
void access(JSValue o,const char* key,JSValue events,JSValue value,JSValue list={},int action=0){JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function([](const JSValue&self,const Args&,const Args&c)->JSValue{
 append(c[0],{tuple({c[1],self.strictEquals(c[3])})});if(c[5].strictEquals(JSValue(1)))throw JSRangeError("get");if(c[5].strictEquals(JSValue(2)))c[4].get("init").call(c[4]);return c[2];},{events,key,value,o,list,action});o.defineProperty(key,d);}
JsonValue dynamic(int mode){WebGLRenderList list;auto events=JSValue::array(),o=object(1),m=material(1),cam=camera();JSValue saved;auto opaque=list.opaque.value();std::string err;
 if(mode<=5){for(auto key:{"id","isInstancedMesh","isSkinnedMesh","renderOrder"})access(o,key,events,o.get(key),list.object(),mode==1&&std::string(key)=="renderOrder"?1:mode==2&&std::string(key)=="id"?2:0);access(cam,"reversedDepth",events,mode==3?JSValue(true):JSValue(false));access(m,"transmission",events,mode==4?JSValue(1):JSValue(0));access(m,"transparent",events,true);err=attempt([&]{list.push(o,2,m,0,3,4,cam);});}
 else if(mode==6){list.opaque=JSValue::array();err=attempt([&]{list.object().get("push").call(17,{o,2,m,0,3,4,cam});});}
 else if(mode==7||mode==8){auto fn=JSValue::function([](const JSValue&self,const Args&a,const Args&c)->JSValue{append(c[0],{tuple({"append",self.strictEquals(c[1]),a[0].get("id")})});if(c[2].toBoolean())throw JSRangeError("push");return 9;},{events,opaque,mode==8});access(opaque,"push",events,fn);err=attempt([&]{list.push(o,2,m,0,3,4,cam);});}
 else if(mode==9||mode==10){list.push(o,2,m,0,3,4,cam);saved=opaque.get("0");list.init();access(saved,"id",events,1);err=attempt([&]{if(mode==9)list.push(o,2,m,0,3,4,cam);else list.finish();});}
 else if(mode==11){list.push(o,2,m,0,3,4,cam);saved=opaque.get("0");list.push(object(2),3,m,0,4,5,cam);auto later=opaque.get("1");list.init();saved.set("id",nullptr);list.finish();saved=later;}
 else if(mode==12){err=attempt([&]{list.push(o,2,m,0,3,4);});}
 else if(mode==13){list.push(o,2,m,0,3,4,cam);saved=opaque.get("0");list.init();o.set("id",7);err=attempt([&]{list.unshift(o,2,m,0,3,4);});}
 else if(mode==14){list.push(o,2,m,0,3,4,cam);list.push(object(2),2,m,0,2,4,cam);auto fn=JSValue::function([](const JSValue&self,const Args&a,const Args&c)->JSValue{append(c[0],{tuple({"sort",self.strictEquals(c[1]),a[0].get("name")})});return {};},{events,opaque});access(opaque,"sort",events,fn);err=attempt([&]{list.sort(false,17);});}
 else if(mode==15){list.push(o,2,m,0,3,4,cam);list.push(object(2),2,m,0,2,4,cam);err=attempt([&]{list.sort(17);});}
 else if(mode==16){list.push(o,2,m,0,3,4,cam);err=attempt([&]{list.sort(17);});}
 return O{{"kind","dynamic"},{"mode",mode},{"error",err},{"state",state(list)},{"captured",items(opaque)},{"saved",item(saved)},{"events",encode(events)}};}
JsonValue cacheCase(int mode){WebGLRenderLists cache;auto scene=JSValue::object();const auto first=cache.get(scene,0);JSValue depth;switch(mode){case 0:depth=0;break;case 1:depth=1;break;case 2:depth=9;break;case 3:depth=-1;break;case 4:depth=.5;break;case 5:depth="0";break;case 6:depth="length";break;case 7:depth=NAN;break;case 8:depth=nullptr;break;case 9:depth="push";break;default:depth=JSValue();}
 JSValue next,again;auto err=attempt([&]{next=cache.get(scene,depth);again=cache.get(scene,depth);});cache.dispose();const auto replaced=cache.get(scene,0);
 auto fresh=JSValue::object();JSValue atHigh=cache.get(fresh,9);return O{{"kind","cache"},{"mode",mode},{"error",err},{"firstSame",next.strictEquals(first)},{"againSame",again.strictEquals(next)},{"nextType",next.isCallable()?"function":next.isObject()?"list":next.isUndefined()?"undefined":next.isNull()?"null":"number"},{"nextScalar",next.isObject()?JsonValue("@object"):encode(next)},{"disposed",!replaced.strictEquals(first)},{"firstHighIsZero",cache.get(fresh,0).strictEquals(atHigh)}};}
JsonValue comparatorCase(bool reverse,int mode,bool getters){
 WebGLRenderList list;auto a=object(1),b=object(2),ma=material(1),mb=material(1),events=JSValue::array();
 for(auto value:{a,b}){value.set("groupOrder",0);value.set("materialVariant",0);value.set("z",0);}a.set("material",ma);b.set("material",mb);
 const auto field=std::vector<std::string>{"groupOrder","renderOrder","material","materialVariant","z","id"}[std::min(mode,5)];
 if(mode<6){if(field=="material")mb.set("id",3);else b.set(field,3);}
 if(mode==6){a.set("z",NAN);b.set("z",NAN);}if(mode==7){a.set("groupOrder",JSValue());b.set("groupOrder",0);}
 if(mode==8){a.set("groupOrder","10");b.set("groupOrder","2");}
 if(mode==9||mode==10){auto v=JSValue::object();v.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args&args,const Args&c)->JSValue{append(c[0],{tuple({"convert",args[0]})});if(c[1].toBoolean())throw JSRangeError("coerce");return 4;},{events,mode==10}));a.set("groupOrder",v);}
 if(mode==11){a.set("material",nullptr);b.set("material",nullptr);}
 if(getters){for(auto value:{a,b})for(auto key:{"groupOrder","renderOrder","material","materialVariant","z","id"})access(value,key,events,value.get(key));for(auto value:{ma,mb})access(value,"id",events,value.get("id"));}
 for(int i=0;i<2;++i)list.push(object(i),i,material(i,reverse?1:0),0,i,i,camera());
 auto target=reverse?list.transmissive.value():list.opaque.value();auto captured=JSValue::object();
 target.set("sort",JSValue::function([](const JSValue&,const Args&args,const Args&c)->JSValue{auto o=c[0];o.set("sort",args[0]);return {};},{captured}));list.sort();auto compare=captured.get("sort");JSValue result;const auto error=attempt([&]{result=compare.call(JSValue(),{a,b});});
 return O{{"kind","compare"},{"reverse",reverse},{"mode",mode},{"getters",getters},{"error",error},{"result",encode(result)},{"events",encode(events)}};
}
JsonValue cacheDynamic(int mode){
 WebGLRenderLists cache;auto scene=JSValue::object(),events=JSValue::array(),trace=JSValue::object();
 auto proto=nativeWeakMapConstructor().get("prototype");const auto originalGet=proto.get("get"),originalSet=proto.get("set");
 JSValue first=cache.get(scene,0),second,result;std::string error;
 if(mode<4){auto depth=JSValue::object();depth.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args&a,const Args&c)->JSValue{append(c[0],{tuple({"depth",a[0]})});if(c[2].strictEquals(JSValue(2)))throw JSRangeError("depth");if(c[2].strictEquals(JSValue(3)))c[1].get("dispose").call(c[1]);return c[2].strictEquals(JSValue(1))?JSValue("length"):JSValue(0);},{events,cache.object(),mode}));error=attempt([&]{result=cache.get(scene,depth);});}
 else if(mode<8){auto fake=JSValue::object();fake.set("length",mode==6?JSValue("2"):JSValue(0));fake.set("0",123);fake.set("push",JSValue::function([](const JSValue&self,const Args&a,const Args&c)->JSValue{append(c[0],{tuple({"push",self.strictEquals(c[1]),a[0].get("opaque").length()})});if(c[2].toBoolean())throw JSRangeError("push");return 0;},{events,fake,mode==7}));
 proto.set("get",JSValue::function([](const JSValue&self,const Args&a,const Args&c)->JSValue{append(c[0],{tuple({"get",JSWeakMap::isMap(self),a[0].strictEquals(c[1])})});return c[2];},{events,scene,fake}));error=attempt([&]{result=cache.get(scene,mode==6?JSValue("10"):JSValue(0));});}
 else if(mode<11){proto.set("get",JSValue::function([](const JSValue&self,const Args&a,const Args&c)->JSValue{append(c[0],{tuple({"get",JSWeakMap::isMap(self),a[0].strictEquals(c[1])})});if(c[3].strictEquals(JSValue(8)))c[2].get("dispose").call(c[2]);return {};},{events,scene,cache.object(),mode}));
 proto.set("set",JSValue::function([](const JSValue&self,const Args&a,const Args&c)->JSValue{append(c[0],{tuple({"set",JSWeakMap::isMap(self),a[0].strictEquals(c[1]),a[1].length()})});if(c[4].strictEquals(JSValue(10)))throw JSRangeError("set");if(c[4].strictEquals(JSValue(9)))c[2].get("dispose").call(c[2]);return c[3].call(self,a);},{events,scene,cache.object(),originalSet,mode}));error=attempt([&]{result=cache.get(scene,0);});}
 else {error=attempt([&]{result=cache.get(mode==11?JSValue(17):mode==12?JSValue(nullptr):JSValue(),0);});}
 proto.set("get",originalGet);proto.set("set",originalSet);second=cache.get(scene,0);
 return O{{"kind","cacheDynamic"},{"mode",mode},{"error",error},{"firstSame",result.strictEquals(first)},{"secondSame",result.strictEquals(second)},{"result",result.isObject()?JsonValue("@object"):encode(result)},{"events",encode(events)}};
}

JsonValue names(const JSValue& value){A a;for(const auto& key:value.ownPropertyNames())a.emplace_back(key);return a;}
JsonValue descriptor(const JSValue& value,const char* key){auto d=value.getOwnPropertyDescriptor(key);if(!d)return nullptr;return O{{"writable",d->writable.value_or(false)},{"enumerable",d->enumerable.value_or(false)},{"configurable",d->configurable.value_or(false)}};}
JsonValue functionConstruction(int index,int mode){
 WebGLRenderList list;WebGLRenderLists cache;auto scene=JSValue::object(),first=cache.get(scene,0);
 for(int i=0;i<4;++i)list.push(object(i),i,material(i,i>=2?1:0),0,4-i,i,camera());
 auto captured=JSValue::object();for(auto key:{"opaque","transmissive"}){auto array=list.object().get(key),original=array.get("sort");array.set("sort",JSValue::function([](const JSValue&,const Args&a,const Args&c)->JSValue{auto value=c[0];value.set(c[1].string(),a[0]);return {};},{captured,key}));list.sort();array.set("sort",original);}
 JSValue fn;Args args;
 if(index<5){fn=list.object().get(std::vector<std::string>{"init","push","unshift","finish","sort"}[index]);if(index==1||index==2){args={object(9),9,material(9),0,9,9};if(index==1)args.push_back(camera());}if(index==3)list.init();}
 else if(index<7){fn=cache.object().get(index==5?"get":"dispose");if(index==5)args={scene,0};}
 else{auto array=index==7?list.opaque.value():list.transmissive.value();fn=captured.get(index==7?"opaque":"transmissive");args={array.get("0"),array.get("1")};}
 auto proto=fn.get("prototype");const O meta{{"names",names(fn)},{"name",encode(fn.get("name"))},{"length",encode(fn.get("length"))},{"nameDescriptor",descriptor(fn,"name")},{"lengthDescriptor",descriptor(fn,"length")},{"prototypeDescriptor",descriptor(fn,"prototype")},{"prototypeNames",names(proto)},{"constructorDescriptor",descriptor(proto,"constructor")},{"constructorLink",proto.get("constructor").strictEquals(fn)},{"constructible",fn.isConstructible()}};
 auto alternate=JSValue::object();alternate.set("marker",7);auto target=JSValue::constructor([](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("newTarget must not execute");},{},"Target",0,JSValue::ConstructorKind::Callable);
 if(mode==1)target.set("prototype",alternate);if(mode==2)target.set("prototype",17);if(mode==3)fn.set("prototype",alternate);if(mode==4)target=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return {};});if(mode==5)args.clear();if(mode==6)fn.set("prototype",nullptr);if(mode==7&&index==5)args={scene,"length"};
 JSValue result;const auto error=attempt([&]{result=(mode==1||mode==2||mode==4)?fn.construct(args,target):fn.construct(args);});
 auto resultProto=result.isObject()?result.getPrototype():JSValue();auto objectProto=JSValue::object().getPrototype();
 const O output{{"kind","construction"},{"index",index},{"mode",mode},{"meta",meta},{"error",error},{"objectResult",result.isObject()},{"firstList",result.strictEquals(first)},{"ownPrototype",result.isObject()&&resultProto.strictEquals(fn.get("prototype"))},{"alternatePrototype",result.isObject()&&resultProto.strictEquals(alternate)},{"objectPrototype",result.isObject()&&resultProto.strictEquals(objectProto)},{"resultKeys",result.isObject()?names(result):JsonValue("@undefined")},{"state",state(list)},{"cacheSame",cache.get(scene,0).strictEquals(first)}};
 if(mode==3||mode==6)fn.set("prototype",proto);return output;
}

JSValue defaultComparator(bool reverse){
 WebGLRenderList list;for(int i=0;i<2;++i)list.push(object(i),i,material(i,reverse?1:0),0,i,i,camera());auto array=reverse?list.transmissive.value():list.opaque.value();auto holder=JSValue::object();array.set("sort",JSValue::function([](const JSValue&,const Args&a,const Args&c)->JSValue{auto value=c[0];value.set("fn",a[0]);return {};},{holder}));list.sort();return holder.get("fn");
}
JsonValue modulePersistence(bool reverse){
 auto anchor=JSValue::object();JSWeakValue old;{auto fn=defaultComparator(reverse);fn.set("moduleMarker",123);auto prototype=fn.get("prototype");prototype.set("moduleMarker",456);old=JSWeakValue(fn);}anchor.collectGarbage();
 auto fn=defaultComparator(reverse);auto locked=old.lock();O result{{"kind","modulePersistence"},{"reverse",reverse},{"identity",locked&&locked->strictEquals(fn)},{"functionMarker",encode(fn.get("moduleMarker"))},{"prototypeMarker",encode(fn.get("prototype").get("moduleMarker"))}};
 fn.erase("moduleMarker");auto prototype=fn.get("prototype");prototype.erase("moduleMarker");return result;
}
void nativeModuleLifetimeRegression(){
 std::exception_ptr error;std::thread worker([&]{try{
  JSWeakValue old[2];{auto anchor=JSValue::object();for(int i=0;i<2;++i){auto fn=defaultComparator(i!=0);fn.set("moduleMarker",123);old[i]=JSWeakValue(fn);}anchor.collectGarbage();for(int i=0;i<2;++i){auto fn=defaultComparator(i!=0);if(fn.get("moduleMarker").number()!=123||old[i].expired())throw std::runtime_error("Module lost during same-heap GC");}}
  for(int i=0;i<2;++i)if(!old[i].expired())throw std::runtime_error("Module cache owns heap lifetime");
  for(int i=0;i<2;++i){auto fn=defaultComparator(i!=0);if(!fn.get("moduleMarker").isUndefined())throw std::runtime_error("New heap inherited old module mutation");}
 }catch(...){error=std::current_exception();}});worker.join();if(error)std::rethrow_exception(error);
}

}
int main(){nativeModuleLifetimeRegression();A output;for(int m=0;m<3;++m)output.push_back(basic(m));for(int n:{0,1,3,6,21,64,257})for(int m=0;m<8;++m)output.push_back(sorted(n,m));for(int m=0;m<17;++m)output.push_back(dynamic(m));for(int m=0;m<11;++m)output.push_back(cacheCase(m));for(bool reverse:{false,true})for(int m=0;m<12;++m)for(bool getters:{false,true})output.push_back(comparatorCase(reverse,m,getters));for(int m=0;m<14;++m)output.push_back(cacheDynamic(m));for(int index=0;index<9;++index)for(int mode=0;mode<8;++mode)output.push_back(functionConstruction(index,mode));for(bool reverse:{false,true})output.push_back(modulePersistence(reverse));std::cout<<JsonValue(output).dump()<<'\n';}
