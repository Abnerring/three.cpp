#include "math/Triangle.h"
#include "core/VertexAttribute.h"
#include "math/Box3.h"
#include "math/Plane.h"
#include <cmath>

Vector3& Triangle::getNormal(const Vector3& a, const Vector3& b, const Vector3& c,
                             Vector3& target) {
    Vector3 _v0;
    target.subVectors(c, b);
    _v0.subVectors(a, b);
    target.cross(_v0);
    const auto targetLengthSq = target.lengthSq();
    if (targetLengthSq > 0) {
        return target.multiplyScalar(1 / std::sqrt(targetLengthSq));
    }
    return target.set(0, 0, 0);
}

Vector3* Triangle::getBarycoord(const Vector3& point, const Vector3& a, const Vector3& b,
                                const Vector3& c, Vector3& target) {
    Vector3 _v0, _v1, _v2;
    _v0.subVectors(c, a);
    _v1.subVectors(b, a);
    _v2.subVectors(point, a);
    const auto dot00 = _v0.dot(_v0);
    const auto dot01 = _v0.dot(_v1);
    const auto dot02 = _v0.dot(_v2);
    const auto dot11 = _v1.dot(_v1);
    const auto dot12 = _v1.dot(_v2);
    const auto denom = (dot00 * dot11 - dot01 * dot01);
    if (denom == 0) {
        target.set(0, 0, 0);
        return nullptr;
    }
    const auto invDenom = 1 / denom;
    const auto u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    const auto v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    return &target.set(1 - u - v, v, u);
}

bool Triangle::containsPoint(const Vector3& point, const Vector3& a, const Vector3& b,
                             const Vector3& c) {
    Vector3 _v3;
    if (Triangle::getBarycoord(point, a, b, c, _v3) == nullptr) {
        return false;
    }
    return (_v3.x >= 0) && (_v3.y >= 0) && ((_v3.x + _v3.y) <= 1);
}

bool Triangle::isFrontFacing(const Vector3& a, const Vector3& b, const Vector3& c,
                             const Vector3& direction) {
    Vector3 _v0, _v1;
    _v0.subVectors(c, b);
    _v1.subVectors(a, b);
    return _v0.cross(_v1).dot(direction) < 0;
}

Triangle& Triangle::set(const Vector3& a, const Vector3& b, const Vector3& c) {
    this->a.copy(a);
    this->b.copy(b);
    this->c.copy(c);
    return *this;
}

Triangle& Triangle::setFromAttributeAndIndices(const VertexAttribute& attribute, size_t i0,
                                               size_t i1, size_t i2) {
    this->a.fromBufferAttribute(attribute, i0);
    this->b.fromBufferAttribute(attribute, i1);
    this->c.fromBufferAttribute(attribute, i2);
    return *this;
}

Triangle& Triangle::copy(const Triangle& triangle) {
    this->a.copy(triangle.a);
    this->b.copy(triangle.b);
    this->c.copy(triangle.c);
    return *this;
}

HIGH_PRECISION Triangle::getArea() const {
    Vector3 _v0, _v1;
    _v0.subVectors(this->c, this->b);
    _v1.subVectors(this->a, this->b);
    return _v0.cross(_v1).length() * 0.5;
}

Vector3& Triangle::getMidpoint(Vector3& target) const {
    return target.addVectors(this->a, this->b).add(this->c).multiplyScalar(1.0 / 3);
}

Vector3& Triangle::getNormal(Vector3& target) const {
    return Triangle::getNormal(this->a, this->b, this->c, target);
}

Plane& Triangle::getPlane(Plane& target) const {
    return target.setFromCoplanarPoints(this->a, this->b, this->c);
}

Vector3* Triangle::getBarycoord(const Vector3& point, Vector3& target) const {
    return Triangle::getBarycoord(point, this->a, this->b, this->c, target);
}

bool Triangle::containsPoint(const Vector3& point) const {
    return Triangle::containsPoint(point, this->a, this->b, this->c);
}

bool Triangle::isFrontFacing(const Vector3& direction) const {
    return Triangle::isFrontFacing(this->a, this->b, this->c, direction);
}

bool Triangle::intersectsBox(const Box3& box) const {
    return box.intersectsTriangle(*this);
}

Vector3& Triangle::closestPointToPoint(const Vector3& p, Vector3& target) const {
    Vector3 _vab, _vac, _vbc, _vap, _vbp, _vcp;
    const auto& a = this->a;
    const auto& b = this->b;
    const auto& c = this->c;
    HIGH_PRECISION v, w;
    _vab.subVectors(b, a);
    _vac.subVectors(c, a);
    _vap.subVectors(p, a);
    const auto d1 = _vab.dot(_vap);
    const auto d2 = _vac.dot(_vap);
    if (d1 <= 0 && d2 <= 0) {
        return target.copy(a);
    }
    _vbp.subVectors(p, b);
    const auto d3 = _vab.dot(_vbp);
    const auto d4 = _vac.dot(_vbp);
    if (d3 >= 0 && d4 <= d3) {
        return target.copy(b);
    }
    const auto vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0 && d1 - d3 > 0) {
        v = d1 / (d1 - d3);
        return target.copy(a).addScaledVector(_vab, v);
    }
    _vcp.subVectors(p, c);
    const auto d5 = _vab.dot(_vcp);
    const auto d6 = _vac.dot(_vcp);
    if (d6 >= 0 && d5 <= d6) {
        return target.copy(c);
    }
    const auto vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) {
        w = d2 / (d2 - d6);
        return target.copy(a).addScaledVector(_vac, w);
    }
    const auto va = d3 * d6 - d5 * d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
        _vbc.subVectors(c, b);
        w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return target.copy(b).addScaledVector(_vbc, w);
    }
    const auto denom = 1 / (va + vb + vc);
    v = vb * denom;
    w = vc * denom;
    return target.copy(a).addScaledVector(_vab, v).addScaledVector(_vac, w);
}

bool Triangle::equals(const Triangle& triangle) const {
    return triangle.a.equals(this->a) && triangle.b.equals(this->b) && triangle.c.equals(this->c);
}

Triangle::Triangle(const Vector3& a, const Vector3& b, const Vector3& c) : a(a), b(b), c(c) {}
Triangle Triangle::clone() const {
    return Triangle().copy(*this);
}
Triangle& Triangle::setFromPointsAndIndices(const std::vector<Vector3>& points, size_t i0,
                                            size_t i1, size_t i2) {
    a.copy(points.at(i0));
    b.copy(points.at(i1));
    c.copy(points.at(i2));
    return *this;
}
namespace {
template <class V> V attributeVector(const Vector4& source) {
    V value;
    value.setX(source.x).setY(source.y);
    if constexpr (requires { value.setZ(0); })
        value.setZ(source.z);
    if constexpr (requires { value.setW(0); })
        value.setW(source.w);
    return value;
}
template <class V>
V& interpolateAttribute(const VertexAttribute& attr, size_t i1, size_t i2, size_t i3,
                        const Vector3& barycoord, V& target) {
    Vector4 v40, v41, v42;
    v40.setScalar(0);
    v41.setScalar(0);
    v42.setScalar(0);
    v40.fromBufferAttribute(attr, i1);
    v41.fromBufferAttribute(attr, i2);
    v42.fromBufferAttribute(attr, i3);
    target.setScalar(0);
    target.addScaledVector(attributeVector<V>(v40), barycoord.x);
    target.addScaledVector(attributeVector<V>(v41), barycoord.y);
    target.addScaledVector(attributeVector<V>(v42), barycoord.z);
    return target;
}
} // namespace
Vector2& Triangle::getInterpolatedAttribute(const VertexAttribute& attr, size_t i1, size_t i2,
                                            size_t i3, const Vector3& barycoord, Vector2& target) {
    return interpolateAttribute(attr, i1, i2, i3, barycoord, target);
}
Vector3& Triangle::getInterpolatedAttribute(const VertexAttribute& attr, size_t i1, size_t i2,
                                            size_t i3, const Vector3& barycoord, Vector3& target) {
    return interpolateAttribute(attr, i1, i2, i3, barycoord, target);
}
Vector4& Triangle::getInterpolatedAttribute(const VertexAttribute& attr, size_t i1, size_t i2,
                                            size_t i3, const Vector3& barycoord, Vector4& target) {
    return interpolateAttribute(attr, i1, i2, i3, barycoord, target);
}

// Complete native managed Triangle source class; typed implementation above remains intact.
#include "core/JSGlobal.h"
#include "core/JSModuleObject.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "math/JSNumberMath.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Ray.h"
#include "math/Triangle.h"
#include "core/Object3D.h"
#include "core/BufferGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "constants.h"
#include <string_view>
#include <unordered_set>
namespace TriangleManaged {

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

JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
JSValue a=arg(nativeStatementTemp1,0);
if(a.isUndefined())a=([&]()->JSValue{const auto nativeStatementTemp6=nativeStatementTemp3.get("Vector3");Args nativeStatementTemp7;return nativeStatementTemp6.construct(nativeStatementTemp7);}());
JSValue b=arg(nativeStatementTemp1,1);
if(b.isUndefined())b=([&]()->JSValue{const auto nativeStatementTemp8=nativeStatementTemp3.get("Vector3");Args nativeStatementTemp9;return nativeStatementTemp8.construct(nativeStatementTemp9);}());
JSValue c=arg(nativeStatementTemp1,2);
if(c.isUndefined())c=([&]()->JSValue{const auto nativeStatementTemp10=nativeStatementTemp3.get("Vector3");Args nativeStatementTemp11;return nativeStatementTemp10.construct(nativeStatementTemp11);}());
([&]()->JSValue{auto nativeStatementTemp12=nativeStatementTemp4;const auto nativeStatementTemp13=JSValue("a");const auto nativeStatementTemp14=a;setReference(nativeStatementTemp12,nativeStatementTemp13,nativeStatementTemp14);return nativeStatementTemp14;}());
([&]()->JSValue{auto nativeStatementTemp17=nativeStatementTemp4;const auto nativeStatementTemp18=JSValue("b");const auto nativeStatementTemp19=b;setReference(nativeStatementTemp17,nativeStatementTemp18,nativeStatementTemp19);return nativeStatementTemp19;}());
([&]()->JSValue{auto nativeStatementTemp22=nativeStatementTemp4;const auto nativeStatementTemp23=JSValue("c");const auto nativeStatementTemp24=c;setReference(nativeStatementTemp22,nativeStatementTemp23,nativeStatementTemp24);return nativeStatementTemp24;}());
return JSValue{};
}

JSValue method1(const JSValue& nativeStatementTemp27,const Args& nativeStatementTemp28,const Args& nativeStatementTemp29){
auto nativeStatementTemp30=nativeStatementTemp29[0];auto nativeStatementTemp31=nativeStatementTemp27;
JSValue a=arg(nativeStatementTemp28,0);
JSValue b=arg(nativeStatementTemp28,1);
JSValue c=arg(nativeStatementTemp28,2);
JSValue target=arg(nativeStatementTemp28,3);
([&]()->JSValue{auto nativeStatementTemp36=target;const auto nativeStatementTemp37=JSValue("subVectors");const auto nativeStatementTemp33=nativeStatementTemp36.get(nativeStatementTemp37);const auto nativeStatementTemp34=nativeStatementTemp36;Args nativeStatementTemp35;nativeStatementTemp35.push_back(c);nativeStatementTemp35.push_back(b);return nativeStatementTemp33.call(nativeStatementTemp34,nativeStatementTemp35);}());
([&]()->JSValue{auto nativeStatementTemp41=nativeStatementTemp30.get("_v0");const auto nativeStatementTemp42=JSValue("subVectors");const auto nativeStatementTemp38=nativeStatementTemp41.get(nativeStatementTemp42);const auto nativeStatementTemp39=nativeStatementTemp41;Args nativeStatementTemp40;nativeStatementTemp40.push_back(a);nativeStatementTemp40.push_back(b);return nativeStatementTemp38.call(nativeStatementTemp39,nativeStatementTemp40);}());
([&]()->JSValue{auto nativeStatementTemp46=target;const auto nativeStatementTemp47=JSValue("cross");const auto nativeStatementTemp43=nativeStatementTemp46.get(nativeStatementTemp47);const auto nativeStatementTemp44=nativeStatementTemp46;Args nativeStatementTemp45;nativeStatementTemp45.push_back(nativeStatementTemp30.get("_v0"));return nativeStatementTemp43.call(nativeStatementTemp44,nativeStatementTemp45);}());
JSValue targetLengthSq=([&]()->JSValue{auto nativeStatementTemp51=target;const auto nativeStatementTemp52=JSValue("lengthSq");const auto nativeStatementTemp48=nativeStatementTemp51.get(nativeStatementTemp52);const auto nativeStatementTemp49=nativeStatementTemp51;Args nativeStatementTemp50;return nativeStatementTemp48.call(nativeStatementTemp49,nativeStatementTemp50);}());
if(([&]()->JSValue{const auto nativeStatementTemp53=targetLengthSq;const auto nativeStatementTemp54=JSValue(0);return binary(">",nativeStatementTemp53,nativeStatementTemp54);}()).toBoolean()){
return ([&]()->JSValue{auto nativeStatementTemp58=target;const auto nativeStatementTemp59=JSValue("multiplyScalar");const auto nativeStatementTemp55=nativeStatementTemp58.get(nativeStatementTemp59);const auto nativeStatementTemp56=nativeStatementTemp58;Args nativeStatementTemp57;nativeStatementTemp57.push_back(([&]()->JSValue{const auto nativeStatementTemp60=JSValue(1);const auto nativeStatementTemp61=([&]()->JSValue{auto nativeStatementTemp65=nativeGlobalBinding("Math");const auto nativeStatementTemp66=JSValue("sqrt");const auto nativeStatementTemp62=nativeStatementTemp65.get(nativeStatementTemp66);const auto nativeStatementTemp63=nativeStatementTemp65;Args nativeStatementTemp64;nativeStatementTemp64.push_back(targetLengthSq);return nativeStatementTemp62.call(nativeStatementTemp63,nativeStatementTemp64);}());return binary("/",nativeStatementTemp60,nativeStatementTemp61);}()));return nativeStatementTemp55.call(nativeStatementTemp56,nativeStatementTemp57);}());

}
return ([&]()->JSValue{auto nativeStatementTemp70=target;const auto nativeStatementTemp71=JSValue("set");const auto nativeStatementTemp67=nativeStatementTemp70.get(nativeStatementTemp71);const auto nativeStatementTemp68=nativeStatementTemp70;Args nativeStatementTemp69;nativeStatementTemp69.push_back(JSValue(0));nativeStatementTemp69.push_back(JSValue(0));nativeStatementTemp69.push_back(JSValue(0));return nativeStatementTemp67.call(nativeStatementTemp68,nativeStatementTemp69);}());
return JSValue{};
}

