// Native managed source port; pinned three.js 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "renderers/common/DataMap.h"
#include "core/JSGlobal.h"
#include "core/JSModuleObject.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSRegExp.h"
#include "math/JSNumberMath.h"

#include <exception>
#include <string_view>
#include <unordered_set>
namespace DataMapManaged{

using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const JSValue& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
void setReference(JSValue base,const JSValue& key,const JSValue& value){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot assign property of null/undefined");const auto property=key.toPropertyKey();if(base.isObject()){base.set(property,value);return;}auto object=base.toObject();if(!object.reflectSet(property,value,base))throw JSTypeError("Cannot assign primitive property");}
JSValue deleteReference(JSValue base,const JSValue& key){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot delete property of null/undefined");const auto property=key.toPropertyKey();auto object=base.toObject();if(!object.erase(property))throw JSTypeError("Cannot delete nonconfigurable property");return true;}
JSValue argumentObject(const Args& args){auto object=JSValue::object();JSPropertyDescriptor length;length.value=args.size();length.writable=length.configurable=true;object.defineProperty("length",length);for(unsigned i=0;i<args.size();++i)own(object,JSValue(i),args[i]);return object;}
JSValue typeOf(const JSValue& value){if(value.isUndefined())return "undefined";if(value.isCallable())return "function";if(value.isSymbol())return "symbol";switch(value.type()){case JSValue::Type::Boolean:return "boolean";case JSValue::Type::Number:return "number";case JSValue::Type::String:return "string";default:return "object";}}
JSValue binary(std::string_view op,const JSValue& left,const JSValue& right){
 if(op=="in")return right.has(left);if(op=="===")return left.strictEquals(right);if(op=="!==")return !left.strictEquals(right);
 if(op=="+"){const auto a=left.toPrimitive();const auto b=right.toPrimitive();if(a.type()==JSValue::Type::String||b.type()==JSValue::Type::String){const auto x=a.toString();const auto y=b.toString();return x+y;}const auto x=a.toNumber();const auto y=b.toNumber();return x+y;}
 if(op=="<"||op==">"||op=="<="||op==">="){const auto a=left.toPrimitive(JSValue::PreferredType::Number);const auto b=right.toPrimitive(JSValue::PreferredType::Number);if(a.type()==JSValue::Type::String&&b.type()==JSValue::Type::String){const auto x=jsUTF16(a.string()),y=jsUTF16(b.string());if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}const auto x=a.toNumber();const auto y=b.toNumber();if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}
 const auto a=left.toNumber();const auto b=right.toNumber();if(op=="-")return a-b;if(op=="*")return a*b;if(op=="/")return a/b;if(op=="**")return JSNumberMath::pow(a,b);throw std::logic_error("Untranslated arithmetic operator");
}
void appendSpread(Args& values,const JSValue& iterable){auto iterator=getJSIterator(iterable);while(const auto value=stepJSIterator(iterator))values.push_back(*value);}
void copyDataProperties(JSValue target,JSValue source){if(source.isNull()||source.isUndefined())return;if(!source.isObject())source=source.toObject();for(const auto& key:source.ownPropertyKeys()){const auto d=source.getOwnPropertyDescriptor(key);if(d&&d->enumerable.value_or(false)){const auto value=source.get(key);own(target,key,value);}}}
class ForInCursor{JSValue object;std::vector<JSValue> keys;std::unordered_set<std::string> visited;std::size_t index=0;bool loaded=false;public:explicit ForInCursor(JSValue source):object(source.isNull()||source.isUndefined()?JSValue(nullptr):source.toObject()){}std::optional<JSValue> next(){while(!object.isNull()){if(!loaded){keys=object.ownPropertyKeys();index=0;loaded=true;}while(index<keys.size()){const auto key=keys[index++];if(key.isSymbol())continue;const auto name=key.string();if(visited.contains(name))continue;const auto d=object.getOwnPropertyDescriptor(key);if(!d)continue;visited.insert(name);if(d->enumerable.value_or(false))return key;}object=object.getPrototype();loaded=false;}return std::nullopt;}};

JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
([&]()->JSValue{auto nativeStatementTemp6=nativeStatementTemp4;const auto nativeStatementTemp7=JSValue("data");const auto nativeStatementTemp8=([&]()->JSValue{const auto nativeStatementTemp11=nativeGlobalBinding("WeakMap");Args nativeStatementTemp12;return nativeStatementTemp11.construct(nativeStatementTemp12);}());setReference(nativeStatementTemp6,nativeStatementTemp7,nativeStatementTemp8);return nativeStatementTemp8;}());
return JSValue{};
}

JSValue method1(const JSValue& nativeStatementTemp13,const Args& nativeStatementTemp14,const Args& nativeStatementTemp15){
auto nativeStatementTemp16=nativeStatementTemp15[0];auto nativeStatementTemp17=nativeStatementTemp13;
JSValue object=arg(nativeStatementTemp14,0);
JSValue map=([&]()->JSValue{auto nativeStatementTemp22=([&]()->JSValue{const auto nativeStatementTemp24=nativeStatementTemp17;const auto nativeStatementTemp25=JSValue("data");return nativeStatementTemp24.get(nativeStatementTemp25);}());const auto nativeStatementTemp23=JSValue("get");const auto nativeStatementTemp19=nativeStatementTemp22.get(nativeStatementTemp23);const auto nativeStatementTemp20=nativeStatementTemp22;Args nativeStatementTemp21;nativeStatementTemp21.push_back(object);return nativeStatementTemp19.call(nativeStatementTemp20,nativeStatementTemp21);}());
if(([&]()->JSValue{const auto nativeStatementTemp26=map;const auto nativeStatementTemp27=JSValue{};return binary("===",nativeStatementTemp26,nativeStatementTemp27);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp28=([&]()->JSValue{auto nativeStatementTemp31=JSValue::object();return nativeStatementTemp31;}());map=nativeStatementTemp28;return nativeStatementTemp28;}());
([&]()->JSValue{auto nativeStatementTemp35=([&]()->JSValue{const auto nativeStatementTemp37=nativeStatementTemp17;const auto nativeStatementTemp38=JSValue("data");return nativeStatementTemp37.get(nativeStatementTemp38);}());const auto nativeStatementTemp36=JSValue("set");const auto nativeStatementTemp32=nativeStatementTemp35.get(nativeStatementTemp36);const auto nativeStatementTemp33=nativeStatementTemp35;Args nativeStatementTemp34;nativeStatementTemp34.push_back(object);nativeStatementTemp34.push_back(map);return nativeStatementTemp32.call(nativeStatementTemp33,nativeStatementTemp34);}());

}
return map;
return JSValue{};
}

JSValue method2(const JSValue& nativeStatementTemp39,const Args& nativeStatementTemp40,const Args& nativeStatementTemp41){
auto nativeStatementTemp42=nativeStatementTemp41[0];auto nativeStatementTemp43=nativeStatementTemp39;
JSValue object=arg(nativeStatementTemp40,0);
JSValue map=JSValue(nullptr);
if(([&]()->JSValue{auto nativeStatementTemp48=([&]()->JSValue{const auto nativeStatementTemp50=nativeStatementTemp43;const auto nativeStatementTemp51=JSValue("data");return nativeStatementTemp50.get(nativeStatementTemp51);}());const auto nativeStatementTemp49=JSValue("has");const auto nativeStatementTemp45=nativeStatementTemp48.get(nativeStatementTemp49);const auto nativeStatementTemp46=nativeStatementTemp48;Args nativeStatementTemp47;nativeStatementTemp47.push_back(object);return nativeStatementTemp45.call(nativeStatementTemp46,nativeStatementTemp47);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp52=([&]()->JSValue{auto nativeStatementTemp58=([&]()->JSValue{const auto nativeStatementTemp60=nativeStatementTemp43;const auto nativeStatementTemp61=JSValue("data");return nativeStatementTemp60.get(nativeStatementTemp61);}());const auto nativeStatementTemp59=JSValue("get");const auto nativeStatementTemp55=nativeStatementTemp58.get(nativeStatementTemp59);const auto nativeStatementTemp56=nativeStatementTemp58;Args nativeStatementTemp57;nativeStatementTemp57.push_back(object);return nativeStatementTemp55.call(nativeStatementTemp56,nativeStatementTemp57);}());map=nativeStatementTemp52;return nativeStatementTemp52;}());
([&]()->JSValue{auto nativeStatementTemp65=([&]()->JSValue{const auto nativeStatementTemp67=nativeStatementTemp43;const auto nativeStatementTemp68=JSValue("data");return nativeStatementTemp67.get(nativeStatementTemp68);}());const auto nativeStatementTemp66=JSValue("delete");const auto nativeStatementTemp62=nativeStatementTemp65.get(nativeStatementTemp66);const auto nativeStatementTemp63=nativeStatementTemp65;Args nativeStatementTemp64;nativeStatementTemp64.push_back(object);return nativeStatementTemp62.call(nativeStatementTemp63,nativeStatementTemp64);}());

}
return map;
return JSValue{};
}

JSValue method3(const JSValue& nativeStatementTemp69,const Args& nativeStatementTemp70,const Args& nativeStatementTemp71){
auto nativeStatementTemp72=nativeStatementTemp71[0];auto nativeStatementTemp73=nativeStatementTemp69;
JSValue object=arg(nativeStatementTemp70,0);
return ([&]()->JSValue{auto nativeStatementTemp78=([&]()->JSValue{const auto nativeStatementTemp80=nativeStatementTemp73;const auto nativeStatementTemp81=JSValue("data");return nativeStatementTemp80.get(nativeStatementTemp81);}());const auto nativeStatementTemp79=JSValue("has");const auto nativeStatementTemp75=nativeStatementTemp78.get(nativeStatementTemp79);const auto nativeStatementTemp76=nativeStatementTemp78;Args nativeStatementTemp77;nativeStatementTemp77.push_back(object);return nativeStatementTemp75.call(nativeStatementTemp76,nativeStatementTemp77);}());
return JSValue{};
}

JSValue method4(const JSValue& nativeStatementTemp82,const Args& nativeStatementTemp83,const Args& nativeStatementTemp84){
auto nativeStatementTemp85=nativeStatementTemp84[0];auto nativeStatementTemp86=nativeStatementTemp82;
([&]()->JSValue{auto nativeStatementTemp88=nativeStatementTemp86;const auto nativeStatementTemp89=JSValue("data");const auto nativeStatementTemp90=([&]()->JSValue{const auto nativeStatementTemp93=nativeGlobalBinding("WeakMap");Args nativeStatementTemp94;return nativeStatementTemp93.construct(nativeStatementTemp94);}());setReference(nativeStatementTemp88,nativeStatementTemp89,nativeStatementTemp90);return nativeStatementTemp90;}());
return JSValue{};
}
}
JSValue nativeDataMapModule(){using namespace DataMapManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"DataMap module");auto module=JSValue::object(nullptr);
auto ctor=JSValue::constructor(method0,{module},"DataMap",0);ctor.setFunctionSourceText("class DataMap {\r\n\r\n\t/**\r\n\t * Constructs a new data map.\r\n\t */\r\n\tconstructor() {\r\n\r\n\t\t/**\r\n\t\t * `DataMap` internally uses a weak map\r\n\t\t * to manage its data.\r\n\t\t *\r\n\t\t * @type {WeakMap<Object, Object>}\r\n\t\t */\r\n\t\tthis.data = new WeakMap();\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns the dictionary for the given object.\r\n\t *\r\n\t * @param {Object} object - The object.\r\n\t * @return {Object} The dictionary.\r\n\t */\r\n\tget( object ) {\r\n\r\n\t\tlet map = this.data.get( object );\r\n\r\n\t\tif ( map === undefined ) {\r\n\r\n\t\t\tmap = {};\r\n\t\t\tthis.data.set( object, map );\r\n\r\n\t\t}\r\n\r\n\t\treturn map;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Deletes the dictionary for the given object.\r\n\t *\r\n\t * @param {Object} object - The object.\r\n\t * @return {?Object} The deleted dictionary.\r\n\t */\r\n\tdelete( object ) {\r\n\r\n\t\tlet map = null;\r\n\r\n\t\tif ( this.data.has( object ) ) {\r\n\r\n\t\t\tmap = this.data.get( object );\r\n\r\n\t\t\tthis.data.delete( object );\r\n\r\n\t\t}\r\n\r\n\t\treturn map;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if the given object has a dictionary defined.\r\n\t *\r\n\t * @param {Object} object - The object to test.\r\n\t * @return {boolean} Whether a dictionary is defined or not.\r\n\t */\r\n\thas( object ) {\r\n\r\n\t\treturn this.data.has( object );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Frees internal resources.\r\n\t */\r\n\tdispose() {\r\n\r\n\t\tthis.data = new WeakMap();\r\n\r\n\t}\r\n\r\n}");auto prototype=ctor.get("prototype");own(module,"DataMap",ctor);own(module,"_home",prototype);
{auto function=JSValue::function(method1,{module},"get",1);function.setFunctionSourceText("get( object ) {\r\n\r\n\t\tlet map = this.data.get( object );\r\n\r\n\t\tif ( map === undefined ) {\r\n\r\n\t\t\tmap = {};\r\n\t\t\tthis.data.set( object, map );\r\n\r\n\t\t}\r\n\r\n\t\treturn map;\r\n\r\n\t}");JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("get",d);}
{auto function=JSValue::function(method2,{module},"delete",1);function.setFunctionSourceText("delete( object ) {\r\n\r\n\t\tlet map = null;\r\n\r\n\t\tif ( this.data.has( object ) ) {\r\n\r\n\t\t\tmap = this.data.get( object );\r\n\r\n\t\t\tthis.data.delete( object );\r\n\r\n\t\t}\r\n\r\n\t\treturn map;\r\n\r\n\t}");JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("delete",d);}
{auto function=JSValue::function(method3,{module},"has",1);function.setFunctionSourceText("has( object ) {\r\n\r\n\t\treturn this.data.has( object );\r\n\r\n\t}");JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("has",d);}
{auto function=JSValue::function(method4,{module},"dispose",0);function.setFunctionSourceText("dispose() {\r\n\r\n\t\tthis.data = new WeakMap();\r\n\r\n\t}");JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("dispose",d);}
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();own(facade,"default",ctor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeDataMapBinding(const std::string& name){const auto keeper=nativeDataMapModule();return name=="default"?DataMapManaged::lexical().lock()->get("DataMap"):JSValue{};}
JSValue nativeDataMapConstructor(){return nativeDataMapBinding("default");}
