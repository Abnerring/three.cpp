#include "objects/Skeleton.h"
#include "math/MathUtils.h"
#include <cmath>
#include <iostream>

namespace {
std::shared_ptr<Bone> boneAt(const Skeleton::Bones& values, std::size_t i) {
    const auto storage = values.storage();
    return i < storage->values.size() && !storage->holes.contains(i) ? storage->values[i] : nullptr;
}
const Matrix4& inverseAt(const Skeleton::Inverses& values, std::size_t i) {
    if (i >= values.size() || values.storage()->holes.contains(i) || !values[i])
        throw JSTypeError("Cannot read properties of undefined (reading 'elements')");
    return *values[i];
}
JsonValue matrixJSON(const Matrix4& matrix) {
    JsonValue::Array result;
    for (const auto value : matrix.elements()) result.emplace_back(value);
    return result;
}
}
Skeleton::Skeleton(Bones inputBones, Inverses inverses)
    : uuid(MathUtils::generateUUID()), bones(inputBones.slice()), boneInverses(std::move(inverses)) {
    init();
}
void Skeleton::init() {
    boneMatrices = Float32Array(double(bones.size()) * 16);
    if (boneInverses.empty()) calculateInverses();
    else if (bones.size() != boneInverses.size()) {
        std::cerr << "THREE.Skeleton: Number of inverse bone matrices does not match amount of bones.\n";
        boneInverses = Inverses();
        for (std::size_t i = 0, length = bones.size(); i < length; ++i)
            boneInverses.push_back(std::make_shared<Matrix4>());
    }
}
void Skeleton::calculateInverses() {
    boneInverses.clear();
    for (std::size_t i = 0, length = bones.size(); i < length; ++i) {
        auto inverse = std::make_shared<Matrix4>();
        if (const auto bone = boneAt(bones, i)) inverse->copy(bone->matrixWorld).invert();
        boneInverses.push_back(std::move(inverse));
    }
}
void Skeleton::pose() {
    for (std::size_t i = 0, length = bones.size(); i < length; ++i)
        if (const auto bone = boneAt(bones, i)) bone->matrixWorld.copy(inverseAt(boneInverses, i)).invert();
    for (std::size_t i = 0, length = bones.size(); i < length; ++i) {
        const auto bone = boneAt(bones, i);
        if (!bone) continue;
        if (bone->parent && bone->parent->isBone)
            bone->matrix.copy(bone->parent->matrixWorld).invert().multiply(bone->matrixWorld);
        else bone->matrix.copy(bone->matrixWorld);
        bone->matrix.decompose(bone->position, bone->quaternion, bone->scale);
    }
}
void Skeleton::update() {
    const auto currentBones = bones;
    const auto inverses = boneInverses;
    auto matrices = boneMatrices;
    const auto texture = boneTexture;
    const Matrix4 identity;
    Matrix4 offset;
    for (std::size_t i = 0, length = currentBones.size(); i < length; ++i) {
        const auto bone = boneAt(currentBones, i);
        offset.multiplyMatrices(bone ? bone->matrixWorld : identity, inverseAt(inverses, i));
        const auto elements = offset.elements();
        for (std::size_t j = 0; j < 16; ++j) matrices.setIndex(double(i * 16 + j), elements[j]);
    }
    if (texture) texture->needsUpdate = true;
}
std::shared_ptr<Skeleton> Skeleton::clone() const { return std::make_shared<Skeleton>(bones, boneInverses); }
Skeleton& Skeleton::computeBoneTexture() {
    const double size = std::max(std::ceil(std::sqrt(double(bones.size()) * 4) / 4) * 4, 4.0);
    Float32Array matrices(size * size * 4);
    matrices.set(boneMatrices);
    auto texture = std::make_shared<DataTexture>(matrices, size, size, RGBAFormat, FloatType);
    texture->needsUpdate = true;
    boneMatrices = matrices;
    boneTexture = std::move(texture);
    return *this;
}
std::shared_ptr<Bone> Skeleton::getBoneByName(const std::string& name) const {
    for (std::size_t i = 0, length = bones.size(); i < length; ++i) {
        const auto bone = boneAt(bones, i);
        if (!bone) throw JSTypeError("Cannot read properties of null (reading 'name')");
        if (bone->name == name) return bone;
    }
    return nullptr;
}
void Skeleton::dispose() {
    if (boneTexture) {
        const auto texture = boneTexture;
        texture->dispose();
        boneTexture.reset();
    }
}
Skeleton& Skeleton::fromJSON(const JsonValue& json,
                            const std::map<std::string, std::shared_ptr<Bone>>& lookup) {
    uuid = json.at("uuid").get<std::string>();
    const auto& ids = json.at("bones").get<JsonValue::Array>();
    const auto& inverses = json.at("boneInverses").get<JsonValue::Array>();
    for (std::size_t i = 0, length = ids.size(); i < length; ++i) {
        const auto id = ids[i].get<std::string>();
        const auto found = lookup.find(id);
        std::shared_ptr<Bone> bone;
        if (found == lookup.end()) {
            std::cerr << "THREE.Skeleton: No bone found with UUID: " << id << '\n';
            bone = std::make_shared<Bone>();
        } else bone = found->second;
        bones.push_back(std::move(bone));
        if (i >= inverses.size()) throw JSTypeError("Cannot read properties of undefined (reading '0')");
        const auto& values = inverses[i].get<JsonValue::Array>();
        auto inverse = std::make_shared<Matrix4>();
        for (std::size_t j = 0; j < 16; ++j)
            inverse->elements()[j] = j < values.size() ? values[j].get<double>() : std::numeric_limits<double>::quiet_NaN();
        boneInverses.push_back(std::move(inverse));
    }
    init();
    return *this;
}
JsonValue Skeleton::toJSON() const {
    JsonValue result = JsonValue::Object{
        {"metadata", JsonValue::Object{{"version", 4.7}, {"type", "Skeleton"}, {"generator", "Skeleton.toJSON"}}},
        {"bones", JsonValue::Array{}}, {"boneInverses", JsonValue::Array{}}, {"uuid", uuid}};
    for (std::size_t i = 0, length = bones.size(); i < length; ++i) {
        const auto bone = boneAt(bones, i);
        if (!bone) throw JSTypeError("Cannot read properties of null (reading 'uuid')");
        result["bones"].get<JsonValue::Array>().emplace_back(bone->uuid);
        result["boneInverses"].get<JsonValue::Array>().push_back(matrixJSON(inverseAt(boneInverses, i)));
    }
    return result;
}

