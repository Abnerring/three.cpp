#include "core/JSValue.h"
#include "core/JSNumber.h"
#include "core/JSBoolean.h"
#include "core/JSSymbol.h"
#include "core/JSProxy.h"
#include "core/JSPromise.h"
#include "nodes/core/StackTrace.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args = std::vector<JSValue>;
using O = JsonValue::Object;
using A = JsonValue::Array;
JSValue array(const Args& values = {}) { auto a=JSValue::array();for(const auto& v:values)a.set(std::to_string(a.length()),v);return a; }
void log(JSValue events,const Args& values){events.set(std::to_string(events.length()),array(values));}
JsonValue encode(const JSValue& v) {
 if(v.isUndefined())return "@undefined";if(v.isNull())return nullptr;if(v.isCallable())return "@function";
 if(v.isSymbol())return "@symbol";if(v.type()==JSValue::Type::String)return v.string();if(v.type()==JSValue::Type::Boolean)return v.toBoolean();
 if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "@NaN";if(std::isinf(n))return n<0?"@-Infinity":"@Infinity";if(n==0&&std::signbit(n))return "@-0";return n;}
 if(v.type()==JSValue::Type::Array){A a;for(unsigned i=0;i<v.length();++i)a.push_back(v.has(std::to_string(i))?encode(v.get(std::to_string(i))):JsonValue("@hole"));return a;}
 O o;for(const auto& k:v.keys()){const auto d=v.getOwnPropertyDescriptor(k);o[k]=d->value?encode(*d->value):JsonValue("@accessor");}return o;
}
void getter(JSValue o,const std::string& key,JSValue get){JSPropertyDescriptor d;d.getter=get;d.enumerable=d.configurable=true;o.defineProperty(key,d);}
JSValue intrinsic(){return JSValue::array().get("some");}
template<class F>std::string error(F&& f){try{f();return "";}catch(const JSThrownValue& e){return "throw:"+e.value.toString();}catch(const JSTypeError&){return "TypeError";}catch(const JSRangeError&){return "RangeError";}catch(...){return "Error";}}
JsonValue execute(const std::string& kind,int mode,const JSValue& receiver,JSValue events,JSValue callback,JSValue thisArg={}){
 JSValue result;const auto failure=error([&]{result=intrinsic().call(receiver,{callback,thisArg});});
 return O{{"kind",kind},{"mode",mode},{"error",failure},{"result",encode(result)},{"events",encode(events)}};
}
JSValue ordinaryCallback(JSValue receiver,JSValue events,JSValue thisArg,int mode){return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
 const auto i=a[1];log(c[1],{"callback",a[0],i,self.sameValue(c[2]),a[2].sameValue(c[0])});
 if(c[3].strictEquals(0))return false;if(c[3].strictEquals(1))return int(i.number())%2==0;return i.strictEquals(5);
},{receiver,events,thisArg,mode},"callback",3);}
JsonValue sparse(int mask,bool isArray,bool inherited,int mode){
 auto receiver=isArray?JSValue::array():JSValue::object(),events=array(),thisArg=JSValue::object(),p=JSValue::object(receiver.getPrototype());receiver.setPrototype(p);receiver.set("length",6);
 for(int i=0;i<6;++i)if(mask&(1<<i))(inherited&&i%2?p:receiver).set(std::to_string(i),i==0?JSValue(-0.):i==2?JSValue():i==4?JSValue(NAN):JSValue(i));
 return execute(std::string(isArray?"array":"object")+(inherited?"-inherited":"-own"),mask*3+mode,receiver,events,ordinaryCallback(receiver,events,thisArg,mode),thisArg);
}
JsonValue mutation(int mode,bool isArray){
 auto receiver=isArray?JSValue::array():JSValue::object(),events=array(),thisArg=JSValue::object(),p=JSValue::object(receiver.getPrototype());receiver.setPrototype(p);receiver.set("length",4);
 for(int i=0;i<4;++i)receiver.set(std::to_string(i),i);
 if(mode==4||mode==5){receiver.erase("1");p.set("1",11);}
 if(mode==12||mode==13)getter(receiver,mode==12?"0":"2",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{log(c[1],{"get",self.sameValue(c[0])});if(c[2].strictEquals(13))throw JSRangeError("getter");auto o=c[0];o.set("1",91);o.collectGarbage();return 90;},{receiver,events,mode}));
 const auto callback=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
  auto o=c[0];const auto i=a[1];log(c[1],{"callback",a[0],i,self.sameValue(c[2]),a[2].sameValue(o)});const auto mode=int(c[3].number());
  if(i.strictEquals(0)){
   if(mode==0)o.erase("2");if(mode==1)o.set("1",41);if(mode==2)o.set("length",1);if(mode==3){o.set("4",44);o.set("length",5);}if(mode==4)o.setPrototype(JSValue::object(nullptr));
   if(mode==5){auto p=JSValue::object(nullptr);p.set("1",51);o.setPrototype(p);}if(mode==6){o.erase("2");o.set("2",62);}if(mode==7)o.erase("0");
   if(mode==8){const auto result=intrinsic().call(o,{JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{log(c[0],{"inner-callback",a[1]});return a[1].strictEquals(1);},{c[1]})});log(c[1],{"inner-result",result});}
   if(mode==9){o.collectGarbage();log(c[1],{"gc",a[2].sameValue(o)});}
  }
  if(mode==10&&i.strictEquals(1))throw JSThrownValue("marker");if(mode==11&&i.strictEquals(1))throw JSRangeError("callback");return false;
 },{receiver,events,thisArg,mode},"callback",3);
 return execute(isArray?"mutation-array":"mutation-object",mode,receiver,events,callback,thisArg);
}
JsonValue lengthCase(int mode,bool invalidCallback){
 auto receiver=JSValue::object(),events=array(),thisArg=JSValue::object();receiver.set("0",-0.);receiver.set("1",1);receiver.set("2",2);
 const Args lengths{JSValue(),nullptr,false,true,0,-0.,-1,-INFINITY,NAN,0.9,1.9,3.9," 3.8 ","invalid",4294967297.,9007199254740991.,INFINITY};
 JSValue length=mode<int(lengths.size())?lengths[mode]:JSValue{};
 if(mode>=17&&mode<=21){length=JSValue::object();length.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],{"valueOf"});if(c[2].strictEquals(18))throw JSRangeError("length");if(c[2].strictEquals(19))return JSValue::object();if(c[2].strictEquals(20)){auto o=c[1];o.set("0",20);}if(c[2].strictEquals(21)){auto o=c[1];o.set("length",0);}return 2.9;},{events,receiver,mode}));length.set("toString",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],{"toString"});return "2.9";},{events}));}
 if(mode==22)length=JSValue::symbol("length");
 if(mode==23){length=JSValue::object();length.set(JSValue::wellKnownSymbol("toPrimitive"),JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{log(c[0],{"toPrimitive",a[0]});return 2.9;},{events}));}
 auto state=JSValue::object();state.set("reads",0);
 getter(receiver,"length",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{log(c[1],{"length",self.sameValue(c[0])});auto state=c[3];const auto reads=state.get("reads").number()+1;state.set("reads",reads);if(reads>1)throw JSRangeError("length read twice");return c[2];},{receiver,events,length,state}));
 const auto cb=invalidCallback?JSValue(17):JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{log(c[1],{"callback",a[0],a[1],self.sameValue(c[2]),a[2].sameValue(c[0])});return true;},{receiver,events,thisArg});
 return execute(invalidCallback?"length-invalid-callback":"length",mode,receiver,events,cb,thisArg);
}
JsonValue primitive(int mode){
 const Args values{JSValue(),nullptr,false,true,0,-0.,42,NAN,"","a","ab","\xF0\x9F\x98\x80",JSValue::symbol("s")};const auto receiver=values[mode];auto events=array();
 const auto cb=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{log(c[0],{"callback",a[0],a[1],self.isUndefined(),a[2].isObject(),a[2].type()==JSValue::Type::String});return false;},{events});
 return execute("primitive",mode,receiver,events,cb);
}
JsonValue wrapper(int mode){
 const auto value=mode==0?JSValue(17):mode==1?JSValue(true):JSValue::symbol("s");auto prototype=mode==0?nativeNumberPrototype():mode==1?nativeBooleanPrototype():nativeSymbolPrototype(),events=array();
 prototype.set("length",1);prototype.set("0",71);
 auto cb=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{const auto v=c[1].number()==0?nativeNumberValue(a[2]):c[1].number()==1?nativeBooleanValue(a[2]):nativeSymbolValue(a[2]);log(c[0],{"callback",a[0],a[1],self.isUndefined(),a[2].getPrototype().sameValue(c[2]),v.sameValue(c[3])});return false;},{events,mode,prototype,value});
 auto result=execute("primitive-prototype",mode,value,events,cb);prototype.erase("length");prototype.erase("0");return result;
}
JsonValue truth(int mode){
 const Args values{JSValue(),nullptr,false,true,0,-0.,NAN,1,"","x",JSValue::object(),JSValue::symbol("truth")};auto receiver=array({7,8}),events=array(),thisArg=JSValue::object(),result=values[mode];
 if(result.isObject())result.set(JSValue::wellKnownSymbol("toPrimitive"),JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("ToBoolean must not convert objects");}));
 auto cb=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{log(c[0],{"callback",a[0],a[1],self.sameValue(c[1])});return c[2];},{events,thisArg,result});
 return execute("truth",mode,receiver,events,cb,thisArg);
}
JsonValue thisValue(int mode){
 const Args values{JSValue(),nullptr,false,17,"x",JSValue::symbol("this"),JSValue::object()};auto events=array(),receiver=array({1}),thisArg=values[mode];
 auto cb=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{log(c[0],{"callback",self.sameValue(c[1]),a[1],a[2].sameValue(c[2])});return true;},{events,thisArg,receiver});
 return execute("strict-thisArg",mode,receiver,events,cb,thisArg);
}
JsonValue invalidCallback(int mode,bool empty){auto receiver=JSValue::object(),events=array();getter(receiver,"length",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],{"length"});return c[1];},{events,empty?0:1}));receiver.set("0",1);const Args callbacks{JSValue(),nullptr,false,17,"callback",JSValue::symbol("callback"),JSValue::object()};return execute(empty?"invalid-callback-empty":"invalid-callback",mode,receiver,events,callbacks[mode]);}
JsonValue proxyCase(int mode){
 auto target=JSValue::object(),events=array(),handler=JSValue::object();target.set("length",3);target.set("0",7);target.set("2",9);
 if(mode==11)target.set("length",4294967296.);if(mode==12)target.set("length",4294967297.);if(mode==13)target.set("length",INFINITY);if(mode==14)target.set("length",-INFINITY);if(mode==15)target.set("length",NAN);
 handler.set("get",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{log(c[0],{"get",a[1]});if(c[1].strictEquals(4)&&a[1].strictEquals("0"))throw JSThrownValue("get");if(c[1].strictEquals(6)&&a[1].strictEquals("length"))throw JSThrownValue("length");return a[0].get(a[1],a[2]);},{events,mode}));
 handler.set("has",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{log(c[0],{"has",a[1]});if(((c[1].strictEquals(3)||c[1].strictEquals(12))&&a[1].strictEquals("1"))||((c[1].strictEquals(11)||c[1].strictEquals(13))&&a[1].strictEquals("0")))throw JSThrownValue("has");if(c[1].strictEquals(5)&&a[1].strictEquals("1")){auto o=a[0];o.set("1",81);return true;}if(c[1].strictEquals(10)&&a[1].strictEquals("0")){auto o=a[0];o.erase("0");return true;}if(c[1].strictEquals(16)&&a[1].strictEquals("1"))return true;return a[0].has(a[1]);},{events,mode}));
 auto receiver=createNativeProxy(target,handler),thisArg=JSValue::object();
 JSValue cb=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{log(c[0],{"callback",a[0],a[1],self.sameValue(c[2]),a[2].sameValue(c[1])});if(c[3].strictEquals(7)){revokeNativeProxy(c[1]);return false;}return c[3].strictEquals(1);},{events,receiver,thisArg,mode});
 if(mode==2||mode==6)cb=17;if(mode==8){auto p=createNativeProxy(cb,JSValue::object());revokeNativeProxy(p);cb=p;}
 if(mode==9)cb=JSValue::constructor(+[](const JSValue& self,const Args&,const Args&)->JSValue{return self;},{},"Class");
 if(mode==17){auto apply=JSValue::object();apply.set("apply",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{log(c[0],{"apply",a[1].sameValue(c[1]),a[2].length()});a[2].collectGarbage();Args values;for(unsigned i=0;i<a[2].length();++i)values.push_back(a[2].get(std::to_string(i)));return a[0].call(a[1],values);},{events,thisArg}));cb=createNativeProxy(cb,apply);}
 if(mode==18){revokeNativeProxy(receiver);cb=17;}
 return execute("proxy",mode,receiver,events,cb,thisArg);
}
JsonValue metadata(){auto method=intrinsic();JSValue parent=JSValue::array().getPrototype();const auto d=parent.getOwnPropertyDescriptor("some");return O{{"kind","metadata"},{"exists",d.has_value()},{"name",encode(method.isObject()?method.get("name"):JSValue{})},{"length",encode(method.isObject()?method.get("length"):JSValue{})},{"own",method.isObject()?encode(array([&]{Args v;for(const auto& key:method.ownPropertyNames())v.push_back(key);return v;}())):JsonValue("@undefined")},{"source",method.isCallable()?method.nativeFunctionSource():"@undefined"},{"construct",error([&]{method.construct();})},{"writable",d&&d->writable.value_or(false)},{"enumerable",d&&d->enumerable.value_or(false)},{"configurable",d&&d->configurable.value_or(false)}};}
JsonValue stack(int mode){const Args texts{"Error: test\n    at first (/app/main.js:12:3)\n    at internals (/app/ThingNode.js:99:1)\n    at second (/app/worker.js:20:8)","Error\n    at a (/app/StackTrace.js:1:1)\n    at b (/app/TSLCore.js:2:2)\n    at c (/app/three.webgpu.min.js:3:3)","Error\nfire@https://host.test/view.js?hmr=1:40:9\n    at /app/plain.js:2:5"};JSValue value;const auto failure=error([&]{auto st=nativeStackTraceConstructor().construct({texts[mode]});value=JSValue::object();value.set("stack",st.get("stack"));value.set("location",st.get("getLocation").call(st));value.set("message",st.get("getError").call(st,{"message"}));});return O{{"kind","stacktrace-regression"},{"mode",mode},{"error",failure},{"value",encode(value)}};}
JsonValue lifetime(){A observations;auto realm=JSValue::object();for(int round=0;round<3;++round){std::vector<JSWeakValue> weak;
 {auto receiver=JSValue::object(),thisArg=JSValue::object(),capture=JSValue::object(),value=JSValue::object();receiver.set("length",1);receiver.set("0",value);capture.set("self",capture);auto state=JSValue::object();state.set("held",false);
  auto callback=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{auto receiver=a[2];receiver.erase("0");receiver.collectGarbage();auto state=c[1];state.set("held",self.sameValue(c[2])&&a[0].get("alive").toBoolean()&&c[0].get("self").sameValue(c[0]));return false;},{capture,state,thisArg});
  value.set("alive",true);weak={JSWeakValue(receiver),JSWeakValue(thisArg),JSWeakValue(capture),JSWeakValue(value),JSWeakValue(callback)};receiver.set("callback",callback);
  value={};capture={};const auto result=intrinsic().call(receiver,{callback,thisArg});
  if(!result.strictEquals(false)||!state.get("held").toBoolean())throw std::logic_error("some lost active callback graph");
  receiver.erase("callback");callback={};receiver={};state={};thisArg={};
 }
 endJSJob();realm.collectGarbage();bool dropped=true;for(const auto& w:weak)dropped=dropped&&w.expired();if(!dropped)throw std::logic_error("some retained completed graph");observations.push_back(O{{"round",round},{"phase","completed-graph-dropped"},{"gone",true}});
 {auto receiver=array({1}),thisArg=JSValue::object();thisArg.set("alive",true);auto result=intrinsic().call(receiver,{JSValue::function(+[](const JSValue& self,const Args& a,const Args&)->JSValue{a[2].collectGarbage();return self.get("alive");}),thisArg});if(!result.strictEquals(true))throw std::logic_error("some lost thisArg");}
 observations.push_back(O{{"round",round},{"phase","callback-roots-held"},{"alive",true}});
 }return O{{"rounds",3},{"observations",observations}};}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--lifetime"){std::cout<<lifetime().dump();return 0;}if(argc!=1)throw std::invalid_argument("Only --lifetime is accepted");A results;
 for(bool isArray:{false,true})for(bool inherited:{false,true})for(int mask=0;mask<64;++mask)for(int mode=0;mode<3;++mode)results.push_back(sparse(mask,isArray,inherited,mode));
 for(bool isArray:{false,true})for(int mode=0;mode<14;++mode)results.push_back(mutation(mode,isArray));
 for(bool invalid:{false,true})for(int mode=0;mode<24;++mode)results.push_back(lengthCase(mode,invalid));
 for(int mode=0;mode<13;++mode)results.push_back(primitive(mode));for(int mode=0;mode<3;++mode)results.push_back(wrapper(mode));for(int mode=0;mode<12;++mode)results.push_back(truth(mode));for(int mode=0;mode<7;++mode)results.push_back(thisValue(mode));for(bool empty:{false,true})for(int mode=0;mode<7;++mode)results.push_back(invalidCallback(mode,empty));for(int mode=0;mode<19;++mode)results.push_back(proxyCase(mode));results.push_back(metadata());for(int mode=0;mode<3;++mode)results.push_back(stack(mode));std::cout<<JsonValue(results).dump();}
 catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
