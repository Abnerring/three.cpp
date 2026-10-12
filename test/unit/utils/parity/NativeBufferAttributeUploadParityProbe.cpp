#include "core/BufferAttribute.h"
#include "core/NativeObject.h"
#include "core/NativeArray.h"
#include "renderers/webgl/WebGLAttributes.h"
#include <iostream>
#include <cmath>
namespace {
using Args=std::vector<JSValue>;using O=JsonValue::Object;using A=JsonValue::Array;
JSValue call(JSValue self,const char* method,const Args& args={}){return self.get(method).call(self,args);}
void data(JSValue self,const std::string& key,JSValue value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;self.defineProperty(key,d);}
JSValue array(const Args& values={}){auto a=JSValue::array();for(std::size_t i=0;i<values.size();++i)a.set(std::to_string(i),values[i]);return a;}
void push(JSValue a,const JSValue& value){a.set(std::to_string(a.length()),value);}
void log(JSValue state,Args values){push(state.get("trace"),array(values));}
JsonValue encode(const JSValue& value){
 if(value.isUndefined())return "@undefined";if(value.isNull())return nullptr;if(value.isCallable())return "@function";
 if(value.type()==JSValue::Type::Boolean)return value.toBoolean();if(value.type()==JSValue::Type::String)return value.string();
 if(value.type()==JSValue::Type::Number){auto n=value.number();if(std::isnan(n))return "@NaN";if(n==0&&std::signbit(n))return "@-0";if(!std::isfinite(n))return n<0?"-Infinity":"Infinity";return n;}
 if(value.isHostObject("three.AnimationArray")){const auto native=NativeArray_detail::unwrap(value);A result;for(std::size_t i=0;i<native.size();++i)result.push_back(encode(native.get(double(i))));return O{{"typed",value.get("constructor").get("name").string()},{"values",result}};}
 if(value.type()==JSValue::Type::Array){A result;for(unsigned i=0;i<value.length();++i)result.push_back(encode(value.get(std::to_string(i))));return result;}
 O result;for(const auto& key:value.keys())result[key]=encode(value.get(key));return result;
}
JSValue frozen(const JSValue& value){return JSValue::fromJSON(encode(value));}
template<class F>JsonValue attempt(F&& fn){try{return O{{"value",encode(fn())}};}catch(const JSTypeError&){return O{{"error","TypeError"}};}catch(const JSRangeError&){return O{{"error","RangeError"}};}catch(const std::exception&){return O{{"error","Error"}};}}
std::shared_ptr<BufferAttribute> native(JSValue attribute){return std::dynamic_pointer_cast<BufferAttribute>(vertexAttribute(attribute));}
JSValue replacement(bool half,int value){return nativeArray(Uint16Array(std::vector<std::uint16_t>(half?8:16,static_cast<std::uint16_t>(value))));}
JSValue bytes(JSValue source,double start=0,double count=-1){const auto elementSize=source.get("BYTES_PER_ELEMENT").toNumber(),offset=source.get("byteOffset").toNumber();if(count<0)count=source.get("length").toNumber()-start;
 return NativeArray_detail::constructor(1).construct({source.get("buffer"),offset+start*elementSize,count*elementSize});}
JSValue replacementUpload(const JSValue& self,const Args&,const Args& c){auto state=c[0];const auto calls=state.get("calls").number()+1;state.set("calls",calls);log(state,{"replacement",self.sameValue(state.get("attribute")),calls});return {};}
JSValue upload(const JSValue& self,const Args&,const Args& c){
 auto state=c[0],attribute=state.get("attribute");const auto owner=native(attribute);auto calls=state.get("calls").number()+1;state.set("calls",calls);
 log(state,{"upload",self.sameValue(attribute),calls,attribute.get("version"),attribute.get("updateRanges").get("length"),attribute.get("array").get("constructor").get("name")});if(state.get("busy").toBoolean())return {};
 const int behavior=int(state.get("behavior").number());const bool half=state.get("half").toBoolean();
 if(behavior==1)owner->array=Uint16Array(std::vector<std::uint16_t>(half?8:16,9));
 if(behavior==2)owner->array=Float32Array(std::vector<float>(9,10));
 if(behavior==3)owner->version=attribute.get("version").toNumber()+2;
 if(behavior==4)owner->isFloat16BufferAttribute=true;
 if(behavior==5)attribute.set("onUploadCallback",nullptr);
 if(behavior==6)throw JSRangeError("upload");
 if(behavior==7)call(state.get("manager"),"remove",{attribute});
 if(behavior==8){state.set("busy",true);call(state.get("manager"),"update",{attribute,77});state.set("busy",false);}
 if(behavior==9)owner->dispose();
 if(behavior==10)attribute.set("onUploadCallback",JSValue::function(replacementUpload,{state}));
 if(behavior==11){owner->array=Uint16Array(std::vector<std::uint16_t>(half?8:16,11));owner->needsUpdate=true;}
 if(behavior==12){owner->isFloat16BufferAttribute=true;owner->version=attribute.get("version").toNumber()+.5;}
 if(behavior==13){owner->clearUpdateRanges();owner->addUpdateRange(6,1);}
 if(behavior==14){attribute.set("onUploadCallback",JSValue::function(replacementUpload,{state}));state.set("busy",true);call(state.get("manager"),"update",{attribute,77});state.set("busy",false);}
 if(behavior==15)owner->dispose();
 return {};
}
JSValue gpu(const JSValue& self,const Args& args,const Args& c){
 auto state=c[0];const auto method=c[1].string();const bool receiver=self.sameValue(state.get("gl"));auto storage=state.get("storage");
 if(method=="createBuffer"){log(state,{"create",receiver});auto next=state.get("next").number()+1;state.set("next",next);return next;}
 if(method=="bindBuffer"){log(state,{"bind",receiver,args[0],args[1]});state.set("bound",args[1]);return {};}
 if(method=="bufferData"){log(state,{"data",receiver,args[0],frozen(args[1]),args[2]});const auto input=NativeArray_detail::unwrap(bytes(args[1]));auto result=JSValue::array();for(std::size_t i=0;i<input.size();++i)push(result,input.get(double(i)));storage.set(state.get("bound").toString(),result);return {};}
 if(method=="bufferSubData"){
  const auto start=args.size()>3?args[3]:JSValue(),count=args.size()>4?args[4]:JSValue();log(state,{"sub",receiver,args[0],args[1],frozen(args[2]),frozen(start),frozen(count)});
  const auto calls=state.get("subCalls").number()+1;state.set("subCalls",calls);if(state.get("subThrow").strictEquals(JSValue(calls)))throw JSRangeError("sub");
  const auto first=start.isUndefined()?0:start.toNumber(),n=count.isUndefined()||count.strictEquals(JSValue(0))?args[2].get("length").toNumber()-first:count.toNumber();
  const auto input=NativeArray_detail::unwrap(bytes(args[2],first,n));auto dest=storage.get(state.get("bound").toString());for(std::size_t i=0;i<input.size();++i)dest.set(JSValue(args[1].toNumber()+double(i)).toString(),input.get(double(i)));
  const auto after=state.get("onSub");if(after.isCallable())after.call(JSValue(),{calls});return {};
 }
 log(state,{"delete",receiver,args[0]});storage.erase(args[0].toString());auto after=state.get("onDelete");if(after.isCallable())after.call(JSValue());return {};
}
struct Fixture{
 std::shared_ptr<BufferAttribute> owner;JSValue attribute,state=JSValue::object(nullptr),gl=JSValue::object(),manager;
 explicit Fixture(bool half=false){
  owner=half?std::make_shared<BufferAttribute>(Uint16Array({1,2,3,4,5,6,7,8}),2):std::make_shared<BufferAttribute>(Float32Array({1,2,3,4,5,6,7,8}),2);attribute=nativeAttribute(owner);
  data(state,"attribute",attribute);data(state,"trace",JSValue::array());data(state,"storage",JSValue::object());data(state,"gl",gl);data(state,"next",0);data(state,"subCalls",0);data(state,"behavior",0);data(state,"busy",false);data(state,"calls",0);data(state,"half",half);
  for(const auto* method:{"createBuffer","bindBuffer","bufferData","bufferSubData","deleteBuffer"})data(gl,method,JSValue::function(gpu,{state,method}));
  const char* names[]={"FLOAT","HALF_FLOAT","UNSIGNED_SHORT","SHORT","UNSIGNED_INT","INT","BYTE","UNSIGNED_BYTE"};const int values[]={5126,5131,5123,5122,5125,5124,5120,5121};for(int i=0;i<8;++i)data(gl,names[i],values[i]);
  manager=WebGLAttributes(gl).object();data(state,"manager",manager);data(state,"upload",JSValue::function(upload,{state}));data(state,"baseClear",attribute.get("clearUpdateRanges"));
  // The source is a real C++ BufferAttribute and uses its native callback API.
  owner->onUpload([this]{upload(attribute,{},Args{state});});
 }
 JSValue update(){return call(manager,"update",{attribute,34962});}
 JsonValue cache(){return encode(call(manager,"get",{attribute}));}
 JsonValue trace(){return encode(state.get("trace"));}
 JsonValue gpuState(){A result;auto storage=state.get("storage");for(const auto& key:storage.keys())result.push_back(O{{"buffer",JSValue(key).toNumber()},{"bytes",encode(storage.get(key))}});return result;}
};
JsonValue nativeUpload(int behavior,bool updating){
 Fixture f(behavior==4||behavior==12);auto a=f.attribute;f.owner->addUpdateRange(0,1);JsonValue initial="not-run";if(updating){initial=attempt([&]{return f.update();});f.state.get("trace").set("length",0);a.get("array").set("0",33);f.owner->needsUpdate=true;}
 if(behavior==15)f.owner->addEventListener("dispose",[&](auto& event){log(f.state,{"dispose-listener",event.target==f.owner.get()});call(f.manager,"remove",{a});});
 const auto original=a.get("array"),ranges=a.get("updateRanges");f.state.set("behavior",behavior);auto result=attempt([&]{return f.update();});const auto after=f.cache(),firstTrace=f.trace(),firstGPU=f.gpuState();
 f.state.set("behavior",0);auto noVersion=attempt([&]{return f.update();});f.owner->needsUpdate=true;auto nextVersion=attempt([&]{return f.update();});
 return O{{"kind","native-upload"},{"behavior",behavior},{"updating",updating},{"initial",initial},{"result",result},{"after",after},{"firstTrace",firstTrace},{"firstGPU",firstGPU},{"noVersion",noVersion},{"nextVersion",nextVersion},{"cache",f.cache()},{"array",encode(a.get("array"))},{"sameArray",a.get("array").sameValue(original)},{"sameRanges",a.get("updateRanges").sameValue(ranges)},{"ranges",encode(a.get("updateRanges"))},{"version",encode(a.get("version"))},{"half",encode(a.get("isFloat16BufferAttribute"))},{"trace",f.trace()},{"gpu",f.gpuState()}};
}
JSValue range(JSValue start,JSValue count){auto result=JSValue::object();data(result,"start",start);data(result,"count",count);return result;}
JSValue sortRanges(const JSValue& self,const Args& args,const Args& c){
 auto state=c[0],attribute=state.get("attribute"),ranges=state.get("ranges");const auto mode=c[1].number();
 if(mode==1){auto handles=state.get("handles");auto compared=args[0].call(JSValue(),{handles.get("0"),handles.get("1")});log(state,{"sort-call",self.sameValue(ranges),compared});}
 if(mode==2){log(state,{"sort-reverse",self.sameValue(ranges)});const auto n=self.length();for(unsigned i=0;i<n/2;++i){auto a=self.get(std::to_string(i)),b=self.get(std::to_string(n-1-i));auto array=self;array.set(std::to_string(i),b);array.set(std::to_string(n-1-i),a);}return self;}
 if(mode==12){log(state,{"sort-replace",self.sameValue(ranges)});attribute.set("updateRanges",JSValue::array());}
 return state.get("baseSort").call(self,args);
}
JSValue clearRanges(const JSValue& self,const Args&,const Args& c){auto state=c[0],attribute=state.get("attribute");const int mode=int(c[1].number());log(state,{mode==3?"clear-noop":mode==4?"clear-throw":"clear-version",self.sameValue(attribute)});if(mode==4)throw JSRangeError("clear");if(mode==6){attribute.set("version",attribute.get("version").toNumber()+2);state.get("baseClear").call(self);}return {};}
JsonValue nativeRanges(int mode){
 Fixture f;auto a=f.attribute;f.update();f.state.get("trace").set("length",0);for(int i=0;i<8;++i)a.get("array").set(std::to_string(i),8-i);f.owner->addUpdateRange(6,1);f.owner->addUpdateRange(1,2);f.owner->addUpdateRange(3,2);
 auto ranges=a.get("updateRanges"),handles=array({ranges.get("0"),ranges.get("1"),ranges.get("2")});data(f.state,"ranges",ranges);data(f.state,"handles",handles);data(f.state,"baseSort",ranges.get("sort"));
 if(mode==1){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function([](const JSValue&,const Args&,const Args&c)->JSValue{log(c[0],{"sort-get"});return JSValue::function(sortRanges,{c[0],1});},{f.state});ranges.defineProperty("sort",d);}
 if(mode==2||mode==12)ranges.set("sort",JSValue::function(sortRanges,{f.state,mode}));
 if(mode==3||mode==4||mode==6)a.set("clearUpdateRanges",JSValue::function(clearRanges,{f.state,mode}));
 if(mode==5)f.state.set("onSub",JSValue::function([](const JSValue&,const Args&args,const Args&c)->JSValue{if(args[0].strictEquals(JSValue(1))){auto state=c[0],replacement=array({range(0,1)});state.set("replacement",replacement);auto a=state.get("attribute");a.set("updateRanges",replacement);log(state,{"replace-ranges"});}return {};},{f.state}));
 if(mode==7){JSPropertyDescriptor d;d.value=2;d.writable=false;auto item=handles.get("1");item.defineProperty("count",d);}
 if(mode==8){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function([](const JSValue&,const Args&,const Args&c)->JSValue{log(c[0],{"start-get"});return 1;},{f.state});auto item=handles.get("1");item.defineProperty("start",d);}
 if(mode==9){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function([](const JSValue&,const Args&,const Args&c)->JSValue{log(c[0],{"count-get"});return 2;},{f.state});d.setter=JSValue::function([](const JSValue&,const Args&a,const Args&c)->JSValue{log(c[0],{"count-set",a[0]});throw JSRangeError("count");},{f.state});auto item=handles.get("1");item.defineProperty("count",d);}
 if(mode==10||mode==11){ranges.set("length",0);f.owner->addUpdateRange(0,1);f.owner->addUpdateRange(4,1);if(mode==10)f.state.set("subThrow",2);else f.state.set("onSub",JSValue::function([](const JSValue&,const Args&args,const Args&c)->JSValue{if(args[0].strictEquals(JSValue(1))){auto a=c[0].get("attribute");a.set("onUploadCallback",nullptr);}return {};},{f.state}));}
 if(mode==13)a.set("clearUpdateRanges",nullptr);
 f.owner->needsUpdate=true;auto result=attempt([&]{return f.update();});auto cache=f.cache(),snapshot=encode(ranges),handleValues=encode(handles),live=encode(a.get("updateRanges"));
 return O{{"kind","native-ranges"},{"mode",mode},{"result",result},{"cache",cache},{"ranges",snapshot},{"handles",handleValues},{"live",live},{"sameRanges",ranges.sameValue(a.get("updateRanges"))},{"replacementActive",f.state.get("replacement").sameValue(a.get("updateRanges"))},{"version",encode(a.get("version"))},{"trace",f.trace()},{"gpu",f.gpuState()}};
}
JSValue inheritedGetter(const JSValue& self,const Args&,const Args& c){
 auto state=c[0],attribute=state.get("attribute");const int mode=int(c[1].number());
 if(mode==0||mode==1){const auto gets=state.get("gets").number()+1;state.set("gets",gets);log(state,{"array-get",self.sameValue(attribute),gets});if(mode==1&&gets>1)return nativeArray(Float32Array(std::vector<float>(8,9)));return state.get("original");}
 if(mode==2){log(state,{"usage-get",self.sameValue(attribute)});return state.get("originalUsage");}
 if(mode==3){log(state,{"upload-get",self.sameValue(attribute)});return state.get("upload");}
 if(mode==4){log(state,{"ranges-get",self.sameValue(attribute)});return state.get("originalRanges");}
 if(mode==5){const auto gets=state.get("gets").number()+1;state.set("gets",gets);log(state,{"version-get",self.sameValue(attribute),gets});return gets;}
 if(mode==6){log(state,{"clear-get",self.sameValue(attribute)});return JSValue::function([](const JSValue&self,const Args&,const Args&c)->JSValue{log(c[0],{"clear-call",self.sameValue(c[0].get("attribute"))});return c[0].get("baseClear").call(self);},{state});}
 if(mode==7){log(state,{"half-get",self.sameValue(attribute)});return true;}
 log(state,{mode==8?"array-throw":"upload-throw",self.sameValue(attribute)});throw JSRangeError("inherited getter");
}
JsonValue inherited(int mode){
 Fixture f(mode==7);auto a=f.attribute,original=a.get("array"),originalUsage=a.get("usage"),originalRanges=a.get("updateRanges"),proto=JSValue::object(a.getPrototype());a.setPrototype(proto);
 data(f.state,"original",original);data(f.state,"originalUsage",originalUsage);data(f.state,"originalRanges",originalRanges);data(f.state,"gets",0);
 const auto key=std::vector<std::string>{"array","array","usage","onUploadCallback","updateRanges","version","clearUpdateRanges","isFloat16BufferAttribute","array","onUploadCallback"}[mode];if(mode!=6&&mode!=7)a.erase(key);
 JSPropertyDescriptor d;d.getter=JSValue::function(inheritedGetter,{f.state,mode});if(mode==5)d.setter=JSValue::function([](const JSValue&self,const Args&args,const Args&c)->JSValue{log(c[0],{"version-set",self.sameValue(c[0].get("attribute")),args[0]});return {};},{f.state});proto.defineProperty(key,d);
 auto initial=attempt([&]{return f.update();});original.set("0",9);push(originalRanges,range(0,1));f.owner->needsUpdate=true;auto updated=attempt([&]{return f.update();});
 return O{{"kind","native-inherited"},{"mode",mode},{"initial",initial},{"update",updated},{"cache",f.cache()},{"original",encode(original)},{"ranges",encode(originalRanges)},{"trace",f.trace()},{"gpu",f.gpuState()}};
}
JsonValue disposal(int mode){
 Fixture f;auto a=f.attribute;f.update();f.state.get("trace").set("length",0);const auto first=call(f.manager,"get",{a});
 if(mode==1)f.owner->addEventListener("dispose",[&](auto&){call(f.manager,"remove",{a});});
 if(mode==2)f.state.set("onDelete",JSValue::function([](const JSValue&,const Args&,const Args&c)->JSValue{auto state=c[0];if(!state.get("busy").toBoolean()){state.set("busy",true);auto a=state.get("attribute");a.set("needsUpdate",true);call(state.get("manager"),"update",{a,34962});state.set("busy",false);}return {};},{f.state}));
 auto action=attempt([&]()->JSValue{if(mode==2)return call(f.manager,"remove",{a});f.owner->dispose();return {};});const auto after=f.cache();auto again=attempt([&]{return f.update();});
 return O{{"kind","native-dispose"},{"mode",mode},{"action",action},{"after",after},{"again",again},{"same",first.sameValue(call(f.manager,"get",{a}))},{"cache",f.cache()},{"trace",f.trace()},{"gpu",f.gpuState()}};
}
}
int main(){try{A output;for(int behavior=0;behavior<16;++behavior)for(bool updating:{false,true})output.push_back(nativeUpload(behavior,updating));for(int mode=0;mode<14;++mode)output.push_back(nativeRanges(mode));for(int mode=0;mode<10;++mode)output.push_back(inherited(mode));for(int mode=0;mode<3;++mode)output.push_back(disposal(mode));std::cout<<JsonValue(output).dump()<<'\n';}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
