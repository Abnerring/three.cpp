#include "cameras/Camera.h"
#include <stdexcept>

JsonValue CameraView::toJSON() const {
    return JsonValue::Object{{"enabled", enabled},       {"fullWidth", fullWidth},
                             {"fullHeight", fullHeight}, {"offsetX", offsetX},
                             {"offsetY", offsetY},       {"width", width},
                             {"height", height}};
}

Camera::Camera() {
    isCamera = true;
    type = "Camera";
}
std::optional<JsonValue> Camera::propertyValue(const std::string& property) const {
    if (property == "isCamera")
        return isCamera;
    if (property == "coordinateSystem")
        return coordinateSystem;
    if (property == "reversedDepth")
        return reversedDepth;
    return Object3D::propertyValue(property);
}
Camera& Camera::copy(const Object3D& source, bool recursive) {
    const auto* camera = dynamic_cast<const Camera*>(&source);
    if (!camera)
        throw std::invalid_argument("Camera::copy requires a Camera");
    Object3D::copy(source, recursive);
    matrixWorldInverse.copy(camera->matrixWorldInverse);
    projectionMatrix.copy(camera->projectionMatrix);
    projectionMatrixInverse.copy(camera->projectionMatrixInverse);
    coordinateSystem = camera->coordinateSystem;
    return *this;
}
Vector3& Camera::getWorldDirection(Vector3& target) {
    Object3D::getWorldDirection(target);
    target.negate();
    return target;
}
void Camera::updateViewMatrix() {
    Vector3 p, s;
    Quaternion q;
    matrixWorld.decompose(p, q, s);
    // three.js r186 excludes scale from camera view matrices to conform to glTF.
    if (s.x == 1 && s.y == 1 && s.z == 1)
        matrixWorldInverse.copy(matrixWorld);
    else {
        s.set(1, 1, 1);
        matrixWorldInverse.compose(p, q, s);
    }
    matrixWorldInverse.invert();
}
void Camera::updateMatrixWorld(bool force) {
    Object3D::updateMatrixWorld(force);
    updateViewMatrix();
}
void Camera::updateWorldMatrix(bool updateParents, bool updateChildren, bool force) {
    Object3D::updateWorldMatrix(updateParents, updateChildren, force);
    updateViewMatrix();
}
std::shared_ptr<Object3D> Camera::clone(bool) const {
    auto result = std::make_shared<Camera>();
    result->copy(*this);
    return result;
}
#include "math/JSNumberMath.h"

