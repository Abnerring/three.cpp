#include "math/Ray.h"
#include "math/Box3.h"
#include "math/MathUtils.h"
#include "math/Matrix4.h"
#include "math/Plane.h"
#include "math/Sphere.h"
#include <cmath>

Ray& Ray::set(const Vector3& origin, const Vector3& direction) {
    this->origin.copy(origin);
    this->direction.copy(direction);
    return *this;
}

Ray& Ray::copy(const Ray& ray) {
    this->origin.copy(ray.origin);
    this->direction.copy(ray.direction);
    return *this;
}

Vector3& Ray::at(HIGH_PRECISION t, Vector3& target) const {
    return target.copy(this->origin).addScaledVector(this->direction, t);
}

Ray& Ray::lookAt(const Vector3& v) {
    this->direction.copy(v).sub(this->origin).normalize();
    return *this;
}

Ray& Ray::recast(HIGH_PRECISION t) {
    Vector3 _vector;
    this->origin.copy(this->at(t, _vector));
    return *this;
}

Vector3& Ray::closestPointToPoint(const Vector3& point, Vector3& target) const {
    target.subVectors(point, this->origin);
    const auto directionDistance = target.dot(this->direction);
    if (directionDistance < 0) {
        return target.copy(this->origin);
    }
    return target.copy(this->origin).addScaledVector(this->direction, directionDistance);
}

HIGH_PRECISION Ray::distanceToPoint(const Vector3& point) const {
    return std::sqrt(this->distanceSqToPoint(point));
}

HIGH_PRECISION Ray::distanceSqToPoint(const Vector3& point) const {
    Vector3 _vector;
    const auto directionDistance = _vector.subVectors(point, this->origin).dot(this->direction);
    if (directionDistance < 0) {
        return this->origin.distanceToSquared(point);
    }
    _vector.copy(this->origin).addScaledVector(this->direction, directionDistance);
    return _vector.distanceToSquared(point);
}

HIGH_PRECISION Ray::distanceSqToSegment(const Vector3& v0, const Vector3& v1,
                                        Vector3* optionalPointOnRay,
                                        Vector3* optionalPointOnSegment) const {
    Vector3 _segCenter, _segDir, _diff;
    _segCenter.copy(v0).add(v1).multiplyScalar(0.5);
    _segDir.copy(v1).sub(v0).normalize();
    _diff.copy(this->origin).sub(_segCenter);
    const auto segExtent = v0.distanceTo(v1) * 0.5;
    const auto a01 = -this->direction.dot(_segDir);
    const auto b0 = _diff.dot(this->direction);
    const auto b1 = -_diff.dot(_segDir);
    const auto c = _diff.lengthSq();
    const auto det = std::abs(1 - a01 * a01);
    HIGH_PRECISION s0, s1, sqrDist, extDet;
    if (det > 0) {
        s0 = a01 * b1 - b0;
        s1 = a01 * b0 - b1;
        extDet = segExtent * det;
        if (s0 >= 0) {
            if (s1 >= -extDet) {
                if (s1 <= extDet) {
                    const auto invDet = 1 / det;
                    s0 *= invDet;
                    s1 *= invDet;
                    sqrDist = s0 * (s0 + a01 * s1 + 2 * b0) + s1 * (a01 * s0 + s1 + 2 * b1) + c;
                } else {
                    s1 = segExtent;
                    s0 = MathUtils::max(0, -(a01 * s1 + b0));
                    sqrDist = -s0 * s0 + s1 * (s1 + 2 * b1) + c;
                }
            } else {
                s1 = -segExtent;
                s0 = MathUtils::max(0, -(a01 * s1 + b0));
                sqrDist = -s0 * s0 + s1 * (s1 + 2 * b1) + c;
            }
        } else {
            if (s1 <= -extDet) {
                s0 = MathUtils::max(0, -(-a01 * segExtent + b0));
                s1 = (s0 > 0) ? -segExtent
                              : MathUtils::min(MathUtils::max(-segExtent, -b1), segExtent);
                sqrDist = -s0 * s0 + s1 * (s1 + 2 * b1) + c;
            } else if (s1 <= extDet) {
                s0 = 0;
                s1 = MathUtils::min(MathUtils::max(-segExtent, -b1), segExtent);
                sqrDist = s1 * (s1 + 2 * b1) + c;
            } else {
                s0 = MathUtils::max(0, -(a01 * segExtent + b0));
                s1 = (s0 > 0) ? segExtent
                              : MathUtils::min(MathUtils::max(-segExtent, -b1), segExtent);
                sqrDist = -s0 * s0 + s1 * (s1 + 2 * b1) + c;
            }
        }
    } else {
        s1 = (a01 > 0) ? -segExtent : segExtent;
        s0 = MathUtils::max(0, -(a01 * s1 + b0));
        sqrDist = -s0 * s0 + s1 * (s1 + 2 * b1) + c;
    }
    if (optionalPointOnRay) {
        optionalPointOnRay->copy(this->origin).addScaledVector(this->direction, s0);
    }
    if (optionalPointOnSegment) {
        optionalPointOnSegment->copy(_segCenter).addScaledVector(_segDir, s1);
    }
    return sqrDist;
}

Vector3* Ray::intersectSphere(const Sphere& sphere, Vector3& target) const {
    Vector3 _vector;
    if (sphere.radius < 0)
        return nullptr;
    _vector.subVectors(sphere.center, this->origin);
    const auto tca = _vector.dot(this->direction);
    const auto d2 = _vector.dot(_vector) - tca * tca;
    const auto radius2 = sphere.radius * sphere.radius;
    if (d2 > radius2)
        return nullptr;
    const auto thc = std::sqrt(radius2 - d2);
    const auto t0 = tca - thc;
    const auto t1 = tca + thc;
    if (t1 < 0)
        return nullptr;
    if (t0 < 0)
        return &this->at(t1, target);
    return &this->at(t0, target);
}

bool Ray::intersectsSphere(const Sphere& sphere) const {
    if (sphere.radius < 0)
        return false;
    return this->distanceSqToPoint(sphere.center) <= (sphere.radius * sphere.radius);
}

bool Ray::intersectsPlane(const Plane& plane) const {
    const auto distToPoint = plane.distanceToPoint(this->origin);
    if (distToPoint == 0) {
        return true;
    }
    const auto denominator = plane.normal.dot(this->direction);
    if (denominator * distToPoint < 0) {
        return true;
    }
    return false;
}

Vector3* Ray::intersectBox(const Box3& box, Vector3& target) const {
    HIGH_PRECISION tmin, tmax, tymin, tymax, tzmin, tzmax;
    const auto invdirx = 1 / this->direction.x, invdiry = 1 / this->direction.y,
               invdirz = 1 / this->direction.z;
    const auto& origin = this->origin;
    if (invdirx >= 0) {
        tmin = (box.min.x - origin.x) * invdirx;
        tmax = (box.max.x - origin.x) * invdirx;
    } else {
        tmin = (box.max.x - origin.x) * invdirx;
        tmax = (box.min.x - origin.x) * invdirx;
    }
    if (invdiry >= 0) {
        tymin = (box.min.y - origin.y) * invdiry;
        tymax = (box.max.y - origin.y) * invdiry;
    } else {
        tymin = (box.max.y - origin.y) * invdiry;
        tymax = (box.min.y - origin.y) * invdiry;
    }
    if ((tmin > tymax) || (tymin > tmax))
        return nullptr;
    if (tymin > tmin || std::isnan(tmin))
        tmin = tymin;
    if (tymax < tmax || std::isnan(tmax))
        tmax = tymax;
    if (invdirz >= 0) {
        tzmin = (box.min.z - origin.z) * invdirz;
        tzmax = (box.max.z - origin.z) * invdirz;
    } else {
        tzmin = (box.max.z - origin.z) * invdirz;
        tzmax = (box.min.z - origin.z) * invdirz;
    }
    if ((tmin > tzmax) || (tzmin > tmax))
        return nullptr;
    if (tzmin > tmin || tmin != tmin)
        tmin = tzmin;
    if (tzmax < tmax || tmax != tmax)
        tmax = tzmax;
    if (tmax < 0)
        return nullptr;
    return &this->at(tmin >= 0 ? tmin : tmax, target);
}

bool Ray::intersectsBox(const Box3& box) const {
    Vector3 _vector;
    return this->intersectBox(box, _vector) != nullptr;
}

Vector3* Ray::intersectTriangle(const Vector3& a, const Vector3& b, const Vector3& c,
                                bool backfaceCulling, Vector3& target) const {
    const auto& origin = this->origin;
    const auto& direction = this->direction;
    const auto dx = direction.x;
    const auto dy = direction.y;
    const auto dz = direction.z;
    const auto aox = a.x - origin.x, aoy = a.y - origin.y, aoz = a.z - origin.z;
    const auto box = b.x - origin.x, boy = b.y - origin.y, boz = b.z - origin.z;
    const auto cox = c.x - origin.x, coy = c.y - origin.y, coz = c.z - origin.z;
    const auto adx = std::abs(dx), ady = std::abs(dy), adz = std::abs(dz);
    HIGH_PRECISION dkx, dky, dkz;
    HIGH_PRECISION akx, aky, akz, bkx, bky, bkz, ckx, cky, ckz;
    if (adx >= ady && adx >= adz) {
        dkz = dx;
        akz = aox;
        bkz = box;
        ckz = cox;
        if (dx >= 0) {
            dkx = dy;
            dky = dz;
            akx = aoy;
            aky = aoz;
            bkx = boy;
            bky = boz;
            ckx = coy;
            cky = coz;
        } else {
            dkx = dz;
            dky = dy;
            akx = aoz;
            aky = aoy;
            bkx = boz;
            bky = boy;
            ckx = coz;
            cky = coy;
        }
    } else if (ady >= adz) {
        dkz = dy;
        akz = aoy;
        bkz = boy;
        ckz = coy;
        if (dy >= 0) {
            dkx = dz;
            dky = dx;
            akx = aoz;
            aky = aox;
            bkx = boz;
            bky = box;
            ckx = coz;
            cky = cox;
        } else {
            dkx = dx;
            dky = dz;
            akx = aox;
            aky = aoz;
            bkx = box;
            bky = boz;
            ckx = cox;
            cky = coz;
        }
    } else {
        dkz = dz;
        akz = aoz;
        bkz = boz;
        ckz = coz;
        if (dz >= 0) {
            dkx = dx;
            dky = dy;
            akx = aox;
            aky = aoy;
            bkx = box;
            bky = boy;
            ckx = cox;
            cky = coy;
        } else {
            dkx = dy;
            dky = dx;
            akx = aoy;
            aky = aox;
            bkx = boy;
            bky = box;
            ckx = coy;
            cky = cox;
        }
    }
    if (dkz == 0)
        return nullptr;
    const auto sx = dkx / dkz, sy = dky / dkz, sz = 1 / dkz;
    const auto ax = akx - sx * akz, ay = aky - sy * akz;
    const auto bx = bkx - sx * bkz, by = bky - sy * bkz;
    const auto cx = ckx - sx * ckz, cy = cky - sy * ckz;
    const auto u = cx * by - cy * bx;
    const auto v = ax * cy - ay * cx;
    const auto w = bx * ay - by * ax;
    if (backfaceCulling) {
        if (u < 0 || v < 0 || w < 0)
            return nullptr;
    } else {
        if ((u < 0 || v < 0 || w < 0) && (u > 0 || v > 0 || w > 0))
            return nullptr;
    }
    const auto det = u + v + w;
    if (det == 0)
        return nullptr;
    const auto tScaled = sz * (u * akz + v * bkz + w * ckz);
    if (det > 0 ? tScaled < 0 : tScaled > 0)
        return nullptr;
    return &this->at(tScaled / det, target);
}

Ray& Ray::applyMatrix4(const Matrix4& matrix4) {
    this->origin.applyMatrix4(matrix4);
    this->direction.transformDirection(matrix4);
    return *this;
}

bool Ray::equals(const Ray& ray) const {
    return ray.origin.equals(this->origin) && ray.direction.equals(this->direction);
}

Ray::Ray(const Vector3& origin, const Vector3& direction) : origin(origin), direction(direction) {}
Ray Ray::clone() const {
    return Ray().copy(*this);
}
std::optional<HIGH_PRECISION> Ray::distanceToPlane(const Plane& plane) const {
    const auto denominator = plane.normal.dot(direction);
    if (denominator == 0) {
        if (plane.distanceToPoint(origin) == 0)
            return 0;
        return std::nullopt;
    }
    const auto t = -(origin.dot(plane.normal) + plane.constant) / denominator;
    if (t >= 0)
        return t;
    return std::nullopt;
}
Vector3* Ray::intersectPlane(const Plane& plane, Vector3& target) const {
    const auto t = distanceToPlane(plane);
    if (!t)
        return nullptr;
    return &at(*t, target);
}

// Complete native managed Ray source class; typed implementation above remains intact.
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
namespace RayManaged {

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
JSValue origin=arg(nativeStatementTemp1,0);
if(origin.isUndefined())origin=([&]()->JSValue{const auto nativeStatementTemp6=nativeStatementTemp3.get("Vector3");Args nativeStatementTemp7;return nativeStatementTemp6.construct(nativeStatementTemp7);}());
JSValue direction=arg(nativeStatementTemp1,1);
if(direction.isUndefined())direction=([&]()->JSValue{const auto nativeStatementTemp8=nativeStatementTemp3.get("Vector3");Args nativeStatementTemp9;nativeStatementTemp9.push_back(JSValue(0));nativeStatementTemp9.push_back(JSValue(0));nativeStatementTemp9.push_back(([&]()->JSValue{const auto nativeStatementTemp10=JSValue(1);return -nativeStatementTemp10.toNumber();}()));return nativeStatementTemp8.construct(nativeStatementTemp9);}());
([&]()->JSValue{auto nativeStatementTemp11=nativeStatementTemp4;const auto nativeStatementTemp12=JSValue("origin");const auto nativeStatementTemp13=origin;setReference(nativeStatementTemp11,nativeStatementTemp12,nativeStatementTemp13);return nativeStatementTemp13;}());
([&]()->JSValue{auto nativeStatementTemp16=nativeStatementTemp4;const auto nativeStatementTemp17=JSValue("direction");const auto nativeStatementTemp18=direction;setReference(nativeStatementTemp16,nativeStatementTemp17,nativeStatementTemp18);return nativeStatementTemp18;}());
return JSValue{};
}

JSValue method1(const JSValue& nativeStatementTemp21,const Args& nativeStatementTemp22,const Args& nativeStatementTemp23){
auto nativeStatementTemp24=nativeStatementTemp23[0];auto nativeStatementTemp25=nativeStatementTemp21;
JSValue origin=arg(nativeStatementTemp22,0);
JSValue direction=arg(nativeStatementTemp22,1);
([&]()->JSValue{auto nativeStatementTemp30=([&]()->JSValue{const auto nativeStatementTemp32=nativeStatementTemp25;const auto nativeStatementTemp33=JSValue("origin");return nativeStatementTemp32.get(nativeStatementTemp33);}());const auto nativeStatementTemp31=JSValue("copy");const auto nativeStatementTemp27=nativeStatementTemp30.get(nativeStatementTemp31);const auto nativeStatementTemp28=nativeStatementTemp30;Args nativeStatementTemp29;nativeStatementTemp29.push_back(origin);return nativeStatementTemp27.call(nativeStatementTemp28,nativeStatementTemp29);}());
([&]()->JSValue{auto nativeStatementTemp37=([&]()->JSValue{const auto nativeStatementTemp39=nativeStatementTemp25;const auto nativeStatementTemp40=JSValue("direction");return nativeStatementTemp39.get(nativeStatementTemp40);}());const auto nativeStatementTemp38=JSValue("copy");const auto nativeStatementTemp34=nativeStatementTemp37.get(nativeStatementTemp38);const auto nativeStatementTemp35=nativeStatementTemp37;Args nativeStatementTemp36;nativeStatementTemp36.push_back(direction);return nativeStatementTemp34.call(nativeStatementTemp35,nativeStatementTemp36);}());
return nativeStatementTemp25;
return JSValue{};
}

JSValue method2(const JSValue& nativeStatementTemp41,const Args& nativeStatementTemp42,const Args& nativeStatementTemp43){
auto nativeStatementTemp44=nativeStatementTemp43[0];auto nativeStatementTemp45=nativeStatementTemp41;
JSValue ray=arg(nativeStatementTemp42,0);
([&]()->JSValue{auto nativeStatementTemp50=([&]()->JSValue{const auto nativeStatementTemp52=nativeStatementTemp45;const auto nativeStatementTemp53=JSValue("origin");return nativeStatementTemp52.get(nativeStatementTemp53);}());const auto nativeStatementTemp51=JSValue("copy");const auto nativeStatementTemp47=nativeStatementTemp50.get(nativeStatementTemp51);const auto nativeStatementTemp48=nativeStatementTemp50;Args nativeStatementTemp49;nativeStatementTemp49.push_back(([&]()->JSValue{const auto nativeStatementTemp54=ray;const auto nativeStatementTemp55=JSValue("origin");return nativeStatementTemp54.get(nativeStatementTemp55);}()));return nativeStatementTemp47.call(nativeStatementTemp48,nativeStatementTemp49);}());
([&]()->JSValue{auto nativeStatementTemp59=([&]()->JSValue{const auto nativeStatementTemp61=nativeStatementTemp45;const auto nativeStatementTemp62=JSValue("direction");return nativeStatementTemp61.get(nativeStatementTemp62);}());const auto nativeStatementTemp60=JSValue("copy");const auto nativeStatementTemp56=nativeStatementTemp59.get(nativeStatementTemp60);const auto nativeStatementTemp57=nativeStatementTemp59;Args nativeStatementTemp58;nativeStatementTemp58.push_back(([&]()->JSValue{const auto nativeStatementTemp63=ray;const auto nativeStatementTemp64=JSValue("direction");return nativeStatementTemp63.get(nativeStatementTemp64);}()));return nativeStatementTemp56.call(nativeStatementTemp57,nativeStatementTemp58);}());
return nativeStatementTemp45;
return JSValue{};
}

JSValue method3(const JSValue& nativeStatementTemp65,const Args& nativeStatementTemp66,const Args& nativeStatementTemp67){
auto nativeStatementTemp68=nativeStatementTemp67[0];auto nativeStatementTemp69=nativeStatementTemp65;
JSValue t=arg(nativeStatementTemp66,0);
JSValue target=arg(nativeStatementTemp66,1);
return ([&]()->JSValue{auto nativeStatementTemp74=([&]()->JSValue{auto nativeStatementTemp79=target;const auto nativeStatementTemp80=JSValue("copy");const auto nativeStatementTemp76=nativeStatementTemp79.get(nativeStatementTemp80);const auto nativeStatementTemp77=nativeStatementTemp79;Args nativeStatementTemp78;nativeStatementTemp78.push_back(([&]()->JSValue{const auto nativeStatementTemp81=nativeStatementTemp69;const auto nativeStatementTemp82=JSValue("origin");return nativeStatementTemp81.get(nativeStatementTemp82);}()));return nativeStatementTemp76.call(nativeStatementTemp77,nativeStatementTemp78);}());const auto nativeStatementTemp75=JSValue("addScaledVector");const auto nativeStatementTemp71=nativeStatementTemp74.get(nativeStatementTemp75);const auto nativeStatementTemp72=nativeStatementTemp74;Args nativeStatementTemp73;nativeStatementTemp73.push_back(([&]()->JSValue{const auto nativeStatementTemp83=nativeStatementTemp69;const auto nativeStatementTemp84=JSValue("direction");return nativeStatementTemp83.get(nativeStatementTemp84);}()));nativeStatementTemp73.push_back(t);return nativeStatementTemp71.call(nativeStatementTemp72,nativeStatementTemp73);}());
return JSValue{};
}

JSValue method4(const JSValue& nativeStatementTemp85,const Args& nativeStatementTemp86,const Args& nativeStatementTemp87){
auto nativeStatementTemp88=nativeStatementTemp87[0];auto nativeStatementTemp89=nativeStatementTemp85;
JSValue v=arg(nativeStatementTemp86,0);
([&]()->JSValue{auto nativeStatementTemp94=([&]()->JSValue{auto nativeStatementTemp99=([&]()->JSValue{auto nativeStatementTemp104=([&]()->JSValue{const auto nativeStatementTemp106=nativeStatementTemp89;const auto nativeStatementTemp107=JSValue("direction");return nativeStatementTemp106.get(nativeStatementTemp107);}());const auto nativeStatementTemp105=JSValue("copy");const auto nativeStatementTemp101=nativeStatementTemp104.get(nativeStatementTemp105);const auto nativeStatementTemp102=nativeStatementTemp104;Args nativeStatementTemp103;nativeStatementTemp103.push_back(v);return nativeStatementTemp101.call(nativeStatementTemp102,nativeStatementTemp103);}());const auto nativeStatementTemp100=JSValue("sub");const auto nativeStatementTemp96=nativeStatementTemp99.get(nativeStatementTemp100);const auto nativeStatementTemp97=nativeStatementTemp99;Args nativeStatementTemp98;nativeStatementTemp98.push_back(([&]()->JSValue{const auto nativeStatementTemp108=nativeStatementTemp89;const auto nativeStatementTemp109=JSValue("origin");return nativeStatementTemp108.get(nativeStatementTemp109);}()));return nativeStatementTemp96.call(nativeStatementTemp97,nativeStatementTemp98);}());const auto nativeStatementTemp95=JSValue("normalize");const auto nativeStatementTemp91=nativeStatementTemp94.get(nativeStatementTemp95);const auto nativeStatementTemp92=nativeStatementTemp94;Args nativeStatementTemp93;return nativeStatementTemp91.call(nativeStatementTemp92,nativeStatementTemp93);}());
return nativeStatementTemp89;
return JSValue{};
}

JSValue method5(const JSValue& nativeStatementTemp110,const Args& nativeStatementTemp111,const Args& nativeStatementTemp112){
auto nativeStatementTemp113=nativeStatementTemp112[0];auto nativeStatementTemp114=nativeStatementTemp110;
JSValue t=arg(nativeStatementTemp111,0);
([&]()->JSValue{auto nativeStatementTemp119=([&]()->JSValue{const auto nativeStatementTemp121=nativeStatementTemp114;const auto nativeStatementTemp122=JSValue("origin");return nativeStatementTemp121.get(nativeStatementTemp122);}());const auto nativeStatementTemp120=JSValue("copy");const auto nativeStatementTemp116=nativeStatementTemp119.get(nativeStatementTemp120);const auto nativeStatementTemp117=nativeStatementTemp119;Args nativeStatementTemp118;nativeStatementTemp118.push_back(([&]()->JSValue{auto nativeStatementTemp126=nativeStatementTemp114;const auto nativeStatementTemp127=JSValue("at");const auto nativeStatementTemp123=nativeStatementTemp126.get(nativeStatementTemp127);const auto nativeStatementTemp124=nativeStatementTemp126;Args nativeStatementTemp125;nativeStatementTemp125.push_back(t);nativeStatementTemp125.push_back(nativeStatementTemp113.get("_vector"));return nativeStatementTemp123.call(nativeStatementTemp124,nativeStatementTemp125);}()));return nativeStatementTemp116.call(nativeStatementTemp117,nativeStatementTemp118);}());
return nativeStatementTemp114;
return JSValue{};
}

