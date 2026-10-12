#include "core/JSValue.h"
#include <cmath>
#include <iostream>
#include <limits>
namespace {
using Args=std::vector<JSValue>;using O=JsonValue::Object;using A=JsonValue::Array;
void append(JSValue array,Args args){array.get("push").call(array,args);}
JSValue tuple(Args args){auto a=JSValue::array();append(a,args);return a;}
JsonValue encode(const JSValue& v){switch(v.type()){
case JSValue::Type::Undefined:return "@undefined";case JSValue::Type::Null:return nullptr;
case JSValue::Type::Boolean:return v.toBoolean();case JSValue::Type::String:return v.string();
case JSValue::Type::Number:{auto n=v.number();if(std::isnan(n))return "@NaN";if(std::isinf(n))return n<0?"@-Infinity":"@Infinity";if(n==0&&std::signbit(n))return "@-0";return n;}
case JSValue::Type::Function:return "@function";
case JSValue::Type::Array:{A a;for(unsigned i=0;i<v.length();++i)a.push_back(encode(v.get(std::to_string(i))));return a;}
default:{O o;for(auto& k:v.keys())o[k]=encode(v.get(k));return o;}}
}
JsonValue snapshot(JSValue o){O props;for(const auto& k:o.ownPropertyNames()){
 auto d=o.getOwnPropertyDescriptor(k);props[k]=d->value?encode(*d->value):JsonValue("@accessor");
}return props;}
JSValue getter(JSValue events,std::string label,JSValue value,JSValue mutate={},int action=0){return JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{
 append(c[0],{tuple({c[1],self.strictEquals(c[3])})});
 if(c[4].strictEquals(JSValue(1)))throw JSRangeError("getter");
 if(c[4].strictEquals(JSValue(2))){auto o=c[3];o.set("2",-100);}
 return c[2];},{events,label,value,mutate,action});}
void accessor(JSValue o,std::string key,JSValue events,JSValue value,int mode=0){JSPropertyDescriptor d;
 d.getter=getter(events,"get:"+key,value,o,mode);
 if(mode!=3)d.setter=JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{
 append(c[0],{tuple({c[1],args[0],self.strictEquals(c[2])})});if(c[3].toBoolean())throw JSRangeError("setter");return {};},{events,"set:"+key,o,mode==4});
 d.configurable=d.enumerable=true;o.defineProperty(key,d);}
std::string error(const auto& operation){try{operation();return "";}catch(const JSTypeError&){return "TypeError";}catch(const JSRangeError&){return "RangeError";}catch(...){return "Error";}}
JSValue comparison(JSValue events,JSValue receiver,int mode){auto state=JSValue::object(nullptr);state.set("count",0);return JSValue::function([](const JSValue& self,const Args& a,const Args& c)->JSValue{
 auto state=c[3];const auto count=state.get("count").toNumber()+1;state.set("count",count);
 append(c[0],{tuple({a[0],a[1],self.isUndefined()})});auto receiver=c[1];const auto mode=int(c[2].toNumber());
 if(mode==3&&count==2)throw JSRangeError("compare");
 if(mode==4&&count==1)receiver.get("push").call(receiver,{99});
 if(mode==5&&count==1){receiver.set("length",0);receiver.set("7",88);}
 if(mode==6&&count==1){auto reverse=JSValue::function([](const JSValue&,const Args&a,const Args&)->JSValue{return a[1].toNumber()-a[0].toNumber();});receiver.get("sort").call(receiver,{reverse});}
 if(mode==7){auto v=JSValue::object();v.setPrimitiveConverter(JSValue::function([](const JSValue& self,const Args& args,const Args& cap)->JSValue{append(cap[0],{tuple({"coerce",args[0],self.isObject()})});return cap[1];},{c[0],a[0].toNumber()-a[1].toNumber()}));return v;}
 if(mode==2)return NAN;if(mode==1)return a[1].toNumber()-a[0].toNumber();return a[0].toNumber()-a[1].toNumber();
 },{events,receiver,mode,state});}
