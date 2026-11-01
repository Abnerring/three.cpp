#include "nodes/utils/CubeMapNode.h"

#include <map>

namespace {
// One conversion per source texture, keyed by the texture's identity.
std::map<std::uint64_t, std::uint64_t> gCache;
std::uint64_t gNextId = 1;
}  // namespace

bool CubeMapNode::needsConversion(const std::string& mapping) {
    // Both equirectangular kinds, the reflection one and the refraction one.
    return mapping == kEquirectReflection || mapping == kEquirectRefraction;
}

CubeMapNode::Source CubeMapNode::source(bool isTextureNode, bool isMaterialReferenceNode) {
    if (isTextureNode) return Source::TextureNode;
    if (isMaterialReferenceNode) return Source::MaterialReference;
    return Source::None;
}

bool CubeMapNode::cachesPerTexture() {
    return true;
}

bool CubeMapNode::reappliesMappingOnHit() {
    return true;
}

std::uint32_t CubeMapNode::cachedCount() {
    return static_cast<std::uint32_t>(gCache.size());
}

std::uint64_t CubeMapNode::conversionFor(std::uint64_t textureId) {
    const auto found = gCache.find(textureId);
    if (found != gCache.end()) return found->second;
    const std::uint64_t made = gNextId;
    gNextId += 1;
    gCache[textureId] = made;
    return made;
}

void CubeMapNode::reset() {
    gCache.clear();
    gNextId = 1;
}

// Native managed source port; pinned three.js 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "nodes/utils/CubeMapNode.h"
#include "core/JSGlobal.h"
#include "core/JSModuleObject.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSRegExp.h"
#include "math/JSNumberMath.h"
#include "nodes/core/Node.h"
#include "nodes/core/constants.h"
#include "nodes/tsl/TSLBase.h"
#include "textures/CubeTexture.h"
#include "nodes/accessors/CubeTextureNode.h"
#include "renderers/common/CubeRenderTarget.h"
#include "constants.h"
#include <exception>
#include <string_view>
#include <unordered_set>
namespace CubeMapNodeManaged{

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
JSValue helper0(const JSValue& nativeStatementTemp181,const Args& nativeStatementTemp182,const Args& nativeStatementTemp183){
auto nativeStatementTemp184=nativeStatementTemp183[0];auto nativeStatementTemp185=nativeStatementTemp181;
JSValue image=arg(nativeStatementTemp182,0);
if(([&]()->JSValue{const auto nativeStatementTemp187=([&]()->JSValue{const auto nativeStatementTemp188=image;const auto nativeStatementTemp189=JSValue(nullptr);return binary("===",nativeStatementTemp188,nativeStatementTemp189);}());if(nativeStatementTemp187.toBoolean())return nativeStatementTemp187;return ([&]()->JSValue{const auto nativeStatementTemp190=image;const auto nativeStatementTemp191=JSValue{};return binary("===",nativeStatementTemp190,nativeStatementTemp191);}());}()).toBoolean())return JSValue(false);
return ([&]()->JSValue{const auto nativeStatementTemp192=([&]()->JSValue{const auto nativeStatementTemp194=image;const auto nativeStatementTemp195=JSValue("height");return nativeStatementTemp194.get(nativeStatementTemp195);}());const auto nativeStatementTemp193=JSValue(0);return binary(">",nativeStatementTemp192,nativeStatementTemp193);}());
return JSValue{};
}

JSValue helper1(const JSValue& nativeStatementTemp196,const Args& nativeStatementTemp197,const Args& nativeStatementTemp198){
auto nativeStatementTemp199=nativeStatementTemp198[0];auto nativeStatementTemp200=nativeStatementTemp196;
JSValue event=arg(nativeStatementTemp197,0);
JSValue texture=([&]()->JSValue{const auto nativeStatementTemp202=event;const auto nativeStatementTemp203=JSValue("target");return nativeStatementTemp202.get(nativeStatementTemp203);}());
([&]()->JSValue{auto nativeStatementTemp207=texture;const auto nativeStatementTemp208=JSValue("removeEventListener");const auto nativeStatementTemp204=nativeStatementTemp207.get(nativeStatementTemp208);const auto nativeStatementTemp205=nativeStatementTemp207;Args nativeStatementTemp206;nativeStatementTemp206.push_back(JSValue("dispose"));nativeStatementTemp206.push_back(nativeStatementTemp199.get("onTextureDispose"));return nativeStatementTemp204.call(nativeStatementTemp205,nativeStatementTemp206);}());
JSValue renderTarget=([&]()->JSValue{auto nativeStatementTemp212=nativeStatementTemp199.get("_cache");const auto nativeStatementTemp213=JSValue("get");const auto nativeStatementTemp209=nativeStatementTemp212.get(nativeStatementTemp213);const auto nativeStatementTemp210=nativeStatementTemp212;Args nativeStatementTemp211;nativeStatementTemp211.push_back(texture);return nativeStatementTemp209.call(nativeStatementTemp210,nativeStatementTemp211);}());
if(([&]()->JSValue{const auto nativeStatementTemp214=renderTarget;const auto nativeStatementTemp215=JSValue{};return binary("!==",nativeStatementTemp214,nativeStatementTemp215);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp219=nativeStatementTemp199.get("_cache");const auto nativeStatementTemp220=JSValue("delete");const auto nativeStatementTemp216=nativeStatementTemp219.get(nativeStatementTemp220);const auto nativeStatementTemp217=nativeStatementTemp219;Args nativeStatementTemp218;nativeStatementTemp218.push_back(texture);return nativeStatementTemp216.call(nativeStatementTemp217,nativeStatementTemp218);}());
([&]()->JSValue{auto nativeStatementTemp224=renderTarget;const auto nativeStatementTemp225=JSValue("dispose");const auto nativeStatementTemp221=nativeStatementTemp224.get(nativeStatementTemp225);const auto nativeStatementTemp222=nativeStatementTemp224;Args nativeStatementTemp223;return nativeStatementTemp221.call(nativeStatementTemp222,nativeStatementTemp223);}());

}
return JSValue{};
}

JSValue helper2(const JSValue& nativeStatementTemp226,const Args& nativeStatementTemp227,const Args& nativeStatementTemp228){
auto nativeStatementTemp229=nativeStatementTemp228[0];auto nativeStatementTemp230=nativeStatementTemp226;
JSValue texture=arg(nativeStatementTemp227,0);
JSValue mapping=arg(nativeStatementTemp227,1);
if(([&]()->JSValue{const auto nativeStatementTemp232=mapping;const auto nativeStatementTemp233=nativeStatementTemp229.get("EquirectangularReflectionMapping");return binary("===",nativeStatementTemp232,nativeStatementTemp233);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp234=texture;const auto nativeStatementTemp235=JSValue("mapping");const auto nativeStatementTemp236=nativeStatementTemp229.get("CubeReflectionMapping");setReference(nativeStatementTemp234,nativeStatementTemp235,nativeStatementTemp236);return nativeStatementTemp236;}());

}
else if(([&]()->JSValue{const auto nativeStatementTemp239=mapping;const auto nativeStatementTemp240=nativeStatementTemp229.get("EquirectangularRefractionMapping");return binary("===",nativeStatementTemp239,nativeStatementTemp240);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp241=texture;const auto nativeStatementTemp242=JSValue("mapping");const auto nativeStatementTemp243=nativeStatementTemp229.get("CubeRefractionMapping");setReference(nativeStatementTemp241,nativeStatementTemp242,nativeStatementTemp243);return nativeStatementTemp243;}());

}
return JSValue{};
}
JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
return JSValue("CubeMapNode");
return JSValue{};
}