JSValue method6(const JSValue& nativeStatementTemp128,const Args& nativeStatementTemp129,const Args& nativeStatementTemp130){
auto nativeStatementTemp131=nativeStatementTemp130[0];auto nativeStatementTemp132=nativeStatementTemp128;
JSValue point=arg(nativeStatementTemp129,0);
JSValue target=arg(nativeStatementTemp129,1);
([&]()->JSValue{auto nativeStatementTemp137=target;const auto nativeStatementTemp138=JSValue("subVectors");const auto nativeStatementTemp134=nativeStatementTemp137.get(nativeStatementTemp138);const auto nativeStatementTemp135=nativeStatementTemp137;Args nativeStatementTemp136;nativeStatementTemp136.push_back(point);nativeStatementTemp136.push_back(([&]()->JSValue{const auto nativeStatementTemp139=nativeStatementTemp132;const auto nativeStatementTemp140=JSValue("origin");return nativeStatementTemp139.get(nativeStatementTemp140);}()));return nativeStatementTemp134.call(nativeStatementTemp135,nativeStatementTemp136);}());
JSValue directionDistance=([&]()->JSValue{auto nativeStatementTemp144=target;const auto nativeStatementTemp145=JSValue("dot");const auto nativeStatementTemp141=nativeStatementTemp144.get(nativeStatementTemp145);const auto nativeStatementTemp142=nativeStatementTemp144;Args nativeStatementTemp143;nativeStatementTemp143.push_back(([&]()->JSValue{const auto nativeStatementTemp146=nativeStatementTemp132;const auto nativeStatementTemp147=JSValue("direction");return nativeStatementTemp146.get(nativeStatementTemp147);}()));return nativeStatementTemp141.call(nativeStatementTemp142,nativeStatementTemp143);}());
if(([&]()->JSValue{const auto nativeStatementTemp148=directionDistance;const auto nativeStatementTemp149=JSValue(0);return binary("<",nativeStatementTemp148,nativeStatementTemp149);}()).toBoolean()){
return ([&]()->JSValue{auto nativeStatementTemp153=target;const auto nativeStatementTemp154=JSValue("copy");const auto nativeStatementTemp150=nativeStatementTemp153.get(nativeStatementTemp154);const auto nativeStatementTemp151=nativeStatementTemp153;Args nativeStatementTemp152;nativeStatementTemp152.push_back(([&]()->JSValue{const auto nativeStatementTemp155=nativeStatementTemp132;const auto nativeStatementTemp156=JSValue("origin");return nativeStatementTemp155.get(nativeStatementTemp156);}()));return nativeStatementTemp150.call(nativeStatementTemp151,nativeStatementTemp152);}());

}
return ([&]()->JSValue{auto nativeStatementTemp160=([&]()->JSValue{auto nativeStatementTemp165=target;const auto nativeStatementTemp166=JSValue("copy");const auto nativeStatementTemp162=nativeStatementTemp165.get(nativeStatementTemp166);const auto nativeStatementTemp163=nativeStatementTemp165;Args nativeStatementTemp164;nativeStatementTemp164.push_back(([&]()->JSValue{const auto nativeStatementTemp167=nativeStatementTemp132;const auto nativeStatementTemp168=JSValue("origin");return nativeStatementTemp167.get(nativeStatementTemp168);}()));return nativeStatementTemp162.call(nativeStatementTemp163,nativeStatementTemp164);}());const auto nativeStatementTemp161=JSValue("addScaledVector");const auto nativeStatementTemp157=nativeStatementTemp160.get(nativeStatementTemp161);const auto nativeStatementTemp158=nativeStatementTemp160;Args nativeStatementTemp159;nativeStatementTemp159.push_back(([&]()->JSValue{const auto nativeStatementTemp169=nativeStatementTemp132;const auto nativeStatementTemp170=JSValue("direction");return nativeStatementTemp169.get(nativeStatementTemp170);}()));nativeStatementTemp159.push_back(directionDistance);return nativeStatementTemp157.call(nativeStatementTemp158,nativeStatementTemp159);}());
return JSValue{};
}

JSValue method7(const JSValue& nativeStatementTemp171,const Args& nativeStatementTemp172,const Args& nativeStatementTemp173){
auto nativeStatementTemp174=nativeStatementTemp173[0];auto nativeStatementTemp175=nativeStatementTemp171;
JSValue point=arg(nativeStatementTemp172,0);
return ([&]()->JSValue{auto nativeStatementTemp180=nativeGlobalBinding("Math");const auto nativeStatementTemp181=JSValue("sqrt");const auto nativeStatementTemp177=nativeStatementTemp180.get(nativeStatementTemp181);const auto nativeStatementTemp178=nativeStatementTemp180;Args nativeStatementTemp179;nativeStatementTemp179.push_back(([&]()->JSValue{auto nativeStatementTemp185=nativeStatementTemp175;const auto nativeStatementTemp186=JSValue("distanceSqToPoint");const auto nativeStatementTemp182=nativeStatementTemp185.get(nativeStatementTemp186);const auto nativeStatementTemp183=nativeStatementTemp185;Args nativeStatementTemp184;nativeStatementTemp184.push_back(point);return nativeStatementTemp182.call(nativeStatementTemp183,nativeStatementTemp184);}()));return nativeStatementTemp177.call(nativeStatementTemp178,nativeStatementTemp179);}());
return JSValue{};
}

JSValue method8(const JSValue& nativeStatementTemp187,const Args& nativeStatementTemp188,const Args& nativeStatementTemp189){
auto nativeStatementTemp190=nativeStatementTemp189[0];auto nativeStatementTemp191=nativeStatementTemp187;
JSValue point=arg(nativeStatementTemp188,0);
JSValue directionDistance=([&]()->JSValue{auto nativeStatementTemp196=([&]()->JSValue{auto nativeStatementTemp201=nativeStatementTemp190.get("_vector");const auto nativeStatementTemp202=JSValue("subVectors");const auto nativeStatementTemp198=nativeStatementTemp201.get(nativeStatementTemp202);const auto nativeStatementTemp199=nativeStatementTemp201;Args nativeStatementTemp200;nativeStatementTemp200.push_back(point);nativeStatementTemp200.push_back(([&]()->JSValue{const auto nativeStatementTemp203=nativeStatementTemp191;const auto nativeStatementTemp204=JSValue("origin");return nativeStatementTemp203.get(nativeStatementTemp204);}()));return nativeStatementTemp198.call(nativeStatementTemp199,nativeStatementTemp200);}());const auto nativeStatementTemp197=JSValue("dot");const auto nativeStatementTemp193=nativeStatementTemp196.get(nativeStatementTemp197);const auto nativeStatementTemp194=nativeStatementTemp196;Args nativeStatementTemp195;nativeStatementTemp195.push_back(([&]()->JSValue{const auto nativeStatementTemp205=nativeStatementTemp191;const auto nativeStatementTemp206=JSValue("direction");return nativeStatementTemp205.get(nativeStatementTemp206);}()));return nativeStatementTemp193.call(nativeStatementTemp194,nativeStatementTemp195);}());
if(([&]()->JSValue{const auto nativeStatementTemp207=directionDistance;const auto nativeStatementTemp208=JSValue(0);return binary("<",nativeStatementTemp207,nativeStatementTemp208);}()).toBoolean()){
return ([&]()->JSValue{auto nativeStatementTemp212=([&]()->JSValue{const auto nativeStatementTemp214=nativeStatementTemp191;const auto nativeStatementTemp215=JSValue("origin");return nativeStatementTemp214.get(nativeStatementTemp215);}());const auto nativeStatementTemp213=JSValue("distanceToSquared");const auto nativeStatementTemp209=nativeStatementTemp212.get(nativeStatementTemp213);const auto nativeStatementTemp210=nativeStatementTemp212;Args nativeStatementTemp211;nativeStatementTemp211.push_back(point);return nativeStatementTemp209.call(nativeStatementTemp210,nativeStatementTemp211);}());

}
([&]()->JSValue{auto nativeStatementTemp219=([&]()->JSValue{auto nativeStatementTemp224=nativeStatementTemp190.get("_vector");const auto nativeStatementTemp225=JSValue("copy");const auto nativeStatementTemp221=nativeStatementTemp224.get(nativeStatementTemp225);const auto nativeStatementTemp222=nativeStatementTemp224;Args nativeStatementTemp223;nativeStatementTemp223.push_back(([&]()->JSValue{const auto nativeStatementTemp226=nativeStatementTemp191;const auto nativeStatementTemp227=JSValue("origin");return nativeStatementTemp226.get(nativeStatementTemp227);}()));return nativeStatementTemp221.call(nativeStatementTemp222,nativeStatementTemp223);}());const auto nativeStatementTemp220=JSValue("addScaledVector");const auto nativeStatementTemp216=nativeStatementTemp219.get(nativeStatementTemp220);const auto nativeStatementTemp217=nativeStatementTemp219;Args nativeStatementTemp218;nativeStatementTemp218.push_back(([&]()->JSValue{const auto nativeStatementTemp228=nativeStatementTemp191;const auto nativeStatementTemp229=JSValue("direction");return nativeStatementTemp228.get(nativeStatementTemp229);}()));nativeStatementTemp218.push_back(directionDistance);return nativeStatementTemp216.call(nativeStatementTemp217,nativeStatementTemp218);}());
return ([&]()->JSValue{auto nativeStatementTemp233=nativeStatementTemp190.get("_vector");const auto nativeStatementTemp234=JSValue("distanceToSquared");const auto nativeStatementTemp230=nativeStatementTemp233.get(nativeStatementTemp234);const auto nativeStatementTemp231=nativeStatementTemp233;Args nativeStatementTemp232;nativeStatementTemp232.push_back(point);return nativeStatementTemp230.call(nativeStatementTemp231,nativeStatementTemp232);}());
return JSValue{};
}

