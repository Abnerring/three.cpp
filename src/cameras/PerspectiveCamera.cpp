#include "cameras/PerspectiveCamera.h"
#include "math/MathUtils.h"
#include "math/JSNumberMath.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

PerspectiveCamera::PerspectiveCamera(HIGH_PRECISION fov, HIGH_PRECISION aspect, HIGH_PRECISION near,
                                     HIGH_PRECISION far)
    : fov(fov), near(near), far(far), aspect(aspect) {
    type = "PerspectiveCamera";
    updateProjectionMatrix();
}
std::optional<JsonValue> PerspectiveCamera::propertyValue(const std::string& property) const {
    if (property == "isPerspectiveCamera")
        return isPerspectiveCamera;
    if (property == "fov")
        return fov;
    if (property == "zoom")
        return zoom;
    if (property == "near")
        return near;
    if (property == "far")
        return far;
    if (property == "focus")
        return focus;
    if (property == "aspect")
        return aspect;
    if (property == "filmGauge")
        return filmGauge;
    if (property == "filmOffset")
        return filmOffset;
    return Camera::propertyValue(property);
}
PerspectiveCamera& PerspectiveCamera::copy(const Object3D& source, bool recursive) {
    const auto* camera = dynamic_cast<const PerspectiveCamera*>(&source);
    if (!camera)
        throw std::invalid_argument("PerspectiveCamera::copy requires a PerspectiveCamera");
    Camera::copy(source, recursive);
    fov = camera->fov;
    zoom = camera->zoom;
    near = camera->near;
    far = camera->far;
    focus = camera->focus;
    aspect = camera->aspect;
    view = camera->view;
    filmGauge = camera->filmGauge;
    filmOffset = camera->filmOffset;
    return *this;
}
std::shared_ptr<Object3D> PerspectiveCamera::clone(bool) const {
    auto result = std::make_shared<PerspectiveCamera>();
    result->copy(*this);
    return result;
}
void PerspectiveCamera::setFocalLength(HIGH_PRECISION focalLength) {
    const auto vExtentSlope = HIGH_PRECISION(0.5) * getFilmHeight() / focalLength;
    fov = 180 / MATH_PI * 2 * JSNumberMath::atan(vExtentSlope);
    updateProjectionMatrix();
}
HIGH_PRECISION PerspectiveCamera::getFocalLength() const {
    return HIGH_PRECISION(0.5) * getFilmHeight() /
           JSNumberMath::tan(MATH_PI / 180 * HIGH_PRECISION(0.5) * fov);
}
HIGH_PRECISION PerspectiveCamera::getEffectiveFOV() const {
    return 180 / MATH_PI * 2 *
           JSNumberMath::atan(JSNumberMath::tan(MATH_PI / 180 * HIGH_PRECISION(0.5) * fov) / zoom);
}
HIGH_PRECISION PerspectiveCamera::getFilmWidth() const {
    return filmGauge * std::min(aspect, HIGH_PRECISION(1));
}
HIGH_PRECISION PerspectiveCamera::getFilmHeight() const {
    return filmGauge / std::max(aspect, HIGH_PRECISION(1));
}
void PerspectiveCamera::getViewBounds(HIGH_PRECISION distance, Vector2& minTarget,
                                      Vector2& maxTarget) const {
    Vector3 v{-1, -1, HIGH_PRECISION(0.5)};
    v.applyMatrix4(projectionMatrixInverse);
    minTarget.set(v.x, v.y);
    minTarget.multiplyScalar(-distance / v.z);
    v.set(1, 1, HIGH_PRECISION(0.5));
    v.applyMatrix4(projectionMatrixInverse);
    maxTarget.set(v.x, v.y);
    maxTarget.multiplyScalar(-distance / v.z);
}
Vector2& PerspectiveCamera::getViewSize(HIGH_PRECISION distance, Vector2& target) const {
    Vector2 min, max;
    getViewBounds(distance, min, max);
    target.subVectors(max, min);
    return target;
}
void PerspectiveCamera::setViewOffset(HIGH_PRECISION fullWidth, HIGH_PRECISION fullHeight,
                                      HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION width,
                                      HIGH_PRECISION height) {
    aspect = fullWidth / fullHeight;
    view = CameraView{true, fullWidth, fullHeight, x, y, width, height};
    updateProjectionMatrix();
}
void PerspectiveCamera::clearViewOffset() {
    if (view)
        view->enabled = false;
    updateProjectionMatrix();
}
void PerspectiveCamera::updateProjectionMatrix() {
    auto top = near * JSNumberMath::tan(MATH_PI / 180 * HIGH_PRECISION(0.5) * fov) / zoom;
    auto height = 2 * top;
    auto width = aspect * height;
    auto left = HIGH_PRECISION(-0.5) * width;
    if (view && view->enabled) {
        left += view->offsetX * width / view->fullWidth;
        top -= view->offsetY * height / view->fullHeight;
        width *= view->width / view->fullWidth;
        height *= view->height / view->fullHeight;
    }
    if (filmOffset != 0)
        left += near * filmOffset / getFilmWidth();
    projectionMatrix.makePerspective(left, left + width, top, top - height, near, far,
                                     coordinateSystem, reversedDepth);
    projectionMatrixInverse.copy(projectionMatrix);
    projectionMatrixInverse.invert();
}
JsonValue PerspectiveCamera::toJSON(Object3DJSONContext* meta) const {
    auto output = Camera::toJSON(meta);
    auto& object = output["object"];
    object["fov"] = fov;
    object["zoom"] = zoom;
    object["near"] = near;
    object["far"] = far;
    object["focus"] = focus;
    object["aspect"] = aspect;
    if (view)
        object["view"] = view->toJSON();
    object["filmGauge"] = filmGauge;
    object["filmOffset"] = filmOffset;
    return output;
}
#include "math/JSNumberMath.h"

// Complete managed native port of pinned PerspectiveCamera.js. See THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSPromise.h"
#include "cameras/PerspectiveCameraRuntimeSource.inc"
#include "cameras/Camera.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include <string_view>
#include <unordered_set>
namespace PerspectiveCameraManaged{

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
JSValue fov=arg(nativeStatementTemp1,0);
if(fov.isUndefined())fov=JSValue(50);
JSValue aspect=arg(nativeStatementTemp1,1);
if(aspect.isUndefined())aspect=JSValue(1);
JSValue near=arg(nativeStatementTemp1,2);
if(near.isUndefined())near=JSValue(0.1);
JSValue far=arg(nativeStatementTemp1,3);
if(far.isUndefined())far=JSValue(2000);
([&]()->JSValue{const auto nativeStatementTemp9=nativeStatementTemp3.get("PerspectiveCamera").getPrototype();Args nativeStatementTemp8;nativeStatementTemp4=nativeStatementTemp9.construct(nativeStatementTemp8,nativeStatementTemp5);return nativeStatementTemp4;}());
([&]()->JSValue{auto nativeStatementTemp10=nativeStatementTemp4;const auto nativeStatementTemp11=JSValue("isPerspectiveCamera");const auto nativeStatementTemp12=JSValue(true);setReference(nativeStatementTemp10,nativeStatementTemp11,nativeStatementTemp12);return nativeStatementTemp12;}());
([&]()->JSValue{auto nativeStatementTemp15=nativeStatementTemp4;const auto nativeStatementTemp16=JSValue("type");const auto nativeStatementTemp17=JSValue("PerspectiveCamera");setReference(nativeStatementTemp15,nativeStatementTemp16,nativeStatementTemp17);return nativeStatementTemp17;}());
([&]()->JSValue{auto nativeStatementTemp20=nativeStatementTemp4;const auto nativeStatementTemp21=JSValue("fov");const auto nativeStatementTemp22=fov;setReference(nativeStatementTemp20,nativeStatementTemp21,nativeStatementTemp22);return nativeStatementTemp22;}());
([&]()->JSValue{auto nativeStatementTemp25=nativeStatementTemp4;const auto nativeStatementTemp26=JSValue("zoom");const auto nativeStatementTemp27=JSValue(1);setReference(nativeStatementTemp25,nativeStatementTemp26,nativeStatementTemp27);return nativeStatementTemp27;}());
([&]()->JSValue{auto nativeStatementTemp30=nativeStatementTemp4;const auto nativeStatementTemp31=JSValue("near");const auto nativeStatementTemp32=near;setReference(nativeStatementTemp30,nativeStatementTemp31,nativeStatementTemp32);return nativeStatementTemp32;}());
([&]()->JSValue{auto nativeStatementTemp35=nativeStatementTemp4;const auto nativeStatementTemp36=JSValue("far");const auto nativeStatementTemp37=far;setReference(nativeStatementTemp35,nativeStatementTemp36,nativeStatementTemp37);return nativeStatementTemp37;}());
([&]()->JSValue{auto nativeStatementTemp40=nativeStatementTemp4;const auto nativeStatementTemp41=JSValue("focus");const auto nativeStatementTemp42=JSValue(10);setReference(nativeStatementTemp40,nativeStatementTemp41,nativeStatementTemp42);return nativeStatementTemp42;}());
([&]()->JSValue{auto nativeStatementTemp45=nativeStatementTemp4;const auto nativeStatementTemp46=JSValue("aspect");const auto nativeStatementTemp47=aspect;setReference(nativeStatementTemp45,nativeStatementTemp46,nativeStatementTemp47);return nativeStatementTemp47;}());
([&]()->JSValue{auto nativeStatementTemp50=nativeStatementTemp4;const auto nativeStatementTemp51=JSValue("view");const auto nativeStatementTemp52=JSValue(nullptr);setReference(nativeStatementTemp50,nativeStatementTemp51,nativeStatementTemp52);return nativeStatementTemp52;}());
([&]()->JSValue{auto nativeStatementTemp55=nativeStatementTemp4;const auto nativeStatementTemp56=JSValue("filmGauge");const auto nativeStatementTemp57=JSValue(35);setReference(nativeStatementTemp55,nativeStatementTemp56,nativeStatementTemp57);return nativeStatementTemp57;}());
([&]()->JSValue{auto nativeStatementTemp60=nativeStatementTemp4;const auto nativeStatementTemp61=JSValue("filmOffset");const auto nativeStatementTemp62=JSValue(0);setReference(nativeStatementTemp60,nativeStatementTemp61,nativeStatementTemp62);return nativeStatementTemp62;}());
([&]()->JSValue{auto nativeStatementTemp68=nativeStatementTemp4;const auto nativeStatementTemp69=JSValue("updateProjectionMatrix");const auto nativeStatementTemp65=nativeStatementTemp68.get(nativeStatementTemp69);const auto nativeStatementTemp66=nativeStatementTemp68;Args nativeStatementTemp67;return nativeStatementTemp65.call(nativeStatementTemp66,nativeStatementTemp67);}());
return nativeStatementTemp4;
}

