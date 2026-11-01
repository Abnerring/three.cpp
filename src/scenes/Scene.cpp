#include "scenes/Scene.h"
#include "materials/Material.h"
#include "textures/Texture.h"
#include <stdexcept>

std::optional<JsonValue> Scene::propertyValue(const std::string& property) const {
    if (property == "isScene")
        return isScene;
    if (property == "backgroundBlurriness")
        return backgroundBlurriness;
    if (property == "backgroundIntensity")
        return backgroundIntensity;
    if (property == "environmentIntensity")
        return environmentIntensity;
    return Object3D::propertyValue(property);
}
Scene& Scene::copy(const Object3D& source, bool recursive) {
    const auto* scene = dynamic_cast<const Scene*>(&source);
    if (!scene)
        throw std::invalid_argument("Scene::copy requires a Scene");
    Object3D::copy(source, recursive);
    background = scene->background;
    const auto sourceEnvironment = scene->environment;
    environment = sourceEnvironment ? sourceEnvironment->clone() : nullptr;
    fog = scene->fog ? scene->fog->clone() : nullptr;
    backgroundBlurriness = scene->backgroundBlurriness;
    backgroundIntensity = scene->backgroundIntensity;
    backgroundRotation.copy(scene->backgroundRotation);
    environmentIntensity = scene->environmentIntensity;
    environmentRotation.copy(scene->environmentRotation);
    overrideMaterial = scene->overrideMaterial ? scene->overrideMaterial->clone() : nullptr;
    return *this;
}
std::shared_ptr<Object3D> Scene::clone(bool recursive) const {
    auto result = std::make_shared<Scene>();
    result->copy(*this, recursive);
    return result;
}
void Scene::serializeObjectResources(Object3DJSONContext& meta, JsonValue& object) const {
    if (background)
        object["background"] = background->getHex();
    const auto texture = environment;
    if (texture && !texture->isRenderTargetTexture) object["environment"] = texture->toJSON(&meta).at("uuid");
}
JsonValue Scene::toJSON(Object3DJSONContext* meta) const {
    auto output = Object3D::toJSON(meta);
    auto& object = output["object"];
    if (fog)
        object["fog"] = fog->toJSON();
    object["backgroundBlurriness"] = backgroundBlurriness;
    object["backgroundIntensity"] = backgroundIntensity;
    object["backgroundRotation"] =
        JsonValue::Array{backgroundRotation.x, backgroundRotation.y, backgroundRotation.z,
                         backgroundRotation.getOrderString()};
    object["environmentIntensity"] = environmentIntensity;
    object["environmentRotation"] =
        JsonValue::Array{environmentRotation.x, environmentRotation.y, environmentRotation.z,
                         environmentRotation.getOrderString()};
    return output;
}

// Complete managed native port of pinned Scene.js. See THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSPromise.h"
#include "core/JSError.h"
#include "math/JSNumberMath.h"
#include "scenes/SceneRuntimeSource.inc"
#include "core/Object3D.h"
#include "math/Euler.h"
#include <string_view>
#include <unordered_set>
namespace SceneManaged{

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

JSValue makeLexicalCell(bool mutableBinding){auto cell=JSValue::object(nullptr);cell.setInternalSlot("initialized",false);cell.setInternalSlot("mutable",mutableBinding);return cell;}
JSValue readLexical(const JSValue& cell){if(!cell.getInternalSlot("initialized").toBoolean())throw JSReferenceError("Cannot access lexical binding before initialization");return cell.getInternalSlot("value");}
void initializeLexical(JSValue cell,const JSValue& value){if(cell.getInternalSlot("initialized").toBoolean())throw std::logic_error("Lexical binding initialized twice");cell.setInternalSlot("value",value);cell.setInternalSlot("initialized",true);}
void writeLexical(JSValue cell,const JSValue& value){if(!cell.getInternalSlot("initialized").toBoolean())throw JSReferenceError("Cannot access lexical binding before initialization");if(!cell.getInternalSlot("mutable").toBoolean())throw JSTypeError("Assignment to constant variable");cell.setInternalSlot("value",value);}

JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}

JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
const auto nativeStatementTemp5=nativeStatementTemp0;nativeStatementTemp4={};
([&]()->JSValue{const auto nativeStatementTemp9=nativeStatementTemp3.get("Scene").getPrototype();Args nativeStatementTemp8;nativeStatementTemp4=nativeStatementTemp9.construct(nativeStatementTemp8,nativeStatementTemp5);return nativeStatementTemp4;}());
([&]()->JSValue{auto nativeStatementTemp10=nativeStatementTemp4;const auto nativeStatementTemp11=JSValue("isScene");const auto nativeStatementTemp12=JSValue(true);setReference(nativeStatementTemp10,nativeStatementTemp11,nativeStatementTemp12);return nativeStatementTemp12;}());
([&]()->JSValue{auto nativeStatementTemp15=nativeStatementTemp4;const auto nativeStatementTemp16=JSValue("type");const auto nativeStatementTemp17=JSValue("Scene");setReference(nativeStatementTemp15,nativeStatementTemp16,nativeStatementTemp17);return nativeStatementTemp17;}());
([&]()->JSValue{auto nativeStatementTemp20=nativeStatementTemp4;const auto nativeStatementTemp21=JSValue("background");const auto nativeStatementTemp22=JSValue(nullptr);setReference(nativeStatementTemp20,nativeStatementTemp21,nativeStatementTemp22);return nativeStatementTemp22;}());
([&]()->JSValue{auto nativeStatementTemp25=nativeStatementTemp4;const auto nativeStatementTemp26=JSValue("environment");const auto nativeStatementTemp27=JSValue(nullptr);setReference(nativeStatementTemp25,nativeStatementTemp26,nativeStatementTemp27);return nativeStatementTemp27;}());
([&]()->JSValue{auto nativeStatementTemp30=nativeStatementTemp4;const auto nativeStatementTemp31=JSValue("fog");const auto nativeStatementTemp32=JSValue(nullptr);setReference(nativeStatementTemp30,nativeStatementTemp31,nativeStatementTemp32);return nativeStatementTemp32;}());
([&]()->JSValue{auto nativeStatementTemp35=nativeStatementTemp4;const auto nativeStatementTemp36=JSValue("backgroundBlurriness");const auto nativeStatementTemp37=JSValue(0);setReference(nativeStatementTemp35,nativeStatementTemp36,nativeStatementTemp37);return nativeStatementTemp37;}());
([&]()->JSValue{auto nativeStatementTemp40=nativeStatementTemp4;const auto nativeStatementTemp41=JSValue("backgroundIntensity");const auto nativeStatementTemp42=JSValue(1);setReference(nativeStatementTemp40,nativeStatementTemp41,nativeStatementTemp42);return nativeStatementTemp42;}());
([&]()->JSValue{auto nativeStatementTemp45=nativeStatementTemp4;const auto nativeStatementTemp46=JSValue("backgroundRotation");const auto nativeStatementTemp47=([&]()->JSValue{const auto nativeStatementTemp50=nativeStatementTemp3.get("Euler");Args nativeStatementTemp51;return nativeStatementTemp50.construct(nativeStatementTemp51);}());setReference(nativeStatementTemp45,nativeStatementTemp46,nativeStatementTemp47);return nativeStatementTemp47;}());
([&]()->JSValue{auto nativeStatementTemp52=nativeStatementTemp4;const auto nativeStatementTemp53=JSValue("environmentIntensity");const auto nativeStatementTemp54=JSValue(1);setReference(nativeStatementTemp52,nativeStatementTemp53,nativeStatementTemp54);return nativeStatementTemp54;}());
([&]()->JSValue{auto nativeStatementTemp57=nativeStatementTemp4;const auto nativeStatementTemp58=JSValue("environmentRotation");const auto nativeStatementTemp59=([&]()->JSValue{const auto nativeStatementTemp62=nativeStatementTemp3.get("Euler");Args nativeStatementTemp63;return nativeStatementTemp62.construct(nativeStatementTemp63);}());setReference(nativeStatementTemp57,nativeStatementTemp58,nativeStatementTemp59);return nativeStatementTemp59;}());
([&]()->JSValue{auto nativeStatementTemp64=nativeStatementTemp4;const auto nativeStatementTemp65=JSValue("overrideMaterial");const auto nativeStatementTemp66=JSValue(nullptr);setReference(nativeStatementTemp64,nativeStatementTemp65,nativeStatementTemp66);return nativeStatementTemp66;}());
if(([&]()->JSValue{const auto nativeStatementTemp69=([&]()->JSValue{const auto nativeStatementTemp71=nativeGlobalObject();if(!nativeStatementTemp71.has("__THREE_DEVTOOLS__"))return JSValue("undefined");return typeOf(nativeStatementTemp71.get("__THREE_DEVTOOLS__"));}());const auto nativeStatementTemp70=JSValue("undefined");return binary("!==",nativeStatementTemp69,nativeStatementTemp70);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp75=nativeGlobalBinding("__THREE_DEVTOOLS__");const auto nativeStatementTemp76=JSValue("dispatchEvent");const auto nativeStatementTemp72=nativeStatementTemp75.get(nativeStatementTemp76);const auto nativeStatementTemp73=nativeStatementTemp75;Args nativeStatementTemp74;nativeStatementTemp74.push_back(([&]()->JSValue{const auto nativeStatementTemp77=nativeGlobalBinding("CustomEvent");Args nativeStatementTemp78;nativeStatementTemp78.push_back(JSValue("observe"));nativeStatementTemp78.push_back(([&]()->JSValue{auto nativeStatementTemp79=JSValue::object();{const auto nativeStatementTemp80=JSValue("detail");const auto nativeStatementTemp81=nativeStatementTemp4;own(nativeStatementTemp79,nativeStatementTemp80,nativeStatementTemp81);}return nativeStatementTemp79;}()));return nativeStatementTemp77.construct(nativeStatementTemp78);}()));return nativeStatementTemp72.call(nativeStatementTemp73,nativeStatementTemp74);}());

}
return nativeStatementTemp4;
}