JSValue method9(const JSValue& nativeStatementTemp235,const Args& nativeStatementTemp236,const Args& nativeStatementTemp237){
auto nativeStatementTemp238=nativeStatementTemp237[0];auto nativeStatementTemp239=nativeStatementTemp235;
JSValue v0=arg(nativeStatementTemp236,0);
JSValue v1=arg(nativeStatementTemp236,1);
JSValue optionalPointOnRay=arg(nativeStatementTemp236,2);
JSValue optionalPointOnSegment=arg(nativeStatementTemp236,3);
([&]()->JSValue{auto nativeStatementTemp244=([&]()->JSValue{auto nativeStatementTemp249=([&]()->JSValue{auto nativeStatementTemp254=nativeStatementTemp238.get("_segCenter");const auto nativeStatementTemp255=JSValue("copy");const auto nativeStatementTemp251=nativeStatementTemp254.get(nativeStatementTemp255);const auto nativeStatementTemp252=nativeStatementTemp254;Args nativeStatementTemp253;nativeStatementTemp253.push_back(v0);return nativeStatementTemp251.call(nativeStatementTemp252,nativeStatementTemp253);}());const auto nativeStatementTemp250=JSValue("add");const auto nativeStatementTemp246=nativeStatementTemp249.get(nativeStatementTemp250);const auto nativeStatementTemp247=nativeStatementTemp249;Args nativeStatementTemp248;nativeStatementTemp248.push_back(v1);return nativeStatementTemp246.call(nativeStatementTemp247,nativeStatementTemp248);}());const auto nativeStatementTemp245=JSValue("multiplyScalar");const auto nativeStatementTemp241=nativeStatementTemp244.get(nativeStatementTemp245);const auto nativeStatementTemp242=nativeStatementTemp244;Args nativeStatementTemp243;nativeStatementTemp243.push_back(JSValue(0.5));return nativeStatementTemp241.call(nativeStatementTemp242,nativeStatementTemp243);}());
([&]()->JSValue{auto nativeStatementTemp259=([&]()->JSValue{auto nativeStatementTemp264=([&]()->JSValue{auto nativeStatementTemp269=nativeStatementTemp238.get("_segDir");const auto nativeStatementTemp270=JSValue("copy");const auto nativeStatementTemp266=nativeStatementTemp269.get(nativeStatementTemp270);const auto nativeStatementTemp267=nativeStatementTemp269;Args nativeStatementTemp268;nativeStatementTemp268.push_back(v1);return nativeStatementTemp266.call(nativeStatementTemp267,nativeStatementTemp268);}());const auto nativeStatementTemp265=JSValue("sub");const auto nativeStatementTemp261=nativeStatementTemp264.get(nativeStatementTemp265);const auto nativeStatementTemp262=nativeStatementTemp264;Args nativeStatementTemp263;nativeStatementTemp263.push_back(v0);return nativeStatementTemp261.call(nativeStatementTemp262,nativeStatementTemp263);}());const auto nativeStatementTemp260=JSValue("normalize");const auto nativeStatementTemp256=nativeStatementTemp259.get(nativeStatementTemp260);const auto nativeStatementTemp257=nativeStatementTemp259;Args nativeStatementTemp258;return nativeStatementTemp256.call(nativeStatementTemp257,nativeStatementTemp258);}());
([&]()->JSValue{auto nativeStatementTemp274=([&]()->JSValue{auto nativeStatementTemp279=nativeStatementTemp238.get("_diff");const auto nativeStatementTemp280=JSValue("copy");const auto nativeStatementTemp276=nativeStatementTemp279.get(nativeStatementTemp280);const auto nativeStatementTemp277=nativeStatementTemp279;Args nativeStatementTemp278;nativeStatementTemp278.push_back(([&]()->JSValue{const auto nativeStatementTemp281=nativeStatementTemp239;const auto nativeStatementTemp282=JSValue("origin");return nativeStatementTemp281.get(nativeStatementTemp282);}()));return nativeStatementTemp276.call(nativeStatementTemp277,nativeStatementTemp278);}());const auto nativeStatementTemp275=JSValue("sub");const auto nativeStatementTemp271=nativeStatementTemp274.get(nativeStatementTemp275);const auto nativeStatementTemp272=nativeStatementTemp274;Args nativeStatementTemp273;nativeStatementTemp273.push_back(nativeStatementTemp238.get("_segCenter"));return nativeStatementTemp271.call(nativeStatementTemp272,nativeStatementTemp273);}());
JSValue segExtent=([&]()->JSValue{const auto nativeStatementTemp283=([&]()->JSValue{auto nativeStatementTemp288=v0;const auto nativeStatementTemp289=JSValue("distanceTo");const auto nativeStatementTemp285=nativeStatementTemp288.get(nativeStatementTemp289);const auto nativeStatementTemp286=nativeStatementTemp288;Args nativeStatementTemp287;nativeStatementTemp287.push_back(v1);return nativeStatementTemp285.call(nativeStatementTemp286,nativeStatementTemp287);}());const auto nativeStatementTemp284=JSValue(0.5);return binary("*",nativeStatementTemp283,nativeStatementTemp284);}());
JSValue a01=([&]()->JSValue{const auto nativeStatementTemp290=([&]()->JSValue{auto nativeStatementTemp294=([&]()->JSValue{const auto nativeStatementTemp296=nativeStatementTemp239;const auto nativeStatementTemp297=JSValue("direction");return nativeStatementTemp296.get(nativeStatementTemp297);}());const auto nativeStatementTemp295=JSValue("dot");const auto nativeStatementTemp291=nativeStatementTemp294.get(nativeStatementTemp295);const auto nativeStatementTemp292=nativeStatementTemp294;Args nativeStatementTemp293;nativeStatementTemp293.push_back(nativeStatementTemp238.get("_segDir"));return nativeStatementTemp291.call(nativeStatementTemp292,nativeStatementTemp293);}());return -nativeStatementTemp290.toNumber();}());
JSValue b0=([&]()->JSValue{auto nativeStatementTemp301=nativeStatementTemp238.get("_diff");const auto nativeStatementTemp302=JSValue("dot");const auto nativeStatementTemp298=nativeStatementTemp301.get(nativeStatementTemp302);const auto nativeStatementTemp299=nativeStatementTemp301;Args nativeStatementTemp300;nativeStatementTemp300.push_back(([&]()->JSValue{const auto nativeStatementTemp303=nativeStatementTemp239;const auto nativeStatementTemp304=JSValue("direction");return nativeStatementTemp303.get(nativeStatementTemp304);}()));return nativeStatementTemp298.call(nativeStatementTemp299,nativeStatementTemp300);}());
JSValue b1=([&]()->JSValue{const auto nativeStatementTemp305=([&]()->JSValue{auto nativeStatementTemp309=nativeStatementTemp238.get("_diff");const auto nativeStatementTemp310=JSValue("dot");const auto nativeStatementTemp306=nativeStatementTemp309.get(nativeStatementTemp310);const auto nativeStatementTemp307=nativeStatementTemp309;Args nativeStatementTemp308;nativeStatementTemp308.push_back(nativeStatementTemp238.get("_segDir"));return nativeStatementTemp306.call(nativeStatementTemp307,nativeStatementTemp308);}());return -nativeStatementTemp305.toNumber();}());
JSValue c=([&]()->JSValue{auto nativeStatementTemp314=nativeStatementTemp238.get("_diff");const auto nativeStatementTemp315=JSValue("lengthSq");const auto nativeStatementTemp311=nativeStatementTemp314.get(nativeStatementTemp315);const auto nativeStatementTemp312=nativeStatementTemp314;Args nativeStatementTemp313;return nativeStatementTemp311.call(nativeStatementTemp312,nativeStatementTemp313);}());
JSValue det=([&]()->JSValue{auto nativeStatementTemp319=nativeGlobalBinding("Math");const auto nativeStatementTemp320=JSValue("abs");const auto nativeStatementTemp316=nativeStatementTemp319.get(nativeStatementTemp320);const auto nativeStatementTemp317=nativeStatementTemp319;Args nativeStatementTemp318;nativeStatementTemp318.push_back(([&]()->JSValue{const auto nativeStatementTemp321=JSValue(1);const auto nativeStatementTemp322=([&]()->JSValue{const auto nativeStatementTemp323=a01;const auto nativeStatementTemp324=a01;return binary("*",nativeStatementTemp323,nativeStatementTemp324);}());return binary("-",nativeStatementTemp321,nativeStatementTemp322);}()));return nativeStatementTemp316.call(nativeStatementTemp317,nativeStatementTemp318);}());
JSValue s0=JSValue{};
JSValue s1=JSValue{};
JSValue sqrDist=JSValue{};
JSValue extDet=JSValue{};
if(([&]()->JSValue{const auto nativeStatementTemp325=det;const auto nativeStatementTemp326=JSValue(0);return binary(">",nativeStatementTemp325,nativeStatementTemp326);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp327=([&]()->JSValue{const auto nativeStatementTemp330=([&]()->JSValue{const auto nativeStatementTemp332=a01;const auto nativeStatementTemp333=b1;return binary("*",nativeStatementTemp332,nativeStatementTemp333);}());const auto nativeStatementTemp331=b0;return binary("-",nativeStatementTemp330,nativeStatementTemp331);}());s0=nativeStatementTemp327;return nativeStatementTemp327;}());
([&]()->JSValue{const auto nativeStatementTemp334=([&]()->JSValue{const auto nativeStatementTemp337=([&]()->JSValue{const auto nativeStatementTemp339=a01;const auto nativeStatementTemp340=b0;return binary("*",nativeStatementTemp339,nativeStatementTemp340);}());const auto nativeStatementTemp338=b1;return binary("-",nativeStatementTemp337,nativeStatementTemp338);}());s1=nativeStatementTemp334;return nativeStatementTemp334;}());
([&]()->JSValue{const auto nativeStatementTemp341=([&]()->JSValue{const auto nativeStatementTemp344=segExtent;const auto nativeStatementTemp345=det;return binary("*",nativeStatementTemp344,nativeStatementTemp345);}());extDet=nativeStatementTemp341;return nativeStatementTemp341;}());
if(([&]()->JSValue{const auto nativeStatementTemp346=s0;const auto nativeStatementTemp347=JSValue(0);return binary(">=",nativeStatementTemp346,nativeStatementTemp347);}()).toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp348=s1;const auto nativeStatementTemp349=([&]()->JSValue{const auto nativeStatementTemp350=extDet;return -nativeStatementTemp350.toNumber();}());return binary(">=",nativeStatementTemp348,nativeStatementTemp349);}()).toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp351=s1;const auto nativeStatementTemp352=extDet;return binary("<=",nativeStatementTemp351,nativeStatementTemp352);}()).toBoolean()){
JSValue invDet=([&]()->JSValue{const auto nativeStatementTemp353=JSValue(1);const auto nativeStatementTemp354=det;return binary("/",nativeStatementTemp353,nativeStatementTemp354);}());
([&]()->JSValue{const auto nativeStatementTemp356=s0;const auto nativeStatementTemp357=invDet;const auto nativeStatementTemp355=binary("*",nativeStatementTemp356,nativeStatementTemp357);s0=nativeStatementTemp355;return nativeStatementTemp355;}());
([&]()->JSValue{const auto nativeStatementTemp359=s1;const auto nativeStatementTemp360=invDet;const auto nativeStatementTemp358=binary("*",nativeStatementTemp359,nativeStatementTemp360);s1=nativeStatementTemp358;return nativeStatementTemp358;}());
([&]()->JSValue{const auto nativeStatementTemp361=([&]()->JSValue{const auto nativeStatementTemp364=([&]()->JSValue{const auto nativeStatementTemp366=([&]()->JSValue{const auto nativeStatementTemp368=s0;const auto nativeStatementTemp369=([&]()->JSValue{const auto nativeStatementTemp370=([&]()->JSValue{const auto nativeStatementTemp372=s0;const auto nativeStatementTemp373=([&]()->JSValue{const auto nativeStatementTemp374=a01;const auto nativeStatementTemp375=s1;return binary("*",nativeStatementTemp374,nativeStatementTemp375);}());return binary("+",nativeStatementTemp372,nativeStatementTemp373);}());const auto nativeStatementTemp371=([&]()->JSValue{const auto nativeStatementTemp376=JSValue(2);const auto nativeStatementTemp377=b0;return binary("*",nativeStatementTemp376,nativeStatementTemp377);}());return binary("+",nativeStatementTemp370,nativeStatementTemp371);}());return binary("*",nativeStatementTemp368,nativeStatementTemp369);}());const auto nativeStatementTemp367=([&]()->JSValue{const auto nativeStatementTemp378=s1;const auto nativeStatementTemp379=([&]()->JSValue{const auto nativeStatementTemp380=([&]()->JSValue{const auto nativeStatementTemp382=([&]()->JSValue{const auto nativeStatementTemp384=a01;const auto nativeStatementTemp385=s0;return binary("*",nativeStatementTemp384,nativeStatementTemp385);}());const auto nativeStatementTemp383=s1;return binary("+",nativeStatementTemp382,nativeStatementTemp383);}());const auto nativeStatementTemp381=([&]()->JSValue{const auto nativeStatementTemp386=JSValue(2);const auto nativeStatementTemp387=b1;return binary("*",nativeStatementTemp386,nativeStatementTemp387);}());return binary("+",nativeStatementTemp380,nativeStatementTemp381);}());return binary("*",nativeStatementTemp378,nativeStatementTemp379);}());return binary("+",nativeStatementTemp366,nativeStatementTemp367);}());const auto nativeStatementTemp365=c;return binary("+",nativeStatementTemp364,nativeStatementTemp365);}());sqrDist=nativeStatementTemp361;return nativeStatementTemp361;}());

}
else {
([&]()->JSValue{const auto nativeStatementTemp388=segExtent;s1=nativeStatementTemp388;return nativeStatementTemp388;}());
([&]()->JSValue{const auto nativeStatementTemp391=([&]()->JSValue{auto nativeStatementTemp397=nativeGlobalBinding("Math");const auto nativeStatementTemp398=JSValue("max");const auto nativeStatementTemp394=nativeStatementTemp397.get(nativeStatementTemp398);const auto nativeStatementTemp395=nativeStatementTemp397;Args nativeStatementTemp396;nativeStatementTemp396.push_back(JSValue(0));nativeStatementTemp396.push_back(([&]()->JSValue{const auto nativeStatementTemp399=([&]()->JSValue{const auto nativeStatementTemp400=([&]()->JSValue{const auto nativeStatementTemp402=a01;const auto nativeStatementTemp403=s1;return binary("*",nativeStatementTemp402,nativeStatementTemp403);}());const auto nativeStatementTemp401=b0;return binary("+",nativeStatementTemp400,nativeStatementTemp401);}());return -nativeStatementTemp399.toNumber();}()));return nativeStatementTemp394.call(nativeStatementTemp395,nativeStatementTemp396);}());s0=nativeStatementTemp391;return nativeStatementTemp391;}());
([&]()->JSValue{const auto nativeStatementTemp404=([&]()->JSValue{const auto nativeStatementTemp407=([&]()->JSValue{const auto nativeStatementTemp409=([&]()->JSValue{const auto nativeStatementTemp411=([&]()->JSValue{const auto nativeStatementTemp413=s0;return -nativeStatementTemp413.toNumber();}());const auto nativeStatementTemp412=s0;return binary("*",nativeStatementTemp411,nativeStatementTemp412);}());const auto nativeStatementTemp410=([&]()->JSValue{const auto nativeStatementTemp414=s1;const auto nativeStatementTemp415=([&]()->JSValue{const auto nativeStatementTemp416=s1;const auto nativeStatementTemp417=([&]()->JSValue{const auto nativeStatementTemp418=JSValue(2);const auto nativeStatementTemp419=b1;return binary("*",nativeStatementTemp418,nativeStatementTemp419);}());return binary("+",nativeStatementTemp416,nativeStatementTemp417);}());return binary("*",nativeStatementTemp414,nativeStatementTemp415);}());return binary("+",nativeStatementTemp409,nativeStatementTemp410);}());const auto nativeStatementTemp408=c;return binary("+",nativeStatementTemp407,nativeStatementTemp408);}());sqrDist=nativeStatementTemp404;return nativeStatementTemp404;}());

}

}
else {
([&]()->JSValue{const auto nativeStatementTemp420=([&]()->JSValue{const auto nativeStatementTemp423=segExtent;return -nativeStatementTemp423.toNumber();}());s1=nativeStatementTemp420;return nativeStatementTemp420;}());
([&]()->JSValue{const auto nativeStatementTemp424=([&]()->JSValue{auto nativeStatementTemp430=nativeGlobalBinding("Math");const auto nativeStatementTemp431=JSValue("max");const auto nativeStatementTemp427=nativeStatementTemp430.get(nativeStatementTemp431);const auto nativeStatementTemp428=nativeStatementTemp430;Args nativeStatementTemp429;nativeStatementTemp429.push_back(JSValue(0));nativeStatementTemp429.push_back(([&]()->JSValue{const auto nativeStatementTemp432=([&]()->JSValue{const auto nativeStatementTemp433=([&]()->JSValue{const auto nativeStatementTemp435=a01;const auto nativeStatementTemp436=s1;return binary("*",nativeStatementTemp435,nativeStatementTemp436);}());const auto nativeStatementTemp434=b0;return binary("+",nativeStatementTemp433,nativeStatementTemp434);}());return -nativeStatementTemp432.toNumber();}()));return nativeStatementTemp427.call(nativeStatementTemp428,nativeStatementTemp429);}());s0=nativeStatementTemp424;return nativeStatementTemp424;}());
([&]()->JSValue{const auto nativeStatementTemp437=([&]()->JSValue{const auto nativeStatementTemp440=([&]()->JSValue{const auto nativeStatementTemp442=([&]()->JSValue{const auto nativeStatementTemp444=([&]()->JSValue{const auto nativeStatementTemp446=s0;return -nativeStatementTemp446.toNumber();}());const auto nativeStatementTemp445=s0;return binary("*",nativeStatementTemp444,nativeStatementTemp445);}());const auto nativeStatementTemp443=([&]()->JSValue{const auto nativeStatementTemp447=s1;const auto nativeStatementTemp448=([&]()->JSValue{const auto nativeStatementTemp449=s1;const auto nativeStatementTemp450=([&]()->JSValue{const auto nativeStatementTemp451=JSValue(2);const auto nativeStatementTemp452=b1;return binary("*",nativeStatementTemp451,nativeStatementTemp452);}());return binary("+",nativeStatementTemp449,nativeStatementTemp450);}());return binary("*",nativeStatementTemp447,nativeStatementTemp448);}());return binary("+",nativeStatementTemp442,nativeStatementTemp443);}());const auto nativeStatementTemp441=c;return binary("+",nativeStatementTemp440,nativeStatementTemp441);}());sqrDist=nativeStatementTemp437;return nativeStatementTemp437;}());

}

}
else {
if(([&]()->JSValue{const auto nativeStatementTemp453=s1;const auto nativeStatementTemp454=([&]()->JSValue{const auto nativeStatementTemp455=extDet;return -nativeStatementTemp455.toNumber();}());return binary("<=",nativeStatementTemp453,nativeStatementTemp454);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp456=([&]()->JSValue{auto nativeStatementTemp462=nativeGlobalBinding("Math");const auto nativeStatementTemp463=JSValue("max");const auto nativeStatementTemp459=nativeStatementTemp462.get(nativeStatementTemp463);const auto nativeStatementTemp460=nativeStatementTemp462;Args nativeStatementTemp461;nativeStatementTemp461.push_back(JSValue(0));nativeStatementTemp461.push_back(([&]()->JSValue{const auto nativeStatementTemp464=([&]()->JSValue{const auto nativeStatementTemp465=([&]()->JSValue{const auto nativeStatementTemp467=([&]()->JSValue{const auto nativeStatementTemp469=a01;return -nativeStatementTemp469.toNumber();}());const auto nativeStatementTemp468=segExtent;return binary("*",nativeStatementTemp467,nativeStatementTemp468);}());const auto nativeStatementTemp466=b0;return binary("+",nativeStatementTemp465,nativeStatementTemp466);}());return -nativeStatementTemp464.toNumber();}()));return nativeStatementTemp459.call(nativeStatementTemp460,nativeStatementTemp461);}());s0=nativeStatementTemp456;return nativeStatementTemp456;}());
([&]()->JSValue{const auto nativeStatementTemp470=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp473=s0;const auto nativeStatementTemp474=JSValue(0);return binary(">",nativeStatementTemp473,nativeStatementTemp474);}()).toBoolean())return ([&]()->JSValue{const auto nativeStatementTemp475=segExtent;return -nativeStatementTemp475.toNumber();}());return ([&]()->JSValue{auto nativeStatementTemp479=nativeGlobalBinding("Math");const auto nativeStatementTemp480=JSValue("min");const auto nativeStatementTemp476=nativeStatementTemp479.get(nativeStatementTemp480);const auto nativeStatementTemp477=nativeStatementTemp479;Args nativeStatementTemp478;nativeStatementTemp478.push_back(([&]()->JSValue{auto nativeStatementTemp484=nativeGlobalBinding("Math");const auto nativeStatementTemp485=JSValue("max");const auto nativeStatementTemp481=nativeStatementTemp484.get(nativeStatementTemp485);const auto nativeStatementTemp482=nativeStatementTemp484;Args nativeStatementTemp483;nativeStatementTemp483.push_back(([&]()->JSValue{const auto nativeStatementTemp486=segExtent;return -nativeStatementTemp486.toNumber();}()));nativeStatementTemp483.push_back(([&]()->JSValue{const auto nativeStatementTemp487=b1;return -nativeStatementTemp487.toNumber();}()));return nativeStatementTemp481.call(nativeStatementTemp482,nativeStatementTemp483);}()));nativeStatementTemp478.push_back(segExtent);return nativeStatementTemp476.call(nativeStatementTemp477,nativeStatementTemp478);}());}());s1=nativeStatementTemp470;return nativeStatementTemp470;}());
([&]()->JSValue{const auto nativeStatementTemp488=([&]()->JSValue{const auto nativeStatementTemp491=([&]()->JSValue{const auto nativeStatementTemp493=([&]()->JSValue{const auto nativeStatementTemp495=([&]()->JSValue{const auto nativeStatementTemp497=s0;return -nativeStatementTemp497.toNumber();}());const auto nativeStatementTemp496=s0;return binary("*",nativeStatementTemp495,nativeStatementTemp496);}());const auto nativeStatementTemp494=([&]()->JSValue{const auto nativeStatementTemp498=s1;const auto nativeStatementTemp499=([&]()->JSValue{const auto nativeStatementTemp500=s1;const auto nativeStatementTemp501=([&]()->JSValue{const auto nativeStatementTemp502=JSValue(2);const auto nativeStatementTemp503=b1;return binary("*",nativeStatementTemp502,nativeStatementTemp503);}());return binary("+",nativeStatementTemp500,nativeStatementTemp501);}());return binary("*",nativeStatementTemp498,nativeStatementTemp499);}());return binary("+",nativeStatementTemp493,nativeStatementTemp494);}());const auto nativeStatementTemp492=c;return binary("+",nativeStatementTemp491,nativeStatementTemp492);}());sqrDist=nativeStatementTemp488;return nativeStatementTemp488;}());

}
else if(([&]()->JSValue{const auto nativeStatementTemp504=s1;const auto nativeStatementTemp505=extDet;return binary("<=",nativeStatementTemp504,nativeStatementTemp505);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp506=JSValue(0);s0=nativeStatementTemp506;return nativeStatementTemp506;}());
([&]()->JSValue{const auto nativeStatementTemp509=([&]()->JSValue{auto nativeStatementTemp515=nativeGlobalBinding("Math");const auto nativeStatementTemp516=JSValue("min");const auto nativeStatementTemp512=nativeStatementTemp515.get(nativeStatementTemp516);const auto nativeStatementTemp513=nativeStatementTemp515;Args nativeStatementTemp514;nativeStatementTemp514.push_back(([&]()->JSValue{auto nativeStatementTemp520=nativeGlobalBinding("Math");const auto nativeStatementTemp521=JSValue("max");const auto nativeStatementTemp517=nativeStatementTemp520.get(nativeStatementTemp521);const auto nativeStatementTemp518=nativeStatementTemp520;Args nativeStatementTemp519;nativeStatementTemp519.push_back(([&]()->JSValue{const auto nativeStatementTemp522=segExtent;return -nativeStatementTemp522.toNumber();}()));nativeStatementTemp519.push_back(([&]()->JSValue{const auto nativeStatementTemp523=b1;return -nativeStatementTemp523.toNumber();}()));return nativeStatementTemp517.call(nativeStatementTemp518,nativeStatementTemp519);}()));nativeStatementTemp514.push_back(segExtent);return nativeStatementTemp512.call(nativeStatementTemp513,nativeStatementTemp514);}());s1=nativeStatementTemp509;return nativeStatementTemp509;}());
([&]()->JSValue{const auto nativeStatementTemp524=([&]()->JSValue{const auto nativeStatementTemp527=([&]()->JSValue{const auto nativeStatementTemp529=s1;const auto nativeStatementTemp530=([&]()->JSValue{const auto nativeStatementTemp531=s1;const auto nativeStatementTemp532=([&]()->JSValue{const auto nativeStatementTemp533=JSValue(2);const auto nativeStatementTemp534=b1;return binary("*",nativeStatementTemp533,nativeStatementTemp534);}());return binary("+",nativeStatementTemp531,nativeStatementTemp532);}());return binary("*",nativeStatementTemp529,nativeStatementTemp530);}());const auto nativeStatementTemp528=c;return binary("+",nativeStatementTemp527,nativeStatementTemp528);}());sqrDist=nativeStatementTemp524;return nativeStatementTemp524;}());

}
else {
([&]()->JSValue{const auto nativeStatementTemp535=([&]()->JSValue{auto nativeStatementTemp541=nativeGlobalBinding("Math");const auto nativeStatementTemp542=JSValue("max");const auto nativeStatementTemp538=nativeStatementTemp541.get(nativeStatementTemp542);const auto nativeStatementTemp539=nativeStatementTemp541;Args nativeStatementTemp540;nativeStatementTemp540.push_back(JSValue(0));nativeStatementTemp540.push_back(([&]()->JSValue{const auto nativeStatementTemp543=([&]()->JSValue{const auto nativeStatementTemp544=([&]()->JSValue{const auto nativeStatementTemp546=a01;const auto nativeStatementTemp547=segExtent;return binary("*",nativeStatementTemp546,nativeStatementTemp547);}());const auto nativeStatementTemp545=b0;return binary("+",nativeStatementTemp544,nativeStatementTemp545);}());return -nativeStatementTemp543.toNumber();}()));return nativeStatementTemp538.call(nativeStatementTemp539,nativeStatementTemp540);}());s0=nativeStatementTemp535;return nativeStatementTemp535;}());
([&]()->JSValue{const auto nativeStatementTemp548=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp551=s0;const auto nativeStatementTemp552=JSValue(0);return binary(">",nativeStatementTemp551,nativeStatementTemp552);}()).toBoolean())return segExtent;return ([&]()->JSValue{auto nativeStatementTemp556=nativeGlobalBinding("Math");const auto nativeStatementTemp557=JSValue("min");const auto nativeStatementTemp553=nativeStatementTemp556.get(nativeStatementTemp557);const auto nativeStatementTemp554=nativeStatementTemp556;Args nativeStatementTemp555;nativeStatementTemp555.push_back(([&]()->JSValue{auto nativeStatementTemp561=nativeGlobalBinding("Math");const auto nativeStatementTemp562=JSValue("max");const auto nativeStatementTemp558=nativeStatementTemp561.get(nativeStatementTemp562);const auto nativeStatementTemp559=nativeStatementTemp561;Args nativeStatementTemp560;nativeStatementTemp560.push_back(([&]()->JSValue{const auto nativeStatementTemp563=segExtent;return -nativeStatementTemp563.toNumber();}()));nativeStatementTemp560.push_back(([&]()->JSValue{const auto nativeStatementTemp564=b1;return -nativeStatementTemp564.toNumber();}()));return nativeStatementTemp558.call(nativeStatementTemp559,nativeStatementTemp560);}()));nativeStatementTemp555.push_back(segExtent);return nativeStatementTemp553.call(nativeStatementTemp554,nativeStatementTemp555);}());}());s1=nativeStatementTemp548;return nativeStatementTemp548;}());
([&]()->JSValue{const auto nativeStatementTemp565=([&]()->JSValue{const auto nativeStatementTemp568=([&]()->JSValue{const auto nativeStatementTemp570=([&]()->JSValue{const auto nativeStatementTemp572=([&]()->JSValue{const auto nativeStatementTemp574=s0;return -nativeStatementTemp574.toNumber();}());const auto nativeStatementTemp573=s0;return binary("*",nativeStatementTemp572,nativeStatementTemp573);}());const auto nativeStatementTemp571=([&]()->JSValue{const auto nativeStatementTemp575=s1;const auto nativeStatementTemp576=([&]()->JSValue{const auto nativeStatementTemp577=s1;const auto nativeStatementTemp578=([&]()->JSValue{const auto nativeStatementTemp579=JSValue(2);const auto nativeStatementTemp580=b1;return binary("*",nativeStatementTemp579,nativeStatementTemp580);}());return binary("+",nativeStatementTemp577,nativeStatementTemp578);}());return binary("*",nativeStatementTemp575,nativeStatementTemp576);}());return binary("+",nativeStatementTemp570,nativeStatementTemp571);}());const auto nativeStatementTemp569=c;return binary("+",nativeStatementTemp568,nativeStatementTemp569);}());sqrDist=nativeStatementTemp565;return nativeStatementTemp565;}());

}

}

}
else {
([&]()->JSValue{const auto nativeStatementTemp581=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp584=a01;const auto nativeStatementTemp585=JSValue(0);return binary(">",nativeStatementTemp584,nativeStatementTemp585);}()).toBoolean())return ([&]()->JSValue{const auto nativeStatementTemp586=segExtent;return -nativeStatementTemp586.toNumber();}());return segExtent;}());s1=nativeStatementTemp581;return nativeStatementTemp581;}());
([&]()->JSValue{const auto nativeStatementTemp587=([&]()->JSValue{auto nativeStatementTemp593=nativeGlobalBinding("Math");const auto nativeStatementTemp594=JSValue("max");const auto nativeStatementTemp590=nativeStatementTemp593.get(nativeStatementTemp594);const auto nativeStatementTemp591=nativeStatementTemp593;Args nativeStatementTemp592;nativeStatementTemp592.push_back(JSValue(0));nativeStatementTemp592.push_back(([&]()->JSValue{const auto nativeStatementTemp595=([&]()->JSValue{const auto nativeStatementTemp596=([&]()->JSValue{const auto nativeStatementTemp598=a01;const auto nativeStatementTemp599=s1;return binary("*",nativeStatementTemp598,nativeStatementTemp599);}());const auto nativeStatementTemp597=b0;return binary("+",nativeStatementTemp596,nativeStatementTemp597);}());return -nativeStatementTemp595.toNumber();}()));return nativeStatementTemp590.call(nativeStatementTemp591,nativeStatementTemp592);}());s0=nativeStatementTemp587;return nativeStatementTemp587;}());
([&]()->JSValue{const auto nativeStatementTemp600=([&]()->JSValue{const auto nativeStatementTemp603=([&]()->JSValue{const auto nativeStatementTemp605=([&]()->JSValue{const auto nativeStatementTemp607=([&]()->JSValue{const auto nativeStatementTemp609=s0;return -nativeStatementTemp609.toNumber();}());const auto nativeStatementTemp608=s0;return binary("*",nativeStatementTemp607,nativeStatementTemp608);}());const auto nativeStatementTemp606=([&]()->JSValue{const auto nativeStatementTemp610=s1;const auto nativeStatementTemp611=([&]()->JSValue{const auto nativeStatementTemp612=s1;const auto nativeStatementTemp613=([&]()->JSValue{const auto nativeStatementTemp614=JSValue(2);const auto nativeStatementTemp615=b1;return binary("*",nativeStatementTemp614,nativeStatementTemp615);}());return binary("+",nativeStatementTemp612,nativeStatementTemp613);}());return binary("*",nativeStatementTemp610,nativeStatementTemp611);}());return binary("+",nativeStatementTemp605,nativeStatementTemp606);}());const auto nativeStatementTemp604=c;return binary("+",nativeStatementTemp603,nativeStatementTemp604);}());sqrDist=nativeStatementTemp600;return nativeStatementTemp600;}());

}
if(optionalPointOnRay.toBoolean()){
([&]()->JSValue{auto nativeStatementTemp619=([&]()->JSValue{auto nativeStatementTemp624=optionalPointOnRay;const auto nativeStatementTemp625=JSValue("copy");const auto nativeStatementTemp621=nativeStatementTemp624.get(nativeStatementTemp625);const auto nativeStatementTemp622=nativeStatementTemp624;Args nativeStatementTemp623;nativeStatementTemp623.push_back(([&]()->JSValue{const auto nativeStatementTemp626=nativeStatementTemp239;const auto nativeStatementTemp627=JSValue("origin");return nativeStatementTemp626.get(nativeStatementTemp627);}()));return nativeStatementTemp621.call(nativeStatementTemp622,nativeStatementTemp623);}());const auto nativeStatementTemp620=JSValue("addScaledVector");const auto nativeStatementTemp616=nativeStatementTemp619.get(nativeStatementTemp620);const auto nativeStatementTemp617=nativeStatementTemp619;Args nativeStatementTemp618;nativeStatementTemp618.push_back(([&]()->JSValue{const auto nativeStatementTemp628=nativeStatementTemp239;const auto nativeStatementTemp629=JSValue("direction");return nativeStatementTemp628.get(nativeStatementTemp629);}()));nativeStatementTemp618.push_back(s0);return nativeStatementTemp616.call(nativeStatementTemp617,nativeStatementTemp618);}());

}
if(optionalPointOnSegment.toBoolean()){
([&]()->JSValue{auto nativeStatementTemp633=([&]()->JSValue{auto nativeStatementTemp638=optionalPointOnSegment;const auto nativeStatementTemp639=JSValue("copy");const auto nativeStatementTemp635=nativeStatementTemp638.get(nativeStatementTemp639);const auto nativeStatementTemp636=nativeStatementTemp638;Args nativeStatementTemp637;nativeStatementTemp637.push_back(nativeStatementTemp238.get("_segCenter"));return nativeStatementTemp635.call(nativeStatementTemp636,nativeStatementTemp637);}());const auto nativeStatementTemp634=JSValue("addScaledVector");const auto nativeStatementTemp630=nativeStatementTemp633.get(nativeStatementTemp634);const auto nativeStatementTemp631=nativeStatementTemp633;Args nativeStatementTemp632;nativeStatementTemp632.push_back(nativeStatementTemp238.get("_segDir"));nativeStatementTemp632.push_back(s1);return nativeStatementTemp630.call(nativeStatementTemp631,nativeStatementTemp632);}());

}
return sqrDist;
return JSValue{};
}

JSValue method10(const JSValue& nativeStatementTemp640,const Args& nativeStatementTemp641,const Args& nativeStatementTemp642){
auto nativeStatementTemp643=nativeStatementTemp642[0];auto nativeStatementTemp644=nativeStatementTemp640;
JSValue sphere=arg(nativeStatementTemp641,0);
JSValue target=arg(nativeStatementTemp641,1);
if(([&]()->JSValue{const auto nativeStatementTemp646=([&]()->JSValue{const auto nativeStatementTemp648=sphere;const auto nativeStatementTemp649=JSValue("radius");return nativeStatementTemp648.get(nativeStatementTemp649);}());const auto nativeStatementTemp647=JSValue(0);return binary("<",nativeStatementTemp646,nativeStatementTemp647);}()).toBoolean())return JSValue(nullptr);
([&]()->JSValue{auto nativeStatementTemp653=nativeStatementTemp643.get("_vector");const auto nativeStatementTemp654=JSValue("subVectors");const auto nativeStatementTemp650=nativeStatementTemp653.get(nativeStatementTemp654);const auto nativeStatementTemp651=nativeStatementTemp653;Args nativeStatementTemp652;nativeStatementTemp652.push_back(([&]()->JSValue{const auto nativeStatementTemp655=sphere;const auto nativeStatementTemp656=JSValue("center");return nativeStatementTemp655.get(nativeStatementTemp656);}()));nativeStatementTemp652.push_back(([&]()->JSValue{const auto nativeStatementTemp657=nativeStatementTemp644;const auto nativeStatementTemp658=JSValue("origin");return nativeStatementTemp657.get(nativeStatementTemp658);}()));return nativeStatementTemp650.call(nativeStatementTemp651,nativeStatementTemp652);}());
JSValue tca=([&]()->JSValue{auto nativeStatementTemp662=nativeStatementTemp643.get("_vector");const auto nativeStatementTemp663=JSValue("dot");const auto nativeStatementTemp659=nativeStatementTemp662.get(nativeStatementTemp663);const auto nativeStatementTemp660=nativeStatementTemp662;Args nativeStatementTemp661;nativeStatementTemp661.push_back(([&]()->JSValue{const auto nativeStatementTemp664=nativeStatementTemp644;const auto nativeStatementTemp665=JSValue("direction");return nativeStatementTemp664.get(nativeStatementTemp665);}()));return nativeStatementTemp659.call(nativeStatementTemp660,nativeStatementTemp661);}());
JSValue d2=([&]()->JSValue{const auto nativeStatementTemp666=([&]()->JSValue{auto nativeStatementTemp671=nativeStatementTemp643.get("_vector");const auto nativeStatementTemp672=JSValue("dot");const auto nativeStatementTemp668=nativeStatementTemp671.get(nativeStatementTemp672);const auto nativeStatementTemp669=nativeStatementTemp671;Args nativeStatementTemp670;nativeStatementTemp670.push_back(nativeStatementTemp643.get("_vector"));return nativeStatementTemp668.call(nativeStatementTemp669,nativeStatementTemp670);}());const auto nativeStatementTemp667=([&]()->JSValue{const auto nativeStatementTemp673=tca;const auto nativeStatementTemp674=tca;return binary("*",nativeStatementTemp673,nativeStatementTemp674);}());return binary("-",nativeStatementTemp666,nativeStatementTemp667);}());
JSValue radius2=([&]()->JSValue{const auto nativeStatementTemp675=([&]()->JSValue{const auto nativeStatementTemp677=sphere;const auto nativeStatementTemp678=JSValue("radius");return nativeStatementTemp677.get(nativeStatementTemp678);}());const auto nativeStatementTemp676=([&]()->JSValue{const auto nativeStatementTemp679=sphere;const auto nativeStatementTemp680=JSValue("radius");return nativeStatementTemp679.get(nativeStatementTemp680);}());return binary("*",nativeStatementTemp675,nativeStatementTemp676);}());
if(([&]()->JSValue{const auto nativeStatementTemp681=d2;const auto nativeStatementTemp682=radius2;return binary(">",nativeStatementTemp681,nativeStatementTemp682);}()).toBoolean())return JSValue(nullptr);
JSValue thc=([&]()->JSValue{auto nativeStatementTemp686=nativeGlobalBinding("Math");const auto nativeStatementTemp687=JSValue("sqrt");const auto nativeStatementTemp683=nativeStatementTemp686.get(nativeStatementTemp687);const auto nativeStatementTemp684=nativeStatementTemp686;Args nativeStatementTemp685;nativeStatementTemp685.push_back(([&]()->JSValue{const auto nativeStatementTemp688=radius2;const auto nativeStatementTemp689=d2;return binary("-",nativeStatementTemp688,nativeStatementTemp689);}()));return nativeStatementTemp683.call(nativeStatementTemp684,nativeStatementTemp685);}());
JSValue t0=([&]()->JSValue{const auto nativeStatementTemp690=tca;const auto nativeStatementTemp691=thc;return binary("-",nativeStatementTemp690,nativeStatementTemp691);}());
JSValue t1=([&]()->JSValue{const auto nativeStatementTemp692=tca;const auto nativeStatementTemp693=thc;return binary("+",nativeStatementTemp692,nativeStatementTemp693);}());
if(([&]()->JSValue{const auto nativeStatementTemp694=t1;const auto nativeStatementTemp695=JSValue(0);return binary("<",nativeStatementTemp694,nativeStatementTemp695);}()).toBoolean())return JSValue(nullptr);
if(([&]()->JSValue{const auto nativeStatementTemp696=t0;const auto nativeStatementTemp697=JSValue(0);return binary("<",nativeStatementTemp696,nativeStatementTemp697);}()).toBoolean())return ([&]()->JSValue{auto nativeStatementTemp701=nativeStatementTemp644;const auto nativeStatementTemp702=JSValue("at");const auto nativeStatementTemp698=nativeStatementTemp701.get(nativeStatementTemp702);const auto nativeStatementTemp699=nativeStatementTemp701;Args nativeStatementTemp700;nativeStatementTemp700.push_back(t1);nativeStatementTemp700.push_back(target);return nativeStatementTemp698.call(nativeStatementTemp699,nativeStatementTemp700);}());
return ([&]()->JSValue{auto nativeStatementTemp706=nativeStatementTemp644;const auto nativeStatementTemp707=JSValue("at");const auto nativeStatementTemp703=nativeStatementTemp706.get(nativeStatementTemp707);const auto nativeStatementTemp704=nativeStatementTemp706;Args nativeStatementTemp705;nativeStatementTemp705.push_back(t0);nativeStatementTemp705.push_back(target);return nativeStatementTemp703.call(nativeStatementTemp704,nativeStatementTemp705);}());
return JSValue{};
}