JSValue method1(const JSValue& nativeStatementTemp70,const Args& nativeStatementTemp71,const Args& nativeStatementTemp72){
auto nativeStatementTemp73=nativeStatementTemp72[0];auto nativeStatementTemp74=nativeStatementTemp70;
JSValue source=arg(nativeStatementTemp71,0);
JSValue recursive=arg(nativeStatementTemp71,1);
([&]()->JSValue{const auto nativeStatementTemp79=nativeStatementTemp73.get("_home").getPrototype();const auto nativeStatementTemp80=JSValue("copy");const auto nativeStatementTemp76=nativeStatementTemp79.get(nativeStatementTemp80,nativeStatementTemp74);const auto nativeStatementTemp77=nativeStatementTemp74;Args nativeStatementTemp78;nativeStatementTemp78.push_back(source);nativeStatementTemp78.push_back(recursive);return nativeStatementTemp76.call(nativeStatementTemp77,nativeStatementTemp78);}());
([&]()->JSValue{auto nativeStatementTemp81=nativeStatementTemp74;const auto nativeStatementTemp82=JSValue("fov");const auto nativeStatementTemp83=([&]()->JSValue{const auto nativeStatementTemp86=source;const auto nativeStatementTemp87=JSValue("fov");return nativeStatementTemp86.get(nativeStatementTemp87);}());setReference(nativeStatementTemp81,nativeStatementTemp82,nativeStatementTemp83);return nativeStatementTemp83;}());
([&]()->JSValue{auto nativeStatementTemp88=nativeStatementTemp74;const auto nativeStatementTemp89=JSValue("zoom");const auto nativeStatementTemp90=([&]()->JSValue{const auto nativeStatementTemp93=source;const auto nativeStatementTemp94=JSValue("zoom");return nativeStatementTemp93.get(nativeStatementTemp94);}());setReference(nativeStatementTemp88,nativeStatementTemp89,nativeStatementTemp90);return nativeStatementTemp90;}());
([&]()->JSValue{auto nativeStatementTemp95=nativeStatementTemp74;const auto nativeStatementTemp96=JSValue("near");const auto nativeStatementTemp97=([&]()->JSValue{const auto nativeStatementTemp100=source;const auto nativeStatementTemp101=JSValue("near");return nativeStatementTemp100.get(nativeStatementTemp101);}());setReference(nativeStatementTemp95,nativeStatementTemp96,nativeStatementTemp97);return nativeStatementTemp97;}());
([&]()->JSValue{auto nativeStatementTemp102=nativeStatementTemp74;const auto nativeStatementTemp103=JSValue("far");const auto nativeStatementTemp104=([&]()->JSValue{const auto nativeStatementTemp107=source;const auto nativeStatementTemp108=JSValue("far");return nativeStatementTemp107.get(nativeStatementTemp108);}());setReference(nativeStatementTemp102,nativeStatementTemp103,nativeStatementTemp104);return nativeStatementTemp104;}());
([&]()->JSValue{auto nativeStatementTemp109=nativeStatementTemp74;const auto nativeStatementTemp110=JSValue("focus");const auto nativeStatementTemp111=([&]()->JSValue{const auto nativeStatementTemp114=source;const auto nativeStatementTemp115=JSValue("focus");return nativeStatementTemp114.get(nativeStatementTemp115);}());setReference(nativeStatementTemp109,nativeStatementTemp110,nativeStatementTemp111);return nativeStatementTemp111;}());
([&]()->JSValue{auto nativeStatementTemp116=nativeStatementTemp74;const auto nativeStatementTemp117=JSValue("aspect");const auto nativeStatementTemp118=([&]()->JSValue{const auto nativeStatementTemp121=source;const auto nativeStatementTemp122=JSValue("aspect");return nativeStatementTemp121.get(nativeStatementTemp122);}());setReference(nativeStatementTemp116,nativeStatementTemp117,nativeStatementTemp118);return nativeStatementTemp118;}());
([&]()->JSValue{auto nativeStatementTemp123=nativeStatementTemp74;const auto nativeStatementTemp124=JSValue("view");const auto nativeStatementTemp125=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp128=([&]()->JSValue{const auto nativeStatementTemp130=source;const auto nativeStatementTemp131=JSValue("view");return nativeStatementTemp130.get(nativeStatementTemp131);}());const auto nativeStatementTemp129=JSValue(nullptr);return binary("===",nativeStatementTemp128,nativeStatementTemp129);}()).toBoolean())return JSValue(nullptr);return ([&]()->JSValue{auto nativeStatementTemp135=nativeGlobalBinding("Object");const auto nativeStatementTemp136=JSValue("assign");const auto nativeStatementTemp132=nativeStatementTemp135.get(nativeStatementTemp136);const auto nativeStatementTemp133=nativeStatementTemp135;Args nativeStatementTemp134;nativeStatementTemp134.push_back(([&]()->JSValue{auto nativeStatementTemp137=JSValue::object();return nativeStatementTemp137;}()));nativeStatementTemp134.push_back(([&]()->JSValue{const auto nativeStatementTemp138=source;const auto nativeStatementTemp139=JSValue("view");return nativeStatementTemp138.get(nativeStatementTemp139);}()));return nativeStatementTemp132.call(nativeStatementTemp133,nativeStatementTemp134);}());}());setReference(nativeStatementTemp123,nativeStatementTemp124,nativeStatementTemp125);return nativeStatementTemp125;}());
([&]()->JSValue{auto nativeStatementTemp140=nativeStatementTemp74;const auto nativeStatementTemp141=JSValue("filmGauge");const auto nativeStatementTemp142=([&]()->JSValue{const auto nativeStatementTemp145=source;const auto nativeStatementTemp146=JSValue("filmGauge");return nativeStatementTemp145.get(nativeStatementTemp146);}());setReference(nativeStatementTemp140,nativeStatementTemp141,nativeStatementTemp142);return nativeStatementTemp142;}());
([&]()->JSValue{auto nativeStatementTemp147=nativeStatementTemp74;const auto nativeStatementTemp148=JSValue("filmOffset");const auto nativeStatementTemp149=([&]()->JSValue{const auto nativeStatementTemp152=source;const auto nativeStatementTemp153=JSValue("filmOffset");return nativeStatementTemp152.get(nativeStatementTemp153);}());setReference(nativeStatementTemp147,nativeStatementTemp148,nativeStatementTemp149);return nativeStatementTemp149;}());
return nativeStatementTemp74;
return JSValue{};
}

