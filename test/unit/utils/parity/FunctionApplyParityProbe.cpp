#include "core/JSFunction.h"
#include "core/JSGenerator.h"
#include "core/JSProxy.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
namespace {
using Args=std::vector<JSValue>;
void push(JSValue array,JSValue value){array.set(std::to_string(array.length()),value);}
JSValue list(const Args& values){auto out=JSValue::array();for(const auto& v:values)push(out,v);return out;}
JSValue normalize(const JSValue& v){if(v.isUndefined())return "#undefined";if(v.isSymbol())return "#symbol";if(v.isCallable())return "#function";if(v.type()==JSValue::Type::Number&&std::isnan(v.toNumber()))return "#NaN";return v;}
void prop(JSValue object,const std::string& key,JSValue value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
void getter(JSValue object,const JSValue& key,JSValue value){JSPropertyDescriptor d;d.getter=value;d.enumerable=d.configurable=true;object.defineProperty(key,d);}
JSValue failure(){try{throw;}catch(const JSGeneratorThrown& e){auto out=JSValue::object();out.set("thrown",normalize(e.value()));return out;}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception&){return "#Error";}}
JSValue target(const JSValue& receiver,const Args& args,const Args& captures){push(captures[0],"call");auto out=JSValue::object();out.set("thisValue",normalize(receiver));auto values=JSValue::array();for(const auto& v:args)push(values,normalize(v));out.set("args",values);return out;}
JSValue loggedGet(const JSValue&,const Args&,const Args& c){push(c[0],c[1]);if(c.size()>3&&c[3].toBoolean())throw JSGeneratorThrown(c[2]);return c[2];}
JSValue read(JSValue log,const char* key,JSValue value,bool throws=false){return JSValue::function(loggedGet,{log,key,value,throws},"",0);}
JSValue loggedArray(JSValue log,JSValue length=2){auto out=JSValue::object();getter(out,"length",read(log,"length",length));getter(out,"0",read(log,"0",4));getter(out,"1",read(log,"1",5));return out;}
JSValue run(const std::string& mode){
 auto log=JSValue::array();auto fn=JSValue::function(target,{log},"target",0);auto fp=fn.getPrototype();const auto apply=fp.get("apply");
 auto out=JSValue::object();JSValue arrayLike=loggedArray(log),thisArg="THIS";
 if(mode=="missing"||mode=="undefined")arrayLike={};else if(mode=="null")arrayLike=nullptr;
 else if(mode=="dense")arrayLike=list({1,2,3});
 else if(mode=="boxedString")arrayLike=JSValue("ab").toObject();
 else if(mode=="callPropertyIgnored")getter(fn,"call",read(log,"target.call",61,true));
 else if(mode=="sparse"){arrayLike=JSValue::array(3);arrayLike.set("1",7);}
 else if(mode=="inherited"){auto parent=JSValue::object();parent.set("1",9);arrayLike=JSValue::object(parent);arrayLike.set("length",3);arrayLike.set("0",8);}
 else if(mode=="string")arrayLike="abc";else if(mode=="number")arrayLike=2;else if(mode=="boolean")arrayLike=true;else if(mode=="symbol")arrayLike=JSValue::symbol("test");
 else if(mode=="negative")arrayLike=loggedArray(log,-1);else if(mode=="nan")arrayLike=loggedArray(log,std::numeric_limits<double>::quiet_NaN());
 else if(mode=="fraction")arrayLike=loggedArray(log,1.9);else if(mode=="lengthString")arrayLike=loggedArray(log,"2.9");
 else if(mode=="lengthUndefined")arrayLike=loggedArray(log,{});else if(mode=="lengthSymbol")arrayLike=loggedArray(log,JSValue::symbol());
 else if(mode=="conversion"||mode=="conversionThrows"||mode=="primitiveHint"){
  auto length=JSValue::object();
  if(mode=="primitiveHint"){
   JSPropertyDescriptor d;d.value=JSValue::function(+[](const JSValue&,const Args&a,const Args&c)->JSValue{push(c[0],"convert:"+a[0].toString());return 2;},{log});d.writable=d.configurable=true;length.defineProperty(JSValue::wellKnownSymbol("toPrimitive"),d);
  }else{
   prop(length,"valueOf",read(log,"valueOf",mode=="conversionThrows"?JSValue(29):JSValue::object(),mode=="conversionThrows"));
   prop(length,"toString",read(log,"toString","2"));
  }arrayLike=loggedArray(log,length);
 }
 else if(mode=="lengthThrows")getter(arrayLike,"length",read(log,"length",23,true));
 else if(mode=="indexThrows")getter(arrayLike,"1",read(log,"1",31,true));
 else if(mode=="hugeLength"||mode=="capacityExceeded"){arrayLike=loggedArray(log,mode=="capacityExceeded"?134217729.0:9007199254740992.0);getter(arrayLike,"0",read(log,"0",37,true));}
 else if(mode=="iteratorIgnored")getter(arrayLike,JSValue::wellKnownSymbol("iterator"),read(log,"iterator",41,true));
 else if(mode=="proxy"){
  auto handler=JSValue::object();handler.set("get",JSValue::function(+[](const JSValue&,const Args&a,const Args&c)->JSValue{push(c[0],"get:"+a[1].toString());return a[0].get(a[1]);},{log}));
  handler.set("has",read(log,"has",true,true));arrayLike=createNativeProxy(arrayLike,handler);
 }
 else if(mode=="badTarget"||mode=="badTargetRevokedList"){fn=JSValue::object();if(mode=="badTargetRevokedList"){arrayLike=createNativeProxy(arrayLike,JSValue::object());revokeNativeProxy(arrayLike);}}
 else if(mode=="revokedList"){arrayLike=createNativeProxy(arrayLike,JSValue::object());revokeNativeProxy(arrayLike);}
 else if(mode=="revokedTarget"){fn=createNativeProxy(fn,JSValue::object());revokeNativeProxy(fn);}
 else if(mode=="classTarget")fn=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"C");
 else if(mode=="proxyTarget"){
  auto handler=JSValue::object();handler.set("apply",JSValue::function(+[](const JSValue&,const Args&a,const Args&c)->JSValue{push(c[0],"applyTrap");Args args;for(unsigned i=0;i<a[2].length();++i)args.push_back(a[2].get(std::to_string(i)));return a[0].call(a[1],args);},{log}));fn=createNativeProxy(fn,handler);
 }
 else if(mode=="mutateLength"){
  arrayLike=JSValue::object();arrayLike.set("length",2);getter(arrayLike,"0",JSValue::function(+[](const JSValue&self,const Args&,const Args&c)->JSValue{push(c[0],"0");auto object=self;object.set("length",0);object.set("1",17);return 12;},{log}));
 }
 else if(mode=="reentrant"){
  getter(arrayLike,"0",JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{push(c[0],"0");c[1].call(c[2],{"INNER",list({9})});return 4;},{log,apply,fn}));
 }
 else if(mode=="prototypeReplaced"){
  fp.set("apply",JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{push(c[0],"replacement");return 91;},{log}));
  out.set("dynamic",fn.get("apply").call(fn,{thisArg,arrayLike}));
 }
 else if(mode=="thisUndefined")thisArg={};else if(mode=="thisNull")thisArg=nullptr;else if(mode=="thisNumber")thisArg=19;
 else if(mode=="thisObject"){thisArg=JSValue::object();thisArg.set("marker",5);}
 else if(mode=="bound")fn=fn.bind("BOUND",{8});
 else if(mode=="callableList"){arrayLike=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"arraylike",2);arrayLike.set("0",6);arrayLike.set("1",7);}
 try{out.set("result",mode=="missing"?apply.call(fn):apply.call(fn,{thisArg,arrayLike}));}catch(...){out.set("error",failure());}
 out.set("events",log);return out;
}
JSValue descriptor(const JSPropertyDescriptor& d){auto out=JSValue::object();out.set("enumerable",d.enumerable.value_or(false));out.set("configurable",d.configurable.value_or(false));if(d.value){out.set("value",normalize(*d.value));out.set("writable",d.writable.value_or(false));}return out;}
JSValue metadata(){
 auto f=JSValue::function(target,{JSValue::array()},"target",0);auto fp=f.getPrototype();const auto apply=fp.get("apply");auto out=JSValue::object();
 auto ad=descriptor(*fp.getOwnPropertyDescriptor("apply"));ad.set("value","#function");out.set("applyProperty",ad);out.set("applyName",descriptor(*apply.getOwnPropertyDescriptor("name")));out.set("applyLength",descriptor(*apply.getOwnPropertyDescriptor("length")));out.set("applySource",apply.nativeFunctionSource());out.set("applyConstructible",apply.isConstructible());
 Args accessors;auto accessorMetadata=JSValue::array();for(const auto* key:{"caller","arguments"}){
  const auto d=*fp.getOwnPropertyDescriptor(key);out.set(key,descriptor(d));for(const auto& fn:{*d.getter,*d.setter}){
   accessors.push_back(fn);auto info=JSValue::object();info.set("name",descriptor(*fn.getOwnPropertyDescriptor("name")));info.set("length",descriptor(*fn.getOwnPropertyDescriptor("length")));info.set("extensible",fn.isExtensible());info.set("constructible",fn.isConstructible());info.set("source",fn.nativeFunctionSource());auto keys=JSValue::array();for(const auto& k:fn.ownPropertyKeys())push(keys,k);info.set("keys",keys);push(accessorMetadata,info);
  }
 }out.set("accessors",accessorMetadata);auto matrix=JSValue::array();for(const auto&a:accessors){auto row=JSValue::array();for(const auto&b:accessors)push(row,a.sameValue(b));push(matrix,row);}out.set("identity",matrix);
 auto errorCases=JSValue::array();auto proxy=createNativeProxy(f,JSValue::object());auto revoked=createNativeProxy(f,JSValue::object());revokeNativeProxy(revoked);
 for(const auto& value:Args{{},nullptr,3,"x",JSValue::object(),fp,apply,f,f.bind({}),proxy,revoked,nativeFunctionConstructor()}){
  auto row=JSValue::array();for(const auto& a:accessors){try{push(row,normalize(a.call(value,{123})));}catch(...){push(row,failure());}}push(errorCases,row);
 }out.set("receivers",errorCases);
 auto sloppy=nativeFunctionConstructor().call();auto sloppyResults=JSValue::array();for(const auto&a:accessors){try{push(sloppyResults,normalize(a.call(sloppy,{123})));}catch(...){push(sloppyResults,failure());}}out.set("sloppy",sloppyResults);
 auto sloppyKeys=JSValue::array();for(const auto&key:sloppy.ownPropertyKeys())push(sloppyKeys,key);out.set("sloppyOwnKeys",sloppyKeys);out.set("sloppyOwnCaller",sloppy.hasOwn("caller"));out.set("sloppyOwnArguments",sloppy.hasOwn("arguments"));out.set("sloppyCallResult",normalize(sloppy.call()));
 auto spoofed=JSValue::function(target,{JSValue::array()});spoofed.set("Function.languageMode","sloppy-empty");auto sloppyProxy=createNativeProxy(sloppy,JSValue::object());auto sloppyBound=sloppy.bind({});auto rejected=JSValue::array();for(const auto&value:Args{spoofed,sloppyProxy,sloppyBound}){auto row=JSValue::array();for(const auto&a:accessors){try{push(row,normalize(a.call(value,{123})));}catch(...){push(row,failure());}}push(rejected,row);}out.set("sloppyBrandRejected",rejected);
 for(const auto& a:accessors){auto mutableAccessor=a;mutableAccessor.set("extra",3);}out.set("accessorsAcceptProperties",accessors[0].get("extra").strictEquals(3));
 auto direct=JSValue::array();for(const auto* key:{"caller","arguments"}){try{push(direct,normalize(f.get(key)));}catch(...){push(direct,failure());}try{f.set(key,123);push(direct,"#undefined");}catch(...){push(direct,failure());}}out.set("directProperties",direct);
 auto replacement=JSValue::object();replacement.set("marker",1);prop(fp,"caller",replacement);out.set("replacedCaller",f.get("caller").sameValue(replacement));fp.erase("arguments");out.set("deletedArguments",f.get("arguments").isUndefined());
 return out;
}
void lifetime(){for(unsigned round=0;round<3;++round){JSWeakValue weakApply,weakList,weakTarget;std::vector<JSWeakValue> weakAccessors;{
 auto keeper=JSValue::object();auto fn=JSValue::function(+[](const JSValue&,const Args&a,const Args&)->JSValue{a[0].collectGarbage();return a[0].get("token");});auto fp=fn.getPrototype();auto apply=fp.get("apply");weakApply=JSWeakValue(apply);weakTarget=JSWeakValue(fn);
 for(const auto* key:{"caller","arguments"}){const auto d=*fp.getOwnPropertyDescriptor(key);weakAccessors.emplace_back(*d.getter);weakAccessors.emplace_back(*d.setter);}
 {
  auto arrayLike=JSValue::object();weakList=JSWeakValue(arrayLike);arrayLike.set("length",2);
  getter(arrayLike,"0",JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{auto transient=JSValue::object();transient.set("token",47);auto state=c[0];state.set("weakCarrier",transient);return transient;},{keeper}));
  getter(arrayLike,"1",JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{auto state=c[0];const auto weak=JSWeakValue(state.get("weakCarrier"));state.erase("weakCarrier");state.collectGarbage();if(weak.expired())throw std::logic_error("Accumulated argument collected during Get");return 2;},{keeper}));
  if(!apply.call(fn,{{},arrayLike}).strictEquals(47))throw std::logic_error("Apply lost argument across collection");
 }
 keeper.collectGarbage();if(!weakList.expired())throw std::logic_error("Apply retained argument list");
 {
  // An abrupt later Get must release the previously accumulated argument.
  auto weak=std::make_shared<JSWeakValue>();auto carrier=JSValue::hostObject(weak,weak.get(),"FunctionApplyWeakCarrier");auto arrayLike=JSValue::object();arrayLike.set("length",2);
  getter(arrayLike,"0",JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{auto value=JSValue::object();value.set("token",71);*std::static_pointer_cast<JSWeakValue>(c[0].hostPayload("FunctionApplyWeakCarrier"))=JSWeakValue(value);return value;},{carrier}));
  getter(arrayLike,"1",JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{c[0].collectGarbage();if(std::static_pointer_cast<JSWeakValue>(c[0].hostPayload("FunctionApplyWeakCarrier"))->expired())throw std::logic_error("Argument lost before abrupt Get");throw JSGeneratorThrown(73);},{carrier}));
  bool threw=false;try{apply.call(fn,{{},arrayLike});}catch(const JSGeneratorThrown&e){threw=e.value().strictEquals(73);}if(!threw)throw std::logic_error("Abrupt Get was swallowed");keeper.collectGarbage();if(!weak->expired())throw std::logic_error("Abrupt apply retained accumulated value");
 }
 if(weakApply.expired())throw std::logic_error("Apply intrinsic collected early");for(const auto&w:weakAccessors)if(w.expired())throw std::logic_error("Accessor collected early");
 }JSValue::nativeSafepoint();if(!weakApply.expired()||!weakTarget.expired()||JSValue::hasActiveHeap())throw std::logic_error("Function apply retained heap");for(const auto&w:weakAccessors)if(!w.expired())throw std::logic_error("Accessor retained heap");}
 std::cout<<"FunctionApply lifetime: 3 passed\n";
}
}
int main(int argc,char**argv){try{const std::string mode=argc>1?argv[1]:"dense";if(mode=="--lifetime")lifetime();else std::cout<<(mode=="metadata"?metadata():run(mode)).toJSON()->dump();}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