JSValue method11(const JSValue& nativeStatementTemp708,const Args& nativeStatementTemp709,const Args& nativeStatementTemp710){
auto nativeStatementTemp711=nativeStatementTemp710[0];auto nativeStatementTemp712=nativeStatementTemp708;
JSValue sphere=arg(nativeStatementTemp709,0);
if(([&]()->JSValue{const auto nativeStatementTemp714=([&]()->JSValue{const auto nativeStatementTemp716=sphere;const auto nativeStatementTemp717=JSValue("radius");return nativeStatementTemp716.get(nativeStatementTemp717);}());const auto nativeStatementTemp715=JSValue(0);return binary("<",nativeStatementTemp714,nativeStatementTemp715);}()).toBoolean())return JSValue(false);
return ([&]()->JSValue{const auto nativeStatementTemp718=([&]()->JSValue{auto nativeStatementTemp723=nativeStatementTemp712;const auto nativeStatementTemp724=JSValue("distanceSqToPoint");const auto nativeStatementTemp720=nativeStatementTemp723.get(nativeStatementTemp724);const auto nativeStatementTemp721=nativeStatementTemp723;Args nativeStatementTemp722;nativeStatementTemp722.push_back(([&]()->JSValue{const auto nativeStatementTemp725=sphere;const auto nativeStatementTemp726=JSValue("center");return nativeStatementTemp725.get(nativeStatementTemp726);}()));return nativeStatementTemp720.call(nativeStatementTemp721,nativeStatementTemp722);}());const auto nativeStatementTemp719=([&]()->JSValue{const auto nativeStatementTemp727=([&]()->JSValue{const auto nativeStatementTemp729=sphere;const auto nativeStatementTemp730=JSValue("radius");return nativeStatementTemp729.get(nativeStatementTemp730);}());const auto nativeStatementTemp728=([&]()->JSValue{const auto nativeStatementTemp731=sphere;const auto nativeStatementTemp732=JSValue("radius");return nativeStatementTemp731.get(nativeStatementTemp732);}());return binary("*",nativeStatementTemp727,nativeStatementTemp728);}());return binary("<=",nativeStatementTemp718,nativeStatementTemp719);}());
return JSValue{};
}

JSValue method12(const JSValue& nativeStatementTemp733,const Args& nativeStatementTemp734,const Args& nativeStatementTemp735){
auto nativeStatementTemp736=nativeStatementTemp735[0];auto nativeStatementTemp737=nativeStatementTemp733;
JSValue plane=arg(nativeStatementTemp734,0);
JSValue denominator=([&]()->JSValue{auto nativeStatementTemp742=([&]()->JSValue{const auto nativeStatementTemp744=plane;const auto nativeStatementTemp745=JSValue("normal");return nativeStatementTemp744.get(nativeStatementTemp745);}());const auto nativeStatementTemp743=JSValue("dot");const auto nativeStatementTemp739=nativeStatementTemp742.get(nativeStatementTemp743);const auto nativeStatementTemp740=nativeStatementTemp742;Args nativeStatementTemp741;nativeStatementTemp741.push_back(([&]()->JSValue{const auto nativeStatementTemp746=nativeStatementTemp737;const auto nativeStatementTemp747=JSValue("direction");return nativeStatementTemp746.get(nativeStatementTemp747);}()));return nativeStatementTemp739.call(nativeStatementTemp740,nativeStatementTemp741);}());
if(([&]()->JSValue{const auto nativeStatementTemp748=denominator;const auto nativeStatementTemp749=JSValue(0);return binary("===",nativeStatementTemp748,nativeStatementTemp749);}()).toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp750=([&]()->JSValue{auto nativeStatementTemp755=plane;const auto nativeStatementTemp756=JSValue("distanceToPoint");const auto nativeStatementTemp752=nativeStatementTemp755.get(nativeStatementTemp756);const auto nativeStatementTemp753=nativeStatementTemp755;Args nativeStatementTemp754;nativeStatementTemp754.push_back(([&]()->JSValue{const auto nativeStatementTemp757=nativeStatementTemp737;const auto nativeStatementTemp758=JSValue("origin");return nativeStatementTemp757.get(nativeStatementTemp758);}()));return nativeStatementTemp752.call(nativeStatementTemp753,nativeStatementTemp754);}());const auto nativeStatementTemp751=JSValue(0);return binary("===",nativeStatementTemp750,nativeStatementTemp751);}()).toBoolean()){
return JSValue(0);

}
return JSValue(nullptr);

}
JSValue t=([&]()->JSValue{const auto nativeStatementTemp759=([&]()->JSValue{const auto nativeStatementTemp761=([&]()->JSValue{const auto nativeStatementTemp762=([&]()->JSValue{auto nativeStatementTemp767=([&]()->JSValue{const auto nativeStatementTemp769=nativeStatementTemp737;const auto nativeStatementTemp770=JSValue("origin");return nativeStatementTemp769.get(nativeStatementTemp770);}());const auto nativeStatementTemp768=JSValue("dot");const auto nativeStatementTemp764=nativeStatementTemp767.get(nativeStatementTemp768);const auto nativeStatementTemp765=nativeStatementTemp767;Args nativeStatementTemp766;nativeStatementTemp766.push_back(([&]()->JSValue{const auto nativeStatementTemp771=plane;const auto nativeStatementTemp772=JSValue("normal");return nativeStatementTemp771.get(nativeStatementTemp772);}()));return nativeStatementTemp764.call(nativeStatementTemp765,nativeStatementTemp766);}());const auto nativeStatementTemp763=([&]()->JSValue{const auto nativeStatementTemp773=plane;const auto nativeStatementTemp774=JSValue("constant");return nativeStatementTemp773.get(nativeStatementTemp774);}());return binary("+",nativeStatementTemp762,nativeStatementTemp763);}());return -nativeStatementTemp761.toNumber();}());const auto nativeStatementTemp760=denominator;return binary("/",nativeStatementTemp759,nativeStatementTemp760);}());
return ([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp775=t;const auto nativeStatementTemp776=JSValue(0);return binary(">=",nativeStatementTemp775,nativeStatementTemp776);}()).toBoolean())return t;return JSValue(nullptr);}());
return JSValue{};
}

JSValue method13(const JSValue& nativeStatementTemp777,const Args& nativeStatementTemp778,const Args& nativeStatementTemp779){
auto nativeStatementTemp780=nativeStatementTemp779[0];auto nativeStatementTemp781=nativeStatementTemp777;
JSValue plane=arg(nativeStatementTemp778,0);
JSValue target=arg(nativeStatementTemp778,1);
JSValue t=([&]()->JSValue{auto nativeStatementTemp786=nativeStatementTemp781;const auto nativeStatementTemp787=JSValue("distanceToPlane");const auto nativeStatementTemp783=nativeStatementTemp786.get(nativeStatementTemp787);const auto nativeStatementTemp784=nativeStatementTemp786;Args nativeStatementTemp785;nativeStatementTemp785.push_back(plane);return nativeStatementTemp783.call(nativeStatementTemp784,nativeStatementTemp785);}());
if(([&]()->JSValue{const auto nativeStatementTemp788=t;const auto nativeStatementTemp789=JSValue(nullptr);return binary("===",nativeStatementTemp788,nativeStatementTemp789);}()).toBoolean()){
return JSValue(nullptr);

}
return ([&]()->JSValue{auto nativeStatementTemp793=nativeStatementTemp781;const auto nativeStatementTemp794=JSValue("at");const auto nativeStatementTemp790=nativeStatementTemp793.get(nativeStatementTemp794);const auto nativeStatementTemp791=nativeStatementTemp793;Args nativeStatementTemp792;nativeStatementTemp792.push_back(t);nativeStatementTemp792.push_back(target);return nativeStatementTemp790.call(nativeStatementTemp791,nativeStatementTemp792);}());
return JSValue{};
}

JSValue method14(const JSValue& nativeStatementTemp795,const Args& nativeStatementTemp796,const Args& nativeStatementTemp797){
auto nativeStatementTemp798=nativeStatementTemp797[0];auto nativeStatementTemp799=nativeStatementTemp795;
JSValue plane=arg(nativeStatementTemp796,0);
JSValue distToPoint=([&]()->JSValue{auto nativeStatementTemp804=plane;const auto nativeStatementTemp805=JSValue("distanceToPoint");const auto nativeStatementTemp801=nativeStatementTemp804.get(nativeStatementTemp805);const auto nativeStatementTemp802=nativeStatementTemp804;Args nativeStatementTemp803;nativeStatementTemp803.push_back(([&]()->JSValue{const auto nativeStatementTemp806=nativeStatementTemp799;const auto nativeStatementTemp807=JSValue("origin");return nativeStatementTemp806.get(nativeStatementTemp807);}()));return nativeStatementTemp801.call(nativeStatementTemp802,nativeStatementTemp803);}());
if(([&]()->JSValue{const auto nativeStatementTemp808=distToPoint;const auto nativeStatementTemp809=JSValue(0);return binary("===",nativeStatementTemp808,nativeStatementTemp809);}()).toBoolean()){
return JSValue(true);

}
JSValue denominator=([&]()->JSValue{auto nativeStatementTemp813=([&]()->JSValue{const auto nativeStatementTemp815=plane;const auto nativeStatementTemp816=JSValue("normal");return nativeStatementTemp815.get(nativeStatementTemp816);}());const auto nativeStatementTemp814=JSValue("dot");const auto nativeStatementTemp810=nativeStatementTemp813.get(nativeStatementTemp814);const auto nativeStatementTemp811=nativeStatementTemp813;Args nativeStatementTemp812;nativeStatementTemp812.push_back(([&]()->JSValue{const auto nativeStatementTemp817=nativeStatementTemp799;const auto nativeStatementTemp818=JSValue("direction");return nativeStatementTemp817.get(nativeStatementTemp818);}()));return nativeStatementTemp810.call(nativeStatementTemp811,nativeStatementTemp812);}());
if(([&]()->JSValue{const auto nativeStatementTemp819=([&]()->JSValue{const auto nativeStatementTemp821=denominator;const auto nativeStatementTemp822=distToPoint;return binary("*",nativeStatementTemp821,nativeStatementTemp822);}());const auto nativeStatementTemp820=JSValue(0);return binary("<",nativeStatementTemp819,nativeStatementTemp820);}()).toBoolean()){
return JSValue(true);

}
return JSValue(false);
return JSValue{};
}

JSValue method15(const JSValue& nativeStatementTemp823,const Args& nativeStatementTemp824,const Args& nativeStatementTemp825){
auto nativeStatementTemp826=nativeStatementTemp825[0];auto nativeStatementTemp827=nativeStatementTemp823;
JSValue box=arg(nativeStatementTemp824,0);
JSValue target=arg(nativeStatementTemp824,1);
JSValue tmin=JSValue{};
JSValue tmax=JSValue{};
JSValue tymin=JSValue{};
JSValue tymax=JSValue{};
JSValue tzmin=JSValue{};
JSValue tzmax=JSValue{};
JSValue invdirx=([&]()->JSValue{const auto nativeStatementTemp829=JSValue(1);const auto nativeStatementTemp830=([&]()->JSValue{const auto nativeStatementTemp831=([&]()->JSValue{const auto nativeStatementTemp833=nativeStatementTemp827;const auto nativeStatementTemp834=JSValue("direction");return nativeStatementTemp833.get(nativeStatementTemp834);}());const auto nativeStatementTemp832=JSValue("x");return nativeStatementTemp831.get(nativeStatementTemp832);}());return binary("/",nativeStatementTemp829,nativeStatementTemp830);}());
JSValue invdiry=([&]()->JSValue{const auto nativeStatementTemp835=JSValue(1);const auto nativeStatementTemp836=([&]()->JSValue{const auto nativeStatementTemp837=([&]()->JSValue{const auto nativeStatementTemp839=nativeStatementTemp827;const auto nativeStatementTemp840=JSValue("direction");return nativeStatementTemp839.get(nativeStatementTemp840);}());const auto nativeStatementTemp838=JSValue("y");return nativeStatementTemp837.get(nativeStatementTemp838);}());return binary("/",nativeStatementTemp835,nativeStatementTemp836);}());
JSValue invdirz=([&]()->JSValue{const auto nativeStatementTemp841=JSValue(1);const auto nativeStatementTemp842=([&]()->JSValue{const auto nativeStatementTemp843=([&]()->JSValue{const auto nativeStatementTemp845=nativeStatementTemp827;const auto nativeStatementTemp846=JSValue("direction");return nativeStatementTemp845.get(nativeStatementTemp846);}());const auto nativeStatementTemp844=JSValue("z");return nativeStatementTemp843.get(nativeStatementTemp844);}());return binary("/",nativeStatementTemp841,nativeStatementTemp842);}());
JSValue origin=([&]()->JSValue{const auto nativeStatementTemp847=nativeStatementTemp827;const auto nativeStatementTemp848=JSValue("origin");return nativeStatementTemp847.get(nativeStatementTemp848);}());
if(([&]()->JSValue{const auto nativeStatementTemp849=invdirx;const auto nativeStatementTemp850=JSValue(0);return binary(">=",nativeStatementTemp849,nativeStatementTemp850);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp851=([&]()->JSValue{const auto nativeStatementTemp854=([&]()->JSValue{const auto nativeStatementTemp856=([&]()->JSValue{const auto nativeStatementTemp858=([&]()->JSValue{const auto nativeStatementTemp860=box;const auto nativeStatementTemp861=JSValue("min");return nativeStatementTemp860.get(nativeStatementTemp861);}());const auto nativeStatementTemp859=JSValue("x");return nativeStatementTemp858.get(nativeStatementTemp859);}());const auto nativeStatementTemp857=([&]()->JSValue{const auto nativeStatementTemp862=origin;const auto nativeStatementTemp863=JSValue("x");return nativeStatementTemp862.get(nativeStatementTemp863);}());return binary("-",nativeStatementTemp856,nativeStatementTemp857);}());const auto nativeStatementTemp855=invdirx;return binary("*",nativeStatementTemp854,nativeStatementTemp855);}());tmin=nativeStatementTemp851;return nativeStatementTemp851;}());
([&]()->JSValue{const auto nativeStatementTemp864=([&]()->JSValue{const auto nativeStatementTemp867=([&]()->JSValue{const auto nativeStatementTemp869=([&]()->JSValue{const auto nativeStatementTemp871=([&]()->JSValue{const auto nativeStatementTemp873=box;const auto nativeStatementTemp874=JSValue("max");return nativeStatementTemp873.get(nativeStatementTemp874);}());const auto nativeStatementTemp872=JSValue("x");return nativeStatementTemp871.get(nativeStatementTemp872);}());const auto nativeStatementTemp870=([&]()->JSValue{const auto nativeStatementTemp875=origin;const auto nativeStatementTemp876=JSValue("x");return nativeStatementTemp875.get(nativeStatementTemp876);}());return binary("-",nativeStatementTemp869,nativeStatementTemp870);}());const auto nativeStatementTemp868=invdirx;return binary("*",nativeStatementTemp867,nativeStatementTemp868);}());tmax=nativeStatementTemp864;return nativeStatementTemp864;}());

}
else {
([&]()->JSValue{const auto nativeStatementTemp877=([&]()->JSValue{const auto nativeStatementTemp880=([&]()->JSValue{const auto nativeStatementTemp882=([&]()->JSValue{const auto nativeStatementTemp884=([&]()->JSValue{const auto nativeStatementTemp886=box;const auto nativeStatementTemp887=JSValue("max");return nativeStatementTemp886.get(nativeStatementTemp887);}());const auto nativeStatementTemp885=JSValue("x");return nativeStatementTemp884.get(nativeStatementTemp885);}());const auto nativeStatementTemp883=([&]()->JSValue{const auto nativeStatementTemp888=origin;const auto nativeStatementTemp889=JSValue("x");return nativeStatementTemp888.get(nativeStatementTemp889);}());return binary("-",nativeStatementTemp882,nativeStatementTemp883);}());const auto nativeStatementTemp881=invdirx;return binary("*",nativeStatementTemp880,nativeStatementTemp881);}());tmin=nativeStatementTemp877;return nativeStatementTemp877;}());
([&]()->JSValue{const auto nativeStatementTemp890=([&]()->JSValue{const auto nativeStatementTemp893=([&]()->JSValue{const auto nativeStatementTemp895=([&]()->JSValue{const auto nativeStatementTemp897=([&]()->JSValue{const auto nativeStatementTemp899=box;const auto nativeStatementTemp900=JSValue("min");return nativeStatementTemp899.get(nativeStatementTemp900);}());const auto nativeStatementTemp898=JSValue("x");return nativeStatementTemp897.get(nativeStatementTemp898);}());const auto nativeStatementTemp896=([&]()->JSValue{const auto nativeStatementTemp901=origin;const auto nativeStatementTemp902=JSValue("x");return nativeStatementTemp901.get(nativeStatementTemp902);}());return binary("-",nativeStatementTemp895,nativeStatementTemp896);}());const auto nativeStatementTemp894=invdirx;return binary("*",nativeStatementTemp893,nativeStatementTemp894);}());tmax=nativeStatementTemp890;return nativeStatementTemp890;}());

}
if(([&]()->JSValue{const auto nativeStatementTemp903=invdiry;const auto nativeStatementTemp904=JSValue(0);return binary(">=",nativeStatementTemp903,nativeStatementTemp904);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp905=([&]()->JSValue{const auto nativeStatementTemp908=([&]()->JSValue{const auto nativeStatementTemp910=([&]()->JSValue{const auto nativeStatementTemp912=([&]()->JSValue{const auto nativeStatementTemp914=box;const auto nativeStatementTemp915=JSValue("min");return nativeStatementTemp914.get(nativeStatementTemp915);}());const auto nativeStatementTemp913=JSValue("y");return nativeStatementTemp912.get(nativeStatementTemp913);}());const auto nativeStatementTemp911=([&]()->JSValue{const auto nativeStatementTemp916=origin;const auto nativeStatementTemp917=JSValue("y");return nativeStatementTemp916.get(nativeStatementTemp917);}());return binary("-",nativeStatementTemp910,nativeStatementTemp911);}());const auto nativeStatementTemp909=invdiry;return binary("*",nativeStatementTemp908,nativeStatementTemp909);}());tymin=nativeStatementTemp905;return nativeStatementTemp905;}());
([&]()->JSValue{const auto nativeStatementTemp918=([&]()->JSValue{const auto nativeStatementTemp921=([&]()->JSValue{const auto nativeStatementTemp923=([&]()->JSValue{const auto nativeStatementTemp925=([&]()->JSValue{const auto nativeStatementTemp927=box;const auto nativeStatementTemp928=JSValue("max");return nativeStatementTemp927.get(nativeStatementTemp928);}());const auto nativeStatementTemp926=JSValue("y");return nativeStatementTemp925.get(nativeStatementTemp926);}());const auto nativeStatementTemp924=([&]()->JSValue{const auto nativeStatementTemp929=origin;const auto nativeStatementTemp930=JSValue("y");return nativeStatementTemp929.get(nativeStatementTemp930);}());return binary("-",nativeStatementTemp923,nativeStatementTemp924);}());const auto nativeStatementTemp922=invdiry;return binary("*",nativeStatementTemp921,nativeStatementTemp922);}());tymax=nativeStatementTemp918;return nativeStatementTemp918;}());

}
else {
([&]()->JSValue{const auto nativeStatementTemp931=([&]()->JSValue{const auto nativeStatementTemp934=([&]()->JSValue{const auto nativeStatementTemp936=([&]()->JSValue{const auto nativeStatementTemp938=([&]()->JSValue{const auto nativeStatementTemp940=box;const auto nativeStatementTemp941=JSValue("max");return nativeStatementTemp940.get(nativeStatementTemp941);}());const auto nativeStatementTemp939=JSValue("y");return nativeStatementTemp938.get(nativeStatementTemp939);}());const auto nativeStatementTemp937=([&]()->JSValue{const auto nativeStatementTemp942=origin;const auto nativeStatementTemp943=JSValue("y");return nativeStatementTemp942.get(nativeStatementTemp943);}());return binary("-",nativeStatementTemp936,nativeStatementTemp937);}());const auto nativeStatementTemp935=invdiry;return binary("*",nativeStatementTemp934,nativeStatementTemp935);}());tymin=nativeStatementTemp931;return nativeStatementTemp931;}());
([&]()->JSValue{const auto nativeStatementTemp944=([&]()->JSValue{const auto nativeStatementTemp947=([&]()->JSValue{const auto nativeStatementTemp949=([&]()->JSValue{const auto nativeStatementTemp951=([&]()->JSValue{const auto nativeStatementTemp953=box;const auto nativeStatementTemp954=JSValue("min");return nativeStatementTemp953.get(nativeStatementTemp954);}());const auto nativeStatementTemp952=JSValue("y");return nativeStatementTemp951.get(nativeStatementTemp952);}());const auto nativeStatementTemp950=([&]()->JSValue{const auto nativeStatementTemp955=origin;const auto nativeStatementTemp956=JSValue("y");return nativeStatementTemp955.get(nativeStatementTemp956);}());return binary("-",nativeStatementTemp949,nativeStatementTemp950);}());const auto nativeStatementTemp948=invdiry;return binary("*",nativeStatementTemp947,nativeStatementTemp948);}());tymax=nativeStatementTemp944;return nativeStatementTemp944;}());

}
if(([&]()->JSValue{const auto nativeStatementTemp957=([&]()->JSValue{const auto nativeStatementTemp958=tmin;const auto nativeStatementTemp959=tymax;return binary(">",nativeStatementTemp958,nativeStatementTemp959);}());if(nativeStatementTemp957.toBoolean())return nativeStatementTemp957;return ([&]()->JSValue{const auto nativeStatementTemp960=tymin;const auto nativeStatementTemp961=tmax;return binary(">",nativeStatementTemp960,nativeStatementTemp961);}());}()).toBoolean())return JSValue(nullptr);
if(([&]()->JSValue{const auto nativeStatementTemp962=([&]()->JSValue{const auto nativeStatementTemp963=tymin;const auto nativeStatementTemp964=tmin;return binary(">",nativeStatementTemp963,nativeStatementTemp964);}());if(nativeStatementTemp962.toBoolean())return nativeStatementTemp962;return ([&]()->JSValue{const auto nativeStatementTemp965=nativeGlobalBinding("isNaN");const JSValue nativeStatementTemp966;Args nativeStatementTemp967;nativeStatementTemp967.push_back(tmin);return nativeStatementTemp965.call(nativeStatementTemp966,nativeStatementTemp967);}());}()).toBoolean())([&]()->JSValue{const auto nativeStatementTemp968=tymin;tmin=nativeStatementTemp968;return nativeStatementTemp968;}());
if(([&]()->JSValue{const auto nativeStatementTemp971=([&]()->JSValue{const auto nativeStatementTemp972=tymax;const auto nativeStatementTemp973=tmax;return binary("<",nativeStatementTemp972,nativeStatementTemp973);}());if(nativeStatementTemp971.toBoolean())return nativeStatementTemp971;return ([&]()->JSValue{const auto nativeStatementTemp974=nativeGlobalBinding("isNaN");const JSValue nativeStatementTemp975;Args nativeStatementTemp976;nativeStatementTemp976.push_back(tmax);return nativeStatementTemp974.call(nativeStatementTemp975,nativeStatementTemp976);}());}()).toBoolean())([&]()->JSValue{const auto nativeStatementTemp977=tymax;tmax=nativeStatementTemp977;return nativeStatementTemp977;}());
if(([&]()->JSValue{const auto nativeStatementTemp980=invdirz;const auto nativeStatementTemp981=JSValue(0);return binary(">=",nativeStatementTemp980,nativeStatementTemp981);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp982=([&]()->JSValue{const auto nativeStatementTemp985=([&]()->JSValue{const auto nativeStatementTemp987=([&]()->JSValue{const auto nativeStatementTemp989=([&]()->JSValue{const auto nativeStatementTemp991=box;const auto nativeStatementTemp992=JSValue("min");return nativeStatementTemp991.get(nativeStatementTemp992);}());const auto nativeStatementTemp990=JSValue("z");return nativeStatementTemp989.get(nativeStatementTemp990);}());const auto nativeStatementTemp988=([&]()->JSValue{const auto nativeStatementTemp993=origin;const auto nativeStatementTemp994=JSValue("z");return nativeStatementTemp993.get(nativeStatementTemp994);}());return binary("-",nativeStatementTemp987,nativeStatementTemp988);}());const auto nativeStatementTemp986=invdirz;return binary("*",nativeStatementTemp985,nativeStatementTemp986);}());tzmin=nativeStatementTemp982;return nativeStatementTemp982;}());
([&]()->JSValue{const auto nativeStatementTemp995=([&]()->JSValue{const auto nativeStatementTemp998=([&]()->JSValue{const auto nativeStatementTemp1000=([&]()->JSValue{const auto nativeStatementTemp1002=([&]()->JSValue{const auto nativeStatementTemp1004=box;const auto nativeStatementTemp1005=JSValue("max");return nativeStatementTemp1004.get(nativeStatementTemp1005);}());const auto nativeStatementTemp1003=JSValue("z");return nativeStatementTemp1002.get(nativeStatementTemp1003);}());const auto nativeStatementTemp1001=([&]()->JSValue{const auto nativeStatementTemp1006=origin;const auto nativeStatementTemp1007=JSValue("z");return nativeStatementTemp1006.get(nativeStatementTemp1007);}());return binary("-",nativeStatementTemp1000,nativeStatementTemp1001);}());const auto nativeStatementTemp999=invdirz;return binary("*",nativeStatementTemp998,nativeStatementTemp999);}());tzmax=nativeStatementTemp995;return nativeStatementTemp995;}());

}
else {
([&]()->JSValue{const auto nativeStatementTemp1008=([&]()->JSValue{const auto nativeStatementTemp1011=([&]()->JSValue{const auto nativeStatementTemp1013=([&]()->JSValue{const auto nativeStatementTemp1015=([&]()->JSValue{const auto nativeStatementTemp1017=box;const auto nativeStatementTemp1018=JSValue("max");return nativeStatementTemp1017.get(nativeStatementTemp1018);}());const auto nativeStatementTemp1016=JSValue("z");return nativeStatementTemp1015.get(nativeStatementTemp1016);}());const auto nativeStatementTemp1014=([&]()->JSValue{const auto nativeStatementTemp1019=origin;const auto nativeStatementTemp1020=JSValue("z");return nativeStatementTemp1019.get(nativeStatementTemp1020);}());return binary("-",nativeStatementTemp1013,nativeStatementTemp1014);}());const auto nativeStatementTemp1012=invdirz;return binary("*",nativeStatementTemp1011,nativeStatementTemp1012);}());tzmin=nativeStatementTemp1008;return nativeStatementTemp1008;}());
([&]()->JSValue{const auto nativeStatementTemp1021=([&]()->JSValue{const auto nativeStatementTemp1024=([&]()->JSValue{const auto nativeStatementTemp1026=([&]()->JSValue{const auto nativeStatementTemp1028=([&]()->JSValue{const auto nativeStatementTemp1030=box;const auto nativeStatementTemp1031=JSValue("min");return nativeStatementTemp1030.get(nativeStatementTemp1031);}());const auto nativeStatementTemp1029=JSValue("z");return nativeStatementTemp1028.get(nativeStatementTemp1029);}());const auto nativeStatementTemp1027=([&]()->JSValue{const auto nativeStatementTemp1032=origin;const auto nativeStatementTemp1033=JSValue("z");return nativeStatementTemp1032.get(nativeStatementTemp1033);}());return binary("-",nativeStatementTemp1026,nativeStatementTemp1027);}());const auto nativeStatementTemp1025=invdirz;return binary("*",nativeStatementTemp1024,nativeStatementTemp1025);}());tzmax=nativeStatementTemp1021;return nativeStatementTemp1021;}());

}
if(([&]()->JSValue{const auto nativeStatementTemp1034=([&]()->JSValue{const auto nativeStatementTemp1035=tmin;const auto nativeStatementTemp1036=tzmax;return binary(">",nativeStatementTemp1035,nativeStatementTemp1036);}());if(nativeStatementTemp1034.toBoolean())return nativeStatementTemp1034;return ([&]()->JSValue{const auto nativeStatementTemp1037=tzmin;const auto nativeStatementTemp1038=tmax;return binary(">",nativeStatementTemp1037,nativeStatementTemp1038);}());}()).toBoolean())return JSValue(nullptr);
if(([&]()->JSValue{const auto nativeStatementTemp1039=([&]()->JSValue{const auto nativeStatementTemp1040=tzmin;const auto nativeStatementTemp1041=tmin;return binary(">",nativeStatementTemp1040,nativeStatementTemp1041);}());if(nativeStatementTemp1039.toBoolean())return nativeStatementTemp1039;return ([&]()->JSValue{const auto nativeStatementTemp1042=tmin;const auto nativeStatementTemp1043=tmin;return binary("!==",nativeStatementTemp1042,nativeStatementTemp1043);}());}()).toBoolean())([&]()->JSValue{const auto nativeStatementTemp1044=tzmin;tmin=nativeStatementTemp1044;return nativeStatementTemp1044;}());
if(([&]()->JSValue{const auto nativeStatementTemp1047=([&]()->JSValue{const auto nativeStatementTemp1048=tzmax;const auto nativeStatementTemp1049=tmax;return binary("<",nativeStatementTemp1048,nativeStatementTemp1049);}());if(nativeStatementTemp1047.toBoolean())return nativeStatementTemp1047;return ([&]()->JSValue{const auto nativeStatementTemp1050=tmax;const auto nativeStatementTemp1051=tmax;return binary("!==",nativeStatementTemp1050,nativeStatementTemp1051);}());}()).toBoolean())([&]()->JSValue{const auto nativeStatementTemp1052=tzmax;tmax=nativeStatementTemp1052;return nativeStatementTemp1052;}());
if(([&]()->JSValue{const auto nativeStatementTemp1055=tmax;const auto nativeStatementTemp1056=JSValue(0);return binary("<",nativeStatementTemp1055,nativeStatementTemp1056);}()).toBoolean())return JSValue(nullptr);
return ([&]()->JSValue{auto nativeStatementTemp1060=nativeStatementTemp827;const auto nativeStatementTemp1061=JSValue("at");const auto nativeStatementTemp1057=nativeStatementTemp1060.get(nativeStatementTemp1061);const auto nativeStatementTemp1058=nativeStatementTemp1060;Args nativeStatementTemp1059;nativeStatementTemp1059.push_back(([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp1062=tmin;const auto nativeStatementTemp1063=JSValue(0);return binary(">=",nativeStatementTemp1062,nativeStatementTemp1063);}()).toBoolean())return tmin;return tmax;}()));nativeStatementTemp1059.push_back(target);return nativeStatementTemp1057.call(nativeStatementTemp1058,nativeStatementTemp1059);}());
return JSValue{};
}

