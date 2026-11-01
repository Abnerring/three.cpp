#include "math/Matrix2.h"
#include <limits>
#include "core/JSModuleObject.h"

Matrix2::Matrix2(double n11, double n12, double n21, double n22) {
    set(n11, n12, n21, n22);
}
std::span<double, 4> Matrix2::elements() {
    return elements_;
}
std::span<const double, 4> Matrix2::elements() const {
    return elements_;
}
Matrix2& Matrix2::identity() {
    return set(1, 0, 0, 1);
}
Matrix2& Matrix2::set(double n11, double n12, double n21, double n22) {
    elements_[0] = n11;
    elements_[2] = n12;
    elements_[1] = n21;
    elements_[3] = n22;
    return *this;
}
Matrix2& Matrix2::fromArray(const std::vector<double>& array, std::size_t offset) {
    for (std::size_t i = 0; i < 4; ++i)
        elements_[i] = offset < array.size() && i < array.size() - offset
                           ? array[offset + i]
                           : std::numeric_limits<double>::quiet_NaN();
    return *this;
}


// Complete pinned native managed matrices; see THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSPromise.h"
#include "math/Vector3.h"
#include "math/JSNumberMath.h"
#include "constants.h"
#include "utils.h"
#include <bit>
#include <cmath>
#include <string_view>
#include <unordered_set>