JSValue method2(const JSValue& nativeStatementTemp72,const Args& nativeStatementTemp73,const Args& nativeStatementTemp74){
auto nativeStatementTemp75=nativeStatementTemp74[0];auto nativeStatementTemp76=nativeStatementTemp72;
JSValue point=arg(nativeStatementTemp73,0);
JSValue a=arg(nativeStatementTemp73,1);
JSValue b=arg(nativeStatementTemp73,2);
JSValue c=arg(nativeStatementTemp73,3);
JSValue target=arg(nativeStatementTemp73,4);
([&]()->JSValue{auto nativeStatementTemp81=nativeStatementTemp75.get("_v0");const auto nativeStatementTemp82=JSValue("subVectors");const auto nativeStatementTemp78=nativeStatementTemp81.get(nativeStatementTemp82);const auto nativeStatementTemp79=nativeStatementTemp81;Args nativeStatementTemp80;nativeStatementTemp80.push_back(c);nativeStatementTemp80.push_back(a);return nativeStatementTemp78.call(nativeStatementTemp79,nativeStatementTemp80);}());
([&]()->JSValue{auto nativeStatementTemp86=nativeStatementTemp75.get("_v1");const auto nativeStatementTemp87=JSValue("subVectors");const auto nativeStatementTemp83=nativeStatementTemp86.get(nativeStatementTemp87);const auto nativeStatementTemp84=nativeStatementTemp86;Args nativeStatementTemp85;nativeStatementTemp85.push_back(b);nativeStatementTemp85.push_back(a);return nativeStatementTemp83.call(nativeStatementTemp84,nativeStatementTemp85);}());
([&]()->JSValue{auto nativeStatementTemp91=nativeStatementTemp75.get("_v2");const auto nativeStatementTemp92=JSValue("subVectors");const auto nativeStatementTemp88=nativeStatementTemp91.get(nativeStatementTemp92);const auto nativeStatementTemp89=nativeStatementTemp91;Args nativeStatementTemp90;nativeStatementTemp90.push_back(point);nativeStatementTemp90.push_back(a);return nativeStatementTemp88.call(nativeStatementTemp89,nativeStatementTemp90);}());
JSValue dot00=([&]()->JSValue{auto nativeStatementTemp96=nativeStatementTemp75.get("_v0");const auto nativeStatementTemp97=JSValue("dot");const auto nativeStatementTemp93=nativeStatementTemp96.get(nativeStatementTemp97);const auto nativeStatementTemp94=nativeStatementTemp96;Args nativeStatementTemp95;nativeStatementTemp95.push_back(nativeStatementTemp75.get("_v0"));return nativeStatementTemp93.call(nativeStatementTemp94,nativeStatementTemp95);}());
JSValue dot01=([&]()->JSValue{auto nativeStatementTemp101=nativeStatementTemp75.get("_v0");const auto nativeStatementTemp102=JSValue("dot");const auto nativeStatementTemp98=nativeStatementTemp101.get(nativeStatementTemp102);const auto nativeStatementTemp99=nativeStatementTemp101;Args nativeStatementTemp100;nativeStatementTemp100.push_back(nativeStatementTemp75.get("_v1"));return nativeStatementTemp98.call(nativeStatementTemp99,nativeStatementTemp100);}());
JSValue dot02=([&]()->JSValue{auto nativeStatementTemp106=nativeStatementTemp75.get("_v0");const auto nativeStatementTemp107=JSValue("dot");const auto nativeStatementTemp103=nativeStatementTemp106.get(nativeStatementTemp107);const auto nativeStatementTemp104=nativeStatementTemp106;Args nativeStatementTemp105;nativeStatementTemp105.push_back(nativeStatementTemp75.get("_v2"));return nativeStatementTemp103.call(nativeStatementTemp104,nativeStatementTemp105);}());
JSValue dot11=([&]()->JSValue{auto nativeStatementTemp111=nativeStatementTemp75.get("_v1");const auto nativeStatementTemp112=JSValue("dot");const auto nativeStatementTemp108=nativeStatementTemp111.get(nativeStatementTemp112);const auto nativeStatementTemp109=nativeStatementTemp111;Args nativeStatementTemp110;nativeStatementTemp110.push_back(nativeStatementTemp75.get("_v1"));return nativeStatementTemp108.call(nativeStatementTemp109,nativeStatementTemp110);}());
JSValue dot12=([&]()->JSValue{auto nativeStatementTemp116=nativeStatementTemp75.get("_v1");const auto nativeStatementTemp117=JSValue("dot");const auto nativeStatementTemp113=nativeStatementTemp116.get(nativeStatementTemp117);const auto nativeStatementTemp114=nativeStatementTemp116;Args nativeStatementTemp115;nativeStatementTemp115.push_back(nativeStatementTemp75.get("_v2"));return nativeStatementTemp113.call(nativeStatementTemp114,nativeStatementTemp115);}());
JSValue denom=([&]()->JSValue{const auto nativeStatementTemp118=([&]()->JSValue{const auto nativeStatementTemp120=dot00;const auto nativeStatementTemp121=dot11;return binary("*",nativeStatementTemp120,nativeStatementTemp121);}());const auto nativeStatementTemp119=([&]()->JSValue{const auto nativeStatementTemp122=dot01;const auto nativeStatementTemp123=dot01;return binary("*",nativeStatementTemp122,nativeStatementTemp123);}());return binary("-",nativeStatementTemp118,nativeStatementTemp119);}());
if(([&]()->JSValue{const auto nativeStatementTemp124=denom;const auto nativeStatementTemp125=JSValue(0);return binary("===",nativeStatementTemp124,nativeStatementTemp125);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp129=target;const auto nativeStatementTemp130=JSValue("set");const auto nativeStatementTemp126=nativeStatementTemp129.get(nativeStatementTemp130);const auto nativeStatementTemp127=nativeStatementTemp129;Args nativeStatementTemp128;nativeStatementTemp128.push_back(JSValue(0));nativeStatementTemp128.push_back(JSValue(0));nativeStatementTemp128.push_back(JSValue(0));return nativeStatementTemp126.call(nativeStatementTemp127,nativeStatementTemp128);}());
return JSValue(nullptr);

}
JSValue invDenom=([&]()->JSValue{const auto nativeStatementTemp131=JSValue(1);const auto nativeStatementTemp132=denom;return binary("/",nativeStatementTemp131,nativeStatementTemp132);}());
JSValue u=([&]()->JSValue{const auto nativeStatementTemp133=([&]()->JSValue{const auto nativeStatementTemp135=([&]()->JSValue{const auto nativeStatementTemp137=dot11;const auto nativeStatementTemp138=dot02;return binary("*",nativeStatementTemp137,nativeStatementTemp138);}());const auto nativeStatementTemp136=([&]()->JSValue{const auto nativeStatementTemp139=dot01;const auto nativeStatementTemp140=dot12;return binary("*",nativeStatementTemp139,nativeStatementTemp140);}());return binary("-",nativeStatementTemp135,nativeStatementTemp136);}());const auto nativeStatementTemp134=invDenom;return binary("*",nativeStatementTemp133,nativeStatementTemp134);}());
JSValue v=([&]()->JSValue{const auto nativeStatementTemp141=([&]()->JSValue{const auto nativeStatementTemp143=([&]()->JSValue{const auto nativeStatementTemp145=dot00;const auto nativeStatementTemp146=dot12;return binary("*",nativeStatementTemp145,nativeStatementTemp146);}());const auto nativeStatementTemp144=([&]()->JSValue{const auto nativeStatementTemp147=dot01;const auto nativeStatementTemp148=dot02;return binary("*",nativeStatementTemp147,nativeStatementTemp148);}());return binary("-",nativeStatementTemp143,nativeStatementTemp144);}());const auto nativeStatementTemp142=invDenom;return binary("*",nativeStatementTemp141,nativeStatementTemp142);}());
return ([&]()->JSValue{auto nativeStatementTemp152=target;const auto nativeStatementTemp153=JSValue("set");const auto nativeStatementTemp149=nativeStatementTemp152.get(nativeStatementTemp153);const auto nativeStatementTemp150=nativeStatementTemp152;Args nativeStatementTemp151;nativeStatementTemp151.push_back(([&]()->JSValue{const auto nativeStatementTemp154=([&]()->JSValue{const auto nativeStatementTemp156=JSValue(1);const auto nativeStatementTemp157=u;return binary("-",nativeStatementTemp156,nativeStatementTemp157);}());const auto nativeStatementTemp155=v;return binary("-",nativeStatementTemp154,nativeStatementTemp155);}()));nativeStatementTemp151.push_back(v);nativeStatementTemp151.push_back(u);return nativeStatementTemp149.call(nativeStatementTemp150,nativeStatementTemp151);}());
return JSValue{};
}

JSValue method3(const JSValue& nativeStatementTemp158,const Args& nativeStatementTemp159,const Args& nativeStatementTemp160){
auto nativeStatementTemp161=nativeStatementTemp160[0];auto nativeStatementTemp162=nativeStatementTemp158;
JSValue point=arg(nativeStatementTemp159,0);
JSValue a=arg(nativeStatementTemp159,1);
JSValue b=arg(nativeStatementTemp159,2);
JSValue c=arg(nativeStatementTemp159,3);
if(([&]()->JSValue{const auto nativeStatementTemp164=([&]()->JSValue{auto nativeStatementTemp169=nativeStatementTemp162;const auto nativeStatementTemp170=JSValue("getBarycoord");const auto nativeStatementTemp166=nativeStatementTemp169.get(nativeStatementTemp170);const auto nativeStatementTemp167=nativeStatementTemp169;Args nativeStatementTemp168;nativeStatementTemp168.push_back(point);nativeStatementTemp168.push_back(a);nativeStatementTemp168.push_back(b);nativeStatementTemp168.push_back(c);nativeStatementTemp168.push_back(nativeStatementTemp161.get("_v3"));return nativeStatementTemp166.call(nativeStatementTemp167,nativeStatementTemp168);}());const auto nativeStatementTemp165=JSValue(nullptr);return binary("===",nativeStatementTemp164,nativeStatementTemp165);}()).toBoolean()){
return JSValue(false);

}
return ([&]()->JSValue{const auto nativeStatementTemp171=([&]()->JSValue{const auto nativeStatementTemp172=([&]()->JSValue{const auto nativeStatementTemp173=([&]()->JSValue{const auto nativeStatementTemp175=nativeStatementTemp161.get("_v3");const auto nativeStatementTemp176=JSValue("x");return nativeStatementTemp175.get(nativeStatementTemp176);}());const auto nativeStatementTemp174=JSValue(0);return binary(">=",nativeStatementTemp173,nativeStatementTemp174);}());if(!nativeStatementTemp172.toBoolean())return nativeStatementTemp172;return ([&]()->JSValue{const auto nativeStatementTemp177=([&]()->JSValue{const auto nativeStatementTemp179=nativeStatementTemp161.get("_v3");const auto nativeStatementTemp180=JSValue("y");return nativeStatementTemp179.get(nativeStatementTemp180);}());const auto nativeStatementTemp178=JSValue(0);return binary(">=",nativeStatementTemp177,nativeStatementTemp178);}());}());if(!nativeStatementTemp171.toBoolean())return nativeStatementTemp171;return ([&]()->JSValue{const auto nativeStatementTemp181=([&]()->JSValue{const auto nativeStatementTemp183=([&]()->JSValue{const auto nativeStatementTemp185=nativeStatementTemp161.get("_v3");const auto nativeStatementTemp186=JSValue("x");return nativeStatementTemp185.get(nativeStatementTemp186);}());const auto nativeStatementTemp184=([&]()->JSValue{const auto nativeStatementTemp187=nativeStatementTemp161.get("_v3");const auto nativeStatementTemp188=JSValue("y");return nativeStatementTemp187.get(nativeStatementTemp188);}());return binary("+",nativeStatementTemp183,nativeStatementTemp184);}());const auto nativeStatementTemp182=JSValue(1);return binary("<=",nativeStatementTemp181,nativeStatementTemp182);}());}());
return JSValue{};
}

