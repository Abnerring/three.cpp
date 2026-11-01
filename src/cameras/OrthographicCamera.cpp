#include "cameras/OrthographicCamera.h"
#include <stdexcept>

OrthographicCamera::OrthographicCamera(HIGH_PRECISION left, HIGH_PRECISION right,
                                       HIGH_PRECISION top, HIGH_PRECISION bottom,
                                       HIGH_PRECISION near, HIGH_PRECISION far)
    : left(left), right(right), top(top), bottom(bottom), near(near), far(far) {
    type = "OrthographicCamera";
    updateProjectionMatrix();
}
std::optional<JsonValue> OrthographicCamera::propertyValue(const std::string& property) const {
    if (property == "isOrthographicCamera")
        return isOrthographicCamera;
    if (property == "zoom")
        return zoom;
    if (property == "left")
        return left;
    if (property == "right")
        return right;
    if (property == "top")
        return top;
    if (property == "bottom")
        return bottom;
    if (property == "near")
        return near;
    if (property == "far")
        return far;
    return Camera::propertyValue(property);
}
OrthographicCamera& OrthographicCamera::copy(const Object3D& source, bool recursive) {
    const auto* camera = dynamic_cast<const OrthographicCamera*>(&source);
    if (!camera)
        throw std::invalid_argument("OrthographicCamera::copy requires an OrthographicCamera");
    Camera::copy(source, recursive);
    left = camera->left;
    right = camera->right;
    top = camera->top;
    bottom = camera->bottom;
    near = camera->near;
    far = camera->far;
    zoom = camera->zoom;
    view = camera->view;
    return *this;
}
std::shared_ptr<Object3D> OrthographicCamera::clone(bool) const {
    auto result = std::make_shared<OrthographicCamera>();
    result->copy(*this);
    return result;
}
void OrthographicCamera::setViewOffset(HIGH_PRECISION fullWidth, HIGH_PRECISION fullHeight,
                                       HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION width,
                                       HIGH_PRECISION height) {
    view = CameraView{true, fullWidth, fullHeight, x, y, width, height};
    updateProjectionMatrix();
}
void OrthographicCamera::clearViewOffset() {
    if (view)
        view->enabled = false;
    updateProjectionMatrix();
}
void OrthographicCamera::updateProjectionMatrix() {
    const auto dx = (right - left) / (2 * zoom), dy = (top - bottom) / (2 * zoom);
    const auto cx = (right + left) / 2, cy = (top + bottom) / 2;
    auto l = cx - dx, r = cx + dx, t = cy + dy, b = cy - dy;
    if (view && view->enabled) {
        const auto scaleW = (right - left) / view->fullWidth / zoom;
        const auto scaleH = (top - bottom) / view->fullHeight / zoom;
        l += scaleW * view->offsetX;
        r = l + scaleW * view->width;
        t -= scaleH * view->offsetY;
        b = t - scaleH * view->height;
    }
    projectionMatrix.makeOrthographic(l, r, t, b, near, far, coordinateSystem, reversedDepth);
    projectionMatrixInverse.copy(projectionMatrix);
    projectionMatrixInverse.invert();
}
JsonValue OrthographicCamera::toJSON(Object3DJSONContext* meta) const {
    auto output = Camera::toJSON(meta);
    auto& object = output["object"];
    object["zoom"] = zoom;
    object["left"] = left;
    object["right"] = right;
    object["top"] = top;
    object["bottom"] = bottom;
    object["near"] = near;
    object["far"] = far;
    if (view)
        object["view"] = view->toJSON();
    return output;
}
#include "math/JSNumberMath.h"

