#include "objects/SkinnedMesh.h"
#include "core/Raycaster.h"
#include <iostream>

namespace {
// three.js shares these scratch values across calls, including callback reentry.
thread_local Vector3 skinVertex;
thread_local Sphere skinSphere;
thread_local Matrix4 skinInverse;
thread_local Ray skinRay;
std::shared_ptr<VertexAttribute> skinAttribute(const std::shared_ptr<BufferGeometry>& geometry,
                                     const char* name) {
    if (!geometry) throw JSTypeError("Cannot read properties of null (reading 'attributes')");
    const auto attribute = geometry->getAttribute(name);
    if (!attribute) throw JSTypeError("Cannot read properties of undefined (reading 'getX')");
    return attribute;
}
void skinVectors(const std::shared_ptr<BufferGeometry>& geometry, double index, Vector4& indices, Vector4& weights) {
    indices.fromBufferAttribute(*skinAttribute(geometry, "skinIndex"), index);
    weights.fromBufferAttribute(*skinAttribute(geometry, "skinWeight"), index);
}
Matrix4 boneMatrix(const std::shared_ptr<Skeleton>& skeleton, double index) {
    if (!skeleton) throw JSTypeError("Cannot read properties of undefined (reading 'bones')");
    if (!std::isfinite(index) || index < 0 || std::floor(index) != index || index >= skeleton->bones.size())
        throw JSTypeError("Cannot read properties of undefined (reading 'matrixWorld')");
    const auto i = std::size_t(index);
    const auto bone = skeleton->bones.storage()->holes.contains(i) ? nullptr : skeleton->bones[i];
    if (!bone) throw JSTypeError("Cannot read properties of null (reading 'matrixWorld')");
    if (i >= skeleton->boneInverses.size() || skeleton->boneInverses.storage()->holes.contains(i) || !skeleton->boneInverses[i])
        throw JSTypeError("Cannot read properties of undefined (reading 'elements')");
    Matrix4 matrix;
    matrix.multiplyMatrices(bone->matrixWorld, *skeleton->boneInverses[i]);
    return matrix;
}
}
SkinnedMesh::SkinnedMesh(std::shared_ptr<BufferGeometry> geometry, std::shared_ptr<Material> material)
    : Mesh(std::move(geometry), std::move(material)) { isSkinnedMesh = true; type = "SkinnedMesh"; }
SkinnedMesh::SkinnedMesh(std::shared_ptr<BufferGeometry> geometry,
                         std::vector<std::shared_ptr<Material>> materials)
    : Mesh(std::move(geometry), std::move(materials)) { isSkinnedMesh = true; type = "SkinnedMesh"; }
