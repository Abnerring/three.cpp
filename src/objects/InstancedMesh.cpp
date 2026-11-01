#include "objects/InstancedMesh.h"
#include "core/Raycaster.h"

namespace {
thread_local Matrix4 instanceLocalMatrix, instanceWorldMatrix;
thread_local Box3 instanceBox;
thread_local Sphere instanceSphere;
struct RaycastScratch {
    Mesh mesh;
    std::vector<Intersection> intersects;
};
RaycastScratch& raycastScratch() { static thread_local RaycastScratch value; return value; }
double readArray(const BufferAttribute::Array& array, double index) {
    return std::visit([&](const auto& values) { return values.get(index).toNumber(); }, array);
}
void writeArray(BufferAttribute::Array& array, double index, double value) {
    std::visit([&](auto& values) { values.setIndex(index, value); }, array);
}
TextureImage::Ptr morphImage(const std::shared_ptr<DataTexture>& texture) {
    if (!texture) throw JSTypeError("Cannot read properties of null (reading 'source')");
    if (!texture->source) throw JSTypeError("Cannot read properties of null (reading 'data')");
    if (!texture->source->data) throw JSTypeError("Cannot read properties of null (reading 'data')");
    return texture->source->data;
}
double readImage(const TextureImage::Data& data, double index) {
    return std::visit([&](const auto& values) -> double {
        using T = std::decay_t<decltype(values)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) throw JSTypeError("Cannot read properties of null");
        else if constexpr (std::is_same_v<T, JSValue>) return values.get(JSValue(index).toString()).toNumber();
        else return values.get(index).toNumber();
    }, data);
}
void writeImage(TextureImage::Data& data, double index, double value) {
    std::visit([&](auto& values) {
        using T = std::decay_t<decltype(values)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) throw JSTypeError("Cannot set properties of null");
        else if constexpr (std::is_same_v<T, JSValue>) values.set(JSValue(index).toString(), value);
        else values.setIndex(index, value);
    }, data);
}
void setImage(TextureImage::Data& data, const std::vector<double>& values, double offset) {
    std::visit([&](auto& array) {
        using T = std::decay_t<decltype(array)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) throw JSTypeError("Cannot read properties of null (reading 'set')");
        else if constexpr (std::is_same_v<T, JSValue>) {
            auto input = JSValue::array();
            for (std::size_t i = 0; i < values.size(); ++i) input.set(std::to_string(i), values[i]);
            array.get("set").call(array, {input, offset});
        } else array.set(values, offset);
    }, data);
}
}
InstancedMesh::InstancedMesh(std::shared_ptr<BufferGeometry> geometry,
                             std::shared_ptr<Material> material, JSValue count)
    : Mesh(std::move(geometry), std::move(material), false) { initializeInstances(count); }
InstancedMesh::InstancedMesh(std::shared_ptr<BufferGeometry> geometry,
                             std::vector<std::shared_ptr<Material>> materials, JSValue count)
    : Mesh(std::move(geometry), std::move(materials), false) { initializeInstances(count); }