// Complete managed native port of pinned OrthographicCamera.js. See THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSPromise.h"
#include "cameras/OrthographicCameraRuntimeSource.inc"
#include "cameras/Camera.h"
#include <string_view>
#include <unordered_set>
namespace OrthographicCameraManaged{

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
JSValue left=arg(nativeStatementTemp1,0);
if(left.isUndefined())left=([&]()->JSValue{const auto nativeStatementTemp6=JSValue(1);return -nativeStatementTemp6.toNumber();}());
JSValue right=arg(nativeStatementTemp1,1);
if(right.isUndefined())right=JSValue(1);
JSValue top=arg(nativeStatementTemp1,2);
if(top.isUndefined())top=JSValue(1);
JSValue bottom=arg(nativeStatementTemp1,3);
if(bottom.isUndefined())bottom=([&]()->JSValue{const auto nativeStatementTemp7=JSValue(1);return -nativeStatementTemp7.toNumber();}());
JSValue near=arg(nativeStatementTemp1,4);
if(near.isUndefined())near=JSValue(0.1);
JSValue far=arg(nativeStatementTemp1,5);
if(far.isUndefined())far=JSValue(2000);
([&]()->JSValue{const auto nativeStatementTemp11=nativeStatementTemp3.get("OrthographicCamera").getPrototype();Args nativeStatementTemp10;nativeStatementTemp4=nativeStatementTemp11.construct(nativeStatementTemp10,nativeStatementTemp5);return nativeStatementTemp4;}());
([&]()->JSValue{auto nativeStatementTemp12=nativeStatementTemp4;const auto nativeStatementTemp13=JSValue("isOrthographicCamera");const auto nativeStatementTemp14=JSValue(true);setReference(nativeStatementTemp12,nativeStatementTemp13,nativeStatementTemp14);return nativeStatementTemp14;}());
([&]()->JSValue{auto nativeStatementTemp17=nativeStatementTemp4;const auto nativeStatementTemp18=JSValue("type");const auto nativeStatementTemp19=JSValue("OrthographicCamera");setReference(nativeStatementTemp17,nativeStatementTemp18,nativeStatementTemp19);return nativeStatementTemp19;}());
([&]()->JSValue{auto nativeStatementTemp22=nativeStatementTemp4;const auto nativeStatementTemp23=JSValue("zoom");const auto nativeStatementTemp24=JSValue(1);setReference(nativeStatementTemp22,nativeStatementTemp23,nativeStatementTemp24);return nativeStatementTemp24;}());
([&]()->JSValue{auto nativeStatementTemp27=nativeStatementTemp4;const auto nativeStatementTemp28=JSValue("view");const auto nativeStatementTemp29=JSValue(nullptr);setReference(nativeStatementTemp27,nativeStatementTemp28,nativeStatementTemp29);return nativeStatementTemp29;}());
([&]()->JSValue{auto nativeStatementTemp32=nativeStatementTemp4;const auto nativeStatementTemp33=JSValue("left");const auto nativeStatementTemp34=left;setReference(nativeStatementTemp32,nativeStatementTemp33,nativeStatementTemp34);return nativeStatementTemp34;}());
([&]()->JSValue{auto nativeStatementTemp37=nativeStatementTemp4;const auto nativeStatementTemp38=JSValue("right");const auto nativeStatementTemp39=right;setReference(nativeStatementTemp37,nativeStatementTemp38,nativeStatementTemp39);return nativeStatementTemp39;}());
([&]()->JSValue{auto nativeStatementTemp42=nativeStatementTemp4;const auto nativeStatementTemp43=JSValue("top");const auto nativeStatementTemp44=top;setReference(nativeStatementTemp42,nativeStatementTemp43,nativeStatementTemp44);return nativeStatementTemp44;}());
([&]()->JSValue{auto nativeStatementTemp47=nativeStatementTemp4;const auto nativeStatementTemp48=JSValue("bottom");const auto nativeStatementTemp49=bottom;setReference(nativeStatementTemp47,nativeStatementTemp48,nativeStatementTemp49);return nativeStatementTemp49;}());
([&]()->JSValue{auto nativeStatementTemp52=nativeStatementTemp4;const auto nativeStatementTemp53=JSValue("near");const auto nativeStatementTemp54=near;setReference(nativeStatementTemp52,nativeStatementTemp53,nativeStatementTemp54);return nativeStatementTemp54;}());
([&]()->JSValue{auto nativeStatementTemp57=nativeStatementTemp4;const auto nativeStatementTemp58=JSValue("far");const auto nativeStatementTemp59=far;setReference(nativeStatementTemp57,nativeStatementTemp58,nativeStatementTemp59);return nativeStatementTemp59;}());
([&]()->JSValue{auto nativeStatementTemp65=nativeStatementTemp4;const auto nativeStatementTemp66=JSValue("updateProjectionMatrix");const auto nativeStatementTemp62=nativeStatementTemp65.get(nativeStatementTemp66);const auto nativeStatementTemp63=nativeStatementTemp65;Args nativeStatementTemp64;return nativeStatementTemp62.call(nativeStatementTemp63,nativeStatementTemp64);}());
return nativeStatementTemp4;
}

JSValue method1(const JSValue& nativeStatementTemp67,const Args& nativeStatementTemp68,const Args& nativeStatementTemp69){
auto nativeStatementTemp70=nativeStatementTemp69[0];auto nativeStatementTemp71=nativeStatementTemp67;
JSValue source=arg(nativeStatementTemp68,0);
JSValue recursive=arg(nativeStatementTemp68,1);
([&]()->JSValue{const auto nativeStatementTemp76=nativeStatementTemp70.get("_home").getPrototype();const auto nativeStatementTemp77=JSValue("copy");const auto nativeStatementTemp73=nativeStatementTemp76.get(nativeStatementTemp77,nativeStatementTemp71);const auto nativeStatementTemp74=nativeStatementTemp71;Args nativeStatementTemp75;nativeStatementTemp75.push_back(source);nativeStatementTemp75.push_back(recursive);return nativeStatementTemp73.call(nativeStatementTemp74,nativeStatementTemp75);}());
([&]()->JSValue{auto nativeStatementTemp78=nativeStatementTemp71;const auto nativeStatementTemp79=JSValue("left");const auto nativeStatementTemp80=([&]()->JSValue{const auto nativeStatementTemp83=source;const auto nativeStatementTemp84=JSValue("left");return nativeStatementTemp83.get(nativeStatementTemp84);}());setReference(nativeStatementTemp78,nativeStatementTemp79,nativeStatementTemp80);return nativeStatementTemp80;}());
([&]()->JSValue{auto nativeStatementTemp85=nativeStatementTemp71;const auto nativeStatementTemp86=JSValue("right");const auto nativeStatementTemp87=([&]()->JSValue{const auto nativeStatementTemp90=source;const auto nativeStatementTemp91=JSValue("right");return nativeStatementTemp90.get(nativeStatementTemp91);}());setReference(nativeStatementTemp85,nativeStatementTemp86,nativeStatementTemp87);return nativeStatementTemp87;}());
([&]()->JSValue{auto nativeStatementTemp92=nativeStatementTemp71;const auto nativeStatementTemp93=JSValue("top");const auto nativeStatementTemp94=([&]()->JSValue{const auto nativeStatementTemp97=source;const auto nativeStatementTemp98=JSValue("top");return nativeStatementTemp97.get(nativeStatementTemp98);}());setReference(nativeStatementTemp92,nativeStatementTemp93,nativeStatementTemp94);return nativeStatementTemp94;}());
([&]()->JSValue{auto nativeStatementTemp99=nativeStatementTemp71;const auto nativeStatementTemp100=JSValue("bottom");const auto nativeStatementTemp101=([&]()->JSValue{const auto nativeStatementTemp104=source;const auto nativeStatementTemp105=JSValue("bottom");return nativeStatementTemp104.get(nativeStatementTemp105);}());setReference(nativeStatementTemp99,nativeStatementTemp100,nativeStatementTemp101);return nativeStatementTemp101;}());
([&]()->JSValue{auto nativeStatementTemp106=nativeStatementTemp71;const auto nativeStatementTemp107=JSValue("near");const auto nativeStatementTemp108=([&]()->JSValue{const auto nativeStatementTemp111=source;const auto nativeStatementTemp112=JSValue("near");return nativeStatementTemp111.get(nativeStatementTemp112);}());setReference(nativeStatementTemp106,nativeStatementTemp107,nativeStatementTemp108);return nativeStatementTemp108;}());
([&]()->JSValue{auto nativeStatementTemp113=nativeStatementTemp71;const auto nativeStatementTemp114=JSValue("far");const auto nativeStatementTemp115=([&]()->JSValue{const auto nativeStatementTemp118=source;const auto nativeStatementTemp119=JSValue("far");return nativeStatementTemp118.get(nativeStatementTemp119);}());setReference(nativeStatementTemp113,nativeStatementTemp114,nativeStatementTemp115);return nativeStatementTemp115;}());
([&]()->JSValue{auto nativeStatementTemp120=nativeStatementTemp71;const auto nativeStatementTemp121=JSValue("zoom");const auto nativeStatementTemp122=([&]()->JSValue{const auto nativeStatementTemp125=source;const auto nativeStatementTemp126=JSValue("zoom");return nativeStatementTemp125.get(nativeStatementTemp126);}());setReference(nativeStatementTemp120,nativeStatementTemp121,nativeStatementTemp122);return nativeStatementTemp122;}());
([&]()->JSValue{auto nativeStatementTemp127=nativeStatementTemp71;const auto nativeStatementTemp128=JSValue("view");const auto nativeStatementTemp129=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp132=([&]()->JSValue{const auto nativeStatementTemp134=source;const auto nativeStatementTemp135=JSValue("view");return nativeStatementTemp134.get(nativeStatementTemp135);}());const auto nativeStatementTemp133=JSValue(nullptr);return binary("===",nativeStatementTemp132,nativeStatementTemp133);}()).toBoolean())return JSValue(nullptr);return ([&]()->JSValue{auto nativeStatementTemp139=nativeGlobalBinding("Object");const auto nativeStatementTemp140=JSValue("assign");const auto nativeStatementTemp136=nativeStatementTemp139.get(nativeStatementTemp140);const auto nativeStatementTemp137=nativeStatementTemp139;Args nativeStatementTemp138;nativeStatementTemp138.push_back(([&]()->JSValue{auto nativeStatementTemp141=JSValue::object();return nativeStatementTemp141;}()));nativeStatementTemp138.push_back(([&]()->JSValue{const auto nativeStatementTemp142=source;const auto nativeStatementTemp143=JSValue("view");return nativeStatementTemp142.get(nativeStatementTemp143);}()));return nativeStatementTemp136.call(nativeStatementTemp137,nativeStatementTemp138);}());}());setReference(nativeStatementTemp127,nativeStatementTemp128,nativeStatementTemp129);return nativeStatementTemp129;}());
return nativeStatementTemp71;
return JSValue{};
}

