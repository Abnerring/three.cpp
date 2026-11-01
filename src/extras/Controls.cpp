#include "extras/Controls.h"

Controls::Controls(std::shared_ptr<Object3D> object, JSValue domElement)
    : object(std::move(object)), domElement(std::move(domElement)) {
    // keys = {}; the input maps start as ordinary empty/placeholder objects upstream.
    keys = JSValue::object();
    mouseButtons = JSValue::object();
    mouseButtons.set("LEFT", JSValue(nullptr));
    mouseButtons.set("MIDDLE", JSValue(nullptr));
    mouseButtons.set("RIGHT", JSValue(nullptr));
    touches = JSValue::object();
    touches.set("ONE", JSValue(nullptr));
    touches.set("TWO", JSValue(nullptr));
}

void Controls::connect(JSValue element) {
    // `if ( this.domElement !== null ) this.disconnect();` keeps the strict null test:
    // undefined still disconnects, exactly like the upstream comparison.
    if (!domElement.isNull()) disconnect();
    domElement = std::move(element);
}

// Complete native source port of src/extras/Controls.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "math/JSNumberMath.h"
#include "extras/ControlsHost.h"
#include "extras/Controls.h"
#include "math/Vector3.h"
#include "math/Quaternion.h"
#include <string_view>
#include <unordered_set>
namespace ControlsManaged{

using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const JSValue& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
void setReference(JSValue base,const JSValue& key,const JSValue& value){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot assign property of null/undefined");const auto property=key.toPropertyKey();if(base.isObject()){base.set(property,value);return;}auto object=base.toObject();if(!object.reflectSet(property,value,base))throw JSTypeError("Cannot assign primitive property");}
JSValue deleteReference(JSValue base,const JSValue& key){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot delete property of null/undefined");const auto property=key.toPropertyKey();auto object=base.toObject();if(!object.erase(property))throw JSTypeError("Cannot delete nonconfigurable property");return true;}
JSValue argumentObject(const Args& args){auto object=JSValue::object();JSPropertyDescriptor length;length.value=args.size();length.writable=length.configurable=true;object.defineProperty("length",length);for(unsigned i=0;i<args.size();++i)own(object,JSValue(i),args[i]);return object;}
JSValue typeOf(const JSValue& value){if(value.isUndefined())return "undefined";if(value.isCallable())return "function";if(value.isSymbol())return "symbol";switch(value.type()){case JSValue::Type::Boolean:return "boolean";case JSValue::Type::Number:return "number";case JSValue::Type::String:return "string";default:return "object";}}

bool abstractEqual(const JSValue& left,const JSValue& right){
 if(left.type()==right.type())return left.strictEquals(right);
 if((left.isNull()&&right.isUndefined())||(left.isUndefined()&&right.isNull()))return true;
 if(left.type()==JSValue::Type::Boolean)return abstractEqual(JSValue(left.toNumber()),right);
 if(right.type()==JSValue::Type::Boolean)return abstractEqual(left,JSValue(right.toNumber()));
 if(left.type()==JSValue::Type::Number&&right.type()==JSValue::Type::String)return left.strictEquals(JSValue(right.toNumber()));
 if(left.type()==JSValue::Type::String&&right.type()==JSValue::Type::Number)return JSValue(left.toNumber()).strictEquals(right);
 if(left.isObject()&&!right.isObject()&&!right.isNull()&&!right.isUndefined())return abstractEqual(left.toPrimitive(),right);
 if(right.isObject()&&!left.isObject()&&!left.isNull()&&!left.isUndefined())return abstractEqual(left,right.toPrimitive());
 return false;
}
JSValue binary(std::string_view op,const JSValue& left,const JSValue& right){
 if(op=="==")return abstractEqual(left,right);if(op=="!=")return !abstractEqual(left,right);

 if(op=="in")return right.has(left);if(op=="===")return left.strictEquals(right);if(op=="!==")return !left.strictEquals(right);
 if(op=="+"){const auto a=left.toPrimitive();const auto b=right.toPrimitive();if(a.type()==JSValue::Type::String||b.type()==JSValue::Type::String){const auto x=a.toString();const auto y=b.toString();return x+y;}const auto x=a.toNumber();const auto y=b.toNumber();return x+y;}
 if(op=="<"||op==">"||op=="<="||op==">="){const auto a=left.toPrimitive(JSValue::PreferredType::Number);const auto b=right.toPrimitive(JSValue::PreferredType::Number);if(a.type()==JSValue::Type::String&&b.type()==JSValue::Type::String){const auto x=jsUTF16(a.string()),y=jsUTF16(b.string());if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}const auto x=a.toNumber();const auto y=b.toNumber();if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}
 const auto a=left.toNumber();const auto b=right.toNumber();if(op=="-")return a-b;if(op=="*")return a*b;if(op=="/")return a/b;if(op=="**")return JSNumberMath::pow(a,b);throw std::logic_error("Untranslated arithmetic operator");
}
void appendSpread(Args& values,const JSValue& iterable){auto iterator=getJSIterator(iterable);while(const auto value=stepJSIterator(iterator))values.push_back(*value);}
void copyDataProperties(JSValue target,JSValue source){if(source.isNull()||source.isUndefined())return;if(!source.isObject())source=source.toObject();for(const auto& key:source.ownPropertyKeys()){const auto d=source.getOwnPropertyDescriptor(key);if(d&&d->enumerable.value_or(false)){const auto value=source.get(key);own(target,key,value);}}}
class ForInCursor{JSValue object;std::vector<JSValue> keys;std::unordered_set<std::string> visited;std::size_t index=0;bool loaded=false;public:explicit ForInCursor(JSValue source):object(source.isNull()||source.isUndefined()?JSValue(nullptr):source.toObject()){}std::optional<JSValue> next(){while(!object.isNull()){if(!loaded){keys=object.ownPropertyKeys();index=0;loaded=true;}while(index<keys.size()){const auto key=keys[index++];if(key.isSymbol())continue;const auto name=key.string();if(visited.contains(name))continue;const auto d=object.getOwnPropertyDescriptor(key);if(!d)continue;visited.insert(name);if(d->enumerable.value_or(false))return key;}object=object.getPrototype();loaded=false;}return std::nullopt;}};

JSValue makeLexicalCell(bool mutableBinding){auto cell=JSValue::object(nullptr);cell.setInternalSlot("initialized",false);cell.setInternalSlot("mutable",mutableBinding);return cell;}
JSValue readLexical(const JSValue& cell){if(!cell.getInternalSlot("initialized").toBoolean())throw JSReferenceError("Cannot access lexical binding before initialization");return cell.getInternalSlot("value");}
void initializeLexical(JSValue cell,const JSValue& value){if(cell.getInternalSlot("initialized").toBoolean())throw std::logic_error("Lexical binding initialized twice");cell.setInternalSlot("value",value);cell.setInternalSlot("initialized",true);}
void writeLexical(JSValue cell,const JSValue& value){if(!cell.getInternalSlot("initialized").toBoolean())throw JSReferenceError("Cannot access lexical binding before initialization");if(!cell.getInternalSlot("mutable").toBoolean())throw JSTypeError("Assignment to constant variable");cell.setInternalSlot("value",value);}

JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
const auto nativeStatementTemp5=nativeStatementTemp0;nativeStatementTemp4={};
JSValue object=arg(nativeStatementTemp1,0);
JSValue domElement=arg(nativeStatementTemp1,1);
if(domElement.isUndefined())domElement=JSValue(nullptr);
([&]()->JSValue{const auto nativeStatementTemp9=nativeStatementTemp3.get("Controls").getPrototype();Args nativeStatementTemp8;nativeStatementTemp4=nativeStatementTemp9.construct(nativeStatementTemp8,nativeStatementTemp5);return nativeStatementTemp4;}());
([&]()->JSValue{auto nativeStatementTemp10=nativeStatementTemp4;const auto nativeStatementTemp11=JSValue("object");const auto nativeStatementTemp12=object;setReference(nativeStatementTemp10,nativeStatementTemp11,nativeStatementTemp12);return nativeStatementTemp12;}());
([&]()->JSValue{auto nativeStatementTemp15=nativeStatementTemp4;const auto nativeStatementTemp16=JSValue("domElement");const auto nativeStatementTemp17=domElement;setReference(nativeStatementTemp15,nativeStatementTemp16,nativeStatementTemp17);return nativeStatementTemp17;}());
([&]()->JSValue{auto nativeStatementTemp20=nativeStatementTemp4;const auto nativeStatementTemp21=JSValue("enabled");const auto nativeStatementTemp22=JSValue(true);setReference(nativeStatementTemp20,nativeStatementTemp21,nativeStatementTemp22);return nativeStatementTemp22;}());
([&]()->JSValue{auto nativeStatementTemp25=nativeStatementTemp4;const auto nativeStatementTemp26=JSValue("state");const auto nativeStatementTemp27=([&]()->JSValue{const auto nativeStatementTemp30=JSValue(1);return -nativeStatementTemp30.toNumber();}());setReference(nativeStatementTemp25,nativeStatementTemp26,nativeStatementTemp27);return nativeStatementTemp27;}());
([&]()->JSValue{auto nativeStatementTemp31=nativeStatementTemp4;const auto nativeStatementTemp32=JSValue("keys");const auto nativeStatementTemp33=([&]()->JSValue{auto nativeStatementTemp36=JSValue::object();return nativeStatementTemp36;}());setReference(nativeStatementTemp31,nativeStatementTemp32,nativeStatementTemp33);return nativeStatementTemp33;}());
([&]()->JSValue{auto nativeStatementTemp37=nativeStatementTemp4;const auto nativeStatementTemp38=JSValue("mouseButtons");const auto nativeStatementTemp39=([&]()->JSValue{auto nativeStatementTemp42=JSValue::object();{const auto nativeStatementTemp43=JSValue("LEFT");const auto nativeStatementTemp44=JSValue(nullptr);own(nativeStatementTemp42,nativeStatementTemp43,nativeStatementTemp44);}{const auto nativeStatementTemp45=JSValue("MIDDLE");const auto nativeStatementTemp46=JSValue(nullptr);own(nativeStatementTemp42,nativeStatementTemp45,nativeStatementTemp46);}{const auto nativeStatementTemp47=JSValue("RIGHT");const auto nativeStatementTemp48=JSValue(nullptr);own(nativeStatementTemp42,nativeStatementTemp47,nativeStatementTemp48);}return nativeStatementTemp42;}());setReference(nativeStatementTemp37,nativeStatementTemp38,nativeStatementTemp39);return nativeStatementTemp39;}());
([&]()->JSValue{auto nativeStatementTemp49=nativeStatementTemp4;const auto nativeStatementTemp50=JSValue("touches");const auto nativeStatementTemp51=([&]()->JSValue{auto nativeStatementTemp54=JSValue::object();{const auto nativeStatementTemp55=JSValue("ONE");const auto nativeStatementTemp56=JSValue(nullptr);own(nativeStatementTemp54,nativeStatementTemp55,nativeStatementTemp56);}{const auto nativeStatementTemp57=JSValue("TWO");const auto nativeStatementTemp58=JSValue(nullptr);own(nativeStatementTemp54,nativeStatementTemp57,nativeStatementTemp58);}return nativeStatementTemp54;}());setReference(nativeStatementTemp49,nativeStatementTemp50,nativeStatementTemp51);return nativeStatementTemp51;}());
return nativeStatementTemp4;
}

JSValue method1(const JSValue& nativeStatementTemp59,const Args& nativeStatementTemp60,const Args& nativeStatementTemp61){
auto nativeStatementTemp62=nativeStatementTemp61[0];auto nativeStatementTemp63=nativeStatementTemp59;
JSValue element=arg(nativeStatementTemp60,0);
if(([&]()->JSValue{const auto nativeStatementTemp65=([&]()->JSValue{const auto nativeStatementTemp67=nativeStatementTemp63;const auto nativeStatementTemp68=JSValue("domElement");return nativeStatementTemp67.get(nativeStatementTemp68);}());const auto nativeStatementTemp66=JSValue(nullptr);return binary("!==",nativeStatementTemp65,nativeStatementTemp66);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp72=nativeStatementTemp63;const auto nativeStatementTemp73=JSValue("disconnect");const auto nativeStatementTemp69=nativeStatementTemp72.get(nativeStatementTemp73);const auto nativeStatementTemp70=nativeStatementTemp72;Args nativeStatementTemp71;return nativeStatementTemp69.call(nativeStatementTemp70,nativeStatementTemp71);}());
([&]()->JSValue{auto nativeStatementTemp74=nativeStatementTemp63;const auto nativeStatementTemp75=JSValue("domElement");const auto nativeStatementTemp76=element;setReference(nativeStatementTemp74,nativeStatementTemp75,nativeStatementTemp76);return nativeStatementTemp76;}());
return JSValue{};
}

JSValue method2(const JSValue& nativeStatementTemp79,const Args& nativeStatementTemp80,const Args& nativeStatementTemp81){
auto nativeStatementTemp82=nativeStatementTemp81[0];auto nativeStatementTemp83=nativeStatementTemp79;
return JSValue{};
}

JSValue method3(const JSValue& nativeStatementTemp85,const Args& nativeStatementTemp86,const Args& nativeStatementTemp87){
auto nativeStatementTemp88=nativeStatementTemp87[0];auto nativeStatementTemp89=nativeStatementTemp85;
return JSValue{};
}

JSValue method4(const JSValue& nativeStatementTemp91,const Args& nativeStatementTemp92,const Args& nativeStatementTemp93){
auto nativeStatementTemp94=nativeStatementTemp93[0];auto nativeStatementTemp95=nativeStatementTemp91;
return JSValue{};
}
}
JSValue nativeControlsModule(){using namespace ControlsManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject()){const auto status=cached->getInternalSlot("status");if(status.strictEquals("evaluated"))return cached->getInternalSlot("facade");if(status.strictEquals("failed"))throw JSThrownValue(cached->getInternalSlot("error"));throw JSModuleInitializationError("Controls module");}auto module=JSValue::object(nullptr);module.retainForHeapLifetime();weak=JSWeakValue(module);lexical()=weak;module.setInternalSlot("status","initializing");try{
own(module,"EventDispatcher",EventDispatcher::runtimeConstructor());

auto ctor=JSValue::builtinConstructor(method0,{module},"Controls",1);ctor.setFunctionSourceText("class Controls extends EventDispatcher {\r\n\r\n\t/**\r\n\t * Constructs a new controls instance.\r\n\t *\r\n\t * @param {Object3D} object - The object that is managed by the controls.\r\n\t * @param {?HTMLElement} domElement - The HTML element used for event listeners.\r\n\t */\r\n\tconstructor( object, domElement = null ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\t/**\r\n\t\t * The object that is managed by the controls.\r\n\t\t *\r\n\t\t * @type {Object3D}\r\n\t\t */\r\n\t\tthis.object = object;\r\n\r\n\t\t/**\r\n\t\t * The HTML element used for event listeners.\r\n\t\t *\r\n\t\t * @type {?HTMLElement}\r\n\t\t * @default null\r\n\t\t */\r\n\t\tthis.domElement = domElement;\r\n\r\n\t\t/**\r\n\t\t * Whether the controls responds to user input or not.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.enabled = true;\r\n\r\n\t\t/**\r\n\t\t * The internal state of the controls.\r\n\t\t *\r\n\t\t * @type {number}\r\n\t\t * @default -1\r\n\t\t */\r\n\t\tthis.state = - 1;\r\n\r\n\t\t/**\r\n\t\t * This object defines the keyboard input of the controls.\r\n\t\t *\r\n\t\t * @type {Object}\r\n\t\t */\r\n\t\tthis.keys = {};\r\n\r\n\t\t/**\r\n\t\t * This object defines what type of actions are assigned to the available mouse buttons.\r\n\t\t * It depends on the control implementation what kind of mouse buttons and actions are supported.\r\n\t\t *\r\n\t\t * @type {{LEFT: ?number, MIDDLE: ?number, RIGHT: ?number}}\r\n\t\t */\r\n\t\tthis.mouseButtons = { LEFT: null, MIDDLE: null, RIGHT: null };\r\n\r\n\t\t/**\r\n\t\t * This object defines what type of actions are assigned to what kind of touch interaction.\r\n\t\t * It depends on the control implementation what kind of touch interaction and actions are supported.\r\n\t\t *\r\n\t\t * @type {{ONE: ?number, TWO: ?number}}\r\n\t\t */\r\n\t\tthis.touches = { ONE: null, TWO: null };\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Connects the controls to the DOM. This method has so called \"side effects\" since\r\n\t * it adds the module's event listeners to the DOM.\r\n\t *\r\n\t * @param {HTMLElement} element - The DOM element to connect to.\r\n\t */\r\n\tconnect( element ) {\r\n\r\n\t\tif ( this.domElement !== null ) this.disconnect();\r\n\r\n\t\tthis.domElement = element;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Disconnects the controls from the DOM.\r\n\t */\r\n\tdisconnect() {}\r\n\r\n\t/**\r\n\t * Call this method if you no longer want use to the controls. It frees all internal\r\n\t * resources and removes all event listeners.\r\n\t */\r\n\tdispose() {}\r\n\r\n\t/**\r\n\t * Controls should implement this method if they have to update their internal state\r\n\t * per simulation step.\r\n\t *\r\n\t * @param {number} [delta] - The time delta in seconds.\r\n\t */\r\n\tupdate( /* delta */ ) {}\r\n\r\n}");ctor.setPrototype(module.get("EventDispatcher"));auto prototype=ctor.get("prototype");prototype.setPrototype(module.get("EventDispatcher").get("prototype"));own(module,"Controls",ctor);
{auto fn=JSValue::function(method1,{module,prototype},"connect",1);fn.setFunctionSourceText("connect( element ) {\r\n\r\n\t\tif ( this.domElement !== null ) this.disconnect();\r\n\r\n\t\tthis.domElement = element;\r\n\r\n\t}");JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("connect",d);}
{auto fn=JSValue::function(method2,{module,prototype},"disconnect",0);fn.setFunctionSourceText("disconnect() {}");JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("disconnect",d);}
{auto fn=JSValue::function(method3,{module,prototype},"dispose",0);fn.setFunctionSourceText("dispose() {}");JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("dispose",d);}
{auto fn=JSValue::function(method4,{module,prototype},"update",0);fn.setFunctionSourceText("update( /* delta */ ) {}");JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("update",d);}
auto facade=JSValue::object();own(facade,"Controls",ctor);module.setInternalSlot("facade",facade);module.setInternalSlot("status","evaluated");return facade;}catch(...){module.setInternalSlot("error",nativeExceptionValue(std::current_exception()));module.setInternalSlot("status","failed");throw JSThrownValue(module.getInternalSlot("error"));}}
JSValue nativeControlsBinding(const std::string& name){const auto keeper=nativeControlsModule();return ControlsManaged::lexical().lock()->get(name);}
JSValue nativeControlsConstructor(){return nativeControlsBinding("Controls");}