JSValue method2(const JSValue& nativeStatementTemp154,const Args& nativeStatementTemp155,const Args& nativeStatementTemp156){
auto nativeStatementTemp157=nativeStatementTemp156[0];auto nativeStatementTemp158=nativeStatementTemp154;
JSValue focalLength=arg(nativeStatementTemp155,0);
JSValue vExtentSlope=([&]()->JSValue{const auto nativeStatementTemp160=([&]()->JSValue{const auto nativeStatementTemp162=JSValue(0.5);const auto nativeStatementTemp163=([&]()->JSValue{auto nativeStatementTemp167=nativeStatementTemp158;const auto nativeStatementTemp168=JSValue("getFilmHeight");const auto nativeStatementTemp164=nativeStatementTemp167.get(nativeStatementTemp168);const auto nativeStatementTemp165=nativeStatementTemp167;Args nativeStatementTemp166;return nativeStatementTemp164.call(nativeStatementTemp165,nativeStatementTemp166);}());return binary("*",nativeStatementTemp162,nativeStatementTemp163);}());const auto nativeStatementTemp161=focalLength;return binary("/",nativeStatementTemp160,nativeStatementTemp161);}());
([&]()->JSValue{auto nativeStatementTemp169=nativeStatementTemp158;const auto nativeStatementTemp170=JSValue("fov");const auto nativeStatementTemp171=([&]()->JSValue{const auto nativeStatementTemp174=([&]()->JSValue{const auto nativeStatementTemp176=nativeStatementTemp157.get("RAD2DEG");const auto nativeStatementTemp177=JSValue(2);return binary("*",nativeStatementTemp176,nativeStatementTemp177);}());const auto nativeStatementTemp175=([&]()->JSValue{auto nativeStatementTemp181=nativeGlobalBinding("Math");const auto nativeStatementTemp182=JSValue("atan");const auto nativeStatementTemp178=nativeStatementTemp181.get(nativeStatementTemp182);const auto nativeStatementTemp179=nativeStatementTemp181;Args nativeStatementTemp180;nativeStatementTemp180.push_back(vExtentSlope);return nativeStatementTemp178.call(nativeStatementTemp179,nativeStatementTemp180);}());return binary("*",nativeStatementTemp174,nativeStatementTemp175);}());setReference(nativeStatementTemp169,nativeStatementTemp170,nativeStatementTemp171);return nativeStatementTemp171;}());
([&]()->JSValue{auto nativeStatementTemp186=nativeStatementTemp158;const auto nativeStatementTemp187=JSValue("updateProjectionMatrix");const auto nativeStatementTemp183=nativeStatementTemp186.get(nativeStatementTemp187);const auto nativeStatementTemp184=nativeStatementTemp186;Args nativeStatementTemp185;return nativeStatementTemp183.call(nativeStatementTemp184,nativeStatementTemp185);}());
return JSValue{};
}

JSValue method3(const JSValue& nativeStatementTemp188,const Args& nativeStatementTemp189,const Args& nativeStatementTemp190){
auto nativeStatementTemp191=nativeStatementTemp190[0];auto nativeStatementTemp192=nativeStatementTemp188;
JSValue vExtentSlope=([&]()->JSValue{auto nativeStatementTemp197=nativeGlobalBinding("Math");const auto nativeStatementTemp198=JSValue("tan");const auto nativeStatementTemp194=nativeStatementTemp197.get(nativeStatementTemp198);const auto nativeStatementTemp195=nativeStatementTemp197;Args nativeStatementTemp196;nativeStatementTemp196.push_back(([&]()->JSValue{const auto nativeStatementTemp199=([&]()->JSValue{const auto nativeStatementTemp201=nativeStatementTemp191.get("DEG2RAD");const auto nativeStatementTemp202=JSValue(0.5);return binary("*",nativeStatementTemp201,nativeStatementTemp202);}());const auto nativeStatementTemp200=([&]()->JSValue{const auto nativeStatementTemp203=nativeStatementTemp192;const auto nativeStatementTemp204=JSValue("fov");return nativeStatementTemp203.get(nativeStatementTemp204);}());return binary("*",nativeStatementTemp199,nativeStatementTemp200);}()));return nativeStatementTemp194.call(nativeStatementTemp195,nativeStatementTemp196);}());
return ([&]()->JSValue{const auto nativeStatementTemp205=([&]()->JSValue{const auto nativeStatementTemp207=JSValue(0.5);const auto nativeStatementTemp208=([&]()->JSValue{auto nativeStatementTemp212=nativeStatementTemp192;const auto nativeStatementTemp213=JSValue("getFilmHeight");const auto nativeStatementTemp209=nativeStatementTemp212.get(nativeStatementTemp213);const auto nativeStatementTemp210=nativeStatementTemp212;Args nativeStatementTemp211;return nativeStatementTemp209.call(nativeStatementTemp210,nativeStatementTemp211);}());return binary("*",nativeStatementTemp207,nativeStatementTemp208);}());const auto nativeStatementTemp206=vExtentSlope;return binary("/",nativeStatementTemp205,nativeStatementTemp206);}());
return JSValue{};
}

JSValue method4(const JSValue& nativeStatementTemp214,const Args& nativeStatementTemp215,const Args& nativeStatementTemp216){
auto nativeStatementTemp217=nativeStatementTemp216[0];auto nativeStatementTemp218=nativeStatementTemp214;
return ([&]()->JSValue{const auto nativeStatementTemp220=([&]()->JSValue{const auto nativeStatementTemp222=nativeStatementTemp217.get("RAD2DEG");const auto nativeStatementTemp223=JSValue(2);return binary("*",nativeStatementTemp222,nativeStatementTemp223);}());const auto nativeStatementTemp221=([&]()->JSValue{auto nativeStatementTemp227=nativeGlobalBinding("Math");const auto nativeStatementTemp228=JSValue("atan");const auto nativeStatementTemp224=nativeStatementTemp227.get(nativeStatementTemp228);const auto nativeStatementTemp225=nativeStatementTemp227;Args nativeStatementTemp226;nativeStatementTemp226.push_back(([&]()->JSValue{const auto nativeStatementTemp229=([&]()->JSValue{auto nativeStatementTemp234=nativeGlobalBinding("Math");const auto nativeStatementTemp235=JSValue("tan");const auto nativeStatementTemp231=nativeStatementTemp234.get(nativeStatementTemp235);const auto nativeStatementTemp232=nativeStatementTemp234;Args nativeStatementTemp233;nativeStatementTemp233.push_back(([&]()->JSValue{const auto nativeStatementTemp236=([&]()->JSValue{const auto nativeStatementTemp238=nativeStatementTemp217.get("DEG2RAD");const auto nativeStatementTemp239=JSValue(0.5);return binary("*",nativeStatementTemp238,nativeStatementTemp239);}());const auto nativeStatementTemp237=([&]()->JSValue{const auto nativeStatementTemp240=nativeStatementTemp218;const auto nativeStatementTemp241=JSValue("fov");return nativeStatementTemp240.get(nativeStatementTemp241);}());return binary("*",nativeStatementTemp236,nativeStatementTemp237);}()));return nativeStatementTemp231.call(nativeStatementTemp232,nativeStatementTemp233);}());const auto nativeStatementTemp230=([&]()->JSValue{const auto nativeStatementTemp242=nativeStatementTemp218;const auto nativeStatementTemp243=JSValue("zoom");return nativeStatementTemp242.get(nativeStatementTemp243);}());return binary("/",nativeStatementTemp229,nativeStatementTemp230);}()));return nativeStatementTemp224.call(nativeStatementTemp225,nativeStatementTemp226);}());return binary("*",nativeStatementTemp220,nativeStatementTemp221);}());
return JSValue{};
}

JSValue method5(const JSValue& nativeStatementTemp244,const Args& nativeStatementTemp245,const Args& nativeStatementTemp246){
auto nativeStatementTemp247=nativeStatementTemp246[0];auto nativeStatementTemp248=nativeStatementTemp244;
return ([&]()->JSValue{const auto nativeStatementTemp250=([&]()->JSValue{const auto nativeStatementTemp252=nativeStatementTemp248;const auto nativeStatementTemp253=JSValue("filmGauge");return nativeStatementTemp252.get(nativeStatementTemp253);}());const auto nativeStatementTemp251=([&]()->JSValue{auto nativeStatementTemp257=nativeGlobalBinding("Math");const auto nativeStatementTemp258=JSValue("min");const auto nativeStatementTemp254=nativeStatementTemp257.get(nativeStatementTemp258);const auto nativeStatementTemp255=nativeStatementTemp257;Args nativeStatementTemp256;nativeStatementTemp256.push_back(([&]()->JSValue{const auto nativeStatementTemp259=nativeStatementTemp248;const auto nativeStatementTemp260=JSValue("aspect");return nativeStatementTemp259.get(nativeStatementTemp260);}()));nativeStatementTemp256.push_back(JSValue(1));return nativeStatementTemp254.call(nativeStatementTemp255,nativeStatementTemp256);}());return binary("*",nativeStatementTemp250,nativeStatementTemp251);}());
return JSValue{};
}