JSValue method2(const JSValue& nativeStatementTemp144,const Args& nativeStatementTemp145,const Args& nativeStatementTemp146){
auto nativeStatementTemp147=nativeStatementTemp146[0];auto nativeStatementTemp148=nativeStatementTemp144;
JSValue fullWidth=arg(nativeStatementTemp145,0);
JSValue fullHeight=arg(nativeStatementTemp145,1);
JSValue x=arg(nativeStatementTemp145,2);
JSValue y=arg(nativeStatementTemp145,3);
JSValue width=arg(nativeStatementTemp145,4);
JSValue height=arg(nativeStatementTemp145,5);
if(([&]()->JSValue{const auto nativeStatementTemp150=([&]()->JSValue{const auto nativeStatementTemp152=nativeStatementTemp148;const auto nativeStatementTemp153=JSValue("view");return nativeStatementTemp152.get(nativeStatementTemp153);}());const auto nativeStatementTemp151=JSValue(nullptr);return binary("===",nativeStatementTemp150,nativeStatementTemp151);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp154=nativeStatementTemp148;const auto nativeStatementTemp155=JSValue("view");const auto nativeStatementTemp156=([&]()->JSValue{auto nativeStatementTemp159=JSValue::object();{const auto nativeStatementTemp160=JSValue("enabled");const auto nativeStatementTemp161=JSValue(true);own(nativeStatementTemp159,nativeStatementTemp160,nativeStatementTemp161);}{const auto nativeStatementTemp162=JSValue("fullWidth");const auto nativeStatementTemp163=JSValue(1);own(nativeStatementTemp159,nativeStatementTemp162,nativeStatementTemp163);}{const auto nativeStatementTemp164=JSValue("fullHeight");const auto nativeStatementTemp165=JSValue(1);own(nativeStatementTemp159,nativeStatementTemp164,nativeStatementTemp165);}{const auto nativeStatementTemp166=JSValue("offsetX");const auto nativeStatementTemp167=JSValue(0);own(nativeStatementTemp159,nativeStatementTemp166,nativeStatementTemp167);}{const auto nativeStatementTemp168=JSValue("offsetY");const auto nativeStatementTemp169=JSValue(0);own(nativeStatementTemp159,nativeStatementTemp168,nativeStatementTemp169);}{const auto nativeStatementTemp170=JSValue("width");const auto nativeStatementTemp171=JSValue(1);own(nativeStatementTemp159,nativeStatementTemp170,nativeStatementTemp171);}{const auto nativeStatementTemp172=JSValue("height");const auto nativeStatementTemp173=JSValue(1);own(nativeStatementTemp159,nativeStatementTemp172,nativeStatementTemp173);}return nativeStatementTemp159;}());setReference(nativeStatementTemp154,nativeStatementTemp155,nativeStatementTemp156);return nativeStatementTemp156;}());

}
([&]()->JSValue{auto nativeStatementTemp174=([&]()->JSValue{const auto nativeStatementTemp176=nativeStatementTemp148;const auto nativeStatementTemp177=JSValue("view");return nativeStatementTemp176.get(nativeStatementTemp177);}());const auto nativeStatementTemp175=JSValue("enabled");const auto nativeStatementTemp178=JSValue(true);setReference(nativeStatementTemp174,nativeStatementTemp175,nativeStatementTemp178);return nativeStatementTemp178;}());
([&]()->JSValue{auto nativeStatementTemp181=([&]()->JSValue{const auto nativeStatementTemp183=nativeStatementTemp148;const auto nativeStatementTemp184=JSValue("view");return nativeStatementTemp183.get(nativeStatementTemp184);}());const auto nativeStatementTemp182=JSValue("fullWidth");const auto nativeStatementTemp185=fullWidth;setReference(nativeStatementTemp181,nativeStatementTemp182,nativeStatementTemp185);return nativeStatementTemp185;}());
([&]()->JSValue{auto nativeStatementTemp188=([&]()->JSValue{const auto nativeStatementTemp190=nativeStatementTemp148;const auto nativeStatementTemp191=JSValue("view");return nativeStatementTemp190.get(nativeStatementTemp191);}());const auto nativeStatementTemp189=JSValue("fullHeight");const auto nativeStatementTemp192=fullHeight;setReference(nativeStatementTemp188,nativeStatementTemp189,nativeStatementTemp192);return nativeStatementTemp192;}());
([&]()->JSValue{auto nativeStatementTemp195=([&]()->JSValue{const auto nativeStatementTemp197=nativeStatementTemp148;const auto nativeStatementTemp198=JSValue("view");return nativeStatementTemp197.get(nativeStatementTemp198);}());const auto nativeStatementTemp196=JSValue("offsetX");const auto nativeStatementTemp199=x;setReference(nativeStatementTemp195,nativeStatementTemp196,nativeStatementTemp199);return nativeStatementTemp199;}());
([&]()->JSValue{auto nativeStatementTemp202=([&]()->JSValue{const auto nativeStatementTemp204=nativeStatementTemp148;const auto nativeStatementTemp205=JSValue("view");return nativeStatementTemp204.get(nativeStatementTemp205);}());const auto nativeStatementTemp203=JSValue("offsetY");const auto nativeStatementTemp206=y;setReference(nativeStatementTemp202,nativeStatementTemp203,nativeStatementTemp206);return nativeStatementTemp206;}());
([&]()->JSValue{auto nativeStatementTemp209=([&]()->JSValue{const auto nativeStatementTemp211=nativeStatementTemp148;const auto nativeStatementTemp212=JSValue("view");return nativeStatementTemp211.get(nativeStatementTemp212);}());const auto nativeStatementTemp210=JSValue("width");const auto nativeStatementTemp213=width;setReference(nativeStatementTemp209,nativeStatementTemp210,nativeStatementTemp213);return nativeStatementTemp213;}());
([&]()->JSValue{auto nativeStatementTemp216=([&]()->JSValue{const auto nativeStatementTemp218=nativeStatementTemp148;const auto nativeStatementTemp219=JSValue("view");return nativeStatementTemp218.get(nativeStatementTemp219);}());const auto nativeStatementTemp217=JSValue("height");const auto nativeStatementTemp220=height;setReference(nativeStatementTemp216,nativeStatementTemp217,nativeStatementTemp220);return nativeStatementTemp220;}());
([&]()->JSValue{auto nativeStatementTemp226=nativeStatementTemp148;const auto nativeStatementTemp227=JSValue("updateProjectionMatrix");const auto nativeStatementTemp223=nativeStatementTemp226.get(nativeStatementTemp227);const auto nativeStatementTemp224=nativeStatementTemp226;Args nativeStatementTemp225;return nativeStatementTemp223.call(nativeStatementTemp224,nativeStatementTemp225);}());
return JSValue{};
}

