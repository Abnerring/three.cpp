#include "lights/Light.h"
#include <unordered_map>
#include <stdexcept>

namespace { std::unordered_map<std::string, JSValue>& nodeClasses() { static std::unordered_map<std::string, JSValue> values; return values; } }
Light::Light(const Color& color, double intensity) : color(color), intensity(intensity) { isLight = true; type = "Light"; }
void Light::registerNodeFor(const std::string& className, const JSValue& nodeClass) { nodeClasses().insert_or_assign(className, nodeClass); }
std::optional<JSValue> Light::registeredNodeFor(const std::string& className) { const auto& values = nodeClasses(); const auto found = values.find(className); return found == values.end() ? std::nullopt : std::optional<JSValue>(found->second); }
void Light::registerNode(const JSValue& nodeClass) { registerNodeFor("Light", nodeClass); }
JSValue Light::getLightNode() const { return registeredNodeFor("Light").value_or(JSValue()); }
Light& Light::copy(const Object3D& source, bool recursive) {
    const auto* light = dynamic_cast<const Light*>(&source);
    if (!light) throw std::invalid_argument("Light::copy requires a Light");
    Object3D::copy(source, recursive);
    color.copy(light->color); intensity = light->intensity;
    return *this;
}
std::shared_ptr<Object3D> Light::clone(bool recursive) const { auto result = std::make_shared<Light>(); result->copy(*this, recursive); return result; }
JsonValue Light::toJSON(Object3DJSONContext* meta) const { auto data = Object3D::toJSON(meta); data["object"]["color"] = color.getHex(); data["object"]["intensity"] = intensity; return data; }
std::optional<JsonValue> Light::propertyValue(const std::string& property) const { if (property == "intensity") return intensity; return Object3D::propertyValue(property); }

// Complete class-body native source port; full Nodes/TSL barrel evaluation is separately pending.
// Fixed three.js 11738a9d55e502fb484cefffcea3a66dc44e9fe9
#include "lights/Light.h"
#include "core/JSGlobal.h"
#include "core/JSModuleObject.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSRegExp.h"
#include "core/JSAsyncFunction.h"
#include "core/JSPromise.h"
#include "math/JSNumberMath.h"
#include "core/Object3D.h"
#include "math/Color.h"
#include <exception>
#include <cmath>
#include <string_view>
#include <unordered_set>
namespace LightManaged {

using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const JSValue& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
void setReference(JSValue base,const JSValue& key,const JSValue& value){base.set(key,value);}
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
JSValue lightNodeClass=arg(nativeStatementTemp1,0);
([&]()->JSValue{auto nativeStatementTemp6=([&]()->JSValue{const auto nativeStatementTemp8=nativeStatementTemp4;const auto nativeStatementTemp9=JSValue("prototype");return nativeStatementTemp8.get(nativeStatementTemp9);}());const auto nativeStatementTemp7=JSValue("_lightNode");const auto nativeStatementTemp10=lightNodeClass;setReference(nativeStatementTemp6,nativeStatementTemp7,nativeStatementTemp10);return nativeStatementTemp10;}());
return JSValue{};
}

JSValue method1(const JSValue& nativeStatementTemp13,const Args& nativeStatementTemp14,const Args& nativeStatementTemp15){
auto nativeStatementTemp16=nativeStatementTemp15[0];auto nativeStatementTemp17=nativeStatementTemp13;
const auto nativeStatementTemp18=nativeStatementTemp13;nativeStatementTemp17={};
JSValue color=arg(nativeStatementTemp14,0);
JSValue nativeStatementTemp19=arg(nativeStatementTemp14,1);if(nativeStatementTemp19.isUndefined())nativeStatementTemp19=JSValue(1);
JSValue intensity=nativeStatementTemp19;
([&]()->JSValue{const auto nativeStatementTemp23=nativeStatementTemp16.get("Light").getPrototype();Args nativeStatementTemp22;nativeStatementTemp17=nativeStatementTemp23.construct(nativeStatementTemp22,nativeStatementTemp18);return nativeStatementTemp17;}());
([&]()->JSValue{auto nativeStatementTemp24=nativeStatementTemp17;const auto nativeStatementTemp25=JSValue("isLight");const auto nativeStatementTemp26=JSValue(true);setReference(nativeStatementTemp24,nativeStatementTemp25,nativeStatementTemp26);return nativeStatementTemp26;}());
([&]()->JSValue{auto nativeStatementTemp29=nativeStatementTemp17;const auto nativeStatementTemp30=JSValue("type");const auto nativeStatementTemp31=JSValue("Light");setReference(nativeStatementTemp29,nativeStatementTemp30,nativeStatementTemp31);return nativeStatementTemp31;}());
([&]()->JSValue{auto nativeStatementTemp34=nativeStatementTemp17;const auto nativeStatementTemp35=JSValue("color");const auto nativeStatementTemp36=([&]()->JSValue{const auto nativeStatementTemp39=nativeStatementTemp16.get("Color");Args nativeStatementTemp40;nativeStatementTemp40.push_back(color);return nativeStatementTemp39.construct(nativeStatementTemp40);}());setReference(nativeStatementTemp34,nativeStatementTemp35,nativeStatementTemp36);return nativeStatementTemp36;}());
([&]()->JSValue{auto nativeStatementTemp41=nativeStatementTemp17;const auto nativeStatementTemp42=JSValue("intensity");const auto nativeStatementTemp43=intensity;setReference(nativeStatementTemp41,nativeStatementTemp42,nativeStatementTemp43);return nativeStatementTemp43;}());
return nativeStatementTemp17;
}