void SkinnedMesh::computeBoundingBox() {
    const auto currentGeometry = geometry;
    if (!boundingBox) boundingBox = std::make_shared<Box3>();
    boundingBox->makeEmpty();
    const auto position = skinAttribute(currentGeometry, "position");
    for (std::size_t i = 0; double(i) < position->getCount(); ++i) {
        getVertexPosition(i, skinVertex);
        boundingBox->expandByPoint(skinVertex);
    }
}
void SkinnedMesh::computeBoundingSphere() {
    const auto currentGeometry = geometry;
    if (!boundingSphere) boundingSphere = std::make_shared<Sphere>();
    boundingSphere->makeEmpty();
    const auto position = skinAttribute(currentGeometry, "position");
    for (std::size_t i = 0; double(i) < position->getCount(); ++i) {
        getVertexPosition(i, skinVertex);
        boundingSphere->expandByPoint(skinVertex);
    }
}
Object3D& SkinnedMesh::copy(const Object3D& source, bool recursive) {
    const auto& other = dynamic_cast<const SkinnedMesh&>(source);
    Mesh::copy(source, recursive);
    bindMode = other.bindMode;
    bindMatrix.copy(other.bindMatrix);
    bindMatrixInverse.copy(other.bindMatrixInverse);
    skeleton = other.skeleton;
    boundingBox = other.boundingBox ? std::make_shared<Box3>(other.boundingBox->clone()) : nullptr;
    boundingSphere = other.boundingSphere ? std::make_shared<Sphere>(other.boundingSphere->clone()) : nullptr;
    return *this;
}
std::shared_ptr<Object3D> SkinnedMesh::clone(bool recursive) const {
    auto result = std::make_shared<SkinnedMesh>(); result->copy(*this, recursive); return result;
}
bool SkinnedMesh::raycast(const Raycaster& raycaster, std::vector<Intersection>& intersects) {
    if (!material && !(materialIsArray || !materials.empty())) return true;
    if (!boundingSphere) computeBoundingSphere();
    skinSphere.copy(*boundingSphere).applyMatrix4(matrixWorld);
    if (!raycaster.ray.intersectsSphere(skinSphere)) return true;
    skinInverse.copy(matrixWorld).invert();
    skinRay.copy(raycaster.ray).applyMatrix4(skinInverse);
    if (boundingBox && !skinRay.intersectsBox(*boundingBox)) return true;
    _computeIntersections(raycaster, intersects, skinRay);
    return true;
}
Vector3& SkinnedMesh::getVertexPosition(std::size_t index, Vector3& target) const {
    Mesh::getVertexPosition(index, target);
    applyBoneTransform(double(index), target);
    return target;
}
void SkinnedMesh::bind(std::shared_ptr<Skeleton> armature, const Matrix4* matrix) {
    skeleton = std::move(armature);
    if (!matrix) {
        updateMatrixWorld(true);
        if (!skeleton) throw JSTypeError("Cannot read properties of null (reading 'calculateInverses')");
        skeleton->calculateInverses();
        matrix = &matrixWorld;
    }
    bindMatrix.copy(*matrix);
    bindMatrixInverse.copy(*matrix).invert();
}
void SkinnedMesh::pose() {
    if (!skeleton) throw JSTypeError("Cannot read properties of undefined (reading 'pose')");
    skeleton->pose();
}
void SkinnedMesh::normalizeSkinWeights() {
    if (!geometry) throw JSTypeError("Cannot read properties of null (reading 'attributes')");
    const auto weight = geometry->getAttribute("skinWeight");
    if (!weight) throw JSTypeError("Cannot read properties of undefined (reading 'count')");
    Vector4 vector;
    const double length = weight->getCount();
    for (std::size_t i = 0; double(i) < length; ++i) {
        vector.fromBufferAttribute(*weight, double(i));
        const double scale = 1 / vector.manhattanLength();
        if (scale != std::numeric_limits<double>::infinity()) vector.multiplyScalar(scale);
        else vector.set(1, 0, 0, 0);
        weight->setXYZW(double(i), vector.x, vector.y, vector.z, vector.w);
    }
}
void SkinnedMesh::updateMatrixWorld(bool force) {
    Mesh::updateMatrixWorld(force);
    if (bindMode == AttachedBindMode) bindMatrixInverse.copy(matrixWorld).invert();
    else if (bindMode == DetachedBindMode) bindMatrixInverse.copy(bindMatrix).invert();
    else std::cerr << "THREE.SkinnedMesh: Unrecognized bindMode: " << bindMode << '\n';
}
Vector3& SkinnedMesh::applyBoneTransform(double index, Vector3& target) const {
    const auto armature = skeleton;
    const auto currentGeometry = geometry;
    Vector4 indices, weights; skinVectors(currentGeometry, index, indices, weights);
    Vector4 base(target.x, target.y, target.z, 1);
    target.set(0, 0, 0);
    base.applyMatrix4(bindMatrix);
    for (std::size_t i = 0; i < 4; ++i) {
        const double weight = weights.getComponent(i);
        if (weight == 0) continue;
        Vector4 vector; vector.copy(base).applyMatrix4(boneMatrix(armature, indices.getComponent(i)));
        target.x += vector.x * weight; target.y += vector.y * weight; target.z += vector.z * weight;
    }
    return target.applyMatrix4(bindMatrixInverse);
}
Vector4& SkinnedMesh::applyBoneTransform(double index, Vector4& target) const {
    const auto armature = skeleton;
    const auto currentGeometry = geometry;
    Vector4 indices, weights; skinVectors(currentGeometry, index, indices, weights);
    Vector4 base; base.copy(target);
    target.set(0, 0, 0, 0);
    base.applyMatrix4(bindMatrix);
    for (std::size_t i = 0; i < 4; ++i) {
        const double weight = weights.getComponent(i);
        if (weight == 0) continue;
        Vector4 vector; vector.copy(base).applyMatrix4(boneMatrix(armature, indices.getComponent(i)));
        target.addScaledVector(vector, weight);
    }
    target.w = base.w;
    return target.applyMatrix4(bindMatrixInverse);
}
void SkinnedMesh::serializeBeforeMaterial(Object3DJSONContext& meta, JsonValue& object) const {
    object["bindMode"] = bindMode;
    JsonValue::Array matrix;
    for (const auto value : bindMatrix.elements()) matrix.emplace_back(value);
    object["bindMatrix"] = std::move(matrix);
    if (skeleton) {
        const auto value = skeleton;
        meta.cacheResource("skeletons", value->uuid, [&] { return value->toJSON(); });
        object["skeleton"] = value->uuid;
    }
}