// Complete managed source authority; existing typed renderer adapter is preserved above.
#include "core/JSGlobal.h"
#include "core/JSModuleObject.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "math/JSNumberMath.h"
#include "math/MathUtils.h"
#include "math/Matrix4.h"
#include "objects/Bone.h"
#include "textures/DataTexture.h"
#include "constants.h"
#include "utils.h"
#include "objects/SkeletonManagedSource.inc"
#include <string_view>
#include <unordered_set>
namespace SkeletonManaged {

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
#include "objects/SkeletonManagedStatements.inc"
}
JSValue nativeSkeletonModule(){using namespace SkeletonManaged;static thread_local JSWeakValue weak;if(const auto existing=weak.lock();existing&&existing->isObject())return *existing;static thread_local bool initializing=false;JSModuleInitializationScope initialization(initializing,"Skeleton module");auto module=JSValue::object(nullptr);
own(module,"RGBAFormat",nativeConstantsBinding("RGBAFormat"));
own(module,"FloatType",nativeConstantsBinding("FloatType"));
own(module,"Bone",nativeBoneConstructor());
own(module,"Matrix4",nativeMatrix4Constructor());
own(module,"DataTexture",nativeDataTextureConstructor());
own(module,"generateUUID",nativeMathUtilsGenerateUUID());
own(module,"warn",ThreeUtils::nativeWarnFunction());
own(module,"_offsetMatrix",([&]()->JSValue{const auto nativeStatementTemp559=module.get("Matrix4");Args nativeStatementTemp560;return nativeStatementTemp559.construct(nativeStatementTemp560);}()));
own(module,"_identityMatrix",([&]()->JSValue{const auto nativeStatementTemp561=module.get("Matrix4");Args nativeStatementTemp562;return nativeStatementTemp561.construct(nativeStatementTemp562);}()));
auto ctor=JSValue::constructor(method0,{module},"Skeleton",0);ctor.setFunctionSourceText(SkeletonManagedSource::classSource);auto prototype=ctor.get("prototype");own(module,"Skeleton",ctor);own(module,"_home",prototype);
{auto fn=JSValue::function(method1,{module},"init",0);fn.setFunctionSourceText(SkeletonManagedSource::method1);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("init",d);}
{auto fn=JSValue::function(method2,{module},"calculateInverses",0);fn.setFunctionSourceText(SkeletonManagedSource::method2);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("calculateInverses",d);}
{auto fn=JSValue::function(method3,{module},"pose",0);fn.setFunctionSourceText(SkeletonManagedSource::method3);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("pose",d);}
{auto fn=JSValue::function(method4,{module},"update",0);fn.setFunctionSourceText(SkeletonManagedSource::method4);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("update",d);}
{auto fn=JSValue::function(method5,{module},"clone",0);fn.setFunctionSourceText(SkeletonManagedSource::method5);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("clone",d);}
{auto fn=JSValue::function(method6,{module},"computeBoneTexture",0);fn.setFunctionSourceText(SkeletonManagedSource::method6);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("computeBoneTexture",d);}
{auto fn=JSValue::function(method7,{module},"getBoneByName",1);fn.setFunctionSourceText(SkeletonManagedSource::method7);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("getBoneByName",d);}
{auto fn=JSValue::function(method8,{module},"dispose",0);fn.setFunctionSourceText(SkeletonManagedSource::method8);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("dispose",d);}
{auto fn=JSValue::function(method9,{module},"fromJSON",2);fn.setFunctionSourceText(SkeletonManagedSource::method9);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("fromJSON",d);}
{auto fn=JSValue::function(method10,{module},"toJSON",0);fn.setFunctionSourceText(SkeletonManagedSource::method10);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("toJSON",d);}
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();own(facade,"Skeleton",ctor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeSkeletonBinding(const std::string& name){const auto keeper=nativeSkeletonModule();return SkeletonManaged::lexical().lock()->get(name);}
JSValue nativeSkeletonConstructor(){return nativeSkeletonBinding("Skeleton");}