JSValue method16(const JSValue& nativeStatementTemp1064,const Args& nativeStatementTemp1065,const Args& nativeStatementTemp1066){
auto nativeStatementTemp1067=nativeStatementTemp1066[0];auto nativeStatementTemp1068=nativeStatementTemp1064;
JSValue box=arg(nativeStatementTemp1065,0);
return ([&]()->JSValue{const auto nativeStatementTemp1070=([&]()->JSValue{auto nativeStatementTemp1075=nativeStatementTemp1068;const auto nativeStatementTemp1076=JSValue("intersectBox");const auto nativeStatementTemp1072=nativeStatementTemp1075.get(nativeStatementTemp1076);const auto nativeStatementTemp1073=nativeStatementTemp1075;Args nativeStatementTemp1074;nativeStatementTemp1074.push_back(box);nativeStatementTemp1074.push_back(nativeStatementTemp1067.get("_vector"));return nativeStatementTemp1072.call(nativeStatementTemp1073,nativeStatementTemp1074);}());const auto nativeStatementTemp1071=JSValue(nullptr);return binary("!==",nativeStatementTemp1070,nativeStatementTemp1071);}());
return JSValue{};
}

JSValue method17(const JSValue& nativeStatementTemp1077,const Args& nativeStatementTemp1078,const Args& nativeStatementTemp1079){
auto nativeStatementTemp1080=nativeStatementTemp1079[0];auto nativeStatementTemp1081=nativeStatementTemp1077;
JSValue a=arg(nativeStatementTemp1078,0);
JSValue b=arg(nativeStatementTemp1078,1);
JSValue c=arg(nativeStatementTemp1078,2);
JSValue backfaceCulling=arg(nativeStatementTemp1078,3);
JSValue target=arg(nativeStatementTemp1078,4);
JSValue origin=([&]()->JSValue{const auto nativeStatementTemp1083=nativeStatementTemp1081;const auto nativeStatementTemp1084=JSValue("origin");return nativeStatementTemp1083.get(nativeStatementTemp1084);}());
JSValue direction=([&]()->JSValue{const auto nativeStatementTemp1085=nativeStatementTemp1081;const auto nativeStatementTemp1086=JSValue("direction");return nativeStatementTemp1085.get(nativeStatementTemp1086);}());
JSValue dx=([&]()->JSValue{const auto nativeStatementTemp1087=direction;const auto nativeStatementTemp1088=JSValue("x");return nativeStatementTemp1087.get(nativeStatementTemp1088);}());
JSValue dy=([&]()->JSValue{const auto nativeStatementTemp1089=direction;const auto nativeStatementTemp1090=JSValue("y");return nativeStatementTemp1089.get(nativeStatementTemp1090);}());
JSValue dz=([&]()->JSValue{const auto nativeStatementTemp1091=direction;const auto nativeStatementTemp1092=JSValue("z");return nativeStatementTemp1091.get(nativeStatementTemp1092);}());
JSValue aox=([&]()->JSValue{const auto nativeStatementTemp1093=([&]()->JSValue{const auto nativeStatementTemp1095=a;const auto nativeStatementTemp1096=JSValue("x");return nativeStatementTemp1095.get(nativeStatementTemp1096);}());const auto nativeStatementTemp1094=([&]()->JSValue{const auto nativeStatementTemp1097=origin;const auto nativeStatementTemp1098=JSValue("x");return nativeStatementTemp1097.get(nativeStatementTemp1098);}());return binary("-",nativeStatementTemp1093,nativeStatementTemp1094);}());
JSValue aoy=([&]()->JSValue{const auto nativeStatementTemp1099=([&]()->JSValue{const auto nativeStatementTemp1101=a;const auto nativeStatementTemp1102=JSValue("y");return nativeStatementTemp1101.get(nativeStatementTemp1102);}());const auto nativeStatementTemp1100=([&]()->JSValue{const auto nativeStatementTemp1103=origin;const auto nativeStatementTemp1104=JSValue("y");return nativeStatementTemp1103.get(nativeStatementTemp1104);}());return binary("-",nativeStatementTemp1099,nativeStatementTemp1100);}());
JSValue aoz=([&]()->JSValue{const auto nativeStatementTemp1105=([&]()->JSValue{const auto nativeStatementTemp1107=a;const auto nativeStatementTemp1108=JSValue("z");return nativeStatementTemp1107.get(nativeStatementTemp1108);}());const auto nativeStatementTemp1106=([&]()->JSValue{const auto nativeStatementTemp1109=origin;const auto nativeStatementTemp1110=JSValue("z");return nativeStatementTemp1109.get(nativeStatementTemp1110);}());return binary("-",nativeStatementTemp1105,nativeStatementTemp1106);}());
JSValue box=([&]()->JSValue{const auto nativeStatementTemp1111=([&]()->JSValue{const auto nativeStatementTemp1113=b;const auto nativeStatementTemp1114=JSValue("x");return nativeStatementTemp1113.get(nativeStatementTemp1114);}());const auto nativeStatementTemp1112=([&]()->JSValue{const auto nativeStatementTemp1115=origin;const auto nativeStatementTemp1116=JSValue("x");return nativeStatementTemp1115.get(nativeStatementTemp1116);}());return binary("-",nativeStatementTemp1111,nativeStatementTemp1112);}());
JSValue boy=([&]()->JSValue{const auto nativeStatementTemp1117=([&]()->JSValue{const auto nativeStatementTemp1119=b;const auto nativeStatementTemp1120=JSValue("y");return nativeStatementTemp1119.get(nativeStatementTemp1120);}());const auto nativeStatementTemp1118=([&]()->JSValue{const auto nativeStatementTemp1121=origin;const auto nativeStatementTemp1122=JSValue("y");return nativeStatementTemp1121.get(nativeStatementTemp1122);}());return binary("-",nativeStatementTemp1117,nativeStatementTemp1118);}());
JSValue boz=([&]()->JSValue{const auto nativeStatementTemp1123=([&]()->JSValue{const auto nativeStatementTemp1125=b;const auto nativeStatementTemp1126=JSValue("z");return nativeStatementTemp1125.get(nativeStatementTemp1126);}());const auto nativeStatementTemp1124=([&]()->JSValue{const auto nativeStatementTemp1127=origin;const auto nativeStatementTemp1128=JSValue("z");return nativeStatementTemp1127.get(nativeStatementTemp1128);}());return binary("-",nativeStatementTemp1123,nativeStatementTemp1124);}());
JSValue cox=([&]()->JSValue{const auto nativeStatementTemp1129=([&]()->JSValue{const auto nativeStatementTemp1131=c;const auto nativeStatementTemp1132=JSValue("x");return nativeStatementTemp1131.get(nativeStatementTemp1132);}());const auto nativeStatementTemp1130=([&]()->JSValue{const auto nativeStatementTemp1133=origin;const auto nativeStatementTemp1134=JSValue("x");return nativeStatementTemp1133.get(nativeStatementTemp1134);}());return binary("-",nativeStatementTemp1129,nativeStatementTemp1130);}());
JSValue coy=([&]()->JSValue{const auto nativeStatementTemp1135=([&]()->JSValue{const auto nativeStatementTemp1137=c;const auto nativeStatementTemp1138=JSValue("y");return nativeStatementTemp1137.get(nativeStatementTemp1138);}());const auto nativeStatementTemp1136=([&]()->JSValue{const auto nativeStatementTemp1139=origin;const auto nativeStatementTemp1140=JSValue("y");return nativeStatementTemp1139.get(nativeStatementTemp1140);}());return binary("-",nativeStatementTemp1135,nativeStatementTemp1136);}());
JSValue coz=([&]()->JSValue{const auto nativeStatementTemp1141=([&]()->JSValue{const auto nativeStatementTemp1143=c;const auto nativeStatementTemp1144=JSValue("z");return nativeStatementTemp1143.get(nativeStatementTemp1144);}());const auto nativeStatementTemp1142=([&]()->JSValue{const auto nativeStatementTemp1145=origin;const auto nativeStatementTemp1146=JSValue("z");return nativeStatementTemp1145.get(nativeStatementTemp1146);}());return binary("-",nativeStatementTemp1141,nativeStatementTemp1142);}());
JSValue adx=([&]()->JSValue{auto nativeStatementTemp1150=nativeGlobalBinding("Math");const auto nativeStatementTemp1151=JSValue("abs");const auto nativeStatementTemp1147=nativeStatementTemp1150.get(nativeStatementTemp1151);const auto nativeStatementTemp1148=nativeStatementTemp1150;Args nativeStatementTemp1149;nativeStatementTemp1149.push_back(dx);return nativeStatementTemp1147.call(nativeStatementTemp1148,nativeStatementTemp1149);}());
JSValue ady=([&]()->JSValue{auto nativeStatementTemp1155=nativeGlobalBinding("Math");const auto nativeStatementTemp1156=JSValue("abs");const auto nativeStatementTemp1152=nativeStatementTemp1155.get(nativeStatementTemp1156);const auto nativeStatementTemp1153=nativeStatementTemp1155;Args nativeStatementTemp1154;nativeStatementTemp1154.push_back(dy);return nativeStatementTemp1152.call(nativeStatementTemp1153,nativeStatementTemp1154);}());
JSValue adz=([&]()->JSValue{auto nativeStatementTemp1160=nativeGlobalBinding("Math");const auto nativeStatementTemp1161=JSValue("abs");const auto nativeStatementTemp1157=nativeStatementTemp1160.get(nativeStatementTemp1161);const auto nativeStatementTemp1158=nativeStatementTemp1160;Args nativeStatementTemp1159;nativeStatementTemp1159.push_back(dz);return nativeStatementTemp1157.call(nativeStatementTemp1158,nativeStatementTemp1159);}());
JSValue dkx=JSValue{};
JSValue dky=JSValue{};
JSValue dkz=JSValue{};
JSValue akx=JSValue{};
JSValue aky=JSValue{};
JSValue akz=JSValue{};
JSValue bkx=JSValue{};
JSValue bky=JSValue{};
JSValue bkz=JSValue{};
JSValue ckx=JSValue{};
JSValue cky=JSValue{};
JSValue ckz=JSValue{};
if(([&]()->JSValue{const auto nativeStatementTemp1162=([&]()->JSValue{const auto nativeStatementTemp1163=adx;const auto nativeStatementTemp1164=ady;return binary(">=",nativeStatementTemp1163,nativeStatementTemp1164);}());if(!nativeStatementTemp1162.toBoolean())return nativeStatementTemp1162;return ([&]()->JSValue{const auto nativeStatementTemp1165=adx;const auto nativeStatementTemp1166=adz;return binary(">=",nativeStatementTemp1165,nativeStatementTemp1166);}());}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp1167=dx;dkz=nativeStatementTemp1167;return nativeStatementTemp1167;}());
([&]()->JSValue{const auto nativeStatementTemp1170=aox;akz=nativeStatementTemp1170;return nativeStatementTemp1170;}());
([&]()->JSValue{const auto nativeStatementTemp1173=box;bkz=nativeStatementTemp1173;return nativeStatementTemp1173;}());
([&]()->JSValue{const auto nativeStatementTemp1176=cox;ckz=nativeStatementTemp1176;return nativeStatementTemp1176;}());
if(([&]()->JSValue{const auto nativeStatementTemp1179=dx;const auto nativeStatementTemp1180=JSValue(0);return binary(">=",nativeStatementTemp1179,nativeStatementTemp1180);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp1181=dy;dkx=nativeStatementTemp1181;return nativeStatementTemp1181;}());
([&]()->JSValue{const auto nativeStatementTemp1184=dz;dky=nativeStatementTemp1184;return nativeStatementTemp1184;}());
([&]()->JSValue{const auto nativeStatementTemp1187=aoy;akx=nativeStatementTemp1187;return nativeStatementTemp1187;}());
([&]()->JSValue{const auto nativeStatementTemp1190=aoz;aky=nativeStatementTemp1190;return nativeStatementTemp1190;}());
([&]()->JSValue{const auto nativeStatementTemp1193=boy;bkx=nativeStatementTemp1193;return nativeStatementTemp1193;}());
([&]()->JSValue{const auto nativeStatementTemp1196=boz;bky=nativeStatementTemp1196;return nativeStatementTemp1196;}());
([&]()->JSValue{const auto nativeStatementTemp1199=coy;ckx=nativeStatementTemp1199;return nativeStatementTemp1199;}());
([&]()->JSValue{const auto nativeStatementTemp1202=coz;cky=nativeStatementTemp1202;return nativeStatementTemp1202;}());

}
else {
([&]()->JSValue{const auto nativeStatementTemp1205=dz;dkx=nativeStatementTemp1205;return nativeStatementTemp1205;}());
([&]()->JSValue{const auto nativeStatementTemp1208=dy;dky=nativeStatementTemp1208;return nativeStatementTemp1208;}());
([&]()->JSValue{const auto nativeStatementTemp1211=aoz;akx=nativeStatementTemp1211;return nativeStatementTemp1211;}());
([&]()->JSValue{const auto nativeStatementTemp1214=aoy;aky=nativeStatementTemp1214;return nativeStatementTemp1214;}());
([&]()->JSValue{const auto nativeStatementTemp1217=boz;bkx=nativeStatementTemp1217;return nativeStatementTemp1217;}());
([&]()->JSValue{const auto nativeStatementTemp1220=boy;bky=nativeStatementTemp1220;return nativeStatementTemp1220;}());
([&]()->JSValue{const auto nativeStatementTemp1223=coz;ckx=nativeStatementTemp1223;return nativeStatementTemp1223;}());
([&]()->JSValue{const auto nativeStatementTemp1226=coy;cky=nativeStatementTemp1226;return nativeStatementTemp1226;}());

}

}
else if(([&]()->JSValue{const auto nativeStatementTemp1229=ady;const auto nativeStatementTemp1230=adz;return binary(">=",nativeStatementTemp1229,nativeStatementTemp1230);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp1231=dy;dkz=nativeStatementTemp1231;return nativeStatementTemp1231;}());
([&]()->JSValue{const auto nativeStatementTemp1234=aoy;akz=nativeStatementTemp1234;return nativeStatementTemp1234;}());
([&]()->JSValue{const auto nativeStatementTemp1237=boy;bkz=nativeStatementTemp1237;return nativeStatementTemp1237;}());
([&]()->JSValue{const auto nativeStatementTemp1240=coy;ckz=nativeStatementTemp1240;return nativeStatementTemp1240;}());
if(([&]()->JSValue{const auto nativeStatementTemp1243=dy;const auto nativeStatementTemp1244=JSValue(0);return binary(">=",nativeStatementTemp1243,nativeStatementTemp1244);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp1245=dz;dkx=nativeStatementTemp1245;return nativeStatementTemp1245;}());
([&]()->JSValue{const auto nativeStatementTemp1248=dx;dky=nativeStatementTemp1248;return nativeStatementTemp1248;}());
([&]()->JSValue{const auto nativeStatementTemp1251=aoz;akx=nativeStatementTemp1251;return nativeStatementTemp1251;}());
([&]()->JSValue{const auto nativeStatementTemp1254=aox;aky=nativeStatementTemp1254;return nativeStatementTemp1254;}());
([&]()->JSValue{const auto nativeStatementTemp1257=boz;bkx=nativeStatementTemp1257;return nativeStatementTemp1257;}());
([&]()->JSValue{const auto nativeStatementTemp1260=box;bky=nativeStatementTemp1260;return nativeStatementTemp1260;}());
([&]()->JSValue{const auto nativeStatementTemp1263=coz;ckx=nativeStatementTemp1263;return nativeStatementTemp1263;}());
([&]()->JSValue{const auto nativeStatementTemp1266=cox;cky=nativeStatementTemp1266;return nativeStatementTemp1266;}());

}
else {
([&]()->JSValue{const auto nativeStatementTemp1269=dx;dkx=nativeStatementTemp1269;return nativeStatementTemp1269;}());
([&]()->JSValue{const auto nativeStatementTemp1272=dz;dky=nativeStatementTemp1272;return nativeStatementTemp1272;}());
([&]()->JSValue{const auto nativeStatementTemp1275=aox;akx=nativeStatementTemp1275;return nativeStatementTemp1275;}());
([&]()->JSValue{const auto nativeStatementTemp1278=aoz;aky=nativeStatementTemp1278;return nativeStatementTemp1278;}());
([&]()->JSValue{const auto nativeStatementTemp1281=box;bkx=nativeStatementTemp1281;return nativeStatementTemp1281;}());
([&]()->JSValue{const auto nativeStatementTemp1284=boz;bky=nativeStatementTemp1284;return nativeStatementTemp1284;}());
([&]()->JSValue{const auto nativeStatementTemp1287=cox;ckx=nativeStatementTemp1287;return nativeStatementTemp1287;}());
([&]()->JSValue{const auto nativeStatementTemp1290=coz;cky=nativeStatementTemp1290;return nativeStatementTemp1290;}());

}

}
else {
([&]()->JSValue{const auto nativeStatementTemp1293=dz;dkz=nativeStatementTemp1293;return nativeStatementTemp1293;}());
([&]()->JSValue{const auto nativeStatementTemp1296=aoz;akz=nativeStatementTemp1296;return nativeStatementTemp1296;}());
([&]()->JSValue{const auto nativeStatementTemp1299=boz;bkz=nativeStatementTemp1299;return nativeStatementTemp1299;}());
([&]()->JSValue{const auto nativeStatementTemp1302=coz;ckz=nativeStatementTemp1302;return nativeStatementTemp1302;}());
if(([&]()->JSValue{const auto nativeStatementTemp1305=dz;const auto nativeStatementTemp1306=JSValue(0);return binary(">=",nativeStatementTemp1305,nativeStatementTemp1306);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp1307=dx;dkx=nativeStatementTemp1307;return nativeStatementTemp1307;}());
([&]()->JSValue{const auto nativeStatementTemp1310=dy;dky=nativeStatementTemp1310;return nativeStatementTemp1310;}());
([&]()->JSValue{const auto nativeStatementTemp1313=aox;akx=nativeStatementTemp1313;return nativeStatementTemp1313;}());
([&]()->JSValue{const auto nativeStatementTemp1316=aoy;aky=nativeStatementTemp1316;return nativeStatementTemp1316;}());
([&]()->JSValue{const auto nativeStatementTemp1319=box;bkx=nativeStatementTemp1319;return nativeStatementTemp1319;}());
([&]()->JSValue{const auto nativeStatementTemp1322=boy;bky=nativeStatementTemp1322;return nativeStatementTemp1322;}());
([&]()->JSValue{const auto nativeStatementTemp1325=cox;ckx=nativeStatementTemp1325;return nativeStatementTemp1325;}());
([&]()->JSValue{const auto nativeStatementTemp1328=coy;cky=nativeStatementTemp1328;return nativeStatementTemp1328;}());

}
else {
([&]()->JSValue{const auto nativeStatementTemp1331=dy;dkx=nativeStatementTemp1331;return nativeStatementTemp1331;}());
([&]()->JSValue{const auto nativeStatementTemp1334=dx;dky=nativeStatementTemp1334;return nativeStatementTemp1334;}());
([&]()->JSValue{const auto nativeStatementTemp1337=aoy;akx=nativeStatementTemp1337;return nativeStatementTemp1337;}());
([&]()->JSValue{const auto nativeStatementTemp1340=aox;aky=nativeStatementTemp1340;return nativeStatementTemp1340;}());
([&]()->JSValue{const auto nativeStatementTemp1343=boy;bkx=nativeStatementTemp1343;return nativeStatementTemp1343;}());
([&]()->JSValue{const auto nativeStatementTemp1346=box;bky=nativeStatementTemp1346;return nativeStatementTemp1346;}());
([&]()->JSValue{const auto nativeStatementTemp1349=coy;ckx=nativeStatementTemp1349;return nativeStatementTemp1349;}());
([&]()->JSValue{const auto nativeStatementTemp1352=cox;cky=nativeStatementTemp1352;return nativeStatementTemp1352;}());

}

}
if(([&]()->JSValue{const auto nativeStatementTemp1355=dkz;const auto nativeStatementTemp1356=JSValue(0);return binary("===",nativeStatementTemp1355,nativeStatementTemp1356);}()).toBoolean())return JSValue(nullptr);
JSValue sx=([&]()->JSValue{const auto nativeStatementTemp1357=dkx;const auto nativeStatementTemp1358=dkz;return binary("/",nativeStatementTemp1357,nativeStatementTemp1358);}());
JSValue sy=([&]()->JSValue{const auto nativeStatementTemp1359=dky;const auto nativeStatementTemp1360=dkz;return binary("/",nativeStatementTemp1359,nativeStatementTemp1360);}());
JSValue sz=([&]()->JSValue{const auto nativeStatementTemp1361=JSValue(1);const auto nativeStatementTemp1362=dkz;return binary("/",nativeStatementTemp1361,nativeStatementTemp1362);}());
JSValue ax=([&]()->JSValue{const auto nativeStatementTemp1363=akx;const auto nativeStatementTemp1364=([&]()->JSValue{const auto nativeStatementTemp1365=sx;const auto nativeStatementTemp1366=akz;return binary("*",nativeStatementTemp1365,nativeStatementTemp1366);}());return binary("-",nativeStatementTemp1363,nativeStatementTemp1364);}());
JSValue ay=([&]()->JSValue{const auto nativeStatementTemp1367=aky;const auto nativeStatementTemp1368=([&]()->JSValue{const auto nativeStatementTemp1369=sy;const auto nativeStatementTemp1370=akz;return binary("*",nativeStatementTemp1369,nativeStatementTemp1370);}());return binary("-",nativeStatementTemp1367,nativeStatementTemp1368);}());
JSValue bx=([&]()->JSValue{const auto nativeStatementTemp1371=bkx;const auto nativeStatementTemp1372=([&]()->JSValue{const auto nativeStatementTemp1373=sx;const auto nativeStatementTemp1374=bkz;return binary("*",nativeStatementTemp1373,nativeStatementTemp1374);}());return binary("-",nativeStatementTemp1371,nativeStatementTemp1372);}());
JSValue by=([&]()->JSValue{const auto nativeStatementTemp1375=bky;const auto nativeStatementTemp1376=([&]()->JSValue{const auto nativeStatementTemp1377=sy;const auto nativeStatementTemp1378=bkz;return binary("*",nativeStatementTemp1377,nativeStatementTemp1378);}());return binary("-",nativeStatementTemp1375,nativeStatementTemp1376);}());
JSValue cx=([&]()->JSValue{const auto nativeStatementTemp1379=ckx;const auto nativeStatementTemp1380=([&]()->JSValue{const auto nativeStatementTemp1381=sx;const auto nativeStatementTemp1382=ckz;return binary("*",nativeStatementTemp1381,nativeStatementTemp1382);}());return binary("-",nativeStatementTemp1379,nativeStatementTemp1380);}());
JSValue cy=([&]()->JSValue{const auto nativeStatementTemp1383=cky;const auto nativeStatementTemp1384=([&]()->JSValue{const auto nativeStatementTemp1385=sy;const auto nativeStatementTemp1386=ckz;return binary("*",nativeStatementTemp1385,nativeStatementTemp1386);}());return binary("-",nativeStatementTemp1383,nativeStatementTemp1384);}());
JSValue u=([&]()->JSValue{const auto nativeStatementTemp1387=([&]()->JSValue{const auto nativeStatementTemp1389=cx;const auto nativeStatementTemp1390=by;return binary("*",nativeStatementTemp1389,nativeStatementTemp1390);}());const auto nativeStatementTemp1388=([&]()->JSValue{const auto nativeStatementTemp1391=cy;const auto nativeStatementTemp1392=bx;return binary("*",nativeStatementTemp1391,nativeStatementTemp1392);}());return binary("-",nativeStatementTemp1387,nativeStatementTemp1388);}());
JSValue v=([&]()->JSValue{const auto nativeStatementTemp1393=([&]()->JSValue{const auto nativeStatementTemp1395=ax;const auto nativeStatementTemp1396=cy;return binary("*",nativeStatementTemp1395,nativeStatementTemp1396);}());const auto nativeStatementTemp1394=([&]()->JSValue{const auto nativeStatementTemp1397=ay;const auto nativeStatementTemp1398=cx;return binary("*",nativeStatementTemp1397,nativeStatementTemp1398);}());return binary("-",nativeStatementTemp1393,nativeStatementTemp1394);}());
JSValue w=([&]()->JSValue{const auto nativeStatementTemp1399=([&]()->JSValue{const auto nativeStatementTemp1401=bx;const auto nativeStatementTemp1402=ay;return binary("*",nativeStatementTemp1401,nativeStatementTemp1402);}());const auto nativeStatementTemp1400=([&]()->JSValue{const auto nativeStatementTemp1403=by;const auto nativeStatementTemp1404=ax;return binary("*",nativeStatementTemp1403,nativeStatementTemp1404);}());return binary("-",nativeStatementTemp1399,nativeStatementTemp1400);}());
if(backfaceCulling.toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp1405=([&]()->JSValue{const auto nativeStatementTemp1406=([&]()->JSValue{const auto nativeStatementTemp1407=u;const auto nativeStatementTemp1408=JSValue(0);return binary("<",nativeStatementTemp1407,nativeStatementTemp1408);}());if(nativeStatementTemp1406.toBoolean())return nativeStatementTemp1406;return ([&]()->JSValue{const auto nativeStatementTemp1409=v;const auto nativeStatementTemp1410=JSValue(0);return binary("<",nativeStatementTemp1409,nativeStatementTemp1410);}());}());if(nativeStatementTemp1405.toBoolean())return nativeStatementTemp1405;return ([&]()->JSValue{const auto nativeStatementTemp1411=w;const auto nativeStatementTemp1412=JSValue(0);return binary("<",nativeStatementTemp1411,nativeStatementTemp1412);}());}()).toBoolean())return JSValue(nullptr);

}
else {
if(([&]()->JSValue{const auto nativeStatementTemp1413=([&]()->JSValue{const auto nativeStatementTemp1414=([&]()->JSValue{const auto nativeStatementTemp1415=([&]()->JSValue{const auto nativeStatementTemp1416=u;const auto nativeStatementTemp1417=JSValue(0);return binary("<",nativeStatementTemp1416,nativeStatementTemp1417);}());if(nativeStatementTemp1415.toBoolean())return nativeStatementTemp1415;return ([&]()->JSValue{const auto nativeStatementTemp1418=v;const auto nativeStatementTemp1419=JSValue(0);return binary("<",nativeStatementTemp1418,nativeStatementTemp1419);}());}());if(nativeStatementTemp1414.toBoolean())return nativeStatementTemp1414;return ([&]()->JSValue{const auto nativeStatementTemp1420=w;const auto nativeStatementTemp1421=JSValue(0);return binary("<",nativeStatementTemp1420,nativeStatementTemp1421);}());}());if(!nativeStatementTemp1413.toBoolean())return nativeStatementTemp1413;return ([&]()->JSValue{const auto nativeStatementTemp1422=([&]()->JSValue{const auto nativeStatementTemp1423=([&]()->JSValue{const auto nativeStatementTemp1424=u;const auto nativeStatementTemp1425=JSValue(0);return binary(">",nativeStatementTemp1424,nativeStatementTemp1425);}());if(nativeStatementTemp1423.toBoolean())return nativeStatementTemp1423;return ([&]()->JSValue{const auto nativeStatementTemp1426=v;const auto nativeStatementTemp1427=JSValue(0);return binary(">",nativeStatementTemp1426,nativeStatementTemp1427);}());}());if(nativeStatementTemp1422.toBoolean())return nativeStatementTemp1422;return ([&]()->JSValue{const auto nativeStatementTemp1428=w;const auto nativeStatementTemp1429=JSValue(0);return binary(">",nativeStatementTemp1428,nativeStatementTemp1429);}());}());}()).toBoolean())return JSValue(nullptr);

}
JSValue det=([&]()->JSValue{const auto nativeStatementTemp1430=([&]()->JSValue{const auto nativeStatementTemp1432=u;const auto nativeStatementTemp1433=v;return binary("+",nativeStatementTemp1432,nativeStatementTemp1433);}());const auto nativeStatementTemp1431=w;return binary("+",nativeStatementTemp1430,nativeStatementTemp1431);}());
if(([&]()->JSValue{const auto nativeStatementTemp1434=det;const auto nativeStatementTemp1435=JSValue(0);return binary("===",nativeStatementTemp1434,nativeStatementTemp1435);}()).toBoolean())return JSValue(nullptr);
JSValue tScaled=([&]()->JSValue{const auto nativeStatementTemp1436=sz;const auto nativeStatementTemp1437=([&]()->JSValue{const auto nativeStatementTemp1438=([&]()->JSValue{const auto nativeStatementTemp1440=([&]()->JSValue{const auto nativeStatementTemp1442=u;const auto nativeStatementTemp1443=akz;return binary("*",nativeStatementTemp1442,nativeStatementTemp1443);}());const auto nativeStatementTemp1441=([&]()->JSValue{const auto nativeStatementTemp1444=v;const auto nativeStatementTemp1445=bkz;return binary("*",nativeStatementTemp1444,nativeStatementTemp1445);}());return binary("+",nativeStatementTemp1440,nativeStatementTemp1441);}());const auto nativeStatementTemp1439=([&]()->JSValue{const auto nativeStatementTemp1446=w;const auto nativeStatementTemp1447=ckz;return binary("*",nativeStatementTemp1446,nativeStatementTemp1447);}());return binary("+",nativeStatementTemp1438,nativeStatementTemp1439);}());return binary("*",nativeStatementTemp1436,nativeStatementTemp1437);}());
if(([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp1448=det;const auto nativeStatementTemp1449=JSValue(0);return binary(">",nativeStatementTemp1448,nativeStatementTemp1449);}()).toBoolean())return ([&]()->JSValue{const auto nativeStatementTemp1450=tScaled;const auto nativeStatementTemp1451=JSValue(0);return binary("<",nativeStatementTemp1450,nativeStatementTemp1451);}());return ([&]()->JSValue{const auto nativeStatementTemp1452=tScaled;const auto nativeStatementTemp1453=JSValue(0);return binary(">",nativeStatementTemp1452,nativeStatementTemp1453);}());}()).toBoolean())return JSValue(nullptr);
return ([&]()->JSValue{auto nativeStatementTemp1457=nativeStatementTemp1081;const auto nativeStatementTemp1458=JSValue("at");const auto nativeStatementTemp1454=nativeStatementTemp1457.get(nativeStatementTemp1458);const auto nativeStatementTemp1455=nativeStatementTemp1457;Args nativeStatementTemp1456;nativeStatementTemp1456.push_back(([&]()->JSValue{const auto nativeStatementTemp1459=tScaled;const auto nativeStatementTemp1460=det;return binary("/",nativeStatementTemp1459,nativeStatementTemp1460);}()));nativeStatementTemp1456.push_back(target);return nativeStatementTemp1454.call(nativeStatementTemp1455,nativeStatementTemp1456);}());
return JSValue{};
}