// Complete native managed source class; original typed renderer adapter remains above.
#include "core/JSGlobal.h"
#include "core/JSModuleObject.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSError.h"
#include "core/NativeObject.h"
#include "core/InstancedBufferAttribute.h"
#include "core/BufferGeometry.h"
#include "math/JSNumberMath.h"
#include "math/Matrix4.h"
#include "math/Box3.h"
#include "math/Sphere.h"
#include "math/Frustum.h"
#include "math/FrustumArray.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Color.h"
#include "math/ColorManagement.h"
#include "math/Ray.h"
#include "objects/Mesh.h"
#include "textures/DataTexture.h"
#include "constants.h"
#include "utils.h"
#include <string_view>
#include <unordered_set>
#include "objects/SkinnedMeshManagedSource.inc"
namespace SkinnedMeshManaged {

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

class ForInCursor{
 JSValue receiver;std::vector<std::string> keys;std::size_t index=0;
 public:explicit ForInCursor(JSValue source):receiver(source.isNull()||source.isUndefined()||source.isSymbol()?JSValue(nullptr):source.toObject()){
 std::unordered_set<std::string> visited;auto level=receiver;
 while(!level.isNull()){const bool proxy=level.isProxy();for(const auto&key:level.ownPropertyNames()){if(visited.contains(key))continue;if(proxy){visited.insert(key);keys.push_back(key);}else{const auto d=level.getOwnPropertyDescriptor(key);if(!d)continue;visited.insert(key);if(d->enumerable.value_or(false))keys.push_back(key);}}level=level.getPrototype();}}
 std::optional<JSValue> next(){while(index<keys.size()){const auto key=keys[index++];auto owner=receiver;while(!owner.isNull()){const auto d=owner.getOwnPropertyDescriptor(key);if(d){if(!owner.isProxy()||d->enumerable.value_or(false))return JSValue(key);break;}if(owner.isHostObject("three.AnimationArray")&&(key=="-0"||JSValue(JSValue(key).toNumber()).toString()==key))break;owner=owner.getPrototype();}}return std::nullopt;}
};

JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
#include "objects/SkinnedMeshManagedStatements.inc"
}
JSValue nativeSkinnedMeshModule(){using namespace SkinnedMeshManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;static thread_local bool initializing=false;JSModuleInitializationScope initialization(initializing,"SkinnedMesh module");auto module=JSValue::object(nullptr);
own(module,"Mesh",nativeMeshConstructor());
own(module,"Box3",nativeBox3Constructor());
own(module,"Matrix4",nativeMatrix4Constructor());
own(module,"Sphere",nativeSphereConstructor());
own(module,"Vector3",nativeVector3Constructor());
own(module,"Vector4",nativeVector4Constructor());
own(module,"Ray",nativeRayConstructor());
own(module,"AttachedBindMode",nativeConstantsBinding("AttachedBindMode"));
own(module,"DetachedBindMode",nativeConstantsBinding("DetachedBindMode"));
own(module,"warn",ThreeUtils::nativeWarnFunction());
own(module,"_baseVector",([&]()->JSValue{const auto nativeStatementTemp615=module.get("Vector4");Args nativeStatementTemp616;return nativeStatementTemp615.construct(nativeStatementTemp616);}()));
own(module,"_skinIndex",([&]()->JSValue{const auto nativeStatementTemp617=module.get("Vector4");Args nativeStatementTemp618;return nativeStatementTemp617.construct(nativeStatementTemp618);}()));
own(module,"_skinWeight",([&]()->JSValue{const auto nativeStatementTemp619=module.get("Vector4");Args nativeStatementTemp620;return nativeStatementTemp619.construct(nativeStatementTemp620);}()));
own(module,"_vector4",([&]()->JSValue{const auto nativeStatementTemp621=module.get("Vector4");Args nativeStatementTemp622;return nativeStatementTemp621.construct(nativeStatementTemp622);}()));
own(module,"_matrix4",([&]()->JSValue{const auto nativeStatementTemp623=module.get("Matrix4");Args nativeStatementTemp624;return nativeStatementTemp623.construct(nativeStatementTemp624);}()));
own(module,"_vertex",([&]()->JSValue{const auto nativeStatementTemp625=module.get("Vector3");Args nativeStatementTemp626;return nativeStatementTemp625.construct(nativeStatementTemp626);}()));
own(module,"_sphere",([&]()->JSValue{const auto nativeStatementTemp627=module.get("Sphere");Args nativeStatementTemp628;return nativeStatementTemp627.construct(nativeStatementTemp628);}()));
own(module,"_inverseMatrix",([&]()->JSValue{const auto nativeStatementTemp629=module.get("Matrix4");Args nativeStatementTemp630;return nativeStatementTemp629.construct(nativeStatementTemp630);}()));
own(module,"_ray",([&]()->JSValue{const auto nativeStatementTemp631=module.get("Ray");Args nativeStatementTemp632;return nativeStatementTemp631.construct(nativeStatementTemp632);}()));
{auto ctor=JSValue::builtinConstructor(SkinnedMeshMethod0,{module,JSValue{}},"SkinnedMesh",2);ctor.setFunctionSourceText(SkinnedMeshManagedSource::SkinnedMeshClass);auto prototype=ctor.get("prototype");own(module,"SkinnedMesh",ctor);
ctor.setPrototype(module.get("Mesh"));prototype.setPrototype(module.get("Mesh").get("prototype"));
{auto fn=JSValue::function(SkinnedMeshMethod1,{module,prototype},"computeBoundingBox",0);fn.setFunctionSourceText(SkinnedMeshManagedSource::SkinnedMeshMethod1);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("computeBoundingBox",d);}
{auto fn=JSValue::function(SkinnedMeshMethod2,{module,prototype},"computeBoundingSphere",0);fn.setFunctionSourceText(SkinnedMeshManagedSource::SkinnedMeshMethod2);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("computeBoundingSphere",d);}
{auto fn=JSValue::function(SkinnedMeshMethod3,{module,prototype},"copy",2);fn.setFunctionSourceText(SkinnedMeshManagedSource::SkinnedMeshMethod3);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("copy",d);}
{auto fn=JSValue::function(SkinnedMeshMethod4,{module,prototype},"raycast",2);fn.setFunctionSourceText(SkinnedMeshManagedSource::SkinnedMeshMethod4);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("raycast",d);}
{auto fn=JSValue::function(SkinnedMeshMethod5,{module,prototype},"getVertexPosition",2);fn.setFunctionSourceText(SkinnedMeshManagedSource::SkinnedMeshMethod5);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getVertexPosition",d);}
{auto fn=JSValue::function(SkinnedMeshMethod6,{module,prototype},"bind",2);fn.setFunctionSourceText(SkinnedMeshManagedSource::SkinnedMeshMethod6);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("bind",d);}
{auto fn=JSValue::function(SkinnedMeshMethod7,{module,prototype},"pose",0);fn.setFunctionSourceText(SkinnedMeshManagedSource::SkinnedMeshMethod7);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("pose",d);}
{auto fn=JSValue::function(SkinnedMeshMethod8,{module,prototype},"normalizeSkinWeights",0);fn.setFunctionSourceText(SkinnedMeshManagedSource::SkinnedMeshMethod8);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("normalizeSkinWeights",d);}
{auto fn=JSValue::function(SkinnedMeshMethod9,{module,prototype},"updateMatrixWorld",1);fn.setFunctionSourceText(SkinnedMeshManagedSource::SkinnedMeshMethod9);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("updateMatrixWorld",d);}
{auto fn=JSValue::function(SkinnedMeshMethod10,{module,prototype},"applyBoneTransform",2);fn.setFunctionSourceText(SkinnedMeshManagedSource::SkinnedMeshMethod10);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("applyBoneTransform",d);}
}
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();own(facade,"SkinnedMesh",module.get("SkinnedMesh"));facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeSkinnedMeshBinding(const std::string& key){const auto keeper=nativeSkinnedMeshModule();return SkinnedMeshManaged::lexical().lock()->get(key);}
JSValue nativeSkinnedMeshConstructor(){return nativeSkinnedMeshBinding("SkinnedMesh");}