JSValue method1(const JSValue& nativeStatementTemp6,const Args& nativeStatementTemp7,const Args& nativeStatementTemp8){
auto nativeStatementTemp9=nativeStatementTemp8[0];auto nativeStatementTemp10=nativeStatementTemp6;
const auto nativeStatementTemp11=nativeStatementTemp6;nativeStatementTemp10={};
JSValue envNode=arg(nativeStatementTemp7,0);
([&]()->JSValue{const auto nativeStatementTemp15=nativeStatementTemp9.get("CubeMapNode").getPrototype();Args nativeStatementTemp14;nativeStatementTemp14.push_back(JSValue("vec3"));nativeStatementTemp10=nativeStatementTemp15.construct(nativeStatementTemp14,nativeStatementTemp11);return nativeStatementTemp10;}());
([&]()->JSValue{auto nativeStatementTemp16=nativeStatementTemp10;const auto nativeStatementTemp17=JSValue("envNode");const auto nativeStatementTemp18=envNode;setReference(nativeStatementTemp16,nativeStatementTemp17,nativeStatementTemp18);return nativeStatementTemp18;}());
([&]()->JSValue{auto nativeStatementTemp21=nativeStatementTemp10;const auto nativeStatementTemp22=JSValue("_cubeTexture");const auto nativeStatementTemp23=JSValue(nullptr);setReference(nativeStatementTemp21,nativeStatementTemp22,nativeStatementTemp23);return nativeStatementTemp23;}());
([&]()->JSValue{auto nativeStatementTemp26=nativeStatementTemp10;const auto nativeStatementTemp27=JSValue("_cubeTextureNode");const auto nativeStatementTemp28=([&]()->JSValue{const auto nativeStatementTemp31=nativeStatementTemp9.get("cubeTexture");const JSValue nativeStatementTemp32;Args nativeStatementTemp33;nativeStatementTemp33.push_back(JSValue(nullptr));return nativeStatementTemp31.call(nativeStatementTemp32,nativeStatementTemp33);}());setReference(nativeStatementTemp26,nativeStatementTemp27,nativeStatementTemp28);return nativeStatementTemp28;}());
JSValue defaultTexture=([&]()->JSValue{const auto nativeStatementTemp34=nativeStatementTemp9.get("CubeTexture");Args nativeStatementTemp35;return nativeStatementTemp34.construct(nativeStatementTemp35);}());
([&]()->JSValue{auto nativeStatementTemp36=defaultTexture;const auto nativeStatementTemp37=JSValue("isRenderTargetTexture");const auto nativeStatementTemp38=JSValue(true);setReference(nativeStatementTemp36,nativeStatementTemp37,nativeStatementTemp38);return nativeStatementTemp38;}());
([&]()->JSValue{auto nativeStatementTemp41=nativeStatementTemp10;const auto nativeStatementTemp42=JSValue("_defaultTexture");const auto nativeStatementTemp43=defaultTexture;setReference(nativeStatementTemp41,nativeStatementTemp42,nativeStatementTemp43);return nativeStatementTemp43;}());
([&]()->JSValue{auto nativeStatementTemp46=nativeStatementTemp10;const auto nativeStatementTemp47=JSValue("updateBeforeType");const auto nativeStatementTemp48=([&]()->JSValue{const auto nativeStatementTemp51=nativeStatementTemp9.get("NodeUpdateType");const auto nativeStatementTemp52=JSValue("RENDER");return nativeStatementTemp51.get(nativeStatementTemp52);}());setReference(nativeStatementTemp46,nativeStatementTemp47,nativeStatementTemp48);return nativeStatementTemp48;}());
return nativeStatementTemp10;
}

