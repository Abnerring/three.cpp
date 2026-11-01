#include "core/EventDispatcher.h"
#include <algorithm>
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "math/JSNumberMath.h"
#include "core/JSStringEncoding.h"
#include <unordered_set>
#include <string_view>


namespace EventDispatcherObject3DDependency{

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

JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
JSValue type=arg(nativeStatementTemp1,0);
JSValue listener=arg(nativeStatementTemp1,1);
if(([&]()->JSValue{const auto nativeStatementTemp6=([&]()->JSValue{const auto nativeStatementTemp8=nativeStatementTemp4;const auto nativeStatementTemp9=JSValue("_listeners");return nativeStatementTemp8.get(nativeStatementTemp9);}());const auto nativeStatementTemp7=JSValue{};return binary("===",nativeStatementTemp6,nativeStatementTemp7);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp10=nativeStatementTemp4;const auto nativeStatementTemp11=JSValue("_listeners");const auto nativeStatementTemp12=([&]()->JSValue{auto nativeStatementTemp15=JSValue::object();return nativeStatementTemp15;}());setReference(nativeStatementTemp10,nativeStatementTemp11,nativeStatementTemp12);return nativeStatementTemp12;}());
JSValue listeners=([&]()->JSValue{const auto nativeStatementTemp16=nativeStatementTemp4;const auto nativeStatementTemp17=JSValue("_listeners");return nativeStatementTemp16.get(nativeStatementTemp17);}());
if(([&]()->JSValue{const auto nativeStatementTemp18=([&]()->JSValue{const auto nativeStatementTemp20=listeners;const auto nativeStatementTemp21=type;return nativeStatementTemp20.get(nativeStatementTemp21);}());const auto nativeStatementTemp19=JSValue{};return binary("===",nativeStatementTemp18,nativeStatementTemp19);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp22=listeners;const auto nativeStatementTemp23=type;const auto nativeStatementTemp24=([&]()->JSValue{auto nativeStatementTemp27=JSValue::array();return nativeStatementTemp27;}());setReference(nativeStatementTemp22,nativeStatementTemp23,nativeStatementTemp24);return nativeStatementTemp24;}());

}
if(([&]()->JSValue{const auto nativeStatementTemp28=([&]()->JSValue{auto nativeStatementTemp33=([&]()->JSValue{const auto nativeStatementTemp35=listeners;const auto nativeStatementTemp36=type;return nativeStatementTemp35.get(nativeStatementTemp36);}());const auto nativeStatementTemp34=JSValue("indexOf");const auto nativeStatementTemp30=nativeStatementTemp33.get(nativeStatementTemp34);const auto nativeStatementTemp31=nativeStatementTemp33;Args nativeStatementTemp32;nativeStatementTemp32.push_back(listener);return nativeStatementTemp30.call(nativeStatementTemp31,nativeStatementTemp32);}());const auto nativeStatementTemp29=([&]()->JSValue{const auto nativeStatementTemp37=JSValue(1);return -nativeStatementTemp37.toNumber();}());return binary("===",nativeStatementTemp28,nativeStatementTemp29);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp41=([&]()->JSValue{const auto nativeStatementTemp43=listeners;const auto nativeStatementTemp44=type;return nativeStatementTemp43.get(nativeStatementTemp44);}());const auto nativeStatementTemp42=JSValue("push");const auto nativeStatementTemp38=nativeStatementTemp41.get(nativeStatementTemp42);const auto nativeStatementTemp39=nativeStatementTemp41;Args nativeStatementTemp40;nativeStatementTemp40.push_back(listener);return nativeStatementTemp38.call(nativeStatementTemp39,nativeStatementTemp40);}());

}
return JSValue{};
}
JSValue method1(const JSValue& nativeStatementTemp45,const Args& nativeStatementTemp46,const Args& nativeStatementTemp47){
auto nativeStatementTemp48=nativeStatementTemp47[0];auto nativeStatementTemp49=nativeStatementTemp45;
JSValue type=arg(nativeStatementTemp46,0);
JSValue listener=arg(nativeStatementTemp46,1);
JSValue listeners=([&]()->JSValue{const auto nativeStatementTemp51=nativeStatementTemp49;const auto nativeStatementTemp52=JSValue("_listeners");return nativeStatementTemp51.get(nativeStatementTemp52);}());
if(([&]()->JSValue{const auto nativeStatementTemp53=listeners;const auto nativeStatementTemp54=JSValue{};return binary("===",nativeStatementTemp53,nativeStatementTemp54);}()).toBoolean())return JSValue(false);
return ([&]()->JSValue{const auto nativeStatementTemp55=([&]()->JSValue{const auto nativeStatementTemp56=([&]()->JSValue{const auto nativeStatementTemp58=listeners;const auto nativeStatementTemp59=type;return nativeStatementTemp58.get(nativeStatementTemp59);}());const auto nativeStatementTemp57=JSValue{};return binary("!==",nativeStatementTemp56,nativeStatementTemp57);}());if(!nativeStatementTemp55.toBoolean())return nativeStatementTemp55;return ([&]()->JSValue{const auto nativeStatementTemp60=([&]()->JSValue{auto nativeStatementTemp65=([&]()->JSValue{const auto nativeStatementTemp67=listeners;const auto nativeStatementTemp68=type;return nativeStatementTemp67.get(nativeStatementTemp68);}());const auto nativeStatementTemp66=JSValue("indexOf");const auto nativeStatementTemp62=nativeStatementTemp65.get(nativeStatementTemp66);const auto nativeStatementTemp63=nativeStatementTemp65;Args nativeStatementTemp64;nativeStatementTemp64.push_back(listener);return nativeStatementTemp62.call(nativeStatementTemp63,nativeStatementTemp64);}());const auto nativeStatementTemp61=([&]()->JSValue{const auto nativeStatementTemp69=JSValue(1);return -nativeStatementTemp69.toNumber();}());return binary("!==",nativeStatementTemp60,nativeStatementTemp61);}());}());
return JSValue{};
}
JSValue method2(const JSValue& nativeStatementTemp70,const Args& nativeStatementTemp71,const Args& nativeStatementTemp72){
auto nativeStatementTemp73=nativeStatementTemp72[0];auto nativeStatementTemp74=nativeStatementTemp70;
JSValue type=arg(nativeStatementTemp71,0);
JSValue listener=arg(nativeStatementTemp71,1);
JSValue listeners=([&]()->JSValue{const auto nativeStatementTemp76=nativeStatementTemp74;const auto nativeStatementTemp77=JSValue("_listeners");return nativeStatementTemp76.get(nativeStatementTemp77);}());
if(([&]()->JSValue{const auto nativeStatementTemp78=listeners;const auto nativeStatementTemp79=JSValue{};return binary("===",nativeStatementTemp78,nativeStatementTemp79);}()).toBoolean())return JSValue{};
JSValue listenerArray=([&]()->JSValue{const auto nativeStatementTemp80=listeners;const auto nativeStatementTemp81=type;return nativeStatementTemp80.get(nativeStatementTemp81);}());
if(([&]()->JSValue{const auto nativeStatementTemp82=listenerArray;const auto nativeStatementTemp83=JSValue{};return binary("!==",nativeStatementTemp82,nativeStatementTemp83);}()).toBoolean()){
JSValue index=([&]()->JSValue{auto nativeStatementTemp87=listenerArray;const auto nativeStatementTemp88=JSValue("indexOf");const auto nativeStatementTemp84=nativeStatementTemp87.get(nativeStatementTemp88);const auto nativeStatementTemp85=nativeStatementTemp87;Args nativeStatementTemp86;nativeStatementTemp86.push_back(listener);return nativeStatementTemp84.call(nativeStatementTemp85,nativeStatementTemp86);}());
if(([&]()->JSValue{const auto nativeStatementTemp89=index;const auto nativeStatementTemp90=([&]()->JSValue{const auto nativeStatementTemp91=JSValue(1);return -nativeStatementTemp91.toNumber();}());return binary("!==",nativeStatementTemp89,nativeStatementTemp90);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp95=listenerArray;const auto nativeStatementTemp96=JSValue("splice");const auto nativeStatementTemp92=nativeStatementTemp95.get(nativeStatementTemp96);const auto nativeStatementTemp93=nativeStatementTemp95;Args nativeStatementTemp94;nativeStatementTemp94.push_back(index);nativeStatementTemp94.push_back(JSValue(1));return nativeStatementTemp92.call(nativeStatementTemp93,nativeStatementTemp94);}());

}

}
return JSValue{};
}
JSValue method3(const JSValue& nativeStatementTemp97,const Args& nativeStatementTemp98,const Args& nativeStatementTemp99){
auto nativeStatementTemp100=nativeStatementTemp99[0];auto nativeStatementTemp101=nativeStatementTemp97;
JSValue event=arg(nativeStatementTemp98,0);
JSValue listeners=([&]()->JSValue{const auto nativeStatementTemp103=nativeStatementTemp101;const auto nativeStatementTemp104=JSValue("_listeners");return nativeStatementTemp103.get(nativeStatementTemp104);}());
if(([&]()->JSValue{const auto nativeStatementTemp105=listeners;const auto nativeStatementTemp106=JSValue{};return binary("===",nativeStatementTemp105,nativeStatementTemp106);}()).toBoolean())return JSValue{};
JSValue listenerArray=([&]()->JSValue{const auto nativeStatementTemp107=listeners;const auto nativeStatementTemp108=([&]()->JSValue{const auto nativeStatementTemp109=event;const auto nativeStatementTemp110=JSValue("type");return nativeStatementTemp109.get(nativeStatementTemp110);}());return nativeStatementTemp107.get(nativeStatementTemp108);}());
if(([&]()->JSValue{const auto nativeStatementTemp111=listenerArray;const auto nativeStatementTemp112=JSValue{};return binary("!==",nativeStatementTemp111,nativeStatementTemp112);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp113=event;const auto nativeStatementTemp114=JSValue("target");const auto nativeStatementTemp115=nativeStatementTemp101;setReference(nativeStatementTemp113,nativeStatementTemp114,nativeStatementTemp115);return nativeStatementTemp115;}());
JSValue array=([&]()->JSValue{auto nativeStatementTemp121=listenerArray;const auto nativeStatementTemp122=JSValue("slice");const auto nativeStatementTemp118=nativeStatementTemp121.get(nativeStatementTemp122);const auto nativeStatementTemp119=nativeStatementTemp121;Args nativeStatementTemp120;nativeStatementTemp120.push_back(JSValue(0));return nativeStatementTemp118.call(nativeStatementTemp119,nativeStatementTemp120);}());
{JSValue i=JSValue(0);
JSValue l=([&]()->JSValue{const auto nativeStatementTemp123=array;const auto nativeStatementTemp124=JSValue("length");return nativeStatementTemp123.get(nativeStatementTemp124);}());
for(;([&]()->JSValue{const auto nativeStatementTemp125=i;const auto nativeStatementTemp126=l;return binary("<",nativeStatementTemp125,nativeStatementTemp126);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp127=i.toNumber();const auto nativeStatementTemp128=nativeStatementTemp127+1;i=nativeStatementTemp128;return nativeStatementTemp127;}())){
([&]()->JSValue{auto nativeStatementTemp132=([&]()->JSValue{const auto nativeStatementTemp134=array;const auto nativeStatementTemp135=i;return nativeStatementTemp134.get(nativeStatementTemp135);}());const auto nativeStatementTemp133=JSValue("call");const auto nativeStatementTemp129=nativeStatementTemp132.get(nativeStatementTemp133);const auto nativeStatementTemp130=nativeStatementTemp132;Args nativeStatementTemp131;nativeStatementTemp131.push_back(nativeStatementTemp101);nativeStatementTemp131.push_back(event);return nativeStatementTemp129.call(nativeStatementTemp130,nativeStatementTemp131);}());

}
}
([&]()->JSValue{auto nativeStatementTemp136=event;const auto nativeStatementTemp137=JSValue("target");const auto nativeStatementTemp138=JSValue(nullptr);setReference(nativeStatementTemp136,nativeStatementTemp137,nativeStatementTemp138);return nativeStatementTemp138;}());

}
return JSValue{};
}
}
JSValue EventDispatcher::runtimeConstructor(){
using namespace EventDispatcherObject3DDependency;static thread_local JSWeakValue weak;if(const auto existing=weak.lock();existing&&existing->isConstructible())return *existing;auto module=JSValue::object(nullptr);auto ctor=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"EventDispatcher",0);
ctor.setFunctionSourceText("class EventDispatcher {\r\n\r\n\t/**\r\n\t * Adds the given event listener to the given event type.\r\n\t *\r\n\t * @param {string} type - The type of event to listen to.\r\n\t * @param {Function} listener - The function that gets called when the event is fired.\r\n\t */\r\n\taddEventListener( type, listener ) {\r\n\r\n\t\tif ( this._listeners === undefined ) this._listeners = {};\r\n\r\n\t\tconst listeners = this._listeners;\r\n\r\n\t\tif ( listeners[ type ] === undefined ) {\r\n\r\n\t\t\tlisteners[ type ] = [];\r\n\r\n\t\t}\r\n\r\n\t\tif ( listeners[ type ].indexOf( listener ) === - 1 ) {\r\n\r\n\t\t\tlisteners[ type ].push( listener );\r\n\r\n\t\t}\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if the given event listener has been added to the given event type.\r\n\t *\r\n\t * @param {string} type - The type of event.\r\n\t * @param {Function} listener - The listener to check.\r\n\t * @return {boolean} Whether the given event listener has been added to the given event type.\r\n\t */\r\n\thasEventListener( type, listener ) {\r\n\r\n\t\tconst listeners = this._listeners;\r\n\r\n\t\tif ( listeners === undefined ) return false;\r\n\r\n\t\treturn listeners[ type ] !== undefined && listeners[ type ].indexOf( listener ) !== - 1;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Removes the given event listener from the given event type.\r\n\t *\r\n\t * @param {string} type - The type of event.\r\n\t * @param {Function} listener - The listener to remove.\r\n\t */\r\n\tremoveEventListener( type, listener ) {\r\n\r\n\t\tconst listeners = this._listeners;\r\n\r\n\t\tif ( listeners === undefined ) return;\r\n\r\n\t\tconst listenerArray = listeners[ type ];\r\n\r\n\t\tif ( listenerArray !== undefined ) {\r\n\r\n\t\t\tconst index = listenerArray.indexOf( listener );\r\n\r\n\t\t\tif ( index !== - 1 ) {\r\n\r\n\t\t\t\tlistenerArray.splice( index, 1 );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Dispatches an event object.\r\n\t *\r\n\t * @param {Object} event - The event that gets fired.\r\n\t */\r\n\tdispatchEvent( event ) {\r\n\r\n\t\tconst listeners = this._listeners;\r\n\r\n\t\tif ( listeners === undefined ) return;\r\n\r\n\t\tconst listenerArray = listeners[ event.type ];\r\n\r\n\t\tif ( listenerArray !== undefined ) {\r\n\r\n\t\t\tevent.target = this;\r\n\r\n\t\t\t// Make a copy, in case listeners are removed while iterating.\r\n\t\t\tconst array = listenerArray.slice( 0 );\r\n\r\n\t\t\tfor ( let i = 0, l = array.length; i < l; i ++ ) {\r\n\r\n\t\t\t\tarray[ i ].call( this, event );\r\n\r\n\t\t\t}\r\n\r\n\t\t\tevent.target = null;\r\n\r\n\t\t}\r\n\r\n\t}\r\n\r\n}");auto prototype=ctor.get("prototype");
{auto fn=JSValue::function(method0,{module},"addEventListener",2);fn.setFunctionSourceText("addEventListener( type, listener ) {\r\n\r\n\t\tif ( this._listeners === undefined ) this._listeners = {};\r\n\r\n\t\tconst listeners = this._listeners;\r\n\r\n\t\tif ( listeners[ type ] === undefined ) {\r\n\r\n\t\t\tlisteners[ type ] = [];\r\n\r\n\t\t}\r\n\r\n\t\tif ( listeners[ type ].indexOf( listener ) === - 1 ) {\r\n\r\n\t\t\tlisteners[ type ].push( listener );\r\n\r\n\t\t}\r\n\r\n\t}");JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("addEventListener",d);}
{auto fn=JSValue::function(method1,{module},"hasEventListener",2);fn.setFunctionSourceText("hasEventListener( type, listener ) {\r\n\r\n\t\tconst listeners = this._listeners;\r\n\r\n\t\tif ( listeners === undefined ) return false;\r\n\r\n\t\treturn listeners[ type ] !== undefined && listeners[ type ].indexOf( listener ) !== - 1;\r\n\r\n\t}");JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("hasEventListener",d);}
{auto fn=JSValue::function(method2,{module},"removeEventListener",2);fn.setFunctionSourceText("removeEventListener( type, listener ) {\r\n\r\n\t\tconst listeners = this._listeners;\r\n\r\n\t\tif ( listeners === undefined ) return;\r\n\r\n\t\tconst listenerArray = listeners[ type ];\r\n\r\n\t\tif ( listenerArray !== undefined ) {\r\n\r\n\t\t\tconst index = listenerArray.indexOf( listener );\r\n\r\n\t\t\tif ( index !== - 1 ) {\r\n\r\n\t\t\t\tlistenerArray.splice( index, 1 );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t}");JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("removeEventListener",d);}
{auto fn=JSValue::function(method3,{module},"dispatchEvent",1);fn.setFunctionSourceText("dispatchEvent( event ) {\r\n\r\n\t\tconst listeners = this._listeners;\r\n\r\n\t\tif ( listeners === undefined ) return;\r\n\r\n\t\tconst listenerArray = listeners[ event.type ];\r\n\r\n\t\tif ( listenerArray !== undefined ) {\r\n\r\n\t\t\tevent.target = this;\r\n\r\n\t\t\t// Make a copy, in case listeners are removed while iterating.\r\n\t\t\tconst array = listenerArray.slice( 0 );\r\n\r\n\t\t\tfor ( let i = 0, l = array.length; i < l; i ++ ) {\r\n\r\n\t\t\t\tarray[ i ].call( this, event );\r\n\r\n\t\t\t}\r\n\r\n\t\t\tevent.target = null;\r\n\r\n\t\t}\r\n\r\n\t}");JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("dispatchEvent",d);}
ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;
}
JSValue EventDispatcher::runtimePrototype() { return runtimeConstructor().get("prototype"); }