JSValue method6(const JSValue& nativeStatementTemp261,const Args& nativeStatementTemp262,const Args& nativeStatementTemp263){
auto nativeStatementTemp264=nativeStatementTemp263[0];auto nativeStatementTemp265=nativeStatementTemp261;
return ([&]()->JSValue{const auto nativeStatementTemp267=([&]()->JSValue{const auto nativeStatementTemp269=nativeStatementTemp265;const auto nativeStatementTemp270=JSValue("filmGauge");return nativeStatementTemp269.get(nativeStatementTemp270);}());const auto nativeStatementTemp268=([&]()->JSValue{auto nativeStatementTemp274=nativeGlobalBinding("Math");const auto nativeStatementTemp275=JSValue("max");const auto nativeStatementTemp271=nativeStatementTemp274.get(nativeStatementTemp275);const auto nativeStatementTemp272=nativeStatementTemp274;Args nativeStatementTemp273;nativeStatementTemp273.push_back(([&]()->JSValue{const auto nativeStatementTemp276=nativeStatementTemp265;const auto nativeStatementTemp277=JSValue("aspect");return nativeStatementTemp276.get(nativeStatementTemp277);}()));nativeStatementTemp273.push_back(JSValue(1));return nativeStatementTemp271.call(nativeStatementTemp272,nativeStatementTemp273);}());return binary("/",nativeStatementTemp267,nativeStatementTemp268);}());
return JSValue{};
}

JSValue method7(const JSValue& nativeStatementTemp278,const Args& nativeStatementTemp279,const Args& nativeStatementTemp280){
auto nativeStatementTemp281=nativeStatementTemp280[0];auto nativeStatementTemp282=nativeStatementTemp278;
JSValue distance=arg(nativeStatementTemp279,0);
JSValue minTarget=arg(nativeStatementTemp279,1);
JSValue maxTarget=arg(nativeStatementTemp279,2);
([&]()->JSValue{auto nativeStatementTemp287=([&]()->JSValue{auto nativeStatementTemp292=nativeStatementTemp281.get("_v3");const auto nativeStatementTemp293=JSValue("set");const auto nativeStatementTemp289=nativeStatementTemp292.get(nativeStatementTemp293);const auto nativeStatementTemp290=nativeStatementTemp292;Args nativeStatementTemp291;nativeStatementTemp291.push_back(([&]()->JSValue{const auto nativeStatementTemp294=JSValue(1);return -nativeStatementTemp294.toNumber();}()));nativeStatementTemp291.push_back(([&]()->JSValue{const auto nativeStatementTemp295=JSValue(1);return -nativeStatementTemp295.toNumber();}()));nativeStatementTemp291.push_back(JSValue(0.5));return nativeStatementTemp289.call(nativeStatementTemp290,nativeStatementTemp291);}());const auto nativeStatementTemp288=JSValue("applyMatrix4");const auto nativeStatementTemp284=nativeStatementTemp287.get(nativeStatementTemp288);const auto nativeStatementTemp285=nativeStatementTemp287;Args nativeStatementTemp286;nativeStatementTemp286.push_back(([&]()->JSValue{const auto nativeStatementTemp296=nativeStatementTemp282;const auto nativeStatementTemp297=JSValue("projectionMatrixInverse");return nativeStatementTemp296.get(nativeStatementTemp297);}()));return nativeStatementTemp284.call(nativeStatementTemp285,nativeStatementTemp286);}());
([&]()->JSValue{auto nativeStatementTemp301=([&]()->JSValue{auto nativeStatementTemp306=minTarget;const auto nativeStatementTemp307=JSValue("set");const auto nativeStatementTemp303=nativeStatementTemp306.get(nativeStatementTemp307);const auto nativeStatementTemp304=nativeStatementTemp306;Args nativeStatementTemp305;nativeStatementTemp305.push_back(([&]()->JSValue{const auto nativeStatementTemp308=nativeStatementTemp281.get("_v3");const auto nativeStatementTemp309=JSValue("x");return nativeStatementTemp308.get(nativeStatementTemp309);}()));nativeStatementTemp305.push_back(([&]()->JSValue{const auto nativeStatementTemp310=nativeStatementTemp281.get("_v3");const auto nativeStatementTemp311=JSValue("y");return nativeStatementTemp310.get(nativeStatementTemp311);}()));return nativeStatementTemp303.call(nativeStatementTemp304,nativeStatementTemp305);}());const auto nativeStatementTemp302=JSValue("multiplyScalar");const auto nativeStatementTemp298=nativeStatementTemp301.get(nativeStatementTemp302);const auto nativeStatementTemp299=nativeStatementTemp301;Args nativeStatementTemp300;nativeStatementTemp300.push_back(([&]()->JSValue{const auto nativeStatementTemp312=([&]()->JSValue{const auto nativeStatementTemp314=distance;return -nativeStatementTemp314.toNumber();}());const auto nativeStatementTemp313=([&]()->JSValue{const auto nativeStatementTemp315=nativeStatementTemp281.get("_v3");const auto nativeStatementTemp316=JSValue("z");return nativeStatementTemp315.get(nativeStatementTemp316);}());return binary("/",nativeStatementTemp312,nativeStatementTemp313);}()));return nativeStatementTemp298.call(nativeStatementTemp299,nativeStatementTemp300);}());
([&]()->JSValue{auto nativeStatementTemp320=([&]()->JSValue{auto nativeStatementTemp325=nativeStatementTemp281.get("_v3");const auto nativeStatementTemp326=JSValue("set");const auto nativeStatementTemp322=nativeStatementTemp325.get(nativeStatementTemp326);const auto nativeStatementTemp323=nativeStatementTemp325;Args nativeStatementTemp324;nativeStatementTemp324.push_back(JSValue(1));nativeStatementTemp324.push_back(JSValue(1));nativeStatementTemp324.push_back(JSValue(0.5));return nativeStatementTemp322.call(nativeStatementTemp323,nativeStatementTemp324);}());const auto nativeStatementTemp321=JSValue("applyMatrix4");const auto nativeStatementTemp317=nativeStatementTemp320.get(nativeStatementTemp321);const auto nativeStatementTemp318=nativeStatementTemp320;Args nativeStatementTemp319;nativeStatementTemp319.push_back(([&]()->JSValue{const auto nativeStatementTemp327=nativeStatementTemp282;const auto nativeStatementTemp328=JSValue("projectionMatrixInverse");return nativeStatementTemp327.get(nativeStatementTemp328);}()));return nativeStatementTemp317.call(nativeStatementTemp318,nativeStatementTemp319);}());
([&]()->JSValue{auto nativeStatementTemp332=([&]()->JSValue{auto nativeStatementTemp337=maxTarget;const auto nativeStatementTemp338=JSValue("set");const auto nativeStatementTemp334=nativeStatementTemp337.get(nativeStatementTemp338);const auto nativeStatementTemp335=nativeStatementTemp337;Args nativeStatementTemp336;nativeStatementTemp336.push_back(([&]()->JSValue{const auto nativeStatementTemp339=nativeStatementTemp281.get("_v3");const auto nativeStatementTemp340=JSValue("x");return nativeStatementTemp339.get(nativeStatementTemp340);}()));nativeStatementTemp336.push_back(([&]()->JSValue{const auto nativeStatementTemp341=nativeStatementTemp281.get("_v3");const auto nativeStatementTemp342=JSValue("y");return nativeStatementTemp341.get(nativeStatementTemp342);}()));return nativeStatementTemp334.call(nativeStatementTemp335,nativeStatementTemp336);}());const auto nativeStatementTemp333=JSValue("multiplyScalar");const auto nativeStatementTemp329=nativeStatementTemp332.get(nativeStatementTemp333);const auto nativeStatementTemp330=nativeStatementTemp332;Args nativeStatementTemp331;nativeStatementTemp331.push_back(([&]()->JSValue{const auto nativeStatementTemp343=([&]()->JSValue{const auto nativeStatementTemp345=distance;return -nativeStatementTemp345.toNumber();}());const auto nativeStatementTemp344=([&]()->JSValue{const auto nativeStatementTemp346=nativeStatementTemp281.get("_v3");const auto nativeStatementTemp347=JSValue("z");return nativeStatementTemp346.get(nativeStatementTemp347);}());return binary("/",nativeStatementTemp343,nativeStatementTemp344);}()));return nativeStatementTemp329.call(nativeStatementTemp330,nativeStatementTemp331);}());
return JSValue{};
}