// Complete managed native port of pinned Camera.js. See THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSPromise.h"
#include "cameras/CameraRuntimeSource.inc"
#include "math/Matrix4.h"
#include "core/Object3D.h"
#include "math/Vector3.h"
#include "math/Quaternion.h"
#include <string_view>
#include <unordered_set>
namespace CameraManaged{

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
([&]()->JSValue{const auto nativeStatementTemp9=nativeStatementTemp3.get("Camera").getPrototype();Args nativeStatementTemp8;nativeStatementTemp4=nativeStatementTemp9.construct(nativeStatementTemp8,nativeStatementTemp5);return nativeStatementTemp4;}());
([&]()->JSValue{auto nativeStatementTemp10=nativeStatementTemp4;const auto nativeStatementTemp11=JSValue("isCamera");const auto nativeStatementTemp12=JSValue(true);setReference(nativeStatementTemp10,nativeStatementTemp11,nativeStatementTemp12);return nativeStatementTemp12;}());
([&]()->JSValue{auto nativeStatementTemp15=nativeStatementTemp4;const auto nativeStatementTemp16=JSValue("type");const auto nativeStatementTemp17=JSValue("Camera");setReference(nativeStatementTemp15,nativeStatementTemp16,nativeStatementTemp17);return nativeStatementTemp17;}());
([&]()->JSValue{auto nativeStatementTemp20=nativeStatementTemp4;const auto nativeStatementTemp21=JSValue("matrixWorldInverse");const auto nativeStatementTemp22=([&]()->JSValue{const auto nativeStatementTemp25=nativeStatementTemp3.get("Matrix4");Args nativeStatementTemp26;return nativeStatementTemp25.construct(nativeStatementTemp26);}());setReference(nativeStatementTemp20,nativeStatementTemp21,nativeStatementTemp22);return nativeStatementTemp22;}());
([&]()->JSValue{auto nativeStatementTemp27=nativeStatementTemp4;const auto nativeStatementTemp28=JSValue("projectionMatrix");const auto nativeStatementTemp29=([&]()->JSValue{const auto nativeStatementTemp32=nativeStatementTemp3.get("Matrix4");Args nativeStatementTemp33;return nativeStatementTemp32.construct(nativeStatementTemp33);}());setReference(nativeStatementTemp27,nativeStatementTemp28,nativeStatementTemp29);return nativeStatementTemp29;}());
([&]()->JSValue{auto nativeStatementTemp34=nativeStatementTemp4;const auto nativeStatementTemp35=JSValue("projectionMatrixInverse");const auto nativeStatementTemp36=([&]()->JSValue{const auto nativeStatementTemp39=nativeStatementTemp3.get("Matrix4");Args nativeStatementTemp40;return nativeStatementTemp39.construct(nativeStatementTemp40);}());setReference(nativeStatementTemp34,nativeStatementTemp35,nativeStatementTemp36);return nativeStatementTemp36;}());
([&]()->JSValue{auto nativeStatementTemp41=nativeStatementTemp4;const auto nativeStatementTemp42=JSValue("coordinateSystem");const auto nativeStatementTemp43=nativeStatementTemp3.get("WebGLCoordinateSystem");setReference(nativeStatementTemp41,nativeStatementTemp42,nativeStatementTemp43);return nativeStatementTemp43;}());
([&]()->JSValue{auto nativeStatementTemp46=nativeStatementTemp4;const auto nativeStatementTemp47=JSValue("_reversedDepth");const auto nativeStatementTemp48=JSValue(false);setReference(nativeStatementTemp46,nativeStatementTemp47,nativeStatementTemp48);return nativeStatementTemp48;}());
return nativeStatementTemp4;
}

JSValue method1(const JSValue& nativeStatementTemp51,const Args& nativeStatementTemp52,const Args& nativeStatementTemp53){
auto nativeStatementTemp54=nativeStatementTemp53[0];auto nativeStatementTemp55=nativeStatementTemp51;
return ([&]()->JSValue{const auto nativeStatementTemp57=nativeStatementTemp55;const auto nativeStatementTemp58=JSValue("_reversedDepth");return nativeStatementTemp57.get(nativeStatementTemp58);}());
return JSValue{};
}