JSValue method2(const JSValue& nativeStatementTemp53,const Args& nativeStatementTemp54,const Args& nativeStatementTemp55){
auto nativeStatementTemp56=nativeStatementTemp55[0];auto nativeStatementTemp57=nativeStatementTemp53;
JSValue frame=arg(nativeStatementTemp54,0);
const auto nativeStatementTemp59=frame;if(nativeStatementTemp59.isNull()||nativeStatementTemp59.isUndefined())throw JSTypeError("Cannot destructure null/undefined");
JSValue renderer=nativeStatementTemp59.get(JSValue("renderer"));
JSValue material=nativeStatementTemp59.get(JSValue("material"));
JSValue envNode=([&]()->JSValue{const auto nativeStatementTemp60=nativeStatementTemp57;const auto nativeStatementTemp61=JSValue("envNode");return nativeStatementTemp60.get(nativeStatementTemp61);}());
if(([&]()->JSValue{const auto nativeStatementTemp62=([&]()->JSValue{const auto nativeStatementTemp63=envNode;const auto nativeStatementTemp64=JSValue("isTextureNode");return nativeStatementTemp63.get(nativeStatementTemp64);}());if(nativeStatementTemp62.toBoolean())return nativeStatementTemp62;return ([&]()->JSValue{const auto nativeStatementTemp65=envNode;const auto nativeStatementTemp66=JSValue("isMaterialReferenceNode");return nativeStatementTemp65.get(nativeStatementTemp66);}());}()).toBoolean()){
JSValue texture=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp67=envNode;const auto nativeStatementTemp68=JSValue("isTextureNode");return nativeStatementTemp67.get(nativeStatementTemp68);}()).toBoolean())return ([&]()->JSValue{const auto nativeStatementTemp69=envNode;const auto nativeStatementTemp70=JSValue("value");return nativeStatementTemp69.get(nativeStatementTemp70);}());return ([&]()->JSValue{const auto nativeStatementTemp71=material;const auto nativeStatementTemp72=([&]()->JSValue{const auto nativeStatementTemp73=envNode;const auto nativeStatementTemp74=JSValue("property");return nativeStatementTemp73.get(nativeStatementTemp74);}());return nativeStatementTemp71.get(nativeStatementTemp72);}());}());
if(([&]()->JSValue{const auto nativeStatementTemp75=texture;if(!nativeStatementTemp75.toBoolean())return nativeStatementTemp75;return ([&]()->JSValue{const auto nativeStatementTemp76=texture;const auto nativeStatementTemp77=JSValue("isTexture");return nativeStatementTemp76.get(nativeStatementTemp77);}());}()).toBoolean()){
JSValue mapping=([&]()->JSValue{const auto nativeStatementTemp78=texture;const auto nativeStatementTemp79=JSValue("mapping");return nativeStatementTemp78.get(nativeStatementTemp79);}());
if(([&]()->JSValue{const auto nativeStatementTemp80=([&]()->JSValue{const auto nativeStatementTemp81=mapping;const auto nativeStatementTemp82=nativeStatementTemp56.get("EquirectangularReflectionMapping");return binary("===",nativeStatementTemp81,nativeStatementTemp82);}());if(nativeStatementTemp80.toBoolean())return nativeStatementTemp80;return ([&]()->JSValue{const auto nativeStatementTemp83=mapping;const auto nativeStatementTemp84=nativeStatementTemp56.get("EquirectangularRefractionMapping");return binary("===",nativeStatementTemp83,nativeStatementTemp84);}());}()).toBoolean()){
if(([&]()->JSValue{auto nativeStatementTemp88=nativeStatementTemp56.get("_cache");const auto nativeStatementTemp89=JSValue("has");const auto nativeStatementTemp85=nativeStatementTemp88.get(nativeStatementTemp89);const auto nativeStatementTemp86=nativeStatementTemp88;Args nativeStatementTemp87;nativeStatementTemp87.push_back(texture);return nativeStatementTemp85.call(nativeStatementTemp86,nativeStatementTemp87);}()).toBoolean()){
JSValue cubeMap=([&]()->JSValue{auto nativeStatementTemp93=nativeStatementTemp56.get("_cache");const auto nativeStatementTemp94=JSValue("get");const auto nativeStatementTemp90=nativeStatementTemp93.get(nativeStatementTemp94);const auto nativeStatementTemp91=nativeStatementTemp93;Args nativeStatementTemp92;nativeStatementTemp92.push_back(texture);return nativeStatementTemp90.call(nativeStatementTemp91,nativeStatementTemp92);}());
([&]()->JSValue{const auto nativeStatementTemp95=nativeStatementTemp56.get("mapTextureMapping");const JSValue nativeStatementTemp96;Args nativeStatementTemp97;nativeStatementTemp97.push_back(cubeMap);nativeStatementTemp97.push_back(([&]()->JSValue{const auto nativeStatementTemp98=texture;const auto nativeStatementTemp99=JSValue("mapping");return nativeStatementTemp98.get(nativeStatementTemp99);}()));return nativeStatementTemp95.call(nativeStatementTemp96,nativeStatementTemp97);}());
([&]()->JSValue{auto nativeStatementTemp100=nativeStatementTemp57;const auto nativeStatementTemp101=JSValue("_cubeTexture");const auto nativeStatementTemp102=cubeMap;setReference(nativeStatementTemp100,nativeStatementTemp101,nativeStatementTemp102);return nativeStatementTemp102;}());

}
else {
JSValue image=([&]()->JSValue{const auto nativeStatementTemp105=texture;const auto nativeStatementTemp106=JSValue("image");return nativeStatementTemp105.get(nativeStatementTemp106);}());
if(([&]()->JSValue{const auto nativeStatementTemp107=nativeStatementTemp56.get("isEquirectangularMapReady");const JSValue nativeStatementTemp108;Args nativeStatementTemp109;nativeStatementTemp109.push_back(image);return nativeStatementTemp107.call(nativeStatementTemp108,nativeStatementTemp109);}()).toBoolean()){
JSValue renderTarget=([&]()->JSValue{const auto nativeStatementTemp110=nativeStatementTemp56.get("CubeRenderTarget");Args nativeStatementTemp111;nativeStatementTemp111.push_back(([&]()->JSValue{const auto nativeStatementTemp112=image;const auto nativeStatementTemp113=JSValue("height");return nativeStatementTemp112.get(nativeStatementTemp113);}()));return nativeStatementTemp110.construct(nativeStatementTemp111);}());
([&]()->JSValue{auto nativeStatementTemp117=renderTarget;const auto nativeStatementTemp118=JSValue("fromEquirectangularTexture");const auto nativeStatementTemp114=nativeStatementTemp117.get(nativeStatementTemp118);const auto nativeStatementTemp115=nativeStatementTemp117;Args nativeStatementTemp116;nativeStatementTemp116.push_back(renderer);nativeStatementTemp116.push_back(texture);return nativeStatementTemp114.call(nativeStatementTemp115,nativeStatementTemp116);}());
([&]()->JSValue{const auto nativeStatementTemp119=nativeStatementTemp56.get("mapTextureMapping");const JSValue nativeStatementTemp120;Args nativeStatementTemp121;nativeStatementTemp121.push_back(([&]()->JSValue{const auto nativeStatementTemp122=renderTarget;const auto nativeStatementTemp123=JSValue("texture");return nativeStatementTemp122.get(nativeStatementTemp123);}()));nativeStatementTemp121.push_back(([&]()->JSValue{const auto nativeStatementTemp124=texture;const auto nativeStatementTemp125=JSValue("mapping");return nativeStatementTemp124.get(nativeStatementTemp125);}()));return nativeStatementTemp119.call(nativeStatementTemp120,nativeStatementTemp121);}());
([&]()->JSValue{auto nativeStatementTemp126=nativeStatementTemp57;const auto nativeStatementTemp127=JSValue("_cubeTexture");const auto nativeStatementTemp128=([&]()->JSValue{const auto nativeStatementTemp131=renderTarget;const auto nativeStatementTemp132=JSValue("texture");return nativeStatementTemp131.get(nativeStatementTemp132);}());setReference(nativeStatementTemp126,nativeStatementTemp127,nativeStatementTemp128);return nativeStatementTemp128;}());
([&]()->JSValue{auto nativeStatementTemp136=nativeStatementTemp56.get("_cache");const auto nativeStatementTemp137=JSValue("set");const auto nativeStatementTemp133=nativeStatementTemp136.get(nativeStatementTemp137);const auto nativeStatementTemp134=nativeStatementTemp136;Args nativeStatementTemp135;nativeStatementTemp135.push_back(texture);nativeStatementTemp135.push_back(([&]()->JSValue{const auto nativeStatementTemp138=renderTarget;const auto nativeStatementTemp139=JSValue("texture");return nativeStatementTemp138.get(nativeStatementTemp139);}()));return nativeStatementTemp133.call(nativeStatementTemp134,nativeStatementTemp135);}());
([&]()->JSValue{auto nativeStatementTemp143=texture;const auto nativeStatementTemp144=JSValue("addEventListener");const auto nativeStatementTemp140=nativeStatementTemp143.get(nativeStatementTemp144);const auto nativeStatementTemp141=nativeStatementTemp143;Args nativeStatementTemp142;nativeStatementTemp142.push_back(JSValue("dispose"));nativeStatementTemp142.push_back(nativeStatementTemp56.get("onTextureDispose"));return nativeStatementTemp140.call(nativeStatementTemp141,nativeStatementTemp142);}());

}
else {
([&]()->JSValue{auto nativeStatementTemp145=nativeStatementTemp57;const auto nativeStatementTemp146=JSValue("_cubeTexture");const auto nativeStatementTemp147=([&]()->JSValue{const auto nativeStatementTemp150=nativeStatementTemp57;const auto nativeStatementTemp151=JSValue("_defaultTexture");return nativeStatementTemp150.get(nativeStatementTemp151);}());setReference(nativeStatementTemp145,nativeStatementTemp146,nativeStatementTemp147);return nativeStatementTemp147;}());

}

}
([&]()->JSValue{auto nativeStatementTemp152=([&]()->JSValue{const auto nativeStatementTemp154=nativeStatementTemp57;const auto nativeStatementTemp155=JSValue("_cubeTextureNode");return nativeStatementTemp154.get(nativeStatementTemp155);}());const auto nativeStatementTemp153=JSValue("value");const auto nativeStatementTemp156=([&]()->JSValue{const auto nativeStatementTemp159=nativeStatementTemp57;const auto nativeStatementTemp160=JSValue("_cubeTexture");return nativeStatementTemp159.get(nativeStatementTemp160);}());setReference(nativeStatementTemp152,nativeStatementTemp153,nativeStatementTemp156);return nativeStatementTemp156;}());

}
else {
([&]()->JSValue{auto nativeStatementTemp161=nativeStatementTemp57;const auto nativeStatementTemp162=JSValue("_cubeTextureNode");const auto nativeStatementTemp163=([&]()->JSValue{const auto nativeStatementTemp166=nativeStatementTemp57;const auto nativeStatementTemp167=JSValue("envNode");return nativeStatementTemp166.get(nativeStatementTemp167);}());setReference(nativeStatementTemp161,nativeStatementTemp162,nativeStatementTemp163);return nativeStatementTemp163;}());

}

}

}
return JSValue{};
}

