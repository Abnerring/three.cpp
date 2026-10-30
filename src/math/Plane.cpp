#include "math/Plane.h"
#include "math/Box3.h"
#include "math/Line3.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/Sphere.h"
#include <cmath>

Plane& Plane::set(const Vector3& normal, HIGH_PRECISION constant) {
    this->normal.copy(normal);
    this->constant = constant;
    return *this;
}

Plane& Plane::setComponents(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z,
                            HIGH_PRECISION w) {
    this->normal.set(x, y, z);
    this->constant = w;
    return *this;
}

Plane& Plane::setFromNormalAndCoplanarPoint(const Vector3& normal, const Vector3& point) {
    this->normal.copy(normal);
    this->constant = -point.dot(this->normal);
    return *this;
}

Plane& Plane::setFromCoplanarPoints(const Vector3& a, const Vector3& b, const Vector3& c) {
    Vector3 _vector1, _vector2;
    const auto normal = _vector1.subVectors(c, b).cross(_vector2.subVectors(a, b)).normalize();
    this->setFromNormalAndCoplanarPoint(normal, a);
    return *this;
}

Plane& Plane::copy(const Plane& plane) {
    this->normal.copy(plane.normal);
    this->constant = plane.constant;
    return *this;
}

Plane& Plane::normalize() {
    const auto inverseNormalLength = 1.0 / this->normal.length();
    this->normal.multiplyScalar(inverseNormalLength);
    this->constant *= inverseNormalLength;
    return *this;
}

Plane& Plane::negate() {
    this->constant *= -1;
    this->normal.negate();
    return *this;
}

HIGH_PRECISION Plane::distanceToPoint(const Vector3& point) const {
    return this->normal.dot(point) + this->constant;
}

HIGH_PRECISION Plane::distanceToSphere(const Sphere& sphere) const {
    return this->distanceToPoint(sphere.center) - sphere.radius;
}

Vector3& Plane::projectPoint(const Vector3& point, Vector3& target) const {
    return target.copy(point).addScaledVector(this->normal, -this->distanceToPoint(point));
}

Vector3* Plane::intersectLine(const Line3& line, Vector3& target, bool clampToLine) const {
    Vector3 _vector1;
    const auto direction = line.delta(_vector1);
    const auto denominator = this->normal.dot(direction);
    if (denominator == 0) {
        if (this->distanceToPoint(line.start) == 0) {
            return &target.copy(line.start);
        }
        return nullptr;
    }
    const auto t = -(line.start.dot(this->normal) + this->constant) / denominator;
    if ((clampToLine == true) && (t < 0 || t > 1)) {
        return nullptr;
    }
    return &target.copy(line.start).addScaledVector(direction, t);
}

bool Plane::intersectsLine(const Line3& line) const {
    const auto startSign = this->distanceToPoint(line.start);
    const auto endSign = this->distanceToPoint(line.end);
    return (startSign < 0 && endSign > 0) || (endSign < 0 && startSign > 0);
}

bool Plane::intersectsBox(const Box3& box) const {
    return box.intersectsPlane(*this);
}

bool Plane::intersectsSphere(const Sphere& sphere) const {
    return sphere.intersectsPlane(*this);
}

Vector3& Plane::coplanarPoint(Vector3& target) const {
    return target.copy(this->normal).multiplyScalar(-this->constant);
}

Plane& Plane::translate(const Vector3& offset) {
    this->constant -= offset.dot(this->normal);
    return *this;
}

bool Plane::equals(const Plane& plane) const {
    return plane.normal.equals(this->normal) && (plane.constant == this->constant);
}

Plane::Plane(const Vector3& normal, HIGH_PRECISION constant) : normal(normal), constant(constant) {}
Plane Plane::clone() const {
    return Plane().copy(*this);
}
Plane& Plane::applyMatrix4(const Matrix4& matrix, const Matrix3* optionalNormalMatrix) {
    Matrix3 scratchNormalMatrix;
    const Matrix3& normalMatrix =
        optionalNormalMatrix ? *optionalNormalMatrix : scratchNormalMatrix.getNormalMatrix(matrix);
    Vector3 referencePoint;
    coplanarPoint(referencePoint).applyMatrix4(matrix);
    auto& transformedNormal = normal.applyMatrix3(normalMatrix).normalize();
    constant = -referencePoint.dot(transformedNormal);
    return *this;
}
JsonValue Plane::toJSON() const {
    return JsonValue::Object{{"normal", JsonValue::Array{normal.x, normal.y, normal.z}},
                             {"constant", constant}};
}
Plane& Plane::fromJSON(const JsonValue& json) {
    const auto& array = json.at("normal").get<JsonValue::Array>();
    normal.set(array.at(0).get<double>(), array.at(1).get<double>(), array.at(2).get<double>());
    constant = json.at("constant").get<double>();
    return *this;
}