JSValue method2(const JSValue& nativeStatementTemp59,const Args& nativeStatementTemp60,const Args& nativeStatementTemp61){
auto nativeStatementTemp62=nativeStatementTemp61[0];auto nativeStatementTemp63=nativeStatementTemp59;
JSValue source=arg(nativeStatementTemp60,0);
JSValue recursive=arg(nativeStatementTemp60,1);
([&]()->JSValue{const auto nativeStatementTemp68=nativeStatementTemp62.get("_home").getPrototype();const auto nativeStatementTemp69=JSValue("copy");const auto nativeStatementTemp65=nativeStatementTemp68.get(nativeStatementTemp69,nativeStatementTemp63);const auto nativeStatementTemp66=nativeStatementTemp63;Args nativeStatementTemp67;nativeStatementTemp67.push_back(source);nativeStatementTemp67.push_back(recursive);return nativeStatementTemp65.call(nativeStatementTemp66,nativeStatementTemp67);}());
([&]()->JSValue{auto nativeStatementTemp73=([&]()->JSValue{const auto nativeStatementTemp75=nativeStatementTemp63;const auto nativeStatementTemp76=JSValue("matrixWorldInverse");return nativeStatementTemp75.get(nativeStatementTemp76);}());const auto nativeStatementTemp74=JSValue("copy");const auto nativeStatementTemp70=nativeStatementTemp73.get(nativeStatementTemp74);const auto nativeStatementTemp71=nativeStatementTemp73;Args nativeStatementTemp72;nativeStatementTemp72.push_back(([&]()->JSValue{const auto nativeStatementTemp77=source;const auto nativeStatementTemp78=JSValue("matrixWorldInverse");return nativeStatementTemp77.get(nativeStatementTemp78);}()));return nativeStatementTemp70.call(nativeStatementTemp71,nativeStatementTemp72);}());
([&]()->JSValue{auto nativeStatementTemp82=([&]()->JSValue{const auto nativeStatementTemp84=nativeStatementTemp63;const auto nativeStatementTemp85=JSValue("projectionMatrix");return nativeStatementTemp84.get(nativeStatementTemp85);}());const auto nativeStatementTemp83=JSValue("copy");const auto nativeStatementTemp79=nativeStatementTemp82.get(nativeStatementTemp83);const auto nativeStatementTemp80=nativeStatementTemp82;Args nativeStatementTemp81;nativeStatementTemp81.push_back(([&]()->JSValue{const auto nativeStatementTemp86=source;const auto nativeStatementTemp87=JSValue("projectionMatrix");return nativeStatementTemp86.get(nativeStatementTemp87);}()));return nativeStatementTemp79.call(nativeStatementTemp80,nativeStatementTemp81);}());
([&]()->JSValue{auto nativeStatementTemp91=([&]()->JSValue{const auto nativeStatementTemp93=nativeStatementTemp63;const auto nativeStatementTemp94=JSValue("projectionMatrixInverse");return nativeStatementTemp93.get(nativeStatementTemp94);}());const auto nativeStatementTemp92=JSValue("copy");const auto nativeStatementTemp88=nativeStatementTemp91.get(nativeStatementTemp92);const auto nativeStatementTemp89=nativeStatementTemp91;Args nativeStatementTemp90;nativeStatementTemp90.push_back(([&]()->JSValue{const auto nativeStatementTemp95=source;const auto nativeStatementTemp96=JSValue("projectionMatrixInverse");return nativeStatementTemp95.get(nativeStatementTemp96);}()));return nativeStatementTemp88.call(nativeStatementTemp89,nativeStatementTemp90);}());
([&]()->JSValue{auto nativeStatementTemp97=nativeStatementTemp63;const auto nativeStatementTemp98=JSValue("coordinateSystem");const auto nativeStatementTemp99=([&]()->JSValue{const auto nativeStatementTemp102=source;const auto nativeStatementTemp103=JSValue("coordinateSystem");return nativeStatementTemp102.get(nativeStatementTemp103);}());setReference(nativeStatementTemp97,nativeStatementTemp98,nativeStatementTemp99);return nativeStatementTemp99;}());
return nativeStatementTemp63;
return JSValue{};
}

JSValue method3(const JSValue& nativeStatementTemp104,const Args& nativeStatementTemp105,const Args& nativeStatementTemp106){
auto nativeStatementTemp107=nativeStatementTemp106[0];auto nativeStatementTemp108=nativeStatementTemp104;
JSValue target=arg(nativeStatementTemp105,0);
return ([&]()->JSValue{auto nativeStatementTemp113=([&]()->JSValue{const auto nativeStatementTemp118=nativeStatementTemp107.get("_home").getPrototype();const auto nativeStatementTemp119=JSValue("getWorldDirection");const auto nativeStatementTemp115=nativeStatementTemp118.get(nativeStatementTemp119,nativeStatementTemp108);const auto nativeStatementTemp116=nativeStatementTemp108;Args nativeStatementTemp117;nativeStatementTemp117.push_back(target);return nativeStatementTemp115.call(nativeStatementTemp116,nativeStatementTemp117);}());const auto nativeStatementTemp114=JSValue("negate");const auto nativeStatementTemp110=nativeStatementTemp113.get(nativeStatementTemp114);const auto nativeStatementTemp111=nativeStatementTemp113;Args nativeStatementTemp112;return nativeStatementTemp110.call(nativeStatementTemp111,nativeStatementTemp112);}());
return JSValue{};
}