JSValue method18(const JSValue& nativeStatementTemp1461,const Args& nativeStatementTemp1462,const Args& nativeStatementTemp1463){
auto nativeStatementTemp1464=nativeStatementTemp1463[0];auto nativeStatementTemp1465=nativeStatementTemp1461;
JSValue matrix4=arg(nativeStatementTemp1462,0);
([&]()->JSValue{auto nativeStatementTemp1470=([&]()->JSValue{const auto nativeStatementTemp1472=nativeStatementTemp1465;const auto nativeStatementTemp1473=JSValue("origin");return nativeStatementTemp1472.get(nativeStatementTemp1473);}());const auto nativeStatementTemp1471=JSValue("applyMatrix4");const auto nativeStatementTemp1467=nativeStatementTemp1470.get(nativeStatementTemp1471);const auto nativeStatementTemp1468=nativeStatementTemp1470;Args nativeStatementTemp1469;nativeStatementTemp1469.push_back(matrix4);return nativeStatementTemp1467.call(nativeStatementTemp1468,nativeStatementTemp1469);}());
([&]()->JSValue{auto nativeStatementTemp1477=([&]()->JSValue{const auto nativeStatementTemp1479=nativeStatementTemp1465;const auto nativeStatementTemp1480=JSValue("direction");return nativeStatementTemp1479.get(nativeStatementTemp1480);}());const auto nativeStatementTemp1478=JSValue("transformDirection");const auto nativeStatementTemp1474=nativeStatementTemp1477.get(nativeStatementTemp1478);const auto nativeStatementTemp1475=nativeStatementTemp1477;Args nativeStatementTemp1476;nativeStatementTemp1476.push_back(matrix4);return nativeStatementTemp1474.call(nativeStatementTemp1475,nativeStatementTemp1476);}());
return nativeStatementTemp1465;
return JSValue{};
}

JSValue method19(const JSValue& nativeStatementTemp1481,const Args& nativeStatementTemp1482,const Args& nativeStatementTemp1483){
auto nativeStatementTemp1484=nativeStatementTemp1483[0];auto nativeStatementTemp1485=nativeStatementTemp1481;
JSValue ray=arg(nativeStatementTemp1482,0);
return ([&]()->JSValue{const auto nativeStatementTemp1487=([&]()->JSValue{auto nativeStatementTemp1491=([&]()->JSValue{const auto nativeStatementTemp1493=ray;const auto nativeStatementTemp1494=JSValue("origin");return nativeStatementTemp1493.get(nativeStatementTemp1494);}());const auto nativeStatementTemp1492=JSValue("equals");const auto nativeStatementTemp1488=nativeStatementTemp1491.get(nativeStatementTemp1492);const auto nativeStatementTemp1489=nativeStatementTemp1491;Args nativeStatementTemp1490;nativeStatementTemp1490.push_back(([&]()->JSValue{const auto nativeStatementTemp1495=nativeStatementTemp1485;const auto nativeStatementTemp1496=JSValue("origin");return nativeStatementTemp1495.get(nativeStatementTemp1496);}()));return nativeStatementTemp1488.call(nativeStatementTemp1489,nativeStatementTemp1490);}());if(!nativeStatementTemp1487.toBoolean())return nativeStatementTemp1487;return ([&]()->JSValue{auto nativeStatementTemp1500=([&]()->JSValue{const auto nativeStatementTemp1502=ray;const auto nativeStatementTemp1503=JSValue("direction");return nativeStatementTemp1502.get(nativeStatementTemp1503);}());const auto nativeStatementTemp1501=JSValue("equals");const auto nativeStatementTemp1497=nativeStatementTemp1500.get(nativeStatementTemp1501);const auto nativeStatementTemp1498=nativeStatementTemp1500;Args nativeStatementTemp1499;nativeStatementTemp1499.push_back(([&]()->JSValue{const auto nativeStatementTemp1504=nativeStatementTemp1485;const auto nativeStatementTemp1505=JSValue("direction");return nativeStatementTemp1504.get(nativeStatementTemp1505);}()));return nativeStatementTemp1497.call(nativeStatementTemp1498,nativeStatementTemp1499);}());}());
return JSValue{};
}

