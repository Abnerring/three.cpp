// Native managed source port; pinned three.js 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "textures/CompressedCubeTexture.h"
#include "core/JSGlobal.h"
#include "core/JSModuleObject.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSRegExp.h"
#include "core/JSAsyncFunction.h"
#include "core/JSPromise.h"
#include "math/JSNumberMath.h"
#include "constants.h"
#include "textures/CompressedTexture.h"
#include <exception>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <unordered_set>
namespace CompressedCubeTextureManaged{

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
 const auto a=left.toNumber();const auto b=right.toNumber();if(op==">>"){const auto uint32=[](double x)->std::uint32_t{if(!std::isfinite(x)||x==0)return 0;auto r=std::fmod(std::trunc(x),4294967296.);if(r<0)r+=4294967296.;return static_cast<std::uint32_t>(r);};return std::bit_cast<std::int32_t>(uint32(a))>>(uint32(b)&31);}if(op=="-")return a-b;if(op=="*")return a*b;if(op=="/")return a/b;if(op=="**")return JSNumberMath::pow(a,b);throw std::logic_error("Untranslated arithmetic operator");
}
void appendSpread(Args& values,const JSValue& iterable){auto iterator=getJSIterator(iterable);while(const auto value=stepJSIterator(iterator))values.push_back(*value);}
void copyDataProperties(JSValue target,JSValue source){if(source.isNull()||source.isUndefined())return;if(!source.isObject())source=source.toObject();for(const auto& key:source.ownPropertyKeys()){const auto d=source.getOwnPropertyDescriptor(key);if(d&&d->enumerable.value_or(false)){const auto value=source.get(key);own(target,key,value);}}}
class ForInCursor{JSValue object;std::vector<JSValue> keys;std::unordered_set<std::string> visited;std::size_t index=0;bool loaded=false;public:explicit ForInCursor(JSValue source):object(source.isNull()||source.isUndefined()?JSValue(nullptr):source.toObject()){}std::optional<JSValue> next(){while(!object.isNull()){if(!loaded){keys=object.ownPropertyKeys();index=0;loaded=true;}while(index<keys.size()){const auto key=keys[index++];if(key.isSymbol())continue;const auto name=key.string();if(visited.contains(name))continue;const auto d=object.getOwnPropertyDescriptor(key);if(!d)continue;visited.insert(name);if(d->enumerable.value_or(false))return key;}object=object.getPrototype();loaded=false;}return std::nullopt;}};

JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue rejectAsyncAwait(const JSValue&,const Args& args,const Args& c){auto state=c[0];const auto reject=state.get("reject");const auto reason=arg(args,0);for(const auto& key:state.ownPropertyKeys())state.erase(key);reject.call({}, {reason});return {};}
JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
const auto nativeStatementTemp5=nativeStatementTemp0;nativeStatementTemp4={};
JSValue images=arg(nativeStatementTemp1,0);
JSValue format=arg(nativeStatementTemp1,1);
JSValue type=arg(nativeStatementTemp1,2);
([&]()->JSValue{const auto nativeStatementTemp9=nativeStatementTemp3.get("CompressedCubeTexture").getPrototype();Args nativeStatementTemp8;nativeStatementTemp8.push_back(JSValue{});nativeStatementTemp8.push_back(([&]()->JSValue{const auto nativeStatementTemp10=([&]()->JSValue{const auto nativeStatementTemp12=images;const auto nativeStatementTemp13=JSValue(0);return nativeStatementTemp12.get(nativeStatementTemp13);}());const auto nativeStatementTemp11=JSValue("width");return nativeStatementTemp10.get(nativeStatementTemp11);}()));nativeStatementTemp8.push_back(([&]()->JSValue{const auto nativeStatementTemp14=([&]()->JSValue{const auto nativeStatementTemp16=images;const auto nativeStatementTemp17=JSValue(0);return nativeStatementTemp16.get(nativeStatementTemp17);}());const auto nativeStatementTemp15=JSValue("height");return nativeStatementTemp14.get(nativeStatementTemp15);}()));nativeStatementTemp8.push_back(format);nativeStatementTemp8.push_back(type);nativeStatementTemp8.push_back(nativeStatementTemp3.get("CubeReflectionMapping"));nativeStatementTemp4=nativeStatementTemp9.construct(nativeStatementTemp8,nativeStatementTemp5);return nativeStatementTemp4;}());
([&]()->JSValue{auto nativeStatementTemp18=nativeStatementTemp4;const auto nativeStatementTemp19=JSValue("isCompressedCubeTexture");const auto nativeStatementTemp20=JSValue(true);setReference(nativeStatementTemp18,nativeStatementTemp19,nativeStatementTemp20);return nativeStatementTemp20;}());
([&]()->JSValue{auto nativeStatementTemp23=nativeStatementTemp4;const auto nativeStatementTemp24=JSValue("isCubeTexture");const auto nativeStatementTemp25=JSValue(true);setReference(nativeStatementTemp23,nativeStatementTemp24,nativeStatementTemp25);return nativeStatementTemp25;}());
([&]()->JSValue{auto nativeStatementTemp28=nativeStatementTemp4;const auto nativeStatementTemp29=JSValue("image");const auto nativeStatementTemp30=images;setReference(nativeStatementTemp28,nativeStatementTemp29,nativeStatementTemp30);return nativeStatementTemp30;}());
return nativeStatementTemp4;
}
}
JSValue nativeCompressedCubeTextureModule(){using namespace CompressedCubeTextureManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"CompressedCubeTexture module");auto module=JSValue::object(nullptr);
own(module,"CubeReflectionMapping",nativeConstantsBinding("CubeReflectionMapping"));
own(module,"CompressedTexture",nativeCompressedTextureConstructor());
auto ctor=JSValue::builtinConstructor(method0,{module},"CompressedCubeTexture",3);ctor.setFunctionSourceText("class CompressedCubeTexture extends CompressedTexture {\r\n\r\n\t/**\r\n\t * Constructs a new compressed texture.\r\n\t *\r\n\t * @param {Array<CompressedTexture>} images - An array of compressed textures.\r\n\t * @param {number} [format=RGBAFormat] - The texture format.\r\n\t * @param {number} [type=UnsignedByteType] - The texture type.\r\n\t */\r\n\tconstructor( images, format, type ) {\r\n\r\n\t\tsuper( undefined, images[ 0 ].width, images[ 0 ].height, format, type, CubeReflectionMapping );\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isCompressedCubeTexture = true;\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isCubeTexture = true;\r\n\r\n\t\tthis.image = images;\r\n\r\n\t}\r\n\r\n}");auto prototype=ctor.get("prototype");own(module,"CompressedCubeTexture",ctor);own(module,"_home",prototype);
ctor.setPrototype(module.get("CompressedTexture"));prototype.setPrototype(module.get("CompressedTexture").get("prototype"));
own(module,"default",ctor);module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();own(facade,"default",ctor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeCompressedCubeTextureBinding(const std::string& name){const auto keeper=nativeCompressedCubeTextureModule();return CompressedCubeTextureManaged::lexical().lock()->get(name);}
JSValue nativeCompressedCubeTextureConstructor(){return nativeCompressedCubeTextureBinding("default");}