JSValue method4(const JSValue& nativeStatementTemp189,const Args& nativeStatementTemp190,const Args& nativeStatementTemp191){
auto nativeStatementTemp192=nativeStatementTemp191[0];auto nativeStatementTemp193=nativeStatementTemp189;
JSValue point=arg(nativeStatementTemp190,0);
JSValue p1=arg(nativeStatementTemp190,1);
JSValue p2=arg(nativeStatementTemp190,2);
JSValue p3=arg(nativeStatementTemp190,3);
JSValue v1=arg(nativeStatementTemp190,4);
JSValue v2=arg(nativeStatementTemp190,5);
JSValue v3=arg(nativeStatementTemp190,6);
JSValue target=arg(nativeStatementTemp190,7);
if(([&]()->JSValue{const auto nativeStatementTemp195=([&]()->JSValue{auto nativeStatementTemp200=nativeStatementTemp193;const auto nativeStatementTemp201=JSValue("getBarycoord");const auto nativeStatementTemp197=nativeStatementTemp200.get(nativeStatementTemp201);const auto nativeStatementTemp198=nativeStatementTemp200;Args nativeStatementTemp199;nativeStatementTemp199.push_back(point);nativeStatementTemp199.push_back(p1);nativeStatementTemp199.push_back(p2);nativeStatementTemp199.push_back(p3);nativeStatementTemp199.push_back(nativeStatementTemp192.get("_v3"));return nativeStatementTemp197.call(nativeStatementTemp198,nativeStatementTemp199);}());const auto nativeStatementTemp196=JSValue(nullptr);return binary("===",nativeStatementTemp195,nativeStatementTemp196);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp202=target;const auto nativeStatementTemp203=JSValue("x");const auto nativeStatementTemp204=JSValue(0);setReference(nativeStatementTemp202,nativeStatementTemp203,nativeStatementTemp204);return nativeStatementTemp204;}());
([&]()->JSValue{auto nativeStatementTemp207=target;const auto nativeStatementTemp208=JSValue("y");const auto nativeStatementTemp209=JSValue(0);setReference(nativeStatementTemp207,nativeStatementTemp208,nativeStatementTemp209);return nativeStatementTemp209;}());
if(([&]()->JSValue{const auto nativeStatementTemp212=JSValue("z");const auto nativeStatementTemp213=target;return binary("in",nativeStatementTemp212,nativeStatementTemp213);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp214=target;const auto nativeStatementTemp215=JSValue("z");const auto nativeStatementTemp216=JSValue(0);setReference(nativeStatementTemp214,nativeStatementTemp215,nativeStatementTemp216);return nativeStatementTemp216;}());
if(([&]()->JSValue{const auto nativeStatementTemp219=JSValue("w");const auto nativeStatementTemp220=target;return binary("in",nativeStatementTemp219,nativeStatementTemp220);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp221=target;const auto nativeStatementTemp222=JSValue("w");const auto nativeStatementTemp223=JSValue(0);setReference(nativeStatementTemp221,nativeStatementTemp222,nativeStatementTemp223);return nativeStatementTemp223;}());
return JSValue(nullptr);

}
([&]()->JSValue{auto nativeStatementTemp229=target;const auto nativeStatementTemp230=JSValue("setScalar");const auto nativeStatementTemp226=nativeStatementTemp229.get(nativeStatementTemp230);const auto nativeStatementTemp227=nativeStatementTemp229;Args nativeStatementTemp228;nativeStatementTemp228.push_back(JSValue(0));return nativeStatementTemp226.call(nativeStatementTemp227,nativeStatementTemp228);}());
([&]()->JSValue{auto nativeStatementTemp234=target;const auto nativeStatementTemp235=JSValue("addScaledVector");const auto nativeStatementTemp231=nativeStatementTemp234.get(nativeStatementTemp235);const auto nativeStatementTemp232=nativeStatementTemp234;Args nativeStatementTemp233;nativeStatementTemp233.push_back(v1);nativeStatementTemp233.push_back(([&]()->JSValue{const auto nativeStatementTemp236=nativeStatementTemp192.get("_v3");const auto nativeStatementTemp237=JSValue("x");return nativeStatementTemp236.get(nativeStatementTemp237);}()));return nativeStatementTemp231.call(nativeStatementTemp232,nativeStatementTemp233);}());
([&]()->JSValue{auto nativeStatementTemp241=target;const auto nativeStatementTemp242=JSValue("addScaledVector");const auto nativeStatementTemp238=nativeStatementTemp241.get(nativeStatementTemp242);const auto nativeStatementTemp239=nativeStatementTemp241;Args nativeStatementTemp240;nativeStatementTemp240.push_back(v2);nativeStatementTemp240.push_back(([&]()->JSValue{const auto nativeStatementTemp243=nativeStatementTemp192.get("_v3");const auto nativeStatementTemp244=JSValue("y");return nativeStatementTemp243.get(nativeStatementTemp244);}()));return nativeStatementTemp238.call(nativeStatementTemp239,nativeStatementTemp240);}());
([&]()->JSValue{auto nativeStatementTemp248=target;const auto nativeStatementTemp249=JSValue("addScaledVector");const auto nativeStatementTemp245=nativeStatementTemp248.get(nativeStatementTemp249);const auto nativeStatementTemp246=nativeStatementTemp248;Args nativeStatementTemp247;nativeStatementTemp247.push_back(v3);nativeStatementTemp247.push_back(([&]()->JSValue{const auto nativeStatementTemp250=nativeStatementTemp192.get("_v3");const auto nativeStatementTemp251=JSValue("z");return nativeStatementTemp250.get(nativeStatementTemp251);}()));return nativeStatementTemp245.call(nativeStatementTemp246,nativeStatementTemp247);}());
return target;
return JSValue{};
}

JSValue method5(const JSValue& nativeStatementTemp252,const Args& nativeStatementTemp253,const Args& nativeStatementTemp254){
auto nativeStatementTemp255=nativeStatementTemp254[0];auto nativeStatementTemp256=nativeStatementTemp252;
JSValue attr=arg(nativeStatementTemp253,0);
JSValue i1=arg(nativeStatementTemp253,1);
JSValue i2=arg(nativeStatementTemp253,2);
JSValue i3=arg(nativeStatementTemp253,3);
JSValue barycoord=arg(nativeStatementTemp253,4);
JSValue target=arg(nativeStatementTemp253,5);
([&]()->JSValue{auto nativeStatementTemp261=nativeStatementTemp255.get("_v40");const auto nativeStatementTemp262=JSValue("setScalar");const auto nativeStatementTemp258=nativeStatementTemp261.get(nativeStatementTemp262);const auto nativeStatementTemp259=nativeStatementTemp261;Args nativeStatementTemp260;nativeStatementTemp260.push_back(JSValue(0));return nativeStatementTemp258.call(nativeStatementTemp259,nativeStatementTemp260);}());
([&]()->JSValue{auto nativeStatementTemp266=nativeStatementTemp255.get("_v41");const auto nativeStatementTemp267=JSValue("setScalar");const auto nativeStatementTemp263=nativeStatementTemp266.get(nativeStatementTemp267);const auto nativeStatementTemp264=nativeStatementTemp266;Args nativeStatementTemp265;nativeStatementTemp265.push_back(JSValue(0));return nativeStatementTemp263.call(nativeStatementTemp264,nativeStatementTemp265);}());
([&]()->JSValue{auto nativeStatementTemp271=nativeStatementTemp255.get("_v42");const auto nativeStatementTemp272=JSValue("setScalar");const auto nativeStatementTemp268=nativeStatementTemp271.get(nativeStatementTemp272);const auto nativeStatementTemp269=nativeStatementTemp271;Args nativeStatementTemp270;nativeStatementTemp270.push_back(JSValue(0));return nativeStatementTemp268.call(nativeStatementTemp269,nativeStatementTemp270);}());
([&]()->JSValue{auto nativeStatementTemp276=nativeStatementTemp255.get("_v40");const auto nativeStatementTemp277=JSValue("fromBufferAttribute");const auto nativeStatementTemp273=nativeStatementTemp276.get(nativeStatementTemp277);const auto nativeStatementTemp274=nativeStatementTemp276;Args nativeStatementTemp275;nativeStatementTemp275.push_back(attr);nativeStatementTemp275.push_back(i1);return nativeStatementTemp273.call(nativeStatementTemp274,nativeStatementTemp275);}());
([&]()->JSValue{auto nativeStatementTemp281=nativeStatementTemp255.get("_v41");const auto nativeStatementTemp282=JSValue("fromBufferAttribute");const auto nativeStatementTemp278=nativeStatementTemp281.get(nativeStatementTemp282);const auto nativeStatementTemp279=nativeStatementTemp281;Args nativeStatementTemp280;nativeStatementTemp280.push_back(attr);nativeStatementTemp280.push_back(i2);return nativeStatementTemp278.call(nativeStatementTemp279,nativeStatementTemp280);}());
([&]()->JSValue{auto nativeStatementTemp286=nativeStatementTemp255.get("_v42");const auto nativeStatementTemp287=JSValue("fromBufferAttribute");const auto nativeStatementTemp283=nativeStatementTemp286.get(nativeStatementTemp287);const auto nativeStatementTemp284=nativeStatementTemp286;Args nativeStatementTemp285;nativeStatementTemp285.push_back(attr);nativeStatementTemp285.push_back(i3);return nativeStatementTemp283.call(nativeStatementTemp284,nativeStatementTemp285);}());
([&]()->JSValue{auto nativeStatementTemp291=target;const auto nativeStatementTemp292=JSValue("setScalar");const auto nativeStatementTemp288=nativeStatementTemp291.get(nativeStatementTemp292);const auto nativeStatementTemp289=nativeStatementTemp291;Args nativeStatementTemp290;nativeStatementTemp290.push_back(JSValue(0));return nativeStatementTemp288.call(nativeStatementTemp289,nativeStatementTemp290);}());
([&]()->JSValue{auto nativeStatementTemp296=target;const auto nativeStatementTemp297=JSValue("addScaledVector");const auto nativeStatementTemp293=nativeStatementTemp296.get(nativeStatementTemp297);const auto nativeStatementTemp294=nativeStatementTemp296;Args nativeStatementTemp295;nativeStatementTemp295.push_back(nativeStatementTemp255.get("_v40"));nativeStatementTemp295.push_back(([&]()->JSValue{const auto nativeStatementTemp298=barycoord;const auto nativeStatementTemp299=JSValue("x");return nativeStatementTemp298.get(nativeStatementTemp299);}()));return nativeStatementTemp293.call(nativeStatementTemp294,nativeStatementTemp295);}());
([&]()->JSValue{auto nativeStatementTemp303=target;const auto nativeStatementTemp304=JSValue("addScaledVector");const auto nativeStatementTemp300=nativeStatementTemp303.get(nativeStatementTemp304);const auto nativeStatementTemp301=nativeStatementTemp303;Args nativeStatementTemp302;nativeStatementTemp302.push_back(nativeStatementTemp255.get("_v41"));nativeStatementTemp302.push_back(([&]()->JSValue{const auto nativeStatementTemp305=barycoord;const auto nativeStatementTemp306=JSValue("y");return nativeStatementTemp305.get(nativeStatementTemp306);}()));return nativeStatementTemp300.call(nativeStatementTemp301,nativeStatementTemp302);}());
([&]()->JSValue{auto nativeStatementTemp310=target;const auto nativeStatementTemp311=JSValue("addScaledVector");const auto nativeStatementTemp307=nativeStatementTemp310.get(nativeStatementTemp311);const auto nativeStatementTemp308=nativeStatementTemp310;Args nativeStatementTemp309;nativeStatementTemp309.push_back(nativeStatementTemp255.get("_v42"));nativeStatementTemp309.push_back(([&]()->JSValue{const auto nativeStatementTemp312=barycoord;const auto nativeStatementTemp313=JSValue("z");return nativeStatementTemp312.get(nativeStatementTemp313);}()));return nativeStatementTemp307.call(nativeStatementTemp308,nativeStatementTemp309);}());
return target;
return JSValue{};
}

JSValue method6(const JSValue& nativeStatementTemp314,const Args& nativeStatementTemp315,const Args& nativeStatementTemp316){
auto nativeStatementTemp317=nativeStatementTemp316[0];auto nativeStatementTemp318=nativeStatementTemp314;
JSValue a=arg(nativeStatementTemp315,0);
JSValue b=arg(nativeStatementTemp315,1);
JSValue c=arg(nativeStatementTemp315,2);
JSValue direction=arg(nativeStatementTemp315,3);
([&]()->JSValue{auto nativeStatementTemp323=nativeStatementTemp317.get("_v0");const auto nativeStatementTemp324=JSValue("subVectors");const auto nativeStatementTemp320=nativeStatementTemp323.get(nativeStatementTemp324);const auto nativeStatementTemp321=nativeStatementTemp323;Args nativeStatementTemp322;nativeStatementTemp322.push_back(c);nativeStatementTemp322.push_back(b);return nativeStatementTemp320.call(nativeStatementTemp321,nativeStatementTemp322);}());
([&]()->JSValue{auto nativeStatementTemp328=nativeStatementTemp317.get("_v1");const auto nativeStatementTemp329=JSValue("subVectors");const auto nativeStatementTemp325=nativeStatementTemp328.get(nativeStatementTemp329);const auto nativeStatementTemp326=nativeStatementTemp328;Args nativeStatementTemp327;nativeStatementTemp327.push_back(a);nativeStatementTemp327.push_back(b);return nativeStatementTemp325.call(nativeStatementTemp326,nativeStatementTemp327);}());
return ([&]()->JSValue{const auto nativeStatementTemp330=([&]()->JSValue{auto nativeStatementTemp335=([&]()->JSValue{auto nativeStatementTemp340=nativeStatementTemp317.get("_v0");const auto nativeStatementTemp341=JSValue("cross");const auto nativeStatementTemp337=nativeStatementTemp340.get(nativeStatementTemp341);const auto nativeStatementTemp338=nativeStatementTemp340;Args nativeStatementTemp339;nativeStatementTemp339.push_back(nativeStatementTemp317.get("_v1"));return nativeStatementTemp337.call(nativeStatementTemp338,nativeStatementTemp339);}());const auto nativeStatementTemp336=JSValue("dot");const auto nativeStatementTemp332=nativeStatementTemp335.get(nativeStatementTemp336);const auto nativeStatementTemp333=nativeStatementTemp335;Args nativeStatementTemp334;nativeStatementTemp334.push_back(direction);return nativeStatementTemp332.call(nativeStatementTemp333,nativeStatementTemp334);}());const auto nativeStatementTemp331=JSValue(0);return binary("<",nativeStatementTemp330,nativeStatementTemp331);}());
return JSValue{};
}

JSValue method7(const JSValue& nativeStatementTemp342,const Args& nativeStatementTemp343,const Args& nativeStatementTemp344){
auto nativeStatementTemp345=nativeStatementTemp344[0];auto nativeStatementTemp346=nativeStatementTemp342;
JSValue a=arg(nativeStatementTemp343,0);
JSValue b=arg(nativeStatementTemp343,1);
JSValue c=arg(nativeStatementTemp343,2);
([&]()->JSValue{auto nativeStatementTemp351=([&]()->JSValue{const auto nativeStatementTemp353=nativeStatementTemp346;const auto nativeStatementTemp354=JSValue("a");return nativeStatementTemp353.get(nativeStatementTemp354);}());const auto nativeStatementTemp352=JSValue("copy");const auto nativeStatementTemp348=nativeStatementTemp351.get(nativeStatementTemp352);const auto nativeStatementTemp349=nativeStatementTemp351;Args nativeStatementTemp350;nativeStatementTemp350.push_back(a);return nativeStatementTemp348.call(nativeStatementTemp349,nativeStatementTemp350);}());
([&]()->JSValue{auto nativeStatementTemp358=([&]()->JSValue{const auto nativeStatementTemp360=nativeStatementTemp346;const auto nativeStatementTemp361=JSValue("b");return nativeStatementTemp360.get(nativeStatementTemp361);}());const auto nativeStatementTemp359=JSValue("copy");const auto nativeStatementTemp355=nativeStatementTemp358.get(nativeStatementTemp359);const auto nativeStatementTemp356=nativeStatementTemp358;Args nativeStatementTemp357;nativeStatementTemp357.push_back(b);return nativeStatementTemp355.call(nativeStatementTemp356,nativeStatementTemp357);}());
([&]()->JSValue{auto nativeStatementTemp365=([&]()->JSValue{const auto nativeStatementTemp367=nativeStatementTemp346;const auto nativeStatementTemp368=JSValue("c");return nativeStatementTemp367.get(nativeStatementTemp368);}());const auto nativeStatementTemp366=JSValue("copy");const auto nativeStatementTemp362=nativeStatementTemp365.get(nativeStatementTemp366);const auto nativeStatementTemp363=nativeStatementTemp365;Args nativeStatementTemp364;nativeStatementTemp364.push_back(c);return nativeStatementTemp362.call(nativeStatementTemp363,nativeStatementTemp364);}());
return nativeStatementTemp346;
return JSValue{};
}

JSValue method8(const JSValue& nativeStatementTemp369,const Args& nativeStatementTemp370,const Args& nativeStatementTemp371){
auto nativeStatementTemp372=nativeStatementTemp371[0];auto nativeStatementTemp373=nativeStatementTemp369;
JSValue points=arg(nativeStatementTemp370,0);
JSValue i0=arg(nativeStatementTemp370,1);
JSValue i1=arg(nativeStatementTemp370,2);
JSValue i2=arg(nativeStatementTemp370,3);
([&]()->JSValue{auto nativeStatementTemp378=([&]()->JSValue{const auto nativeStatementTemp380=nativeStatementTemp373;const auto nativeStatementTemp381=JSValue("a");return nativeStatementTemp380.get(nativeStatementTemp381);}());const auto nativeStatementTemp379=JSValue("copy");const auto nativeStatementTemp375=nativeStatementTemp378.get(nativeStatementTemp379);const auto nativeStatementTemp376=nativeStatementTemp378;Args nativeStatementTemp377;nativeStatementTemp377.push_back(([&]()->JSValue{const auto nativeStatementTemp382=points;const auto nativeStatementTemp383=i0;return nativeStatementTemp382.get(nativeStatementTemp383);}()));return nativeStatementTemp375.call(nativeStatementTemp376,nativeStatementTemp377);}());
([&]()->JSValue{auto nativeStatementTemp387=([&]()->JSValue{const auto nativeStatementTemp389=nativeStatementTemp373;const auto nativeStatementTemp390=JSValue("b");return nativeStatementTemp389.get(nativeStatementTemp390);}());const auto nativeStatementTemp388=JSValue("copy");const auto nativeStatementTemp384=nativeStatementTemp387.get(nativeStatementTemp388);const auto nativeStatementTemp385=nativeStatementTemp387;Args nativeStatementTemp386;nativeStatementTemp386.push_back(([&]()->JSValue{const auto nativeStatementTemp391=points;const auto nativeStatementTemp392=i1;return nativeStatementTemp391.get(nativeStatementTemp392);}()));return nativeStatementTemp384.call(nativeStatementTemp385,nativeStatementTemp386);}());
([&]()->JSValue{auto nativeStatementTemp396=([&]()->JSValue{const auto nativeStatementTemp398=nativeStatementTemp373;const auto nativeStatementTemp399=JSValue("c");return nativeStatementTemp398.get(nativeStatementTemp399);}());const auto nativeStatementTemp397=JSValue("copy");const auto nativeStatementTemp393=nativeStatementTemp396.get(nativeStatementTemp397);const auto nativeStatementTemp394=nativeStatementTemp396;Args nativeStatementTemp395;nativeStatementTemp395.push_back(([&]()->JSValue{const auto nativeStatementTemp400=points;const auto nativeStatementTemp401=i2;return nativeStatementTemp400.get(nativeStatementTemp401);}()));return nativeStatementTemp393.call(nativeStatementTemp394,nativeStatementTemp395);}());
return nativeStatementTemp373;
return JSValue{};
}

JSValue method9(const JSValue& nativeStatementTemp402,const Args& nativeStatementTemp403,const Args& nativeStatementTemp404){
auto nativeStatementTemp405=nativeStatementTemp404[0];auto nativeStatementTemp406=nativeStatementTemp402;
JSValue attribute=arg(nativeStatementTemp403,0);
JSValue i0=arg(nativeStatementTemp403,1);
JSValue i1=arg(nativeStatementTemp403,2);
JSValue i2=arg(nativeStatementTemp403,3);
([&]()->JSValue{auto nativeStatementTemp411=([&]()->JSValue{const auto nativeStatementTemp413=nativeStatementTemp406;const auto nativeStatementTemp414=JSValue("a");return nativeStatementTemp413.get(nativeStatementTemp414);}());const auto nativeStatementTemp412=JSValue("fromBufferAttribute");const auto nativeStatementTemp408=nativeStatementTemp411.get(nativeStatementTemp412);const auto nativeStatementTemp409=nativeStatementTemp411;Args nativeStatementTemp410;nativeStatementTemp410.push_back(attribute);nativeStatementTemp410.push_back(i0);return nativeStatementTemp408.call(nativeStatementTemp409,nativeStatementTemp410);}());
([&]()->JSValue{auto nativeStatementTemp418=([&]()->JSValue{const auto nativeStatementTemp420=nativeStatementTemp406;const auto nativeStatementTemp421=JSValue("b");return nativeStatementTemp420.get(nativeStatementTemp421);}());const auto nativeStatementTemp419=JSValue("fromBufferAttribute");const auto nativeStatementTemp415=nativeStatementTemp418.get(nativeStatementTemp419);const auto nativeStatementTemp416=nativeStatementTemp418;Args nativeStatementTemp417;nativeStatementTemp417.push_back(attribute);nativeStatementTemp417.push_back(i1);return nativeStatementTemp415.call(nativeStatementTemp416,nativeStatementTemp417);}());
([&]()->JSValue{auto nativeStatementTemp425=([&]()->JSValue{const auto nativeStatementTemp427=nativeStatementTemp406;const auto nativeStatementTemp428=JSValue("c");return nativeStatementTemp427.get(nativeStatementTemp428);}());const auto nativeStatementTemp426=JSValue("fromBufferAttribute");const auto nativeStatementTemp422=nativeStatementTemp425.get(nativeStatementTemp426);const auto nativeStatementTemp423=nativeStatementTemp425;Args nativeStatementTemp424;nativeStatementTemp424.push_back(attribute);nativeStatementTemp424.push_back(i2);return nativeStatementTemp422.call(nativeStatementTemp423,nativeStatementTemp424);}());
return nativeStatementTemp406;
return JSValue{};
}

JSValue method10(const JSValue& nativeStatementTemp429,const Args& nativeStatementTemp430,const Args& nativeStatementTemp431){
auto nativeStatementTemp432=nativeStatementTemp431[0];auto nativeStatementTemp433=nativeStatementTemp429;
return ([&]()->JSValue{auto nativeStatementTemp438=([&]()->JSValue{const auto nativeStatementTemp440=([&]()->JSValue{const auto nativeStatementTemp442=nativeStatementTemp433;const auto nativeStatementTemp443=JSValue("constructor");return nativeStatementTemp442.get(nativeStatementTemp443);}());Args nativeStatementTemp441;return nativeStatementTemp440.construct(nativeStatementTemp441);}());const auto nativeStatementTemp439=JSValue("copy");const auto nativeStatementTemp435=nativeStatementTemp438.get(nativeStatementTemp439);const auto nativeStatementTemp436=nativeStatementTemp438;Args nativeStatementTemp437;nativeStatementTemp437.push_back(nativeStatementTemp433);return nativeStatementTemp435.call(nativeStatementTemp436,nativeStatementTemp437);}());
return JSValue{};
}

JSValue method11(const JSValue& nativeStatementTemp444,const Args& nativeStatementTemp445,const Args& nativeStatementTemp446){
auto nativeStatementTemp447=nativeStatementTemp446[0];auto nativeStatementTemp448=nativeStatementTemp444;
JSValue triangle=arg(nativeStatementTemp445,0);
([&]()->JSValue{auto nativeStatementTemp453=([&]()->JSValue{const auto nativeStatementTemp455=nativeStatementTemp448;const auto nativeStatementTemp456=JSValue("a");return nativeStatementTemp455.get(nativeStatementTemp456);}());const auto nativeStatementTemp454=JSValue("copy");const auto nativeStatementTemp450=nativeStatementTemp453.get(nativeStatementTemp454);const auto nativeStatementTemp451=nativeStatementTemp453;Args nativeStatementTemp452;nativeStatementTemp452.push_back(([&]()->JSValue{const auto nativeStatementTemp457=triangle;const auto nativeStatementTemp458=JSValue("a");return nativeStatementTemp457.get(nativeStatementTemp458);}()));return nativeStatementTemp450.call(nativeStatementTemp451,nativeStatementTemp452);}());
([&]()->JSValue{auto nativeStatementTemp462=([&]()->JSValue{const auto nativeStatementTemp464=nativeStatementTemp448;const auto nativeStatementTemp465=JSValue("b");return nativeStatementTemp464.get(nativeStatementTemp465);}());const auto nativeStatementTemp463=JSValue("copy");const auto nativeStatementTemp459=nativeStatementTemp462.get(nativeStatementTemp463);const auto nativeStatementTemp460=nativeStatementTemp462;Args nativeStatementTemp461;nativeStatementTemp461.push_back(([&]()->JSValue{const auto nativeStatementTemp466=triangle;const auto nativeStatementTemp467=JSValue("b");return nativeStatementTemp466.get(nativeStatementTemp467);}()));return nativeStatementTemp459.call(nativeStatementTemp460,nativeStatementTemp461);}());
([&]()->JSValue{auto nativeStatementTemp471=([&]()->JSValue{const auto nativeStatementTemp473=nativeStatementTemp448;const auto nativeStatementTemp474=JSValue("c");return nativeStatementTemp473.get(nativeStatementTemp474);}());const auto nativeStatementTemp472=JSValue("copy");const auto nativeStatementTemp468=nativeStatementTemp471.get(nativeStatementTemp472);const auto nativeStatementTemp469=nativeStatementTemp471;Args nativeStatementTemp470;nativeStatementTemp470.push_back(([&]()->JSValue{const auto nativeStatementTemp475=triangle;const auto nativeStatementTemp476=JSValue("c");return nativeStatementTemp475.get(nativeStatementTemp476);}()));return nativeStatementTemp468.call(nativeStatementTemp469,nativeStatementTemp470);}());
return nativeStatementTemp448;
return JSValue{};
}

JSValue method12(const JSValue& nativeStatementTemp477,const Args& nativeStatementTemp478,const Args& nativeStatementTemp479){
auto nativeStatementTemp480=nativeStatementTemp479[0];auto nativeStatementTemp481=nativeStatementTemp477;
([&]()->JSValue{auto nativeStatementTemp486=nativeStatementTemp480.get("_v0");const auto nativeStatementTemp487=JSValue("subVectors");const auto nativeStatementTemp483=nativeStatementTemp486.get(nativeStatementTemp487);const auto nativeStatementTemp484=nativeStatementTemp486;Args nativeStatementTemp485;nativeStatementTemp485.push_back(([&]()->JSValue{const auto nativeStatementTemp488=nativeStatementTemp481;const auto nativeStatementTemp489=JSValue("c");return nativeStatementTemp488.get(nativeStatementTemp489);}()));nativeStatementTemp485.push_back(([&]()->JSValue{const auto nativeStatementTemp490=nativeStatementTemp481;const auto nativeStatementTemp491=JSValue("b");return nativeStatementTemp490.get(nativeStatementTemp491);}()));return nativeStatementTemp483.call(nativeStatementTemp484,nativeStatementTemp485);}());
([&]()->JSValue{auto nativeStatementTemp495=nativeStatementTemp480.get("_v1");const auto nativeStatementTemp496=JSValue("subVectors");const auto nativeStatementTemp492=nativeStatementTemp495.get(nativeStatementTemp496);const auto nativeStatementTemp493=nativeStatementTemp495;Args nativeStatementTemp494;nativeStatementTemp494.push_back(([&]()->JSValue{const auto nativeStatementTemp497=nativeStatementTemp481;const auto nativeStatementTemp498=JSValue("a");return nativeStatementTemp497.get(nativeStatementTemp498);}()));nativeStatementTemp494.push_back(([&]()->JSValue{const auto nativeStatementTemp499=nativeStatementTemp481;const auto nativeStatementTemp500=JSValue("b");return nativeStatementTemp499.get(nativeStatementTemp500);}()));return nativeStatementTemp492.call(nativeStatementTemp493,nativeStatementTemp494);}());
return ([&]()->JSValue{const auto nativeStatementTemp501=([&]()->JSValue{auto nativeStatementTemp506=([&]()->JSValue{auto nativeStatementTemp511=nativeStatementTemp480.get("_v0");const auto nativeStatementTemp512=JSValue("cross");const auto nativeStatementTemp508=nativeStatementTemp511.get(nativeStatementTemp512);const auto nativeStatementTemp509=nativeStatementTemp511;Args nativeStatementTemp510;nativeStatementTemp510.push_back(nativeStatementTemp480.get("_v1"));return nativeStatementTemp508.call(nativeStatementTemp509,nativeStatementTemp510);}());const auto nativeStatementTemp507=JSValue("length");const auto nativeStatementTemp503=nativeStatementTemp506.get(nativeStatementTemp507);const auto nativeStatementTemp504=nativeStatementTemp506;Args nativeStatementTemp505;return nativeStatementTemp503.call(nativeStatementTemp504,nativeStatementTemp505);}());const auto nativeStatementTemp502=JSValue(0.5);return binary("*",nativeStatementTemp501,nativeStatementTemp502);}());
return JSValue{};
}

JSValue method13(const JSValue& nativeStatementTemp513,const Args& nativeStatementTemp514,const Args& nativeStatementTemp515){
auto nativeStatementTemp516=nativeStatementTemp515[0];auto nativeStatementTemp517=nativeStatementTemp513;
JSValue target=arg(nativeStatementTemp514,0);
return ([&]()->JSValue{auto nativeStatementTemp522=([&]()->JSValue{auto nativeStatementTemp527=([&]()->JSValue{auto nativeStatementTemp532=target;const auto nativeStatementTemp533=JSValue("addVectors");const auto nativeStatementTemp529=nativeStatementTemp532.get(nativeStatementTemp533);const auto nativeStatementTemp530=nativeStatementTemp532;Args nativeStatementTemp531;nativeStatementTemp531.push_back(([&]()->JSValue{const auto nativeStatementTemp534=nativeStatementTemp517;const auto nativeStatementTemp535=JSValue("a");return nativeStatementTemp534.get(nativeStatementTemp535);}()));nativeStatementTemp531.push_back(([&]()->JSValue{const auto nativeStatementTemp536=nativeStatementTemp517;const auto nativeStatementTemp537=JSValue("b");return nativeStatementTemp536.get(nativeStatementTemp537);}()));return nativeStatementTemp529.call(nativeStatementTemp530,nativeStatementTemp531);}());const auto nativeStatementTemp528=JSValue("add");const auto nativeStatementTemp524=nativeStatementTemp527.get(nativeStatementTemp528);const auto nativeStatementTemp525=nativeStatementTemp527;Args nativeStatementTemp526;nativeStatementTemp526.push_back(([&]()->JSValue{const auto nativeStatementTemp538=nativeStatementTemp517;const auto nativeStatementTemp539=JSValue("c");return nativeStatementTemp538.get(nativeStatementTemp539);}()));return nativeStatementTemp524.call(nativeStatementTemp525,nativeStatementTemp526);}());const auto nativeStatementTemp523=JSValue("multiplyScalar");const auto nativeStatementTemp519=nativeStatementTemp522.get(nativeStatementTemp523);const auto nativeStatementTemp520=nativeStatementTemp522;Args nativeStatementTemp521;nativeStatementTemp521.push_back(([&]()->JSValue{const auto nativeStatementTemp540=JSValue(1);const auto nativeStatementTemp541=JSValue(3);return binary("/",nativeStatementTemp540,nativeStatementTemp541);}()));return nativeStatementTemp519.call(nativeStatementTemp520,nativeStatementTemp521);}());
return JSValue{};
}

JSValue method14(const JSValue& nativeStatementTemp542,const Args& nativeStatementTemp543,const Args& nativeStatementTemp544){
auto nativeStatementTemp545=nativeStatementTemp544[0];auto nativeStatementTemp546=nativeStatementTemp542;
JSValue target=arg(nativeStatementTemp543,0);
return ([&]()->JSValue{auto nativeStatementTemp551=nativeStatementTemp545.get("Triangle");const auto nativeStatementTemp552=JSValue("getNormal");const auto nativeStatementTemp548=nativeStatementTemp551.get(nativeStatementTemp552);const auto nativeStatementTemp549=nativeStatementTemp551;Args nativeStatementTemp550;nativeStatementTemp550.push_back(([&]()->JSValue{const auto nativeStatementTemp553=nativeStatementTemp546;const auto nativeStatementTemp554=JSValue("a");return nativeStatementTemp553.get(nativeStatementTemp554);}()));nativeStatementTemp550.push_back(([&]()->JSValue{const auto nativeStatementTemp555=nativeStatementTemp546;const auto nativeStatementTemp556=JSValue("b");return nativeStatementTemp555.get(nativeStatementTemp556);}()));nativeStatementTemp550.push_back(([&]()->JSValue{const auto nativeStatementTemp557=nativeStatementTemp546;const auto nativeStatementTemp558=JSValue("c");return nativeStatementTemp557.get(nativeStatementTemp558);}()));nativeStatementTemp550.push_back(target);return nativeStatementTemp548.call(nativeStatementTemp549,nativeStatementTemp550);}());
return JSValue{};
}

JSValue method15(const JSValue& nativeStatementTemp559,const Args& nativeStatementTemp560,const Args& nativeStatementTemp561){
auto nativeStatementTemp562=nativeStatementTemp561[0];auto nativeStatementTemp563=nativeStatementTemp559;
JSValue target=arg(nativeStatementTemp560,0);
return ([&]()->JSValue{auto nativeStatementTemp568=target;const auto nativeStatementTemp569=JSValue("setFromCoplanarPoints");const auto nativeStatementTemp565=nativeStatementTemp568.get(nativeStatementTemp569);const auto nativeStatementTemp566=nativeStatementTemp568;Args nativeStatementTemp567;nativeStatementTemp567.push_back(([&]()->JSValue{const auto nativeStatementTemp570=nativeStatementTemp563;const auto nativeStatementTemp571=JSValue("a");return nativeStatementTemp570.get(nativeStatementTemp571);}()));nativeStatementTemp567.push_back(([&]()->JSValue{const auto nativeStatementTemp572=nativeStatementTemp563;const auto nativeStatementTemp573=JSValue("b");return nativeStatementTemp572.get(nativeStatementTemp573);}()));nativeStatementTemp567.push_back(([&]()->JSValue{const auto nativeStatementTemp574=nativeStatementTemp563;const auto nativeStatementTemp575=JSValue("c");return nativeStatementTemp574.get(nativeStatementTemp575);}()));return nativeStatementTemp565.call(nativeStatementTemp566,nativeStatementTemp567);}());
return JSValue{};
}

JSValue method16(const JSValue& nativeStatementTemp576,const Args& nativeStatementTemp577,const Args& nativeStatementTemp578){
auto nativeStatementTemp579=nativeStatementTemp578[0];auto nativeStatementTemp580=nativeStatementTemp576;
JSValue point=arg(nativeStatementTemp577,0);
JSValue target=arg(nativeStatementTemp577,1);
return ([&]()->JSValue{auto nativeStatementTemp585=nativeStatementTemp579.get("Triangle");const auto nativeStatementTemp586=JSValue("getBarycoord");const auto nativeStatementTemp582=nativeStatementTemp585.get(nativeStatementTemp586);const auto nativeStatementTemp583=nativeStatementTemp585;Args nativeStatementTemp584;nativeStatementTemp584.push_back(point);nativeStatementTemp584.push_back(([&]()->JSValue{const auto nativeStatementTemp587=nativeStatementTemp580;const auto nativeStatementTemp588=JSValue("a");return nativeStatementTemp587.get(nativeStatementTemp588);}()));nativeStatementTemp584.push_back(([&]()->JSValue{const auto nativeStatementTemp589=nativeStatementTemp580;const auto nativeStatementTemp590=JSValue("b");return nativeStatementTemp589.get(nativeStatementTemp590);}()));nativeStatementTemp584.push_back(([&]()->JSValue{const auto nativeStatementTemp591=nativeStatementTemp580;const auto nativeStatementTemp592=JSValue("c");return nativeStatementTemp591.get(nativeStatementTemp592);}()));nativeStatementTemp584.push_back(target);return nativeStatementTemp582.call(nativeStatementTemp583,nativeStatementTemp584);}());
return JSValue{};
}

JSValue method17(const JSValue& nativeStatementTemp593,const Args& nativeStatementTemp594,const Args& nativeStatementTemp595){
auto nativeStatementTemp596=nativeStatementTemp595[0];auto nativeStatementTemp597=nativeStatementTemp593;
JSValue point=arg(nativeStatementTemp594,0);
JSValue v1=arg(nativeStatementTemp594,1);
JSValue v2=arg(nativeStatementTemp594,2);
JSValue v3=arg(nativeStatementTemp594,3);
JSValue target=arg(nativeStatementTemp594,4);
return ([&]()->JSValue{auto nativeStatementTemp602=nativeStatementTemp596.get("Triangle");const auto nativeStatementTemp603=JSValue("getInterpolation");const auto nativeStatementTemp599=nativeStatementTemp602.get(nativeStatementTemp603);const auto nativeStatementTemp600=nativeStatementTemp602;Args nativeStatementTemp601;nativeStatementTemp601.push_back(point);nativeStatementTemp601.push_back(([&]()->JSValue{const auto nativeStatementTemp604=nativeStatementTemp597;const auto nativeStatementTemp605=JSValue("a");return nativeStatementTemp604.get(nativeStatementTemp605);}()));nativeStatementTemp601.push_back(([&]()->JSValue{const auto nativeStatementTemp606=nativeStatementTemp597;const auto nativeStatementTemp607=JSValue("b");return nativeStatementTemp606.get(nativeStatementTemp607);}()));nativeStatementTemp601.push_back(([&]()->JSValue{const auto nativeStatementTemp608=nativeStatementTemp597;const auto nativeStatementTemp609=JSValue("c");return nativeStatementTemp608.get(nativeStatementTemp609);}()));nativeStatementTemp601.push_back(v1);nativeStatementTemp601.push_back(v2);nativeStatementTemp601.push_back(v3);nativeStatementTemp601.push_back(target);return nativeStatementTemp599.call(nativeStatementTemp600,nativeStatementTemp601);}());
return JSValue{};
}

JSValue method18(const JSValue& nativeStatementTemp610,const Args& nativeStatementTemp611,const Args& nativeStatementTemp612){
auto nativeStatementTemp613=nativeStatementTemp612[0];auto nativeStatementTemp614=nativeStatementTemp610;
JSValue point=arg(nativeStatementTemp611,0);
return ([&]()->JSValue{auto nativeStatementTemp619=nativeStatementTemp613.get("Triangle");const auto nativeStatementTemp620=JSValue("containsPoint");const auto nativeStatementTemp616=nativeStatementTemp619.get(nativeStatementTemp620);const auto nativeStatementTemp617=nativeStatementTemp619;Args nativeStatementTemp618;nativeStatementTemp618.push_back(point);nativeStatementTemp618.push_back(([&]()->JSValue{const auto nativeStatementTemp621=nativeStatementTemp614;const auto nativeStatementTemp622=JSValue("a");return nativeStatementTemp621.get(nativeStatementTemp622);}()));nativeStatementTemp618.push_back(([&]()->JSValue{const auto nativeStatementTemp623=nativeStatementTemp614;const auto nativeStatementTemp624=JSValue("b");return nativeStatementTemp623.get(nativeStatementTemp624);}()));nativeStatementTemp618.push_back(([&]()->JSValue{const auto nativeStatementTemp625=nativeStatementTemp614;const auto nativeStatementTemp626=JSValue("c");return nativeStatementTemp625.get(nativeStatementTemp626);}()));return nativeStatementTemp616.call(nativeStatementTemp617,nativeStatementTemp618);}());
return JSValue{};
}

JSValue method19(const JSValue& nativeStatementTemp627,const Args& nativeStatementTemp628,const Args& nativeStatementTemp629){
auto nativeStatementTemp630=nativeStatementTemp629[0];auto nativeStatementTemp631=nativeStatementTemp627;
JSValue direction=arg(nativeStatementTemp628,0);
return ([&]()->JSValue{auto nativeStatementTemp636=nativeStatementTemp630.get("Triangle");const auto nativeStatementTemp637=JSValue("isFrontFacing");const auto nativeStatementTemp633=nativeStatementTemp636.get(nativeStatementTemp637);const auto nativeStatementTemp634=nativeStatementTemp636;Args nativeStatementTemp635;nativeStatementTemp635.push_back(([&]()->JSValue{const auto nativeStatementTemp638=nativeStatementTemp631;const auto nativeStatementTemp639=JSValue("a");return nativeStatementTemp638.get(nativeStatementTemp639);}()));nativeStatementTemp635.push_back(([&]()->JSValue{const auto nativeStatementTemp640=nativeStatementTemp631;const auto nativeStatementTemp641=JSValue("b");return nativeStatementTemp640.get(nativeStatementTemp641);}()));nativeStatementTemp635.push_back(([&]()->JSValue{const auto nativeStatementTemp642=nativeStatementTemp631;const auto nativeStatementTemp643=JSValue("c");return nativeStatementTemp642.get(nativeStatementTemp643);}()));nativeStatementTemp635.push_back(direction);return nativeStatementTemp633.call(nativeStatementTemp634,nativeStatementTemp635);}());
return JSValue{};
}

JSValue method20(const JSValue& nativeStatementTemp644,const Args& nativeStatementTemp645,const Args& nativeStatementTemp646){
auto nativeStatementTemp647=nativeStatementTemp646[0];auto nativeStatementTemp648=nativeStatementTemp644;
JSValue box=arg(nativeStatementTemp645,0);
return ([&]()->JSValue{auto nativeStatementTemp653=box;const auto nativeStatementTemp654=JSValue("intersectsTriangle");const auto nativeStatementTemp650=nativeStatementTemp653.get(nativeStatementTemp654);const auto nativeStatementTemp651=nativeStatementTemp653;Args nativeStatementTemp652;nativeStatementTemp652.push_back(nativeStatementTemp648);return nativeStatementTemp650.call(nativeStatementTemp651,nativeStatementTemp652);}());
return JSValue{};
}

JSValue method21(const JSValue& nativeStatementTemp655,const Args& nativeStatementTemp656,const Args& nativeStatementTemp657){
auto nativeStatementTemp658=nativeStatementTemp657[0];auto nativeStatementTemp659=nativeStatementTemp655;
JSValue p=arg(nativeStatementTemp656,0);
JSValue target=arg(nativeStatementTemp656,1);
JSValue a=([&]()->JSValue{const auto nativeStatementTemp661=nativeStatementTemp659;const auto nativeStatementTemp662=JSValue("a");return nativeStatementTemp661.get(nativeStatementTemp662);}());
JSValue b=([&]()->JSValue{const auto nativeStatementTemp663=nativeStatementTemp659;const auto nativeStatementTemp664=JSValue("b");return nativeStatementTemp663.get(nativeStatementTemp664);}());
JSValue c=([&]()->JSValue{const auto nativeStatementTemp665=nativeStatementTemp659;const auto nativeStatementTemp666=JSValue("c");return nativeStatementTemp665.get(nativeStatementTemp666);}());
JSValue v=JSValue{};
JSValue w=JSValue{};
([&]()->JSValue{auto nativeStatementTemp670=nativeStatementTemp658.get("_vab");const auto nativeStatementTemp671=JSValue("subVectors");const auto nativeStatementTemp667=nativeStatementTemp670.get(nativeStatementTemp671);const auto nativeStatementTemp668=nativeStatementTemp670;Args nativeStatementTemp669;nativeStatementTemp669.push_back(b);nativeStatementTemp669.push_back(a);return nativeStatementTemp667.call(nativeStatementTemp668,nativeStatementTemp669);}());
([&]()->JSValue{auto nativeStatementTemp675=nativeStatementTemp658.get("_vac");const auto nativeStatementTemp676=JSValue("subVectors");const auto nativeStatementTemp672=nativeStatementTemp675.get(nativeStatementTemp676);const auto nativeStatementTemp673=nativeStatementTemp675;Args nativeStatementTemp674;nativeStatementTemp674.push_back(c);nativeStatementTemp674.push_back(a);return nativeStatementTemp672.call(nativeStatementTemp673,nativeStatementTemp674);}());
([&]()->JSValue{auto nativeStatementTemp680=nativeStatementTemp658.get("_vap");const auto nativeStatementTemp681=JSValue("subVectors");const auto nativeStatementTemp677=nativeStatementTemp680.get(nativeStatementTemp681);const auto nativeStatementTemp678=nativeStatementTemp680;Args nativeStatementTemp679;nativeStatementTemp679.push_back(p);nativeStatementTemp679.push_back(a);return nativeStatementTemp677.call(nativeStatementTemp678,nativeStatementTemp679);}());
JSValue d1=([&]()->JSValue{auto nativeStatementTemp685=nativeStatementTemp658.get("_vab");const auto nativeStatementTemp686=JSValue("dot");const auto nativeStatementTemp682=nativeStatementTemp685.get(nativeStatementTemp686);const auto nativeStatementTemp683=nativeStatementTemp685;Args nativeStatementTemp684;nativeStatementTemp684.push_back(nativeStatementTemp658.get("_vap"));return nativeStatementTemp682.call(nativeStatementTemp683,nativeStatementTemp684);}());
JSValue d2=([&]()->JSValue{auto nativeStatementTemp690=nativeStatementTemp658.get("_vac");const auto nativeStatementTemp691=JSValue("dot");const auto nativeStatementTemp687=nativeStatementTemp690.get(nativeStatementTemp691);const auto nativeStatementTemp688=nativeStatementTemp690;Args nativeStatementTemp689;nativeStatementTemp689.push_back(nativeStatementTemp658.get("_vap"));return nativeStatementTemp687.call(nativeStatementTemp688,nativeStatementTemp689);}());
if(([&]()->JSValue{const auto nativeStatementTemp692=([&]()->JSValue{const auto nativeStatementTemp693=d1;const auto nativeStatementTemp694=JSValue(0);return binary("<=",nativeStatementTemp693,nativeStatementTemp694);}());if(!nativeStatementTemp692.toBoolean())return nativeStatementTemp692;return ([&]()->JSValue{const auto nativeStatementTemp695=d2;const auto nativeStatementTemp696=JSValue(0);return binary("<=",nativeStatementTemp695,nativeStatementTemp696);}());}()).toBoolean()){
return ([&]()->JSValue{auto nativeStatementTemp700=target;const auto nativeStatementTemp701=JSValue("copy");const auto nativeStatementTemp697=nativeStatementTemp700.get(nativeStatementTemp701);const auto nativeStatementTemp698=nativeStatementTemp700;Args nativeStatementTemp699;nativeStatementTemp699.push_back(a);return nativeStatementTemp697.call(nativeStatementTemp698,nativeStatementTemp699);}());

}
([&]()->JSValue{auto nativeStatementTemp705=nativeStatementTemp658.get("_vbp");const auto nativeStatementTemp706=JSValue("subVectors");const auto nativeStatementTemp702=nativeStatementTemp705.get(nativeStatementTemp706);const auto nativeStatementTemp703=nativeStatementTemp705;Args nativeStatementTemp704;nativeStatementTemp704.push_back(p);nativeStatementTemp704.push_back(b);return nativeStatementTemp702.call(nativeStatementTemp703,nativeStatementTemp704);}());
JSValue d3=([&]()->JSValue{auto nativeStatementTemp710=nativeStatementTemp658.get("_vab");const auto nativeStatementTemp711=JSValue("dot");const auto nativeStatementTemp707=nativeStatementTemp710.get(nativeStatementTemp711);const auto nativeStatementTemp708=nativeStatementTemp710;Args nativeStatementTemp709;nativeStatementTemp709.push_back(nativeStatementTemp658.get("_vbp"));return nativeStatementTemp707.call(nativeStatementTemp708,nativeStatementTemp709);}());
JSValue d4=([&]()->JSValue{auto nativeStatementTemp715=nativeStatementTemp658.get("_vac");const auto nativeStatementTemp716=JSValue("dot");const auto nativeStatementTemp712=nativeStatementTemp715.get(nativeStatementTemp716);const auto nativeStatementTemp713=nativeStatementTemp715;Args nativeStatementTemp714;nativeStatementTemp714.push_back(nativeStatementTemp658.get("_vbp"));return nativeStatementTemp712.call(nativeStatementTemp713,nativeStatementTemp714);}());
if(([&]()->JSValue{const auto nativeStatementTemp717=([&]()->JSValue{const auto nativeStatementTemp718=d3;const auto nativeStatementTemp719=JSValue(0);return binary(">=",nativeStatementTemp718,nativeStatementTemp719);}());if(!nativeStatementTemp717.toBoolean())return nativeStatementTemp717;return ([&]()->JSValue{const auto nativeStatementTemp720=d4;const auto nativeStatementTemp721=d3;return binary("<=",nativeStatementTemp720,nativeStatementTemp721);}());}()).toBoolean()){
return ([&]()->JSValue{auto nativeStatementTemp725=target;const auto nativeStatementTemp726=JSValue("copy");const auto nativeStatementTemp722=nativeStatementTemp725.get(nativeStatementTemp726);const auto nativeStatementTemp723=nativeStatementTemp725;Args nativeStatementTemp724;nativeStatementTemp724.push_back(b);return nativeStatementTemp722.call(nativeStatementTemp723,nativeStatementTemp724);}());

}
JSValue vc=([&]()->JSValue{const auto nativeStatementTemp727=([&]()->JSValue{const auto nativeStatementTemp729=d1;const auto nativeStatementTemp730=d4;return binary("*",nativeStatementTemp729,nativeStatementTemp730);}());const auto nativeStatementTemp728=([&]()->JSValue{const auto nativeStatementTemp731=d3;const auto nativeStatementTemp732=d2;return binary("*",nativeStatementTemp731,nativeStatementTemp732);}());return binary("-",nativeStatementTemp727,nativeStatementTemp728);}());
if(([&]()->JSValue{const auto nativeStatementTemp733=([&]()->JSValue{const auto nativeStatementTemp734=([&]()->JSValue{const auto nativeStatementTemp735=([&]()->JSValue{const auto nativeStatementTemp736=vc;const auto nativeStatementTemp737=JSValue(0);return binary("<=",nativeStatementTemp736,nativeStatementTemp737);}());if(!nativeStatementTemp735.toBoolean())return nativeStatementTemp735;return ([&]()->JSValue{const auto nativeStatementTemp738=d1;const auto nativeStatementTemp739=JSValue(0);return binary(">=",nativeStatementTemp738,nativeStatementTemp739);}());}());if(!nativeStatementTemp734.toBoolean())return nativeStatementTemp734;return ([&]()->JSValue{const auto nativeStatementTemp740=d3;const auto nativeStatementTemp741=JSValue(0);return binary("<=",nativeStatementTemp740,nativeStatementTemp741);}());}());if(!nativeStatementTemp733.toBoolean())return nativeStatementTemp733;return ([&]()->JSValue{const auto nativeStatementTemp742=([&]()->JSValue{const auto nativeStatementTemp744=d1;const auto nativeStatementTemp745=d3;return binary("-",nativeStatementTemp744,nativeStatementTemp745);}());const auto nativeStatementTemp743=JSValue(0);return binary(">",nativeStatementTemp742,nativeStatementTemp743);}());}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp746=([&]()->JSValue{const auto nativeStatementTemp749=d1;const auto nativeStatementTemp750=([&]()->JSValue{const auto nativeStatementTemp751=d1;const auto nativeStatementTemp752=d3;return binary("-",nativeStatementTemp751,nativeStatementTemp752);}());return binary("/",nativeStatementTemp749,nativeStatementTemp750);}());v=nativeStatementTemp746;return nativeStatementTemp746;}());
return ([&]()->JSValue{auto nativeStatementTemp756=([&]()->JSValue{auto nativeStatementTemp761=target;const auto nativeStatementTemp762=JSValue("copy");const auto nativeStatementTemp758=nativeStatementTemp761.get(nativeStatementTemp762);const auto nativeStatementTemp759=nativeStatementTemp761;Args nativeStatementTemp760;nativeStatementTemp760.push_back(a);return nativeStatementTemp758.call(nativeStatementTemp759,nativeStatementTemp760);}());const auto nativeStatementTemp757=JSValue("addScaledVector");const auto nativeStatementTemp753=nativeStatementTemp756.get(nativeStatementTemp757);const auto nativeStatementTemp754=nativeStatementTemp756;Args nativeStatementTemp755;nativeStatementTemp755.push_back(nativeStatementTemp658.get("_vab"));nativeStatementTemp755.push_back(v);return nativeStatementTemp753.call(nativeStatementTemp754,nativeStatementTemp755);}());

}
([&]()->JSValue{auto nativeStatementTemp766=nativeStatementTemp658.get("_vcp");const auto nativeStatementTemp767=JSValue("subVectors");const auto nativeStatementTemp763=nativeStatementTemp766.get(nativeStatementTemp767);const auto nativeStatementTemp764=nativeStatementTemp766;Args nativeStatementTemp765;nativeStatementTemp765.push_back(p);nativeStatementTemp765.push_back(c);return nativeStatementTemp763.call(nativeStatementTemp764,nativeStatementTemp765);}());
JSValue d5=([&]()->JSValue{auto nativeStatementTemp771=nativeStatementTemp658.get("_vab");const auto nativeStatementTemp772=JSValue("dot");const auto nativeStatementTemp768=nativeStatementTemp771.get(nativeStatementTemp772);const auto nativeStatementTemp769=nativeStatementTemp771;Args nativeStatementTemp770;nativeStatementTemp770.push_back(nativeStatementTemp658.get("_vcp"));return nativeStatementTemp768.call(nativeStatementTemp769,nativeStatementTemp770);}());
JSValue d6=([&]()->JSValue{auto nativeStatementTemp776=nativeStatementTemp658.get("_vac");const auto nativeStatementTemp777=JSValue("dot");const auto nativeStatementTemp773=nativeStatementTemp776.get(nativeStatementTemp777);const auto nativeStatementTemp774=nativeStatementTemp776;Args nativeStatementTemp775;nativeStatementTemp775.push_back(nativeStatementTemp658.get("_vcp"));return nativeStatementTemp773.call(nativeStatementTemp774,nativeStatementTemp775);}());
if(([&]()->JSValue{const auto nativeStatementTemp778=([&]()->JSValue{const auto nativeStatementTemp779=d6;const auto nativeStatementTemp780=JSValue(0);return binary(">=",nativeStatementTemp779,nativeStatementTemp780);}());if(!nativeStatementTemp778.toBoolean())return nativeStatementTemp778;return ([&]()->JSValue{const auto nativeStatementTemp781=d5;const auto nativeStatementTemp782=d6;return binary("<=",nativeStatementTemp781,nativeStatementTemp782);}());}()).toBoolean()){
return ([&]()->JSValue{auto nativeStatementTemp786=target;const auto nativeStatementTemp787=JSValue("copy");const auto nativeStatementTemp783=nativeStatementTemp786.get(nativeStatementTemp787);const auto nativeStatementTemp784=nativeStatementTemp786;Args nativeStatementTemp785;nativeStatementTemp785.push_back(c);return nativeStatementTemp783.call(nativeStatementTemp784,nativeStatementTemp785);}());

}
JSValue vb=([&]()->JSValue{const auto nativeStatementTemp788=([&]()->JSValue{const auto nativeStatementTemp790=d5;const auto nativeStatementTemp791=d2;return binary("*",nativeStatementTemp790,nativeStatementTemp791);}());const auto nativeStatementTemp789=([&]()->JSValue{const auto nativeStatementTemp792=d1;const auto nativeStatementTemp793=d6;return binary("*",nativeStatementTemp792,nativeStatementTemp793);}());return binary("-",nativeStatementTemp788,nativeStatementTemp789);}());
if(([&]()->JSValue{const auto nativeStatementTemp794=([&]()->JSValue{const auto nativeStatementTemp795=([&]()->JSValue{const auto nativeStatementTemp796=vb;const auto nativeStatementTemp797=JSValue(0);return binary("<=",nativeStatementTemp796,nativeStatementTemp797);}());if(!nativeStatementTemp795.toBoolean())return nativeStatementTemp795;return ([&]()->JSValue{const auto nativeStatementTemp798=d2;const auto nativeStatementTemp799=JSValue(0);return binary(">=",nativeStatementTemp798,nativeStatementTemp799);}());}());if(!nativeStatementTemp794.toBoolean())return nativeStatementTemp794;return ([&]()->JSValue{const auto nativeStatementTemp800=d6;const auto nativeStatementTemp801=JSValue(0);return binary("<=",nativeStatementTemp800,nativeStatementTemp801);}());}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp802=([&]()->JSValue{const auto nativeStatementTemp805=d2;const auto nativeStatementTemp806=([&]()->JSValue{const auto nativeStatementTemp807=d2;const auto nativeStatementTemp808=d6;return binary("-",nativeStatementTemp807,nativeStatementTemp808);}());return binary("/",nativeStatementTemp805,nativeStatementTemp806);}());w=nativeStatementTemp802;return nativeStatementTemp802;}());
return ([&]()->JSValue{auto nativeStatementTemp812=([&]()->JSValue{auto nativeStatementTemp817=target;const auto nativeStatementTemp818=JSValue("copy");const auto nativeStatementTemp814=nativeStatementTemp817.get(nativeStatementTemp818);const auto nativeStatementTemp815=nativeStatementTemp817;Args nativeStatementTemp816;nativeStatementTemp816.push_back(a);return nativeStatementTemp814.call(nativeStatementTemp815,nativeStatementTemp816);}());const auto nativeStatementTemp813=JSValue("addScaledVector");const auto nativeStatementTemp809=nativeStatementTemp812.get(nativeStatementTemp813);const auto nativeStatementTemp810=nativeStatementTemp812;Args nativeStatementTemp811;nativeStatementTemp811.push_back(nativeStatementTemp658.get("_vac"));nativeStatementTemp811.push_back(w);return nativeStatementTemp809.call(nativeStatementTemp810,nativeStatementTemp811);}());

}
JSValue va=([&]()->JSValue{const auto nativeStatementTemp819=([&]()->JSValue{const auto nativeStatementTemp821=d3;const auto nativeStatementTemp822=d6;return binary("*",nativeStatementTemp821,nativeStatementTemp822);}());const auto nativeStatementTemp820=([&]()->JSValue{const auto nativeStatementTemp823=d5;const auto nativeStatementTemp824=d4;return binary("*",nativeStatementTemp823,nativeStatementTemp824);}());return binary("-",nativeStatementTemp819,nativeStatementTemp820);}());
if(([&]()->JSValue{const auto nativeStatementTemp825=([&]()->JSValue{const auto nativeStatementTemp826=([&]()->JSValue{const auto nativeStatementTemp827=va;const auto nativeStatementTemp828=JSValue(0);return binary("<=",nativeStatementTemp827,nativeStatementTemp828);}());if(!nativeStatementTemp826.toBoolean())return nativeStatementTemp826;return ([&]()->JSValue{const auto nativeStatementTemp829=([&]()->JSValue{const auto nativeStatementTemp831=d4;const auto nativeStatementTemp832=d3;return binary("-",nativeStatementTemp831,nativeStatementTemp832);}());const auto nativeStatementTemp830=JSValue(0);return binary(">=",nativeStatementTemp829,nativeStatementTemp830);}());}());if(!nativeStatementTemp825.toBoolean())return nativeStatementTemp825;return ([&]()->JSValue{const auto nativeStatementTemp833=([&]()->JSValue{const auto nativeStatementTemp835=d5;const auto nativeStatementTemp836=d6;return binary("-",nativeStatementTemp835,nativeStatementTemp836);}());const auto nativeStatementTemp834=JSValue(0);return binary(">=",nativeStatementTemp833,nativeStatementTemp834);}());}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp840=nativeStatementTemp658.get("_vbc");const auto nativeStatementTemp841=JSValue("subVectors");const auto nativeStatementTemp837=nativeStatementTemp840.get(nativeStatementTemp841);const auto nativeStatementTemp838=nativeStatementTemp840;Args nativeStatementTemp839;nativeStatementTemp839.push_back(c);nativeStatementTemp839.push_back(b);return nativeStatementTemp837.call(nativeStatementTemp838,nativeStatementTemp839);}());
([&]()->JSValue{const auto nativeStatementTemp842=([&]()->JSValue{const auto nativeStatementTemp845=([&]()->JSValue{const auto nativeStatementTemp847=d4;const auto nativeStatementTemp848=d3;return binary("-",nativeStatementTemp847,nativeStatementTemp848);}());const auto nativeStatementTemp846=([&]()->JSValue{const auto nativeStatementTemp849=([&]()->JSValue{const auto nativeStatementTemp851=d4;const auto nativeStatementTemp852=d3;return binary("-",nativeStatementTemp851,nativeStatementTemp852);}());const auto nativeStatementTemp850=([&]()->JSValue{const auto nativeStatementTemp853=d5;const auto nativeStatementTemp854=d6;return binary("-",nativeStatementTemp853,nativeStatementTemp854);}());return binary("+",nativeStatementTemp849,nativeStatementTemp850);}());return binary("/",nativeStatementTemp845,nativeStatementTemp846);}());w=nativeStatementTemp842;return nativeStatementTemp842;}());
return ([&]()->JSValue{auto nativeStatementTemp858=([&]()->JSValue{auto nativeStatementTemp863=target;const auto nativeStatementTemp864=JSValue("copy");const auto nativeStatementTemp860=nativeStatementTemp863.get(nativeStatementTemp864);const auto nativeStatementTemp861=nativeStatementTemp863;Args nativeStatementTemp862;nativeStatementTemp862.push_back(b);return nativeStatementTemp860.call(nativeStatementTemp861,nativeStatementTemp862);}());const auto nativeStatementTemp859=JSValue("addScaledVector");const auto nativeStatementTemp855=nativeStatementTemp858.get(nativeStatementTemp859);const auto nativeStatementTemp856=nativeStatementTemp858;Args nativeStatementTemp857;nativeStatementTemp857.push_back(nativeStatementTemp658.get("_vbc"));nativeStatementTemp857.push_back(w);return nativeStatementTemp855.call(nativeStatementTemp856,nativeStatementTemp857);}());

}
JSValue denom=([&]()->JSValue{const auto nativeStatementTemp865=JSValue(1);const auto nativeStatementTemp866=([&]()->JSValue{const auto nativeStatementTemp867=([&]()->JSValue{const auto nativeStatementTemp869=va;const auto nativeStatementTemp870=vb;return binary("+",nativeStatementTemp869,nativeStatementTemp870);}());const auto nativeStatementTemp868=vc;return binary("+",nativeStatementTemp867,nativeStatementTemp868);}());return binary("/",nativeStatementTemp865,nativeStatementTemp866);}());
([&]()->JSValue{const auto nativeStatementTemp871=([&]()->JSValue{const auto nativeStatementTemp874=vb;const auto nativeStatementTemp875=denom;return binary("*",nativeStatementTemp874,nativeStatementTemp875);}());v=nativeStatementTemp871;return nativeStatementTemp871;}());
([&]()->JSValue{const auto nativeStatementTemp876=([&]()->JSValue{const auto nativeStatementTemp879=vc;const auto nativeStatementTemp880=denom;return binary("*",nativeStatementTemp879,nativeStatementTemp880);}());w=nativeStatementTemp876;return nativeStatementTemp876;}());
return ([&]()->JSValue{auto nativeStatementTemp884=([&]()->JSValue{auto nativeStatementTemp889=([&]()->JSValue{auto nativeStatementTemp894=target;const auto nativeStatementTemp895=JSValue("copy");const auto nativeStatementTemp891=nativeStatementTemp894.get(nativeStatementTemp895);const auto nativeStatementTemp892=nativeStatementTemp894;Args nativeStatementTemp893;nativeStatementTemp893.push_back(a);return nativeStatementTemp891.call(nativeStatementTemp892,nativeStatementTemp893);}());const auto nativeStatementTemp890=JSValue("addScaledVector");const auto nativeStatementTemp886=nativeStatementTemp889.get(nativeStatementTemp890);const auto nativeStatementTemp887=nativeStatementTemp889;Args nativeStatementTemp888;nativeStatementTemp888.push_back(nativeStatementTemp658.get("_vab"));nativeStatementTemp888.push_back(v);return nativeStatementTemp886.call(nativeStatementTemp887,nativeStatementTemp888);}());const auto nativeStatementTemp885=JSValue("addScaledVector");const auto nativeStatementTemp881=nativeStatementTemp884.get(nativeStatementTemp885);const auto nativeStatementTemp882=nativeStatementTemp884;Args nativeStatementTemp883;nativeStatementTemp883.push_back(nativeStatementTemp658.get("_vac"));nativeStatementTemp883.push_back(w);return nativeStatementTemp881.call(nativeStatementTemp882,nativeStatementTemp883);}());
return JSValue{};
}