JSValue method3(const JSValue& nativeStatementTemp228,const Args& nativeStatementTemp229,const Args& nativeStatementTemp230){
auto nativeStatementTemp231=nativeStatementTemp230[0];auto nativeStatementTemp232=nativeStatementTemp228;
if(([&]()->JSValue{const auto nativeStatementTemp234=([&]()->JSValue{const auto nativeStatementTemp236=nativeStatementTemp232;const auto nativeStatementTemp237=JSValue("view");return nativeStatementTemp236.get(nativeStatementTemp237);}());const auto nativeStatementTemp235=JSValue(nullptr);return binary("!==",nativeStatementTemp234,nativeStatementTemp235);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp238=([&]()->JSValue{const auto nativeStatementTemp240=nativeStatementTemp232;const auto nativeStatementTemp241=JSValue("view");return nativeStatementTemp240.get(nativeStatementTemp241);}());const auto nativeStatementTemp239=JSValue("enabled");const auto nativeStatementTemp242=JSValue(false);setReference(nativeStatementTemp238,nativeStatementTemp239,nativeStatementTemp242);return nativeStatementTemp242;}());

}
([&]()->JSValue{auto nativeStatementTemp248=nativeStatementTemp232;const auto nativeStatementTemp249=JSValue("updateProjectionMatrix");const auto nativeStatementTemp245=nativeStatementTemp248.get(nativeStatementTemp249);const auto nativeStatementTemp246=nativeStatementTemp248;Args nativeStatementTemp247;return nativeStatementTemp245.call(nativeStatementTemp246,nativeStatementTemp247);}());
return JSValue{};
}

