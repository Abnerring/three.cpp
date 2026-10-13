#include "core/JSValue.h"
#include <cmath>
#include <iostream>
using Args=std::vector<JSValue>;using O=JsonValue::Object;using A=JsonValue::Array;
void data(JSValue object,const std::string& name,JSValue value){JSPropertyDescriptor d;d.value=value;d.writable=d.configurable=d.enumerable=true;object.defineProperty(name,d);}
JsonValue encode(const JSValue& value){
 if(value.isUndefined())return "undefined";if(value.isNull())return nullptr;
 if(value.type()==JSValue::Type::Number){const auto n=value.number();if(std::isnan(n))return "NaN";if(n==0&&std::signbit(n))return "-0";return n;}
 if(value.type()==JSValue::Type::String)return value.string();if(value.type()==JSValue::Type::Boolean)return value.toBoolean();
 if(value.isCallable())return "function";O props;
 for(const auto& key:value.ownPropertyNames()){const auto d=value.getOwnPropertyDescriptor(key);props[key]=d->value?encode(*d->value):JsonValue("accessor");}
 return props;
}
void log(JSValue events,const JSValue& entry){events.get("push").call(events,{entry});}
JsonValue run(int mode){auto left=JSValue::array(4),right=JSValue::array(2),events=JSValue::array();data(left,"0",1);data(left,"2",JSValue());data(left,"3",4);data(right,"1",8);
 JSValue receiver=left;Args args{right,7};const auto concat=left.get("concat");
 if(mode==1){auto p=JSValue::object(left.getPrototype());data(p,"1",3);left.setPrototype(p);}
 if(mode==2){auto p=JSValue::object(right.getPrototype());data(p,"0",6);right.setPrototype(p);}
 if(mode>=3&&mode<=8){JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
   log(c[2],self.sameValue(c[0])?"left:0":"wrong");auto l=c[0],r=c[1];const int mode=int(c[3].toNumber());
   if(mode==3)l.set("length",1);if(mode==4){l.erase("2");data(l,"1",5);data(l,"4",10);}if(mode==5){r.set("length",3);data(r,"2",9);}
   if(mode==6)throw JSRangeError("getter");if(mode==7){auto nested=JSValue::array();log(c[2],encode(nested).dump());l.collectGarbage();}
   if(mode==8){auto p=JSValue::object(l.getPrototype());data(p,"2",22);l.erase("2");l.setPrototype(p);}return 2;
 },{left,right,events,mode});left.defineProperty("0",d);}
 if(mode>=9&&mode<=13){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
   log(c[1],"constructor");if(c[2].strictEquals(10))throw JSTypeError("constructor");if(c[2].strictEquals(11))return 3;
   if(c[2].strictEquals(12)){auto l=c[0];l.set("length",1);}return c[2].strictEquals(13)?JSValue(nullptr):JSValue();
 },{left,events,mode});left.defineProperty("constructor",d);}
 if(mode==14){receiver=JSValue::object();data(receiver,"length",3);data(receiver,"0",42);}
 if(mode==15)receiver="xy";if(mode==16)receiver=42;if(mode==17)receiver=true;if(mode==18)receiver=nullptr;if(mode==19)receiver={};
 if(mode==20)args={left};if(mode==21){left.set("length",0);right.set("length",0);args={right};}
 if(mode==22){data(left,"0",-0.0);data(left,"3",NAN);}
 if(mode==23){const auto nested=JSValue::array(1);data(nested,"0",42);data(right,"0",nested);}
 JSValue pollutedPrototype;std::optional<JSPropertyDescriptor> oldZero;
 if(mode==24){pollutedPrototype=left.getPrototype();oldZero=pollutedPrototype.getOwnPropertyDescriptor("0");JSPropertyDescriptor d;d.configurable=true;d.setter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSTypeError("prototype setter");});pollutedPrototype.defineProperty("0",d);}
 if(mode==25){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[1],"constructor");auto r=c[0];r.set("length",4);data(r,"3",17);return {};},{right,events});left.defineProperty("constructor",d);}
 if(mode==26){JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[1],"reenter");auto nested=c[0].get("concat").call(c[0],{19});log(c[1],nested.get("length"));return 2;},{right,events});left.defineProperty("0",d);}
 if(mode==27){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("argument constructor must not be read");});right.defineProperty("constructor",d);}
 if(mode==28||mode==30){auto target=mode==28?right:left;if(mode==30)target.set("length",4294967295.0);JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"throw:first");throw JSRangeError("index");},{events});target.defineProperty("0",d);}
 if(mode==29){auto readonly=JSValue::object(left.getPrototype());JSPropertyDescriptor d;d.value=5;d.writable=false;d.configurable=true;readonly.defineProperty("1",d);left.setPrototype(readonly);}
 if(mode==31){auto argument=JSValue::object();JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("non-array length must not be read");});d.configurable=true;argument.defineProperty("length",d);args={argument,"abc"};}
 JSValue result;std::string error;try{result=concat.call(receiver,args);}catch(const JSTypeError&){error="TypeError";}catch(const JSRangeError&){error="RangeError";}
 if(!pollutedPrototype.isUndefined()){if(oldZero)pollutedPrototype.defineProperty("0",*oldZero);else pollutedPrototype.erase("0");}
 return O{{"mode",mode},{"result",encode(result)},{"error",error},{"trace",encode(events)},{"newIdentity",!result.sameValue(receiver)}};
}
int main(){A output;for(int mode=0;mode<32;++mode)output.push_back(run(mode));
 auto array=JSValue::array(),fn=array.get("concat");const auto d=array.getPrototype().getOwnPropertyDescriptor("concat");
 output.push_back(O{{"kind","metadata"},{"name",fn.get("name").string()},{"length",fn.get("length").number()},{"constructible",fn.isConstructible()},{"flags",A{*d->writable,*d->enumerable,*d->configurable}}});
 std::cout<<JsonValue(output).dump()<<'\n';}
