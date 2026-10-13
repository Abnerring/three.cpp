#include "core/JSValue.h"
#include <cmath>
#include <iostream>
namespace {
using Args = std::vector<JSValue>;
using O = JsonValue::Object;
using A = JsonValue::Array;
JSValue array(const Args& values = {}) { auto a=JSValue::array(); for(const auto& v:values)a.set(std::to_string(a.length()),v);return a; }
void log(JSValue events,const Args& values){events.set(std::to_string(events.length()),array(values));}
JsonValue encode(const JSValue& v) {
 if(v.isUndefined())return "@undefined";if(v.isNull())return nullptr;if(v.isCallable())return "@function";
 if(v.type()==JSValue::Type::String)return v.string();if(v.type()==JSValue::Type::Boolean)return v.toBoolean();
 if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "@NaN";if(n==0&&std::signbit(n))return "@-0";return n;}
 if(v.type()==JSValue::Type::Array){A a;for(unsigned i=0;i<v.length();++i)a.push_back(v.has(std::to_string(i))?encode(v.get(std::to_string(i))):JsonValue("@hole"));return a;}
 O o;for(const auto& k:v.keys()){const auto d=v.getOwnPropertyDescriptor(k);o[k]=d->value?encode(*d->value):JsonValue("@accessor");}return o;
}
void accessor(JSValue o,const std::string& key,JSValue getter){JSPropertyDescriptor d;d.getter=getter;d.enumerable=d.configurable=true;o.defineProperty(key,d);}
void accessor(JSValue o,const JSValue& key,JSValue getter){JSPropertyDescriptor d;d.getter=getter;d.enumerable=d.configurable=true;o.defineProperty(key,d);}
void field(JSValue o,const std::string& key,JSValue value,bool writable=true){JSPropertyDescriptor d;d.value=value;d.writable=writable;d.enumerable=d.configurable=true;o.defineProperty(key,d);}
JSValue fn(const char* name="filter"){return JSValue::array().get(name);}
JsonValue run(int mask,bool isArray,bool inherited,int mode,int species,const std::string& method="filter") {
 auto receiver=isArray?JSValue::array():JSValue::object(),events=JSValue::array(),thisArg=JSValue::object(),target=JSValue::object();
 auto p=JSValue::object(receiver.getPrototype());receiver.setPrototype(p);receiver.set("length",6);
 for(int i=0;i<6;++i)if(mask&(1<<i)){auto at=inherited&&i%2?p:receiver;at.set(std::to_string(i),i==0?JSValue(-0.):i==2?JSValue():i==4?JSValue(NAN):JSValue(i));}
 auto callback=JSValue::function([](const JSValue& self,const Args& a,const Args& c)->JSValue{
  auto o=c[0],ev=c[1];const int mode=int(c[3].number()),i=int(a[1].number());log(ev,{"callback",a[0],i,self.sameValue(c[2]),a[2].sameValue(o)});
  if(i==0){if(mode==3)o.erase("2");if(mode==4)o.set("1",41);if(mode==5)o.set("length",1);if(mode==6)o.set("6",66);if(mode==7)o.setPrototype(JSValue::object(nullptr));if(mode==8){auto p=JSValue::object(o.getPrototype());p.set("1",88);o.setPrototype(p);}if(mode==11){o.collectGarbage();log(ev,{"gc"});}if(mode==12){const auto inner=fn().call(o,{JSValue::function([](const JSValue&,const Args& a,const Args&)->JSValue{return a[1].strictEquals(0);})});log(ev,{"inner",inner});}}
  if(mode==9&&i==2)throw JSRangeError("callback");if(mode==1)return i%2==0;if(mode==2)return false;if(mode==10)return JSValue::object();if(mode==13)return "";return true;
 },{receiver,events,thisArg,mode});
 if(mode==14)callback=17;
 if(!isArray){accessor(receiver,std::string("length"),JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{log(c[1],{"length",self.sameValue(c[0])});if(c[2].strictEquals(15))throw JSRangeError("length");return 6;},{receiver,events,mode}));}
 if(species){
  auto constructor=JSValue::object();
  accessor(receiver,std::string("constructor"),JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{log(c[1],{"constructor",self.sameValue(c[0])});if(c[3].strictEquals(1))throw JSRangeError("constructor");if(c[3].strictEquals(2))return nullptr;if(c[3].strictEquals(3))return 17;return c[2];},{receiver,events,constructor,species}));
  auto custom=JSValue::constructor([](const JSValue&,const Args& a,const Args& c)->JSValue{log(c[1],{"new",a[0]});if(c[2].strictEquals(7))throw JSRangeError("new");if(c[2].strictEquals(10)){auto receiver=c[3];receiver.set("1",91);}return c[0];},{target,events,species,receiver},"Species",1);
  accessor(constructor,JSValue::wellKnownSymbol("species"),JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{log(c[0],{"species",self.sameValue(c[3])});const auto mode=c[1];if(mode.strictEquals(4))throw JSRangeError("species");if(mode.strictEquals(5))return nullptr;if(mode.strictEquals(6))return 17;if(mode.strictEquals(11))return {};return c[2];},{events,species,custom,constructor}));
  if(species==8)target.preventExtensions();
  if(species==9){JSPropertyDescriptor d;d.value="locked";d.configurable=false;d.writable=false;d.enumerable=true;target.defineProperty("1",d);}
 }
 JSValue result;std::string error;
 try {if(method=="slice")result=fn("slice").call(receiver,{0,4});else if(method=="splice")result=fn("splice").call(receiver,{0,4});else if(method=="concat")result=fn("concat").call(receiver,{array({8,9})});else result=fn(method.c_str()).call(receiver,{callback,thisArg});}
 catch(const JSTypeError&){error="TypeError";}catch(const JSRangeError&){error="RangeError";}catch(const std::exception&){error="Error";}
 return O{{"mask",mask},{"array",isArray},{"inherited",inherited},{"mode",mode},{"species",species},{"method",method},{"error",error},{"result",encode(result)},{"target",encode(target)},{"trace",encode(events)}};
}
}
int main(){auto realm=JSValue::object();A results;
for(bool isArray:{false,true})for(bool inherited:{false,true})for(int mask=0;mask<64;++mask)for(int mode=0;mode<3;++mode)results.push_back(run(mask,isArray,inherited,mode,0));
for(bool isArray:{false,true})for(int mode=3;mode<16;++mode)results.push_back(run(63,isArray,true,mode,0));
for(const std::string method:{"filter","map","slice","splice","concat"})for(bool isArray:{false,true})for(int species=1;species<12;++species)results.push_back(run(47,isArray,false,0,species,method));
results.push_back(run(63,true,false,14,4));
std::cout<<JsonValue(results).dump();}