JSValue method8(const JSValue& nativeStatementTemp348,const Args& nativeStatementTemp349,const Args& nativeStatementTemp350){
auto nativeStatementTemp351=nativeStatementTemp350[0];auto nativeStatementTemp352=nativeStatementTemp348;
JSValue distance=arg(nativeStatementTemp349,0);
JSValue target=arg(nativeStatementTemp349,1);
([&]()->JSValue{auto nativeStatementTemp357=nativeStatementTemp352;const auto nativeStatementTemp358=JSValue("getViewBounds");const auto nativeStatementTemp354=nativeStatementTemp357.get(nativeStatementTemp358);const auto nativeStatementTemp355=nativeStatementTemp357;Args nativeStatementTemp356;nativeStatementTemp356.push_back(distance);nativeStatementTemp356.push_back(nativeStatementTemp351.get("_minTarget"));nativeStatementTemp356.push_back(nativeStatementTemp351.get("_maxTarget"));return nativeStatementTemp354.call(nativeStatementTemp355,nativeStatementTemp356);}());
return ([&]()->JSValue{auto nativeStatementTemp362=target;const auto nativeStatementTemp363=JSValue("subVectors");const auto nativeStatementTemp359=nativeStatementTemp362.get(nativeStatementTemp363);const auto nativeStatementTemp360=nativeStatementTemp362;Args nativeStatementTemp361;nativeStatementTemp361.push_back(nativeStatementTemp351.get("_maxTarget"));nativeStatementTemp361.push_back(nativeStatementTemp351.get("_minTarget"));return nativeStatementTemp359.call(nativeStatementTemp360,nativeStatementTemp361);}());
return JSValue{};
}

JSValue method9(const JSValue& nativeStatementTemp364,const Args& nativeStatementTemp365,const Args& nativeStatementTemp366){
auto nativeStatementTemp367=nativeStatementTemp366[0];auto nativeStatementTemp368=nativeStatementTemp364;
JSValue fullWidth=arg(nativeStatementTemp365,0);
JSValue fullHeight=arg(nativeStatementTemp365,1);
JSValue x=arg(nativeStatementTemp365,2);
JSValue y=arg(nativeStatementTemp365,3);
JSValue width=arg(nativeStatementTemp365,4);
JSValue height=arg(nativeStatementTemp365,5);
([&]()->JSValue{auto nativeStatementTemp370=nativeStatementTemp368;const auto nativeStatementTemp371=JSValue("aspect");const auto nativeStatementTemp372=([&]()->JSValue{const auto nativeStatementTemp375=fullWidth;const auto nativeStatementTemp376=fullHeight;return binary("/",nativeStatementTemp375,nativeStatementTemp376);}());setReference(nativeStatementTemp370,nativeStatementTemp371,nativeStatementTemp372);return nativeStatementTemp372;}());
if(([&]()->JSValue{const auto nativeStatementTemp377=([&]()->JSValue{const auto nativeStatementTemp379=nativeStatementTemp368;const auto nativeStatementTemp380=JSValue("view");return nativeStatementTemp379.get(nativeStatementTemp380);}());const auto nativeStatementTemp378=JSValue(nullptr);return binary("===",nativeStatementTemp377,nativeStatementTemp378);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp381=nativeStatementTemp368;const auto nativeStatementTemp382=JSValue("view");const auto nativeStatementTemp383=([&]()->JSValue{auto nativeStatementTemp386=JSValue::object();{const auto nativeStatementTemp387=JSValue("enabled");const auto nativeStatementTemp388=JSValue(true);own(nativeStatementTemp386,nativeStatementTemp387,nativeStatementTemp388);}{const auto nativeStatementTemp389=JSValue("fullWidth");const auto nativeStatementTemp390=JSValue(1);own(nativeStatementTemp386,nativeStatementTemp389,nativeStatementTemp390);}{const auto nativeStatementTemp391=JSValue("fullHeight");const auto nativeStatementTemp392=JSValue(1);own(nativeStatementTemp386,nativeStatementTemp391,nativeStatementTemp392);}{const auto nativeStatementTemp393=JSValue("offsetX");const auto nativeStatementTemp394=JSValue(0);own(nativeStatementTemp386,nativeStatementTemp393,nativeStatementTemp394);}{const auto nativeStatementTemp395=JSValue("offsetY");const auto nativeStatementTemp396=JSValue(0);own(nativeStatementTemp386,nativeStatementTemp395,nativeStatementTemp396);}{const auto nativeStatementTemp397=JSValue("width");const auto nativeStatementTemp398=JSValue(1);own(nativeStatementTemp386,nativeStatementTemp397,nativeStatementTemp398);}{const auto nativeStatementTemp399=JSValue("height");const auto nativeStatementTemp400=JSValue(1);own(nativeStatementTemp386,nativeStatementTemp399,nativeStatementTemp400);}return nativeStatementTemp386;}());setReference(nativeStatementTemp381,nativeStatementTemp382,nativeStatementTemp383);return nativeStatementTemp383;}());

}
([&]()->JSValue{auto nativeStatementTemp401=([&]()->JSValue{const auto nativeStatementTemp403=nativeStatementTemp368;const auto nativeStatementTemp404=JSValue("view");return nativeStatementTemp403.get(nativeStatementTemp404);}());const auto nativeStatementTemp402=JSValue("enabled");const auto nativeStatementTemp405=JSValue(true);setReference(nativeStatementTemp401,nativeStatementTemp402,nativeStatementTemp405);return nativeStatementTemp405;}());
([&]()->JSValue{auto nativeStatementTemp408=([&]()->JSValue{const auto nativeStatementTemp410=nativeStatementTemp368;const auto nativeStatementTemp411=JSValue("view");return nativeStatementTemp410.get(nativeStatementTemp411);}());const auto nativeStatementTemp409=JSValue("fullWidth");const auto nativeStatementTemp412=fullWidth;setReference(nativeStatementTemp408,nativeStatementTemp409,nativeStatementTemp412);return nativeStatementTemp412;}());
([&]()->JSValue{auto nativeStatementTemp415=([&]()->JSValue{const auto nativeStatementTemp417=nativeStatementTemp368;const auto nativeStatementTemp418=JSValue("view");return nativeStatementTemp417.get(nativeStatementTemp418);}());const auto nativeStatementTemp416=JSValue("fullHeight");const auto nativeStatementTemp419=fullHeight;setReference(nativeStatementTemp415,nativeStatementTemp416,nativeStatementTemp419);return nativeStatementTemp419;}());
([&]()->JSValue{auto nativeStatementTemp422=([&]()->JSValue{const auto nativeStatementTemp424=nativeStatementTemp368;const auto nativeStatementTemp425=JSValue("view");return nativeStatementTemp424.get(nativeStatementTemp425);}());const auto nativeStatementTemp423=JSValue("offsetX");const auto nativeStatementTemp426=x;setReference(nativeStatementTemp422,nativeStatementTemp423,nativeStatementTemp426);return nativeStatementTemp426;}());
([&]()->JSValue{auto nativeStatementTemp429=([&]()->JSValue{const auto nativeStatementTemp431=nativeStatementTemp368;const auto nativeStatementTemp432=JSValue("view");return nativeStatementTemp431.get(nativeStatementTemp432);}());const auto nativeStatementTemp430=JSValue("offsetY");const auto nativeStatementTemp433=y;setReference(nativeStatementTemp429,nativeStatementTemp430,nativeStatementTemp433);return nativeStatementTemp433;}());
([&]()->JSValue{auto nativeStatementTemp436=([&]()->JSValue{const auto nativeStatementTemp438=nativeStatementTemp368;const auto nativeStatementTemp439=JSValue("view");return nativeStatementTemp438.get(nativeStatementTemp439);}());const auto nativeStatementTemp437=JSValue("width");const auto nativeStatementTemp440=width;setReference(nativeStatementTemp436,nativeStatementTemp437,nativeStatementTemp440);return nativeStatementTemp440;}());
([&]()->JSValue{auto nativeStatementTemp443=([&]()->JSValue{const auto nativeStatementTemp445=nativeStatementTemp368;const auto nativeStatementTemp446=JSValue("view");return nativeStatementTemp445.get(nativeStatementTemp446);}());const auto nativeStatementTemp444=JSValue("height");const auto nativeStatementTemp447=height;setReference(nativeStatementTemp443,nativeStatementTemp444,nativeStatementTemp447);return nativeStatementTemp447;}());
([&]()->JSValue{auto nativeStatementTemp453=nativeStatementTemp368;const auto nativeStatementTemp454=JSValue("updateProjectionMatrix");const auto nativeStatementTemp450=nativeStatementTemp453.get(nativeStatementTemp454);const auto nativeStatementTemp451=nativeStatementTemp453;Args nativeStatementTemp452;return nativeStatementTemp450.call(nativeStatementTemp451,nativeStatementTemp452);}());
return JSValue{};
}