// Complete managed Plane port of pinned three.js; see THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSStringEncoding.h"
#include "math/PlaneSource.inc"
#include <string_view>
namespace PlaneManaged {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,unsigned i,JSValue fallback={}){return i<a.size()&&!a[i].isUndefined()?a[i]:fallback;}
void own(JSValue o,const char* key,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(key,d);}
struct Call{JSValue receiver,callee;JSValue operator()(const Args& a={})const{return callee.call(receiver,a);}};
Call invoke(const JSValue& o,const char* key){return {o,o.get(key)};}
JSValue add(const JSValue& a,const JSValue& b){const auto x=a.toPrimitive(),y=b.toPrimitive();if(x.type()==JSValue::Type::String||y.type()==JSValue::Type::String)return x.toString()+y.toString();const auto left=x.toNumber(),right=y.toNumber();return left+right;}
bool less(const JSValue& a,const JSValue& b){const auto x=a.toPrimitive(JSValue::PreferredType::Number),y=b.toPrimitive(JSValue::PreferredType::Number);if(x.type()==JSValue::Type::String&&y.type()==JSValue::Type::String)return jsUTF16(x.string())<jsUTF16(y.string());const auto left=x.toNumber(),right=y.toNumber();return left<right;}
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue method(const JSValue& receiver,const Args& a,const Args& captures){
 auto self=receiver;const auto name=captures[0].string();const auto state=captures[1],value=arg(a,0);
 if(name=="set"||name=="copy"||name=="fromJSON"){
  const auto target=self.get("normal");const auto f=invoke(target,name=="fromJSON"?"fromArray":"copy");f({name=="set"?value:value.get("normal")});self.set("constant",name=="set"?arg(a,1):value.get("constant"));return self;
 }
 if(name=="setComponents"){const auto f=invoke(self.get("normal"),"set");f({value,arg(a,1),arg(a,2)});self.set("constant",arg(a,3));return self;}
 if(name=="setFromNormalAndCoplanarPoint"){
  invoke(self.get("normal"),"copy")({value});const auto dot=invoke(arg(a,1),"dot");const auto result=dot({self.get("normal")});self.set("constant",-result.toNumber());return self;
 }
 if(name=="setFromCoplanarPoints"){
  const auto first=invoke(state.get("_vector1"),"subVectors")({arg(a,2),arg(a,1)});const auto cross=invoke(first,"cross");const auto second=invoke(state.get("_vector2"),"subVectors")({value,arg(a,1)});const auto normal=invoke(cross({second}),"normalize")();invoke(self,"setFromNormalAndCoplanarPoint")({normal,value});return self;
 }
 if(name=="normalize"){
  const auto length=invoke(self.get("normal"),"length")();const auto inverse=1.0/length.toNumber();invoke(self.get("normal"),"multiplyScalar")({inverse});const auto previous=self.get("constant");self.set("constant",previous.toNumber()*inverse);return self;
 }
 if(name=="negate"){const auto previous=self.get("constant");self.set("constant",previous.toNumber()*-1);invoke(self.get("normal"),"negate")();return self;}
 if(name=="distanceToPoint"){const auto dot=invoke(self.get("normal"),"dot");const auto result=dot({value});const auto constant=self.get("constant");return add(result,constant);}
 if(name=="distanceToSphere"){
  const auto distance=invoke(self,"distanceToPoint");const auto result=distance({value.get("center")});const auto radius=value.get("radius");const auto left=result.toNumber(),right=radius.toNumber();return left-right;
 }
 if(name=="projectPoint"){
  const auto copied=invoke(arg(a,1),"copy")({value});const auto addScaled=invoke(copied,"addScaledVector");const auto normal=self.get("normal");const auto distance=invoke(self,"distanceToPoint")({value});return addScaled({normal,-distance.toNumber()});
 }
 if(name=="intersectLine"){
  const auto direction=invoke(value,"delta")({state.get("_vector1")});const auto denominator=invoke(self.get("normal"),"dot")({direction});
  if(denominator.strictEquals(0)){
   const auto distance=invoke(self,"distanceToPoint")({value.get("start")});if(distance.strictEquals(0)){const auto copy=invoke(arg(a,1),"copy");return copy({value.get("start")});}return nullptr;
  }
  const auto dot=invoke(value.get("start"),"dot");const auto distance=dot({self.get("normal")});const auto constant=self.get("constant");const auto numerator=-add(distance,constant).toNumber();const auto divisor=denominator.toNumber();const auto t=numerator/divisor;
  if(arg(a,2,true).strictEquals(true)&&(t<0||t>1))return nullptr;
  const auto copy=invoke(arg(a,1),"copy");const auto result=copy({value.get("start")});return invoke(result,"addScaledVector")({direction,t});
 }
 if(name=="intersectsLine"){
  const auto start=invoke(self,"distanceToPoint")({value.get("start")});const auto end=invoke(self,"distanceToPoint")({value.get("end")});return (less(start,0)&&less(0,end))||(less(end,0)&&less(0,start));
 }
 if(name=="intersectsBox")return invoke(value,"intersectsPlane")({self});
 if(name=="intersectsSphere")return invoke(value,"intersectsPlane")({self});
 if(name=="coplanarPoint"){const auto copied=invoke(value,"copy")({self.get("normal")});const auto multiply=invoke(copied,"multiplyScalar");return multiply({-self.get("constant").toNumber()});}
 if(name=="applyMatrix4"){
  auto normalMatrix=arg(a,1);if(!normalMatrix.toBoolean())normalMatrix=invoke(state.get("_normalMatrix"),"getNormalMatrix")({value});
  const auto point=invoke(self,"coplanarPoint")({state.get("_vector1")});const auto reference=invoke(point,"applyMatrix4")({value});
  const auto transformed=invoke(self.get("normal"),"applyMatrix3")({normalMatrix});const auto normal=invoke(transformed,"normalize")();const auto dot=invoke(reference,"dot");self.set("constant",-dot({normal}).toNumber());return self;
 }
 if(name=="translate"){
  const auto previous=self.get("constant");const auto dot=invoke(value,"dot");const auto amount=dot({self.get("normal")});const auto left=previous.toNumber(),right=amount.toNumber();self.set("constant",left-right);return self;
 }
 if(name=="equals"){
  const auto equals=invoke(value.get("normal"),"equals");const auto result=equals({self.get("normal")});if(!result.toBoolean())return result;const auto constant=value.get("constant");return constant.strictEquals(self.get("constant"));
 }
 if(name=="clone"){const auto clone=self.get("constructor").construct();return invoke(clone,"copy")({self});}
 if(name=="toJSON"){auto output=JSValue::object();own(output,"normal",invoke(self.get("normal"),"toArray")());own(output,"constant",self.get("constant"));return output;}
 throw std::logic_error("Unknown Plane method");
}
}
JSValue nativePlaneModule(){
 using namespace PlaneManaged;static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
 static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"Plane module");
 const auto matrix=nativeMatrix3Constructor(),vector=nativeVector3Constructor();auto state=JSValue::object(nullptr);
 own(state,"_vector1",vector.construct());own(state,"_vector2",vector.construct());own(state,"_normalMatrix",matrix.construct());
 auto ctor=JSValue::constructor(+[](const JSValue& receiver,const Args& a,const Args& captures)->JSValue{
  const auto normal=arg(a,0).isUndefined()?captures[0].construct({1,0,0}):arg(a,0);const auto constant=arg(a,1,0);auto self=receiver;self.set("isPlane",true);self.set("normal",normal);self.set("constant",constant);return {};
 },{vector},"Plane",0);ctor.setFunctionSourceText(PlaneSource::classSource);
 for(const auto& spec:PlaneSource::methods){if(std::string_view(spec.name)=="constructor")continue;auto f=JSValue::function(method,{spec.name,state},spec.name,spec.arity);f.setFunctionSourceText(spec.source);JSPropertyDescriptor d;d.value=f;d.writable=d.configurable=true;ctor.get("prototype").defineProperty(spec.name,d);}
 own(state,"Plane",ctor);state.retainForHeapLifetime();lexical()=JSWeakValue(state);auto module=JSValue::object();own(module,"Plane",ctor);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativePlaneBinding(const std::string& name){const auto keeper=nativePlaneModule();return PlaneManaged::lexical().lock()->get(name);}
JSValue nativePlaneConstructor(){return nativePlaneBinding("Plane");}