JSValue method2(const JSValue& nativeStatementTemp46,const Args& nativeStatementTemp47,const Args& nativeStatementTemp48){
auto nativeStatementTemp49=nativeStatementTemp48[0];auto nativeStatementTemp50=nativeStatementTemp46;
JSValue source=arg(nativeStatementTemp47,0);
JSValue recursive=arg(nativeStatementTemp47,1);
([&]()->JSValue{const auto nativeStatementTemp55=nativeStatementTemp49.get("_home").getPrototype();const auto nativeStatementTemp56=JSValue("copy");const auto nativeStatementTemp52=nativeStatementTemp55.get(nativeStatementTemp56,nativeStatementTemp50);const auto nativeStatementTemp53=nativeStatementTemp50;Args nativeStatementTemp54;nativeStatementTemp54.push_back(source);nativeStatementTemp54.push_back(recursive);return nativeStatementTemp52.call(nativeStatementTemp53,nativeStatementTemp54);}());
([&]()->JSValue{auto nativeStatementTemp60=([&]()->JSValue{const auto nativeStatementTemp62=nativeStatementTemp50;const auto nativeStatementTemp63=JSValue("color");return nativeStatementTemp62.get(nativeStatementTemp63);}());const auto nativeStatementTemp61=JSValue("copy");const auto nativeStatementTemp57=nativeStatementTemp60.get(nativeStatementTemp61);const auto nativeStatementTemp58=nativeStatementTemp60;Args nativeStatementTemp59;nativeStatementTemp59.push_back(([&]()->JSValue{const auto nativeStatementTemp64=source;const auto nativeStatementTemp65=JSValue("color");return nativeStatementTemp64.get(nativeStatementTemp65);}()));return nativeStatementTemp57.call(nativeStatementTemp58,nativeStatementTemp59);}());
([&]()->JSValue{auto nativeStatementTemp66=nativeStatementTemp50;const auto nativeStatementTemp67=JSValue("intensity");const auto nativeStatementTemp68=([&]()->JSValue{const auto nativeStatementTemp71=source;const auto nativeStatementTemp72=JSValue("intensity");return nativeStatementTemp71.get(nativeStatementTemp72);}());setReference(nativeStatementTemp66,nativeStatementTemp67,nativeStatementTemp68);return nativeStatementTemp68;}());
return nativeStatementTemp50;
return JSValue{};
}

