#include "math/Sphere.h"
#include "math/Box3.h"
#include "math/MathUtils.h"
#include "math/Matrix4.h"
#include "math/Plane.h"
#include <algorithm>
#include <cmath>
Sphere::Sphere(const Vector3& c, double r) : center(c), radius(r) {}
Sphere& Sphere::set(const Vector3& c, double r) {
    center.copy(c);
    radius = r;
    return *this;
}
Sphere& Sphere::copy(const Sphere& s) {
    return set(s.center, s.radius);
}
Sphere Sphere::clone() const {
    return Sphere(center, radius);
}
Sphere& Sphere::setFromPoints(const std::vector<Vector3>& p, const Vector3* c) {
    if (c)
        center.copy(*c);
    else {
        Box3 box;
        box.setFromPoints(p).getCenter(center);
    }
    double maxRadiusSq = 0;
    for (const auto& v : p)
        maxRadiusSq = MathUtils::max(maxRadiusSq, center.distanceToSquared(v));
    radius = std::sqrt(maxRadiusSq);
    return *this;
}
Sphere& Sphere::makeEmpty() {
    center.set(0, 0, 0);
    radius = -1;
    return *this;
}
bool Sphere::isEmpty() const {
    return radius < 0;
}
bool Sphere::containsPoint(const Vector3& p) const {
    return p.distanceToSquared(center) <= radius * radius;
}
double Sphere::distanceToPoint(const Vector3& p) const {
    return p.distanceTo(center) - radius;
}
bool Sphere::intersectsSphere(const Sphere& s) const {
    const double r = radius + s.radius;
    return s.center.distanceToSquared(center) <= r * r;
}
bool Sphere::intersectsBox(const Box3& b) const {
    return b.intersectsSphere(*this);
}
bool Sphere::intersectsPlane(const Plane& plane) const {
    return std::abs(plane.distanceToPoint(center)) <= radius;
}
Vector3& Sphere::clampPoint(const Vector3& p, Vector3& t) const {
    const double distanceSq = center.distanceToSquared(p);
    t.copy(p);
    if (distanceSq > radius * radius) {
        t.sub(center);
        t.normalize();
        t.multiplyScalar(radius);
        t.add(center);
    }
    return t;
}
Box3& Sphere::getBoundingBox(Box3& b) const {
    if (isEmpty())
        b.makeEmpty();
    else
        b.set(center, center).expandByScalar(radius);
    return b;
}
Sphere& Sphere::applyMatrix4(const Matrix4& m) {
    center.applyMatrix4(m);
    const auto e = m.elements();
    const double sx = e[0] * e[0] + e[1] * e[1] + e[2] * e[2],
                 sy = e[4] * e[4] + e[5] * e[5] + e[6] * e[6],
                 sz = e[8] * e[8] + e[9] * e[9] + e[10] * e[10];
    radius *= std::sqrt(MathUtils::max(sx, MathUtils::max(sy, sz)));
    return *this;
}
Sphere& Sphere::translate(const Vector3& v) {
    center.add(v);
    return *this;
}
Sphere& Sphere::expandByPoint(const Vector3& p) {
    if (isEmpty())
        return set(p, 0);
    Vector3 v;
    v.subVectors(p, center);
    const double lengthSq = v.lengthSq();
    if (lengthSq > radius * radius) {
        const double length = std::sqrt(lengthSq);
        const double delta = (length - radius) * .5;
        v.multiplyScalar(delta / length);
        center.add(v);
        radius += delta;
    }
    return *this;
}
Sphere& Sphere::unionWith(const Sphere& s) {
    if (s.isEmpty())
        return *this;
    if (isEmpty())
        return copy(s);
    if (center.equals(s.center)) {
        radius = MathUtils::max(radius, s.radius);
        return *this;
    }
    Vector3 v;
    v.subVectors(s.center, center);
    v.normalize();
    v.multiplyScalar(s.radius);
    Vector3 p(s.center);
    p.add(v);
    expandByPoint(p);
    p.copy(s.center);
    p.sub(v);
    expandByPoint(p);
    return *this;
}
bool Sphere::equals(const Sphere& s) const {
    return center.equals(s.center) && radius == s.radius;
}

// Upstream serialises the centre as an array plus the radius.
JSValue Sphere::toJSON() const {
    auto output = JSValue::object();
    auto center = JSValue::array();
    for (const double value : {this->center.x, this->center.y, this->center.z})
        center.get("push").call(center, {JSValue(value)});
    output.set("center", center);
    output.set("radius", radius);
    return output;
}
Sphere& Sphere::fromJSON(const JSValue& json) {
    const auto array = json.get("center");
    center.x = array.get("0").toNumber();
    center.y = array.get("1").toNumber();
    center.z = array.get("2").toNumber();
    radius = json.get("radius").toNumber();
    return *this;
}
// Complete managed Sphere source implementation; see THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "core/JSStringEncoding.h"
#include "math/SphereSource.inc"
#include <string_view>

