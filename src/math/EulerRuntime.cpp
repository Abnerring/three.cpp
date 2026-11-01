#include "math/Euler.h"
#include "math/Quaternion.h"
#include "math/Matrix4.h"
#include "math/MathUtils.h"
#include "core/JSGenerator.h"
#include "core/JSGlobal.h"
#include "core/JSModuleObject.h"
#include "utils.h"
#include "math/EulerRuntimeSource.inc"
#include <string_view>

namespace EulerManaged {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
JSPropertyDescriptor data(JSValue value,bool enumerable=false){JSPropertyDescriptor d;d.value=std::move(value);d.writable=d.configurable=true;d.enumerable=enumerable;return d;}
void own(JSValue value,const char* key,const JSValue& field){value.defineProperty(key,data(field,true));}
void require(const JSValue& value){if(value.isUndefined()||value.isNull())throw JSTypeError("Cannot access property of null/undefined");}
template<class F>void put(JSValue self,const char* key,F evaluate){const auto value=evaluate();self.set(key,value);}
JSValue add(const JSValue& a,const JSValue& b){const auto left=a.toPrimitive(),right=b.toPrimitive();if(left.type()==JSValue::Type::String||right.type()==JSValue::Type::String)return left.toString()+right.toString();return left.toNumber()+right.toNumber();}
struct Call{JSValue self,method;JSValue operator()(const Args& args)const{return method.call(self,args);}};
Call invoke(const JSValue& self,const char* key){return{self,self.get(key)};}
Call math(const char* key){const auto object=nativeGlobalBinding("Math");return invoke(object,key);}
JSWeakValue& stateCache(){static thread_local JSWeakValue cache;return cache;}
JSValue rotation(JSValue self,const Args& args,const JSValue& state){
 const auto matrix=arg(args,0),rawOrder=arg(args,1);const auto order=rawOrder.isUndefined()?self.get("_order"):rawOrder;
 const auto rawUpdate=arg(args,2),update=rawUpdate.isUndefined()?JSValue(true):rawUpdate;
 const auto te=matrix.get("elements");
 const auto m11=te.get("0"),m12=te.get("4"),m13=te.get("8");
 const auto m21=te.get("1"),m22=te.get("5"),m23=te.get("9");
 const auto m31=te.get("2"),m32=te.get("6"),m33=te.get("10");
 const auto asin=[&](const JSValue& value,bool negative){const auto call=math("asin");const auto clamped=state.getInternalSlot("clamp").call({}, {value,-1,1});return call({negative?JSValue(-clamped.toNumber()):clamped});};
 const auto small=[&](const JSValue& value){const auto call=math("abs");return call({value}).toNumber()<0.9999999;};
 const auto atan=[&](const JSValue& a,const JSValue& b,bool negative=false){const auto call=math("atan2");const auto left=negative?JSValue(-a.toNumber()):a;return call({left,b});};
 if(order.strictEquals("XYZ")){
  put(self,"_y",[&]{return asin(m13,false);});
  if(small(m13)){put(self,"_x",[&]{return atan(m23,m33,true);});put(self,"_z",[&]{return atan(m12,m11,true);});}
  else{put(self,"_x",[&]{return atan(m32,m22);});put(self,"_z",[]{return JSValue(0);});}
 }else if(order.strictEquals("YXZ")){
  put(self,"_x",[&]{return asin(m23,true);});
  if(small(m23)){put(self,"_y",[&]{return atan(m13,m33);});put(self,"_z",[&]{return atan(m21,m22);});}
  else{put(self,"_y",[&]{return atan(m31,m11,true);});put(self,"_z",[]{return JSValue(0);});}
 }else if(order.strictEquals("ZXY")){
  put(self,"_x",[&]{return asin(m32,false);});
  if(small(m32)){put(self,"_y",[&]{return atan(m31,m33,true);});put(self,"_z",[&]{return atan(m12,m22,true);});}
  else{put(self,"_y",[]{return JSValue(0);});put(self,"_z",[&]{return atan(m21,m11);});}
 }else if(order.strictEquals("ZYX")){
  put(self,"_y",[&]{return asin(m31,true);});
  if(small(m31)){put(self,"_x",[&]{return atan(m32,m33);});put(self,"_z",[&]{return atan(m21,m11);});}
  else{put(self,"_x",[]{return JSValue(0);});put(self,"_z",[&]{return atan(m12,m22,true);});}
 }else if(order.strictEquals("YZX")){
  put(self,"_z",[&]{return asin(m21,false);});
  if(small(m21)){put(self,"_x",[&]{return atan(m23,m22,true);});put(self,"_y",[&]{return atan(m31,m11,true);});}
  else{put(self,"_x",[]{return JSValue(0);});put(self,"_y",[&]{return atan(m13,m33);});}
 }else if(order.strictEquals("XZY")){
  put(self,"_z",[&]{return asin(m12,true);});
  if(small(m12)){put(self,"_x",[&]{return atan(m32,m22);});put(self,"_y",[&]{return atan(m13,m11);});}
  else{put(self,"_x",[&]{return atan(m23,m33,true);});put(self,"_y",[]{return JSValue(0);});}
 }else ThreeUtils::warn({add("Euler: .setFromRotationMatrix() encountered an unknown order: ",order)});
 self.set("_order",order);if(update.strictEquals(true))invoke(self,"_onChangeCallback")({});return self;
}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
 auto self=receiver;const auto name=captures[0].string(),kind=captures[1].string();const auto state=captures[2];
 if(kind=="get")return self.get(name=="order"?"_order":"_"+name);
 if(kind=="set"){self.set(name=="order"?"_order":"_"+name,arg(args,0));invoke(self,"_onChangeCallback")({});return {};}
 if(name=="_onChangeCallback")return {};
 if(name=="set"){
  const auto order=arg(args,3).isUndefined()?self.get("_order"):arg(args,3);
  self.set("_x",arg(args,0));self.set("_y",arg(args,1));self.set("_z",arg(args,2));self.set("_order",order);invoke(self,"_onChangeCallback")({});return self;
 }
 if(name=="clone"){const auto ctor=self.get("constructor");const auto x=self.get("_x"),y=self.get("_y"),z=self.get("_z"),order=self.get("_order");return ctor.construct({x,y,z,order});}
 if(name=="copy"){
  const auto value=arg(args,0);for(const auto key:{"_x","_y","_z","_order"})put(self,key,[&]{return value.get(key);});invoke(self,"_onChangeCallback")({});return self;
 }
 if(name=="setFromRotationMatrix")return rotation(self,args,state);
 if(name=="setFromQuaternion"){
  const auto matrix=state.getInternalSlot("_matrix");invoke(matrix,"makeRotationFromQuaternion")({arg(args,0)});const auto call=invoke(self,"setFromRotationMatrix");return call({matrix,arg(args,1),arg(args,2)});
 }
 if(name=="setFromVector3"){
  const auto order=arg(args,1).isUndefined()?self.get("_order"):arg(args,1);const auto call=invoke(self,"set");const auto value=arg(args,0);const auto x=value.get("x"),y=value.get("y"),z=value.get("z");return call({x,y,z,order});
 }
 if(name=="reorder"){
  const auto quaternion=state.getInternalSlot("_quaternion");invoke(quaternion,"setFromEuler")({self});return invoke(self,"setFromQuaternion")({quaternion,arg(args,0)});
 }
 if(name=="equals"){
  const auto value=arg(args,0);for(const auto key:{"_x","_y","_z","_order"}){const auto left=value.get(key);if(!left.strictEquals(self.get(key)))return false;}return true;
 }
 if(name=="fromArray"){
  const auto value=arg(args,0);put(self,"_x",[&]{return value.get("0");});put(self,"_y",[&]{return value.get("1");});put(self,"_z",[&]{return value.get("2");});if(!value.get("3").isUndefined())put(self,"_order",[&]{return value.get("3");});invoke(self,"_onChangeCallback")({});return self;
 }
 if(name=="toArray"){
  auto array=arg(args,0);if(array.isUndefined())array=JSValue::array();const auto offset=arg(args,1).isUndefined()?JSValue(0):arg(args,1);const char* fields[]={"_x","_y","_z","_order"};
  for(unsigned i=0;i<4;++i){const auto expression=i?add(offset,i):offset;const auto value=self.get(fields[i]);require(array);const auto key=expression.toPropertyKey();array.set(key,value);}return array;
 }
 if(name=="_onChange"){self.set("_onChangeCallback",arg(args,0));return self;}
 throw std::logic_error("Unknown managed Euler method: "+name);
}
JSGeneratorStep iterate(const JSValue& original,const JSGeneratorCompletion& c,const JSValue& receiver,const Args&,const Args&){
 if(c.kind==JSGeneratorCompletionKind::Return)return JSGeneratorStep::complete(c.value);if(c.kind==JSGeneratorCompletionKind::Throw)return JSGeneratorStep::abrupt(c.value);
 auto state=original;const auto value=state.get("pc");const auto index=value.isUndefined()?0:static_cast<unsigned>(value.number());state.set("pc",index+1);const char* fields[]={"_x","_y","_z","_order"};return index<4?JSGeneratorStep::yield(receiver.get(fields[index])):JSGeneratorStep::complete();
}
JSValue state(){
 if(const auto cached=stateCache().lock();cached&&cached->isObject())return *cached;
 static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Euler module");
 const auto quaternion=nativeQuaternionConstructor(),matrix=nativeMatrix4Constructor(),clamp=nativeMathUtilsClamp();
 auto state=JSValue::object(nullptr);state.setInternalSlot("clamp",clamp);state.setInternalSlot("_matrix",matrix.construct());state.setInternalSlot("_quaternion",quaternion.construct());
 auto ctor=JSValue::constructor(+[](const JSValue& receiver,const Args& args,const Args& captures)->JSValue{
  auto self=receiver;const auto x=arg(args,0).isUndefined()?JSValue(0):arg(args,0),y=arg(args,1).isUndefined()?JSValue(0):arg(args,1),z=arg(args,2).isUndefined()?JSValue(0):arg(args,2);
  const auto order=arg(args,3).isUndefined()?captures[0].getInternalSlot("Euler").get("DEFAULT_ORDER"):arg(args,3);
  self.set("isEuler",true);self.set("_x",x);self.set("_y",y);self.set("_z",z);self.set("_order",order);return {};
 },{state},"Euler",0);ctor.setFunctionSourceText(EulerRuntimeSource::classSource);state.setInternalSlot("Euler",ctor);
 auto prototype=ctor.get("prototype");
 for(const auto& entry:EulerRuntimeSource::entries){if(std::string_view(entry.kind)=="constructor")continue;const bool generator=entry.generator;auto function=generator?createNativeGeneratorFunction(iterate,{},entry.name,entry.arity,entry.source):JSValue::function(method,{entry.name,entry.kind,state},std::string(entry.kind)=="get"?"get "+std::string(entry.name):std::string(entry.kind)=="set"?"set "+std::string(entry.name):entry.name,entry.arity);function.setFunctionSourceText(entry.source);JSPropertyDescriptor descriptor;descriptor.configurable=true;if(std::string_view(entry.kind)=="get")descriptor.getter=function;else if(std::string_view(entry.kind)=="set")descriptor.setter=function;else{descriptor.value=function;descriptor.writable=true;}if(generator)prototype.defineProperty(JSValue::wellKnownSymbol("iterator"),descriptor);else prototype.defineProperty(entry.name,descriptor);}
 ctor.set("DEFAULT_ORDER","XYZ");auto module=JSValue::object();own(module,"Euler",ctor);state.setInternalSlot("module",module);state.retainForHeapLifetime();stateCache()=JSWeakValue(state);return state;
}
}
JSValue nativeEulerModule(){return EulerManaged::state().getInternalSlot("module");}
JSValue nativeEulerBinding(const std::string& name){const auto state=EulerManaged::state();return name=="Euler"?state.getInternalSlot("Euler"):JSValue{};}
JSValue nativeEulerConstructor(){return nativeEulerBinding("Euler");}