JSValue method10(const JSValue& nativeStatementTemp455,const Args& nativeStatementTemp456,const Args& nativeStatementTemp457){
auto nativeStatementTemp458=nativeStatementTemp457[0];auto nativeStatementTemp459=nativeStatementTemp455;
if(([&]()->JSValue{const auto nativeStatementTemp461=([&]()->JSValue{const auto nativeStatementTemp463=nativeStatementTemp459;const auto nativeStatementTemp464=JSValue("view");return nativeStatementTemp463.get(nativeStatementTemp464);}());const auto nativeStatementTemp462=JSValue(nullptr);return binary("!==",nativeStatementTemp461,nativeStatementTemp462);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp465=([&]()->JSValue{const auto nativeStatementTemp467=nativeStatementTemp459;const auto nativeStatementTemp468=JSValue("view");return nativeStatementTemp467.get(nativeStatementTemp468);}());const auto nativeStatementTemp466=JSValue("enabled");const auto nativeStatementTemp469=JSValue(false);setReference(nativeStatementTemp465,nativeStatementTemp466,nativeStatementTemp469);return nativeStatementTemp469;}());

}
([&]()->JSValue{auto nativeStatementTemp475=nativeStatementTemp459;const auto nativeStatementTemp476=JSValue("updateProjectionMatrix");const auto nativeStatementTemp472=nativeStatementTemp475.get(nativeStatementTemp476);const auto nativeStatementTemp473=nativeStatementTemp475;Args nativeStatementTemp474;return nativeStatementTemp472.call(nativeStatementTemp473,nativeStatementTemp474);}());
return JSValue{};
}

JSValue method11(const JSValue& nativeStatementTemp477,const Args& nativeStatementTemp478,const Args& nativeStatementTemp479){
auto nativeStatementTemp480=nativeStatementTemp479[0];auto nativeStatementTemp481=nativeStatementTemp477;
JSValue near=([&]()->JSValue{const auto nativeStatementTemp483=nativeStatementTemp481;const auto nativeStatementTemp484=JSValue("near");return nativeStatementTemp483.get(nativeStatementTemp484);}());
JSValue top=([&]()->JSValue{const auto nativeStatementTemp485=([&]()->JSValue{const auto nativeStatementTemp487=near;const auto nativeStatementTemp488=([&]()->JSValue{auto nativeStatementTemp492=nativeGlobalBinding("Math");const auto nativeStatementTemp493=JSValue("tan");const auto nativeStatementTemp489=nativeStatementTemp492.get(nativeStatementTemp493);const auto nativeStatementTemp490=nativeStatementTemp492;Args nativeStatementTemp491;nativeStatementTemp491.push_back(([&]()->JSValue{const auto nativeStatementTemp494=([&]()->JSValue{const auto nativeStatementTemp496=nativeStatementTemp480.get("DEG2RAD");const auto nativeStatementTemp497=JSValue(0.5);return binary("*",nativeStatementTemp496,nativeStatementTemp497);}());const auto nativeStatementTemp495=([&]()->JSValue{const auto nativeStatementTemp498=nativeStatementTemp481;const auto nativeStatementTemp499=JSValue("fov");return nativeStatementTemp498.get(nativeStatementTemp499);}());return binary("*",nativeStatementTemp494,nativeStatementTemp495);}()));return nativeStatementTemp489.call(nativeStatementTemp490,nativeStatementTemp491);}());return binary("*",nativeStatementTemp487,nativeStatementTemp488);}());const auto nativeStatementTemp486=([&]()->JSValue{const auto nativeStatementTemp500=nativeStatementTemp481;const auto nativeStatementTemp501=JSValue("zoom");return nativeStatementTemp500.get(nativeStatementTemp501);}());return binary("/",nativeStatementTemp485,nativeStatementTemp486);}());
JSValue height=([&]()->JSValue{const auto nativeStatementTemp502=JSValue(2);const auto nativeStatementTemp503=top;return binary("*",nativeStatementTemp502,nativeStatementTemp503);}());
JSValue width=([&]()->JSValue{const auto nativeStatementTemp504=([&]()->JSValue{const auto nativeStatementTemp506=nativeStatementTemp481;const auto nativeStatementTemp507=JSValue("aspect");return nativeStatementTemp506.get(nativeStatementTemp507);}());const auto nativeStatementTemp505=height;return binary("*",nativeStatementTemp504,nativeStatementTemp505);}());
JSValue left=([&]()->JSValue{const auto nativeStatementTemp508=([&]()->JSValue{const auto nativeStatementTemp510=JSValue(0.5);return -nativeStatementTemp510.toNumber();}());const auto nativeStatementTemp509=width;return binary("*",nativeStatementTemp508,nativeStatementTemp509);}());
JSValue view=([&]()->JSValue{const auto nativeStatementTemp511=nativeStatementTemp481;const auto nativeStatementTemp512=JSValue("view");return nativeStatementTemp511.get(nativeStatementTemp512);}());
if(([&]()->JSValue{const auto nativeStatementTemp513=([&]()->JSValue{const auto nativeStatementTemp514=([&]()->JSValue{const auto nativeStatementTemp516=nativeStatementTemp481;const auto nativeStatementTemp517=JSValue("view");return nativeStatementTemp516.get(nativeStatementTemp517);}());const auto nativeStatementTemp515=JSValue(nullptr);return binary("!==",nativeStatementTemp514,nativeStatementTemp515);}());if(!nativeStatementTemp513.toBoolean())return nativeStatementTemp513;return ([&]()->JSValue{const auto nativeStatementTemp518=([&]()->JSValue{const auto nativeStatementTemp520=nativeStatementTemp481;const auto nativeStatementTemp521=JSValue("view");return nativeStatementTemp520.get(nativeStatementTemp521);}());const auto nativeStatementTemp519=JSValue("enabled");return nativeStatementTemp518.get(nativeStatementTemp519);}());}()).toBoolean()){
JSValue fullWidth=([&]()->JSValue{const auto nativeStatementTemp522=view;const auto nativeStatementTemp523=JSValue("fullWidth");return nativeStatementTemp522.get(nativeStatementTemp523);}());
JSValue fullHeight=([&]()->JSValue{const auto nativeStatementTemp524=view;const auto nativeStatementTemp525=JSValue("fullHeight");return nativeStatementTemp524.get(nativeStatementTemp525);}());
([&]()->JSValue{const auto nativeStatementTemp527=left;const auto nativeStatementTemp528=([&]()->JSValue{const auto nativeStatementTemp529=([&]()->JSValue{const auto nativeStatementTemp531=([&]()->JSValue{const auto nativeStatementTemp533=view;const auto nativeStatementTemp534=JSValue("offsetX");return nativeStatementTemp533.get(nativeStatementTemp534);}());const auto nativeStatementTemp532=width;return binary("*",nativeStatementTemp531,nativeStatementTemp532);}());const auto nativeStatementTemp530=fullWidth;return binary("/",nativeStatementTemp529,nativeStatementTemp530);}());const auto nativeStatementTemp526=binary("+",nativeStatementTemp527,nativeStatementTemp528);left=nativeStatementTemp526;return nativeStatementTemp526;}());
([&]()->JSValue{const auto nativeStatementTemp536=top;const auto nativeStatementTemp537=([&]()->JSValue{const auto nativeStatementTemp538=([&]()->JSValue{const auto nativeStatementTemp540=([&]()->JSValue{const auto nativeStatementTemp542=view;const auto nativeStatementTemp543=JSValue("offsetY");return nativeStatementTemp542.get(nativeStatementTemp543);}());const auto nativeStatementTemp541=height;return binary("*",nativeStatementTemp540,nativeStatementTemp541);}());const auto nativeStatementTemp539=fullHeight;return binary("/",nativeStatementTemp538,nativeStatementTemp539);}());const auto nativeStatementTemp535=binary("-",nativeStatementTemp536,nativeStatementTemp537);top=nativeStatementTemp535;return nativeStatementTemp535;}());
([&]()->JSValue{const auto nativeStatementTemp545=width;const auto nativeStatementTemp546=([&]()->JSValue{const auto nativeStatementTemp547=([&]()->JSValue{const auto nativeStatementTemp549=view;const auto nativeStatementTemp550=JSValue("width");return nativeStatementTemp549.get(nativeStatementTemp550);}());const auto nativeStatementTemp548=fullWidth;return binary("/",nativeStatementTemp547,nativeStatementTemp548);}());const auto nativeStatementTemp544=binary("*",nativeStatementTemp545,nativeStatementTemp546);width=nativeStatementTemp544;return nativeStatementTemp544;}());
([&]()->JSValue{const auto nativeStatementTemp552=height;const auto nativeStatementTemp553=([&]()->JSValue{const auto nativeStatementTemp554=([&]()->JSValue{const auto nativeStatementTemp556=view;const auto nativeStatementTemp557=JSValue("height");return nativeStatementTemp556.get(nativeStatementTemp557);}());const auto nativeStatementTemp555=fullHeight;return binary("/",nativeStatementTemp554,nativeStatementTemp555);}());const auto nativeStatementTemp551=binary("*",nativeStatementTemp552,nativeStatementTemp553);height=nativeStatementTemp551;return nativeStatementTemp551;}());

}
JSValue skew=([&]()->JSValue{const auto nativeStatementTemp558=nativeStatementTemp481;const auto nativeStatementTemp559=JSValue("filmOffset");return nativeStatementTemp558.get(nativeStatementTemp559);}());
if(([&]()->JSValue{const auto nativeStatementTemp560=skew;const auto nativeStatementTemp561=JSValue(0);return binary("!==",nativeStatementTemp560,nativeStatementTemp561);}()).toBoolean())([&]()->JSValue{const auto nativeStatementTemp563=left;const auto nativeStatementTemp564=([&]()->JSValue{const auto nativeStatementTemp565=([&]()->JSValue{const auto nativeStatementTemp567=near;const auto nativeStatementTemp568=skew;return binary("*",nativeStatementTemp567,nativeStatementTemp568);}());const auto nativeStatementTemp566=([&]()->JSValue{auto nativeStatementTemp572=nativeStatementTemp481;const auto nativeStatementTemp573=JSValue("getFilmWidth");const auto nativeStatementTemp569=nativeStatementTemp572.get(nativeStatementTemp573);const auto nativeStatementTemp570=nativeStatementTemp572;Args nativeStatementTemp571;return nativeStatementTemp569.call(nativeStatementTemp570,nativeStatementTemp571);}());return binary("/",nativeStatementTemp565,nativeStatementTemp566);}());const auto nativeStatementTemp562=binary("+",nativeStatementTemp563,nativeStatementTemp564);left=nativeStatementTemp562;return nativeStatementTemp562;}());
([&]()->JSValue{auto nativeStatementTemp577=([&]()->JSValue{const auto nativeStatementTemp579=nativeStatementTemp481;const auto nativeStatementTemp580=JSValue("projectionMatrix");return nativeStatementTemp579.get(nativeStatementTemp580);}());const auto nativeStatementTemp578=JSValue("makePerspective");const auto nativeStatementTemp574=nativeStatementTemp577.get(nativeStatementTemp578);const auto nativeStatementTemp575=nativeStatementTemp577;Args nativeStatementTemp576;nativeStatementTemp576.push_back(left);nativeStatementTemp576.push_back(([&]()->JSValue{const auto nativeStatementTemp581=left;const auto nativeStatementTemp582=width;return binary("+",nativeStatementTemp581,nativeStatementTemp582);}()));nativeStatementTemp576.push_back(top);nativeStatementTemp576.push_back(([&]()->JSValue{const auto nativeStatementTemp583=top;const auto nativeStatementTemp584=height;return binary("-",nativeStatementTemp583,nativeStatementTemp584);}()));nativeStatementTemp576.push_back(near);nativeStatementTemp576.push_back(([&]()->JSValue{const auto nativeStatementTemp585=nativeStatementTemp481;const auto nativeStatementTemp586=JSValue("far");return nativeStatementTemp585.get(nativeStatementTemp586);}()));nativeStatementTemp576.push_back(([&]()->JSValue{const auto nativeStatementTemp587=nativeStatementTemp481;const auto nativeStatementTemp588=JSValue("coordinateSystem");return nativeStatementTemp587.get(nativeStatementTemp588);}()));nativeStatementTemp576.push_back(([&]()->JSValue{const auto nativeStatementTemp589=nativeStatementTemp481;const auto nativeStatementTemp590=JSValue("reversedDepth");return nativeStatementTemp589.get(nativeStatementTemp590);}()));return nativeStatementTemp574.call(nativeStatementTemp575,nativeStatementTemp576);}());
([&]()->JSValue{auto nativeStatementTemp594=([&]()->JSValue{auto nativeStatementTemp599=([&]()->JSValue{const auto nativeStatementTemp601=nativeStatementTemp481;const auto nativeStatementTemp602=JSValue("projectionMatrixInverse");return nativeStatementTemp601.get(nativeStatementTemp602);}());const auto nativeStatementTemp600=JSValue("copy");const auto nativeStatementTemp596=nativeStatementTemp599.get(nativeStatementTemp600);const auto nativeStatementTemp597=nativeStatementTemp599;Args nativeStatementTemp598;nativeStatementTemp598.push_back(([&]()->JSValue{const auto nativeStatementTemp603=nativeStatementTemp481;const auto nativeStatementTemp604=JSValue("projectionMatrix");return nativeStatementTemp603.get(nativeStatementTemp604);}()));return nativeStatementTemp596.call(nativeStatementTemp597,nativeStatementTemp598);}());const auto nativeStatementTemp595=JSValue("invert");const auto nativeStatementTemp591=nativeStatementTemp594.get(nativeStatementTemp595);const auto nativeStatementTemp592=nativeStatementTemp594;Args nativeStatementTemp593;return nativeStatementTemp591.call(nativeStatementTemp592,nativeStatementTemp593);}());
return JSValue{};
}