JSValue EventDispatcher::runtimeObject() { return JSValue::object(runtimePrototype()); }

EventDispatcher::Listener EventDispatcher::addEventListener(const std::string& type,
                                                            std::function<void(Event&)> callback) {
    auto listener = std::make_shared<std::function<void(Event&)>>(std::move(callback));
    addEventListener(type, listener);
    return listener;
}

void EventDispatcher::addEventListener(const std::string& type, const Listener& listener) {
    if (!listener || hasEventListener(type, listener))
        return;
    listeners_[type].push_back(listener);
}

bool EventDispatcher::hasEventListener(const std::string& type, const Listener& listener) const {
    const auto found = listeners_.find(type);
    return found != listeners_.end() &&
           std::find(found->second.begin(), found->second.end(), listener) != found->second.end();
}

void EventDispatcher::removeEventListener(const std::string& type, const Listener& listener) {
    const auto found = listeners_.find(type);
    if (found == listeners_.end())
        return;
    auto& array = found->second;
    const auto entry = std::find(array.begin(), array.end(), listener);
    if (entry != array.end())
        array.erase(entry);
}

void EventDispatcher::dispatchEvent(Event& event) {
    const auto found = listeners_.find(event.type);
    if (found == listeners_.end())
        return;
    event.target = this;
    const auto array = found->second;
    for (const auto& listener : array)
        (*listener)(event);
    event.target = nullptr;
}