namespace SphereManaged {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,unsigned i){return i<a.size()?a[i]:JSValue{};}
void own(JSValue o,const char* key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;o.defineProperty(key,d);}
struct Call{JSValue receiver,callee;JSValue operator()(const Args& a={})const{return callee.call(receiver,a);}};
Call invoke(const JSValue& o,const char* key){return{o,o.get(key)};}
JSValue math(){return nativeGlobalBinding("Math");}
template<class L,class R>JSValue sum(L left,R right){const auto a=left(),b=right();const auto x=a.toPrimitive(),y=b.toPrimitive();if(x.type()==JSValue::Type::String||y.type()==JSValue::Type::String){const auto l=x.toString(),r=y.toString();return l+r;}const auto l=x.toNumber(),r=y.toNumber();return l+r;}
template<class L,class R>double numberOp(char op,L left,R right){const auto a=left(),b=right();const auto x=a.toNumber(),y=b.toNumber();return op=='-'?x-y:op=='*'?x*y:x/y;}
template<class L,class R>bool relation(char op,L left,R right){const auto a=left(),b=right();const auto x=a.toPrimitive(JSValue::PreferredType::Number),y=b.toPrimitive(JSValue::PreferredType::Number);if(x.type()==JSValue::Type::String&&y.type()==JSValue::Type::String){const auto l=jsUTF16(x.string()),r=jsUTF16(y.string());return op=='<'?l<r:op=='>'?l>r:l<=r;}const auto l=x.toNumber(),r=y.toNumber();return op=='<'?l<r:op=='>'?l>r:l<=r;}
#define SPADD(a,b) sum([&]()->JSValue{return a;},[&]()->JSValue{return b;})
#define SPSUB(a,b) numberOp('-',[&]()->JSValue{return a;},[&]()->JSValue{return b;})
#define SPMUL(a,b) numberOp('*',[&]()->JSValue{return a;},[&]()->JSValue{return b;})
#define SPDIV(a,b) numberOp('/',[&]()->JSValue{return a;},[&]()->JSValue{return b;})
#define SPLT(a,b) relation('<',[&]()->JSValue{return a;},[&]()->JSValue{return b;})
#define SPGT(a,b) relation('>',[&]()->JSValue{return a;},[&]()->JSValue{return b;})
#define SPLE(a,b) relation('L',[&]()->JSValue{return a;},[&]()->JSValue{return b;})
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
 auto self=receiver;const auto name=captures[0].string();const auto state=captures[1],value=arg(args,0);
 if(name=="set"||name=="copy"){invoke(self.get("center"),"copy")({name=="set"?value:value.get("center")});self.set("radius",name=="set"?arg(args,1):value.get("radius"));return self;}
 if(name=="setFromPoints"){
  const auto center=self.get("center"),optionalCenter=arg(args,1);
  if(!optionalCenter.isUndefined())invoke(center,"copy")({optionalCenter});else invoke(invoke(state.get("_box"),"setFromPoints")({value}),"getCenter")({center});
  JSValue maxRadiusSq=0;const auto length=value.get("length");
  for(double i=0;SPLT(i,length);++i)maxRadiusSq=invoke(math(),"max")({maxRadiusSq,invoke(center,"distanceToSquared")({value.get(JSValue(i))})});
  self.set("radius",invoke(math(),"sqrt")({maxRadiusSq}));return self;
 }
 if(name=="isEmpty")return SPLT(self.get("radius"),0);
 if(name=="makeEmpty"){invoke(self.get("center"),"set")({0,0,0});self.set("radius",-1);return self;}
 if(name=="containsPoint")return SPLE(invoke(value,"distanceToSquared")({self.get("center")}),SPMUL(self.get("radius"),self.get("radius")));
 if(name=="distanceToPoint")return SPSUB(invoke(value,"distanceTo")({self.get("center")}),self.get("radius"));
 if(name=="intersectsSphere"){
  const auto radiusSum=SPADD(self.get("radius"),value.get("radius"));
  return SPLE(invoke(value.get("center"),"distanceToSquared")({self.get("center")}),SPMUL(radiusSum,radiusSum));
 }
 if(name=="intersectsBox")return invoke(value,"intersectsSphere")({self});
 if(name=="intersectsPlane")return SPLE(invoke(math(),"abs")({invoke(value,"distanceToPoint")({self.get("center")})}),self.get("radius"));
 if(name=="clampPoint"){
  const auto target=arg(args,1),deltaLengthSq=invoke(self.get("center"),"distanceToSquared")({value});
  invoke(target,"copy")({value});
  if(SPGT(deltaLengthSq,SPMUL(self.get("radius"),self.get("radius")))){
   invoke(invoke(target,"sub")({self.get("center")}),"normalize")();
   invoke(invoke(target,"multiplyScalar")({self.get("radius")}),"add")({self.get("center")});
  }return target;
 }
 if(name=="getBoundingBox"){
  if(invoke(self,"isEmpty")().toBoolean()){invoke(value,"makeEmpty")();return value;}
  invoke(value,"set")({self.get("center"),self.get("center")});invoke(value,"expandByScalar")({self.get("radius")});return value;
 }
 if(name=="applyMatrix4"){
  invoke(self.get("center"),"applyMatrix4")({value});self.set("radius",SPMUL(self.get("radius"),invoke(value,"getMaxScaleOnAxis")()));return self;
 }
 if(name=="translate"){invoke(self.get("center"),"add")({value});return self;}
 if(name=="expandByPoint"){
  if(invoke(self,"isEmpty")().toBoolean()){invoke(self.get("center"),"copy")({value});self.set("radius",0);return self;}
  const auto v1=state.get("_v1");invoke(v1,"subVectors")({value,self.get("center")});const auto lengthSq=invoke(v1,"lengthSq")();
  if(SPGT(lengthSq,SPMUL(self.get("radius"),self.get("radius")))){
   const auto length=invoke(math(),"sqrt")({lengthSq});const auto delta=SPSUB(length,self.get("radius"))*0.5;
   invoke(self.get("center"),"addScaledVector")({v1,SPDIV(delta,length)});self.set("radius",SPADD(self.get("radius"),delta));
  }return self;
 }
 if(name=="union"){
  if(invoke(value,"isEmpty")().toBoolean())return self;
  if(invoke(self,"isEmpty")().toBoolean()){invoke(self,"copy")({value});return self;}
  if(invoke(self.get("center"),"equals")({value.get("center")}).strictEquals(true))self.set("radius",invoke(math(),"max")({self.get("radius"),value.get("radius")}));
  else{
   const auto v1=state.get("_v1"),v2=state.get("_v2");invoke(invoke(v2,"subVectors")({value.get("center"),self.get("center")}),"setLength")({value.get("radius")});
   invoke(self,"expandByPoint")({invoke(invoke(v1,"copy")({value.get("center")}),"add")({v2})});
   invoke(self,"expandByPoint")({invoke(invoke(v1,"copy")({value.get("center")}),"sub")({v2})});
  }return self;
 }
 if(name=="equals"){const auto result=invoke(value.get("center"),"equals")({self.get("center")});if(!result.toBoolean())return result;const auto radius=value.get("radius");return radius.strictEquals(self.get("radius"));}
 if(name=="clone"){const auto target=self.get("constructor").construct();return invoke(target,"copy")({self});}
 if(name=="toJSON"){auto result=JSValue::object();own(result,"radius",self.get("radius"));own(result,"center",invoke(self.get("center"),"toArray")());return result;}
 if(name=="fromJSON"){self.set("radius",value.get("radius"));invoke(self.get("center"),"fromArray")({value.get("center")});return self;}
 throw std::logic_error("Unknown managed Sphere method: "+name);
}
#undef SPADD
#undef SPSUB
#undef SPMUL
#undef SPDIV
#undef SPLT
#undef SPGT
#undef SPLE
}
JSValue nativeSphereModule(){
 using namespace SphereManaged;static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
 static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Sphere module");
 const auto box=nativeBox3Binding("Box3"),vector=nativeVector3Constructor();auto state=JSValue::object(nullptr);
 own(state,"Box3",box);own(state,"Vector3",vector);own(state,"_box",box.construct());own(state,"_v1",vector.construct());own(state,"_v2",vector.construct());
 auto ctor=JSValue::constructor(+[](const JSValue& receiver,const Args& args,const Args& captures)->JSValue{
  const auto center=arg(args,0).isUndefined()?captures[0].construct():arg(args,0);const auto radius=arg(args,1).isUndefined()?JSValue(-1):arg(args,1);
  auto self=receiver;self.set("isSphere",true);self.set("center",center);self.set("radius",radius);return {};
 },{vector},"Sphere",0);ctor.setFunctionSourceText(SphereSource::classSource);own(state,"Sphere",ctor);
 auto prototype=ctor.get("prototype");for(const auto& spec:SphereSource::methods){if(std::string_view(spec.name)=="constructor")continue;auto fn=JSValue::function(method,{spec.name,state},spec.name,spec.arity);fn.setFunctionSourceText(spec.source);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty(spec.name,d);}
 auto bindings=JSValue::object(nullptr);own(bindings,"Sphere",ctor);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);
 auto module=JSValue::object();own(module,"Sphere",ctor);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeSphereBinding(const std::string& name){const auto keeper=nativeSphereModule();return SphereManaged::lexical().lock()->get(name);}
JSValue nativeSphereConstructor(){return nativeSphereBinding("Sphere");}