JSValue method20(const JSValue& nativeStatementTemp1506,const Args& nativeStatementTemp1507,const Args& nativeStatementTemp1508){
auto nativeStatementTemp1509=nativeStatementTemp1508[0];auto nativeStatementTemp1510=nativeStatementTemp1506;
return ([&]()->JSValue{auto nativeStatementTemp1515=([&]()->JSValue{const auto nativeStatementTemp1517=([&]()->JSValue{const auto nativeStatementTemp1519=nativeStatementTemp1510;const auto nativeStatementTemp1520=JSValue("constructor");return nativeStatementTemp1519.get(nativeStatementTemp1520);}());Args nativeStatementTemp1518;return nativeStatementTemp1517.construct(nativeStatementTemp1518);}());const auto nativeStatementTemp1516=JSValue("copy");const auto nativeStatementTemp1512=nativeStatementTemp1515.get(nativeStatementTemp1516);const auto nativeStatementTemp1513=nativeStatementTemp1515;Args nativeStatementTemp1514;nativeStatementTemp1514.push_back(nativeStatementTemp1510);return nativeStatementTemp1512.call(nativeStatementTemp1513,nativeStatementTemp1514);}());
return JSValue{};
}
}
JSValue nativeRayModule(){using namespace RayManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Ray module");auto module=JSValue::object(nullptr);
own(module,"Vector3",nativeVector3Constructor());
own(module,"_vector",([&]()->JSValue{const auto nativeStatementTemp1521=module.get("Vector3");Args nativeStatementTemp1522;return nativeStatementTemp1521.construct(nativeStatementTemp1522);}()));
own(module,"_segCenter",([&]()->JSValue{const auto nativeStatementTemp1523=module.get("Vector3");Args nativeStatementTemp1524;return nativeStatementTemp1523.construct(nativeStatementTemp1524);}()));
own(module,"_segDir",([&]()->JSValue{const auto nativeStatementTemp1525=module.get("Vector3");Args nativeStatementTemp1526;return nativeStatementTemp1525.construct(nativeStatementTemp1526);}()));
own(module,"_diff",([&]()->JSValue{const auto nativeStatementTemp1527=module.get("Vector3");Args nativeStatementTemp1528;return nativeStatementTemp1527.construct(nativeStatementTemp1528);}()));
auto ctor=JSValue::constructor(method0,{module},"Ray",0);ctor.setFunctionSourceText("class Ray {\r\n\r\n\t/**\r\n\t * Constructs a new ray.\r\n\t *\r\n\t * @param {Vector3} [origin=(0,0,0)] - The origin of the ray.\r\n\t * @param {Vector3} [direction=(0,0,-1)] - The (normalized) direction of the ray.\r\n\t */\r\n\tconstructor( origin = new Vector3(), direction = new Vector3( 0, 0, - 1 ) ) {\r\n\r\n\t\t/**\r\n\t\t * The origin of the ray.\r\n\t\t *\r\n\t\t * @type {Vector3}\r\n\t\t */\r\n\t\tthis.origin = origin;\r\n\r\n\t\t/**\r\n\t\t * The (normalized) direction of the ray.\r\n\t\t *\r\n\t\t * @type {Vector3}\r\n\t\t */\r\n\t\tthis.direction = direction;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Sets the ray's components by copying the given values.\r\n\t *\r\n\t * @param {Vector3} origin - The origin.\r\n\t * @param {Vector3} direction - The direction.\r\n\t * @return {Ray} A reference to this ray.\r\n\t */\r\n\tset( origin, direction ) {\r\n\r\n\t\tthis.origin.copy( origin );\r\n\t\tthis.direction.copy( direction );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Copies the values of the given ray to this instance.\r\n\t *\r\n\t * @param {Ray} ray - The ray to copy.\r\n\t * @return {Ray} A reference to this ray.\r\n\t */\r\n\tcopy( ray ) {\r\n\r\n\t\tthis.origin.copy( ray.origin );\r\n\t\tthis.direction.copy( ray.direction );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns a vector that is located at a given distance along this ray.\r\n\t *\r\n\t * @param {number} t - The distance along the ray to retrieve a position for.\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {Vector3} A position on the ray.\r\n\t */\r\n\tat( t, target ) {\r\n\r\n\t\treturn target.copy( this.origin ).addScaledVector( this.direction, t );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Adjusts the direction of the ray to point at the given vector in world space.\r\n\t *\r\n\t * @param {Vector3} v - The target position.\r\n\t * @return {Ray} A reference to this ray.\r\n\t */\r\n\tlookAt( v ) {\r\n\r\n\t\tthis.direction.copy( v ).sub( this.origin ).normalize();\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Shift the origin of this ray along its direction by the given distance.\r\n\t *\r\n\t * @param {number} t - The distance along the ray to interpolate.\r\n\t * @return {Ray} A reference to this ray.\r\n\t */\r\n\trecast( t ) {\r\n\r\n\t\tthis.origin.copy( this.at( t, _vector ) );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns the point along this ray that is closest to the given point.\r\n\t *\r\n\t * @param {Vector3} point - A point in 3D space to get the closet location on the ray for.\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {Vector3} The closest point on this ray.\r\n\t */\r\n\tclosestPointToPoint( point, target ) {\r\n\r\n\t\ttarget.subVectors( point, this.origin );\r\n\r\n\t\tconst directionDistance = target.dot( this.direction );\r\n\r\n\t\tif ( directionDistance < 0 ) {\r\n\r\n\t\t\treturn target.copy( this.origin );\r\n\r\n\t\t}\r\n\r\n\t\treturn target.copy( this.origin ).addScaledVector( this.direction, directionDistance );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns the distance of the closest approach between this ray and the given point.\r\n\t *\r\n\t * @param {Vector3} point - A point in 3D space to compute the distance to.\r\n\t * @return {number} The distance.\r\n\t */\r\n\tdistanceToPoint( point ) {\r\n\r\n\t\treturn Math.sqrt( this.distanceSqToPoint( point ) );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns the squared distance of the closest approach between this ray and the given point.\r\n\t *\r\n\t * @param {Vector3} point - A point in 3D space to compute the distance to.\r\n\t * @return {number} The squared distance.\r\n\t */\r\n\tdistanceSqToPoint( point ) {\r\n\r\n\t\tconst directionDistance = _vector.subVectors( point, this.origin ).dot( this.direction );\r\n\r\n\t\t// point behind the ray\r\n\r\n\t\tif ( directionDistance < 0 ) {\r\n\r\n\t\t\treturn this.origin.distanceToSquared( point );\r\n\r\n\t\t}\r\n\r\n\t\t_vector.copy( this.origin ).addScaledVector( this.direction, directionDistance );\r\n\r\n\t\treturn _vector.distanceToSquared( point );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns the squared distance between this ray and the given line segment.\r\n\t *\r\n\t * @param {Vector3} v0 - The start point of the line segment.\r\n\t * @param {Vector3} v1 - The end point of the line segment.\r\n\t * @param {Vector3} [optionalPointOnRay] - When provided, it receives the point on this ray that is closest to the segment.\r\n\t * @param {Vector3} [optionalPointOnSegment] - When provided, it receives the point on the line segment that is closest to this ray.\r\n\t * @return {number} The squared distance.\r\n\t */\r\n\tdistanceSqToSegment( v0, v1, optionalPointOnRay, optionalPointOnSegment ) {\r\n\r\n\t\t// from https://github.com/pmjoniak/GeometricTools/blob/master/GTEngine/Include/Mathematics/GteDistRaySegment.h\r\n\t\t// It returns the min distance between the ray and the segment\r\n\t\t// defined by v0 and v1\r\n\t\t// It can also set two optional targets :\r\n\t\t// - The closest point on the ray\r\n\t\t// - The closest point on the segment\r\n\r\n\t\t_segCenter.copy( v0 ).add( v1 ).multiplyScalar( 0.5 );\r\n\t\t_segDir.copy( v1 ).sub( v0 ).normalize();\r\n\t\t_diff.copy( this.origin ).sub( _segCenter );\r\n\r\n\t\tconst segExtent = v0.distanceTo( v1 ) * 0.5;\r\n\t\tconst a01 = - this.direction.dot( _segDir );\r\n\t\tconst b0 = _diff.dot( this.direction );\r\n\t\tconst b1 = - _diff.dot( _segDir );\r\n\t\tconst c = _diff.lengthSq();\r\n\t\tconst det = Math.abs( 1 - a01 * a01 );\r\n\t\tlet s0, s1, sqrDist, extDet;\r\n\r\n\t\tif ( det > 0 ) {\r\n\r\n\t\t\t// The ray and segment are not parallel.\r\n\r\n\t\t\ts0 = a01 * b1 - b0;\r\n\t\t\ts1 = a01 * b0 - b1;\r\n\t\t\textDet = segExtent * det;\r\n\r\n\t\t\tif ( s0 >= 0 ) {\r\n\r\n\t\t\t\tif ( s1 >= - extDet ) {\r\n\r\n\t\t\t\t\tif ( s1 <= extDet ) {\r\n\r\n\t\t\t\t\t\t// region 0\r\n\t\t\t\t\t\t// Minimum at interior points of ray and segment.\r\n\r\n\t\t\t\t\t\tconst invDet = 1 / det;\r\n\t\t\t\t\t\ts0 *= invDet;\r\n\t\t\t\t\t\ts1 *= invDet;\r\n\t\t\t\t\t\tsqrDist = s0 * ( s0 + a01 * s1 + 2 * b0 ) + s1 * ( a01 * s0 + s1 + 2 * b1 ) + c;\r\n\r\n\t\t\t\t\t} else {\r\n\r\n\t\t\t\t\t\t// region 1\r\n\r\n\t\t\t\t\t\ts1 = segExtent;\r\n\t\t\t\t\t\ts0 = Math.max( 0, - ( a01 * s1 + b0 ) );\r\n\t\t\t\t\t\tsqrDist = - s0 * s0 + s1 * ( s1 + 2 * b1 ) + c;\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t} else {\r\n\r\n\t\t\t\t\t// region 5\r\n\r\n\t\t\t\t\ts1 = - segExtent;\r\n\t\t\t\t\ts0 = Math.max( 0, - ( a01 * s1 + b0 ) );\r\n\t\t\t\t\tsqrDist = - s0 * s0 + s1 * ( s1 + 2 * b1 ) + c;\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t} else {\r\n\r\n\t\t\t\tif ( s1 <= - extDet ) {\r\n\r\n\t\t\t\t\t// region 4\r\n\r\n\t\t\t\t\ts0 = Math.max( 0, - ( - a01 * segExtent + b0 ) );\r\n\t\t\t\t\ts1 = ( s0 > 0 ) ? - segExtent : Math.min( Math.max( - segExtent, - b1 ), segExtent );\r\n\t\t\t\t\tsqrDist = - s0 * s0 + s1 * ( s1 + 2 * b1 ) + c;\r\n\r\n\t\t\t\t} else if ( s1 <= extDet ) {\r\n\r\n\t\t\t\t\t// region 3\r\n\r\n\t\t\t\t\ts0 = 0;\r\n\t\t\t\t\ts1 = Math.min( Math.max( - segExtent, - b1 ), segExtent );\r\n\t\t\t\t\tsqrDist = s1 * ( s1 + 2 * b1 ) + c;\r\n\r\n\t\t\t\t} else {\r\n\r\n\t\t\t\t\t// region 2\r\n\r\n\t\t\t\t\ts0 = Math.max( 0, - ( a01 * segExtent + b0 ) );\r\n\t\t\t\t\ts1 = ( s0 > 0 ) ? segExtent : Math.min( Math.max( - segExtent, - b1 ), segExtent );\r\n\t\t\t\t\tsqrDist = - s0 * s0 + s1 * ( s1 + 2 * b1 ) + c;\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t} else {\r\n\r\n\t\t\t// Ray and segment are parallel.\r\n\r\n\t\t\ts1 = ( a01 > 0 ) ? - segExtent : segExtent;\r\n\t\t\ts0 = Math.max( 0, - ( a01 * s1 + b0 ) );\r\n\t\t\tsqrDist = - s0 * s0 + s1 * ( s1 + 2 * b1 ) + c;\r\n\r\n\t\t}\r\n\r\n\t\tif ( optionalPointOnRay ) {\r\n\r\n\t\t\toptionalPointOnRay.copy( this.origin ).addScaledVector( this.direction, s0 );\r\n\r\n\t\t}\r\n\r\n\t\tif ( optionalPointOnSegment ) {\r\n\r\n\t\t\toptionalPointOnSegment.copy( _segCenter ).addScaledVector( _segDir, s1 );\r\n\r\n\t\t}\r\n\r\n\t\treturn sqrDist;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Intersects this ray with the given sphere, returning the intersection\r\n\t * point or `null` if there is no intersection.\r\n\t *\r\n\t * @param {Sphere} sphere - The sphere to intersect.\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {?Vector3} The intersection point.\r\n\t */\r\n\tintersectSphere( sphere, target ) {\r\n\r\n\t\tif ( sphere.radius < 0 ) return null; // handle empty spheres, see #31187\r\n\r\n\t\t_vector.subVectors( sphere.center, this.origin );\r\n\t\tconst tca = _vector.dot( this.direction );\r\n\t\tconst d2 = _vector.dot( _vector ) - tca * tca;\r\n\t\tconst radius2 = sphere.radius * sphere.radius;\r\n\r\n\t\tif ( d2 > radius2 ) return null;\r\n\r\n\t\tconst thc = Math.sqrt( radius2 - d2 );\r\n\r\n\t\t// t0 = first intersect point - entrance on front of sphere\r\n\t\tconst t0 = tca - thc;\r\n\r\n\t\t// t1 = second intersect point - exit point on back of sphere\r\n\t\tconst t1 = tca + thc;\r\n\r\n\t\t// test to see if t1 is behind the ray - if so, return null\r\n\t\tif ( t1 < 0 ) return null;\r\n\r\n\t\t// test to see if t0 is behind the ray:\r\n\t\t// if it is, the ray is inside the sphere, so return the second exit point scaled by t1,\r\n\t\t// in order to always return an intersect point that is in front of the ray.\r\n\t\tif ( t0 < 0 ) return this.at( t1, target );\r\n\r\n\t\t// else t0 is in front of the ray, so return the first collision point scaled by t0\r\n\t\treturn this.at( t0, target );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if this ray intersects with the given sphere.\r\n\t *\r\n\t * @param {Sphere} sphere - The sphere to intersect.\r\n\t * @return {boolean} Whether this ray intersects with the given sphere or not.\r\n\t */\r\n\tintersectsSphere( sphere ) {\r\n\r\n\t\tif ( sphere.radius < 0 ) return false; // handle empty spheres, see #31187\r\n\r\n\t\treturn this.distanceSqToPoint( sphere.center ) <= ( sphere.radius * sphere.radius );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Computes the distance from the ray's origin to the given plane. Returns `null` if the ray\r\n\t * does not intersect with the plane.\r\n\t *\r\n\t * @param {Plane} plane - The plane to compute the distance to.\r\n\t * @return {?number} Whether this ray intersects with the given sphere or not.\r\n\t */\r\n\tdistanceToPlane( plane ) {\r\n\r\n\t\tconst denominator = plane.normal.dot( this.direction );\r\n\r\n\t\tif ( denominator === 0 ) {\r\n\r\n\t\t\t// line is coplanar, return origin\r\n\t\t\tif ( plane.distanceToPoint( this.origin ) === 0 ) {\r\n\r\n\t\t\t\treturn 0;\r\n\r\n\t\t\t}\r\n\r\n\t\t\t// Null is preferable to undefined since undefined means.... it is undefined\r\n\r\n\t\t\treturn null;\r\n\r\n\t\t}\r\n\r\n\t\tconst t = - ( this.origin.dot( plane.normal ) + plane.constant ) / denominator;\r\n\r\n\t\t// Return if the ray never intersects the plane\r\n\r\n\t\treturn t >= 0 ? t : null;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Intersects this ray with the given plane, returning the intersection\r\n\t * point or `null` if there is no intersection.\r\n\t *\r\n\t * @param {Plane} plane - The plane to intersect.\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {?Vector3} The intersection point.\r\n\t */\r\n\tintersectPlane( plane, target ) {\r\n\r\n\t\tconst t = this.distanceToPlane( plane );\r\n\r\n\t\tif ( t === null ) {\r\n\r\n\t\t\treturn null;\r\n\r\n\t\t}\r\n\r\n\t\treturn this.at( t, target );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if this ray intersects with the given plane.\r\n\t *\r\n\t * @param {Plane} plane - The plane to intersect.\r\n\t * @return {boolean} Whether this ray intersects with the given plane or not.\r\n\t */\r\n\tintersectsPlane( plane ) {\r\n\r\n\t\t// check if the ray lies on the plane first\r\n\r\n\t\tconst distToPoint = plane.distanceToPoint( this.origin );\r\n\r\n\t\tif ( distToPoint === 0 ) {\r\n\r\n\t\t\treturn true;\r\n\r\n\t\t}\r\n\r\n\t\tconst denominator = plane.normal.dot( this.direction );\r\n\r\n\t\tif ( denominator * distToPoint < 0 ) {\r\n\r\n\t\t\treturn true;\r\n\r\n\t\t}\r\n\r\n\t\t// ray origin is behind the plane (and is pointing behind it)\r\n\r\n\t\treturn false;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Intersects this ray with the given bounding box, returning the intersection\r\n\t * point or `null` if there is no intersection.\r\n\t *\r\n\t * @param {Box3} box - The box to intersect.\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {?Vector3} The intersection point.\r\n\t */\r\n\tintersectBox( box, target ) {\r\n\r\n\t\tlet tmin, tmax, tymin, tymax, tzmin, tzmax;\r\n\r\n\t\tconst invdirx = 1 / this.direction.x,\r\n\t\t\tinvdiry = 1 / this.direction.y,\r\n\t\t\tinvdirz = 1 / this.direction.z;\r\n\r\n\t\tconst origin = this.origin;\r\n\r\n\t\tif ( invdirx >= 0 ) {\r\n\r\n\t\t\ttmin = ( box.min.x - origin.x ) * invdirx;\r\n\t\t\ttmax = ( box.max.x - origin.x ) * invdirx;\r\n\r\n\t\t} else {\r\n\r\n\t\t\ttmin = ( box.max.x - origin.x ) * invdirx;\r\n\t\t\ttmax = ( box.min.x - origin.x ) * invdirx;\r\n\r\n\t\t}\r\n\r\n\t\tif ( invdiry >= 0 ) {\r\n\r\n\t\t\ttymin = ( box.min.y - origin.y ) * invdiry;\r\n\t\t\ttymax = ( box.max.y - origin.y ) * invdiry;\r\n\r\n\t\t} else {\r\n\r\n\t\t\ttymin = ( box.max.y - origin.y ) * invdiry;\r\n\t\t\ttymax = ( box.min.y - origin.y ) * invdiry;\r\n\r\n\t\t}\r\n\r\n\t\tif ( ( tmin > tymax ) || ( tymin > tmax ) ) return null;\r\n\r\n\t\tif ( tymin > tmin || isNaN( tmin ) ) tmin = tymin;\r\n\r\n\t\tif ( tymax < tmax || isNaN( tmax ) ) tmax = tymax;\r\n\r\n\t\tif ( invdirz >= 0 ) {\r\n\r\n\t\t\ttzmin = ( box.min.z - origin.z ) * invdirz;\r\n\t\t\ttzmax = ( box.max.z - origin.z ) * invdirz;\r\n\r\n\t\t} else {\r\n\r\n\t\t\ttzmin = ( box.max.z - origin.z ) * invdirz;\r\n\t\t\ttzmax = ( box.min.z - origin.z ) * invdirz;\r\n\r\n\t\t}\r\n\r\n\t\tif ( ( tmin > tzmax ) || ( tzmin > tmax ) ) return null;\r\n\r\n\t\tif ( tzmin > tmin || tmin !== tmin ) tmin = tzmin;\r\n\r\n\t\tif ( tzmax < tmax || tmax !== tmax ) tmax = tzmax;\r\n\r\n\t\t//return point closest to the ray (positive side)\r\n\r\n\t\tif ( tmax < 0 ) return null;\r\n\r\n\t\treturn this.at( tmin >= 0 ? tmin : tmax, target );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if this ray intersects with the given box.\r\n\t *\r\n\t * @param {Box3} box - The box to intersect.\r\n\t * @return {boolean} Whether this ray intersects with the given box or not.\r\n\t */\r\n\tintersectsBox( box ) {\r\n\r\n\t\treturn this.intersectBox( box, _vector ) !== null;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Intersects this ray with the given triangle, returning the intersection\r\n\t * point or `null` if there is no intersection.\r\n\t *\r\n\t * @param {Vector3} a - The first vertex of the triangle.\r\n\t * @param {Vector3} b - The second vertex of the triangle.\r\n\t * @param {Vector3} c - The third vertex of the triangle.\r\n\t * @param {boolean} backfaceCulling - Whether to use backface culling or not.\r\n\t * @param {Vector3} target - The target vector that is used to store the method's result.\r\n\t * @return {?Vector3} The intersection point.\r\n\t */\r\n\tintersectTriangle( a, b, c, backfaceCulling, target ) {\r\n\r\n\t\t// Watertight ray/triangle intersection. Reference: Woop, Benthin, Wald,\r\n\t\t// \"Watertight Ray/Triangle Intersection\", JCGT vol. 2 no. 1 (2013), Appendix A.\r\n\t\t// https://jcgt.org/published/0002/01/05/\r\n\r\n\t\tconst origin = this.origin;\r\n\t\tconst direction = this.direction;\r\n\r\n\t\tconst dx = direction.x;\r\n\t\tconst dy = direction.y;\r\n\t\tconst dz = direction.z;\r\n\r\n\t\t// triangle vertices relative to the ray origin\r\n\r\n\t\tconst aox = a.x - origin.x, aoy = a.y - origin.y, aoz = a.z - origin.z;\r\n\t\tconst box = b.x - origin.x, boy = b.y - origin.y, boz = b.z - origin.z;\r\n\t\tconst cox = c.x - origin.x, coy = c.y - origin.y, coz = c.z - origin.z;\r\n\r\n\t\t// Use the dimension where the ray direction is maximal as the projection\r\n\t\t// axis (kz) and read every component already permuted into (kx, ky, kz).\r\n\t\t// kx and ky are swapped when the direction's kz component is negative, to\r\n\t\t// preserve the winding order of triangles.\r\n\r\n\t\tconst adx = Math.abs( dx ), ady = Math.abs( dy ), adz = Math.abs( dz );\r\n\r\n\t\tlet dkx, dky, dkz;\r\n\t\tlet akx, aky, akz, bkx, bky, bkz, ckx, cky, ckz;\r\n\r\n\t\tif ( adx >= ady && adx >= adz ) {\r\n\r\n\t\t\tdkz = dx; akz = aox; bkz = box; ckz = cox;\r\n\r\n\t\t\tif ( dx >= 0 ) {\r\n\r\n\t\t\t\tdkx = dy; dky = dz;\r\n\t\t\t\takx = aoy; aky = aoz; bkx = boy; bky = boz; ckx = coy; cky = coz;\r\n\r\n\t\t\t} else {\r\n\r\n\t\t\t\tdkx = dz; dky = dy;\r\n\t\t\t\takx = aoz; aky = aoy; bkx = boz; bky = boy; ckx = coz; cky = coy;\r\n\r\n\t\t\t}\r\n\r\n\t\t} else if ( ady >= adz ) {\r\n\r\n\t\t\tdkz = dy; akz = aoy; bkz = boy; ckz = coy;\r\n\r\n\t\t\tif ( dy >= 0 ) {\r\n\r\n\t\t\t\tdkx = dz; dky = dx;\r\n\t\t\t\takx = aoz; aky = aox; bkx = boz; bky = box; ckx = coz; cky = cox;\r\n\r\n\t\t\t} else {\r\n\r\n\t\t\t\tdkx = dx; dky = dz;\r\n\t\t\t\takx = aox; aky = aoz; bkx = box; bky = boz; ckx = cox; cky = coz;\r\n\r\n\t\t\t}\r\n\r\n\t\t} else {\r\n\r\n\t\t\tdkz = dz; akz = aoz; bkz = boz; ckz = coz;\r\n\r\n\t\t\tif ( dz >= 0 ) {\r\n\r\n\t\t\t\tdkx = dx; dky = dy;\r\n\t\t\t\takx = aox; aky = aoy; bkx = box; bky = boy; ckx = cox; cky = coy;\r\n\r\n\t\t\t} else {\r\n\r\n\t\t\t\tdkx = dy; dky = dx;\r\n\t\t\t\takx = aoy; aky = aox; bkx = boy; bky = box; ckx = coy; cky = cox;\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\t// a zero direction has no maximal axis and cannot intersect\r\n\r\n\t\tif ( dkz === 0 ) return null;\r\n\r\n\t\t// shear constants that align the ray with the +kz axis\r\n\r\n\t\tconst sx = dkx / dkz, sy = dky / dkz, sz = 1 / dkz;\r\n\r\n\t\t// sheared and scaled vertices\r\n\r\n\t\tconst ax = akx - sx * akz, ay = aky - sy * akz;\r\n\t\tconst bx = bkx - sx * bkz, by = bky - sy * bkz;\r\n\t\tconst cx = ckx - sx * ckz, cy = cky - sy * ckz;\r\n\r\n\t\t// scaled barycentric coordinates (signed edge functions); the shear makes a\r\n\t\t// shared edge evaluate identically for both adjacent triangles, so the ray\r\n\t\t// can never fall between them\r\n\r\n\t\tconst u = cx * by - cy * bx;\r\n\t\tconst v = ax * cy - ay * cx;\r\n\t\tconst w = bx * ay - by * ax;\r\n\r\n\t\tif ( backfaceCulling ) {\r\n\r\n\t\t\tif ( u < 0 || v < 0 || w < 0 ) return null;\r\n\r\n\t\t} else {\r\n\r\n\t\t\tif ( ( u < 0 || v < 0 || w < 0 ) && ( u > 0 || v > 0 || w > 0 ) ) return null;\r\n\r\n\t\t}\r\n\r\n\t\tconst det = u + v + w;\r\n\r\n\t\t// ray is co-planar with the triangle\r\n\r\n\t\tif ( det === 0 ) return null;\r\n\r\n\t\t// scaled hit distance; t = tScaled / det must lie in front of the origin\r\n\r\n\t\tconst tScaled = sz * ( u * akz + v * bkz + w * ckz );\r\n\r\n\t\tif ( det > 0 ? tScaled < 0 : tScaled > 0 ) return null;\r\n\r\n\t\treturn this.at( tScaled / det, target );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Transforms this ray with the given 4x4 transformation matrix.\r\n\t *\r\n\t * @param {Matrix4} matrix4 - The transformation matrix.\r\n\t * @return {Ray} A reference to this ray.\r\n\t */\r\n\tapplyMatrix4( matrix4 ) {\r\n\r\n\t\tthis.origin.applyMatrix4( matrix4 );\r\n\t\tthis.direction.transformDirection( matrix4 );\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if this ray is equal with the given one.\r\n\t *\r\n\t * @param {Ray} ray - The ray to test for equality.\r\n\t * @return {boolean} Whether this ray is equal with the given one.\r\n\t */\r\n\tequals( ray ) {\r\n\r\n\t\treturn ray.origin.equals( this.origin ) && ray.direction.equals( this.direction );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns a new ray with copied values from this instance.\r\n\t *\r\n\t * @return {Ray} A clone of this instance.\r\n\t */\r\n\tclone() {\r\n\r\n\t\treturn new this.constructor().copy( this );\r\n\r\n\t}\r\n\r\n}");auto prototype=ctor.get("prototype");own(module,"Ray",ctor);own(module,"_home",prototype);
{auto function=JSValue::function(method1,{module},"set",2);function.setFunctionSourceText("set( origin, direction ) {\r\n\r\n\t\tthis.origin.copy( origin );\r\n\t\tthis.direction.copy( direction );\r\n\r\n\t\treturn this;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("set",descriptor);}
{auto function=JSValue::function(method2,{module},"copy",1);function.setFunctionSourceText("copy( ray ) {\r\n\r\n\t\tthis.origin.copy( ray.origin );\r\n\t\tthis.direction.copy( ray.direction );\r\n\r\n\t\treturn this;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("copy",descriptor);}
{auto function=JSValue::function(method3,{module},"at",2);function.setFunctionSourceText("at( t, target ) {\r\n\r\n\t\treturn target.copy( this.origin ).addScaledVector( this.direction, t );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("at",descriptor);}
{auto function=JSValue::function(method4,{module},"lookAt",1);function.setFunctionSourceText("lookAt( v ) {\r\n\r\n\t\tthis.direction.copy( v ).sub( this.origin ).normalize();\r\n\r\n\t\treturn this;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("lookAt",descriptor);}
{auto function=JSValue::function(method5,{module},"recast",1);function.setFunctionSourceText("recast( t ) {\r\n\r\n\t\tthis.origin.copy( this.at( t, _vector ) );\r\n\r\n\t\treturn this;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("recast",descriptor);}
{auto function=JSValue::function(method6,{module},"closestPointToPoint",2);function.setFunctionSourceText("closestPointToPoint( point, target ) {\r\n\r\n\t\ttarget.subVectors( point, this.origin );\r\n\r\n\t\tconst directionDistance = target.dot( this.direction );\r\n\r\n\t\tif ( directionDistance < 0 ) {\r\n\r\n\t\t\treturn target.copy( this.origin );\r\n\r\n\t\t}\r\n\r\n\t\treturn target.copy( this.origin ).addScaledVector( this.direction, directionDistance );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("closestPointToPoint",descriptor);}
{auto function=JSValue::function(method7,{module},"distanceToPoint",1);function.setFunctionSourceText("distanceToPoint( point ) {\r\n\r\n\t\treturn Math.sqrt( this.distanceSqToPoint( point ) );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("distanceToPoint",descriptor);}
{auto function=JSValue::function(method8,{module},"distanceSqToPoint",1);function.setFunctionSourceText("distanceSqToPoint( point ) {\r\n\r\n\t\tconst directionDistance = _vector.subVectors( point, this.origin ).dot( this.direction );\r\n\r\n\t\t// point behind the ray\r\n\r\n\t\tif ( directionDistance < 0 ) {\r\n\r\n\t\t\treturn this.origin.distanceToSquared( point );\r\n\r\n\t\t}\r\n\r\n\t\t_vector.copy( this.origin ).addScaledVector( this.direction, directionDistance );\r\n\r\n\t\treturn _vector.distanceToSquared( point );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("distanceSqToPoint",descriptor);}
{auto function=JSValue::function(method9,{module},"distanceSqToSegment",4);function.setFunctionSourceText("distanceSqToSegment( v0, v1, optionalPointOnRay, optionalPointOnSegment ) {\r\n\r\n\t\t// from https://github.com/pmjoniak/GeometricTools/blob/master/GTEngine/Include/Mathematics/GteDistRaySegment.h\r\n\t\t// It returns the min distance between the ray and the segment\r\n\t\t// defined by v0 and v1\r\n\t\t// It can also set two optional targets :\r\n\t\t// - The closest point on the ray\r\n\t\t// - The closest point on the segment\r\n\r\n\t\t_segCenter.copy( v0 ).add( v1 ).multiplyScalar( 0.5 );\r\n\t\t_segDir.copy( v1 ).sub( v0 ).normalize();\r\n\t\t_diff.copy( this.origin ).sub( _segCenter );\r\n\r\n\t\tconst segExtent = v0.distanceTo( v1 ) * 0.5;\r\n\t\tconst a01 = - this.direction.dot( _segDir );\r\n\t\tconst b0 = _diff.dot( this.direction );\r\n\t\tconst b1 = - _diff.dot( _segDir );\r\n\t\tconst c = _diff.lengthSq();\r\n\t\tconst det = Math.abs( 1 - a01 * a01 );\r\n\t\tlet s0, s1, sqrDist, extDet;\r\n\r\n\t\tif ( det > 0 ) {\r\n\r\n\t\t\t// The ray and segment are not parallel.\r\n\r\n\t\t\ts0 = a01 * b1 - b0;\r\n\t\t\ts1 = a01 * b0 - b1;\r\n\t\t\textDet = segExtent * det;\r\n\r\n\t\t\tif ( s0 >= 0 ) {\r\n\r\n\t\t\t\tif ( s1 >= - extDet ) {\r\n\r\n\t\t\t\t\tif ( s1 <= extDet ) {\r\n\r\n\t\t\t\t\t\t// region 0\r\n\t\t\t\t\t\t// Minimum at interior points of ray and segment.\r\n\r\n\t\t\t\t\t\tconst invDet = 1 / det;\r\n\t\t\t\t\t\ts0 *= invDet;\r\n\t\t\t\t\t\ts1 *= invDet;\r\n\t\t\t\t\t\tsqrDist = s0 * ( s0 + a01 * s1 + 2 * b0 ) + s1 * ( a01 * s0 + s1 + 2 * b1 ) + c;\r\n\r\n\t\t\t\t\t} else {\r\n\r\n\t\t\t\t\t\t// region 1\r\n\r\n\t\t\t\t\t\ts1 = segExtent;\r\n\t\t\t\t\t\ts0 = Math.max( 0, - ( a01 * s1 + b0 ) );\r\n\t\t\t\t\t\tsqrDist = - s0 * s0 + s1 * ( s1 + 2 * b1 ) + c;\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t} else {\r\n\r\n\t\t\t\t\t// region 5\r\n\r\n\t\t\t\t\ts1 = - segExtent;\r\n\t\t\t\t\ts0 = Math.max( 0, - ( a01 * s1 + b0 ) );\r\n\t\t\t\t\tsqrDist = - s0 * s0 + s1 * ( s1 + 2 * b1 ) + c;\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t} else {\r\n\r\n\t\t\t\tif ( s1 <= - extDet ) {\r\n\r\n\t\t\t\t\t// region 4\r\n\r\n\t\t\t\t\ts0 = Math.max( 0, - ( - a01 * segExtent + b0 ) );\r\n\t\t\t\t\ts1 = ( s0 > 0 ) ? - segExtent : Math.min( Math.max( - segExtent, - b1 ), segExtent );\r\n\t\t\t\t\tsqrDist = - s0 * s0 + s1 * ( s1 + 2 * b1 ) + c;\r\n\r\n\t\t\t\t} else if ( s1 <= extDet ) {\r\n\r\n\t\t\t\t\t// region 3\r\n\r\n\t\t\t\t\ts0 = 0;\r\n\t\t\t\t\ts1 = Math.min( Math.max( - segExtent, - b1 ), segExtent );\r\n\t\t\t\t\tsqrDist = s1 * ( s1 + 2 * b1 ) + c;\r\n\r\n\t\t\t\t} else {\r\n\r\n\t\t\t\t\t// region 2\r\n\r\n\t\t\t\t\ts0 = Math.max( 0, - ( a01 * segExtent + b0 ) );\r\n\t\t\t\t\ts1 = ( s0 > 0 ) ? segExtent : Math.min( Math.max( - segExtent, - b1 ), segExtent );\r\n\t\t\t\t\tsqrDist = - s0 * s0 + s1 * ( s1 + 2 * b1 ) + c;\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t} else {\r\n\r\n\t\t\t// Ray and segment are parallel.\r\n\r\n\t\t\ts1 = ( a01 > 0 ) ? - segExtent : segExtent;\r\n\t\t\ts0 = Math.max( 0, - ( a01 * s1 + b0 ) );\r\n\t\t\tsqrDist = - s0 * s0 + s1 * ( s1 + 2 * b1 ) + c;\r\n\r\n\t\t}\r\n\r\n\t\tif ( optionalPointOnRay ) {\r\n\r\n\t\t\toptionalPointOnRay.copy( this.origin ).addScaledVector( this.direction, s0 );\r\n\r\n\t\t}\r\n\r\n\t\tif ( optionalPointOnSegment ) {\r\n\r\n\t\t\toptionalPointOnSegment.copy( _segCenter ).addScaledVector( _segDir, s1 );\r\n\r\n\t\t}\r\n\r\n\t\treturn sqrDist;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("distanceSqToSegment",descriptor);}
{auto function=JSValue::function(method10,{module},"intersectSphere",2);function.setFunctionSourceText("intersectSphere( sphere, target ) {\r\n\r\n\t\tif ( sphere.radius < 0 ) return null; // handle empty spheres, see #31187\r\n\r\n\t\t_vector.subVectors( sphere.center, this.origin );\r\n\t\tconst tca = _vector.dot( this.direction );\r\n\t\tconst d2 = _vector.dot( _vector ) - tca * tca;\r\n\t\tconst radius2 = sphere.radius * sphere.radius;\r\n\r\n\t\tif ( d2 > radius2 ) return null;\r\n\r\n\t\tconst thc = Math.sqrt( radius2 - d2 );\r\n\r\n\t\t// t0 = first intersect point - entrance on front of sphere\r\n\t\tconst t0 = tca - thc;\r\n\r\n\t\t// t1 = second intersect point - exit point on back of sphere\r\n\t\tconst t1 = tca + thc;\r\n\r\n\t\t// test to see if t1 is behind the ray - if so, return null\r\n\t\tif ( t1 < 0 ) return null;\r\n\r\n\t\t// test to see if t0 is behind the ray:\r\n\t\t// if it is, the ray is inside the sphere, so return the second exit point scaled by t1,\r\n\t\t// in order to always return an intersect point that is in front of the ray.\r\n\t\tif ( t0 < 0 ) return this.at( t1, target );\r\n\r\n\t\t// else t0 is in front of the ray, so return the first collision point scaled by t0\r\n\t\treturn this.at( t0, target );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("intersectSphere",descriptor);}
{auto function=JSValue::function(method11,{module},"intersectsSphere",1);function.setFunctionSourceText("intersectsSphere( sphere ) {\r\n\r\n\t\tif ( sphere.radius < 0 ) return false; // handle empty spheres, see #31187\r\n\r\n\t\treturn this.distanceSqToPoint( sphere.center ) <= ( sphere.radius * sphere.radius );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("intersectsSphere",descriptor);}
{auto function=JSValue::function(method12,{module},"distanceToPlane",1);function.setFunctionSourceText("distanceToPlane( plane ) {\r\n\r\n\t\tconst denominator = plane.normal.dot( this.direction );\r\n\r\n\t\tif ( denominator === 0 ) {\r\n\r\n\t\t\t// line is coplanar, return origin\r\n\t\t\tif ( plane.distanceToPoint( this.origin ) === 0 ) {\r\n\r\n\t\t\t\treturn 0;\r\n\r\n\t\t\t}\r\n\r\n\t\t\t// Null is preferable to undefined since undefined means.... it is undefined\r\n\r\n\t\t\treturn null;\r\n\r\n\t\t}\r\n\r\n\t\tconst t = - ( this.origin.dot( plane.normal ) + plane.constant ) / denominator;\r\n\r\n\t\t// Return if the ray never intersects the plane\r\n\r\n\t\treturn t >= 0 ? t : null;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("distanceToPlane",descriptor);}
{auto function=JSValue::function(method13,{module},"intersectPlane",2);function.setFunctionSourceText("intersectPlane( plane, target ) {\r\n\r\n\t\tconst t = this.distanceToPlane( plane );\r\n\r\n\t\tif ( t === null ) {\r\n\r\n\t\t\treturn null;\r\n\r\n\t\t}\r\n\r\n\t\treturn this.at( t, target );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("intersectPlane",descriptor);}
{auto function=JSValue::function(method14,{module},"intersectsPlane",1);function.setFunctionSourceText("intersectsPlane( plane ) {\r\n\r\n\t\t// check if the ray lies on the plane first\r\n\r\n\t\tconst distToPoint = plane.distanceToPoint( this.origin );\r\n\r\n\t\tif ( distToPoint === 0 ) {\r\n\r\n\t\t\treturn true;\r\n\r\n\t\t}\r\n\r\n\t\tconst denominator = plane.normal.dot( this.direction );\r\n\r\n\t\tif ( denominator * distToPoint < 0 ) {\r\n\r\n\t\t\treturn true;\r\n\r\n\t\t}\r\n\r\n\t\t// ray origin is behind the plane (and is pointing behind it)\r\n\r\n\t\treturn false;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("intersectsPlane",descriptor);}
{auto function=JSValue::function(method15,{module},"intersectBox",2);function.setFunctionSourceText("intersectBox( box, target ) {\r\n\r\n\t\tlet tmin, tmax, tymin, tymax, tzmin, tzmax;\r\n\r\n\t\tconst invdirx = 1 / this.direction.x,\r\n\t\t\tinvdiry = 1 / this.direction.y,\r\n\t\t\tinvdirz = 1 / this.direction.z;\r\n\r\n\t\tconst origin = this.origin;\r\n\r\n\t\tif ( invdirx >= 0 ) {\r\n\r\n\t\t\ttmin = ( box.min.x - origin.x ) * invdirx;\r\n\t\t\ttmax = ( box.max.x - origin.x ) * invdirx;\r\n\r\n\t\t} else {\r\n\r\n\t\t\ttmin = ( box.max.x - origin.x ) * invdirx;\r\n\t\t\ttmax = ( box.min.x - origin.x ) * invdirx;\r\n\r\n\t\t}\r\n\r\n\t\tif ( invdiry >= 0 ) {\r\n\r\n\t\t\ttymin = ( box.min.y - origin.y ) * invdiry;\r\n\t\t\ttymax = ( box.max.y - origin.y ) * invdiry;\r\n\r\n\t\t} else {\r\n\r\n\t\t\ttymin = ( box.max.y - origin.y ) * invdiry;\r\n\t\t\ttymax = ( box.min.y - origin.y ) * invdiry;\r\n\r\n\t\t}\r\n\r\n\t\tif ( ( tmin > tymax ) || ( tymin > tmax ) ) return null;\r\n\r\n\t\tif ( tymin > tmin || isNaN( tmin ) ) tmin = tymin;\r\n\r\n\t\tif ( tymax < tmax || isNaN( tmax ) ) tmax = tymax;\r\n\r\n\t\tif ( invdirz >= 0 ) {\r\n\r\n\t\t\ttzmin = ( box.min.z - origin.z ) * invdirz;\r\n\t\t\ttzmax = ( box.max.z - origin.z ) * invdirz;\r\n\r\n\t\t} else {\r\n\r\n\t\t\ttzmin = ( box.max.z - origin.z ) * invdirz;\r\n\t\t\ttzmax = ( box.min.z - origin.z ) * invdirz;\r\n\r\n\t\t}\r\n\r\n\t\tif ( ( tmin > tzmax ) || ( tzmin > tmax ) ) return null;\r\n\r\n\t\tif ( tzmin > tmin || tmin !== tmin ) tmin = tzmin;\r\n\r\n\t\tif ( tzmax < tmax || tmax !== tmax ) tmax = tzmax;\r\n\r\n\t\t//return point closest to the ray (positive side)\r\n\r\n\t\tif ( tmax < 0 ) return null;\r\n\r\n\t\treturn this.at( tmin >= 0 ? tmin : tmax, target );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("intersectBox",descriptor);}
{auto function=JSValue::function(method16,{module},"intersectsBox",1);function.setFunctionSourceText("intersectsBox( box ) {\r\n\r\n\t\treturn this.intersectBox( box, _vector ) !== null;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("intersectsBox",descriptor);}
{auto function=JSValue::function(method17,{module},"intersectTriangle",5);function.setFunctionSourceText("intersectTriangle( a, b, c, backfaceCulling, target ) {\r\n\r\n\t\t// Watertight ray/triangle intersection. Reference: Woop, Benthin, Wald,\r\n\t\t// \"Watertight Ray/Triangle Intersection\", JCGT vol. 2 no. 1 (2013), Appendix A.\r\n\t\t// https://jcgt.org/published/0002/01/05/\r\n\r\n\t\tconst origin = this.origin;\r\n\t\tconst direction = this.direction;\r\n\r\n\t\tconst dx = direction.x;\r\n\t\tconst dy = direction.y;\r\n\t\tconst dz = direction.z;\r\n\r\n\t\t// triangle vertices relative to the ray origin\r\n\r\n\t\tconst aox = a.x - origin.x, aoy = a.y - origin.y, aoz = a.z - origin.z;\r\n\t\tconst box = b.x - origin.x, boy = b.y - origin.y, boz = b.z - origin.z;\r\n\t\tconst cox = c.x - origin.x, coy = c.y - origin.y, coz = c.z - origin.z;\r\n\r\n\t\t// Use the dimension where the ray direction is maximal as the projection\r\n\t\t// axis (kz) and read every component already permuted into (kx, ky, kz).\r\n\t\t// kx and ky are swapped when the direction's kz component is negative, to\r\n\t\t// preserve the winding order of triangles.\r\n\r\n\t\tconst adx = Math.abs( dx ), ady = Math.abs( dy ), adz = Math.abs( dz );\r\n\r\n\t\tlet dkx, dky, dkz;\r\n\t\tlet akx, aky, akz, bkx, bky, bkz, ckx, cky, ckz;\r\n\r\n\t\tif ( adx >= ady && adx >= adz ) {\r\n\r\n\t\t\tdkz = dx; akz = aox; bkz = box; ckz = cox;\r\n\r\n\t\t\tif ( dx >= 0 ) {\r\n\r\n\t\t\t\tdkx = dy; dky = dz;\r\n\t\t\t\takx = aoy; aky = aoz; bkx = boy; bky = boz; ckx = coy; cky = coz;\r\n\r\n\t\t\t} else {\r\n\r\n\t\t\t\tdkx = dz; dky = dy;\r\n\t\t\t\takx = aoz; aky = aoy; bkx = boz; bky = boy; ckx = coz; cky = coy;\r\n\r\n\t\t\t}\r\n\r\n\t\t} else if ( ady >= adz ) {\r\n\r\n\t\t\tdkz = dy; akz = aoy; bkz = boy; ckz = coy;\r\n\r\n\t\t\tif ( dy >= 0 ) {\r\n\r\n\t\t\t\tdkx = dz; dky = dx;\r\n\t\t\t\takx = aoz; aky = aox; bkx = boz; bky = box; ckx = coz; cky = cox;\r\n\r\n\t\t\t} else {\r\n\r\n\t\t\t\tdkx = dx; dky = dz;\r\n\t\t\t\takx = aox; aky = aoz; bkx = box; bky = boz; ckx = cox; cky = coz;\r\n\r\n\t\t\t}\r\n\r\n\t\t} else {\r\n\r\n\t\t\tdkz = dz; akz = aoz; bkz = boz; ckz = coz;\r\n\r\n\t\t\tif ( dz >= 0 ) {\r\n\r\n\t\t\t\tdkx = dx; dky = dy;\r\n\t\t\t\takx = aox; aky = aoy; bkx = box; bky = boy; ckx = cox; cky = coy;\r\n\r\n\t\t\t} else {\r\n\r\n\t\t\t\tdkx = dy; dky = dx;\r\n\t\t\t\takx = aoy; aky = aox; bkx = boy; bky = box; ckx = coy; cky = cox;\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\t// a zero direction has no maximal axis and cannot intersect\r\n\r\n\t\tif ( dkz === 0 ) return null;\r\n\r\n\t\t// shear constants that align the ray with the +kz axis\r\n\r\n\t\tconst sx = dkx / dkz, sy = dky / dkz, sz = 1 / dkz;\r\n\r\n\t\t// sheared and scaled vertices\r\n\r\n\t\tconst ax = akx - sx * akz, ay = aky - sy * akz;\r\n\t\tconst bx = bkx - sx * bkz, by = bky - sy * bkz;\r\n\t\tconst cx = ckx - sx * ckz, cy = cky - sy * ckz;\r\n\r\n\t\t// scaled barycentric coordinates (signed edge functions); the shear makes a\r\n\t\t// shared edge evaluate identically for both adjacent triangles, so the ray\r\n\t\t// can never fall between them\r\n\r\n\t\tconst u = cx * by - cy * bx;\r\n\t\tconst v = ax * cy - ay * cx;\r\n\t\tconst w = bx * ay - by * ax;\r\n\r\n\t\tif ( backfaceCulling ) {\r\n\r\n\t\t\tif ( u < 0 || v < 0 || w < 0 ) return null;\r\n\r\n\t\t} else {\r\n\r\n\t\t\tif ( ( u < 0 || v < 0 || w < 0 ) && ( u > 0 || v > 0 || w > 0 ) ) return null;\r\n\r\n\t\t}\r\n\r\n\t\tconst det = u + v + w;\r\n\r\n\t\t// ray is co-planar with the triangle\r\n\r\n\t\tif ( det === 0 ) return null;\r\n\r\n\t\t// scaled hit distance; t = tScaled / det must lie in front of the origin\r\n\r\n\t\tconst tScaled = sz * ( u * akz + v * bkz + w * ckz );\r\n\r\n\t\tif ( det > 0 ? tScaled < 0 : tScaled > 0 ) return null;\r\n\r\n\t\treturn this.at( tScaled / det, target );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("intersectTriangle",descriptor);}
{auto function=JSValue::function(method18,{module},"applyMatrix4",1);function.setFunctionSourceText("applyMatrix4( matrix4 ) {\r\n\r\n\t\tthis.origin.applyMatrix4( matrix4 );\r\n\t\tthis.direction.transformDirection( matrix4 );\r\n\r\n\t\treturn this;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("applyMatrix4",descriptor);}
{auto function=JSValue::function(method19,{module},"equals",1);function.setFunctionSourceText("equals( ray ) {\r\n\r\n\t\treturn ray.origin.equals( this.origin ) && ray.direction.equals( this.direction );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("equals",descriptor);}
{auto function=JSValue::function(method20,{module},"clone",0);function.setFunctionSourceText("clone() {\r\n\r\n\t\treturn new this.constructor().copy( this );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("clone",descriptor);}
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();own(facade,"Ray",ctor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeRayBinding(const std::string& name){const auto keeper=nativeRayModule();const auto state=RayManaged::lexical().lock();return name=="Ray"?state->get(name):JSValue{};}
JSValue nativeRayConstructor(){return nativeRayBinding("Ray");}