JSValue method1(const JSValue& nativeStatementTemp82,const Args& nativeStatementTemp83,const Args& nativeStatementTemp84){
auto nativeStatementTemp85=nativeStatementTemp84[0];auto nativeStatementTemp86=nativeStatementTemp82;
JSValue source=arg(nativeStatementTemp83,0);
JSValue recursive=arg(nativeStatementTemp83,1);
([&]()->JSValue{const auto nativeStatementTemp88=nativeStatementTemp84[1].getPrototype();const auto nativeStatementTemp89=JSValue("copy");const auto nativeStatementTemp90=nativeStatementTemp88.get(nativeStatementTemp89,nativeStatementTemp86);Args nativeStatementTemp91;nativeStatementTemp91.push_back(source);nativeStatementTemp91.push_back(recursive);return nativeStatementTemp90.call(nativeStatementTemp86,nativeStatementTemp91);}());
([&]()->JSValue{auto nativeStatementTemp92=nativeStatementTemp86;const auto nativeStatementTemp93=JSValue("background");const auto nativeStatementTemp94=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp97=([&]()->JSValue{const auto nativeStatementTemp99=source;const auto nativeStatementTemp100=JSValue("background");return nativeStatementTemp99.get(nativeStatementTemp100);}());const auto nativeStatementTemp98=JSValue(nullptr);return binary("!==",nativeStatementTemp97,nativeStatementTemp98);}()).toBoolean())return ([&]()->JSValue{auto nativeStatementTemp104=([&]()->JSValue{const auto nativeStatementTemp106=source;const auto nativeStatementTemp107=JSValue("background");return nativeStatementTemp106.get(nativeStatementTemp107);}());const auto nativeStatementTemp105=JSValue("clone");const auto nativeStatementTemp101=nativeStatementTemp104.get(nativeStatementTemp105);const auto nativeStatementTemp102=nativeStatementTemp104;Args nativeStatementTemp103;return nativeStatementTemp101.call(nativeStatementTemp102,nativeStatementTemp103);}());return JSValue(nullptr);}());setReference(nativeStatementTemp92,nativeStatementTemp93,nativeStatementTemp94);return nativeStatementTemp94;}());
([&]()->JSValue{auto nativeStatementTemp108=nativeStatementTemp86;const auto nativeStatementTemp109=JSValue("environment");const auto nativeStatementTemp110=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp113=([&]()->JSValue{const auto nativeStatementTemp115=source;const auto nativeStatementTemp116=JSValue("environment");return nativeStatementTemp115.get(nativeStatementTemp116);}());const auto nativeStatementTemp114=JSValue(nullptr);return binary("!==",nativeStatementTemp113,nativeStatementTemp114);}()).toBoolean())return ([&]()->JSValue{auto nativeStatementTemp120=([&]()->JSValue{const auto nativeStatementTemp122=source;const auto nativeStatementTemp123=JSValue("environment");return nativeStatementTemp122.get(nativeStatementTemp123);}());const auto nativeStatementTemp121=JSValue("clone");const auto nativeStatementTemp117=nativeStatementTemp120.get(nativeStatementTemp121);const auto nativeStatementTemp118=nativeStatementTemp120;Args nativeStatementTemp119;return nativeStatementTemp117.call(nativeStatementTemp118,nativeStatementTemp119);}());return JSValue(nullptr);}());setReference(nativeStatementTemp108,nativeStatementTemp109,nativeStatementTemp110);return nativeStatementTemp110;}());
([&]()->JSValue{auto nativeStatementTemp124=nativeStatementTemp86;const auto nativeStatementTemp125=JSValue("fog");const auto nativeStatementTemp126=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp129=([&]()->JSValue{const auto nativeStatementTemp131=source;const auto nativeStatementTemp132=JSValue("fog");return nativeStatementTemp131.get(nativeStatementTemp132);}());const auto nativeStatementTemp130=JSValue(nullptr);return binary("!==",nativeStatementTemp129,nativeStatementTemp130);}()).toBoolean())return ([&]()->JSValue{auto nativeStatementTemp136=([&]()->JSValue{const auto nativeStatementTemp138=source;const auto nativeStatementTemp139=JSValue("fog");return nativeStatementTemp138.get(nativeStatementTemp139);}());const auto nativeStatementTemp137=JSValue("clone");const auto nativeStatementTemp133=nativeStatementTemp136.get(nativeStatementTemp137);const auto nativeStatementTemp134=nativeStatementTemp136;Args nativeStatementTemp135;return nativeStatementTemp133.call(nativeStatementTemp134,nativeStatementTemp135);}());return JSValue(nullptr);}());setReference(nativeStatementTemp124,nativeStatementTemp125,nativeStatementTemp126);return nativeStatementTemp126;}());
([&]()->JSValue{auto nativeStatementTemp140=nativeStatementTemp86;const auto nativeStatementTemp141=JSValue("backgroundBlurriness");const auto nativeStatementTemp142=([&]()->JSValue{const auto nativeStatementTemp145=source;const auto nativeStatementTemp146=JSValue("backgroundBlurriness");return nativeStatementTemp145.get(nativeStatementTemp146);}());setReference(nativeStatementTemp140,nativeStatementTemp141,nativeStatementTemp142);return nativeStatementTemp142;}());
([&]()->JSValue{auto nativeStatementTemp147=nativeStatementTemp86;const auto nativeStatementTemp148=JSValue("backgroundIntensity");const auto nativeStatementTemp149=([&]()->JSValue{const auto nativeStatementTemp152=source;const auto nativeStatementTemp153=JSValue("backgroundIntensity");return nativeStatementTemp152.get(nativeStatementTemp153);}());setReference(nativeStatementTemp147,nativeStatementTemp148,nativeStatementTemp149);return nativeStatementTemp149;}());
([&]()->JSValue{auto nativeStatementTemp157=([&]()->JSValue{const auto nativeStatementTemp159=nativeStatementTemp86;const auto nativeStatementTemp160=JSValue("backgroundRotation");return nativeStatementTemp159.get(nativeStatementTemp160);}());const auto nativeStatementTemp158=JSValue("copy");const auto nativeStatementTemp154=nativeStatementTemp157.get(nativeStatementTemp158);const auto nativeStatementTemp155=nativeStatementTemp157;Args nativeStatementTemp156;nativeStatementTemp156.push_back(([&]()->JSValue{const auto nativeStatementTemp161=source;const auto nativeStatementTemp162=JSValue("backgroundRotation");return nativeStatementTemp161.get(nativeStatementTemp162);}()));return nativeStatementTemp154.call(nativeStatementTemp155,nativeStatementTemp156);}());
([&]()->JSValue{auto nativeStatementTemp163=nativeStatementTemp86;const auto nativeStatementTemp164=JSValue("environmentIntensity");const auto nativeStatementTemp165=([&]()->JSValue{const auto nativeStatementTemp168=source;const auto nativeStatementTemp169=JSValue("environmentIntensity");return nativeStatementTemp168.get(nativeStatementTemp169);}());setReference(nativeStatementTemp163,nativeStatementTemp164,nativeStatementTemp165);return nativeStatementTemp165;}());
([&]()->JSValue{auto nativeStatementTemp173=([&]()->JSValue{const auto nativeStatementTemp175=nativeStatementTemp86;const auto nativeStatementTemp176=JSValue("environmentRotation");return nativeStatementTemp175.get(nativeStatementTemp176);}());const auto nativeStatementTemp174=JSValue("copy");const auto nativeStatementTemp170=nativeStatementTemp173.get(nativeStatementTemp174);const auto nativeStatementTemp171=nativeStatementTemp173;Args nativeStatementTemp172;nativeStatementTemp172.push_back(([&]()->JSValue{const auto nativeStatementTemp177=source;const auto nativeStatementTemp178=JSValue("environmentRotation");return nativeStatementTemp177.get(nativeStatementTemp178);}()));return nativeStatementTemp170.call(nativeStatementTemp171,nativeStatementTemp172);}());
([&]()->JSValue{auto nativeStatementTemp179=nativeStatementTemp86;const auto nativeStatementTemp180=JSValue("overrideMaterial");const auto nativeStatementTemp181=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp184=([&]()->JSValue{const auto nativeStatementTemp186=source;const auto nativeStatementTemp187=JSValue("overrideMaterial");return nativeStatementTemp186.get(nativeStatementTemp187);}());const auto nativeStatementTemp185=JSValue(nullptr);return binary("!==",nativeStatementTemp184,nativeStatementTemp185);}()).toBoolean())return ([&]()->JSValue{auto nativeStatementTemp191=([&]()->JSValue{const auto nativeStatementTemp193=source;const auto nativeStatementTemp194=JSValue("overrideMaterial");return nativeStatementTemp193.get(nativeStatementTemp194);}());const auto nativeStatementTemp192=JSValue("clone");const auto nativeStatementTemp188=nativeStatementTemp191.get(nativeStatementTemp192);const auto nativeStatementTemp189=nativeStatementTemp191;Args nativeStatementTemp190;return nativeStatementTemp188.call(nativeStatementTemp189,nativeStatementTemp190);}());return JSValue(nullptr);}());setReference(nativeStatementTemp179,nativeStatementTemp180,nativeStatementTemp181);return nativeStatementTemp181;}());
([&]()->JSValue{auto nativeStatementTemp195=nativeStatementTemp86;const auto nativeStatementTemp196=JSValue("matrixAutoUpdate");const auto nativeStatementTemp197=([&]()->JSValue{const auto nativeStatementTemp200=source;const auto nativeStatementTemp201=JSValue("matrixAutoUpdate");return nativeStatementTemp200.get(nativeStatementTemp201);}());setReference(nativeStatementTemp195,nativeStatementTemp196,nativeStatementTemp197);return nativeStatementTemp197;}());
return nativeStatementTemp86;
return JSValue{};
}

