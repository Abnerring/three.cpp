// Native managed source port; pinned three.js 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "textures/CompressedTexture.h"
#include "core/JSGlobal.h"
#include "core/JSModuleObject.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSRegExp.h"
#include "core/JSAsyncFunction.h"
#include "core/JSPromise.h"
#include "math/JSNumberMath.h"
#include "textures/Texture.h"
#include <exception>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <unordered_set>
namespace CompressedTextureManaged{

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
JSValue mipmaps=arg(nativeStatementTemp1,0);
JSValue width=arg(nativeStatementTemp1,1);
JSValue height=arg(nativeStatementTemp1,2);
JSValue format=arg(nativeStatementTemp1,3);
JSValue type=arg(nativeStatementTemp1,4);
JSValue mapping=arg(nativeStatementTemp1,5);
JSValue wrapS=arg(nativeStatementTemp1,6);
JSValue wrapT=arg(nativeStatementTemp1,7);
JSValue magFilter=arg(nativeStatementTemp1,8);
JSValue minFilter=arg(nativeStatementTemp1,9);
JSValue anisotropy=arg(nativeStatementTemp1,10);
JSValue colorSpace=arg(nativeStatementTemp1,11);
([&]()->JSValue{const auto nativeStatementTemp9=nativeStatementTemp3.get("CompressedTexture").getPrototype();Args nativeStatementTemp8;nativeStatementTemp8.push_back(JSValue(nullptr));nativeStatementTemp8.push_back(mapping);nativeStatementTemp8.push_back(wrapS);nativeStatementTemp8.push_back(wrapT);nativeStatementTemp8.push_back(magFilter);nativeStatementTemp8.push_back(minFilter);nativeStatementTemp8.push_back(format);nativeStatementTemp8.push_back(type);nativeStatementTemp8.push_back(anisotropy);nativeStatementTemp8.push_back(colorSpace);nativeStatementTemp4=nativeStatementTemp9.construct(nativeStatementTemp8,nativeStatementTemp5);return nativeStatementTemp4;}());
([&]()->JSValue{auto nativeStatementTemp10=nativeStatementTemp4;const auto nativeStatementTemp11=JSValue("isCompressedTexture");const auto nativeStatementTemp12=JSValue(true);setReference(nativeStatementTemp10,nativeStatementTemp11,nativeStatementTemp12);return nativeStatementTemp12;}());
([&]()->JSValue{auto nativeStatementTemp15=nativeStatementTemp4;const auto nativeStatementTemp16=JSValue("image");const auto nativeStatementTemp17=([&]()->JSValue{auto nativeStatementTemp20=JSValue::object();{const auto nativeStatementTemp21=JSValue("width");const auto nativeStatementTemp22=width;own(nativeStatementTemp20,nativeStatementTemp21,nativeStatementTemp22);}{const auto nativeStatementTemp23=JSValue("height");const auto nativeStatementTemp24=height;own(nativeStatementTemp20,nativeStatementTemp23,nativeStatementTemp24);}return nativeStatementTemp20;}());setReference(nativeStatementTemp15,nativeStatementTemp16,nativeStatementTemp17);return nativeStatementTemp17;}());
([&]()->JSValue{auto nativeStatementTemp25=nativeStatementTemp4;const auto nativeStatementTemp26=JSValue("mipmaps");const auto nativeStatementTemp27=mipmaps;setReference(nativeStatementTemp25,nativeStatementTemp26,nativeStatementTemp27);return nativeStatementTemp27;}());
([&]()->JSValue{auto nativeStatementTemp30=nativeStatementTemp4;const auto nativeStatementTemp31=JSValue("flipY");const auto nativeStatementTemp32=JSValue(false);setReference(nativeStatementTemp30,nativeStatementTemp31,nativeStatementTemp32);return nativeStatementTemp32;}());
([&]()->JSValue{auto nativeStatementTemp35=nativeStatementTemp4;const auto nativeStatementTemp36=JSValue("generateMipmaps");const auto nativeStatementTemp37=JSValue(false);setReference(nativeStatementTemp35,nativeStatementTemp36,nativeStatementTemp37);return nativeStatementTemp37;}());
return nativeStatementTemp4;
}
}
JSValue nativeCompressedTextureModule(){using namespace CompressedTextureManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"CompressedTexture module");auto module=JSValue::object(nullptr);
own(module,"Texture",nativeTextureConstructor());
auto ctor=JSValue::builtinConstructor(method0,{module},"CompressedTexture",12);ctor.setFunctionSourceText("class CompressedTexture extends Texture {\r\n\r\n\t/**\r\n\t * Constructs a new compressed texture.\r\n\t *\r\n\t * @param {Array<Object>} mipmaps - This array holds for all mipmaps (including the bases mip)\r\n\t * the data and dimensions.\r\n\t * @param {number} width - The width of the texture.\r\n\t * @param {number} height - The height of the texture.\r\n\t * @param {number} [format=RGBAFormat] - The texture format.\r\n\t * @param {number} [type=UnsignedByteType] - The texture type.\r\n\t * @param {number} [mapping=Texture.DEFAULT_MAPPING] - The texture mapping.\r\n\t * @param {number} [wrapS=ClampToEdgeWrapping] - The wrapS value.\r\n\t * @param {number} [wrapT=ClampToEdgeWrapping] - The wrapT value.\r\n\t * @param {number} [magFilter=LinearFilter] - The mag filter value.\r\n\t * @param {number} [minFilter=LinearMipmapLinearFilter] - The min filter value.\r\n\t * @param {number} [anisotropy=Texture.DEFAULT_ANISOTROPY] - The anisotropy value.\r\n\t * @param {string} [colorSpace=NoColorSpace] - The color space.\r\n\t */\r\n\tconstructor( mipmaps, width, height, format, type, mapping, wrapS, wrapT, magFilter, minFilter, anisotropy, colorSpace ) {\r\n\r\n\t\tsuper( null, mapping, wrapS, wrapT, magFilter, minFilter, format, type, anisotropy, colorSpace );\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isCompressedTexture = true;\r\n\r\n\t\t/**\r\n\t\t * The image property of a compressed texture just defines its dimensions.\r\n\t\t *\r\n\t\t * @type {{width:number,height:number}}\r\n\t\t */\r\n\t\tthis.image = { width: width, height: height };\r\n\r\n\t\t/**\r\n\t\t * This array holds for all mipmaps (including the bases mip) the data and dimensions.\r\n\t\t *\r\n\t\t * @type {Array<Object>}\r\n\t\t */\r\n\t\tthis.mipmaps = mipmaps;\r\n\r\n\t\t/**\r\n\t\t * If set to `true`, the texture is flipped along the vertical axis when\r\n\t\t * uploaded to the GPU.\r\n\t\t *\r\n\t\t * Overwritten and set to `false` by default since it is not possible to\r\n\t\t * flip compressed textures.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @default false\r\n\t\t * @readonly\r\n\t\t */\r\n\t\tthis.flipY = false;\r\n\r\n\t\t/**\r\n\t\t * Whether to generate mipmaps (if possible) for a texture.\r\n\t\t *\r\n\t\t * Overwritten and set to `false` by default since it is not\r\n\t\t * possible to generate mipmaps for compressed data. Mipmaps\r\n\t\t * must be embedded in the compressed texture file.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @default false\r\n\t\t * @readonly\r\n\t\t */\r\n\t\tthis.generateMipmaps = false;\r\n\r\n\t}\r\n\r\n}");auto prototype=ctor.get("prototype");own(module,"CompressedTexture",ctor);own(module,"_home",prototype);
ctor.setPrototype(module.get("Texture"));prototype.setPrototype(module.get("Texture").get("prototype"));
own(module,"default",ctor);module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();own(facade,"default",ctor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeCompressedTextureBinding(const std::string& name){const auto keeper=nativeCompressedTextureModule();return CompressedTextureManaged::lexical().lock()->get(name);}
JSValue nativeCompressedTextureConstructor(){return nativeCompressedTextureBinding("default");}