JSValue method4(const JSValue& nativeStatementTemp250,const Args& nativeStatementTemp251,const Args& nativeStatementTemp252){
auto nativeStatementTemp253=nativeStatementTemp252[0];auto nativeStatementTemp254=nativeStatementTemp250;
JSValue dx=([&]()->JSValue{const auto nativeStatementTemp256=([&]()->JSValue{const auto nativeStatementTemp258=([&]()->JSValue{const auto nativeStatementTemp260=nativeStatementTemp254;const auto nativeStatementTemp261=JSValue("right");return nativeStatementTemp260.get(nativeStatementTemp261);}());const auto nativeStatementTemp259=([&]()->JSValue{const auto nativeStatementTemp262=nativeStatementTemp254;const auto nativeStatementTemp263=JSValue("left");return nativeStatementTemp262.get(nativeStatementTemp263);}());return binary("-",nativeStatementTemp258,nativeStatementTemp259);}());const auto nativeStatementTemp257=([&]()->JSValue{const auto nativeStatementTemp264=JSValue(2);const auto nativeStatementTemp265=([&]()->JSValue{const auto nativeStatementTemp266=nativeStatementTemp254;const auto nativeStatementTemp267=JSValue("zoom");return nativeStatementTemp266.get(nativeStatementTemp267);}());return binary("*",nativeStatementTemp264,nativeStatementTemp265);}());return binary("/",nativeStatementTemp256,nativeStatementTemp257);}());
JSValue dy=([&]()->JSValue{const auto nativeStatementTemp268=([&]()->JSValue{const auto nativeStatementTemp270=([&]()->JSValue{const auto nativeStatementTemp272=nativeStatementTemp254;const auto nativeStatementTemp273=JSValue("top");return nativeStatementTemp272.get(nativeStatementTemp273);}());const auto nativeStatementTemp271=([&]()->JSValue{const auto nativeStatementTemp274=nativeStatementTemp254;const auto nativeStatementTemp275=JSValue("bottom");return nativeStatementTemp274.get(nativeStatementTemp275);}());return binary("-",nativeStatementTemp270,nativeStatementTemp271);}());const auto nativeStatementTemp269=([&]()->JSValue{const auto nativeStatementTemp276=JSValue(2);const auto nativeStatementTemp277=([&]()->JSValue{const auto nativeStatementTemp278=nativeStatementTemp254;const auto nativeStatementTemp279=JSValue("zoom");return nativeStatementTemp278.get(nativeStatementTemp279);}());return binary("*",nativeStatementTemp276,nativeStatementTemp277);}());return binary("/",nativeStatementTemp268,nativeStatementTemp269);}());
JSValue cx=([&]()->JSValue{const auto nativeStatementTemp280=([&]()->JSValue{const auto nativeStatementTemp282=([&]()->JSValue{const auto nativeStatementTemp284=nativeStatementTemp254;const auto nativeStatementTemp285=JSValue("right");return nativeStatementTemp284.get(nativeStatementTemp285);}());const auto nativeStatementTemp283=([&]()->JSValue{const auto nativeStatementTemp286=nativeStatementTemp254;const auto nativeStatementTemp287=JSValue("left");return nativeStatementTemp286.get(nativeStatementTemp287);}());return binary("+",nativeStatementTemp282,nativeStatementTemp283);}());const auto nativeStatementTemp281=JSValue(2);return binary("/",nativeStatementTemp280,nativeStatementTemp281);}());
JSValue cy=([&]()->JSValue{const auto nativeStatementTemp288=([&]()->JSValue{const auto nativeStatementTemp290=([&]()->JSValue{const auto nativeStatementTemp292=nativeStatementTemp254;const auto nativeStatementTemp293=JSValue("top");return nativeStatementTemp292.get(nativeStatementTemp293);}());const auto nativeStatementTemp291=([&]()->JSValue{const auto nativeStatementTemp294=nativeStatementTemp254;const auto nativeStatementTemp295=JSValue("bottom");return nativeStatementTemp294.get(nativeStatementTemp295);}());return binary("+",nativeStatementTemp290,nativeStatementTemp291);}());const auto nativeStatementTemp289=JSValue(2);return binary("/",nativeStatementTemp288,nativeStatementTemp289);}());
JSValue left=([&]()->JSValue{const auto nativeStatementTemp296=cx;const auto nativeStatementTemp297=dx;return binary("-",nativeStatementTemp296,nativeStatementTemp297);}());
JSValue right=([&]()->JSValue{const auto nativeStatementTemp298=cx;const auto nativeStatementTemp299=dx;return binary("+",nativeStatementTemp298,nativeStatementTemp299);}());
JSValue top=([&]()->JSValue{const auto nativeStatementTemp300=cy;const auto nativeStatementTemp301=dy;return binary("+",nativeStatementTemp300,nativeStatementTemp301);}());
JSValue bottom=([&]()->JSValue{const auto nativeStatementTemp302=cy;const auto nativeStatementTemp303=dy;return binary("-",nativeStatementTemp302,nativeStatementTemp303);}());
if(([&]()->JSValue{const auto nativeStatementTemp304=([&]()->JSValue{const auto nativeStatementTemp305=([&]()->JSValue{const auto nativeStatementTemp307=nativeStatementTemp254;const auto nativeStatementTemp308=JSValue("view");return nativeStatementTemp307.get(nativeStatementTemp308);}());const auto nativeStatementTemp306=JSValue(nullptr);return binary("!==",nativeStatementTemp305,nativeStatementTemp306);}());if(!nativeStatementTemp304.toBoolean())return nativeStatementTemp304;return ([&]()->JSValue{const auto nativeStatementTemp309=([&]()->JSValue{const auto nativeStatementTemp311=nativeStatementTemp254;const auto nativeStatementTemp312=JSValue("view");return nativeStatementTemp311.get(nativeStatementTemp312);}());const auto nativeStatementTemp310=JSValue("enabled");return nativeStatementTemp309.get(nativeStatementTemp310);}());}()).toBoolean()){
JSValue scaleW=([&]()->JSValue{const auto nativeStatementTemp313=([&]()->JSValue{const auto nativeStatementTemp315=([&]()->JSValue{const auto nativeStatementTemp317=([&]()->JSValue{const auto nativeStatementTemp319=nativeStatementTemp254;const auto nativeStatementTemp320=JSValue("right");return nativeStatementTemp319.get(nativeStatementTemp320);}());const auto nativeStatementTemp318=([&]()->JSValue{const auto nativeStatementTemp321=nativeStatementTemp254;const auto nativeStatementTemp322=JSValue("left");return nativeStatementTemp321.get(nativeStatementTemp322);}());return binary("-",nativeStatementTemp317,nativeStatementTemp318);}());const auto nativeStatementTemp316=([&]()->JSValue{const auto nativeStatementTemp323=([&]()->JSValue{const auto nativeStatementTemp325=nativeStatementTemp254;const auto nativeStatementTemp326=JSValue("view");return nativeStatementTemp325.get(nativeStatementTemp326);}());const auto nativeStatementTemp324=JSValue("fullWidth");return nativeStatementTemp323.get(nativeStatementTemp324);}());return binary("/",nativeStatementTemp315,nativeStatementTemp316);}());const auto nativeStatementTemp314=([&]()->JSValue{const auto nativeStatementTemp327=nativeStatementTemp254;const auto nativeStatementTemp328=JSValue("zoom");return nativeStatementTemp327.get(nativeStatementTemp328);}());return binary("/",nativeStatementTemp313,nativeStatementTemp314);}());
JSValue scaleH=([&]()->JSValue{const auto nativeStatementTemp329=([&]()->JSValue{const auto nativeStatementTemp331=([&]()->JSValue{const auto nativeStatementTemp333=([&]()->JSValue{const auto nativeStatementTemp335=nativeStatementTemp254;const auto nativeStatementTemp336=JSValue("top");return nativeStatementTemp335.get(nativeStatementTemp336);}());const auto nativeStatementTemp334=([&]()->JSValue{const auto nativeStatementTemp337=nativeStatementTemp254;const auto nativeStatementTemp338=JSValue("bottom");return nativeStatementTemp337.get(nativeStatementTemp338);}());return binary("-",nativeStatementTemp333,nativeStatementTemp334);}());const auto nativeStatementTemp332=([&]()->JSValue{const auto nativeStatementTemp339=([&]()->JSValue{const auto nativeStatementTemp341=nativeStatementTemp254;const auto nativeStatementTemp342=JSValue("view");return nativeStatementTemp341.get(nativeStatementTemp342);}());const auto nativeStatementTemp340=JSValue("fullHeight");return nativeStatementTemp339.get(nativeStatementTemp340);}());return binary("/",nativeStatementTemp331,nativeStatementTemp332);}());const auto nativeStatementTemp330=([&]()->JSValue{const auto nativeStatementTemp343=nativeStatementTemp254;const auto nativeStatementTemp344=JSValue("zoom");return nativeStatementTemp343.get(nativeStatementTemp344);}());return binary("/",nativeStatementTemp329,nativeStatementTemp330);}());
([&]()->JSValue{const auto nativeStatementTemp346=left;const auto nativeStatementTemp347=([&]()->JSValue{const auto nativeStatementTemp348=scaleW;const auto nativeStatementTemp349=([&]()->JSValue{const auto nativeStatementTemp350=([&]()->JSValue{const auto nativeStatementTemp352=nativeStatementTemp254;const auto nativeStatementTemp353=JSValue("view");return nativeStatementTemp352.get(nativeStatementTemp353);}());const auto nativeStatementTemp351=JSValue("offsetX");return nativeStatementTemp350.get(nativeStatementTemp351);}());return binary("*",nativeStatementTemp348,nativeStatementTemp349);}());const auto nativeStatementTemp345=binary("+",nativeStatementTemp346,nativeStatementTemp347);left=nativeStatementTemp345;return nativeStatementTemp345;}());
([&]()->JSValue{const auto nativeStatementTemp354=([&]()->JSValue{const auto nativeStatementTemp357=left;const auto nativeStatementTemp358=([&]()->JSValue{const auto nativeStatementTemp359=scaleW;const auto nativeStatementTemp360=([&]()->JSValue{const auto nativeStatementTemp361=([&]()->JSValue{const auto nativeStatementTemp363=nativeStatementTemp254;const auto nativeStatementTemp364=JSValue("view");return nativeStatementTemp363.get(nativeStatementTemp364);}());const auto nativeStatementTemp362=JSValue("width");return nativeStatementTemp361.get(nativeStatementTemp362);}());return binary("*",nativeStatementTemp359,nativeStatementTemp360);}());return binary("+",nativeStatementTemp357,nativeStatementTemp358);}());right=nativeStatementTemp354;return nativeStatementTemp354;}());
([&]()->JSValue{const auto nativeStatementTemp366=top;const auto nativeStatementTemp367=([&]()->JSValue{const auto nativeStatementTemp368=scaleH;const auto nativeStatementTemp369=([&]()->JSValue{const auto nativeStatementTemp370=([&]()->JSValue{const auto nativeStatementTemp372=nativeStatementTemp254;const auto nativeStatementTemp373=JSValue("view");return nativeStatementTemp372.get(nativeStatementTemp373);}());const auto nativeStatementTemp371=JSValue("offsetY");return nativeStatementTemp370.get(nativeStatementTemp371);}());return binary("*",nativeStatementTemp368,nativeStatementTemp369);}());const auto nativeStatementTemp365=binary("-",nativeStatementTemp366,nativeStatementTemp367);top=nativeStatementTemp365;return nativeStatementTemp365;}());
([&]()->JSValue{const auto nativeStatementTemp374=([&]()->JSValue{const auto nativeStatementTemp377=top;const auto nativeStatementTemp378=([&]()->JSValue{const auto nativeStatementTemp379=scaleH;const auto nativeStatementTemp380=([&]()->JSValue{const auto nativeStatementTemp381=([&]()->JSValue{const auto nativeStatementTemp383=nativeStatementTemp254;const auto nativeStatementTemp384=JSValue("view");return nativeStatementTemp383.get(nativeStatementTemp384);}());const auto nativeStatementTemp382=JSValue("height");return nativeStatementTemp381.get(nativeStatementTemp382);}());return binary("*",nativeStatementTemp379,nativeStatementTemp380);}());return binary("-",nativeStatementTemp377,nativeStatementTemp378);}());bottom=nativeStatementTemp374;return nativeStatementTemp374;}());

}
([&]()->JSValue{auto nativeStatementTemp388=([&]()->JSValue{const auto nativeStatementTemp390=nativeStatementTemp254;const auto nativeStatementTemp391=JSValue("projectionMatrix");return nativeStatementTemp390.get(nativeStatementTemp391);}());const auto nativeStatementTemp389=JSValue("makeOrthographic");const auto nativeStatementTemp385=nativeStatementTemp388.get(nativeStatementTemp389);const auto nativeStatementTemp386=nativeStatementTemp388;Args nativeStatementTemp387;nativeStatementTemp387.push_back(left);nativeStatementTemp387.push_back(right);nativeStatementTemp387.push_back(top);nativeStatementTemp387.push_back(bottom);nativeStatementTemp387.push_back(([&]()->JSValue{const auto nativeStatementTemp392=nativeStatementTemp254;const auto nativeStatementTemp393=JSValue("near");return nativeStatementTemp392.get(nativeStatementTemp393);}()));nativeStatementTemp387.push_back(([&]()->JSValue{const auto nativeStatementTemp394=nativeStatementTemp254;const auto nativeStatementTemp395=JSValue("far");return nativeStatementTemp394.get(nativeStatementTemp395);}()));nativeStatementTemp387.push_back(([&]()->JSValue{const auto nativeStatementTemp396=nativeStatementTemp254;const auto nativeStatementTemp397=JSValue("coordinateSystem");return nativeStatementTemp396.get(nativeStatementTemp397);}()));nativeStatementTemp387.push_back(([&]()->JSValue{const auto nativeStatementTemp398=nativeStatementTemp254;const auto nativeStatementTemp399=JSValue("reversedDepth");return nativeStatementTemp398.get(nativeStatementTemp399);}()));return nativeStatementTemp385.call(nativeStatementTemp386,nativeStatementTemp387);}());
([&]()->JSValue{auto nativeStatementTemp403=([&]()->JSValue{auto nativeStatementTemp408=([&]()->JSValue{const auto nativeStatementTemp410=nativeStatementTemp254;const auto nativeStatementTemp411=JSValue("projectionMatrixInverse");return nativeStatementTemp410.get(nativeStatementTemp411);}());const auto nativeStatementTemp409=JSValue("copy");const auto nativeStatementTemp405=nativeStatementTemp408.get(nativeStatementTemp409);const auto nativeStatementTemp406=nativeStatementTemp408;Args nativeStatementTemp407;nativeStatementTemp407.push_back(([&]()->JSValue{const auto nativeStatementTemp412=nativeStatementTemp254;const auto nativeStatementTemp413=JSValue("projectionMatrix");return nativeStatementTemp412.get(nativeStatementTemp413);}()));return nativeStatementTemp405.call(nativeStatementTemp406,nativeStatementTemp407);}());const auto nativeStatementTemp404=JSValue("invert");const auto nativeStatementTemp400=nativeStatementTemp403.get(nativeStatementTemp404);const auto nativeStatementTemp401=nativeStatementTemp403;Args nativeStatementTemp402;return nativeStatementTemp400.call(nativeStatementTemp401,nativeStatementTemp402);}());
return JSValue{};
}