JSValue method4(const JSValue& nativeStatementTemp120,const Args& nativeStatementTemp121,const Args& nativeStatementTemp122){
auto nativeStatementTemp123=nativeStatementTemp122[0];auto nativeStatementTemp124=nativeStatementTemp120;
JSValue force=arg(nativeStatementTemp121,0);
([&]()->JSValue{const auto nativeStatementTemp129=nativeStatementTemp123.get("_home").getPrototype();const auto nativeStatementTemp130=JSValue("updateMatrixWorld");const auto nativeStatementTemp126=nativeStatementTemp129.get(nativeStatementTemp130,nativeStatementTemp124);const auto nativeStatementTemp127=nativeStatementTemp124;Args nativeStatementTemp128;nativeStatementTemp128.push_back(force);return nativeStatementTemp126.call(nativeStatementTemp127,nativeStatementTemp128);}());
([&]()->JSValue{auto nativeStatementTemp134=([&]()->JSValue{const auto nativeStatementTemp136=nativeStatementTemp124;const auto nativeStatementTemp137=JSValue("matrixWorld");return nativeStatementTemp136.get(nativeStatementTemp137);}());const auto nativeStatementTemp135=JSValue("decompose");const auto nativeStatementTemp131=nativeStatementTemp134.get(nativeStatementTemp135);const auto nativeStatementTemp132=nativeStatementTemp134;Args nativeStatementTemp133;nativeStatementTemp133.push_back(nativeStatementTemp123.get("_position"));nativeStatementTemp133.push_back(nativeStatementTemp123.get("_quaternion"));nativeStatementTemp133.push_back(nativeStatementTemp123.get("_scale"));return nativeStatementTemp131.call(nativeStatementTemp132,nativeStatementTemp133);}());
if(([&]()->JSValue{const auto nativeStatementTemp138=([&]()->JSValue{const auto nativeStatementTemp139=([&]()->JSValue{const auto nativeStatementTemp140=([&]()->JSValue{const auto nativeStatementTemp142=nativeStatementTemp123.get("_scale");const auto nativeStatementTemp143=JSValue("x");return nativeStatementTemp142.get(nativeStatementTemp143);}());const auto nativeStatementTemp141=JSValue(1);return binary("===",nativeStatementTemp140,nativeStatementTemp141);}());if(!nativeStatementTemp139.toBoolean())return nativeStatementTemp139;return ([&]()->JSValue{const auto nativeStatementTemp144=([&]()->JSValue{const auto nativeStatementTemp146=nativeStatementTemp123.get("_scale");const auto nativeStatementTemp147=JSValue("y");return nativeStatementTemp146.get(nativeStatementTemp147);}());const auto nativeStatementTemp145=JSValue(1);return binary("===",nativeStatementTemp144,nativeStatementTemp145);}());}());if(!nativeStatementTemp138.toBoolean())return nativeStatementTemp138;return ([&]()->JSValue{const auto nativeStatementTemp148=([&]()->JSValue{const auto nativeStatementTemp150=nativeStatementTemp123.get("_scale");const auto nativeStatementTemp151=JSValue("z");return nativeStatementTemp150.get(nativeStatementTemp151);}());const auto nativeStatementTemp149=JSValue(1);return binary("===",nativeStatementTemp148,nativeStatementTemp149);}());}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp155=([&]()->JSValue{auto nativeStatementTemp160=([&]()->JSValue{const auto nativeStatementTemp162=nativeStatementTemp124;const auto nativeStatementTemp163=JSValue("matrixWorldInverse");return nativeStatementTemp162.get(nativeStatementTemp163);}());const auto nativeStatementTemp161=JSValue("copy");const auto nativeStatementTemp157=nativeStatementTemp160.get(nativeStatementTemp161);const auto nativeStatementTemp158=nativeStatementTemp160;Args nativeStatementTemp159;nativeStatementTemp159.push_back(([&]()->JSValue{const auto nativeStatementTemp164=nativeStatementTemp124;const auto nativeStatementTemp165=JSValue("matrixWorld");return nativeStatementTemp164.get(nativeStatementTemp165);}()));return nativeStatementTemp157.call(nativeStatementTemp158,nativeStatementTemp159);}());const auto nativeStatementTemp156=JSValue("invert");const auto nativeStatementTemp152=nativeStatementTemp155.get(nativeStatementTemp156);const auto nativeStatementTemp153=nativeStatementTemp155;Args nativeStatementTemp154;return nativeStatementTemp152.call(nativeStatementTemp153,nativeStatementTemp154);}());

}
else {
([&]()->JSValue{auto nativeStatementTemp169=([&]()->JSValue{auto nativeStatementTemp174=([&]()->JSValue{const auto nativeStatementTemp176=nativeStatementTemp124;const auto nativeStatementTemp177=JSValue("matrixWorldInverse");return nativeStatementTemp176.get(nativeStatementTemp177);}());const auto nativeStatementTemp175=JSValue("compose");const auto nativeStatementTemp171=nativeStatementTemp174.get(nativeStatementTemp175);const auto nativeStatementTemp172=nativeStatementTemp174;Args nativeStatementTemp173;nativeStatementTemp173.push_back(nativeStatementTemp123.get("_position"));nativeStatementTemp173.push_back(nativeStatementTemp123.get("_quaternion"));nativeStatementTemp173.push_back(([&]()->JSValue{auto nativeStatementTemp181=nativeStatementTemp123.get("_scale");const auto nativeStatementTemp182=JSValue("set");const auto nativeStatementTemp178=nativeStatementTemp181.get(nativeStatementTemp182);const auto nativeStatementTemp179=nativeStatementTemp181;Args nativeStatementTemp180;nativeStatementTemp180.push_back(JSValue(1));nativeStatementTemp180.push_back(JSValue(1));nativeStatementTemp180.push_back(JSValue(1));return nativeStatementTemp178.call(nativeStatementTemp179,nativeStatementTemp180);}()));return nativeStatementTemp171.call(nativeStatementTemp172,nativeStatementTemp173);}());const auto nativeStatementTemp170=JSValue("invert");const auto nativeStatementTemp166=nativeStatementTemp169.get(nativeStatementTemp170);const auto nativeStatementTemp167=nativeStatementTemp169;Args nativeStatementTemp168;return nativeStatementTemp166.call(nativeStatementTemp167,nativeStatementTemp168);}());

}
return JSValue{};
}

