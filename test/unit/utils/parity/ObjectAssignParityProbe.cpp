#include "core/JSValue.h"
#include "core/JSString.h"
#include <iostream>
#include <limits>
using Args=std::vector<JSValue>;using A=JsonValue::Array;using O=JsonValue::Object;
JSPropertyDescriptor data(JSValue v,bool w=true,bool e=true,bool c=true){JSPropertyDescriptor d;d.value=v;d.writable=w;d.enumerable=e;d.configurable=c;return d;}
JsonValue encode(const JSValue& v){if(v.isUndefined())return "undefined";if(v.isSymbol())return O{{"symbol",*v.symbolDescription()}};if(v.isCallable())return "function";if(v.isNull())return nullptr;if(v.type()==JSValue::Type::Number||v.type()==JSValue::Type::String||v.type()==JSValue::Type::Boolean)return *v.toJSON();A fields;for(auto& k:v.ownPropertyKeys()){auto d=*v.getOwnPropertyDescriptor(k);fields.emplace_back(A{encode(k),d.enumerable.value_or(false),d.writable.value_or(false),d.configurable.value_or(false),d.value?encode(*d.value):JsonValue("accessor")});}return fields;}
JSValue assign(const Args& args){auto o=JSValue::intrinsicObjectConstructor();return o.get("assign").call(o,args);}
void emit(const JsonValue& value){std::cout<<value.dump()<<'\n';}
JsonValue run(int mode,int targetKind){auto target=targetKind==0?JSValue::object():targetKind==1?JSValue::array():targetKind==2?JSValue("xy"):targetKind==3?JSValue(nullptr):JSValue();auto source=JSValue::object(),trace=JSValue::array(),symbol=JSValue::symbol("token");
source.defineProperty("9",data(9));source.defineProperty("2",data(2));source.defineProperty("a",data(11));source.defineProperty("b",data(12));source.defineProperty("hidden",data(13,true,false));source.defineProperty(symbol,data(14));
if(mode>=1&&mode<=7){JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{auto log=c[0];log.set(std::to_string(log.length()),"get-a");auto src=self;const auto mode=c[1].number();if(mode==1)src.erase("b");if(mode==2)src.defineProperty("b",data(12,true,false));if(mode==3)src.set("new",15);if(mode==4)src.defineProperty(c[2],data(14,true,false));if(mode==5)src.set("b",17);if(mode==6)throw JSRangeError("getter");if(mode==7)src.set("a",18);return 16;},{trace,mode,symbol});d.enumerable=d.configurable=true;source.defineProperty("a",d);}
if(target.isObject()){
if(mode==8)target.defineProperty("b",data(0,false));if(mode==9)target.preventExtensions();
if(mode==10){auto proto=JSValue::object();JSPropertyDescriptor d;d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{auto log=c[0];log.set(std::to_string(log.length()),a[0]);return {};},{trace});proto.defineProperty("a",d);target.setPrototype(proto);}
if(mode==11){JSPropertyDescriptor d;d.setter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("setter");});target.defineProperty("b",d);}}
std::string error;JSValue result;try{Args sources{target,source};if(mode==12)sources.insert(sources.end(),{nullptr,{},false,7,JSValue::symbol("source"),"ab"});result=assign(sources);}catch(const JSRangeError&){error="RangeError";}catch(const JSTypeError&){error="TypeError";}catch(const std::exception&){error="Error";}
return O{{"mode",mode},{"targetKind",targetKind},{"error",error},{"resultIdentity",target.isObject()&&result.sameValue(target)},{"target",encode(target)},{"result",encode(result)},{"trace",encode(trace)}};}
int main(){try{for(int kind=0;kind<5;++kind)for(int mode=0;mode<13;++mode)emit(run(mode,kind));}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