JSValue method3(const JSValue& nativeStatementTemp73,const Args& nativeStatementTemp74,const Args& nativeStatementTemp75){
auto nativeStatementTemp76=nativeStatementTemp75[0];auto nativeStatementTemp77=nativeStatementTemp73;
JSValue meta=arg(nativeStatementTemp74,0);
JSValue data=([&]()->JSValue{const auto nativeStatementTemp82=nativeStatementTemp76.get("_home").getPrototype();const auto nativeStatementTemp83=JSValue("toJSON");const auto nativeStatementTemp79=nativeStatementTemp82.get(nativeStatementTemp83,nativeStatementTemp77);const auto nativeStatementTemp80=nativeStatementTemp77;Args nativeStatementTemp81;nativeStatementTemp81.push_back(meta);return nativeStatementTemp79.call(nativeStatementTemp80,nativeStatementTemp81);}());
([&]()->JSValue{auto nativeStatementTemp84=([&]()->JSValue{const auto nativeStatementTemp86=data;const auto nativeStatementTemp87=JSValue("object");return nativeStatementTemp86.get(nativeStatementTemp87);}());const auto nativeStatementTemp85=JSValue("color");const auto nativeStatementTemp88=([&]()->JSValue{auto nativeStatementTemp94=([&]()->JSValue{const auto nativeStatementTemp96=nativeStatementTemp77;const auto nativeStatementTemp97=JSValue("color");return nativeStatementTemp96.get(nativeStatementTemp97);}());const auto nativeStatementTemp95=JSValue("getHex");const auto nativeStatementTemp91=nativeStatementTemp94.get(nativeStatementTemp95);const auto nativeStatementTemp92=nativeStatementTemp94;Args nativeStatementTemp93;return nativeStatementTemp91.call(nativeStatementTemp92,nativeStatementTemp93);}());setReference(nativeStatementTemp84,nativeStatementTemp85,nativeStatementTemp88);return nativeStatementTemp88;}());
([&]()->JSValue{auto nativeStatementTemp98=([&]()->JSValue{const auto nativeStatementTemp100=data;const auto nativeStatementTemp101=JSValue("object");return nativeStatementTemp100.get(nativeStatementTemp101);}());const auto nativeStatementTemp99=JSValue("intensity");const auto nativeStatementTemp102=([&]()->JSValue{const auto nativeStatementTemp105=nativeStatementTemp77;const auto nativeStatementTemp106=JSValue("intensity");return nativeStatementTemp105.get(nativeStatementTemp106);}());setReference(nativeStatementTemp98,nativeStatementTemp99,nativeStatementTemp102);return nativeStatementTemp102;}());
return data;
return JSValue{};
}
}
JSValue nativeLightModule(){using namespace LightManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Light module");auto module=JSValue::object(nullptr);
own(module,"Object3D",nativeObject3DConstructor());
own(module,"Color",nativeColorConstructor());
auto constructor=JSValue::builtinConstructor(method1,{module},"Light",1);constructor.setFunctionSourceText("class Light extends Object3D {\r\n\r\n\t/**\r\n\t * Registers the node class that `WebGPURenderer` uses to shade lights of this type.\r\n\t *\r\n\t * @param {AnalyticLightNode.constructor} lightNodeClass - The light node class.\r\n\t */\r\n\tstatic registerNode( lightNodeClass ) {\r\n\r\n\t\tthis.prototype._lightNode = lightNodeClass;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Constructs a new light.\r\n\t *\r\n\t * @param {(number|Color|string)} [color=0xffffff] - The light's color.\r\n\t * @param {number} [intensity=1] - The light's strength/intensity.\r\n\t */\r\n\tconstructor( color, intensity = 1 ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isLight = true;\r\n\r\n\t\tthis.type = 'Light';\r\n\r\n\t\t/**\r\n\t\t * The light's color.\r\n\t\t *\r\n\t\t * @type {Color}\r\n\t\t */\r\n\t\tthis.color = new Color( color );\r\n\r\n\t\t/**\r\n\t\t * The light's intensity.\r\n\t\t *\r\n\t\t * @type {number}\r\n\t\t * @default 1\r\n\t\t */\r\n\t\tthis.intensity = intensity;\r\n\r\n\t}\r\n\r\n\tcopy( source, recursive ) {\r\n\r\n\t\tsuper.copy( source, recursive );\r\n\r\n\t\tthis.color.copy( source.color );\r\n\t\tthis.intensity = source.intensity;\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\ttoJSON( meta ) {\r\n\r\n\t\tconst data = super.toJSON( meta );\r\n\r\n\t\tdata.object.color = this.color.getHex();\r\n\t\tdata.object.intensity = this.intensity;\r\n\r\n\t\treturn data;\r\n\r\n\t}\r\n\r\n}");auto prototype=constructor.get("prototype");own(module,"Light",constructor);own(module,"_home",prototype);
constructor.setPrototype(module.get("Object3D"));prototype.setPrototype(module.get("Object3D").get("prototype"));
{auto function=JSValue::function(method0,{module},"registerNode",1);function.setFunctionSourceText("registerNode( lightNodeClass ) {\r\n\r\n\t\tthis.prototype._lightNode = lightNodeClass;\r\n\r\n\t}");JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;constructor.defineProperty("registerNode",d);}
{auto function=JSValue::function(method2,{module},"copy",2);function.setFunctionSourceText("copy( source, recursive ) {\r\n\r\n\t\tsuper.copy( source, recursive );\r\n\r\n\t\tthis.color.copy( source.color );\r\n\t\tthis.intensity = source.intensity;\r\n\r\n\t\treturn this;\r\n\r\n\t}");JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("copy",d);}
{auto function=JSValue::function(method3,{module},"toJSON",1);function.setFunctionSourceText("toJSON( meta ) {\r\n\r\n\t\tconst data = super.toJSON( meta );\r\n\r\n\t\tdata.object.color = this.color.getHex();\r\n\t\tdata.object.intensity = this.intensity;\r\n\r\n\t\treturn data;\r\n\r\n\t}");JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("toJSON",d);}
own(module,"default",constructor);module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();own(facade,"Light",constructor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeLightBinding(const std::string& name){const auto keeper=nativeLightModule();return LightManaged::lexical().lock()->get(name);}
JSValue nativeLightConstructor(){return nativeLightBinding("default");}