void InstancedMesh::initializeInstances(const JSValue& value) {
    isInstancedMesh = true;
    instanceMatrix = std::make_shared<InstancedBufferAttribute>(Float32Array(value.toNumber() * 16), 16);
    count = value;
    const Matrix4 identity;
    for (double i = 0; i < value.toNumber(); ++i) setMatrixAt(i, identity);
}
void InstancedMesh::computeBoundingBox() {
    const auto source = geometry;
    const auto length = count.asValue();
    if (!boundingBox) boundingBox = std::make_shared<Box3>();
    if (!source) throw JSTypeError("Cannot read properties of null (reading 'boundingBox')");
    if (!source->boundingBox) source->computeBoundingBox();
    boundingBox->makeEmpty();
    for (double i = 0; i < length.toNumber(); ++i) {
        getMatrixAt(i, instanceLocalMatrix);
        instanceBox.copy(*source->boundingBox).applyMatrix4(instanceLocalMatrix);
        boundingBox->unionWith(instanceBox);
    }
}
void InstancedMesh::computeBoundingSphere() {
    const auto source = geometry;
    const auto length = count.asValue();
    if (!boundingSphere) boundingSphere = std::make_shared<Sphere>();
    if (!source) throw JSTypeError("Cannot read properties of null (reading 'boundingSphere')");
    if (!source->boundingSphere) source->computeBoundingSphere();
    boundingSphere->makeEmpty();
    for (double i = 0; i < length.toNumber(); ++i) {
        getMatrixAt(i, instanceLocalMatrix);
        instanceSphere.copy(*source->boundingSphere).applyMatrix4(instanceLocalMatrix);
        boundingSphere->unionWith(instanceSphere);
    }
}
Object3D& InstancedMesh::copy(const Object3D& source, bool recursive) {
    const auto& other = dynamic_cast<const InstancedMesh&>(source);
    Mesh::copy(source, recursive);
    if (!instanceMatrix || !other.instanceMatrix)
        throw JSTypeError("Cannot read properties of null (reading 'copy')");
    instanceMatrix->copy(*other.instanceMatrix);
    morphTexture = other.morphTexture ? std::dynamic_pointer_cast<DataTexture>(other.morphTexture->clone()) : nullptr;
    instanceColor = other.instanceColor ? std::dynamic_pointer_cast<InstancedBufferAttribute>(other.instanceColor->clone()) : nullptr;
    count = other.count;
    boundingBox = other.boundingBox ? std::make_shared<Box3>(other.boundingBox->clone()) : nullptr;
    boundingSphere = other.boundingSphere ? std::make_shared<Sphere>(other.boundingSphere->clone()) : nullptr;
    return *this;
}
std::shared_ptr<Object3D> InstancedMesh::clone(bool recursive) const {
    auto result = std::make_shared<InstancedMesh>(); result->copy(*this, recursive); return result;
}
Color& InstancedMesh::getColorAt(double index, Color& color) const {
    if (!instanceColor) return color.setRGB(1, 1, 1);
    const auto array = instanceColor->array;
    color.r = readArray(array, index * 3);
    color.g = readArray(array, index * 3 + 1);
    color.b = readArray(array, index * 3 + 2);
    return color;
}
Matrix4& InstancedMesh::getMatrixAt(double index, Matrix4& matrix) const {
    if (!instanceMatrix) throw JSTypeError("Cannot read properties of null (reading 'array')");
    const auto array = instanceMatrix->array;
    for (std::size_t i = 0; i < 16; ++i) matrix.elements()[i] = readArray(array, index * 16 + double(i));
    return matrix;
}
void InstancedMesh::getMorphAt(double index, Mesh& object) const {
    const auto influences = object.morphTargetInfluences;
    const auto image = morphImage(morphTexture);
    const auto data = image->data;
    if (!influences) throw JSTypeError("Cannot read properties of undefined (reading 'length')");
    const double length = double(influences->size()) + 1;
    const double dataIndex = index * length + 1;
    const auto storage = influences->storage();
    for (std::size_t i = 0; i < storage->values.size(); ++i) {
        if (storage->flags.contains(i) && !storage->flags.at(i).writable)
            throw JSTypeError("Cannot assign to read only array element");
        storage->values[i] = readImage(data, dataIndex + double(i));
        storage->holes.erase(i);
    }
}
bool InstancedMesh::raycast(const Raycaster& raycaster, std::vector<Intersection>& intersects) {
    const auto length = count.asValue();
    auto& scratch = raycastScratch();
    auto& mesh = scratch.mesh;
    mesh.geometry = geometry; mesh.material = material;
    mesh.materials = materials; mesh.materialIsArray = materialIsArray;
    if (!material && !(materialIsArray || !materials.empty())) return true;
    if (!boundingSphere) computeBoundingSphere();
    instanceSphere.copy(*boundingSphere).applyMatrix4(matrixWorld);
    if (!raycaster.ray.intersectsSphere(instanceSphere)) return true;
    auto& hits = scratch.intersects;
    for (double i = 0; i < length.toNumber(); ++i) {
        getMatrixAt(i, instanceLocalMatrix);
        instanceWorldMatrix.multiplyMatrices(matrixWorld, instanceLocalMatrix);
        mesh.matrixWorld.copy(instanceWorldMatrix);
        mesh.raycast(raycaster, hits);
        for (auto& hit : hits) {
            hit.instanceId = i; hit.object = this; hit.objectOwner = weak_from_this().lock();
            intersects.push_back(std::move(hit));
        }
        hits.clear();
    }
    return true;
}
InstancedMesh& InstancedMesh::setColorAt(double index, const Color& color) {
    if (!instanceColor) {
        if (!instanceMatrix) throw JSTypeError("Cannot read properties of null (reading 'count')");
        Float32Array values(instanceMatrix->count * 3);
        values.assign(values.size(), 1);
        instanceColor = std::make_shared<InstancedBufferAttribute>(values, 3);
    }
    writeArray(instanceColor->array, index * 3, color.r);
    writeArray(instanceColor->array, index * 3 + 1, color.g);
    writeArray(instanceColor->array, index * 3 + 2, color.b);
    return *this;
}
InstancedMesh& InstancedMesh::setMatrixAt(double index, const Matrix4& matrix) {
    if (!instanceMatrix) throw JSTypeError("Cannot read properties of null (reading 'array')");
    for (std::size_t i = 0; i < 16; ++i)
        writeArray(instanceMatrix->array, index * 16 + double(i), matrix.elements()[i]);
    return *this;
}
InstancedMesh& InstancedMesh::setMorphAt(double index, const Mesh& object) {
    const auto influences = object.morphTargetInfluences;
    if (!influences) throw JSTypeError("Cannot read properties of undefined (reading 'length')");
    const double length = double(influences->size()) + 1;
    if (!morphTexture)
        morphTexture = std::make_shared<DataTexture>(Float32Array(length * count), length, count, RedFormat, FloatType);
    const auto image = morphImage(morphTexture);
    auto data = image->data;
    const auto storage = influences->storage();
    std::vector<double> values;
    double sum = 0;
    for (std::size_t i = 0; i < storage->values.size(); ++i) {
        const double value = storage->holes.contains(i) ? std::numeric_limits<double>::quiet_NaN() : storage->values[i];
        sum += value; values.push_back(value);
    }
    if (!geometry) throw JSTypeError("Cannot read properties of null (reading 'morphTargetsRelative')");
    const double base = geometry->morphTargetsRelative ? 1 : 1 - sum;
    const double dataIndex = length * index;
    writeImage(data, dataIndex, base);
    setImage(data, values, dataIndex + 1);
    return *this;
}
void InstancedMesh::dispose() {
    Mesh::dispose();
    if (morphTexture) {
        const auto texture = morphTexture;
        texture->dispose();
        morphTexture.reset();
    }
}
void InstancedMesh::serializeBeforeGeometry(Object3DJSONContext& meta, JsonValue& object) const {
    object["type"] = "InstancedMesh";
    if (const auto value = count.asValue().toJSON()) object["count"] = *value;
    if (!instanceMatrix) throw JSTypeError("Cannot read properties of null (reading 'toJSON')");
    object["instanceMatrix"] = instanceMatrix->toJSON();
    if (instanceColor) object["instanceColor"] = instanceColor->toJSON();
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
#include "objects/InstancedMeshManagedSource.inc"
namespace InstancedMeshManaged {

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
#include "objects/InstancedMeshManagedStatements.inc"
}
JSValue nativeInstancedMeshModule(){using namespace InstancedMeshManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;static thread_local bool initializing=false;JSModuleInitializationScope initialization(initializing,"InstancedMesh module");auto module=JSValue::object(nullptr);
own(module,"InstancedBufferAttribute",nativeInstancedBufferAttributeConstructor());
own(module,"Mesh",nativeMeshConstructor());
own(module,"Box3",nativeBox3Constructor());
own(module,"Matrix4",nativeMatrix4Constructor());
own(module,"Sphere",nativeSphereConstructor());
own(module,"DataTexture",nativeDataTextureConstructor());
own(module,"FloatType",nativeConstantsBinding("FloatType"));
own(module,"RedFormat",nativeConstantsBinding("RedFormat"));
own(module,"_instanceLocalMatrix",([&]()->JSValue{const auto nativeStatementTemp632=module.get("Matrix4");Args nativeStatementTemp633;return nativeStatementTemp632.construct(nativeStatementTemp633);}()));
own(module,"_instanceWorldMatrix",([&]()->JSValue{const auto nativeStatementTemp634=module.get("Matrix4");Args nativeStatementTemp635;return nativeStatementTemp634.construct(nativeStatementTemp635);}()));
own(module,"_instanceIntersects",([&]()->JSValue{auto nativeStatementTemp636=JSValue::array();return nativeStatementTemp636;}()));
own(module,"_box3",([&]()->JSValue{const auto nativeStatementTemp637=module.get("Box3");Args nativeStatementTemp638;return nativeStatementTemp637.construct(nativeStatementTemp638);}()));
own(module,"_identity",([&]()->JSValue{const auto nativeStatementTemp639=module.get("Matrix4");Args nativeStatementTemp640;return nativeStatementTemp639.construct(nativeStatementTemp640);}()));
own(module,"_mesh",([&]()->JSValue{const auto nativeStatementTemp641=module.get("Mesh");Args nativeStatementTemp642;return nativeStatementTemp641.construct(nativeStatementTemp642);}()));
own(module,"_sphere",([&]()->JSValue{const auto nativeStatementTemp643=module.get("Sphere");Args nativeStatementTemp644;return nativeStatementTemp643.construct(nativeStatementTemp644);}()));
{auto ctor=JSValue::builtinConstructor(InstancedMeshMethod0,{module,JSValue{}},"InstancedMesh",3);ctor.setFunctionSourceText(InstancedMeshManagedSource::InstancedMeshClass);auto prototype=ctor.get("prototype");own(module,"InstancedMesh",ctor);
ctor.setPrototype(module.get("Mesh"));prototype.setPrototype(module.get("Mesh").get("prototype"));
{auto fn=JSValue::function(InstancedMeshMethod1,{module,prototype},"computeBoundingBox",0);fn.setFunctionSourceText(InstancedMeshManagedSource::InstancedMeshMethod1);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("computeBoundingBox",d);}
{auto fn=JSValue::function(InstancedMeshMethod2,{module,prototype},"computeBoundingSphere",0);fn.setFunctionSourceText(InstancedMeshManagedSource::InstancedMeshMethod2);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("computeBoundingSphere",d);}
{auto fn=JSValue::function(InstancedMeshMethod3,{module,prototype},"copy",2);fn.setFunctionSourceText(InstancedMeshManagedSource::InstancedMeshMethod3);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("copy",d);}
{auto fn=JSValue::function(InstancedMeshMethod4,{module,prototype},"getColorAt",2);fn.setFunctionSourceText(InstancedMeshManagedSource::InstancedMeshMethod4);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getColorAt",d);}
{auto fn=JSValue::function(InstancedMeshMethod5,{module,prototype},"getMatrixAt",2);fn.setFunctionSourceText(InstancedMeshManagedSource::InstancedMeshMethod5);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getMatrixAt",d);}
{auto fn=JSValue::function(InstancedMeshMethod6,{module,prototype},"getMorphAt",2);fn.setFunctionSourceText(InstancedMeshManagedSource::InstancedMeshMethod6);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getMorphAt",d);}
{auto fn=JSValue::function(InstancedMeshMethod7,{module,prototype},"raycast",2);fn.setFunctionSourceText(InstancedMeshManagedSource::InstancedMeshMethod7);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("raycast",d);}
{auto fn=JSValue::function(InstancedMeshMethod8,{module,prototype},"setColorAt",2);fn.setFunctionSourceText(InstancedMeshManagedSource::InstancedMeshMethod8);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("setColorAt",d);}
{auto fn=JSValue::function(InstancedMeshMethod9,{module,prototype},"setMatrixAt",2);fn.setFunctionSourceText(InstancedMeshManagedSource::InstancedMeshMethod9);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("setMatrixAt",d);}
{auto fn=JSValue::function(InstancedMeshMethod10,{module,prototype},"setMorphAt",2);fn.setFunctionSourceText(InstancedMeshManagedSource::InstancedMeshMethod10);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("setMorphAt",d);}
{auto fn=JSValue::function(InstancedMeshMethod11,{module,prototype},"updateMorphTargets",0);fn.setFunctionSourceText(InstancedMeshManagedSource::InstancedMeshMethod11);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("updateMorphTargets",d);}
{auto fn=JSValue::function(InstancedMeshMethod12,{module,prototype},"dispose",0);fn.setFunctionSourceText(InstancedMeshManagedSource::InstancedMeshMethod12);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("dispose",d);}
}
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();own(facade,"InstancedMesh",module.get("InstancedMesh"));facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeInstancedMeshBinding(const std::string& key){const auto keeper=nativeInstancedMeshModule();return InstancedMeshManaged::lexical().lock()->get(key);}
JSValue nativeInstancedMeshConstructor(){return nativeInstancedMeshBinding("InstancedMesh");}