#include "math/Matrix2RuntimeSource.inc"
namespace Matrix2Managed{

using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const JSValue& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
void setReference(JSValue base,const JSValue& key,const JSValue& value){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot assign property of null/undefined");const auto property=key.toPropertyKey();if(base.isObject()){base.set(property,value);return;}auto object=base.toObject();if(!object.reflectSet(property,value,base))throw JSTypeError("Cannot assign primitive property");}
JSValue deleteReference(JSValue base,const JSValue& key){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot delete property of null/undefined");const auto property=key.toPropertyKey();auto object=base.toObject();if(!object.erase(property))throw JSTypeError("Cannot delete nonconfigurable property");return true;}
JSValue argumentObject(const Args& args){auto object=JSValue::object();JSPropertyDescriptor length;length.value=args.size();length.writable=length.configurable=true;object.defineProperty("length",length);for(unsigned i=0;i<args.size();++i)own(object,JSValue(i),args[i]);return object;}
JSValue typeOf(const JSValue& value){if(value.isUndefined())return "undefined";if(value.isCallable())return "function";if(value.isSymbol())return "symbol";switch(value.type()){case JSValue::Type::Boolean:return "boolean";case JSValue::Type::Number:return "number";case JSValue::Type::String:return "string";default:return "object";}}

std::uint32_t uint32(double value){if(!std::isfinite(value)||value==0)return 0;auto n=std::fmod(std::trunc(value),4294967296.0);if(n<0)n+=4294967296.0;return static_cast<std::uint32_t>(n);}
JSValue binary(std::string_view op,const JSValue& left,const JSValue& right){
 if(op=="in")return right.has(left);if(op=="===")return left.strictEquals(right);if(op=="!==")return !left.strictEquals(right);
 if(op=="+"){const auto a=left.toPrimitive();const auto b=right.toPrimitive();if(a.type()==JSValue::Type::String||b.type()==JSValue::Type::String){const auto x=a.toString();const auto y=b.toString();return x+y;}const auto x=a.toNumber();const auto y=b.toNumber();return x+y;}
 if(op=="<"||op==">"||op=="<="||op==">="){const auto a=left.toPrimitive(JSValue::PreferredType::Number);const auto b=right.toPrimitive(JSValue::PreferredType::Number);if(a.type()==JSValue::Type::String&&b.type()==JSValue::Type::String){const auto x=jsUTF16(a.string()),y=jsUTF16(b.string());if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}const auto x=a.toNumber();const auto y=b.toNumber();if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}
 const auto a=left.toNumber();const auto b=right.toNumber();if(op=="-")return a-b;if(op=="*")return a*b;if(op=="/")return a/b;if(op=="**")return JSNumberMath::pow(a,b);if(op=="%")return std::fmod(a,b);if(op=="&")return std::bit_cast<std::int32_t>(uint32(a)&uint32(b));if(op=="|")return std::bit_cast<std::int32_t>(uint32(a)|uint32(b));if(op=="^")return std::bit_cast<std::int32_t>(uint32(a)^uint32(b));if(op==">")return a>b;if(op==">>")return std::bit_cast<std::int32_t>(uint32(a))>>(uint32(b)&31);if(op==">>>")return uint32(a)>>(uint32(b)&31);throw std::logic_error("Untranslated arithmetic operator");
}
void appendSpread(Args& values,const JSValue& iterable){auto iterator=getJSIterator(iterable);while(const auto value=stepJSIterator(iterator))values.push_back(*value);}
void copyDataProperties(JSValue target,JSValue source){if(source.isNull()||source.isUndefined())return;if(!source.isObject())source=source.toObject();for(const auto& key:source.ownPropertyKeys()){const auto d=source.getOwnPropertyDescriptor(key);if(d&&d->enumerable.value_or(false)){const auto value=source.get(key);own(target,key,value);}}}
class ForInCursor{JSValue object;std::vector<JSValue> keys;std::unordered_set<std::string> visited;std::size_t index=0;bool loaded=false;public:explicit ForInCursor(JSValue source):object(source.isNull()||source.isUndefined()?JSValue(nullptr):source.toObject()){}std::optional<JSValue> next(){while(!object.isNull()){if(!loaded){keys=object.ownPropertyKeys();index=0;loaded=true;}while(index<keys.size()){const auto key=keys[index++];if(key.isSymbol())continue;const auto name=key.string();if(visited.contains(name))continue;const auto d=object.getOwnPropertyDescriptor(key);if(!d)continue;visited.insert(name);if(d->enumerable.value_or(false))return key;}object=object.getPrototype();loaded=false;}return std::nullopt;}};

JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
JSValue n11=arg(nativeStatementTemp1,0);
JSValue n12=arg(nativeStatementTemp1,1);
JSValue n21=arg(nativeStatementTemp1,2);
JSValue n22=arg(nativeStatementTemp1,3);
([&]()->JSValue{auto nativeStatementTemp6=nativeStatementTemp4;const auto nativeStatementTemp7=JSValue("elements");const auto nativeStatementTemp8=([&]()->JSValue{auto nativeStatementTemp11=JSValue::array();own(nativeStatementTemp11,JSValue(0),JSValue(1));own(nativeStatementTemp11,JSValue(1),JSValue(0));own(nativeStatementTemp11,JSValue(2),JSValue(0));own(nativeStatementTemp11,JSValue(3),JSValue(1));return nativeStatementTemp11;}());setReference(nativeStatementTemp6,nativeStatementTemp7,nativeStatementTemp8);return nativeStatementTemp8;}());
if(([&]()->JSValue{const auto nativeStatementTemp12=n11;const auto nativeStatementTemp13=JSValue{};return binary("!==",nativeStatementTemp12,nativeStatementTemp13);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp17=nativeStatementTemp4;const auto nativeStatementTemp18=JSValue("set");const auto nativeStatementTemp14=nativeStatementTemp17.get(nativeStatementTemp18);const auto nativeStatementTemp15=nativeStatementTemp17;Args nativeStatementTemp16;nativeStatementTemp16.push_back(n11);nativeStatementTemp16.push_back(n12);nativeStatementTemp16.push_back(n21);nativeStatementTemp16.push_back(n22);return nativeStatementTemp14.call(nativeStatementTemp15,nativeStatementTemp16);}());

}
return JSValue{};
}
JSValue method1(const JSValue& nativeStatementTemp19,const Args& nativeStatementTemp20,const Args& nativeStatementTemp21){
auto nativeStatementTemp22=nativeStatementTemp21[0];auto nativeStatementTemp23=nativeStatementTemp19;
([&]()->JSValue{auto nativeStatementTemp28=nativeStatementTemp23;const auto nativeStatementTemp29=JSValue("set");const auto nativeStatementTemp25=nativeStatementTemp28.get(nativeStatementTemp29);const auto nativeStatementTemp26=nativeStatementTemp28;Args nativeStatementTemp27;nativeStatementTemp27.push_back(JSValue(1));nativeStatementTemp27.push_back(JSValue(0));nativeStatementTemp27.push_back(JSValue(0));nativeStatementTemp27.push_back(JSValue(1));return nativeStatementTemp25.call(nativeStatementTemp26,nativeStatementTemp27);}());
return nativeStatementTemp23;
return JSValue{};
}
JSValue method2(const JSValue& nativeStatementTemp30,const Args& nativeStatementTemp31,const Args& nativeStatementTemp32){
auto nativeStatementTemp33=nativeStatementTemp32[0];auto nativeStatementTemp34=nativeStatementTemp30;
JSValue array=arg(nativeStatementTemp31,0);
JSValue offset=arg(nativeStatementTemp31,1);
if(offset.isUndefined())offset=JSValue(0);
{JSValue i=JSValue(0);
for(;([&]()->JSValue{const auto nativeStatementTemp36=i;const auto nativeStatementTemp37=JSValue(4);return binary("<",nativeStatementTemp36,nativeStatementTemp37);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp38=i.toNumber();const auto nativeStatementTemp39=nativeStatementTemp38+1;i=nativeStatementTemp39;return nativeStatementTemp38;}())){
([&]()->JSValue{auto nativeStatementTemp40=([&]()->JSValue{const auto nativeStatementTemp42=nativeStatementTemp34;const auto nativeStatementTemp43=JSValue("elements");return nativeStatementTemp42.get(nativeStatementTemp43);}());const auto nativeStatementTemp41=i;const auto nativeStatementTemp44=([&]()->JSValue{const auto nativeStatementTemp47=array;const auto nativeStatementTemp48=([&]()->JSValue{const auto nativeStatementTemp49=i;const auto nativeStatementTemp50=offset;return binary("+",nativeStatementTemp49,nativeStatementTemp50);}());return nativeStatementTemp47.get(nativeStatementTemp48);}());setReference(nativeStatementTemp40,nativeStatementTemp41,nativeStatementTemp44);return nativeStatementTemp44;}());

}
}
return nativeStatementTemp34;
return JSValue{};
}
JSValue method3(const JSValue& nativeStatementTemp51,const Args& nativeStatementTemp52,const Args& nativeStatementTemp53){
auto nativeStatementTemp54=nativeStatementTemp53[0];auto nativeStatementTemp55=nativeStatementTemp51;
JSValue n11=arg(nativeStatementTemp52,0);
JSValue n12=arg(nativeStatementTemp52,1);
JSValue n21=arg(nativeStatementTemp52,2);
JSValue n22=arg(nativeStatementTemp52,3);
JSValue te=([&]()->JSValue{const auto nativeStatementTemp57=nativeStatementTemp55;const auto nativeStatementTemp58=JSValue("elements");return nativeStatementTemp57.get(nativeStatementTemp58);}());
([&]()->JSValue{auto nativeStatementTemp59=te;const auto nativeStatementTemp60=JSValue(0);const auto nativeStatementTemp61=n11;setReference(nativeStatementTemp59,nativeStatementTemp60,nativeStatementTemp61);return nativeStatementTemp61;}());
([&]()->JSValue{auto nativeStatementTemp64=te;const auto nativeStatementTemp65=JSValue(2);const auto nativeStatementTemp66=n12;setReference(nativeStatementTemp64,nativeStatementTemp65,nativeStatementTemp66);return nativeStatementTemp66;}());
([&]()->JSValue{auto nativeStatementTemp69=te;const auto nativeStatementTemp70=JSValue(1);const auto nativeStatementTemp71=n21;setReference(nativeStatementTemp69,nativeStatementTemp70,nativeStatementTemp71);return nativeStatementTemp71;}());
([&]()->JSValue{auto nativeStatementTemp74=te;const auto nativeStatementTemp75=JSValue(3);const auto nativeStatementTemp76=n22;setReference(nativeStatementTemp74,nativeStatementTemp75,nativeStatementTemp76);return nativeStatementTemp76;}());
return nativeStatementTemp55;
return JSValue{};
}
}
JSValue nativeMatrix2Module(){using namespace Matrix2Managed;static thread_local JSWeakValue weak;if(const auto existing=weak.lock();existing&&existing->isObject()){const auto status=existing->getInternalSlot("Matrix2.status");if(status.strictEquals("evaluated"))return existing->getInternalSlot("Matrix2.facade");if(status.strictEquals("failed"))throw JSThrownValue(existing->getInternalSlot("Matrix2.error"));throw JSModuleInitializationError("Matrix2 module");}auto module=JSValue::object(nullptr);module.retainForHeapLifetime();weak=JSWeakValue(module);lexical()=weak;module.setInternalSlot("Matrix2.status","initializing");try{
auto ctor=JSValue::constructor(method0,{module},"Matrix2",4);ctor.setFunctionSourceText(Matrix2RuntimeSource::classSource);own(module,"Matrix2",ctor);auto prototype=ctor.get("prototype");
{auto fn=JSValue::function(method1,{module},"identity",0);fn.setFunctionSourceText(Matrix2RuntimeSource::source1);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("identity",d);}
{auto fn=JSValue::function(method2,{module},"fromArray",1);fn.setFunctionSourceText(Matrix2RuntimeSource::source2);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("fromArray",d);}
{auto fn=JSValue::function(method3,{module},"set",4);fn.setFunctionSourceText(Matrix2RuntimeSource::source3);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("set",d);}
([&]()->JSValue{auto nativeStatementTemp79=([&]()->JSValue{const auto nativeStatementTemp81=ctor;const auto nativeStatementTemp82=JSValue("prototype");return nativeStatementTemp81.get(nativeStatementTemp82);}());const auto nativeStatementTemp80=JSValue("isMatrix2");const auto nativeStatementTemp83=JSValue(true);setReference(nativeStatementTemp79,nativeStatementTemp80,nativeStatementTemp83);return nativeStatementTemp83;}());
auto facade=JSValue::object();own(facade,"Matrix2",ctor);module.setInternalSlot("Matrix2.facade",facade);module.setInternalSlot("Matrix2.status","evaluated");return facade;}catch(...){module.setInternalSlot("Matrix2.error",nativeExceptionValue(std::current_exception()));module.setInternalSlot("Matrix2.status","failed");throw JSThrownValue(module.getInternalSlot("Matrix2.error"));}}
JSValue nativeMatrix2Binding(const std::string& name){const auto keeper=nativeMatrix2Module();return Matrix2Managed::lexical().lock()->get(name);}
JSValue nativeMatrix2Constructor(){return nativeMatrix2Binding("Matrix2");}