JSValue method22(const JSValue& nativeStatementTemp896,const Args& nativeStatementTemp897,const Args& nativeStatementTemp898){
auto nativeStatementTemp899=nativeStatementTemp898[0];auto nativeStatementTemp900=nativeStatementTemp896;
JSValue triangle=arg(nativeStatementTemp897,0);
return ([&]()->JSValue{const auto nativeStatementTemp902=([&]()->JSValue{const auto nativeStatementTemp903=([&]()->JSValue{auto nativeStatementTemp907=([&]()->JSValue{const auto nativeStatementTemp909=triangle;const auto nativeStatementTemp910=JSValue("a");return nativeStatementTemp909.get(nativeStatementTemp910);}());const auto nativeStatementTemp908=JSValue("equals");const auto nativeStatementTemp904=nativeStatementTemp907.get(nativeStatementTemp908);const auto nativeStatementTemp905=nativeStatementTemp907;Args nativeStatementTemp906;nativeStatementTemp906.push_back(([&]()->JSValue{const auto nativeStatementTemp911=nativeStatementTemp900;const auto nativeStatementTemp912=JSValue("a");return nativeStatementTemp911.get(nativeStatementTemp912);}()));return nativeStatementTemp904.call(nativeStatementTemp905,nativeStatementTemp906);}());if(!nativeStatementTemp903.toBoolean())return nativeStatementTemp903;return ([&]()->JSValue{auto nativeStatementTemp916=([&]()->JSValue{const auto nativeStatementTemp918=triangle;const auto nativeStatementTemp919=JSValue("b");return nativeStatementTemp918.get(nativeStatementTemp919);}());const auto nativeStatementTemp917=JSValue("equals");const auto nativeStatementTemp913=nativeStatementTemp916.get(nativeStatementTemp917);const auto nativeStatementTemp914=nativeStatementTemp916;Args nativeStatementTemp915;nativeStatementTemp915.push_back(([&]()->JSValue{const auto nativeStatementTemp920=nativeStatementTemp900;const auto nativeStatementTemp921=JSValue("b");return nativeStatementTemp920.get(nativeStatementTemp921);}()));return nativeStatementTemp913.call(nativeStatementTemp914,nativeStatementTemp915);}());}());if(!nativeStatementTemp902.toBoolean())return nativeStatementTemp902;return ([&]()->JSValue{auto nativeStatementTemp925=([&]()->JSValue{const auto nativeStatementTemp927=triangle;const auto nativeStatementTemp928=JSValue("c");return nativeStatementTemp927.get(nativeStatementTemp928);}());const auto nativeStatementTemp926=JSValue("equals");const auto nativeStatementTemp922=nativeStatementTemp925.get(nativeStatementTemp926);const auto nativeStatementTemp923=nativeStatementTemp925;Args nativeStatementTemp924;nativeStatementTemp924.push_back(([&]()->JSValue{const auto nativeStatementTemp929=nativeStatementTemp900;const auto nativeStatementTemp930=JSValue("c");return nativeStatementTemp929.get(nativeStatementTemp930);}()));return nativeStatementTemp922.call(nativeStatementTemp923,nativeStatementTemp924);}());}());
return JSValue{};
}
}
JSValue nativeTriangleModule(){using namespace TriangleManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Triangle module");auto module=JSValue::object(nullptr);
own(module,"Vector3",nativeVector3Constructor());
own(module,"Vector4",nativeVector4Constructor());
own(module,"_v0",([&]()->JSValue{const auto nativeStatementTemp931=module.get("Vector3");Args nativeStatementTemp932;return nativeStatementTemp931.construct(nativeStatementTemp932);}()));
own(module,"_v1",([&]()->JSValue{const auto nativeStatementTemp933=module.get("Vector3");Args nativeStatementTemp934;return nativeStatementTemp933.construct(nativeStatementTemp934);}()));
own(module,"_v2",([&]()->JSValue{const auto nativeStatementTemp935=module.get("Vector3");Args nativeStatementTemp936;return nativeStatementTemp935.construct(nativeStatementTemp936);}()));
own(module,"_v3",([&]()->JSValue{const auto nativeStatementTemp937=module.get("Vector3");Args nativeStatementTemp938;return nativeStatementTemp937.construct(nativeStatementTemp938);}()));
own(module,"_vab",([&]()->JSValue{const auto nativeStatementTemp939=module.get("Vector3");Args nativeStatementTemp940;return nativeStatementTemp939.construct(nativeStatementTemp940);}()));
own(module,"_vac",([&]()->JSValue{const auto nativeStatementTemp941=module.get("Vector3");Args nativeStatementTemp942;return nativeStatementTemp941.construct(nativeStatementTemp942);}()));
own(module,"_vbc",([&]()->JSValue{const auto nativeStatementTemp943=module.get("Vector3");Args nativeStatementTemp944;return nativeStatementTemp943.construct(nativeStatementTemp944);}()));
own(module,"_vap",([&]()->JSValue{const auto nativeStatementTemp945=module.get("Vector3");Args nativeStatementTemp946;return nativeStatementTemp945.construct(nativeStatementTemp946);}()));
own(module,"_vbp",([&]()->JSValue{const auto nativeStatementTemp947=module.get("Vector3");Args nativeStatementTemp948;return nativeStatementTemp947.construct(nativeStatementTemp948);}()));
own(module,"_vcp",([&]()->JSValue{const auto nativeStatementTemp949=module.get("Vector3");Args nativeStatementTemp950;return nativeStatementTemp949.construct(nativeStatementTemp950);}()));
own(module,"_v40",([&]()->JSValue{const auto nativeStatementTemp951=module.get("Vector4");Args nativeStatementTemp952;return nativeStatementTemp951.construct(nativeStatementTemp952);}()));
own(module,"_v41",([&]()->JSValue{const auto nativeStatementTemp953=module.get("Vector4");Args nativeStatementTemp954;return nativeStatementTemp953.construct(nativeStatementTemp954);}()));
own(module,"_v42",([&]()->JSValue{const auto nativeStatementTemp955=module.get("Vector4");Args nativeStatementTemp956;return nativeStatementTemp955.construct(nativeStatementTemp956);}()));
auto ctor=JSValue::constructor(method0,{module},"Triangle",0);ctor.setFunctionSourceText("class Triangle {\r\n\r\n\t/**\r\n\t * Constructs a new triangle.\r\n\t *\r\n\t * @param {Vector3} [a=(0,0,0)] - The first corner of the triangle.\r\n\t * @param {Vector3} [b=(0,0,0)] - The second corner of the triangle.\r\n\t * @param {Vector3} [c=(0,0,0)] - The third corner of the triangle.\r\n\t */\r\n\tconstructor( a = new Vector3(), b = new Vector3(), c = new Vector3() ) {\r\n\r\n\t\t/**\r\n\t\t * The first corner of the triangle.\r\n\t\t *\r\n\t\t * @type {Vector3}\r\n\t\t */\r\n\t\tthis.a = a;\r\n\r\n\t\t/**\r\n\t\t * The second corner of the triangle.\r\n\t\t *\r\n\t\t * @type {Vector3}\r\n\t\t */\r\n\t\tthis.b = b;\r\n\r\n\t\t/**\r\n\t\t * The third corner of the triangle.\r\n\t\t *\r\n\t\t * @type {Vector3}\r\n\t\t */\r\n\t\tthis.c = c;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Computes the normal vector of a triangle.\r\n\t *\r\n\t * @param {Vector3} a - The first corner of the triangle.\r\n\t * @param {Vector3} b - The second corner of the triangle.\r\n\t * @param {Vector3} c - The third corner of the triangle.\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {Vector3} The triangle's normal.\r\n\t */\r\n\tstatic getNormal( a, b, c, target ) {\r\n\r\n\t\ttarget.subVectors( c, b );\r\n\t\t_v0.subVectors( a, b );\r\n\t\ttarget.cross( _v0 );\r\n\r\n\t\tconst targetLengthSq = target.lengthSq();\r\n\t\tif ( targetLengthSq > 0 ) {\r\n\r\n\t\t\treturn target.multiplyScalar( 1 / Math.sqrt( targetLengthSq ) );\r\n\r\n\t\t}\r\n\r\n\t\treturn target.set( 0, 0, 0 );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Computes a barycentric coordinates from the given vector.\r\n\t * Returns `null` if the triangle is degenerate.\r\n\t *\r\n\t * @param {Vector3} point - A point in 3D space.\r\n\t * @param {Vector3} a - The first corner of the triangle.\r\n\t * @param {Vector3} b - The second corner of the triangle.\r\n\t * @param {Vector3} c - The third corner of the triangle.\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {?Vector3} The barycentric coordinates for the given point\r\n\t */\r\n\tstatic getBarycoord( point, a, b, c, target ) {\r\n\r\n\t\t_v0.subVectors( c, a );\r\n\t\t_v1.subVectors( b, a );\r\n\t\t_v2.subVectors( point, a );\r\n\r\n\t\tconst dot00 = _v0.dot( _v0 );\r\n\t\tconst dot01 = _v0.dot( _v1 );\r\n\t\tconst dot02 = _v0.dot( _v2 );\r\n\t\tconst dot11 = _v1.dot( _v1 );\r\n\t\tconst dot12 = _v1.dot( _v2 );\r\n\r\n\t\tconst denom = ( dot00 * dot11 - dot01 * dot01 );\r\n\r\n\t\t// collinear or singular triangle\r\n\t\tif ( denom === 0 ) {\r\n\r\n\t\t\ttarget.set( 0, 0, 0 );\r\n\t\t\treturn null;\r\n\r\n\t\t}\r\n\r\n\t\tconst invDenom = 1 / denom;\r\n\t\tconst u = ( dot11 * dot02 - dot01 * dot12 ) * invDenom;\r\n\t\tconst v = ( dot00 * dot12 - dot01 * dot02 ) * invDenom;\r\n\r\n\t\t// barycentric coordinates must always sum to 1\r\n\t\treturn target.set( 1 - u - v, v, u );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if the given point, when projected onto the plane of the\r\n\t * triangle, lies within the triangle.\r\n\t *\r\n\t * @param {Vector3} point - The point in 3D space to test.\r\n\t * @param {Vector3} a - The first corner of the triangle.\r\n\t * @param {Vector3} b - The second corner of the triangle.\r\n\t * @param {Vector3} c - The third corner of the triangle.\r\n\t * @return {boolean} Whether the given point, when projected onto the plane of the\r\n\t * triangle, lies within the triangle or not.\r\n\t */\r\n\tstatic containsPoint( point, a, b, c ) {\r\n\r\n\t\t// if the triangle is degenerate then we can't contain a point\r\n\t\tif ( this.getBarycoord( point, a, b, c, _v3 ) === null ) {\r\n\r\n\t\t\treturn false;\r\n\r\n\t\t}\r\n\r\n\t\treturn ( _v3.x >= 0 ) && ( _v3.y >= 0 ) && ( ( _v3.x + _v3.y ) <= 1 );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Computes the value barycentrically interpolated for the given point on the\r\n\t * triangle. Returns `null` if the triangle is degenerate.\r\n\t *\r\n\t * @param {Vector3} point - Position of interpolated point.\r\n\t * @param {Vector3} p1 - The first corner of the triangle.\r\n\t * @param {Vector3} p2 - The second corner of the triangle.\r\n\t * @param {Vector3} p3 - The third corner of the triangle.\r\n\t * @param {Vector3} v1 - Value to interpolate of first vertex.\r\n\t * @param {Vector3} v2 - Value to interpolate of second vertex.\r\n\t * @param {Vector3} v3 - Value to interpolate of third vertex.\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {?Vector3} The interpolated value.\r\n\t */\r\n\tstatic getInterpolation( point, p1, p2, p3, v1, v2, v3, target ) {\r\n\r\n\t\tif ( this.getBarycoord( point, p1, p2, p3, _v3 ) === null ) {\r\n\r\n\t\t\ttarget.x = 0;\r\n\t\t\ttarget.y = 0;\r\n\t\t\tif ( 'z' in target ) target.z = 0;\r\n\t\t\tif ( 'w' in target ) target.w = 0;\r\n\t\t\treturn null;\r\n\r\n\t\t}\r\n\r\n\t\ttarget.setScalar( 0 );\r\n\t\ttarget.addScaledVector( v1, _v3.x );\r\n\t\ttarget.addScaledVector( v2, _v3.y );\r\n\t\ttarget.addScaledVector( v3, _v3.z );\r\n\r\n\t\treturn target;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Computes the value barycentrically interpolated for the given attribute and indices.\r\n\t *\r\n\t * @param {BufferAttribute} attr - The attribute to interpolate.\r\n\t * @param {number} i1 - Index of first vertex.\r\n\t * @param {number} i2 - Index of second vertex.\r\n\t * @param {number} i3 - Index of third vertex.\r\n\t * @param {Vector3} barycoord - The barycoordinate value to use to interpolate.\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {Vector3} The interpolated attribute value.\r\n\t */\r\n\tstatic getInterpolatedAttribute( attr, i1, i2, i3, barycoord, target ) {\r\n\r\n\t\t_v40.setScalar( 0 );\r\n\t\t_v41.setScalar( 0 );\r\n\t\t_v42.setScalar( 0 );\r\n\r\n\t\t_v40.fromBufferAttribute( attr, i1 );\r\n\t\t_v41.fromBufferAttribute( attr, i2 );\r\n\t\t_v42.fromBufferAttribute( attr, i3 );\r\n\r\n\t\ttarget.setScalar( 0 );\r\n\t\ttarget.addScaledVector( _v40, barycoord.x );\r\n\t\ttarget.addScaledVector( _v41, barycoord.y );\r\n\t\ttarget.addScaledVector( _v42, barycoord.z );\r\n\r\n\t\treturn target;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if the triangle is oriented towards the given direction.\r\n\t *\r\n\t * @param {Vector3} a - The first corner of the triangle.\r\n\t * @param {Vector3} b - The second corner of the triangle.\r\n\t * @param {Vector3} c - The third corner of the triangle.\r\n\t * @param {Vector3} direction - The (normalized) direction vector.\r\n\t * @return {boolean} Whether the triangle is oriented towards the given direction or not.\r\n\t */\r\n\tstatic isFrontFacing( a, b, c, direction ) {\r\n\r\n\t\t_v0.subVectors( c, b );\r\n\t\t_v1.subVectors( a, b );\r\n\r\n\t\t// strictly front facing\r\n\t\treturn _v0.cross( _v1 ).dot( direction ) < 0;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Sets the triangle's vertices by copying the given values.\r\n\t *\r\n\t * @param {Vector3} a - The first corner of the triangle.\r\n\t * @param {Vector3} b - The second corner of the triangle.\r\n\t * @param {Vector3} c - The third corner of the triangle.\r\n\t * @return {Triangle} A reference to this triangle.\r\n\t */\r\n\tset( a, b, c ) {\r\n\r\n\t\tthis.a.copy( a );\r\n\t\tthis.b.copy( b );\r\n\t\tthis.c.copy( c );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Sets the triangle's vertices by copying the given array values.\r\n\t *\r\n\t * @param {Array<Vector3>} points - An array with 3D points.\r\n\t * @param {number} i0 - The array index representing the first corner of the triangle.\r\n\t * @param {number} i1 - The array index representing the second corner of the triangle.\r\n\t * @param {number} i2 - The array index representing the third corner of the triangle.\r\n\t * @return {Triangle} A reference to this triangle.\r\n\t */\r\n\tsetFromPointsAndIndices( points, i0, i1, i2 ) {\r\n\r\n\t\tthis.a.copy( points[ i0 ] );\r\n\t\tthis.b.copy( points[ i1 ] );\r\n\t\tthis.c.copy( points[ i2 ] );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Sets the triangle's vertices by copying the given attribute values.\r\n\t *\r\n\t * @param {BufferAttribute} attribute - A buffer attribute with 3D points data.\r\n\t * @param {number} i0 - The attribute index representing the first corner of the triangle.\r\n\t * @param {number} i1 - The attribute index representing the second corner of the triangle.\r\n\t * @param {number} i2 - The attribute index representing the third corner of the triangle.\r\n\t * @return {Triangle} A reference to this triangle.\r\n\t */\r\n\tsetFromAttributeAndIndices( attribute, i0, i1, i2 ) {\r\n\r\n\t\tthis.a.fromBufferAttribute( attribute, i0 );\r\n\t\tthis.b.fromBufferAttribute( attribute, i1 );\r\n\t\tthis.c.fromBufferAttribute( attribute, i2 );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns a new triangle with copied values from this instance.\r\n\t *\r\n\t * @return {Triangle} A clone of this instance.\r\n\t */\r\n\tclone() {\r\n\r\n\t\treturn new this.constructor().copy( this );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Copies the values of the given triangle to this instance.\r\n\t *\r\n\t * @param {Triangle} triangle - The triangle to copy.\r\n\t * @return {Triangle} A reference to this triangle.\r\n\t */\r\n\tcopy( triangle ) {\r\n\r\n\t\tthis.a.copy( triangle.a );\r\n\t\tthis.b.copy( triangle.b );\r\n\t\tthis.c.copy( triangle.c );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Computes the area of the triangle.\r\n\t *\r\n\t * @return {number} The triangle's area.\r\n\t */\r\n\tgetArea() {\r\n\r\n\t\t_v0.subVectors( this.c, this.b );\r\n\t\t_v1.subVectors( this.a, this.b );\r\n\r\n\t\treturn _v0.cross( _v1 ).length() * 0.5;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Computes the midpoint of the triangle.\r\n\t *\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {Vector3} The triangle's midpoint.\r\n\t */\r\n\tgetMidpoint( target ) {\r\n\r\n\t\treturn target.addVectors( this.a, this.b ).add( this.c ).multiplyScalar( 1 / 3 );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Computes the normal of the triangle.\r\n\t *\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {Vector3} The triangle's normal.\r\n\t */\r\n\tgetNormal( target ) {\r\n\r\n\t\treturn Triangle.getNormal( this.a, this.b, this.c, target );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Computes a plane the triangle lies within.\r\n\t *\r\n\t * @param {Plane} target - The target vector that is used to store the method's result.\r\n\t * @return {Plane} The plane the triangle lies within.\r\n\t */\r\n\tgetPlane( target ) {\r\n\r\n\t\treturn target.setFromCoplanarPoints( this.a, this.b, this.c );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Computes a barycentric coordinates from the given vector.\r\n\t * Returns `null` if the triangle is degenerate.\r\n\t *\r\n\t * @param {Vector3} point - A point in 3D space.\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {?Vector3} The barycentric coordinates for the given point\r\n\t */\r\n\tgetBarycoord( point, target ) {\r\n\r\n\t\treturn Triangle.getBarycoord( point, this.a, this.b, this.c, target );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Computes the value barycentrically interpolated for the given point on the\r\n\t * triangle. Returns `null` if the triangle is degenerate.\r\n\t *\r\n\t * @param {Vector3} point - Position of interpolated point.\r\n\t * @param {Vector3} v1 - Value to interpolate of first vertex.\r\n\t * @param {Vector3} v2 - Value to interpolate of second vertex.\r\n\t * @param {Vector3} v3 - Value to interpolate of third vertex.\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {?Vector3} The interpolated value.\r\n\t */\r\n\tgetInterpolation( point, v1, v2, v3, target ) {\r\n\r\n\t\treturn Triangle.getInterpolation( point, this.a, this.b, this.c, v1, v2, v3, target );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if the given point, when projected onto the plane of the\r\n\t * triangle, lies within the triangle.\r\n\t *\r\n\t * @param {Vector3} point - The point in 3D space to test.\r\n\t * @return {boolean} Whether the given point, when projected onto the plane of the\r\n\t * triangle, lies within the triangle or not.\r\n\t */\r\n\tcontainsPoint( point ) {\r\n\r\n\t\treturn Triangle.containsPoint( point, this.a, this.b, this.c );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if the triangle is oriented towards the given direction.\r\n\t *\r\n\t * @param {Vector3} direction - The (normalized) direction vector.\r\n\t * @return {boolean} Whether the triangle is oriented towards the given direction or not.\r\n\t */\r\n\tisFrontFacing( direction ) {\r\n\r\n\t\treturn Triangle.isFrontFacing( this.a, this.b, this.c, direction );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if this triangle intersects with the given box.\r\n\t *\r\n\t * @param {Box3} box - The box to intersect.\r\n\t * @return {boolean} Whether this triangle intersects with the given box or not.\r\n\t */\r\n\tintersectsBox( box ) {\r\n\r\n\t\treturn box.intersectsTriangle( this );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns the closest point on the triangle to the given point.\r\n\t *\r\n\t * @param {Vector3} p - The point to compute the closest point for.\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {Vector3} The closest point on the triangle.\r\n\t */\r\n\tclosestPointToPoint( p, target ) {\r\n\r\n\t\tconst a = this.a, b = this.b, c = this.c;\r\n\t\tlet v, w;\r\n\r\n\t\t// algorithm thanks to Real-Time Collision Detection by Christer Ericson,\r\n\t\t// published by Morgan Kaufmann Publishers, (c) 2005 Elsevier Inc.,\r\n\t\t// under the accompanying license; see chapter 5.1.5 for detailed explanation.\r\n\t\t// basically, we're distinguishing which of the voronoi regions of the triangle\r\n\t\t// the point lies in with the minimum amount of redundant computation.\r\n\r\n\t\t_vab.subVectors( b, a );\r\n\t\t_vac.subVectors( c, a );\r\n\t\t_vap.subVectors( p, a );\r\n\t\tconst d1 = _vab.dot( _vap );\r\n\t\tconst d2 = _vac.dot( _vap );\r\n\t\tif ( d1 <= 0 && d2 <= 0 ) {\r\n\r\n\t\t\t// vertex region of A; barycentric coords (1, 0, 0)\r\n\t\t\treturn target.copy( a );\r\n\r\n\t\t}\r\n\r\n\t\t_vbp.subVectors( p, b );\r\n\t\tconst d3 = _vab.dot( _vbp );\r\n\t\tconst d4 = _vac.dot( _vbp );\r\n\t\tif ( d3 >= 0 && d4 <= d3 ) {\r\n\r\n\t\t\t// vertex region of B; barycentric coords (0, 1, 0)\r\n\t\t\treturn target.copy( b );\r\n\r\n\t\t}\r\n\r\n\t\tconst vc = d1 * d4 - d3 * d2;\r\n\r\n\t\tif ( vc <= 0 && d1 >= 0 && d3 <= 0 && d1 - d3 > 0 ) { // modification of the algorithm: d1 - d3 is the squared length of AB, so skip this region if a and b coincide\r\n\r\n\t\t\tv = d1 / ( d1 - d3 );\r\n\t\t\t// edge region of AB; barycentric coords (1-v, v, 0)\r\n\t\t\treturn target.copy( a ).addScaledVector( _vab, v );\r\n\r\n\t\t}\r\n\r\n\t\t_vcp.subVectors( p, c );\r\n\t\tconst d5 = _vab.dot( _vcp );\r\n\t\tconst d6 = _vac.dot( _vcp );\r\n\t\tif ( d6 >= 0 && d5 <= d6 ) {\r\n\r\n\t\t\t// vertex region of C; barycentric coords (0, 0, 1)\r\n\t\t\treturn target.copy( c );\r\n\r\n\t\t}\r\n\r\n\t\tconst vb = d5 * d2 - d1 * d6;\r\n\t\tif ( vb <= 0 && d2 >= 0 && d6 <= 0 ) {\r\n\r\n\t\t\tw = d2 / ( d2 - d6 );\r\n\t\t\t// edge region of AC; barycentric coords (1-w, 0, w)\r\n\t\t\treturn target.copy( a ).addScaledVector( _vac, w );\r\n\r\n\t\t}\r\n\r\n\t\tconst va = d3 * d6 - d5 * d4;\r\n\t\tif ( va <= 0 && ( d4 - d3 ) >= 0 && ( d5 - d6 ) >= 0 ) {\r\n\r\n\t\t\t_vbc.subVectors( c, b );\r\n\t\t\tw = ( d4 - d3 ) / ( ( d4 - d3 ) + ( d5 - d6 ) );\r\n\t\t\t// edge region of BC; barycentric coords (0, 1-w, w)\r\n\t\t\treturn target.copy( b ).addScaledVector( _vbc, w ); // edge region of BC\r\n\r\n\t\t}\r\n\r\n\t\t// face region\r\n\t\tconst denom = 1 / ( va + vb + vc );\r\n\t\t// u = va * denom\r\n\t\tv = vb * denom;\r\n\t\tw = vc * denom;\r\n\r\n\t\treturn target.copy( a ).addScaledVector( _vab, v ).addScaledVector( _vac, w );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if this triangle is equal with the given one.\r\n\t *\r\n\t * @param {Triangle} triangle - The triangle to test for equality.\r\n\t * @return {boolean} Whether this triangle is equal with the given one.\r\n\t */\r\n\tequals( triangle ) {\r\n\r\n\t\treturn triangle.a.equals( this.a ) && triangle.b.equals( this.b ) && triangle.c.equals( this.c );\r\n\r\n\t}\r\n\r\n}");auto prototype=ctor.get("prototype");own(module,"Triangle",ctor);own(module,"_home",prototype);
{auto function=JSValue::function(method1,{module},"getNormal",4);function.setFunctionSourceText("getNormal( a, b, c, target ) {\r\n\r\n\t\ttarget.subVectors( c, b );\r\n\t\t_v0.subVectors( a, b );\r\n\t\ttarget.cross( _v0 );\r\n\r\n\t\tconst targetLengthSq = target.lengthSq();\r\n\t\tif ( targetLengthSq > 0 ) {\r\n\r\n\t\t\treturn target.multiplyScalar( 1 / Math.sqrt( targetLengthSq ) );\r\n\r\n\t\t}\r\n\r\n\t\treturn target.set( 0, 0, 0 );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;ctor.defineProperty("getNormal",descriptor);}
{auto function=JSValue::function(method2,{module},"getBarycoord",5);function.setFunctionSourceText("getBarycoord( point, a, b, c, target ) {\r\n\r\n\t\t_v0.subVectors( c, a );\r\n\t\t_v1.subVectors( b, a );\r\n\t\t_v2.subVectors( point, a );\r\n\r\n\t\tconst dot00 = _v0.dot( _v0 );\r\n\t\tconst dot01 = _v0.dot( _v1 );\r\n\t\tconst dot02 = _v0.dot( _v2 );\r\n\t\tconst dot11 = _v1.dot( _v1 );\r\n\t\tconst dot12 = _v1.dot( _v2 );\r\n\r\n\t\tconst denom = ( dot00 * dot11 - dot01 * dot01 );\r\n\r\n\t\t// collinear or singular triangle\r\n\t\tif ( denom === 0 ) {\r\n\r\n\t\t\ttarget.set( 0, 0, 0 );\r\n\t\t\treturn null;\r\n\r\n\t\t}\r\n\r\n\t\tconst invDenom = 1 / denom;\r\n\t\tconst u = ( dot11 * dot02 - dot01 * dot12 ) * invDenom;\r\n\t\tconst v = ( dot00 * dot12 - dot01 * dot02 ) * invDenom;\r\n\r\n\t\t// barycentric coordinates must always sum to 1\r\n\t\treturn target.set( 1 - u - v, v, u );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;ctor.defineProperty("getBarycoord",descriptor);}
{auto function=JSValue::function(method3,{module},"containsPoint",4);function.setFunctionSourceText("containsPoint( point, a, b, c ) {\r\n\r\n\t\t// if the triangle is degenerate then we can't contain a point\r\n\t\tif ( this.getBarycoord( point, a, b, c, _v3 ) === null ) {\r\n\r\n\t\t\treturn false;\r\n\r\n\t\t}\r\n\r\n\t\treturn ( _v3.x >= 0 ) && ( _v3.y >= 0 ) && ( ( _v3.x + _v3.y ) <= 1 );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;ctor.defineProperty("containsPoint",descriptor);}
{auto function=JSValue::function(method4,{module},"getInterpolation",8);function.setFunctionSourceText("getInterpolation( point, p1, p2, p3, v1, v2, v3, target ) {\r\n\r\n\t\tif ( this.getBarycoord( point, p1, p2, p3, _v3 ) === null ) {\r\n\r\n\t\t\ttarget.x = 0;\r\n\t\t\ttarget.y = 0;\r\n\t\t\tif ( 'z' in target ) target.z = 0;\r\n\t\t\tif ( 'w' in target ) target.w = 0;\r\n\t\t\treturn null;\r\n\r\n\t\t}\r\n\r\n\t\ttarget.setScalar( 0 );\r\n\t\ttarget.addScaledVector( v1, _v3.x );\r\n\t\ttarget.addScaledVector( v2, _v3.y );\r\n\t\ttarget.addScaledVector( v3, _v3.z );\r\n\r\n\t\treturn target;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;ctor.defineProperty("getInterpolation",descriptor);}
{auto function=JSValue::function(method5,{module},"getInterpolatedAttribute",6);function.setFunctionSourceText("getInterpolatedAttribute( attr, i1, i2, i3, barycoord, target ) {\r\n\r\n\t\t_v40.setScalar( 0 );\r\n\t\t_v41.setScalar( 0 );\r\n\t\t_v42.setScalar( 0 );\r\n\r\n\t\t_v40.fromBufferAttribute( attr, i1 );\r\n\t\t_v41.fromBufferAttribute( attr, i2 );\r\n\t\t_v42.fromBufferAttribute( attr, i3 );\r\n\r\n\t\ttarget.setScalar( 0 );\r\n\t\ttarget.addScaledVector( _v40, barycoord.x );\r\n\t\ttarget.addScaledVector( _v41, barycoord.y );\r\n\t\ttarget.addScaledVector( _v42, barycoord.z );\r\n\r\n\t\treturn target;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;ctor.defineProperty("getInterpolatedAttribute",descriptor);}
{auto function=JSValue::function(method6,{module},"isFrontFacing",4);function.setFunctionSourceText("isFrontFacing( a, b, c, direction ) {\r\n\r\n\t\t_v0.subVectors( c, b );\r\n\t\t_v1.subVectors( a, b );\r\n\r\n\t\t// strictly front facing\r\n\t\treturn _v0.cross( _v1 ).dot( direction ) < 0;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;ctor.defineProperty("isFrontFacing",descriptor);}
{auto function=JSValue::function(method7,{module},"set",3);function.setFunctionSourceText("set( a, b, c ) {\r\n\r\n\t\tthis.a.copy( a );\r\n\t\tthis.b.copy( b );\r\n\t\tthis.c.copy( c );\r\n\r\n\t\treturn this;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("set",descriptor);}
{auto function=JSValue::function(method8,{module},"setFromPointsAndIndices",4);function.setFunctionSourceText("setFromPointsAndIndices( points, i0, i1, i2 ) {\r\n\r\n\t\tthis.a.copy( points[ i0 ] );\r\n\t\tthis.b.copy( points[ i1 ] );\r\n\t\tthis.c.copy( points[ i2 ] );\r\n\r\n\t\treturn this;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("setFromPointsAndIndices",descriptor);}
{auto function=JSValue::function(method9,{module},"setFromAttributeAndIndices",4);function.setFunctionSourceText("setFromAttributeAndIndices( attribute, i0, i1, i2 ) {\r\n\r\n\t\tthis.a.fromBufferAttribute( attribute, i0 );\r\n\t\tthis.b.fromBufferAttribute( attribute, i1 );\r\n\t\tthis.c.fromBufferAttribute( attribute, i2 );\r\n\r\n\t\treturn this;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("setFromAttributeAndIndices",descriptor);}
{auto function=JSValue::function(method10,{module},"clone",0);function.setFunctionSourceText("clone() {\r\n\r\n\t\treturn new this.constructor().copy( this );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("clone",descriptor);}
{auto function=JSValue::function(method11,{module},"copy",1);function.setFunctionSourceText("copy( triangle ) {\r\n\r\n\t\tthis.a.copy( triangle.a );\r\n\t\tthis.b.copy( triangle.b );\r\n\t\tthis.c.copy( triangle.c );\r\n\r\n\t\treturn this;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("copy",descriptor);}
{auto function=JSValue::function(method12,{module},"getArea",0);function.setFunctionSourceText("getArea() {\r\n\r\n\t\t_v0.subVectors( this.c, this.b );\r\n\t\t_v1.subVectors( this.a, this.b );\r\n\r\n\t\treturn _v0.cross( _v1 ).length() * 0.5;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("getArea",descriptor);}
{auto function=JSValue::function(method13,{module},"getMidpoint",1);function.setFunctionSourceText("getMidpoint( target ) {\r\n\r\n\t\treturn target.addVectors( this.a, this.b ).add( this.c ).multiplyScalar( 1 / 3 );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("getMidpoint",descriptor);}
{auto function=JSValue::function(method14,{module},"getNormal",1);function.setFunctionSourceText("getNormal( target ) {\r\n\r\n\t\treturn Triangle.getNormal( this.a, this.b, this.c, target );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("getNormal",descriptor);}
{auto function=JSValue::function(method15,{module},"getPlane",1);function.setFunctionSourceText("getPlane( target ) {\r\n\r\n\t\treturn target.setFromCoplanarPoints( this.a, this.b, this.c );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("getPlane",descriptor);}
{auto function=JSValue::function(method16,{module},"getBarycoord",2);function.setFunctionSourceText("getBarycoord( point, target ) {\r\n\r\n\t\treturn Triangle.getBarycoord( point, this.a, this.b, this.c, target );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("getBarycoord",descriptor);}
{auto function=JSValue::function(method17,{module},"getInterpolation",5);function.setFunctionSourceText("getInterpolation( point, v1, v2, v3, target ) {\r\n\r\n\t\treturn Triangle.getInterpolation( point, this.a, this.b, this.c, v1, v2, v3, target );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("getInterpolation",descriptor);}
{auto function=JSValue::function(method18,{module},"containsPoint",1);function.setFunctionSourceText("containsPoint( point ) {\r\n\r\n\t\treturn Triangle.containsPoint( point, this.a, this.b, this.c );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("containsPoint",descriptor);}
{auto function=JSValue::function(method19,{module},"isFrontFacing",1);function.setFunctionSourceText("isFrontFacing( direction ) {\r\n\r\n\t\treturn Triangle.isFrontFacing( this.a, this.b, this.c, direction );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("isFrontFacing",descriptor);}
{auto function=JSValue::function(method20,{module},"intersectsBox",1);function.setFunctionSourceText("intersectsBox( box ) {\r\n\r\n\t\treturn box.intersectsTriangle( this );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("intersectsBox",descriptor);}
{auto function=JSValue::function(method21,{module},"closestPointToPoint",2);function.setFunctionSourceText("closestPointToPoint( p, target ) {\r\n\r\n\t\tconst a = this.a, b = this.b, c = this.c;\r\n\t\tlet v, w;\r\n\r\n\t\t// algorithm thanks to Real-Time Collision Detection by Christer Ericson,\r\n\t\t// published by Morgan Kaufmann Publishers, (c) 2005 Elsevier Inc.,\r\n\t\t// under the accompanying license; see chapter 5.1.5 for detailed explanation.\r\n\t\t// basically, we're distinguishing which of the voronoi regions of the triangle\r\n\t\t// the point lies in with the minimum amount of redundant computation.\r\n\r\n\t\t_vab.subVectors( b, a );\r\n\t\t_vac.subVectors( c, a );\r\n\t\t_vap.subVectors( p, a );\r\n\t\tconst d1 = _vab.dot( _vap );\r\n\t\tconst d2 = _vac.dot( _vap );\r\n\t\tif ( d1 <= 0 && d2 <= 0 ) {\r\n\r\n\t\t\t// vertex region of A; barycentric coords (1, 0, 0)\r\n\t\t\treturn target.copy( a );\r\n\r\n\t\t}\r\n\r\n\t\t_vbp.subVectors( p, b );\r\n\t\tconst d3 = _vab.dot( _vbp );\r\n\t\tconst d4 = _vac.dot( _vbp );\r\n\t\tif ( d3 >= 0 && d4 <= d3 ) {\r\n\r\n\t\t\t// vertex region of B; barycentric coords (0, 1, 0)\r\n\t\t\treturn target.copy( b );\r\n\r\n\t\t}\r\n\r\n\t\tconst vc = d1 * d4 - d3 * d2;\r\n\r\n\t\tif ( vc <= 0 && d1 >= 0 && d3 <= 0 && d1 - d3 > 0 ) { // modification of the algorithm: d1 - d3 is the squared length of AB, so skip this region if a and b coincide\r\n\r\n\t\t\tv = d1 / ( d1 - d3 );\r\n\t\t\t// edge region of AB; barycentric coords (1-v, v, 0)\r\n\t\t\treturn target.copy( a ).addScaledVector( _vab, v );\r\n\r\n\t\t}\r\n\r\n\t\t_vcp.subVectors( p, c );\r\n\t\tconst d5 = _vab.dot( _vcp );\r\n\t\tconst d6 = _vac.dot( _vcp );\r\n\t\tif ( d6 >= 0 && d5 <= d6 ) {\r\n\r\n\t\t\t// vertex region of C; barycentric coords (0, 0, 1)\r\n\t\t\treturn target.copy( c );\r\n\r\n\t\t}\r\n\r\n\t\tconst vb = d5 * d2 - d1 * d6;\r\n\t\tif ( vb <= 0 && d2 >= 0 && d6 <= 0 ) {\r\n\r\n\t\t\tw = d2 / ( d2 - d6 );\r\n\t\t\t// edge region of AC; barycentric coords (1-w, 0, w)\r\n\t\t\treturn target.copy( a ).addScaledVector( _vac, w );\r\n\r\n\t\t}\r\n\r\n\t\tconst va = d3 * d6 - d5 * d4;\r\n\t\tif ( va <= 0 && ( d4 - d3 ) >= 0 && ( d5 - d6 ) >= 0 ) {\r\n\r\n\t\t\t_vbc.subVectors( c, b );\r\n\t\t\tw = ( d4 - d3 ) / ( ( d4 - d3 ) + ( d5 - d6 ) );\r\n\t\t\t// edge region of BC; barycentric coords (0, 1-w, w)\r\n\t\t\treturn target.copy( b ).addScaledVector( _vbc, w ); // edge region of BC\r\n\r\n\t\t}\r\n\r\n\t\t// face region\r\n\t\tconst denom = 1 / ( va + vb + vc );\r\n\t\t// u = va * denom\r\n\t\tv = vb * denom;\r\n\t\tw = vc * denom;\r\n\r\n\t\treturn target.copy( a ).addScaledVector( _vab, v ).addScaledVector( _vac, w );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("closestPointToPoint",descriptor);}
{auto function=JSValue::function(method22,{module},"equals",1);function.setFunctionSourceText("equals( triangle ) {\r\n\r\n\t\treturn triangle.a.equals( this.a ) && triangle.b.equals( this.b ) && triangle.c.equals( this.c );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("equals",descriptor);}
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();own(facade,"Triangle",ctor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeTriangleBinding(const std::string& name){const auto keeper=nativeTriangleModule();const auto state=TriangleManaged::lexical().lock();return name=="Triangle"?state->get(name):JSValue{};}
JSValue nativeTriangleConstructor(){return nativeTriangleBinding("Triangle");}