JSValue method2(const JSValue& nativeStatementTemp202,const Args& nativeStatementTemp203,const Args& nativeStatementTemp204){
auto nativeStatementTemp205=nativeStatementTemp204[0];auto nativeStatementTemp206=nativeStatementTemp202;
JSValue meta=arg(nativeStatementTemp203,0);
JSValue data=([&]()->JSValue{const auto nativeStatementTemp208=nativeStatementTemp204[1].getPrototype();const auto nativeStatementTemp209=JSValue("toJSON");const auto nativeStatementTemp210=nativeStatementTemp208.get(nativeStatementTemp209,nativeStatementTemp206);Args nativeStatementTemp211;nativeStatementTemp211.push_back(meta);return nativeStatementTemp210.call(nativeStatementTemp206,nativeStatementTemp211);}());
if(([&]()->JSValue{const auto nativeStatementTemp212=([&]()->JSValue{const auto nativeStatementTemp214=nativeStatementTemp206;const auto nativeStatementTemp215=JSValue("fog");return nativeStatementTemp214.get(nativeStatementTemp215);}());const auto nativeStatementTemp213=JSValue(nullptr);return binary("!==",nativeStatementTemp212,nativeStatementTemp213);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp216=([&]()->JSValue{const auto nativeStatementTemp218=data;const auto nativeStatementTemp219=JSValue("object");return nativeStatementTemp218.get(nativeStatementTemp219);}());const auto nativeStatementTemp217=JSValue("fog");const auto nativeStatementTemp220=([&]()->JSValue{auto nativeStatementTemp226=([&]()->JSValue{const auto nativeStatementTemp228=nativeStatementTemp206;const auto nativeStatementTemp229=JSValue("fog");return nativeStatementTemp228.get(nativeStatementTemp229);}());const auto nativeStatementTemp227=JSValue("toJSON");const auto nativeStatementTemp223=nativeStatementTemp226.get(nativeStatementTemp227);const auto nativeStatementTemp224=nativeStatementTemp226;Args nativeStatementTemp225;return nativeStatementTemp223.call(nativeStatementTemp224,nativeStatementTemp225);}());setReference(nativeStatementTemp216,nativeStatementTemp217,nativeStatementTemp220);return nativeStatementTemp220;}());
([&]()->JSValue{auto nativeStatementTemp230=([&]()->JSValue{const auto nativeStatementTemp232=data;const auto nativeStatementTemp233=JSValue("object");return nativeStatementTemp232.get(nativeStatementTemp233);}());const auto nativeStatementTemp231=JSValue("backgroundBlurriness");const auto nativeStatementTemp234=([&]()->JSValue{const auto nativeStatementTemp237=nativeStatementTemp206;const auto nativeStatementTemp238=JSValue("backgroundBlurriness");return nativeStatementTemp237.get(nativeStatementTemp238);}());setReference(nativeStatementTemp230,nativeStatementTemp231,nativeStatementTemp234);return nativeStatementTemp234;}());
([&]()->JSValue{auto nativeStatementTemp239=([&]()->JSValue{const auto nativeStatementTemp241=data;const auto nativeStatementTemp242=JSValue("object");return nativeStatementTemp241.get(nativeStatementTemp242);}());const auto nativeStatementTemp240=JSValue("backgroundIntensity");const auto nativeStatementTemp243=([&]()->JSValue{const auto nativeStatementTemp246=nativeStatementTemp206;const auto nativeStatementTemp247=JSValue("backgroundIntensity");return nativeStatementTemp246.get(nativeStatementTemp247);}());setReference(nativeStatementTemp239,nativeStatementTemp240,nativeStatementTemp243);return nativeStatementTemp243;}());
([&]()->JSValue{auto nativeStatementTemp248=([&]()->JSValue{const auto nativeStatementTemp250=data;const auto nativeStatementTemp251=JSValue("object");return nativeStatementTemp250.get(nativeStatementTemp251);}());const auto nativeStatementTemp249=JSValue("backgroundRotation");const auto nativeStatementTemp252=([&]()->JSValue{auto nativeStatementTemp258=([&]()->JSValue{const auto nativeStatementTemp260=nativeStatementTemp206;const auto nativeStatementTemp261=JSValue("backgroundRotation");return nativeStatementTemp260.get(nativeStatementTemp261);}());const auto nativeStatementTemp259=JSValue("toArray");const auto nativeStatementTemp255=nativeStatementTemp258.get(nativeStatementTemp259);const auto nativeStatementTemp256=nativeStatementTemp258;Args nativeStatementTemp257;return nativeStatementTemp255.call(nativeStatementTemp256,nativeStatementTemp257);}());setReference(nativeStatementTemp248,nativeStatementTemp249,nativeStatementTemp252);return nativeStatementTemp252;}());
([&]()->JSValue{auto nativeStatementTemp262=([&]()->JSValue{const auto nativeStatementTemp264=data;const auto nativeStatementTemp265=JSValue("object");return nativeStatementTemp264.get(nativeStatementTemp265);}());const auto nativeStatementTemp263=JSValue("environmentIntensity");const auto nativeStatementTemp266=([&]()->JSValue{const auto nativeStatementTemp269=nativeStatementTemp206;const auto nativeStatementTemp270=JSValue("environmentIntensity");return nativeStatementTemp269.get(nativeStatementTemp270);}());setReference(nativeStatementTemp262,nativeStatementTemp263,nativeStatementTemp266);return nativeStatementTemp266;}());
([&]()->JSValue{auto nativeStatementTemp271=([&]()->JSValue{const auto nativeStatementTemp273=data;const auto nativeStatementTemp274=JSValue("object");return nativeStatementTemp273.get(nativeStatementTemp274);}());const auto nativeStatementTemp272=JSValue("environmentRotation");const auto nativeStatementTemp275=([&]()->JSValue{auto nativeStatementTemp281=([&]()->JSValue{const auto nativeStatementTemp283=nativeStatementTemp206;const auto nativeStatementTemp284=JSValue("environmentRotation");return nativeStatementTemp283.get(nativeStatementTemp284);}());const auto nativeStatementTemp282=JSValue("toArray");const auto nativeStatementTemp278=nativeStatementTemp281.get(nativeStatementTemp282);const auto nativeStatementTemp279=nativeStatementTemp281;Args nativeStatementTemp280;return nativeStatementTemp278.call(nativeStatementTemp279,nativeStatementTemp280);}());setReference(nativeStatementTemp271,nativeStatementTemp272,nativeStatementTemp275);return nativeStatementTemp275;}());
return data;
return JSValue{};
}
}
JSValue nativeSceneModule(){using namespace SceneManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject()){const auto status=cached->getInternalSlot("status");if(status.strictEquals("evaluated"))return cached->getInternalSlot("facade");if(status.strictEquals("failed"))throw JSThrownValue(cached->getInternalSlot("error"));throw JSModuleInitializationError("Scene module");}auto module=JSValue::object(nullptr);module.retainForHeapLifetime();weak=JSWeakValue(module);lexical()=weak;module.setInternalSlot("status","initializing");try{
own(module,"Object3D",nativeObject3DConstructor());
own(module,"Euler",nativeEulerConstructor());
auto ctor=JSValue::builtinConstructor(method0,{module},"Scene",0);ctor.setFunctionSourceText(SceneRuntimeSource::classSource);ctor.setPrototype(module.get("Object3D"));auto prototype=ctor.get("prototype");prototype.setPrototype(module.get("Object3D").get("prototype"));own(module,"Scene",ctor);
{auto home=prototype;auto fn=JSValue::function(method1,{module,home},"copy",2);fn.setFunctionSourceText(SceneRuntimeSource::method1);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;home.defineProperty("copy",d);}
{auto home=prototype;auto fn=JSValue::function(method2,{module,home},"toJSON",1);fn.setFunctionSourceText(SceneRuntimeSource::method2);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;home.defineProperty("toJSON",d);}
auto facade=JSValue::object();own(facade,"Scene",ctor);module.setInternalSlot("facade",facade);module.setInternalSlot("status","evaluated");return facade;}catch(...){module.setInternalSlot("error",nativeExceptionValue(std::current_exception()));module.setInternalSlot("status","failed");throw JSThrownValue(module.getInternalSlot("error"));}}
JSValue nativeSceneBinding(const std::string& name){const auto keeper=nativeSceneModule();return SceneManaged::lexical().lock()->get(name);}
JSValue nativeSceneConstructor(){return nativeSceneBinding("Scene");}
