#include "math/Box3.h"
#include "core/VertexAttribute.h"
#include "math/MathUtils.h"
#include "math/Matrix4.h"
#include "math/Plane.h"
#include "math/Sphere.h"
#include "math/Triangle.h"
#include "objects/SkinnedMesh.h"
#include "objects/InstancedMesh.h"
#include "objects/Line.h"
#include "objects/Points.h"
#include "objects/Sprite.h"
#include <cmath>
#include <span>
Box3::Box3(const Vector3& a, const Vector3& b) : min(a), max(b) {}
Box3& Box3::setFromObject(Object3D& object, bool precise) {
    makeEmpty(); return expandByObject(object, precise);
}
Box3& Box3::expandByObject(Object3D& object, bool precise) {
    static thread_local Vector3 vector;
    static thread_local Box3 box;
    object.updateWorldMatrix(false, false);
    auto* mesh = dynamic_cast<Mesh*>(&object);
    const auto geometry = mesh ? mesh->geometry : dynamic_cast<Line*>(&object) ? dynamic_cast<Line&>(object).geometry
        : dynamic_cast<Points*>(&object) ? dynamic_cast<Points&>(object).geometry
        : dynamic_cast<Sprite*>(&object) ? dynamic_cast<Sprite&>(object).geometry : nullptr;
    if (mesh || dynamic_cast<Line*>(&object) || dynamic_cast<Points*>(&object) || dynamic_cast<Sprite*>(&object)) {
        if (!geometry) throw JSTypeError("Cannot read properties of null (reading 'getAttribute')");
        const auto position = geometry->getAttribute("position");
        if (precise && position && !object.isInstancedMesh) {
            const auto count = position->getCount();
            for (std::size_t i = 0; double(i) < count; ++i) {
                if (object.isMesh && mesh) mesh->getVertexPosition(i, vector);
                else vector.fromBufferAttribute(*position,double(i));
                vector.applyMatrix4(object.matrixWorld); expandByPoint(vector);
            }
        } else {
            if (auto* skinned = dynamic_cast<SkinnedMesh*>(mesh)) {
                if (!skinned->boundingBox) skinned->computeBoundingBox();
                if (!skinned->boundingBox) throw JSTypeError("Cannot read properties of null (reading 'min')");
                box.copy(*skinned->boundingBox);
            } else if (auto* instanced = dynamic_cast<InstancedMesh*>(mesh)) {
                if (!instanced->boundingBox) instanced->computeBoundingBox();
                if (!instanced->boundingBox) throw JSTypeError("Cannot read properties of null (reading 'min')");
                box.copy(*instanced->boundingBox);
            } else {
                if (!geometry->boundingBox) geometry->computeBoundingBox();
                if (!geometry->boundingBox) throw JSTypeError("Cannot read properties of null (reading 'min')");
                box.copy(*geometry->boundingBox);
            }
            box.applyMatrix4(object.matrixWorld); unionWith(box);
        }
    }
    const auto children = object.children;
    const auto count = children.size();
    for (std::size_t i = 0; i < count; ++i) {
        if (i >= children.size() || children.storage()->holes.contains(i) || !children[i])
            throw JSTypeError("Cannot read properties of undefined (reading 'updateWorldMatrix')");
        auto child = children[i]; expandByObject(*child, precise);
    }
    return *this;
}

namespace {
bool satForAxes(std::span<const double> axes, const Vector3& v0, const Vector3& v1,
                const Vector3& v2, const Vector3& extents) {
    Vector3 testAxis;
    for (std::size_t i = 0; i < axes.size(); i += 3) {
        testAxis.set(axes[i], axes[i + 1], axes[i + 2]);
        const double r = extents.x * std::abs(testAxis.x) +
                         extents.y * std::abs(testAxis.y) +
                         extents.z * std::abs(testAxis.z);
        const double p0 = v0.dot(testAxis), p1 = v1.dot(testAxis), p2 = v2.dot(testAxis);
        if (MathUtils::max(-MathUtils::max(MathUtils::max(p0, p1), p2),
                           MathUtils::min(MathUtils::min(p0, p1), p2)) > r)
            return false;
    }
    return true;
}
} // namespace