JsonValue dense(int count,int mode){auto events=JSValue::array(),o=JSValue::array();
 for(int i=0;i<count;++i)o.set(std::to_string(i),((i*73+19)%29)-14);
 JSValue result;const auto err=error([&]{result=o.get("sort").call(o,{comparison(events,o,mode)});});
 return O{{"kind","dense"},{"count",count},{"mode",mode},{"error",err},{"same",result.strictEquals(o)},{"props",snapshot(o)},{"events",encode(events)}};
}
JsonValue scenario(int mode,bool unshift){auto events=JSValue::array(),o=JSValue::object();o.set("length",4);o.set("0",3);o.set("2",1);o.set("3",JSValue());
 if(mode==0){auto p=JSValue::object();p.set("1",2);o.setPrototype(p);}
 if(mode==1)accessor(o,"0",events,3);
 if(mode==2)accessor(o,"1",events,4,2);
 if(mode==3)accessor(o,"2",events,1,1);
 if(mode==4)accessor(o,"2",events,1,3);
 if(mode==5)accessor(o,"0",events,3,4);
 if(mode==6){JSPropertyDescriptor d;d.writable=false;o.defineProperty("1",d);}
 if(mode==7){JSPropertyDescriptor d;d.configurable=false;o.defineProperty("3",d);}
 if(mode==8){JSPropertyDescriptor d;d.writable=false;o.defineProperty("length",d);}
 if(mode==9)o.preventExtensions();
 if(mode==10){auto n=JSValue::object();n.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args&a,const Args&c)->JSValue{append(c[0],{tuple({"length",a[0]})});return 3.8;},{events}));o.set("length",n);}
 if(mode==11)accessor(o,"length",events,1);
 if(mode==12){accessor(o,"length",events,4);}
 if(mode==13){o.set("length",0);accessor(o,"0",events,3);}
 if(mode==14){o.set("length",-1);}
 if(mode==15){o.set("length",NAN);}
 if(mode==16){o.set("length","3.8");}
 if(mode==17){o.set("length",9007199254740991.);}
 if(mode==18){o.set("length",4294967295.);}
 JSValue result;auto intrinsic=JSValue::array().get(unshift?"unshift":"sort");
 const auto err=error([&]{if(unshift)result=intrinsic.call(o,mode==18?Args{}:Args{8,9});else result=intrinsic.call(o,{comparison(events,o,0)});});
 return O{{"kind",unshift?"unshift":"generic"},{"mode",mode},{"error",err},{"same",result.strictEquals(o)},{"result",result.isObject()?JsonValue("@object"):encode(result)},{"props",snapshot(o)},{"events",encode(events)}};
}
JsonValue defaults(int mode){auto events=JSValue::array();JSValue o;
 switch(mode){case 0:o=tuple({"\xEE\x80\x80","\xF0\x90\x80\x80","a","\xED\xA0\x80","\xED\xB0\x80"});break;
 case 1:o=tuple({10,2,-1,0,-0.,nullptr,true,false,"x",JSValue(),NAN});break;
 case 2:case 3:{o=JSValue::array();for(auto value:{"z","a","m"}){auto v=JSValue::object();v.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args&a,const Args&c)->JSValue{append(c[0],{tuple({c[1],a[0]})});if(c[2].toBoolean())throw JSRangeError("conversion");return c[1];},{events,value,mode==3&&std::string(value)=="m"}));append(o,{v});}break;}
 case 4:o="";break;case 5:o="a";break;case 6:o="ba";break;case 7:o=42;break;case 8:o=true;break;case 9:o=nullptr;break;default:o=JSValue();}
 JSValue result;const auto err=error([&]{result=JSValue::array().get("sort").call(o);});
 return O{{"kind","default"},{"mode",mode},{"error",err},{"props",o.isObject()?snapshot(o):encode(o)},{"returned",result.isObject()?snapshot(result):encode(result)},{"events",encode(events)}};
}
}
int main(){A result;
 for(int count:{0,1,2,3,7,8,31,64,127,256})for(int mode=0;mode<8;++mode)result.push_back(dense(count,mode));
 for(int mode=0;mode<17;++mode)result.push_back(scenario(mode,false));
 for(int mode=0;mode<19;++mode)result.push_back(scenario(mode,true));
 for(int mode=0;mode<11;++mode)result.push_back(defaults(mode));
 std::cout<<JsonValue(result).dump()<<'\n';}