JSValue method5(const JSValue& nativeStatementTemp183,const Args& nativeStatementTemp184,const Args& nativeStatementTemp185){
auto nativeStatementTemp186=nativeStatementTemp185[0];auto nativeStatementTemp187=nativeStatementTemp183;
JSValue updateParents=arg(nativeStatementTemp184,0);
JSValue updateChildren=arg(nativeStatementTemp184,1);
JSValue force=arg(nativeStatementTemp184,2);
if(force.isUndefined())force=JSValue(false);
([&]()->JSValue{const auto nativeStatementTemp192=nativeStatementTemp186.get("_home").getPrototype();const auto nativeStatementTemp193=JSValue("updateWorldMatrix");const auto nativeStatementTemp189=nativeStatementTemp192.get(nativeStatementTemp193,nativeStatementTemp187);const auto nativeStatementTemp190=nativeStatementTemp187;Args nativeStatementTemp191;nativeStatementTemp191.push_back(updateParents);nativeStatementTemp191.push_back(updateChildren);nativeStatementTemp191.push_back(force);return nativeStatementTemp189.call(nativeStatementTemp190,nativeStatementTemp191);}());
([&]()->JSValue{auto nativeStatementTemp197=([&]()->JSValue{const auto nativeStatementTemp199=nativeStatementTemp187;const auto nativeStatementTemp200=JSValue("matrixWorld");return nativeStatementTemp199.get(nativeStatementTemp200);}());const auto nativeStatementTemp198=JSValue("decompose");const auto nativeStatementTemp194=nativeStatementTemp197.get(nativeStatementTemp198);const auto nativeStatementTemp195=nativeStatementTemp197;Args nativeStatementTemp196;nativeStatementTemp196.push_back(nativeStatementTemp186.get("_position"));nativeStatementTemp196.push_back(nativeStatementTemp186.get("_quaternion"));nativeStatementTemp196.push_back(nativeStatementTemp186.get("_scale"));return nativeStatementTemp194.call(nativeStatementTemp195,nativeStatementTemp196);}());
if(([&]()->JSValue{const auto nativeStatementTemp201=([&]()->JSValue{const auto nativeStatementTemp202=([&]()->JSValue{const auto nativeStatementTemp203=([&]()->JSValue{const auto nativeStatementTemp205=nativeStatementTemp186.get("_scale");const auto nativeStatementTemp206=JSValue("x");return nativeStatementTemp205.get(nativeStatementTemp206);}());const auto nativeStatementTemp204=JSValue(1);return binary("===",nativeStatementTemp203,nativeStatementTemp204);}());if(!nativeStatementTemp202.toBoolean())return nativeStatementTemp202;return ([&]()->JSValue{const auto nativeStatementTemp207=([&]()->JSValue{const auto nativeStatementTemp209=nativeStatementTemp186.get("_scale");const auto nativeStatementTemp210=JSValue("y");return nativeStatementTemp209.get(nativeStatementTemp210);}());const auto nativeStatementTemp208=JSValue(1);return binary("===",nativeStatementTemp207,nativeStatementTemp208);}());}());if(!nativeStatementTemp201.toBoolean())return nativeStatementTemp201;return ([&]()->JSValue{const auto nativeStatementTemp211=([&]()->JSValue{const auto nativeStatementTemp213=nativeStatementTemp186.get("_scale");const auto nativeStatementTemp214=JSValue("z");return nativeStatementTemp213.get(nativeStatementTemp214);}());const auto nativeStatementTemp212=JSValue(1);return binary("===",nativeStatementTemp211,nativeStatementTemp212);}());}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp218=([&]()->JSValue{auto nativeStatementTemp223=([&]()->JSValue{const auto nativeStatementTemp225=nativeStatementTemp187;const auto nativeStatementTemp226=JSValue("matrixWorldInverse");return nativeStatementTemp225.get(nativeStatementTemp226);}());const auto nativeStatementTemp224=JSValue("copy");const auto nativeStatementTemp220=nativeStatementTemp223.get(nativeStatementTemp224);const auto nativeStatementTemp221=nativeStatementTemp223;Args nativeStatementTemp222;nativeStatementTemp222.push_back(([&]()->JSValue{const auto nativeStatementTemp227=nativeStatementTemp187;const auto nativeStatementTemp228=JSValue("matrixWorld");return nativeStatementTemp227.get(nativeStatementTemp228);}()));return nativeStatementTemp220.call(nativeStatementTemp221,nativeStatementTemp222);}());const auto nativeStatementTemp219=JSValue("invert");const auto nativeStatementTemp215=nativeStatementTemp218.get(nativeStatementTemp219);const auto nativeStatementTemp216=nativeStatementTemp218;Args nativeStatementTemp217;return nativeStatementTemp215.call(nativeStatementTemp216,nativeStatementTemp217);}());

}
else {
([&]()->JSValue{auto nativeStatementTemp232=([&]()->JSValue{auto nativeStatementTemp237=([&]()->JSValue{const auto nativeStatementTemp239=nativeStatementTemp187;const auto nativeStatementTemp240=JSValue("matrixWorldInverse");return nativeStatementTemp239.get(nativeStatementTemp240);}());const auto nativeStatementTemp238=JSValue("compose");const auto nativeStatementTemp234=nativeStatementTemp237.get(nativeStatementTemp238);const auto nativeStatementTemp235=nativeStatementTemp237;Args nativeStatementTemp236;nativeStatementTemp236.push_back(nativeStatementTemp186.get("_position"));nativeStatementTemp236.push_back(nativeStatementTemp186.get("_quaternion"));nativeStatementTemp236.push_back(([&]()->JSValue{auto nativeStatementTemp244=nativeStatementTemp186.get("_scale");const auto nativeStatementTemp245=JSValue("set");const auto nativeStatementTemp241=nativeStatementTemp244.get(nativeStatementTemp245);const auto nativeStatementTemp242=nativeStatementTemp244;Args nativeStatementTemp243;nativeStatementTemp243.push_back(JSValue(1));nativeStatementTemp243.push_back(JSValue(1));nativeStatementTemp243.push_back(JSValue(1));return nativeStatementTemp241.call(nativeStatementTemp242,nativeStatementTemp243);}()));return nativeStatementTemp234.call(nativeStatementTemp235,nativeStatementTemp236);}());const auto nativeStatementTemp233=JSValue("invert");const auto nativeStatementTemp229=nativeStatementTemp232.get(nativeStatementTemp233);const auto nativeStatementTemp230=nativeStatementTemp232;Args nativeStatementTemp231;return nativeStatementTemp229.call(nativeStatementTemp230,nativeStatementTemp231);}());

}
return JSValue{};
}