bool Box3::intersectsPlane(const Plane& plane) const {
    double minimum, maximum;
    if (plane.normal.x > 0) {
        minimum = plane.normal.x * min.x;
        maximum = plane.normal.x * max.x;
    } else {
        minimum = plane.normal.x * max.x;
        maximum = plane.normal.x * min.x;
    }
    if (plane.normal.y > 0) {
        minimum += plane.normal.y * min.y;
        maximum += plane.normal.y * max.y;
    } else {
        minimum += plane.normal.y * max.y;
        maximum += plane.normal.y * min.y;
    }
    if (plane.normal.z > 0) {
        minimum += plane.normal.z * min.z;
        maximum += plane.normal.z * max.z;
    } else {
        minimum += plane.normal.z * max.z;
        maximum += plane.normal.z * min.z;
    }
    return minimum <= -plane.constant && maximum >= -plane.constant;
}

bool Box3::intersectsTriangle(const Triangle& triangle) const {
    if (isEmpty())
        return false;
    Vector3 center, extents, v0, v1, v2, f0, f1, f2, triangleNormal;
    getCenter(center);
    extents.subVectors(max, center);
    v0.subVectors(triangle.a, center);
    v1.subVectors(triangle.b, center);
    v2.subVectors(triangle.c, center);
    f0.subVectors(v1, v0);
    f1.subVectors(v2, v1);
    f2.subVectors(v0, v2);
    const double edges[] = {0, -f0.z, f0.y, 0, -f1.z, f1.y, 0, -f2.z, f2.y,  f0.z,
                            0, -f0.x, f1.z, 0, -f1.x, f2.z, 0, -f2.x, -f0.y, f0.x,
                            0, -f1.y, f1.x, 0, -f2.y, f2.x, 0};
    if (!satForAxes(edges, v0, v1, v2, extents))
        return false;
    const double faces[] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    if (!satForAxes(faces, v0, v1, v2, extents))
        return false;
    triangleNormal.crossVectors(f0, f1);
    const double normal[] = {triangleNormal.x, triangleNormal.y, triangleNormal.z};
    return satForAxes(normal, v0, v1, v2, extents);
}
Box3& Box3::set(const Vector3& a, const Vector3& b) {
    min.copy(a);
    max.copy(b);
    return *this;
}
Box3& Box3::copy(const Box3& b) {
    return set(b.min, b.max);
}
Box3 Box3::clone() const {
    return Box3(min, max);
}
Box3& Box3::makeEmpty() {
    min.set(INFINITY, INFINITY, INFINITY);
    max.set(-INFINITY, -INFINITY, -INFINITY);
    return *this;
}
bool Box3::isEmpty() const {
    return max.x < min.x || max.y < min.y || max.z < min.z;
}
Box3& Box3::setFromArray(const std::vector<double>& a) {
    makeEmpty();
    for (std::size_t i = 0; i + 2 < a.size(); i += 3)
        expandByPoint(Vector3(a[i], a[i + 1], a[i + 2]));
    return *this;
}
Box3& Box3::setFromPoints(const std::vector<Vector3>& p) {
    makeEmpty();
    for (const auto& v : p)
        expandByPoint(v);
    return *this;
}
Box3& Box3::setFromBufferAttribute(const VertexAttribute& a) {
    makeEmpty();
    Vector3 vector;
    for (std::size_t i = 0; i < a.getCount(); ++i) {
        vector.fromBufferAttribute(a, double(i));
        expandByPoint(vector);
    }
    return *this;
}
Box3& Box3::setFromCenterAndSize(const Vector3& c, const Vector3& s) {
    Vector3 half(s);
    half.multiplyScalar(.5);
    min.copy(c);
    min.sub(half);
    max.copy(c);
    max.add(half);
    return *this;
}
Vector3& Box3::getCenter(Vector3& t) const {
    if (isEmpty())
        t.set(0, 0, 0);
    else {
        t.addVectors(min, max);
        t.multiplyScalar(.5);
    }
    return t;
}
Vector3& Box3::getSize(Vector3& t) const {
    if (isEmpty())
        t.set(0, 0, 0);
    else
        t.subVectors(max, min);
    return t;
}
Box3& Box3::expandByPoint(const Vector3& p) {
    min.min(p);
    max.max(p);
    return *this;
}
Box3& Box3::expandByVector(const Vector3& v) {
    min.sub(v);
    max.add(v);
    return *this;
}
Box3& Box3::expandByScalar(double s) {
    min.addScalar(-s);
    max.addScalar(s);
    return *this;
}
bool Box3::containsPoint(const Vector3& p) const {
    return !(p.x < min.x || p.x > max.x || p.y < min.y || p.y > max.y ||
             p.z < min.z || p.z > max.z);
}
bool Box3::containsBox(const Box3& b) const {
    return min.x <= b.min.x && b.max.x <= max.x && min.y <= b.min.y &&
           b.max.y <= max.y && min.z <= b.min.z && b.max.z <= max.z;
}
bool Box3::intersectsBox(const Box3& b) const {
    return !(b.max.x < min.x || b.min.x > max.x || b.max.y < min.y ||
             b.min.y > max.y || b.max.z < min.z || b.min.z > max.z);
}
bool Box3::intersectsSphere(const Sphere& s) const {
    Vector3 v;
    clampPoint(s.center, v);
    return v.distanceToSquared(s.center) <= s.radius * s.radius;
}
Vector3& Box3::clampPoint(const Vector3& p, Vector3& t) const {
    t.copy(p);
    t.clamp(min, max);
    return t;
}
double Box3::distanceToPoint(const Vector3& p) const {
    Vector3 v;
    clampPoint(p, v);
    return v.distanceTo(p);
}
Vector3& Box3::getParameter(const Vector3& p, Vector3& t) const {
    t.set((p.x - min.x) / (max.x - min.x), (p.y - min.y) / (max.y - min.y),
          (p.z - min.z) / (max.z - min.z));
    return t;
}
Sphere& Box3::getBoundingSphere(Sphere& s) const {
    if (isEmpty())
        s.makeEmpty();
    else {
        Vector3 v;
        getCenter(s.center);
        s.radius = getSize(v).length() * .5;
    }
    return s;
}
Box3& Box3::intersect(const Box3& b) {
    min.max(b.min);
    max.min(b.max);
    if (isEmpty())
        makeEmpty();
    return *this;
}
Box3& Box3::unionWith(const Box3& b) {
    min.min(b.min);
    max.max(b.max);
    return *this;
}
Box3& Box3::applyMatrix4(const Matrix4& m) {
    if (isEmpty())
        return *this;
    const Vector3 a(min), b(max);
    makeEmpty();
    for (int i = 0; i < 8; ++i) {
        Vector3 v(i & 1 ? b.x : a.x, i & 2 ? b.y : a.y, i & 4 ? b.z : a.z);
        v.applyMatrix4(m);
        expandByPoint(v);
    }
    return *this;
}
Box3& Box3::translate(const Vector3& v) {
    min.add(v);
    max.add(v);
    return *this;
}
bool Box3::equals(const Box3& b) const {
    return min.equals(b.min) && max.equals(b.max);
}