JSValue method12(const JSValue& nativeStatementTemp605,const Args& nativeStatementTemp606,const Args& nativeStatementTemp607){
auto nativeStatementTemp608=nativeStatementTemp607[0];auto nativeStatementTemp609=nativeStatementTemp605;
JSValue meta=arg(nativeStatementTemp606,0);
JSValue data=([&]()->JSValue{const auto nativeStatementTemp614=nativeStatementTemp608.get("_home").getPrototype();const auto nativeStatementTemp615=JSValue("toJSON");const auto nativeStatementTemp611=nativeStatementTemp614.get(nativeStatementTemp615,nativeStatementTemp609);const auto nativeStatementTemp612=nativeStatementTemp609;Args nativeStatementTemp613;nativeStatementTemp613.push_back(meta);return nativeStatementTemp611.call(nativeStatementTemp612,nativeStatementTemp613);}());
([&]()->JSValue{auto nativeStatementTemp616=([&]()->JSValue{const auto nativeStatementTemp618=data;const auto nativeStatementTemp619=JSValue("object");return nativeStatementTemp618.get(nativeStatementTemp619);}());const auto nativeStatementTemp617=JSValue("fov");const auto nativeStatementTemp620=([&]()->JSValue{const auto nativeStatementTemp623=nativeStatementTemp609;const auto nativeStatementTemp624=JSValue("fov");return nativeStatementTemp623.get(nativeStatementTemp624);}());setReference(nativeStatementTemp616,nativeStatementTemp617,nativeStatementTemp620);return nativeStatementTemp620;}());
([&]()->JSValue{auto nativeStatementTemp625=([&]()->JSValue{const auto nativeStatementTemp627=data;const auto nativeStatementTemp628=JSValue("object");return nativeStatementTemp627.get(nativeStatementTemp628);}());const auto nativeStatementTemp626=JSValue("zoom");const auto nativeStatementTemp629=([&]()->JSValue{const auto nativeStatementTemp632=nativeStatementTemp609;const auto nativeStatementTemp633=JSValue("zoom");return nativeStatementTemp632.get(nativeStatementTemp633);}());setReference(nativeStatementTemp625,nativeStatementTemp626,nativeStatementTemp629);return nativeStatementTemp629;}());
([&]()->JSValue{auto nativeStatementTemp634=([&]()->JSValue{const auto nativeStatementTemp636=data;const auto nativeStatementTemp637=JSValue("object");return nativeStatementTemp636.get(nativeStatementTemp637);}());const auto nativeStatementTemp635=JSValue("near");const auto nativeStatementTemp638=([&]()->JSValue{const auto nativeStatementTemp641=nativeStatementTemp609;const auto nativeStatementTemp642=JSValue("near");return nativeStatementTemp641.get(nativeStatementTemp642);}());setReference(nativeStatementTemp634,nativeStatementTemp635,nativeStatementTemp638);return nativeStatementTemp638;}());
([&]()->JSValue{auto nativeStatementTemp643=([&]()->JSValue{const auto nativeStatementTemp645=data;const auto nativeStatementTemp646=JSValue("object");return nativeStatementTemp645.get(nativeStatementTemp646);}());const auto nativeStatementTemp644=JSValue("far");const auto nativeStatementTemp647=([&]()->JSValue{const auto nativeStatementTemp650=nativeStatementTemp609;const auto nativeStatementTemp651=JSValue("far");return nativeStatementTemp650.get(nativeStatementTemp651);}());setReference(nativeStatementTemp643,nativeStatementTemp644,nativeStatementTemp647);return nativeStatementTemp647;}());
([&]()->JSValue{auto nativeStatementTemp652=([&]()->JSValue{const auto nativeStatementTemp654=data;const auto nativeStatementTemp655=JSValue("object");return nativeStatementTemp654.get(nativeStatementTemp655);}());const auto nativeStatementTemp653=JSValue("focus");const auto nativeStatementTemp656=([&]()->JSValue{const auto nativeStatementTemp659=nativeStatementTemp609;const auto nativeStatementTemp660=JSValue("focus");return nativeStatementTemp659.get(nativeStatementTemp660);}());setReference(nativeStatementTemp652,nativeStatementTemp653,nativeStatementTemp656);return nativeStatementTemp656;}());
([&]()->JSValue{auto nativeStatementTemp661=([&]()->JSValue{const auto nativeStatementTemp663=data;const auto nativeStatementTemp664=JSValue("object");return nativeStatementTemp663.get(nativeStatementTemp664);}());const auto nativeStatementTemp662=JSValue("aspect");const auto nativeStatementTemp665=([&]()->JSValue{const auto nativeStatementTemp668=nativeStatementTemp609;const auto nativeStatementTemp669=JSValue("aspect");return nativeStatementTemp668.get(nativeStatementTemp669);}());setReference(nativeStatementTemp661,nativeStatementTemp662,nativeStatementTemp665);return nativeStatementTemp665;}());
if(([&]()->JSValue{const auto nativeStatementTemp670=([&]()->JSValue{const auto nativeStatementTemp672=nativeStatementTemp609;const auto nativeStatementTemp673=JSValue("view");return nativeStatementTemp672.get(nativeStatementTemp673);}());const auto nativeStatementTemp671=JSValue(nullptr);return binary("!==",nativeStatementTemp670,nativeStatementTemp671);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp674=([&]()->JSValue{const auto nativeStatementTemp676=data;const auto nativeStatementTemp677=JSValue("object");return nativeStatementTemp676.get(nativeStatementTemp677);}());const auto nativeStatementTemp675=JSValue("view");const auto nativeStatementTemp678=([&]()->JSValue{auto nativeStatementTemp684=nativeGlobalBinding("Object");const auto nativeStatementTemp685=JSValue("assign");const auto nativeStatementTemp681=nativeStatementTemp684.get(nativeStatementTemp685);const auto nativeStatementTemp682=nativeStatementTemp684;Args nativeStatementTemp683;nativeStatementTemp683.push_back(([&]()->JSValue{auto nativeStatementTemp686=JSValue::object();return nativeStatementTemp686;}()));nativeStatementTemp683.push_back(([&]()->JSValue{const auto nativeStatementTemp687=nativeStatementTemp609;const auto nativeStatementTemp688=JSValue("view");return nativeStatementTemp687.get(nativeStatementTemp688);}()));return nativeStatementTemp681.call(nativeStatementTemp682,nativeStatementTemp683);}());setReference(nativeStatementTemp674,nativeStatementTemp675,nativeStatementTemp678);return nativeStatementTemp678;}());
([&]()->JSValue{auto nativeStatementTemp689=([&]()->JSValue{const auto nativeStatementTemp691=data;const auto nativeStatementTemp692=JSValue("object");return nativeStatementTemp691.get(nativeStatementTemp692);}());const auto nativeStatementTemp690=JSValue("filmGauge");const auto nativeStatementTemp693=([&]()->JSValue{const auto nativeStatementTemp696=nativeStatementTemp609;const auto nativeStatementTemp697=JSValue("filmGauge");return nativeStatementTemp696.get(nativeStatementTemp697);}());setReference(nativeStatementTemp689,nativeStatementTemp690,nativeStatementTemp693);return nativeStatementTemp693;}());
([&]()->JSValue{auto nativeStatementTemp698=([&]()->JSValue{const auto nativeStatementTemp700=data;const auto nativeStatementTemp701=JSValue("object");return nativeStatementTemp700.get(nativeStatementTemp701);}());const auto nativeStatementTemp699=JSValue("filmOffset");const auto nativeStatementTemp702=([&]()->JSValue{const auto nativeStatementTemp705=nativeStatementTemp609;const auto nativeStatementTemp706=JSValue("filmOffset");return nativeStatementTemp705.get(nativeStatementTemp706);}());setReference(nativeStatementTemp698,nativeStatementTemp699,nativeStatementTemp702);return nativeStatementTemp702;}());
return data;
return JSValue{};
}
}
JSValue nativePerspectiveCameraModule(){using namespace PerspectiveCameraManaged;static thread_local JSWeakValue weak;if(const auto existing=weak.lock();existing&&existing->isObject())return *existing;static thread_local bool initializing=false;JSModuleInitializationScope initialization(initializing,"PerspectiveCamera module");auto module=JSValue::object(nullptr);
own(module,"Camera",nativeCameraConstructor());
own(module,"RAD2DEG",nativeMathUtilsBinding("RAD2DEG"));
own(module,"DEG2RAD",nativeMathUtilsBinding("DEG2RAD"));
own(module,"Vector2",nativeVector2Constructor());
own(module,"Vector3",nativeVector3Constructor());
own(module,"_v3",([&]()->JSValue{const auto nativeStatementTemp707=module.get("Vector3");Args nativeStatementTemp708;return nativeStatementTemp707.construct(nativeStatementTemp708);}()));
own(module,"_minTarget",([&]()->JSValue{const auto nativeStatementTemp709=module.get("Vector2");Args nativeStatementTemp710;return nativeStatementTemp709.construct(nativeStatementTemp710);}()));
own(module,"_maxTarget",([&]()->JSValue{const auto nativeStatementTemp711=module.get("Vector2");Args nativeStatementTemp712;return nativeStatementTemp711.construct(nativeStatementTemp712);}()));
auto ctor=JSValue::builtinConstructor(method0,{module,JSValue{}},"PerspectiveCamera",0);ctor.setFunctionSourceText(PerspectiveCameraRuntimeSource::classSource);auto prototype=ctor.get("prototype");
ctor.setPrototype(module.get("Camera"));prototype.setPrototype(module.get("Camera").get("prototype"));
own(module,"PerspectiveCamera",ctor);own(module,"_home",prototype);
{auto fn=JSValue::function(method1,{module,prototype},"copy",2);fn.setFunctionSourceText(PerspectiveCameraRuntimeSource::method1);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("copy",d);}
{auto fn=JSValue::function(method2,{module,prototype},"setFocalLength",1);fn.setFunctionSourceText(PerspectiveCameraRuntimeSource::method2);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("setFocalLength",d);}
{auto fn=JSValue::function(method3,{module,prototype},"getFocalLength",0);fn.setFunctionSourceText(PerspectiveCameraRuntimeSource::method3);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getFocalLength",d);}
{auto fn=JSValue::function(method4,{module,prototype},"getEffectiveFOV",0);fn.setFunctionSourceText(PerspectiveCameraRuntimeSource::method4);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getEffectiveFOV",d);}
{auto fn=JSValue::function(method5,{module,prototype},"getFilmWidth",0);fn.setFunctionSourceText(PerspectiveCameraRuntimeSource::method5);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getFilmWidth",d);}
{auto fn=JSValue::function(method6,{module,prototype},"getFilmHeight",0);fn.setFunctionSourceText(PerspectiveCameraRuntimeSource::method6);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getFilmHeight",d);}
{auto fn=JSValue::function(method7,{module,prototype},"getViewBounds",3);fn.setFunctionSourceText(PerspectiveCameraRuntimeSource::method7);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getViewBounds",d);}
{auto fn=JSValue::function(method8,{module,prototype},"getViewSize",2);fn.setFunctionSourceText(PerspectiveCameraRuntimeSource::method8);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getViewSize",d);}
{auto fn=JSValue::function(method9,{module,prototype},"setViewOffset",6);fn.setFunctionSourceText(PerspectiveCameraRuntimeSource::method9);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("setViewOffset",d);}
{auto fn=JSValue::function(method10,{module,prototype},"clearViewOffset",0);fn.setFunctionSourceText(PerspectiveCameraRuntimeSource::method10);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("clearViewOffset",d);}
{auto fn=JSValue::function(method11,{module,prototype},"updateProjectionMatrix",0);fn.setFunctionSourceText(PerspectiveCameraRuntimeSource::method11);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("updateProjectionMatrix",d);}
{auto fn=JSValue::function(method12,{module,prototype},"toJSON",1);fn.setFunctionSourceText(PerspectiveCameraRuntimeSource::method12);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("toJSON",d);}
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto exports=JSValue::object();own(exports,"PerspectiveCamera",module.get("PerspectiveCamera"));exports.retainForHeapLifetime();weak=JSWeakValue(exports);return exports;}
JSValue nativePerspectiveCameraBinding(const std::string& name){const auto keeper=nativePerspectiveCameraModule();return PerspectiveCameraManaged::lexical().lock()->get(name);}
JSValue nativePerspectiveCameraConstructor(){return nativePerspectiveCameraBinding("PerspectiveCamera");}