JSValue method6(const JSValue& nativeStatementTemp246,const Args& nativeStatementTemp247,const Args& nativeStatementTemp248){
auto nativeStatementTemp249=nativeStatementTemp248[0];auto nativeStatementTemp250=nativeStatementTemp246;
return ([&]()->JSValue{auto nativeStatementTemp255=([&]()->JSValue{const auto nativeStatementTemp257=([&]()->JSValue{const auto nativeStatementTemp259=nativeStatementTemp250;const auto nativeStatementTemp260=JSValue("constructor");return nativeStatementTemp259.get(nativeStatementTemp260);}());Args nativeStatementTemp258;return nativeStatementTemp257.construct(nativeStatementTemp258);}());const auto nativeStatementTemp256=JSValue("copy");const auto nativeStatementTemp252=nativeStatementTemp255.get(nativeStatementTemp256);const auto nativeStatementTemp253=nativeStatementTemp255;Args nativeStatementTemp254;nativeStatementTemp254.push_back(nativeStatementTemp250);return nativeStatementTemp252.call(nativeStatementTemp253,nativeStatementTemp254);}());
return JSValue{};
}
}
JSValue nativeCameraClassModule(){using namespace CameraManaged;static thread_local JSWeakValue weak;if(const auto existing=weak.lock();existing&&existing->isObject())return *existing;static thread_local bool initializing=false;JSModuleInitializationScope initialization(initializing,"Camera module");auto module=JSValue::object(nullptr);
own(module,"WebGLCoordinateSystem",JSValue(2000));
own(module,"Matrix4",nativeMatrix4Constructor());
own(module,"Object3D",nativeObject3DConstructor());
own(module,"Vector3",nativeVector3Constructor());
own(module,"Quaternion",nativeQuaternionConstructor());
own(module,"_position",([&]()->JSValue{const auto nativeStatementTemp261=module.get("Vector3");Args nativeStatementTemp262;return nativeStatementTemp261.construct(nativeStatementTemp262);}()));
own(module,"_quaternion",([&]()->JSValue{const auto nativeStatementTemp263=module.get("Quaternion");Args nativeStatementTemp264;return nativeStatementTemp263.construct(nativeStatementTemp264);}()));
own(module,"_scale",([&]()->JSValue{const auto nativeStatementTemp265=module.get("Vector3");Args nativeStatementTemp266;return nativeStatementTemp265.construct(nativeStatementTemp266);}()));
auto ctor=JSValue::builtinConstructor(method0,{module,JSValue{}},"Camera",0);ctor.setFunctionSourceText(CameraRuntimeSource::classSource);auto prototype=ctor.get("prototype");
ctor.setPrototype(module.get("Object3D"));prototype.setPrototype(module.get("Object3D").get("prototype"));
own(module,"Camera",ctor);own(module,"_home",prototype);
{auto fn=JSValue::function(method1,{module,prototype},"get reversedDepth",0);fn.setFunctionSourceText(CameraRuntimeSource::method1);JSPropertyDescriptor d;d.configurable=true;d.getter=fn;prototype.defineProperty("reversedDepth",d);}
{auto fn=JSValue::function(method2,{module,prototype},"copy",2);fn.setFunctionSourceText(CameraRuntimeSource::method2);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("copy",d);}
{auto fn=JSValue::function(method3,{module,prototype},"getWorldDirection",1);fn.setFunctionSourceText(CameraRuntimeSource::method3);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getWorldDirection",d);}
{auto fn=JSValue::function(method4,{module,prototype},"updateMatrixWorld",1);fn.setFunctionSourceText(CameraRuntimeSource::method4);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("updateMatrixWorld",d);}
{auto fn=JSValue::function(method5,{module,prototype},"updateWorldMatrix",2);fn.setFunctionSourceText(CameraRuntimeSource::method5);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("updateWorldMatrix",d);}
{auto fn=JSValue::function(method6,{module,prototype},"clone",0);fn.setFunctionSourceText(CameraRuntimeSource::method6);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("clone",d);}
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto exports=JSValue::object();own(exports,"Camera",module.get("Camera"));exports.retainForHeapLifetime();weak=JSWeakValue(exports);return exports;}
JSValue nativeCameraClassBinding(const std::string& name){const auto keeper=nativeCameraClassModule();return CameraManaged::lexical().lock()->get(name);}
JSValue nativeCameraConstructor(){return nativeCameraClassBinding("Camera");}