JSValue method5(const JSValue& nativeStatementTemp414,const Args& nativeStatementTemp415,const Args& nativeStatementTemp416){
auto nativeStatementTemp417=nativeStatementTemp416[0];auto nativeStatementTemp418=nativeStatementTemp414;
JSValue meta=arg(nativeStatementTemp415,0);
JSValue data=([&]()->JSValue{const auto nativeStatementTemp423=nativeStatementTemp417.get("_home").getPrototype();const auto nativeStatementTemp424=JSValue("toJSON");const auto nativeStatementTemp420=nativeStatementTemp423.get(nativeStatementTemp424,nativeStatementTemp418);const auto nativeStatementTemp421=nativeStatementTemp418;Args nativeStatementTemp422;nativeStatementTemp422.push_back(meta);return nativeStatementTemp420.call(nativeStatementTemp421,nativeStatementTemp422);}());
([&]()->JSValue{auto nativeStatementTemp425=([&]()->JSValue{const auto nativeStatementTemp427=data;const auto nativeStatementTemp428=JSValue("object");return nativeStatementTemp427.get(nativeStatementTemp428);}());const auto nativeStatementTemp426=JSValue("zoom");const auto nativeStatementTemp429=([&]()->JSValue{const auto nativeStatementTemp432=nativeStatementTemp418;const auto nativeStatementTemp433=JSValue("zoom");return nativeStatementTemp432.get(nativeStatementTemp433);}());setReference(nativeStatementTemp425,nativeStatementTemp426,nativeStatementTemp429);return nativeStatementTemp429;}());
([&]()->JSValue{auto nativeStatementTemp434=([&]()->JSValue{const auto nativeStatementTemp436=data;const auto nativeStatementTemp437=JSValue("object");return nativeStatementTemp436.get(nativeStatementTemp437);}());const auto nativeStatementTemp435=JSValue("left");const auto nativeStatementTemp438=([&]()->JSValue{const auto nativeStatementTemp441=nativeStatementTemp418;const auto nativeStatementTemp442=JSValue("left");return nativeStatementTemp441.get(nativeStatementTemp442);}());setReference(nativeStatementTemp434,nativeStatementTemp435,nativeStatementTemp438);return nativeStatementTemp438;}());
([&]()->JSValue{auto nativeStatementTemp443=([&]()->JSValue{const auto nativeStatementTemp445=data;const auto nativeStatementTemp446=JSValue("object");return nativeStatementTemp445.get(nativeStatementTemp446);}());const auto nativeStatementTemp444=JSValue("right");const auto nativeStatementTemp447=([&]()->JSValue{const auto nativeStatementTemp450=nativeStatementTemp418;const auto nativeStatementTemp451=JSValue("right");return nativeStatementTemp450.get(nativeStatementTemp451);}());setReference(nativeStatementTemp443,nativeStatementTemp444,nativeStatementTemp447);return nativeStatementTemp447;}());
([&]()->JSValue{auto nativeStatementTemp452=([&]()->JSValue{const auto nativeStatementTemp454=data;const auto nativeStatementTemp455=JSValue("object");return nativeStatementTemp454.get(nativeStatementTemp455);}());const auto nativeStatementTemp453=JSValue("top");const auto nativeStatementTemp456=([&]()->JSValue{const auto nativeStatementTemp459=nativeStatementTemp418;const auto nativeStatementTemp460=JSValue("top");return nativeStatementTemp459.get(nativeStatementTemp460);}());setReference(nativeStatementTemp452,nativeStatementTemp453,nativeStatementTemp456);return nativeStatementTemp456;}());
([&]()->JSValue{auto nativeStatementTemp461=([&]()->JSValue{const auto nativeStatementTemp463=data;const auto nativeStatementTemp464=JSValue("object");return nativeStatementTemp463.get(nativeStatementTemp464);}());const auto nativeStatementTemp462=JSValue("bottom");const auto nativeStatementTemp465=([&]()->JSValue{const auto nativeStatementTemp468=nativeStatementTemp418;const auto nativeStatementTemp469=JSValue("bottom");return nativeStatementTemp468.get(nativeStatementTemp469);}());setReference(nativeStatementTemp461,nativeStatementTemp462,nativeStatementTemp465);return nativeStatementTemp465;}());
([&]()->JSValue{auto nativeStatementTemp470=([&]()->JSValue{const auto nativeStatementTemp472=data;const auto nativeStatementTemp473=JSValue("object");return nativeStatementTemp472.get(nativeStatementTemp473);}());const auto nativeStatementTemp471=JSValue("near");const auto nativeStatementTemp474=([&]()->JSValue{const auto nativeStatementTemp477=nativeStatementTemp418;const auto nativeStatementTemp478=JSValue("near");return nativeStatementTemp477.get(nativeStatementTemp478);}());setReference(nativeStatementTemp470,nativeStatementTemp471,nativeStatementTemp474);return nativeStatementTemp474;}());
([&]()->JSValue{auto nativeStatementTemp479=([&]()->JSValue{const auto nativeStatementTemp481=data;const auto nativeStatementTemp482=JSValue("object");return nativeStatementTemp481.get(nativeStatementTemp482);}());const auto nativeStatementTemp480=JSValue("far");const auto nativeStatementTemp483=([&]()->JSValue{const auto nativeStatementTemp486=nativeStatementTemp418;const auto nativeStatementTemp487=JSValue("far");return nativeStatementTemp486.get(nativeStatementTemp487);}());setReference(nativeStatementTemp479,nativeStatementTemp480,nativeStatementTemp483);return nativeStatementTemp483;}());
if(([&]()->JSValue{const auto nativeStatementTemp488=([&]()->JSValue{const auto nativeStatementTemp490=nativeStatementTemp418;const auto nativeStatementTemp491=JSValue("view");return nativeStatementTemp490.get(nativeStatementTemp491);}());const auto nativeStatementTemp489=JSValue(nullptr);return binary("!==",nativeStatementTemp488,nativeStatementTemp489);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp492=([&]()->JSValue{const auto nativeStatementTemp494=data;const auto nativeStatementTemp495=JSValue("object");return nativeStatementTemp494.get(nativeStatementTemp495);}());const auto nativeStatementTemp493=JSValue("view");const auto nativeStatementTemp496=([&]()->JSValue{auto nativeStatementTemp502=nativeGlobalBinding("Object");const auto nativeStatementTemp503=JSValue("assign");const auto nativeStatementTemp499=nativeStatementTemp502.get(nativeStatementTemp503);const auto nativeStatementTemp500=nativeStatementTemp502;Args nativeStatementTemp501;nativeStatementTemp501.push_back(([&]()->JSValue{auto nativeStatementTemp504=JSValue::object();return nativeStatementTemp504;}()));nativeStatementTemp501.push_back(([&]()->JSValue{const auto nativeStatementTemp505=nativeStatementTemp418;const auto nativeStatementTemp506=JSValue("view");return nativeStatementTemp505.get(nativeStatementTemp506);}()));return nativeStatementTemp499.call(nativeStatementTemp500,nativeStatementTemp501);}());setReference(nativeStatementTemp492,nativeStatementTemp493,nativeStatementTemp496);return nativeStatementTemp496;}());
return data;
return JSValue{};
}
}
JSValue nativeOrthographicCameraModule(){using namespace OrthographicCameraManaged;static thread_local JSWeakValue weak;if(const auto existing=weak.lock();existing&&existing->isObject())return *existing;static thread_local bool initializing=false;JSModuleInitializationScope initialization(initializing,"OrthographicCamera module");auto module=JSValue::object(nullptr);
own(module,"Camera",nativeCameraConstructor());
auto ctor=JSValue::builtinConstructor(method0,{module,JSValue{}},"OrthographicCamera",0);ctor.setFunctionSourceText(OrthographicCameraRuntimeSource::classSource);auto prototype=ctor.get("prototype");
ctor.setPrototype(module.get("Camera"));prototype.setPrototype(module.get("Camera").get("prototype"));
own(module,"OrthographicCamera",ctor);own(module,"_home",prototype);
{auto fn=JSValue::function(method1,{module,prototype},"copy",2);fn.setFunctionSourceText(OrthographicCameraRuntimeSource::method1);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("copy",d);}
{auto fn=JSValue::function(method2,{module,prototype},"setViewOffset",6);fn.setFunctionSourceText(OrthographicCameraRuntimeSource::method2);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("setViewOffset",d);}
{auto fn=JSValue::function(method3,{module,prototype},"clearViewOffset",0);fn.setFunctionSourceText(OrthographicCameraRuntimeSource::method3);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("clearViewOffset",d);}
{auto fn=JSValue::function(method4,{module,prototype},"updateProjectionMatrix",0);fn.setFunctionSourceText(OrthographicCameraRuntimeSource::method4);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("updateProjectionMatrix",d);}
{auto fn=JSValue::function(method5,{module,prototype},"toJSON",1);fn.setFunctionSourceText(OrthographicCameraRuntimeSource::method5);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("toJSON",d);}
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto exports=JSValue::object();own(exports,"OrthographicCamera",module.get("OrthographicCamera"));exports.retainForHeapLifetime();weak=JSWeakValue(exports);return exports;}
JSValue nativeOrthographicCameraBinding(const std::string& name){const auto keeper=nativeOrthographicCameraModule();return OrthographicCameraManaged::lexical().lock()->get(name);}
JSValue nativeOrthographicCameraConstructor(){return nativeOrthographicCameraBinding("OrthographicCamera");}
