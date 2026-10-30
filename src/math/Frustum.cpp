#include "math/Frustum.h"
#include "math/Box3.h"
#include "math/Matrix4.h"
#include "objects/Mesh.h"
#include "objects/SkinnedMesh.h"
#include "objects/InstancedMesh.h"
#include "objects/Line.h"
#include "objects/Points.h"
#include "objects/Sprite.h"
#include <stdexcept>
#include <string>

Frustum::Frustum(const Plane& p0, const Plane& p1, const Plane& p2, const Plane& p3,
                 const Plane& p4, const Plane& p5)
    : planes{p0, p1, p2, p3, p4, p5} {}
Frustum& Frustum::set(const Plane& p0, const Plane& p1, const Plane& p2, const Plane& p3,
                      const Plane& p4, const Plane& p5) {
    planes[0].copy(p0);
    planes[1].copy(p1);
    planes[2].copy(p2);
    planes[3].copy(p3);
    planes[4].copy(p4);
    planes[5].copy(p5);
    return *this;
}
Frustum& Frustum::copy(const Frustum& frustum) {
    for (size_t i = 0; i < 6; ++i)
        planes[i].copy(frustum.planes[i]);
    return *this;
}
Frustum& Frustum::setFromProjectionMatrix(const Matrix4& m, int coordinateSystem,
                                          bool reversedDepth) {
    const auto me = m.elements();
    const auto me0 = me[0], me1 = me[1], me2 = me[2], me3 = me[3];
    const auto me4 = me[4], me5 = me[5], me6 = me[6], me7 = me[7];
    const auto me8 = me[8], me9 = me[9], me10 = me[10], me11 = me[11];
    const auto me12 = me[12], me13 = me[13], me14 = me[14], me15 = me[15];
    planes[0].setComponents(me3 - me0, me7 - me4, me11 - me8, me15 - me12).normalize();
    planes[1].setComponents(me3 + me0, me7 + me4, me11 + me8, me15 + me12).normalize();
    planes[2].setComponents(me3 + me1, me7 + me5, me11 + me9, me15 + me13).normalize();
    planes[3].setComponents(me3 - me1, me7 - me5, me11 - me9, me15 - me13).normalize();
    if (reversedDepth) {
        planes[4].setComponents(me2, me6, me10, me14).normalize();
        planes[5].setComponents(me3 - me2, me7 - me6, me11 - me10, me15 - me14).normalize();
    } else {
        planes[4].setComponents(me3 - me2, me7 - me6, me11 - me10, me15 - me14).normalize();
        if (coordinateSystem == WebGLCoordinateSystem)
            planes[5].setComponents(me3 + me2, me7 + me6, me11 + me10, me15 + me14).normalize();
        else if (coordinateSystem == WebGPUCoordinateSystem)
            planes[5].setComponents(me2, me6, me10, me14).normalize();
        else
            throw std::invalid_argument(
                "THREE.Frustum.setFromProjectionMatrix(): Invalid coordinate system: " +
                std::to_string(coordinateSystem));
    }
    return *this;
}
Sphere& Frustum::sharedSphere() {
    static thread_local Sphere sphere;
    return sphere;
}
bool Frustum::intersectsObject(Object3D& object) const {
    auto* mesh = dynamic_cast<Mesh*>(&object);
    auto& sphere = sharedSphere();
    if (auto* skinned = dynamic_cast<SkinnedMesh*>(mesh)) {
        if (!skinned->boundingSphere) skinned->computeBoundingSphere();
        if (!skinned->boundingSphere) throw JSTypeError("Cannot read properties of null (reading 'center')");
        sphere.copy(*skinned->boundingSphere);
    } else if (auto* instanced = dynamic_cast<InstancedMesh*>(mesh)) {
        if (!instanced->boundingSphere) instanced->computeBoundingSphere();
        if (!instanced->boundingSphere) throw JSTypeError("Cannot read properties of null (reading 'center')");
        sphere.copy(*instanced->boundingSphere);
    } else {
        const auto geometry = mesh ? mesh->geometry
            : dynamic_cast<Line*>(&object) ? dynamic_cast<Line&>(object).geometry
            : dynamic_cast<Points*>(&object) ? dynamic_cast<Points&>(object).geometry
            : dynamic_cast<Sprite*>(&object) ? dynamic_cast<Sprite&>(object).geometry : nullptr;
        if (!geometry) throw JSTypeError("Cannot read properties of null or undefined (reading 'boundingSphere')");
        if (!geometry->boundingSphere) geometry->computeBoundingSphere();
        if (!geometry->boundingSphere) throw JSTypeError("Cannot read properties of null (reading 'center')");
        sphere.copy(*geometry->boundingSphere);
    }
    sphere.applyMatrix4(object.matrixWorld);
    return intersectsSphere(sphere);
}
bool Frustum::intersectsSprite(const Vector2& center, const Matrix4& matrixWorld) const {
    auto& sphere = sharedSphere();
    sphere.center.set(0, 0, 0);
    const auto offset = Vector2(0.5, 0.5).distanceTo(center);
    sphere.radius = 0.7071067811865476 + offset;
    sphere.applyMatrix4(matrixWorld);
    return intersectsSphere(sphere);
}
bool Frustum::intersectsSphere(const Sphere& sphere) const {
    const auto& center = sphere.center;
    const auto negRadius = -sphere.radius;
    for (size_t i = 0; i < 6; ++i) {
        const auto distance = planes[i].distanceToPoint(center);
        if (distance < negRadius)
            return false;
    }
    return true;
}
bool Frustum::intersectsBox(const Box3& box) const {
    Vector3 vector;
    for (size_t i = 0; i < 6; ++i) {
        const auto& plane = planes[i];
        vector.setX(plane.normal.x > 0 ? box.max.x : box.min.x);
        vector.setY(plane.normal.y > 0 ? box.max.y : box.min.y);
        vector.setZ(plane.normal.z > 0 ? box.max.z : box.min.z);
        if (plane.distanceToPoint(vector) < 0)
            return false;
    }
    return true;
}
bool Frustum::containsPoint(const Vector3& point) const {
    for (size_t i = 0; i < 6; ++i)
        if (planes[i].distanceToPoint(point) < 0)
            return false;
    return true;
}
Frustum Frustum::clone() const {
    return Frustum().copy(*this);
}