JSValue method3(const JSValue& nativeStatementTemp168,const Args& nativeStatementTemp169,const Args& nativeStatementTemp170){
auto nativeStatementTemp171=nativeStatementTemp170[0];auto nativeStatementTemp172=nativeStatementTemp168;
JSValue builder=arg(nativeStatementTemp169,0);
([&]()->JSValue{auto nativeStatementTemp177=nativeStatementTemp172;const auto nativeStatementTemp178=JSValue("updateBefore");const auto nativeStatementTemp174=nativeStatementTemp177.get(nativeStatementTemp178);const auto nativeStatementTemp175=nativeStatementTemp177;Args nativeStatementTemp176;nativeStatementTemp176.push_back(builder);return nativeStatementTemp174.call(nativeStatementTemp175,nativeStatementTemp176);}());
return ([&]()->JSValue{const auto nativeStatementTemp179=nativeStatementTemp172;const auto nativeStatementTemp180=JSValue("_cubeTextureNode");return nativeStatementTemp179.get(nativeStatementTemp180);}());
return JSValue{};
}
}
JSValue nativeCubeMapNodeModule(){using namespace CubeMapNodeManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"CubeMapNode module");auto module=JSValue::object(nullptr);
own(module,"Node",nativeNodeConstructor());
own(module,"NodeUpdateType",nativeNodeConstantsBinding("NodeUpdateType"));
own(module,"nodeProxy",nativeTSLBaseBinding("nodeProxy"));
own(module,"CubeTexture",nativeCubeTextureConstructor());
own(module,"cubeTexture",nativeCubeTextureNodeBinding("cubeTexture"));
own(module,"CubeRenderTarget",nativeCubeRenderTargetConstructor());
own(module,"CubeReflectionMapping",nativeConstantsBinding("CubeReflectionMapping"));
own(module,"CubeRefractionMapping",nativeConstantsBinding("CubeRefractionMapping"));
own(module,"EquirectangularReflectionMapping",nativeConstantsBinding("EquirectangularReflectionMapping"));
own(module,"EquirectangularRefractionMapping",nativeConstantsBinding("EquirectangularRefractionMapping"));
{auto function=JSValue::constructor(helper0,{module},"isEquirectangularMapReady",1,JSValue::ConstructorKind::Callable);function.setFunctionSourceText("function isEquirectangularMapReady( image ) {\r\n\r\n\tif ( image === null || image === undefined ) return false;\r\n\r\n\treturn image.height > 0;\r\n\r\n}");own(module,"isEquirectangularMapReady",function);}
{auto function=JSValue::constructor(helper1,{module},"onTextureDispose",1,JSValue::ConstructorKind::Callable);function.setFunctionSourceText("function onTextureDispose( event ) {\r\n\r\n\tconst texture = event.target;\r\n\r\n\ttexture.removeEventListener( 'dispose', onTextureDispose );\r\n\r\n\tconst renderTarget = _cache.get( texture );\r\n\r\n\tif ( renderTarget !== undefined ) {\r\n\r\n\t\t_cache.delete( texture );\r\n\r\n\t\trenderTarget.dispose();\r\n\r\n\t}\r\n\r\n}");own(module,"onTextureDispose",function);}
{auto function=JSValue::constructor(helper2,{module},"mapTextureMapping",2,JSValue::ConstructorKind::Callable);function.setFunctionSourceText("function mapTextureMapping( texture, mapping ) {\r\n\r\n\tif ( mapping === EquirectangularReflectionMapping ) {\r\n\r\n\t\ttexture.mapping = CubeReflectionMapping;\r\n\r\n\t} else if ( mapping === EquirectangularRefractionMapping ) {\r\n\r\n\t\ttexture.mapping = CubeRefractionMapping;\r\n\r\n\t}\r\n\r\n}");own(module,"mapTextureMapping",function);}
own(module,"_cache",([&]()->JSValue{const auto nativeStatementTemp246=nativeGlobalBinding("WeakMap");Args nativeStatementTemp247;return nativeStatementTemp246.construct(nativeStatementTemp247);}()));
auto ctor=JSValue::builtinConstructor(method1,{module},"CubeMapNode",1);ctor.setFunctionSourceText("class CubeMapNode extends Node {\r\n\r\n\tstatic get type() {\r\n\r\n\t\treturn 'CubeMapNode';\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Constructs a new cube map node.\r\n\t *\r\n\t * @param {Node} envNode - The node representing the environment map.\r\n\t */\r\n\tconstructor( envNode ) {\r\n\r\n\t\tsuper( 'vec3' );\r\n\r\n\t\t/**\r\n\t\t * The node representing the environment map.\r\n\t\t *\r\n\t\t * @type {Node}\r\n\t\t */\r\n\t\tthis.envNode = envNode;\r\n\r\n\t\t/**\r\n\t\t * A reference to the internal cube texture.\r\n\t\t *\r\n\t\t * @private\r\n\t\t * @type {?CubeTexture}\r\n\t\t * @default null\r\n\t\t */\r\n\t\tthis._cubeTexture = null;\r\n\r\n\t\t/**\r\n\t\t * A reference to the internal cube texture node.\r\n\t\t *\r\n\t\t * @private\r\n\t\t * @type {CubeTextureNode}\r\n\t\t */\r\n\t\tthis._cubeTextureNode = cubeTexture( null );\r\n\r\n\t\tconst defaultTexture = new CubeTexture();\r\n\t\tdefaultTexture.isRenderTargetTexture = true;\r\n\r\n\t\t/**\r\n\t\t * A default cube texture that acts as a placeholder.\r\n\t\t * It is used when the conversion from equirectangular to cube\r\n\t\t * map has not finished yet for a given texture.\r\n\t\t *\r\n\t\t * @private\r\n\t\t * @type {CubeTexture}\r\n\t\t */\r\n\t\tthis._defaultTexture = defaultTexture;\r\n\r\n\t\t/**\r\n\t\t * The `updateBeforeType` is set to `NodeUpdateType.RENDER` since the node updates\r\n\t\t * the texture once per render in its {@link CubeMapNode#updateBefore} method.\r\n\t\t *\r\n\t\t * @type {string}\r\n\t\t * @default 'render'\r\n\t\t */\r\n\t\tthis.updateBeforeType = NodeUpdateType.RENDER;\r\n\r\n\t}\r\n\r\n\tupdateBefore( frame ) {\r\n\r\n\t\tconst { renderer, material } = frame;\r\n\r\n\t\tconst envNode = this.envNode;\r\n\r\n\t\tif ( envNode.isTextureNode || envNode.isMaterialReferenceNode ) {\r\n\r\n\t\t\tconst texture = ( envNode.isTextureNode ) ? envNode.value : material[ envNode.property ];\r\n\r\n\t\t\tif ( texture && texture.isTexture ) {\r\n\r\n\t\t\t\tconst mapping = texture.mapping;\r\n\r\n\t\t\t\tif ( mapping === EquirectangularReflectionMapping || mapping === EquirectangularRefractionMapping ) {\r\n\r\n\t\t\t\t\t// check for converted cubemap map\r\n\r\n\t\t\t\t\tif ( _cache.has( texture ) ) {\r\n\r\n\t\t\t\t\t\tconst cubeMap = _cache.get( texture );\r\n\r\n\t\t\t\t\t\tmapTextureMapping( cubeMap, texture.mapping );\r\n\t\t\t\t\t\tthis._cubeTexture = cubeMap;\r\n\r\n\t\t\t\t\t} else {\r\n\r\n\t\t\t\t\t\t// create cube map from equirectangular map\r\n\r\n\t\t\t\t\t\tconst image = texture.image;\r\n\r\n\t\t\t\t\t\tif ( isEquirectangularMapReady( image ) ) {\r\n\r\n\t\t\t\t\t\t\tconst renderTarget = new CubeRenderTarget( image.height );\r\n\t\t\t\t\t\t\trenderTarget.fromEquirectangularTexture( renderer, texture );\r\n\r\n\t\t\t\t\t\t\tmapTextureMapping( renderTarget.texture, texture.mapping );\r\n\t\t\t\t\t\t\tthis._cubeTexture = renderTarget.texture;\r\n\r\n\t\t\t\t\t\t\t_cache.set( texture, renderTarget.texture );\r\n\r\n\t\t\t\t\t\t\ttexture.addEventListener( 'dispose', onTextureDispose );\r\n\r\n\t\t\t\t\t\t} else {\r\n\r\n\t\t\t\t\t\t\t// default cube texture as fallback when equirectangular texture is not yet loaded\r\n\r\n\t\t\t\t\t\t\tthis._cubeTexture = this._defaultTexture;\r\n\r\n\t\t\t\t\t\t}\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t\t//\r\n\r\n\t\t\t\t\tthis._cubeTextureNode.value = this._cubeTexture;\r\n\r\n\t\t\t\t} else {\r\n\r\n\t\t\t\t\t// envNode already refers to a cube map\r\n\r\n\t\t\t\t\tthis._cubeTextureNode = this.envNode;\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t}\r\n\r\n\tsetup( builder ) {\r\n\r\n\t\tthis.updateBefore( builder );\r\n\r\n\t\treturn this._cubeTextureNode;\r\n\r\n\t}\r\n\r\n}");auto prototype=ctor.get("prototype");own(module,"CubeMapNode",ctor);own(module,"_home",prototype);
ctor.setPrototype(module.get("Node"));prototype.setPrototype(module.get("Node").get("prototype"));
{auto function=JSValue::function(method0,{module},"get type",0);function.setFunctionSourceText("get type() {\r\n\r\n\t\treturn 'CubeMapNode';\r\n\r\n\t}");JSPropertyDescriptor d;d.getter=function;d.configurable=true;ctor.defineProperty("type",d);}
{auto function=JSValue::function(method2,{module},"updateBefore",1);function.setFunctionSourceText("updateBefore( frame ) {\r\n\r\n\t\tconst { renderer, material } = frame;\r\n\r\n\t\tconst envNode = this.envNode;\r\n\r\n\t\tif ( envNode.isTextureNode || envNode.isMaterialReferenceNode ) {\r\n\r\n\t\t\tconst texture = ( envNode.isTextureNode ) ? envNode.value : material[ envNode.property ];\r\n\r\n\t\t\tif ( texture && texture.isTexture ) {\r\n\r\n\t\t\t\tconst mapping = texture.mapping;\r\n\r\n\t\t\t\tif ( mapping === EquirectangularReflectionMapping || mapping === EquirectangularRefractionMapping ) {\r\n\r\n\t\t\t\t\t// check for converted cubemap map\r\n\r\n\t\t\t\t\tif ( _cache.has( texture ) ) {\r\n\r\n\t\t\t\t\t\tconst cubeMap = _cache.get( texture );\r\n\r\n\t\t\t\t\t\tmapTextureMapping( cubeMap, texture.mapping );\r\n\t\t\t\t\t\tthis._cubeTexture = cubeMap;\r\n\r\n\t\t\t\t\t} else {\r\n\r\n\t\t\t\t\t\t// create cube map from equirectangular map\r\n\r\n\t\t\t\t\t\tconst image = texture.image;\r\n\r\n\t\t\t\t\t\tif ( isEquirectangularMapReady( image ) ) {\r\n\r\n\t\t\t\t\t\t\tconst renderTarget = new CubeRenderTarget( image.height );\r\n\t\t\t\t\t\t\trenderTarget.fromEquirectangularTexture( renderer, texture );\r\n\r\n\t\t\t\t\t\t\tmapTextureMapping( renderTarget.texture, texture.mapping );\r\n\t\t\t\t\t\t\tthis._cubeTexture = renderTarget.texture;\r\n\r\n\t\t\t\t\t\t\t_cache.set( texture, renderTarget.texture );\r\n\r\n\t\t\t\t\t\t\ttexture.addEventListener( 'dispose', onTextureDispose );\r\n\r\n\t\t\t\t\t\t} else {\r\n\r\n\t\t\t\t\t\t\t// default cube texture as fallback when equirectangular texture is not yet loaded\r\n\r\n\t\t\t\t\t\t\tthis._cubeTexture = this._defaultTexture;\r\n\r\n\t\t\t\t\t\t}\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t\t//\r\n\r\n\t\t\t\t\tthis._cubeTextureNode.value = this._cubeTexture;\r\n\r\n\t\t\t\t} else {\r\n\r\n\t\t\t\t\t// envNode already refers to a cube map\r\n\r\n\t\t\t\t\tthis._cubeTextureNode = this.envNode;\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t}");JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("updateBefore",d);}
{auto function=JSValue::function(method3,{module},"setup",1);function.setFunctionSourceText("setup( builder ) {\r\n\r\n\t\tthis.updateBefore( builder );\r\n\r\n\t\treturn this._cubeTextureNode;\r\n\r\n\t}");JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("setup",d);}
own(module,"cubeMapNode",([&]()->JSValue{auto nativeStatementTemp251=([&]()->JSValue{const auto nativeStatementTemp253=module.get("nodeProxy");const JSValue nativeStatementTemp254;Args nativeStatementTemp255;nativeStatementTemp255.push_back(module.get("CubeMapNode"));return nativeStatementTemp253.call(nativeStatementTemp254,nativeStatementTemp255);}());const auto nativeStatementTemp252=JSValue("setParameterLength");const auto nativeStatementTemp248=nativeStatementTemp251.get(nativeStatementTemp252);const auto nativeStatementTemp249=nativeStatementTemp251;Args nativeStatementTemp250;nativeStatementTemp250.push_back(JSValue(1));return nativeStatementTemp248.call(nativeStatementTemp249,nativeStatementTemp250);}()));
own(module,"default",ctor);module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();own(facade,"default",ctor);own(facade,"cubeMapNode",module.get("cubeMapNode"));facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeCubeMapNodeBinding(const std::string& name){const auto keeper=nativeCubeMapNodeModule();return CubeMapNodeManaged::lexical().lock()->get(name);}
JSValue nativeCubeMapNodeConstructor(){return nativeCubeMapNodeBinding("default");}
