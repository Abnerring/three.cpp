#include "cameras/ArrayCamera.h"

ArrayCamera::ArrayCamera(SharedArray<std::shared_ptr<Camera>> array) : cameras(std::move(array)) {
    isArrayCamera = true;
    isMultiViewCamera = false;
}
std::shared_ptr<Object3D> ArrayCamera::clone(bool) const {
    auto result = std::make_shared<ArrayCamera>();
    // Camera.clone() accepts no recursive parameter in the upstream module.
    result->copy(*this);
    return result;
}
std::optional<JsonValue> ArrayCamera::propertyValue(const std::string& property) const {
    if (property == "isArrayCamera") return isArrayCamera;
    if (property == "isMultiViewCamera") return isMultiViewCamera;
    return PerspectiveCamera::propertyValue(property);
}
#include "math/JSNumberMath.h"

// Complete managed native port of pinned ArrayCamera.js. See THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSPromise.h"
#include "cameras/ArrayCameraRuntimeSource.inc"
#include "cameras/PerspectiveCamera.h"
#include <string_view>
#include <unordered_set>
namespace ArrayCameraManaged{

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
const auto nativeStatementTemp5=nativeStatementTemp0;nativeStatementTemp4={};
JSValue array=arg(nativeStatementTemp1,0);
if(array.isUndefined())array=([&]()->JSValue{auto nativeStatementTemp6=JSValue::array();return nativeStatementTemp6;}());
([&]()->JSValue{const auto nativeStatementTemp10=nativeStatementTemp3.get("ArrayCamera").getPrototype();Args nativeStatementTemp9;nativeStatementTemp4=nativeStatementTemp10.construct(nativeStatementTemp9,nativeStatementTemp5);return nativeStatementTemp4;}());
([&]()->JSValue{auto nativeStatementTemp11=nativeStatementTemp4;const auto nativeStatementTemp12=JSValue("isArrayCamera");const auto nativeStatementTemp13=JSValue(true);setReference(nativeStatementTemp11,nativeStatementTemp12,nativeStatementTemp13);return nativeStatementTemp13;}());
([&]()->JSValue{auto nativeStatementTemp16=nativeStatementTemp4;const auto nativeStatementTemp17=JSValue("isMultiViewCamera");const auto nativeStatementTemp18=JSValue(false);setReference(nativeStatementTemp16,nativeStatementTemp17,nativeStatementTemp18);return nativeStatementTemp18;}());
([&]()->JSValue{auto nativeStatementTemp21=nativeStatementTemp4;const auto nativeStatementTemp22=JSValue("cameras");const auto nativeStatementTemp23=array;setReference(nativeStatementTemp21,nativeStatementTemp22,nativeStatementTemp23);return nativeStatementTemp23;}());
return nativeStatementTemp4;
}
}
JSValue nativeArrayCameraModule(){using namespace ArrayCameraManaged;static thread_local JSWeakValue weak;if(const auto existing=weak.lock();existing&&existing->isObject())return *existing;static thread_local bool initializing=false;JSModuleInitializationScope initialization(initializing,"ArrayCamera module");auto module=JSValue::object(nullptr);
own(module,"PerspectiveCamera",nativePerspectiveCameraConstructor());
auto ctor=JSValue::builtinConstructor(method0,{module,JSValue{}},"ArrayCamera",0);ctor.setFunctionSourceText(ArrayCameraRuntimeSource::classSource);auto prototype=ctor.get("prototype");
ctor.setPrototype(module.get("PerspectiveCamera"));prototype.setPrototype(module.get("PerspectiveCamera").get("prototype"));
own(module,"ArrayCamera",ctor);own(module,"_home",prototype);
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto exports=JSValue::object();own(exports,"ArrayCamera",module.get("ArrayCamera"));exports.retainForHeapLifetime();weak=JSWeakValue(exports);return exports;}
JSValue nativeArrayCameraBinding(const std::string& name){const auto keeper=nativeArrayCameraModule();return ArrayCameraManaged::lexical().lock()->get(name);}
JSValue nativeArrayCameraConstructor(){return nativeArrayCameraBinding("ArrayCamera");}