// Managed source implementation; the typed compatibility methods above remain separate.
#include "core/JSGlobal.h"
#include "core/JSModuleObject.h"
#include "core/JSPromise.h"
#include "core/JSStringEncoding.h"
#include "math/FrustumSource.inc"
#include <string_view>
namespace FrustumManaged {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned index,const JSValue& fallback={}){return index<args.size()&&!args[index].isUndefined()?args[index]:fallback;}
void own(JSValue object,const char* key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
struct Call {JSValue receiver,callee;JSValue operator()(const Args& args={})const{return callee.call(receiver,args);}};
Call invoke(const JSValue& object,const char* key){return {object,object.get(key)};}
JSValue add(const JSValue& left,const JSValue& right){const auto a=left.toPrimitive(),b=right.toPrimitive();if(a.type()==JSValue::Type::String||b.type()==JSValue::Type::String)return a.toString()+b.toString();const auto x=a.toNumber(),y=b.toNumber();return x+y;}
double subtract(const JSValue& left,const JSValue& right){const auto a=left.toNumber(),b=right.toNumber();return a-b;}
bool relation(const JSValue& left,const JSValue& right,bool greater=false){const auto a=left.toPrimitive(JSValue::PreferredType::Number),b=right.toPrimitive(JSValue::PreferredType::Number);if(a.type()==JSValue::Type::String&&b.type()==JSValue::Type::String)return greater?jsUTF16(a.string())>jsUTF16(b.string()):jsUTF16(a.string())<jsUTF16(b.string());const auto x=a.toNumber(),y=b.toNumber();return greater?x>y:x<y;}
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0].string();const auto state=captures[1];const auto value=arg(args,0);
    if(name=="clone"){const auto result=self.get("constructor").construct();return invoke(result,"copy")({self});}
    if(name=="set"||name=="copy"){
        const auto planes=self.get("planes");
        for(unsigned i=0;i<6;++i){const auto copy=invoke(planes.get(JSValue(i)),"copy");const auto source=name=="set"?arg(args,i):value.get("planes").get(JSValue(i));copy({source});}
        return self;
    }
    if(name=="setFromProjectionMatrix"){
        const auto coordinateSystem=arg(args,1,WebGLCoordinateSystem),reversedDepth=arg(args,2,false);
        const auto planes=self.get("planes"),elements=value.get("elements");JSValue me[16];for(unsigned i=0;i<16;++i)me[i]=elements.get(JSValue(i));
        const auto set=[&](unsigned index,int operation,unsigned other){
            const auto setter=invoke(planes.get(JSValue(index)),"setComponents");Args values;
            for(unsigned column=0;column<4;++column){const auto offset=column*4;if(operation<0)values.emplace_back(subtract(me[offset+3],me[offset+other]));else if(operation>0)values.push_back(add(me[offset+3],me[offset+other]));else values.push_back(me[offset+other]);}
            return invoke(setter(values),"normalize")();
        };
        set(0,-1,0);set(1,1,0);set(2,1,1);set(3,-1,1);
        if(reversedDepth.toBoolean()){set(4,0,2);set(5,-1,2);}
        else{
            set(4,-1,2);
            if(coordinateSystem.strictEquals(WebGLCoordinateSystem))set(5,1,2);
            else if(coordinateSystem.strictEquals(WebGPUCoordinateSystem))set(5,0,2);
            else{const auto constructor=nativeGlobalBinding("Error");const auto message=add("THREE.Frustum.setFromProjectionMatrix(): Invalid coordinate system: ",coordinateSystem);throw JSThrownValue(constructor.construct({message}));}
        }
        return self;
    }
    if(name=="intersectsObject"){
        const auto sphere=state.get("_sphere");
        if(!value.get("boundingSphere").isUndefined()){
            if(value.get("boundingSphere").isNull())invoke(value,"computeBoundingSphere")();
            const auto copy=invoke(sphere,"copy");const auto copied=copy({value.get("boundingSphere")});invoke(copied,"applyMatrix4")({value.get("matrixWorld")});
        }else{
            const auto geometry=value.get("geometry");if(geometry.get("boundingSphere").isNull())invoke(geometry,"computeBoundingSphere")();
            const auto copy=invoke(sphere,"copy");const auto copied=copy({geometry.get("boundingSphere")});invoke(copied,"applyMatrix4")({value.get("matrixWorld")});
        }
        return invoke(self,"intersectsSphere")({sphere});
    }
    if(name=="intersectsSprite"){
        const auto sphere=state.get("_sphere");invoke(sphere.get("center"),"set")({0,0,0});
        const auto distance=invoke(state.get("_defaultSpriteCenter"),"distanceTo");const auto offset=distance({value.get("center")});
        auto mutableSphere=sphere;mutableSphere.set("radius",add(0.7071067811865476,offset));invoke(sphere,"applyMatrix4")({value.get("matrixWorld")});return invoke(self,"intersectsSphere")({sphere});
    }
    if(name=="intersectsSphere"){
        const auto planes=self.get("planes"),center=value.get("center");const auto negativeRadius=-value.get("radius").toNumber();
        for(unsigned i=0;i<6;++i){const auto distance=invoke(planes.get(JSValue(i)),"distanceToPoint")({center});if(relation(distance,negativeRadius))return false;}
        return true;
    }
    if(name=="intersectsBox"){
        const auto planes=self.get("planes"),vector=state.get("_vector");
        for(unsigned i=0;i<6;++i){const auto plane=planes.get(JSValue(i));
            for(const auto* key:{"x","y","z"}){const auto normal=plane.get("normal").get(key);const auto corner=value.get(relation(normal,0,true)?"max":"min").get(key);auto target=vector;target.set(key,corner);}
            const auto distance=invoke(plane,"distanceToPoint")({vector});if(relation(distance,0))return false;
        }
        return true;
    }
    if(name=="containsPoint"){
        const auto planes=self.get("planes");for(unsigned i=0;i<6;++i){const auto distance=invoke(planes.get(JSValue(i)),"distanceToPoint")({value});if(relation(distance,0))return false;}return true;
    }
    throw std::logic_error("Unknown managed Frustum method: "+name);
}
}
JSValue nativeFrustumModule(){
    using namespace FrustumManaged;static thread_local JSWeakValue weak;if(const auto module=weak.lock();module&&module->isObject())return *module;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Frustum module");
    const auto vector2=nativeVector2Constructor(),vector3=nativeVector3Constructor(),sphere=nativeSphereConstructor(),plane=nativePlaneConstructor();
    auto state=JSValue::object(nullptr);own(state,"Plane",plane);own(state,"_sphere",sphere.construct());own(state,"_defaultSpriteCenter",vector2.construct({0.5,0.5}));own(state,"_vector",vector3.construct());
    auto constructor=JSValue::constructor(+[](const JSValue& receiver,const Args& args,const Args& captures)->JSValue{
        Args planes;for(unsigned i=0;i<6;++i){const auto input=arg(args,i);planes.push_back(input.isUndefined()?captures[0].construct():input);}
        auto array=JSValue::array();for(unsigned i=0;i<6;++i){JSPropertyDescriptor d;d.value=planes[i];d.writable=d.enumerable=d.configurable=true;array.defineProperty(std::to_string(i),d);}auto self=receiver;self.set("planes",array);return {};
    },{plane},"Frustum",0);constructor.setFunctionSourceText(FrustumSource::classSource);
    for(const auto& spec:FrustumSource::methods){if(std::string_view(spec.name)=="constructor")continue;auto function=JSValue::function(method,{spec.name,state},spec.name,spec.arity);function.setFunctionSourceText(spec.source);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;constructor.get("prototype").defineProperty(spec.name,d);}
    auto bindings=JSValue::object(nullptr);own(bindings,"Frustum",constructor);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);
    auto module=JSValue::object();own(module,"Frustum",constructor);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeFrustumBinding(const std::string& name){const auto keeper=nativeFrustumModule();return FrustumManaged::lexical().lock()->get(name);}
JSValue nativeFrustumConstructor(){return nativeFrustumBinding("Frustum");}