// Upstream serialises the two corners as plain arrays; the same shape is produced here so a saved box
// round trips through fromJSON.
JSValue Box3::toJSON() const {
    auto output = JSValue::object();
    auto minArray = JSValue::array(), maxArray = JSValue::array();
    for (const double value : {min.x, min.y, min.z}) minArray.get("push").call(minArray, {JSValue(value)});
    for (const double value : {max.x, max.y, max.z}) maxArray.get("push").call(maxArray, {JSValue(value)});
    output.set("min", minArray);
    output.set("max", maxArray);
    return output;
}
Box3& Box3::fromJSON(const JSValue& json) {
    const auto read = [&](const char* name, Vector3& target) {
        const auto array = json.get(name);
        target.x = array.get("0").toNumber();
        target.y = array.get("1").toNumber();
        target.z = array.get("2").toNumber();
    };
    read("min", min);
    read("max", max);
    return *this;
}// Complete managed Box3 source implementation; see THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "core/JSStringEncoding.h"
#include "math/Box3Source.inc"
#include <string_view>

namespace Box3Managed {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,unsigned i,const JSValue& fallback={}){return i<a.size()&&!a[i].isUndefined()?a[i]:fallback;}
void own(JSValue o,const char* k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
struct Call{JSValue receiver,callee;JSValue operator()(const Args& a={})const{return callee.call(receiver,a);}};
Call invoke(const JSValue& o,const char* k){return {o,o.get(k)};}
JSValue math(){return nativeGlobalBinding("Math");}
template<class L,class R>double numberOp(char op,L left,R right){const auto a=left(),b=right();const auto x=a.toNumber(),y=b.toNumber();return op=='-'?x-y:op=='*'?x*y:x/y;}
template<class L,class R>bool relation(char op,L left,R right){const auto leftValue=left(),rightValue=right();const auto a=leftValue.toPrimitive(JSValue::PreferredType::Number),b=rightValue.toPrimitive(JSValue::PreferredType::Number);if(a.type()==JSValue::Type::String&&b.type()==JSValue::Type::String){const auto x=jsUTF16(a.string()),y=jsUTF16(b.string());return op=='<'?x<y:op=='>'?x>y:op=='L'?x<=y:x>=y;}const auto x=a.toNumber(),y=b.toNumber();return op=='<'?x<y:op=='>'?x>y:op=='L'?x<=y:x>=y;}
#define B3SUB(a,b) numberOp('-',[&]()->JSValue{return a;},[&]()->JSValue{return b;})
#define B3MUL(a,b) numberOp('*',[&]()->JSValue{return a;},[&]()->JSValue{return b;})
#define B3DIV(a,b) numberOp('/',[&]()->JSValue{return a;},[&]()->JSValue{return b;})
#define B3LT(a,b) relation('<',[&]()->JSValue{return a;},[&]()->JSValue{return b;})
#define B3GT(a,b) relation('>',[&]()->JSValue{return a;},[&]()->JSValue{return b;})
#define B3LE(a,b) relation('L',[&]()->JSValue{return a;},[&]()->JSValue{return b;})
#define B3GE(a,b) relation('G',[&]()->JSValue{return a;},[&]()->JSValue{return b;})
JSValue literal(const Args& values){auto a=JSValue::array();for(unsigned i=0;i<values.size();++i){JSPropertyDescriptor d;d.value=values[i];d.writable=d.enumerable=d.configurable=true;a.defineProperty(std::to_string(i),d);}return a;}
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
JSValue sat(const JSValue&,const Args& a,const Args& captures){
 const auto axes=arg(a,0),v0=arg(a,1),v1=arg(a,2),v2=arg(a,3),extents=arg(a,4),test=captures[0].get("_testAxis");
 const auto j=B3SUB(axes.get("length"),3);
 for(double i=0;i<=j;i+=3){
  invoke(test,"fromArray")({axes,i});
  const auto x=B3MUL(extents.get("x"),invoke(math(),"abs")({test.get("x")}));
  const auto y=B3MUL(extents.get("y"),invoke(math(),"abs")({test.get("y")}));
  const auto xy=x+y;
  const auto z=B3MUL(extents.get("z"),invoke(math(),"abs")({test.get("z")}));const auto r=xy+z;
  const auto p0=invoke(v0,"dot")({test}),p1=invoke(v1,"dot")({test}),p2=invoke(v2,"dot")({test});
  const auto outer=invoke(math(),"max");const auto neg=-invoke(math(),"max")({p0,p1,p2}).toNumber();const auto minimum=invoke(math(),"min")({p0,p1,p2});
  if(B3GT(outer({neg,minimum}),r))return false;
 }
 return true;
}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
 auto self=receiver;const auto name=captures[0].string();const auto state=captures[1];const auto value=arg(args,0);
 if(name=="set"||name=="copy"||name=="fromJSON"){
  const char* method=name=="fromJSON"?"fromArray":"copy";
  invoke(self.get("min"),method)({name=="set"?value:value.get("min")});
  invoke(self.get("max"),method)({name=="set"?arg(args,1):value.get("max")});return self;
 }
 if(name=="clone"){const auto result=self.get("constructor").construct();return invoke(result,"copy")({self});}
 if(name=="makeEmpty"){
  for(const auto* side:{"min","max"}){const auto x=self.get(side),y=self.get(side),z=self.get(side);const auto infinity=side[1]=='i'?INFINITY:-INFINITY;auto mz=z; mz.set("z",infinity);auto my=y;my.set("y",infinity);auto mx=x;mx.set("x",infinity);}return self;
 }
 if(name=="isEmpty")return B3LT(self.get("max").get("x"),self.get("min").get("x"))||B3LT(self.get("max").get("y"),self.get("min").get("y"))||B3LT(self.get("max").get("z"),self.get("min").get("z"));
 if(name=="setFromArray"||name=="setFromBufferAttribute"||name=="setFromPoints"){
  invoke(self,"makeEmpty")();const auto length=value.get(name=="setFromBufferAttribute"?"count":"length");
  for(double i=0;B3LT(i,length);i+=name=="setFromArray"?3:1){const auto expand=invoke(self,"expandByPoint");expand({name=="setFromPoints"?value.get(JSValue(i)):invoke(state.get("_vector"),name=="setFromArray"?"fromArray":"fromBufferAttribute")({value,i})});}return self;
 }
 if(name=="setFromCenterAndSize"){
  const auto half=invoke(invoke(state.get("_vector"),"copy")({arg(args,1)}),"multiplyScalar")({0.5});
  invoke(invoke(self.get("min"),"copy")({value}),"sub")({half});invoke(invoke(self.get("max"),"copy")({value}),"add")({half});return self;
 }
 if(name=="setFromObject"){invoke(self,"makeEmpty")();return invoke(self,"expandByObject")({value,arg(args,1,false)});}
 if(name=="getCenter"||name=="getSize"){
  if(invoke(self,"isEmpty")().toBoolean())return invoke(value,"set")({0,0,0});
  if(name=="getSize")return invoke(value,"subVectors")({self.get("max"),self.get("min")});
  return invoke(invoke(value,"addVectors")({self.get("min"),self.get("max")}),"multiplyScalar")({0.5});
 }
 if(name=="expandByPoint"||name=="expandByVector"||name=="translate"){
  invoke(self.get("min"),name=="expandByPoint"?"min":name=="expandByVector"?"sub":"add")({value});
  invoke(self.get("max"),name=="expandByPoint"?"max":"add")({value});return self;
 }
 if(name=="expandByScalar"){invoke(self.get("min"),"addScalar")({-value.toNumber()});invoke(self.get("max"),"addScalar")({value});return self;}
 if(name=="expandByObject"){
  const auto precise=arg(args,1,false);invoke(value,"updateWorldMatrix")({false,false});const auto geometry=value.get("geometry");
  if(!geometry.isUndefined()){
   const auto position=invoke(geometry,"getAttribute")({"position"});
   if(precise.strictEquals(true)&&!position.isUndefined()&&!value.get("isInstancedMesh").strictEquals(true)&&!value.get("isBatchedMesh").strictEquals(true)){
    const auto length=position.get("count");for(double i=0;B3LT(i,length);++i){
     if(value.get("isMesh").strictEquals(true))invoke(value,"getVertexPosition")({i,state.get("_vector")});else invoke(state.get("_vector"),"fromBufferAttribute")({position,i});
     invoke(state.get("_vector"),"applyMatrix4")({value.get("matrixWorld")});invoke(self,"expandByPoint")({state.get("_vector")});
    }
   }else{
    if(!value.get("boundingBox").isUndefined()){if(value.get("boundingBox").isNull())invoke(value,"computeBoundingBox")();invoke(state.get("_box"),"copy")({value.get("boundingBox")});}
    else{if(geometry.get("boundingBox").isNull())invoke(geometry,"computeBoundingBox")();invoke(state.get("_box"),"copy")({geometry.get("boundingBox")});}
    invoke(state.get("_box"),"applyMatrix4")({value.get("matrixWorld")});invoke(self,"union")({state.get("_box")});
   }
  }
  const auto children=value.get("children"),length=children.get("length");for(double i=0;B3LT(i,length);++i)invoke(self,"expandByObject")({children.get(JSValue(i)),precise});return self;
 }
 if(name=="containsPoint"){
  for(const auto* k:{"x","y","z"})if(!B3GE(value.get(k),self.get("min").get(k))||!B3LE(value.get(k),self.get("max").get(k)))return false;return true;
 }
 if(name=="containsBox"){
  for(const auto* k:{"x","y","z"})if(!B3LE(self.get("min").get(k),value.get("min").get(k))||!B3LE(value.get("max").get(k),self.get("max").get(k)))return false;return true;
 }
 if(name=="intersectsBox"){
  for(const auto* k:{"x","y","z"})if(!B3GE(value.get("max").get(k),self.get("min").get(k))||!B3LE(value.get("min").get(k),self.get("max").get(k)))return false;return true;
 }
 if(name=="getParameter"){
  const auto target=arg(args,1);const auto set=invoke(target,"set");Args values;
  for(const auto* k:{"x","y","z"}){const auto numerator=B3SUB(value.get(k),self.get("min").get(k));const auto denominator=B3SUB(self.get("max").get(k),self.get("min").get(k));values.push_back(numerator/denominator);}return set(values);
 }
 if(name=="intersectsSphere"){
  invoke(self,"clampPoint")({value.get("center"),state.get("_vector")});const auto distance=invoke(state.get("_vector"),"distanceToSquared")({value.get("center")});return B3LE(distance,B3MUL(value.get("radius"),value.get("radius")));
 }
 if(name=="intersectsPlane"){
  double minimum=0,maximum=0;bool first=true;
  for(const auto* k:{"x","y","z"}){
   const bool positive=B3GT(value.get("normal").get(k),0);
   const auto lower=B3MUL(value.get("normal").get(k),self.get(positive?"min":"max").get(k));minimum=first?lower:minimum+lower;
   const auto upper=B3MUL(value.get("normal").get(k),self.get(positive?"max":"min").get(k));maximum=first?upper:maximum+upper;first=false;
  }
  return B3LE(minimum,-value.get("constant").toNumber())&&B3GE(maximum,-value.get("constant").toNumber());
 }
 if(name=="intersectsTriangle"){
  if(invoke(self,"isEmpty")().toBoolean())return false;
  const auto center=state.get("_center"),extents=state.get("_extents"),v0=state.get("_v0"),v1=state.get("_v1"),v2=state.get("_v2"),f0=state.get("_f0"),f1=state.get("_f1"),f2=state.get("_f2"),normal=state.get("_triangleNormal");
  invoke(self,"getCenter")({center});invoke(extents,"subVectors")({self.get("max"),center});
  invoke(v0,"subVectors")({value.get("a"),center});invoke(v1,"subVectors")({value.get("b"),center});invoke(v2,"subVectors")({value.get("c"),center});
  invoke(f0,"subVectors")({v1,v0});invoke(f1,"subVectors")({v2,v1});invoke(f2,"subVectors")({v0,v2});
  auto axes=literal({0,-f0.get("z").toNumber(),f0.get("y"),0,-f1.get("z").toNumber(),f1.get("y"),0,-f2.get("z").toNumber(),f2.get("y"),f0.get("z"),0,-f0.get("x").toNumber(),f1.get("z"),0,-f1.get("x").toNumber(),f2.get("z"),0,-f2.get("x").toNumber(),-f0.get("y").toNumber(),f0.get("x"),0,-f1.get("y").toNumber(),f1.get("x"),0,-f2.get("y").toNumber(),f2.get("x"),0});
  const auto test=state.get("satForAxes");if(!test.call({}, {axes,v0,v1,v2,extents}).toBoolean())return false;
  axes=literal({1,0,0,0,1,0,0,0,1});if(!test.call({}, {axes,v0,v1,v2,extents}).toBoolean())return false;
  invoke(normal,"crossVectors")({f0,f1});axes=literal({normal.get("x"),normal.get("y"),normal.get("z")});return test.call({}, {axes,v0,v1,v2,extents});
 }
 if(name=="clampPoint")return invoke(invoke(arg(args,1),"copy")({value}),"clamp")({self.get("min"),self.get("max")});
 if(name=="distanceToPoint")return invoke(invoke(self,"clampPoint")({value,state.get("_vector")}),"distanceTo")({value});
 if(name=="getBoundingSphere"){
  if(invoke(self,"isEmpty")().toBoolean())invoke(value,"makeEmpty")();
  else{invoke(self,"getCenter")({value.get("center")});const auto size=invoke(self,"getSize")({state.get("_vector")});auto target=value;target.set("radius",B3MUL(invoke(size,"length")(),0.5));}return value;
 }
 if(name=="intersect"||name=="union"){
  invoke(self.get("min"),name=="intersect"?"max":"min")({value.get("min")});invoke(self.get("max"),name=="intersect"?"min":"max")({value.get("max")});if(name=="intersect"&&invoke(self,"isEmpty")().toBoolean())invoke(self,"makeEmpty")();return self;
 }
 if(name=="applyMatrix4"){
  if(invoke(self,"isEmpty")().toBoolean())return self;const auto points=state.get("_points");
  for(unsigned i=0;i<8;++i)invoke(invoke(points.get(JSValue(i)),"set")({self.get(i&4?"max":"min").get("x"),self.get(i&2?"max":"min").get("y"),self.get(i&1?"max":"min").get("z")}),"applyMatrix4")({value});
  invoke(self,"setFromPoints")({points});return self;
 }
 if(name=="equals"){const auto first=invoke(value.get("min"),"equals")({self.get("min")});if(!first.toBoolean())return first;return invoke(value.get("max"),"equals")({self.get("max")});}
 if(name=="toJSON"){auto json=JSValue::object();own(json,"min",invoke(self.get("min"),"toArray")());own(json,"max",invoke(self.get("max"),"toArray")());return json;}
 throw std::logic_error("Unknown managed Box3 method: "+name);
}
#undef B3SUB
#undef B3MUL
#undef B3DIV
#undef B3LT
#undef B3GT
#undef B3LE
#undef B3GE
}
JSValue nativeBox3Module(){
 using namespace Box3Managed;static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
 static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Box3 module");
 const auto vector=nativeVector3Constructor();auto state=JSValue::object(nullptr);own(state,"Vector3",vector);
 auto ctor=JSValue::constructor(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue{
  const auto lower=arg(a,0).isUndefined()?c[0].construct({INFINITY,INFINITY,INFINITY}):arg(a,0);
  const auto upper=arg(a,1).isUndefined()?c[0].construct({-INFINITY,-INFINITY,-INFINITY}):arg(a,1);
  auto self=receiver;self.set("isBox3",true);self.set("min",lower);self.set("max",upper);return {};
 },{vector},"Box3",0);ctor.setFunctionSourceText(Box3Source::classSource);own(state,"Box3",ctor);
 for(const auto& spec:Box3Source::methods){if(std::string_view(spec.name)=="constructor")continue;auto function=JSValue::function(method,{spec.name,state},spec.name,spec.arity);function.setFunctionSourceText(spec.source);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;ctor.get("prototype").defineProperty(spec.name,d);}
 auto points=JSValue::array();for(unsigned i=0;i<8;++i){JSPropertyDescriptor d;d.value=vector.construct();d.writable=d.enumerable=d.configurable=true;points.defineProperty(std::to_string(i),d);}own(state,"_points",points);
 own(state,"_vector",vector.construct());own(state,"_box",ctor.construct());
 for(const auto* key:{"_v0","_v1","_v2","_f0","_f1","_f2","_center","_extents","_triangleNormal","_testAxis"})own(state,key,vector.construct());
 auto satFunction=JSValue::constructor(sat,{state},"satForAxes",5,JSValue::ConstructorKind::Callable);satFunction.setFunctionSourceText(Box3Source::satSource);own(state,"satForAxes",satFunction);
 auto bindings=JSValue::object(nullptr);own(bindings,"Box3",ctor);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);
 auto module=JSValue::object();own(module,"Box3",ctor);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeBox3Binding(const std::string& name){const auto keeper=nativeBox3Module();return Box3Managed::lexical().lock()->get(name);}
JSValue nativeBox3Constructor(){return nativeBox3Binding("Box3");}
