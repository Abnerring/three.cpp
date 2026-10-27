#include "core/NativeObject.h"
#include "core/NativeRenderingObject.h"
#include "core/NativeObjectExtension.h"
#include "core/InterleavedBufferAttribute.h"
#include "animation/AnimationRuntime.h"
#include "core/Object3D.h"
#include "core/SharedArray.h"
#include "materials/MeshBasicMaterial.h"
#include "math/Vector2.h"
#include "math/Vector4.h"
#include "math/ColorManagement.h"
#include "math/JSNumberMath.h"
#include "objects/Mesh.h"
#include "objects/SkinnedMesh.h"
#include "objects/InstancedMesh.h"
#include "objects/Line.h"
#include "objects/Points.h"
#include "objects/Sprite.h"
#include "objects/LOD.h"
#include "helpers/ArrowHelper.h"
#include "helpers/Box3Helper.h"
#include "helpers/BoxHelper.h"
#include "helpers/CameraHelper.h"
#include "helpers/DirectionalLightHelper.h"
#include "helpers/HemisphereLightHelper.h"
#include "helpers/PlaneHelper.h"
#include "helpers/PointLightHelper.h"
#include "helpers/SkeletonHelper.h"
#include "helpers/SpotLightHelper.h"
#include "cameras/ArrayCamera.h"
#include "cameras/OrthographicCamera.h"
#include "lights/AmbientLight.h"
#include "objects/ClippingGroup.h"
#include "core/InstancedBufferGeometry.h"
#include "lights/Light.h"
#include "lights/HemisphereLight.h"
#include "lights/DirectionalLight.h"
#include "lights/PointLight.h"
#include "lights/SpotLight.h"
#include "lights/RectAreaLight.h"
#include "lights/LightProbe.h"
#include "lights/webgpu/IESSpotLight.h"
#include "core/RenderTarget.h"
#include "scenes/Scene.h"
#include "textures/Texture.h"
#include "textures/DepthTexture.h"
#include "extras/DataUtils.h"
#include "math/MathUtils.h"
#include "utils.h"
#include <cmath>
#include <limits>
#include <optional>
#include <iostream>
#include <unordered_set>

struct NativeObjectAccess {
    static double quaternion(const Quaternion& q, int i) {
        return i == 0 ? q.m_x : i == 1 ? q.m_y : i == 2 ? q.m_z : q.m_w;
    }
    static void quaternion(Quaternion& q, int i, double value) {
        (i == 0 ? q.m_x : i == 1 ? q.m_y : i == 2 ? q.m_z : q.m_w) = value;
    }
    static void notify(Quaternion& q) { notifyPropertyChange(q.m_onChangeCallback); }
    static std::shared_ptr<PropertyChangeRecord> callbackRecord(Quaternion& q) { return propertyChangeRecord(q.m_onChangeCallback); }
    static double euler(const Euler& e, int i) { return i == 0 ? e.m_x : i == 1 ? e.m_y : e.m_z; }
    static void euler(Euler& e, int i, double value) { (i == 0 ? e.m_x : i == 1 ? e.m_y : e.m_z) = value; }
    static void order(Euler& e, const std::string& value) {
        if (value == "XYZ") e.m_order = EulerOrder::XYZ;
        else if (value == "YXZ") e.m_order = EulerOrder::YXZ;
        else if (value == "ZXY") e.m_order = EulerOrder::ZXY;
        else if (value == "ZYX") e.m_order = EulerOrder::ZYX;
        else if (value == "YZX") e.m_order = EulerOrder::YZX;
        else if (value == "XZY") e.m_order = EulerOrder::XZY;
        else throw JSTypeError("Native Euler order must name a supported rotation order");
    }
    static void notify(Euler& e) { notifyPropertyChange(e.m_onChangeCallback); }
    static std::shared_ptr<PropertyChangeRecord> callbackRecord(Euler& e) { return propertyChangeRecord(e.m_onChangeCallback); }
};

namespace {
using Args = std::vector<JSValue>;
constexpr const char* objectTag = "three.Object3D";
constexpr const char* materialTag = "three.Material";
constexpr const char* typedTag = "three.AnimationArray";
template <class T> std::shared_ptr<T> payload(const JSValue& self, const char* tag) {
    return std::static_pointer_cast<T>(self.hostPayload(tag));
}
JSValue argument(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue(); }
void accessor(JSValue object, const char* name, JSValue::NativeFunction read,
              JSValue::NativeFunction write = nullptr, bool enumerable = true) {
    JSPropertyDescriptor d;
    if (read) d.getter = JSValue::function(read, {JSValue(name)});
    if (write) d.setter = JSValue::function(write, {JSValue(name)});
    d.enumerable = enumerable; d.configurable = true;
    object.defineProperty(name, d);
}
template <class Factory> JSValue cachedModuleValue(JSWeakValue& cached, Factory create) {
    if (const auto value = cached.lock(); value && value->isObject()) return *value;
    auto value = create();
    value.retainForHeapLifetime();
    cached = JSWeakValue(value);
    return value;
}
void method(JSValue object, const char* name, JSValue::NativeFunction callback, const char* tag = "",
            const char* sourceText = nullptr) {
    static thread_local std::map<std::string, JSWeakValue> functions;
    const auto key = std::string(tag) + ":" + name;
    const auto function = cachedModuleValue(functions[key], [&] {
        auto value = JSValue::function(callback, {JSValue(name)}, sourceText ? name : "");
        if (sourceText) value.setFunctionSourceText(sourceText);
        return value;
    });
    JSPropertyDescriptor d;
    d.value = function;
    d.writable = true; d.configurable = true; d.enumerable = false;
    object.defineProperty(name, d);
}
bool initialize(JSValue object) {
    if (!object.getHostSlot("initialized").isUndefined()) return false;
    object.setHostSlot("initialized", true); return true;
}
std::optional<std::uint32_t> arrayIndex(const std::string& key) {
    if (key.empty() || (key.size() > 1 && key[0] == '0')) return {};
    std::uint64_t n = 0;
    for (char c : key) {
        if (c < '0' || c > '9') return {};
        n = n * 10 + (c - '0'); if (n >= UINT32_MAX) return {};
    }
    return std::uint32_t(n);
}
bool canonicalNumeric(const std::string& key) {
    if (key == "-0") return true;
    const auto number = JSValue(key).toNumber();
    return JSValue(number).toString() == key;
}
JSPropertyDescriptor dataDescriptor(JSValue value, bool writable = true,
                                   bool enumerable = true, bool configurable = true) {
    JSPropertyDescriptor d; d.value = value; d.writable = writable;
    d.enumerable = enumerable; d.configurable = configurable; return d;
}

template <class T> const char* arrayTag();
template <> const char* arrayTag<double>() { return "three.NumberArray"; }
template <> const char* arrayTag<std::shared_ptr<Object3D>>() { return "three.Object3DArray"; }
template <> const char* arrayTag<std::shared_ptr<Material>>() { return "three.MaterialArray"; }
template <> const char* arrayTag<std::shared_ptr<Bone>>() { return "three.BoneArray"; }
template <> const char* arrayTag<std::shared_ptr<Camera>>() { return "three.CameraArray"; }
template <> const char* arrayTag<std::shared_ptr<Plane>>() { return "three.PlaneArray"; }
JSValue wrapPlane(const std::shared_ptr<Plane>& plane);
JSValue wrapBox(const std::shared_ptr<Box3>& box);
template <class T> JSValue nativeElement(const T& value) {
    if constexpr (std::is_same_v<T, double>) return value;
    else if constexpr (std::is_same_v<T, std::shared_ptr<Object3D>> || std::is_same_v<T,std::shared_ptr<Bone>> || std::is_same_v<T,std::shared_ptr<Camera>>) return nativeObject(value);
    else if constexpr (std::is_same_v<T,std::shared_ptr<Plane>>) return wrapPlane(value);
    else return nativeMaterial(value);
}
template <class T> T nativeElement(const JSValue& value) {
    if constexpr (std::is_same_v<T, double>) {
        if (value.type() != JSValue::Type::Number)
            throw JSTypeError("Native numeric arrays require Number elements");
        return value.number();
    } else {
        if (value.isNull()) return {};
        if constexpr (std::is_same_v<T,std::shared_ptr<Bone>>) {
            const auto object = std::dynamic_pointer_cast<Bone>(payload<Object3D>(value,objectTag));
            if (!object) throw JSTypeError("Skeleton bones must be native Bone objects");
            return object;
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<Object3D>>) return payload<Object3D>(value, objectTag);
        else if constexpr (std::is_same_v<T,std::shared_ptr<Camera>>) {
            const auto camera=std::dynamic_pointer_cast<Camera>(payload<Object3D>(value,objectTag));
            if(!camera)throw JSTypeError("ArrayCamera cameras require native Camera objects");return camera;
        }
        else if constexpr (std::is_same_v<T,std::shared_ptr<Plane>>) return payload<Plane>(value,"three.Plane");
        else return payload<Material>(value, materialTag);
    }
}
template <class T> using ArrayStorage = typename SharedArray<T>::Storage;
void traceNativeObject(const JSValue&, const std::shared_ptr<Object3D>&, std::vector<JSValue>&, std::unordered_set<const Object3D*>&);
void traceNativeMaterial(const JSValue&, const std::shared_ptr<Material>&, std::vector<JSValue>&);
void traceNativeGeometry(const JSValue&, const std::shared_ptr<BufferGeometry>&, std::vector<JSValue>&);
void traceNativeTexture(const JSValue&, const std::shared_ptr<Texture>&, std::vector<JSValue>&);
void traceNativeSkeleton(const JSValue&, const std::shared_ptr<Skeleton>&, std::vector<JSValue>&, std::unordered_set<const Object3D*>&);
std::vector<JSNativeOwnerNode> geometryOwners(const JSValue&);
std::vector<JSNativeOwnerNode> materialOwners(const JSValue&);
std::vector<JSNativeOwnerNode> textureOwners(const JSValue&);
std::vector<JSNativeOwnerNode> attributeOwners(const JSValue&);
std::vector<JSNativeOwnerNode> skeletonOwners(const JSValue&);
std::vector<JSNativeOwnerNode> animationObjectOwners(const JSValue&);
std::vector<JSNativeOwnerNode> shadowOwners(const JSValue&);
std::vector<JSNativeOwnerNode> targetOwners(const JSValue&);
void traceShadow(const JSValue&,const std::shared_ptr<LightShadow>&,std::vector<JSValue>&,std::unordered_set<const Object3D*>&);
JSValue nativeShadow(const std::shared_ptr<LightShadow>&);
JSValue nativeShadowTarget(const std::shared_ptr<RenderTarget>&);
JSValue nativeHarmonics(const std::shared_ptr<SphericalHarmonics3>&);
JSValue nativeSkeleton(const std::shared_ptr<Skeleton>& skeleton);
template <class T> std::vector<JSNativeOwnerNode> arrayOwners(const JSValue&);
void cachedEdge(const JSValue& self, std::vector<JSValue>& result, const void* identity,
                const char* tag, JSValue::Type type = JSValue::Type::Object) {
    auto value = self.findHostObject(identity, tag, type);
    if (!value.isUndefined()) result.push_back(std::move(value));
}
template <class T> std::vector<JSValue> arrayTrace(const JSValue& self) {
    std::vector<JSValue> result;
    auto a = payload<ArrayStorage<T>>(self, arrayTag<T>());
    std::unordered_set<const Object3D*> visited;
    for (std::size_t i = 0; i < a->values.size(); ++i) {
        if (a->holes.contains(i)) continue;
        if constexpr (std::is_same_v<T, std::shared_ptr<Object3D>> || std::is_same_v<T,std::shared_ptr<Bone>> || std::is_same_v<T,std::shared_ptr<Camera>>) traceNativeObject(self, a->values[i], result, visited);
        else if constexpr (std::is_same_v<T, std::shared_ptr<Material>>) traceNativeMaterial(self, a->values[i], result);
        else if constexpr (std::is_same_v<T,std::shared_ptr<Plane>>) if(a->values[i]) {cachedEdge(self,result,a->values[i].get(),"three.Plane");cachedEdge(self,result,&a->values[i]->normal,"three.Vector3");}
    }
    return result;
}
template <class T> JSHostPropertyResult arrayOwn(const JSValue& self, const std::string& key) {
    auto a = payload<ArrayStorage<T>>(self, arrayTag<T>());
    if (key == "length") return {true, dataDescriptor(double(a->values.size()), a->lengthWritable, false, false)};
    const auto index = arrayIndex(key); if (!index) return {};
    if (*index >= a->values.size() || a->holes.contains(*index)) return {true, {}};
    const auto flags = a->flags.contains(*index) ? a->flags.at(*index) : typename SharedArray<T>::Flags{};
    return {true, dataDescriptor(nativeElement(a->values[*index]), flags.writable, flags.enumerable, flags.configurable)};
}
template <class T> std::optional<bool> arrayDefine(const JSValue& self, const std::string& key,
                                                 const JSPropertyDescriptor& d) {
    auto a = payload<ArrayStorage<T>>(self, arrayTag<T>());
    if (key == "length") {
        if (d.getter || d.setter) return false;
        if (d.value) {
            const auto first = d.value->toNumber();
            auto converted = std::isfinite(first) ? std::fmod(std::trunc(first), 4294967296.) : 0.;
            if (converted < 0) converted += 4294967296.;
            const auto number = d.value->toNumber();
            if (converted != number) throw JSRangeError("Invalid array length");
            if (d.configurable.value_or(false) || d.enumerable.value_or(false) ||
                (!a->lengthWritable && d.writable.value_or(false))) return false;
            const auto length = std::size_t(converted), old = a->values.size();
            if (!a->lengthWritable && length != old) return false;
            if (length < old) {
                for (std::size_t i = old; i > length; --i) {
                    const auto index = i - 1;
                    if (!a->holes.contains(index) && a->flags.contains(index) && !a->flags.at(index).configurable) {
                        a->values.resize(i);
                        if (d.writable && !*d.writable) a->lengthWritable = false;
                        return false;
                    }
                    a->holes.erase(index); a->flags.erase(index);
                }
            } else for (std::size_t i = old; i < length; ++i) a->holes.insert(i);
            a->values.resize(length);
        } else if (d.configurable.value_or(false) || d.enumerable.value_or(false) ||
                   (!a->lengthWritable && d.writable.value_or(false))) return false;
        if (d.writable) a->lengthWritable = *d.writable;
        return true;
    }
    const auto index = arrayIndex(key); if (!index) return {};
    if (d.getter || d.setter) throw JSTypeError("Native array accessors require runtime array storage");
    const auto present = *index < a->values.size() && !a->holes.contains(*index);
    auto flags = present ? (a->flags.contains(*index) ? a->flags.at(*index) : typename SharedArray<T>::Flags{})
                         : typename SharedArray<T>::Flags{false, false, false};
    if (!present && !self.isExtensible()) return false;
    if (*index >= a->values.size() && !a->lengthWritable) return false;
    if (present && !flags.configurable) {
        if (d.configurable.value_or(false) || (d.enumerable && *d.enumerable != flags.enumerable)) return false;
        if (!flags.writable && (d.writable.value_or(false) ||
            (d.value && !d.value->sameValue(nativeElement(a->values[*index]))))) return false;
    }
    std::optional<T> value;
    if (d.value) value = nativeElement<T>(*d.value);
    else if (!present) throw JSTypeError("Native array new elements require a native value");
    if (*index >= a->values.size()) {
        const auto old = a->values.size(); a->values.resize(std::size_t(*index) + 1);
        for (std::size_t i = old; i < *index; ++i) a->holes.insert(i);
    }
    if (value) a->values[*index] = std::move(*value);
    a->holes.erase(*index);
    if (d.writable) flags.writable = *d.writable;
    if (d.enumerable) flags.enumerable = *d.enumerable;
    if (d.configurable) flags.configurable = *d.configurable;
    a->flags[*index] = flags; return true;
}
template <class T> std::optional<bool> arrayErase(const JSValue& self, const std::string& key) {
    if (key == "length") return false;
    const auto index = arrayIndex(key); if (!index) return {};
    auto a = payload<ArrayStorage<T>>(self, arrayTag<T>());
    if (*index >= a->values.size() || a->holes.contains(*index)) return true;
    if (a->flags.contains(*index) && !a->flags.at(*index).configurable) return false;
    a->holes.insert(*index); a->flags.erase(*index); a->values[*index] = T{}; return true;
}
template <class T> std::vector<std::string> arrayNames(const JSValue& self) {
    auto a = payload<ArrayStorage<T>>(self, arrayTag<T>());
    std::vector<std::string> names;
    for (std::size_t i = 0; i < a->values.size(); ++i) if (!a->holes.contains(i)) names.push_back(std::to_string(i));
    names.push_back("length"); return names;
}
template <class T> JSValue nativeSharedArray(SharedArray<T> array) {
    auto result = JSValue::hostObject(array.storage(), array.storage().get(), arrayTag<T>(), JSValue::Type::Array);
    if (initialize(result)) {
        static const auto hooks = std::make_shared<JSHostObjectHooks>(JSHostObjectHooks{
            arrayOwn<T>, arrayDefine<T>, arrayErase<T>, arrayNames<T>, nullptr, arrayTrace<T>, arrayOwners<T>});
        result.setHostHooks(hooks);
    }
    return result;
}

JSValue mathRead(const JSValue& self, const Args&, const Args& capture) {
    const auto& key = capture[0].string();
    const auto component = key.back();
    const int i = component == 'x' || component == 'r' ? 0 : component == 'y' || component == 'g' ? 1 : component == 'z' || component == 'b' ? 2 : 3;
    if (self.isHostObject("three.Vector2")) { auto v = payload<Vector2>(self, "three.Vector2"); return i == 0 ? v->x : v->y; }
    if (self.isHostObject("three.Vector3")) { auto v = payload<Vector3>(self, "three.Vector3"); return i == 0 ? v->x : i == 1 ? v->y : v->z; }
    if (self.isHostObject("three.Vector4")) { auto v = payload<Vector4>(self, "three.Vector4"); return i == 0 ? v->x : i == 1 ? v->y : i == 2 ? v->z : v->w; }
    if (self.isHostObject("three.Color")) { auto v = payload<Color>(self, "three.Color"); return i == 0 ? v->r : i == 1 ? v->g : v->b; }
    if (self.isHostObject("three.Quaternion")) return NativeObjectAccess::quaternion(*payload<Quaternion>(self, "three.Quaternion"), i);
    auto e = payload<Euler>(self, "three.Euler");
    return key == "order" || key == "_order" ? JSValue(e->getOrderString()) : JSValue(NativeObjectAccess::euler(*e, i));
}
JSValue mathWrite(const JSValue& self, const Args& args, const Args& capture) {
    const auto key = capture[0].string();
    const auto component = key.back();
    const int i = component == 'x' || component == 'r' ? 0 : component == 'y' || component == 'g' ? 1 : component == 'z' || component == 'b' ? 2 : 3;
    if (self.isHostObject("three.Euler") && (key == "order" || key == "_order")) {
        auto e = payload<Euler>(self, "three.Euler"); NativeObjectAccess::order(*e, argument(args, 0).toString());
        if (key[0] != '_') NativeObjectAccess::notify(*e); return {};
    }
    const auto value = argument(args, 0).toNumber();
    if (self.isHostObject("three.Vector2")) { auto v = payload<Vector2>(self, "three.Vector2"); (i == 0 ? v->x : v->y) = value; }
    else if (self.isHostObject("three.Vector3")) { auto v = payload<Vector3>(self, "three.Vector3"); (i == 0 ? v->x : i == 1 ? v->y : v->z) = value; }
    else if (self.isHostObject("three.Vector4")) { auto v = payload<Vector4>(self, "three.Vector4"); (i == 0 ? v->x : i == 1 ? v->y : i == 2 ? v->z : v->w) = value; }
    else if (self.isHostObject("three.Color")) { auto v = payload<Color>(self, "three.Color"); (i == 0 ? v->r : i == 1 ? v->g : v->b) = value; }
    else if (self.isHostObject("three.Quaternion")) {
        auto q = payload<Quaternion>(self, "three.Quaternion"); NativeObjectAccess::quaternion(*q, i, value);
        if (key[0] != '_') NativeObjectAccess::notify(*q);
    } else {
        auto e = payload<Euler>(self, "three.Euler"); NativeObjectAccess::euler(*e, i, value);
        if (key[0] != '_') NativeObjectAccess::notify(*e);
    }
    return {};
}
JSValue mathArray(const JSValue& self, const Args& args, const Args& capture) {
    auto target = self;
    const bool read = capture[0].string() == "toArray";
    auto array = argument(args, 0);
    if (read && array.isUndefined()) array = JSValue::array();
    auto offset = argument(args, 1); if (offset.isUndefined()) offset = 0;
    const auto tag=capture[1].string();
    const bool quaternion = tag=="three.Quaternion", euler = tag=="three.Euler";
    if (euler && !read) offset = 0; // Euler.fromArray has no offset argument.
    const bool color = tag=="three.Color";
    const int count = tag=="three.Vector2" ? 2 : quaternion || euler || tag=="three.Vector4" ? 4 : 3;
    for (int i = 0; i < count; ++i) {
        const auto index = i == 0 ? offset : Interpolant_detail::add(offset, JSValue(i));
        std::string component = euler && i == 3 ? "order" : std::string(1, (color ? "rgb" : "xyzw")[i]);
        if (quaternion || euler) component.insert(component.begin(), '_');
        if (read) { const auto value = self.get(component); array.set(index, value); }
        else {
            const auto value = array.get(index);
            if (euler && i == 3) {
                if (!value.isUndefined()) target.set(component, array.get("3"));
            } else target.set(component, value);
        }
    }
    if (!read && (quaternion || euler)) self.get("_onChangeCallback").call(self,{});
    return read ? array : self;
}
JSValue mathClone(const JSValue& self, const Args&, const Args&) {
    if (self.isHostObject("three.Vector2")) return nativeVector(std::make_shared<Vector2>(payload<Vector2>(self,"three.Vector2")->clone()));
    if (self.isHostObject("three.Vector3")) return nativeVector(std::make_shared<Vector3>(payload<Vector3>(self,"three.Vector3")->clone()));
    if (self.isHostObject("three.Vector4")) return nativeVector(std::make_shared<Vector4>(payload<Vector4>(self,"three.Vector4")->clone()));
    if (self.isHostObject("three.Quaternion")) return nativeQuaternion(std::make_shared<Quaternion>(payload<Quaternion>(self,"three.Quaternion")->clone()));
    if (self.isHostObject("three.Euler")) return nativeEuler(std::make_shared<Euler>(payload<Euler>(self,"three.Euler")->clone()));
    return nativeColor(std::make_shared<Color>(payload<Color>(self,"three.Color")->clone()));
}
JSValue mathCopy(const JSValue& self, const Args& args, const Args&) {
    const auto source = argument(args,0); auto result = self;
    const bool color = self.isHostObject("three.Color");
    const bool euler = self.isHostObject("three.Euler"), quaternion = self.isHostObject("three.Quaternion");
    const auto components = color ? "rgb" : self.isHostObject("three.Vector2") ? "xy" :
        self.isHostObject("three.Vector4") || quaternion ? "xyzw" : "xyz";
    for (const char* component = components; *component; ++component) {
        const std::string key(1,*component); const auto field = euler || quaternion ? "_" + key : key;
        result.set(field,source.get(euler ? field : key));
    }
    if (euler) { result.set("_order",source.get("_order")); NativeObjectAccess::notify(*payload<Euler>(self,"three.Euler")); }
    if (quaternion) NativeObjectAccess::notify(*payload<Quaternion>(self,"three.Quaternion"));
    return self;
}
std::vector<JSNativeOwnerNode> mathOwners(const JSValue& self) {
    const auto owner=self.getHostSlot("nativeObjectOwner");
    if(owner.isHostObject(objectTag))return animationObjectOwners(owner);
    const auto shadow=self.getHostSlot("nativeShadowOwner");
    if(shadow.isHostObject("three.LightShadow"))return shadowOwners(shadow);
    const auto texture=self.getHostSlot("nativeTextureOwner");
    if(texture.isHostObject("three.Texture"))return textureOwners(texture);
    return {{self.hostPayload(self.getHostSlot("nativeTag").string()),{},{}}};
}
// Public mathematical protocols used by renderer uniform modules. Read and
// write through JSValue, so borrowed receivers, accessors and overridden methods
// retain their call order instead of bypassing the managed graph via payloads.
JSValue mathAdd(const JSValue& left,const JSValue& right) {
    const auto a=left.toPrimitive(),b=right.toPrimitive();
    if(a.type()==JSValue::Type::String||b.type()==JSValue::Type::String)return a.toString()+b.toString();
    const auto x=a.toNumber();const auto y=b.toNumber();return x+y;
}
double mathProduct(const JSValue& left,const JSValue& right) {const auto x=left.toNumber();const auto y=right.toNumber();return x*y;}
JSValue mathCall(const JSValue& self,const char* name,const Args& args={}) {return self.get(name).call(self,args);}
JSValue lightingMath(const JSValue& self,const Args& args,const Args& captures) {
    const auto name=captures[0].string(),tag=captures[1].string();auto result=self;
    const auto source=argument(args,0),scalar=argument(args,1);
    const char* components=tag=="three.Color"?"rgb":tag=="three.Vector2"?"xy":tag=="three.Vector4"?"xyzw":"xyz";
    if(name=="copy") {
        for(const char* c=components;*c;++c){const std::string field(1,*c);
            if(tag=="three.Vector4"&&*c=='w') {
                const auto checked=source.get("w");result.set("w",checked.isUndefined()?JSValue(1):source.get("w"));
            } else result.set(field,source.get(field));
        }return self;
    }
    if(name=="negate") {for(const char* c=components;*c;++c){const std::string field(1,*c);result.set(field,-self.get(field).toNumber());}return self;}
    if(name=="fromArray"||name=="toArray") {
        auto array=source;if(name=="toArray"&&array.isUndefined())array=JSValue::array();
        const auto offset=scalar.isUndefined()?JSValue(0):scalar;
        for(int i=0;components[i];++i) {
            const auto index=i?mathAdd(offset,i):offset;const std::string field(1,components[i]);
            if(name=="fromArray") {
                result.set(field,array.get(index));
            } else {
                const auto value=self.get(field);
                array.set(index,value);
            }
        }
        return name=="fromArray"?self:array;
    }
    if(name=="set") {
        auto z=argument(args,2);if(tag=="three.Vector3"&&z.isUndefined())z=self.get("z");
        result.set("x",source);result.set("y",scalar);
        if(tag=="three.Vector3"||tag=="three.Vector4")result.set("z",z);
        if(tag=="three.Vector4")result.set("w",argument(args,3));return self;
    }
    if(name=="multiplyScalar"||name=="multiply"||name=="sub"||name=="addScaledVector") {
        for(const char* c=components;*c;++c) {
            const std::string field(1,*c);const auto current=self.get(field);
            if(name=="multiplyScalar")result.set(field,mathProduct(current,source));
            else if(name=="multiply") {const auto value=source.get(field);result.set(field,mathProduct(current,value));}
            else if(name=="sub") {const auto value=source.get(field);const auto x=current.toNumber();const auto y=value.toNumber();result.set(field,x-y);}
            else {const auto value=source.get(field);const auto product=mathProduct(value,scalar);result.set(field,mathAdd(current,product));}
        }return self;
    }
    if(name=="length"||name=="lengthSq") {
        double sum=0;bool first=true;for(const char* c=components;*c;++c){const std::string field(1,*c);const auto x=self.get(field),y=self.get(field);const auto product=mathProduct(x,y);if(first){sum=product;first=false;}else sum+=product;}
        return name=="length"?std::sqrt(sum):sum;
    }
    if(name=="divideScalar") {const auto fn=self.get("multiplyScalar");const auto reciprocal=1/source.toNumber();return fn.call(self,{reciprocal});}
    if(name=="normalize") {const auto fn=self.get("divideScalar"),length=mathCall(self,"length");return fn.call(self,{length.toBoolean()?length:JSValue(1)});}
    if(name=="setFromMatrixPosition") {const auto elements=source.get("elements");for(int i=0;i<3;++i)result.set(std::string(1,"xyz"[i]),elements.get(std::to_string(12+i)));return self;}
    if(name=="setFromMatrixColumn") {const auto fn=self.get("fromArray"),elements=source.get("elements");const auto offset=scalar.toNumber()*4;return fn.call(self,{elements,offset});}
    if(name=="dot") {
        double total=0;
        for(int i=0;i<3;++i) {
            const std::string field(1,"xyz"[i]);
            const auto left=self.get(field),right=source.get(field);
            const auto product=mathProduct(left,right);
            total=i?total+product:product;
        }
        return total;
    }
    if(name=="applyMatrix3") {
        const auto x=self.get(std::string(1,components[0])),y=self.get(std::string(1,components[1])),z=self.get(std::string(1,components[2])),elements=source.get("elements");
        for(int i=0;i<3;++i) {
            const auto e0=elements.get(std::to_string(i));const auto a=mathProduct(e0,x);
            const auto e1=elements.get(std::to_string(i+3));const auto b=mathProduct(e1,y);
            const auto e2=elements.get(std::to_string(i+6));const auto c=mathProduct(e2,z);
            result.set(std::string(1,components[i]),(a+b)+c);
        }
        return self;
    }
    if(name=="applyMatrix4"||name=="transformDirection") {
        const auto x=self.get("x"),y=self.get("y"),z=self.get("z"),elements=source.get("elements");
        const auto term=[&](int index,const JSValue& value){const auto element=elements.get(std::to_string(index));return mathProduct(element,value);};
        double w=1;
        if(name=="applyMatrix4") {const auto a=term(3,x),b=term(7,y),c=term(11,z);const auto d=elements.get("15");w=1/mathAdd((a+b)+c,d).toNumber();}
        for(int i=0;i<3;++i) {const auto a=term(i,x),b=term(i+4,y),c=term(i+8,z);JSValue value=(a+b)+c;if(name=="applyMatrix4"){const auto d=elements.get(std::to_string(i+12));value=mathAdd(value,d);value=value.toNumber()*w;}result.set(std::string(1,"xyz"[i]),value);}
        return name=="transformDirection"?mathCall(self,"normalize"):self;
    }
    throw JSTypeError("Unknown mathematical protocol");
}
void lightingMethod(JSValue object,const char* name,JSValue::NativeFunction callback,const char* tag,unsigned arity) {
    auto fn=JSValue::function(callback,{name,tag},name,arity);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;d.enumerable=false;object.defineProperty(name,d);
}
JSValue materialsColor(const JSValue&,const Args&,const Args&);
JSValue colorScratch();
#include "NativeMathEqualsSource.inc"
#include "NativeMathStateSource.inc"
JSValue mathEquals(const JSValue& self,const Args& args,const Args& captures) {
    const auto other=argument(args,0);
    const auto& tag=captures[1].string();
    if(tag=="three.Euler") {
        for(const auto* key:{"_x","_y","_z","_order"}) {
            const auto left=other.get(key);const auto right=self.get(key);
            if(!left.strictEquals(right))return false;
        }
        return true;
    }
    const char* components=tag=="three.Color"?"rgb":tag=="three.Vector2"?"xy":tag=="three.Vector4"?"xyzw":"xyz";
    for(const char* component=components;*component;++component) {
        const std::string key(1,*component);
        const auto left=other.get(key);const auto right=self.get(key);
        if(!left.strictEquals(right))return false;
    }
    return true;
}
JSValue matrixEquals(const JSValue& self,const Args& args,const Args& captures) {
    const auto te=self.get("elements");const auto me=argument(args,0).get("elements");
    const unsigned size=captures[1].strictEquals("three.Matrix4")?16:9;
    for(unsigned i=0;i<size;++i) {
        const auto left=te.get(JSValue(i));const auto right=me.get(JSValue(i));
        if(!left.strictEquals(right))return false;
    }
    return true;
}
JSValue mathPrototype(const char* tag,const char* components,const char* flag,bool privateComponents) {
    static thread_local std::map<std::string, JSWeakValue> prototypes;
    return cachedModuleValue(prototypes[tag], [&] {
            auto p = JSValue::object();
            if (!privateComponents && std::string(tag) != "three.Color") p.set(flag,true);
            if (std::string(tag) == "three.Vector2") {
                for (const auto* name : {"width", "height"}) {
                    JSPropertyDescriptor d; d.configurable = true;
                    const auto component = std::string(name) == "width" ? "x" : "y";
                    d.getter = JSValue::function(+[](const JSValue& self,const Args&,const Args& cap)->JSValue {
                        return self.get(cap[0].string());
                    }, {component}, std::string("get ") + name, 0);
                    d.setter = JSValue::function(+[](const JSValue& self,const Args& args,const Args& cap)->JSValue {
                        auto target = self; target.set(cap[0].string(), argument(args,0)); return {};
                    }, {component}, std::string("set ") + name, 1);
                    p.defineProperty(name,d);
                }
            }
            if (privateComponents) {
                for (const char* c = components; *c; ++c) {
                    const std::string key(1,*c); accessor(p,key.c_str(),mathRead,mathWrite,false);
                }
                if (std::string(tag) == "three.Euler") accessor(p,"order",mathRead,mathWrite,false);
            }
            method(p, "fromArray", mathArray, tag); method(p, "toArray", mathArray, tag);
            if(std::string(tag)=="three.Quaternion"||std::string(tag)=="three.Euler") {
                const bool quaternion=std::string(tag)=="three.Quaternion";
                lightingMethod(p,"_onChangeCallback",[](const JSValue&,const Args&,const Args&) {return JSValue{};},tag,0);
                p.get("_onChangeCallback").setFunctionSourceText(quaternion?NativeMathStateSource::Quaternion__onChangeCallback:NativeMathStateSource::Euler__onChangeCallback);
                lightingMethod(p,"_onChange",[](const JSValue& self,const Args& args,const Args&) {
                    auto target=self;target.set("_onChangeCallback",argument(args,0));return self;
                },tag,1);
                p.get("_onChange").setFunctionSourceText(quaternion?NativeMathStateSource::Quaternion__onChange:NativeMathStateSource::Euler__onChange);
                lightingMethod(p,"fromArray",mathArray,tag,1);
                lightingMethod(p,"toArray",mathArray,tag,0);
                p.get("fromArray").setFunctionSourceText(quaternion?NativeMathStateSource::Quaternion_fromArray:NativeMathStateSource::Euler_fromArray);
                p.get("toArray").setFunctionSourceText(quaternion?NativeMathStateSource::Quaternion_toArray:NativeMathStateSource::Euler_toArray);
            }
            method(p, "clone", mathClone, tag);
            method(p, "copy", mathCopy, tag);
            if (std::string(tag)=="three.Euler") {
                lightingMethod(p,"equals",mathEquals,tag,1);
                p.get("equals").setFunctionSourceText(source_equals_Euler);
            }
            if (std::string(tag)=="three.Vector2" || std::string(tag)=="three.Vector3" ||
                std::string(tag)=="three.Vector4" || std::string(tag)=="three.Color") {
                lightingMethod(p,"equals",mathEquals,tag,1);
                p.get("equals").setFunctionSourceText(std::string(tag)=="three.Vector2"?source_equals_Vector2:
                    std::string(tag)=="three.Vector3"?source_equals_Vector3:
                    std::string(tag)=="three.Vector4"?source_equals_Vector4:source_equals_Color);
            }
            if (std::string(tag) == "three.Color") lightingMethod(p,"set",[](const JSValue& self,const Args& args,const Args&) {
                if(!argument(args,1).isUndefined()||!argument(args,2).isUndefined()) {
                    const auto method=self.get("setRGB");
                    method.call(self,{argument(args,0),argument(args,1),argument(args,2)});return self;
                }
                const auto value = argument(args,0);
                if (value.toBoolean() && value.get("isColor").toBoolean()) {
                    self.get("copy").call(self,{value});return self;
                }
                if (value.type() == JSValue::Type::Number) payload<Color>(self,"three.Color")->setHex(value.number());
                else if (value.type() == JSValue::Type::String) payload<Color>(self,"three.Color")->setStyle(value.string());
                return self;
            },tag,3);
            if (std::string(tag) == "three.Color") {
                p.get("set").setFunctionSourceText(NativeMathStateSource::Color_set);
                lightingMethod(p,"setRGB",[](const JSValue& self,const Args& args,const Args&) {
                    auto target=self;auto colorSpace=argument(args,3);
                    if(colorSpace.isUndefined())colorSpace=nativeColorManagement().get("workingColorSpace");
                    target.set("r",argument(args,0));target.set("g",argument(args,1));target.set("b",argument(args,2));
                    const auto management=nativeColorManagement();const auto fn=management.get("colorSpaceToWorking");
                    fn.call(management,{self,colorSpace});return self;
                },tag,3);
                p.get("setRGB").setFunctionSourceText(NativeMathStateSource::Color_setRGB);
                method(p, "getHex", [](const JSValue& self, const Args& args, const Args&) {
                    return JSValue(payload<Color>(self,"three.Color")->getHex(argument(args,0).isUndefined() ? SRGBColorSpace : argument(args,0).toString()));
                }, tag);
                lightingMethod(p,"getRGB",materialsColor,tag,1);
                lightingMethod(p,"applyMatrix3",lightingMath,tag,1);
            }
            if (std::string(tag)=="three.Vector4") {
                for(const auto* name:{"copy","fromArray"})lightingMethod(p,name,lightingMath,tag,1);
                lightingMethod(p,"set",lightingMath,tag,4);
                lightingMethod(p,"toArray",lightingMath,tag,0);
                p.get("set").setFunctionSourceText(NativeMathStateSource::Vector4_set);
                p.get("copy").setFunctionSourceText(NativeMathStateSource::Vector4_copy);
                p.get("fromArray").setFunctionSourceText(NativeMathStateSource::Vector4_fromArray);
                p.get("toArray").setFunctionSourceText(NativeMathStateSource::Vector4_toArray);
            }
            if (std::string(tag)=="three.Color"||std::string(tag)=="three.Vector2"||std::string(tag)=="three.Vector3") {
                for(const auto* name:{"copy","fromArray"})lightingMethod(p,name,lightingMath,tag,1);
                lightingMethod(p,"toArray",lightingMath,tag,0);
                lightingMethod(p,"multiplyScalar",lightingMath,tag,1);
                if(std::string(tag)!="three.Color") {
                    lightingMethod(p,"set",lightingMath,tag,std::string(tag)=="three.Vector2"?2:3);
                    for(const auto* name:{"multiply","sub","divideScalar"})lightingMethod(p,name,lightingMath,tag,1);
                    lightingMethod(p,"addScaledVector",lightingMath,tag,2);
                    for(const auto* name:{"length","lengthSq","normalize"})lightingMethod(p,name,lightingMath,tag,0);
                    if(std::string(tag)=="three.Vector2")lightingMethod(p,"negate",lightingMath,tag,0);
                    if(std::string(tag)=="three.Vector3") {
                        for(const auto* name:{"setFromMatrixPosition","applyMatrix3","applyMatrix4","transformDirection","dot"})lightingMethod(p,name,lightingMath,tag,1);
                        lightingMethod(p,"setFromMatrixColumn",lightingMath,tag,2);
                    }
                }
            }
            return p;
        });
}
JSValue colorScratch() {
    static thread_local JSWeakValue cached;
    return cachedModuleValue(cached,[] {
        auto color=JSValue::object(mathPrototype("three.Color","rgb","isColor",false));
        // The imported Color module owns this ordinary instance. Native scalar
        // fields would coerce borrowed receiver values before conversion uses them.
        color.defineProperty("isColor",dataDescriptor(true));
        for(const auto* field:{"r","g","b"})color.defineProperty(field,dataDescriptor(1));
        return color;
    });
}
JSValue materialsColor(const JSValue& self,const Args& args,const Args&) {
    auto target=argument(args,0),targetSpace=argument(args,1);
    auto management=nativeColorManagement();if(targetSpace.isUndefined())targetSpace=management.get("workingColorSpace");
    const auto fn=management.get("workingToColorSpace");auto scratch=colorScratch();
    const auto converted=mathCall(scratch,"copy",{self});fn.call(management,{converted,targetSpace});
    for(const auto* field:{"r","g","b"})target.set(field,scratch.get(field));return target;
}

JSValue nativePropertyCallback(const JSValue& owner,const PropertyChangeSnapshot& snapshot) {
    auto token=JSValue::hostObject(snapshot,snapshot.get(),"three.PropertyChangeSnapshot");
    return JSValue::function([](const JSValue&,const Args&,const Args& captures) {
        const auto callback=std::static_pointer_cast<std::function<void()>>(captures[1].hostPayload("three.PropertyChangeSnapshot"));
        (*callback)();return JSValue{};
    },{owner,token});
}

template <class T> JSValue wrapMath(std::shared_ptr<T> value, const char* tag, const char* components,
                                   const char* flag, bool privateComponents = false) {
    if (!value) return nullptr;
    auto result = JSValue::hostObject(value, value.get(), tag);
    if (initialize(result)) {
        result.setHostSlot("nativeTag", tag);
        static auto hooks = [] { auto h = std::make_shared<JSHostObjectHooks>(); h->nativeOwnership = mathOwners; return h; }();
        result.setHostHooks(hooks);
        const auto prototype = mathPrototype(tag,components,flag,privateComponents);
        result.setPrototype(prototype);
        if (privateComponents || std::string(tag) == "three.Color") result.set(flag,true);
        for (const char* c = components; *c; ++c) {
            const std::string key = (privateComponents ? "_" : "") + std::string(1,*c);
            accessor(result,key.c_str(),mathRead,mathWrite);
        }
        if (std::string(tag) == "three.Euler") accessor(result,"_order",mathRead,mathWrite);
        if constexpr(std::is_same_v<T,Quaternion>||std::is_same_v<T,Euler>) {
            const auto record=NativeObjectAccess::callbackRecord(*value);
            const JSWeakValue weak(result);
            record->managedNotify=[weak] {
                if(auto owner=weak.lock()) {
                    const auto callback=owner->get("_onChangeCallback");
                    callback.call(*owner,{});return true;
                }
                return false;
            };
            record->managedRegister=[weak](const PropertyChangeSnapshot& snapshot) {
                if(auto owner=weak.lock())owner->set("_onChangeCallback",nativePropertyCallback(*owner,snapshot));
            };
            if(record->registered)result.set("_onChangeCallback",nativePropertyCallback(result,record->callback));
        }
    }
    return result;
}
} // namespace

JSValue nativeVector(const std::shared_ptr<Vector2>& v) { return wrapMath(v, "three.Vector2", "xy", "isVector2"); }
JSValue nativeVector(const std::shared_ptr<Vector3>& v) { return wrapMath(v, "three.Vector3", "xyz", "isVector3"); }
JSValue nativeVector(const std::shared_ptr<Vector4>& v) { return wrapMath(v, "three.Vector4", "xyzw", "isVector4"); }
JSValue nativeQuaternion(const std::shared_ptr<Quaternion>& q) { return wrapMath(q, "three.Quaternion", "xyzw", "isQuaternion", true); }
JSValue nativeEuler(const std::shared_ptr<Euler>& e) { return wrapMath(e, "three.Euler", "xyz", "isEuler", true); }
JSValue nativeColorPrototype() {return mathPrototype("three.Color","rgb","isColor",false);}
JSValue nativeColor(const std::shared_ptr<Color>& c) { auto result=wrapMath(c, "three.Color", "rgb", "isColor"); if(c)colorScratch(); return result; }

namespace {
std::size_t matrixElementCount(const JSValue& self) { return self.isHostObject("three.MatrixElements9") ? 9 : 16; }
std::shared_ptr<double> matrixElementsPayload(const JSValue& self) {
    return payload<double>(self, matrixElementCount(self) == 9 ? "three.MatrixElements9" : "three.MatrixElements16");
}
JSHostPropertyResult matrixElementOwn(const JSValue& self, const std::string& key) {
    const auto size = matrixElementCount(self);
    if (key == "length") return {true,dataDescriptor(double(size),true,false,false)};
    const auto index = arrayIndex(key);
    if (!index) return {};
    if (*index >= size) return {true,{}};
    return {true,dataDescriptor(matrixElementsPayload(self).get()[*index])};
}
std::optional<bool> matrixElementDefine(const JSValue& self, const std::string& key, const JSPropertyDescriptor& descriptor) {
    if (key == "length") {
        if (descriptor.getter || descriptor.setter || (descriptor.value && descriptor.value->toNumber() != double(matrixElementCount(self))) ||
            !descriptor.writable.value_or(true) || descriptor.enumerable.value_or(false) || descriptor.configurable.value_or(false))
            throw JSTypeError("Native matrix elements retain a fixed writable length");
        return true;
    }
    const auto index = arrayIndex(key);
    if (!index) return {};
    if (*index >= matrixElementCount(self) || descriptor.getter || descriptor.setter ||
        !descriptor.writable.value_or(true) || !descriptor.enumerable.value_or(true) || !descriptor.configurable.value_or(true))
        throw JSTypeError("Native matrix elements require existing writable numeric indices");
    if (descriptor.value) matrixElementsPayload(self).get()[*index] = descriptor.value->toNumber();
    return true;
}
std::optional<bool> matrixElementErase(const JSValue& self, const std::string& key) {
    if (key == "length") return false;
    const auto index = arrayIndex(key);
    if (index && *index < matrixElementCount(self)) throw JSTypeError("Native matrix elements cannot be deleted");
    return index ? std::optional(true) : std::nullopt;
}
std::vector<std::string> matrixElementNames(const JSValue& self) {
    std::vector<std::string> names;
    for (std::size_t i = 0; i < matrixElementCount(self); ++i) names.push_back(std::to_string(i));
    names.push_back("length"); return names;
}
JSValue matrixMethod(const JSValue& self, const Args& args, const Args& capture) {
    const auto name = capture[0].string();
    const auto size = self.isHostObject("three.Matrix3") ? 9 : 16;
    if (name == "clone") {
        auto result = size == 9 ? nativeMatrix(std::make_shared<Matrix3>()) : nativeMatrix(std::make_shared<Matrix4>());
        result.get("fromArray").call(result,{self.get("elements")}); return result;
    }
    const bool read = name == "toArray";
    auto array = argument(args,0); if (read && array.isUndefined()) array = JSValue::array();
    auto offset = argument(args,1); if (offset.isUndefined()) offset = 0;
    auto elements = self.get("elements");
    for (int i = 0; i < size; ++i) {
        const auto index = i == 0 ? offset : Interpolant_detail::add(offset, JSValue(i));
        if (read) { const auto value=elements.get(std::to_string(i));const auto key=index.toPrimitive(JSValue::PreferredType::String).toString();array.set(key,value); }
        else elements.set(std::to_string(i),array.get(index.toString()));
    }
    return read ? array : self;
}
JSValue lightingMatrix(const JSValue& self,const Args& args,const Args& captures) {
    const auto name=captures[0].string();auto result=self;const auto source=argument(args,0);
    if(name=="set") {auto elements=self.get("elements");for(int row=0;row<4;++row)for(int col=0;col<4;++col)elements.set(std::to_string(row+col*4),argument(args,row*4+col));return self;}
    if(name=="identity")return mathCall(self,"set",{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1});
    if(name=="copy") {auto elements=self.get("elements");const auto from=source.get("elements");for(int i=0;i<16;++i)elements.set(std::to_string(i),from.get(std::to_string(i)));return self;}
    if(name=="multiply"||name=="premultiply") {const auto fn=self.get("multiplyMatrices");return fn.call(self,name=="multiply"?Args{self,source}:Args{source,self});}
    if(name=="multiplyMatrices") {
        const auto a=source.get("elements"),b=argument(args,1).get("elements");auto elements=self.get("elements");
        std::array<JSValue,16> av,bv;for(int row=0;row<4;++row)for(int col=0;col<4;++col)av[row+col*4]=a.get(std::to_string(row+col*4));
        for(int row=0;row<4;++row)for(int col=0;col<4;++col)bv[row+col*4]=b.get(std::to_string(row+col*4));
        for(int row=0;row<4;++row)for(int col=0;col<4;++col) {const auto p0=mathProduct(av[row],bv[col*4]),p1=mathProduct(av[row+4],bv[col*4+1]),p2=mathProduct(av[row+8],bv[col*4+2]),p3=mathProduct(av[row+12],bv[col*4+3]);elements.set(std::to_string(row+col*4),((p0+p1)+p2)+p3);}return self;
    }
    if(name=="determinantAffine") {
        const auto elements=self.get("elements");std::array<JSValue,9> v;for(int row=0;row<3;++row)for(int col=0;col<3;++col)v[row*3+col]=elements.get(std::to_string(row+col*4));
        const auto p0=mathProduct(v[4],v[8]),p1=mathProduct(v[5],v[7]);const auto t0=mathProduct(v[0],p0-p1);
        const auto p2=mathProduct(v[3],v[8]),p3=mathProduct(v[5],v[6]);const auto t1=mathProduct(v[1],p2-p3);
        const auto p4=mathProduct(v[3],v[7]),p5=mathProduct(v[4],v[6]);const auto t2=mathProduct(v[2],p4-p5);return (t0-t1)+t2;
    }
    if(name=="extractRotation") {
        if(mathCall(source,"determinantAffine").strictEquals(JSValue(0)))return mathCall(self,"identity");
        auto elements=self.get("elements");const auto from=source.get("elements");
        static thread_local JSWeakValue weak;JSValue scratch;if(const auto alive=weak.lock();alive&&alive->isObject())scratch=*alive;else{scratch=nativeVector(std::make_shared<Vector3>());scratch.retainForHeapLifetime();weak=JSWeakValue(scratch);}
        double scales[3];for(int i=0;i<3;++i){const auto vector=mathCall(scratch,"setFromMatrixColumn",{source,i});const auto length=mathCall(vector,"length");scales[i]=1/length.toNumber();}
        for(int col=0;col<3;++col){for(int row=0;row<3;++row){const auto value=from.get(std::to_string(col*4+row));elements.set(std::to_string(col*4+row),mathProduct(value,scales[col]));}elements.set(std::to_string(col*4+3),0);}
        elements.set("12",0);elements.set("13",0);elements.set("14",0);elements.set("15",1);return self;
    }
    throw JSTypeError("Unknown matrix protocol");
}
JSValue clippingMatrix(const JSValue& self,const Args& args,const Args& captures) {
    const auto name=captures[0].string();
    if(name=="getNormalMatrix") {
        const auto from=mathCall(self,"setFromMatrix4",{argument(args,0)});
        const auto inverse=mathCall(from,"invert");
        return mathCall(inverse,"transpose");
    }
    if(name=="setFromMatrix4") {
        const auto elements=argument(args,0).get("elements"),fn=self.get("set");
        Args values;for(const int index:{0,4,8,1,5,9,2,6,10})values.push_back(elements.get(std::to_string(index)));
        fn.call(self,values);return self;
    }
    auto elements=self.get("elements");
    if(name=="set") {
        for(int col=0;col<3;++col)for(int row=0;row<3;++row)
            elements.set(std::to_string(col*3+row),argument(args,row*3+col));
        return self;
    }
    if(name=="transpose") {
        for(const auto pair:{std::pair{1,3},std::pair{2,6},std::pair{5,7}}) {
            const auto first=std::to_string(pair.first),second=std::to_string(pair.second);
            const auto temporary=elements.get(first),value=elements.get(second);
            elements.set(first,value);elements.set(second,temporary);
        }
        return self;
    }
    if(name=="invert") {
        const auto n11=elements.get("0"),n21=elements.get("1"),n31=elements.get("2"),
                   n12=elements.get("3"),n22=elements.get("4"),n32=elements.get("5"),
                   n13=elements.get("6"),n23=elements.get("7"),n33=elements.get("8");
        const auto difference=[](const JSValue& a,const JSValue& b,const JSValue& c,const JSValue& d) {
            const auto first=mathProduct(a,b);const auto second=mathProduct(c,d);return first-second;
        };
        const auto t11=difference(n33,n22,n32,n23),t12=difference(n32,n13,n33,n12),
                   t13=difference(n23,n12,n22,n13);
        const auto d0=mathProduct(n11,t11),d1=mathProduct(n21,t12),d2=mathProduct(n31,t13);
        const auto determinant=(d0+d1)+d2;
        if(determinant==0)return mathCall(self,"set",{0,0,0,0,0,0,0,0,0});
        const auto inverse=1/determinant;
        elements.set("0",t11*inverse);
        elements.set("1",difference(n31,n23,n33,n21)*inverse);
        elements.set("2",difference(n32,n21,n31,n22)*inverse);
        elements.set("3",t12*inverse);
        elements.set("4",difference(n33,n11,n31,n13)*inverse);
        elements.set("5",difference(n31,n12,n32,n11)*inverse);
        elements.set("6",t13*inverse);
        elements.set("7",difference(n21,n13,n23,n11)*inverse);
        elements.set("8",difference(n22,n11,n21,n12)*inverse);
        return self;
    }
    throw JSTypeError("Unknown Matrix3 clipping protocol");
}
JSValue materialsMatrix3(const JSValue& self,const Args& args,const Args& captures) {
    const auto name=captures[0].string();
    if(name=="multiply"||name=="premultiply") {
        const auto fn=self.get("multiplyMatrices");
        const auto source=argument(args,0);
        return fn.call(self,name=="multiply"?Args{self,source}:Args{source,self});
    }
    if(name=="copy") {
        auto elements=self.get("elements");
        const auto source=argument(args,0).get("elements");
        for(int i=0;i<9;++i)elements.set(std::to_string(i),source.get(std::to_string(i)));
        return self;
    }
    if(name=="multiplyMatrices") {
        const auto a=argument(args,0).get("elements");
        const auto b=argument(args,1).get("elements");
        auto elements=self.get("elements");
        std::array<JSValue,9> av,bv;
        for(int row=0;row<3;++row)for(int col=0;col<3;++col)av[row+col*3]=a.get(std::to_string(row+col*3));
        for(int row=0;row<3;++row)for(int col=0;col<3;++col)bv[row+col*3]=b.get(std::to_string(row+col*3));
        for(int row=0;row<3;++row)for(int col=0;col<3;++col) {
            const auto p0=mathProduct(av[row],bv[col*3]);
            const auto p1=mathProduct(av[row+3],bv[col*3+1]);
            const auto p2=mathProduct(av[row+6],bv[col*3+2]);
            elements.set(std::to_string(row+col*3),(p0+p1)+p2);
        }
        return self;
    }
    if(name=="setUvTransform") {
        const auto tx=argument(args,0),ty=argument(args,1),sx=argument(args,2),sy=argument(args,3),
                   rotation=argument(args,4),cx=argument(args,5),cy=argument(args,6);
        const auto c=JSNumberMath::cos(rotation.toNumber());
        const auto s=JSNumberMath::sin(rotation.toNumber());
        const auto fn=self.get("set");
        Args values;
        values.push_back(mathProduct(sx,c));
        values.push_back(mathProduct(sx,s));
        const auto minusSx=-sx.toNumber();
        const auto cCx=mathProduct(c,cx);
        const auto sCy=mathProduct(s,cy);
        const auto xCenter=mathAdd(minusSx*(cCx+sCy),cx);
        values.push_back(mathAdd(xCenter,tx));
        values.push_back(-sy.toNumber()*s);
        values.push_back(mathProduct(sy,c));
        const auto minusSy=-sy.toNumber();
        const auto sCx=mathProduct(-s,cx);
        const auto cCy=mathProduct(c,cy);
        const auto yCenter=mathAdd(minusSy*(sCx+cCy),cy);
        values.push_back(mathAdd(yCenter,ty));
        values.insert(values.end(),{0,0,1});
        fn.call(self,values);
        return self;
    }
    throw JSTypeError("Unknown Matrix3 materials protocol");
}
JSValue materialsRotation(const JSValue& self,const Args& args,const Args&) {
    auto te=self.get("elements");
    const auto euler=argument(args,0);
    const auto x=euler.get("x"),y=euler.get("y"),z=euler.get("z");
    const auto a=JSNumberMath::cos(x.toNumber()),b=JSNumberMath::sin(x.toNumber());
    const auto c=JSNumberMath::cos(y.toNumber()),d=JSNumberMath::sin(y.toNumber());
    const auto e=JSNumberMath::cos(z.toNumber()),f=JSNumberMath::sin(z.toNumber());
    if(euler.get("order").strictEquals("XYZ")) {
        const auto ae=a*e,af=a*f,be=b*e,bf=b*f;
        te.set("0",c*e);te.set("4",-c*f);te.set("8",d);
        te.set("1",af+be*d);te.set("5",ae-bf*d);te.set("9",-b*c);
        te.set("2",bf-ae*d);te.set("6",be+af*d);te.set("10",a*c);
    } else if(euler.get("order").strictEquals("YXZ")) {
        const auto ce=c*e,cf=c*f,de=d*e,df=d*f;
        te.set("0",ce+df*b);te.set("4",de*b-cf);te.set("8",a*d);
        te.set("1",a*f);te.set("5",a*e);te.set("9",-b);
        te.set("2",cf*b-de);te.set("6",df+ce*b);te.set("10",a*c);
    } else if(euler.get("order").strictEquals("ZXY")) {
        const auto ce=c*e,cf=c*f,de=d*e,df=d*f;
        te.set("0",ce-df*b);te.set("4",-a*f);te.set("8",de+cf*b);
        te.set("1",cf+de*b);te.set("5",a*e);te.set("9",df-ce*b);
        te.set("2",-a*d);te.set("6",b);te.set("10",a*c);
    } else if(euler.get("order").strictEquals("ZYX")) {
        const auto ae=a*e,af=a*f,be=b*e,bf=b*f;
        te.set("0",c*e);te.set("4",be*d-af);te.set("8",ae*d+bf);
        te.set("1",c*f);te.set("5",bf*d+ae);te.set("9",af*d-be);
        te.set("2",-d);te.set("6",b*c);te.set("10",a*c);
    } else if(euler.get("order").strictEquals("YZX")) {
        const auto ac=a*c,ad=a*d,bc=b*c,bd=b*d;
        te.set("0",c*e);te.set("4",bd-ac*f);te.set("8",bc*f+ad);
        te.set("1",f);te.set("5",a*e);te.set("9",-b*e);
        te.set("2",-d*e);te.set("6",ad*f+bc);te.set("10",ac-bd*f);
    } else if(euler.get("order").strictEquals("XZY")) {
        const auto ac=a*c,ad=a*d,bc=b*c,bd=b*d;
        te.set("0",c*e);te.set("4",-f);te.set("8",d*e);
        te.set("1",ac*f+bd);te.set("5",a*e);te.set("9",ad*f-bc);
        te.set("2",bc*f-ad);te.set("6",b*e);te.set("10",bd*f+ac);
    }
    for(const auto* key:{"3","7","11","12","13","14"})te.set(key,0);
    te.set("15",1);
    return self;
}
JSValue matrixPrototype(bool matrix4) {
    static thread_local JSWeakValue cached3,cached4;
    return cachedModuleValue(matrix4?cached4:cached3,[matrix4] {
        const auto* tag=matrix4?"three.Matrix4":"three.Matrix3";
        auto p=JSValue::object();
        p.set(matrix4?"isMatrix4":"isMatrix3",true);
        for (const char* name : {"fromArray","toArray","clone"}) method(p,name,matrixMethod,tag);
        lightingMethod(p,"equals",matrixEquals,tag,1);
        p.get("equals").setFunctionSourceText(matrix4?source_equals_Matrix4:source_equals_Matrix3);
        if(matrix4) {
            lightingMethod(p,"set",lightingMatrix,tag,16);lightingMethod(p,"multiplyMatrices",lightingMatrix,tag,2);
            for(const auto* name:{"copy","multiply","premultiply","extractRotation"})lightingMethod(p,name,lightingMatrix,tag,1);
            for(const auto* name:{"identity","determinantAffine"})lightingMethod(p,name,lightingMatrix,tag,0);
            lightingMethod(p,"makeRotationFromEuler",materialsRotation,tag,1);
        } else {
            lightingMethod(p,"set",clippingMatrix,tag,9);
            for(const auto* name:{"setFromMatrix4","getNormalMatrix"})lightingMethod(p,name,clippingMatrix,tag,1);
            for(const auto* name:{"invert","transpose"})lightingMethod(p,name,clippingMatrix,tag,0);
            for(const auto* name:{"copy","multiply","premultiply"})lightingMethod(p,name,materialsMatrix3,tag,1);
            lightingMethod(p,"multiplyMatrices",materialsMatrix3,tag,2);
            lightingMethod(p,"setUvTransform",materialsMatrix3,tag,7);
        }
        return p;
    });
}
template <class T> JSValue wrapMatrix(const std::shared_ptr<T>& value, const char* tag, const char* elementsTag) {
    if (!value) return nullptr;
    auto result = JSValue::hostObject(value,value.get(),tag);
    if (initialize(result)) {
        result.setHostSlot("nativeTag",tag);
        static auto ownerHooks = [] { auto h = std::make_shared<JSHostObjectHooks>(); h->nativeOwnership = mathOwners; return h; }();
        result.setHostHooks(ownerHooks);
        auto elements = JSValue::hostObject(std::shared_ptr<double>(value,value->elements().data()),value->elements().data(),elementsTag,JSValue::Type::Array);
        if (initialize(elements)) {
            elements.setHostSlot("nativeTag",elementsTag);
            static auto hooks = [] { auto h = std::make_shared<JSHostObjectHooks>(JSHostObjectHooks{matrixElementOwn,matrixElementDefine,matrixElementErase,matrixElementNames}); h->nativeOwnership = mathOwners; return h; }();
            elements.setHostHooks(hooks);
        }
        result.setHostSlot("elements",elements);
        accessor(result,"elements",[](const JSValue& self,const Args&,const Args&){ return self.getHostSlot("elements"); });
        const auto prototype=matrixPrototype(std::is_same_v<T,Matrix4>);
        result.setPrototype(prototype);
    }
    return result;
}
}
JSValue nativeMatrix3Prototype() { return matrixPrototype(false); }
JSValue nativeMatrix4Prototype() { return matrixPrototype(true); }
JSValue nativeMatrix(const std::shared_ptr<Matrix3>& matrix) { return wrapMatrix(matrix,"three.Matrix3","three.MatrixElements9"); }
JSValue nativeMatrix(const std::shared_ptr<Matrix4>& matrix) {
    auto target = matrix;
    std::shared_ptr<Object3D> owner;
    while (target && target->aliasTarget()) {if(auto source=target->aliasObjectOwner().lock())owner=std::move(source);target = target->aliasTarget();}
    auto result=wrapMatrix(target,"three.Matrix4","three.MatrixElements16");
    if(owner){auto wrapper=nativeObject(owner);result.setHostSlot("nativeObjectOwner",wrapper);result.getHostSlot("elements").setHostSlot("nativeObjectOwner",wrapper);}
    return result;
}
namespace {
using Dictionary = std::map<std::string, std::size_t>;
JSHostPropertyResult dictionaryOwn(const JSValue& self, const std::string& key) {
    auto map = payload<Dictionary>(self, "three.MorphDictionary");
    const auto found = map->find(key);
    if (found == map->end()) return {};
    return {true, dataDescriptor(double(found->second))};
}
std::optional<bool> dictionaryDefine(const JSValue& self, const std::string& key, const JSPropertyDescriptor& d) {
    auto map = payload<Dictionary>(self, "three.MorphDictionary");
    const auto present = map->contains(key);
    if (d.getter || d.setter || !d.writable.value_or(present) ||
        !d.enumerable.value_or(present) || !d.configurable.value_or(present))
        throw JSTypeError("Native morph dictionary supports only writable enumerable configurable data properties");
    if (!present && !self.isExtensible()) return false;
    if (!d.value) {
        if (!present) throw JSTypeError("Native morph dictionary new properties require numeric indices");
        return true;
    }
    const auto n = d.value->toNumber();
    if (!std::isfinite(n) || n < 0 || std::trunc(n) != n || n > 9007199254740991.)
        throw JSTypeError("Native morph dictionary indices must be nonnegative integers");
    (*map)[key] = std::size_t(n); return true;
}
std::optional<bool> dictionaryErase(const JSValue& self, const std::string& key) {
    payload<Dictionary>(self, "three.MorphDictionary")->erase(key); return true;
}
std::vector<std::string> dictionaryNames(const JSValue& self) {
    std::vector<std::string> result;
    for (const auto& [key, value] : *payload<Dictionary>(self, "three.MorphDictionary")) result.push_back(key);
    return result;
}
JSValue nativeDictionary(const std::shared_ptr<Dictionary>& dictionary) {
    if (!dictionary) return {};
    auto result = JSValue::hostObject(dictionary, dictionary.get(), "three.MorphDictionary");
    if (initialize(result)) {
        static auto hooks = std::make_shared<JSHostObjectHooks>(JSHostObjectHooks{
            dictionaryOwn, dictionaryDefine, dictionaryErase, dictionaryNames});
        result.setHostHooks(hooks);
    }
    return result;
}
JSValue geometryRead(const JSValue& self, const Args&, const Args& capture) {
    auto geometry = payload<BufferGeometry>(self, "three.BufferGeometry");
    const auto& key = capture[0].string();
    if (key == "uuid") return geometry->uuid;
    if (key == "name") return geometry->name;
    if (key == "type") return geometry->type;
    if (key == "isBufferGeometry") return true;
    if (key == "isInstancedBufferGeometry") return geometry->isInstancedBufferGeometry;
    if (key == "instanceCount") if(const auto instanced=std::dynamic_pointer_cast<InstancedBufferGeometry>(geometry))return instanced->instanceCount;
    if (key == "animations") return nativeAnimationClips(geometry->animations);
    if (key == "morphAttributes") {
        // The geometry's map stays live. Its value-vector arrays are snapshots;
        // migrating BufferGeometry.morphAttributes storage is a separate boundary.
        auto object = JSValue::object();
        for (const auto& [name, attributes] : geometry->morphAttributes) {
            auto array = JSValue::array();
            for (std::size_t i = 0; i < attributes.size(); ++i) {
                if (!attributes[i]) { array.set(std::to_string(i), nullptr); continue; }
                array.set(std::to_string(i), nativeAttribute(attributes[i]));
            }
            object.set(name, array);
        }
        return object;
    }
    return {};
}
JSValue wrapGeometry(const std::shared_ptr<BufferGeometry>& geometry) {
    if (!geometry) return nullptr;
    auto result = JSValue::hostObject(geometry, geometry.get(), "three.BufferGeometry");
    if (initialize(result)) {
        for (const char* key : {"uuid", "name", "type", "isBufferGeometry", "morphAttributes", "animations"}) accessor(result, key, geometryRead);
        if(std::dynamic_pointer_cast<InstancedBufferGeometry>(geometry)) {
            accessor(result,"isInstancedBufferGeometry",geometryRead);
            accessor(result,"instanceCount",geometryRead,[](const JSValue& self,const Args& args,const Args&){std::dynamic_pointer_cast<InstancedBufferGeometry>(payload<BufferGeometry>(self,"three.BufferGeometry"))->instanceCount=argument(args,0);return JSValue();});
        }
        static auto hooks = [] { auto h = std::make_shared<JSHostObjectHooks>(); h->trace = [](const JSValue& self) {
            std::vector<JSValue> result; traceNativeGeometry(self, payload<BufferGeometry>(self, "three.BufferGeometry"), result); return result;
        };h->nativeOwnership=geometryOwners; return h; }(); result.setHostHooks(hooks);
        installNativeGeometryRenderingBridge(result,geometry,hooks);
    }
    return result;
}
template <class T> JSValue primitiveRead(const std::shared_ptr<T>& object,const std::string& key) {
    if (key == "material") return object->materialIsArray || !object->materials.empty() ? nativeSharedArray(object->materials) : nativeMaterial(object->material);
    if (key == "geometry") return nativeGeometry(object->geometry);
    if (key == "morphTargetInfluences") return object->morphTargetInfluences ? nativeSharedArray(*object->morphTargetInfluences) : JSValue();
    if (key == "morphTargetDictionary") return nativeDictionary(object->morphTargetDictionary);
    return {};
}
JSValue planeScratch() {
    static thread_local JSWeakValue weak;
    if(const auto alive=weak.lock();alive&&alive->isObject())return *alive;
    auto state=JSValue::object(nullptr);
    // Shared module temporaries use ordinary fields, as in Plane.js. The public
    // native prototypes provide the methods, including user overrides.
    auto vector=JSValue::object(nativeVector(std::make_shared<Vector3>()).getPrototype());
    vector.set("x",0);vector.set("y",0);vector.set("z",0);
    auto matrix=JSValue::object(nativeMatrix(std::make_shared<Matrix3>()).getPrototype());
    auto elements=JSValue::array();
    for(int i=0;i<9;++i) {
        JSPropertyDescriptor entry;entry.value=i%4==0?1:0;
        entry.writable=entry.enumerable=entry.configurable=true;
        elements.defineProperty(std::to_string(i),entry);
    }
    matrix.set("elements",elements);state.set("vector",vector);state.set("normalMatrix",matrix);
    state.retainForHeapLifetime();weak=JSWeakValue(state);return state;
}
JSValue clippingPlane(const JSValue& self,const Args& args,const Args& captures) {
    const auto name=captures[0].string();const auto source=argument(args,0);auto result=self;
    if(name=="copy") {
        const auto normal=self.get("normal"),fn=normal.get("copy"),input=source.get("normal");
        fn.call(normal,{input});result.set("constant",source.get("constant"));return self;
    }
    if(name=="coplanarPoint") {
        const auto fn=source.get("copy"),normal=self.get("normal");
        const auto target=fn.call(source,{normal}),multiply=target.get("multiplyScalar");
        const auto constant=self.get("constant");return multiply.call(target,{-constant.toNumber()});
    }
    if(name=="applyMatrix4") {
        const auto scratch=planeScratch();const auto optional=argument(args,1);
        const auto normalMatrix=optional.toBoolean()?optional:mathCall(scratch.get("normalMatrix"),"getNormalMatrix",{source});
        const auto point=mathCall(self,"coplanarPoint",{scratch.get("vector")});
        const auto reference=mathCall(point,"applyMatrix4",{source});
        const auto transformed=mathCall(self.get("normal"),"applyMatrix3",{normalMatrix});
        const auto normal=mathCall(transformed,"normalize");
        const auto distance=mathCall(reference,"dot",{normal});result.set("constant",-distance.toNumber());return self;
    }
    throw JSTypeError("Unknown Plane clipping protocol");
}
JSValue planePrototype() {
    static thread_local JSWeakValue weakPrototype;
    if(const auto existing=weakPrototype.lock();existing&&existing->isObject())return *existing;
    auto prototype=JSValue::object();
    static_cast<void>(planeScratch());
    for(const auto* name:{"copy","coplanarPoint"})lightingMethod(prototype,name,clippingPlane,"three.Plane",1);
    lightingMethod(prototype,"applyMatrix4",clippingPlane,"three.Plane",2);
    prototype.retainForHeapLifetime();weakPrototype=JSWeakValue(prototype);return prototype;
}
JSValue wrapPlane(const std::shared_ptr<Plane>& plane) {
    if(!plane)return nullptr;auto result=JSValue::hostObject(plane,plane.get(),"three.Plane");
    if(initialize(result)) {
        result.setHostSlot("nativeTag","three.Plane");
        result.setPrototype(planePrototype());
        static auto hooks=[] {auto h=std::make_shared<JSHostObjectHooks>();h->nativeOwnership=mathOwners;return h;}();result.setHostHooks(hooks);
        accessor(result,"normal",[](const JSValue& self,const Args&,const Args&){auto p=payload<Plane>(self,"three.Plane");return nativeVector(std::shared_ptr<Vector3>(p,&p->normal));});
        accessor(result,"constant",[](const JSValue& self,const Args&,const Args&){return JSValue(payload<Plane>(self,"three.Plane")->constant);},[](const JSValue& self,const Args& args,const Args&){payload<Plane>(self,"three.Plane")->constant=argument(args,0).toNumber();return JSValue();});
        result.set("isPlane",true);
    }return result;
}
JSValue wrapBox(const std::shared_ptr<Box3>& box) {
    if(!box)return nullptr;auto result=JSValue::hostObject(box,box.get(),"three.Box3");
    if(initialize(result)) {
        result.setHostSlot("nativeTag","three.Box3");static auto hooks=[] {auto h=std::make_shared<JSHostObjectHooks>();h->nativeOwnership=mathOwners;return h;}();result.setHostHooks(hooks);
        for(const auto* key:{"min","max"})accessor(result,key,[](const JSValue& self,const Args&,const Args& captures){auto b=payload<Box3>(self,"three.Box3");return nativeVector(std::shared_ptr<Vector3>(b,captures[0].string()=="min"?&b->min:&b->max));});result.set("isBox3",true);
    }return result;
}
std::optional<JSValue> helperRead(const std::shared_ptr<Object3D>& object,const std::string& key) {
    if(const auto h=std::dynamic_pointer_cast<ArrowHelper>(object)){if(key=="line")return nativeObject(h->line);if(key=="cone")return nativeObject(h->cone);}
    if(const auto h=std::dynamic_pointer_cast<BoxHelper>(object))if(key=="object")return h->object?nativeObject(h->object):JSValue();
    if(const auto h=std::dynamic_pointer_cast<Box3Helper>(object))if(key=="box")return h->box?wrapBox(h->box):JSValue();
    if(const auto h=std::dynamic_pointer_cast<PlaneHelper>(object)){if(key=="plane")return h->plane?wrapPlane(h->plane):JSValue();if(key=="size")return h->size;}
    if(const auto h=std::dynamic_pointer_cast<CameraHelper>(object)){if(key=="camera")return h->camera?nativeObject(h->camera):JSValue();if(key=="pointMap")return h->pointMap;}
    if(const auto h=std::dynamic_pointer_cast<SkeletonHelper>(object)){if(key=="root")return h->root?nativeObject(h->root):JSValue();if(key=="bones")return nativeSharedArray(h->bones);if(key=="isSkeletonHelper")return h->isSkeletonHelper;}
    if(const auto h=std::dynamic_pointer_cast<PointLightHelper>(object)){if(key=="light")return nativeObject(h->light);if(key=="color")return h->color;}
    if(const auto h=std::dynamic_pointer_cast<DirectionalLightHelper>(object)){if(key=="light")return nativeObject(h->light);if(key=="color")return h->color;if(key=="lightPlane")return nativeObject(h->lightPlane);if(key=="targetLine")return nativeObject(h->targetLine);}
    if(const auto h=std::dynamic_pointer_cast<HemisphereLightHelper>(object)){if(key=="light")return nativeObject(h->light);if(key=="color")return h->color;if(key=="material")return nativeMaterial(h->material);}
    if(const auto h=std::dynamic_pointer_cast<SpotLightHelper>(object)){if(key=="light")return nativeObject(h->light);if(key=="color")return h->color;if(key=="cone")return nativeObject(h->cone);}
    if(const auto c=std::dynamic_pointer_cast<ArrayCamera>(object)){if(key=="cameras")return nativeSharedArray(c->cameras);if(key=="isArrayCamera")return c->isArrayCamera;if(key=="isMultiViewCamera")return c->isMultiViewCamera;}
    if(const auto g=std::dynamic_pointer_cast<ClippingGroup>(object)){if(key=="clippingPlanes")return nativeSharedArray(g->clippingPlanes);if(key=="enabled")return g->enabled;if(key=="clipIntersection")return g->clipIntersection;if(key=="clipShadows")return g->clipShadows;if(key=="isClippingGroup")return g->isClippingGroup;}
    return {};
}
JSValue nativeShadowTarget(const std::shared_ptr<RenderTarget>& target) {
    if(!target)return nullptr;auto result=JSValue::hostObject(target,target.get(),"three.RenderTarget");
    if(initialize(result)) {
        for(const auto* key:{"texture","depthTexture"})accessor(result,key,[](const JSValue& self,const Args&,const Args& c){const auto target=payload<RenderTarget>(self,"three.RenderTarget");return nativeTexture(c[0].string()=="texture"?target->texture.get():target->depthTexture.get());},[](const JSValue& self,const Args& args,const Args& c){const auto target=payload<RenderTarget>(self,"three.RenderTarget");const auto value=argument(args,0);const auto texture=value.isNull()?nullptr:payload<Texture>(value,"three.Texture");if(c[0].string()=="texture")target->texture=texture;else target->depthTexture=texture;return JSValue();});
        static auto hooks=[]{auto h=std::make_shared<JSHostObjectHooks>();h->nativeOwnership=targetOwners;h->trace=+[](const JSValue& self){std::vector<JSValue> values;const auto target=payload<RenderTarget>(self,"three.RenderTarget");for(const auto& texture:target->textures)traceNativeTexture(self,texture,values);traceNativeTexture(self,target->_depthTexture,values);return values;};return h;}();result.setHostHooks(hooks);
    }return result;
}
JSValue shadowRead(const JSValue& self,const Args&,const Args& captures) {
    const auto shadow=payload<LightShadow>(self,"three.LightShadow");const auto key=captures[0].string();
    if(key=="camera")return nativeObject(shadow->camera);
    if(key=="map")return nativeShadowTarget(shadow->map);
    if(key=="mapPass")return nativeShadowTarget(shadow->mapPass);
    if(key=="biasNode")return shadow->biasNode;
    if(key=="mapSize"){auto vector=nativeVector(std::shared_ptr<Vector2>(shadow,&shadow->mapSize));vector.setHostSlot("nativeShadowOwner",self);return vector;}
    if(key=="matrix"){auto matrix=nativeMatrix(std::shared_ptr<Matrix4>(shadow,&shadow->matrix));matrix.setHostSlot("nativeShadowOwner",self);matrix.getHostSlot("elements").setHostSlot("nativeShadowOwner",self);return matrix;}
    if(key=="intensity")return shadow->intensity;if(key=="bias")return shadow->bias;if(key=="normalBias")return shadow->normalBias;if(key=="radius")return shadow->radius;
    return {};
}
JSValue shadowWrite(const JSValue& self,const Args& args,const Args& captures) {
    const auto shadow=payload<LightShadow>(self,"three.LightShadow");const auto key=captures[0].string();const auto value=argument(args,0);
    if(key=="camera") {const auto camera=value.isNull()?nullptr:std::dynamic_pointer_cast<Camera>(payload<Object3D>(value,objectTag));if(!value.isNull()&&!camera)throw JSTypeError("LightShadow camera requires a native Camera");shadow->camera=camera;}
    else if(key=="map"||key=="mapPass") {const auto target=value.isNull()?nullptr:payload<RenderTarget>(value,"three.RenderTarget");(key=="map"?shadow->map:shadow->mapPass)=target;}
    else if(key=="biasNode")shadow->biasNode=value;
    else if(key=="intensity")shadow->intensity=value.toNumber();else if(key=="bias")shadow->bias=value.toNumber();else if(key=="normalBias")shadow->normalBias=value.toNumber();else if(key=="radius")shadow->radius=value.toNumber();
    else if(key=="mapSize"){const auto x=value.get("x").toNumber();const auto y=value.get("y").toNumber();shadow->mapSize.set(x,y);}
    else if(key=="matrix"){const auto values=value.get("elements");for(int i=0;i<16;++i)shadow->matrix.elements()[i]=values.get(std::to_string(i)).toNumber();}
    return {};
}
JSValue shadowMethod(const JSValue& self,const Args& args,const Args& captures) {
    const auto shadow=payload<LightShadow>(self,"three.LightShadow");const auto name=captures[0].string();
    if(!shadow)throw JSTypeError("LightShadow method requires a native LightShadow");
    if(name=="getViewportCount")return shadow->getViewportCount();
    if(name=="getFrameExtents"){auto vector=nativeVector(std::shared_ptr<Vector2>(shadow,&shadow->_frameExtents));vector.setHostSlot("nativeShadowOwner",self);return vector;}
    if(name=="updateMatrices"){const auto light=std::dynamic_pointer_cast<Light>(payload<Object3D>(argument(args,0),objectTag));if(!light)throw JSTypeError("LightShadow requires a native Light");shadow->updateMatrices(*light);return {};}
    if(name=="dispose"){shadow->dispose();return {};}
    return {};
}
JSValue nativeShadow(const std::shared_ptr<LightShadow>& shadow) {
    if(!shadow)return nullptr;auto result=JSValue::hostObject(shadow,shadow.get(),"three.LightShadow");
    if(initialize(result)) {
        for(const auto* field:{"camera","map","mapPass","biasNode","mapSize","matrix","intensity","bias","normalBias","radius"})accessor(result,field,shadowRead,shadowWrite);
        static thread_local JSWeakValue weakPrototype;
        JSValue prototype;if(const auto alive=weakPrototype.lock();alive&&alive->isObject())prototype=*alive;else{
            prototype=JSValue::object();for(const auto* name:{"getViewportCount","getFrameExtents","updateMatrices","dispose"})lightingMethod(prototype,name,shadowMethod,"three.LightShadow",std::string(name)=="updateMatrices"?1:0);
            prototype.retainForHeapLifetime();weakPrototype=JSWeakValue(prototype);
        }result.setPrototype(prototype);
        static auto hooks=[]{auto h=std::make_shared<JSHostObjectHooks>();h->nativeOwnership=shadowOwners;h->trace=+[](const JSValue& self){std::vector<JSValue> values;std::unordered_set<const Object3D*> visited;traceShadow(self,payload<LightShadow>(self,"three.LightShadow"),values,visited);return values;};return h;}();result.setHostHooks(hooks);
    }return result;
}
JSValue nativeHarmonics(const std::shared_ptr<SphericalHarmonics3>& sh) {
    if(!sh)return nullptr;auto result=JSValue::hostObject(sh,sh.get(),"three.SphericalHarmonics3");
    if(initialize(result)) {
        auto coefficients=JSValue::array();for(int i=0;i<9;++i)coefficients.set(std::to_string(i),nativeVector(std::shared_ptr<Vector3>(sh,&sh->coefficients[i])));
        result.set("coefficients",coefficients);result.set("isSphericalHarmonics3",true);
        static auto hooks=[]{auto h=std::make_shared<JSHostObjectHooks>();h->nativeOwnership=+[](const JSValue& self){return std::vector<JSNativeOwnerNode>{{payload<SphericalHarmonics3>(self,"three.SphericalHarmonics3"),{},{}}};};return h;}();result.setHostHooks(hooks);
    }return result;
}
JSValue objectRead(const JSValue& self, const Args&, const Args& capture) {
    auto o = payload<Object3D>(self, objectTag);
    const auto& key = capture[0].string();
    if(const auto value=helperRead(o,key))return *value;
    if(key=="matrix"||key=="matrixWorld") {
        auto& matrix=key=="matrix"?o->matrix:o->matrixWorld;
        auto result=nativeMatrix(std::shared_ptr<Matrix4>(o,&matrix));
        if(!matrix.aliasTarget()){result.setHostSlot("nativeObjectOwner",self);result.getHostSlot("elements").setHostSlot("nativeObjectOwner",self);}
        return result;
    }
    if (key == "uuid") return o->uuid;
    if (key == "name") return o->name;
    if (key == "type") return o->type;
    if (key == "id") return double(o->id);
    if (key == "position") return nativeVector(std::shared_ptr<Vector3>(o, &o->position));
    if (key == "scale") return nativeVector(std::shared_ptr<Vector3>(o, &o->scale));
    if (key == "up") return nativeVector(std::shared_ptr<Vector3>(o, &o->up));
    if (key == "quaternion") return nativeQuaternion(std::shared_ptr<Quaternion>(o, &o->quaternion));
    if (key == "rotation") return nativeEuler(std::shared_ptr<Euler>(o, &o->rotation));
    if (key == "layers") return o->layers.object();
    if (key == "children") return nativeSharedArray(o->children);
    if (key == "animations") return nativeAnimationClips(std::shared_ptr<std::vector<std::shared_ptr<AnimationClip>>>(o,&o->animations));
    if (key == "parent") return o->parent ? nativeObject(o->parent->weak_from_this().lock()) : JSValue(nullptr);
    if (key == "isObject3D") return true;
    if (key == "isMesh") return o->isMesh;
    if (key == "isLight") return o->isLight;
    if (key == "isBone") return o->isBone;
    if (key == "isSkinnedMesh") return o->isSkinnedMesh;
    if (key == "isInstancedMesh") return o->isInstancedMesh;
    if (key == "isLine") return o->isLine;
    if (key == "isLineLoop") return o->isLineLoop;
    if (key == "isLineSegments") return o->isLineSegments;
    if (key == "isPoints") return o->isPoints;
    if (key == "isSprite") return o->isSprite;
    if (key == "visible") return o->visible;
    if (key == "matrixWorldNeedsUpdate") return o->matrixWorldNeedsUpdate;
    if (key == "matrixAutoUpdate") return o->matrixAutoUpdate;
    if (key == "matrixWorldAutoUpdate") return o->matrixWorldAutoUpdate;
    if (key == "renderOrder") return o->renderOrder;
    if (key == "castShadow") return o->castShadow;
    if(const auto camera=std::dynamic_pointer_cast<Camera>(o)) {
        if(key=="matrixWorldInverse"||key=="projectionMatrix"||key=="projectionMatrixInverse") {
            auto& matrix=key=="matrixWorldInverse"?camera->matrixWorldInverse:key=="projectionMatrix"?camera->projectionMatrix:camera->projectionMatrixInverse;
            auto value=nativeMatrix(std::shared_ptr<Matrix4>(camera,&matrix));value.setHostSlot("nativeObjectOwner",self);value.getHostSlot("elements").setHostSlot("nativeObjectOwner",self);return value;
        }
        if(key=="coordinateSystem")return camera->coordinateSystem;
        if(key=="reversedDepth")return camera->reversedDepth;
    }
    if(const auto camera=std::dynamic_pointer_cast<PerspectiveCamera>(o)) {if(key=="near")return camera->near;if(key=="far")return camera->far;}
    if(const auto camera=std::dynamic_pointer_cast<OrthographicCamera>(o)) {if(key=="near")return camera->near;if(key=="far")return camera->far;}
    if (const auto lod = std::dynamic_pointer_cast<LOD>(o)) {
        if (key == "isLOD") return lod->isLOD;
        if (key == "autoUpdate") return lod->autoUpdate;
        if (key == "_currentLevel") return lod->_currentLevel;
    }
    if (auto mesh = std::dynamic_pointer_cast<Mesh>(o)) {
        if (key == "count") return mesh->count.asValue();
        if (key == "material") return mesh->materialIsArray || !mesh->materials.empty() ? nativeSharedArray(mesh->materials) : nativeMaterial(mesh->material);
        if (key == "geometry") return nativeGeometry(mesh->geometry);
        if (key == "morphTargetInfluences") return mesh->morphTargetInfluences ? nativeSharedArray(*mesh->morphTargetInfluences) : JSValue();
        if (key == "morphTargetDictionary") return nativeDictionary(mesh->morphTargetDictionary);
    }
    if (auto line = std::dynamic_pointer_cast<Line>(o)) return primitiveRead(line,key);
    if (auto points = std::dynamic_pointer_cast<Points>(o)) return primitiveRead(points,key);
    if (auto sprite = std::dynamic_pointer_cast<Sprite>(o)) {
        if (key == "material") return nativeMaterial(sprite->material);
        if (key == "geometry") return nativeGeometry(sprite->geometry);
        if (key == "center") return nativeVector(std::shared_ptr<Vector2>(sprite,&sprite->center));
        if (key == "count") return sprite->count.asValue();
    }
    if (auto skinned = std::dynamic_pointer_cast<SkinnedMesh>(o)) {
        if (key == "skeleton") return skinned->skeleton ? nativeSkeleton(skinned->skeleton) : JSValue();
        if (key == "bindMode") return skinned->bindMode;
        if (key == "bindMatrix") return nativeMatrix(std::shared_ptr<Matrix4>(skinned,&skinned->bindMatrix));
        if (key == "bindMatrixInverse") return nativeMatrix(std::shared_ptr<Matrix4>(skinned,&skinned->bindMatrixInverse));
    }
    if (auto light = std::dynamic_pointer_cast<Light>(o)) {
        if (key == "color") return nativeColor(std::shared_ptr<Color>(light,&light->color));
        if (key == "intensity") return light->intensity;
    }
    if (auto light = std::dynamic_pointer_cast<HemisphereLight>(o)) {
        if (key == "groundColor") return nativeColor(std::shared_ptr<Color>(light,&light->groundColor));
    }
    if (auto light = std::dynamic_pointer_cast<DirectionalLight>(o)) {
        if (key == "target") return nativeObject(light->target);
        if (key == "shadow") return nativeShadow(light->shadow);
    }
    if (auto light = std::dynamic_pointer_cast<PointLight>(o)) {
        if (key == "shadow") return nativeShadow(light->shadow);
        if (key == "power") return light->getPower();
        if (key == "distance") return light->distance;
        if (key == "decay") return light->decay;
    }
    if (auto light = std::dynamic_pointer_cast<SpotLight>(o)) {
        if (key == "shadow") return nativeShadow(light->shadow);
        if (key == "target") return nativeObject(light->target);
        if (key == "map") return nativeTexture(light->map);
        if (key == "power") return light->getPower();
        if (key == "distance") return light->distance;
        if (key == "angle") return light->angle;
        if (key == "penumbra") return light->penumbra;
        if (key == "decay") return light->decay;
    }
    if (auto light = std::dynamic_pointer_cast<RectAreaLight>(o)) {
        if (key == "power") return light->getPower();
        if (key == "width") return light->width;
        if (key == "height") return light->height;
    }
    if(const auto light=std::dynamic_pointer_cast<LightProbe>(o))if(key=="sh")return nativeHarmonics(light->sh);
    return {};
}
template <class T> SharedArray<T> readSharedArray(const JSValue& value) {
    if (value.isHostObject(arrayTag<T>())) return SharedArray<T>(payload<ArrayStorage<T>>(value, arrayTag<T>()));
    throw JSTypeError("Native array replacement requires a compatible shared native array");
}
template<class T> std::shared_ptr<T> helperObject(const JSValue& value) {
    if(value.isNull()||value.isUndefined())return nullptr;
    const auto object=std::dynamic_pointer_cast<T>(payload<Object3D>(value,objectTag));
    if(!object)throw JSTypeError("Helper reference requires a compatible native object");return object;
}
bool helperWrite(const std::shared_ptr<Object3D>& object,const std::string& key,const JSValue& value) {
    if(const auto h=std::dynamic_pointer_cast<ArrowHelper>(object)){if(key=="line"){h->line=helperObject<Line>(value);return true;}if(key=="cone"){h->cone=helperObject<Mesh>(value);return true;}}
    if(const auto h=std::dynamic_pointer_cast<BoxHelper>(object))if(key=="object"){h->object=helperObject<Object3D>(value);return true;}
    if(const auto h=std::dynamic_pointer_cast<Box3Helper>(object))if(key=="box"){h->box=value.isNull()||value.isUndefined()?nullptr:payload<Box3>(value,"three.Box3");return true;}
    if(const auto h=std::dynamic_pointer_cast<PlaneHelper>(object)){if(key=="plane"){h->plane=value.isNull()||value.isUndefined()?nullptr:payload<Plane>(value,"three.Plane");return true;}if(key=="size"){h->size=value.toNumber();return true;}}
    if(const auto h=std::dynamic_pointer_cast<CameraHelper>(object)){if(key=="camera"){h->camera=helperObject<Camera>(value);return true;}if(key=="pointMap"){h->pointMap=value;return true;}}
    if(const auto h=std::dynamic_pointer_cast<SkeletonHelper>(object)){if(key=="root"){h->root=helperObject<Object3D>(value);return true;}if(key=="bones"){h->bones=readSharedArray<std::shared_ptr<Object3D>>(value);return true;}}
    if(const auto h=std::dynamic_pointer_cast<PointLightHelper>(object)){if(key=="light"){h->light=helperObject<PointLight>(value);return true;}if(key=="color"){h->color=value;return true;}}
    if(const auto h=std::dynamic_pointer_cast<DirectionalLightHelper>(object)){if(key=="light"){h->light=helperObject<DirectionalLight>(value);return true;}if(key=="color"){h->color=value;return true;}if(key=="lightPlane"){h->lightPlane=helperObject<Line>(value);return true;}if(key=="targetLine"){h->targetLine=helperObject<Line>(value);return true;}}
    if(const auto h=std::dynamic_pointer_cast<HemisphereLightHelper>(object)){if(key=="light"){h->light=helperObject<HemisphereLight>(value);return true;}if(key=="color"){h->color=value;return true;}if(key=="material"){h->material=std::dynamic_pointer_cast<MeshBasicMaterial>(payload<Material>(value,materialTag));return true;}}
    if(const auto h=std::dynamic_pointer_cast<SpotLightHelper>(object)){if(key=="light"){h->light=helperObject<SpotLight>(value);return true;}if(key=="color"){h->color=value;return true;}if(key=="cone"){h->cone=helperObject<LineSegments>(value);return true;}}
    if(const auto c=std::dynamic_pointer_cast<ArrayCamera>(object)){if(key=="cameras"){c->cameras=readSharedArray<std::shared_ptr<Camera>>(value);return true;}if(key=="isMultiViewCamera"){c->isMultiViewCamera=value.toBoolean();return true;}}
    if(const auto g=std::dynamic_pointer_cast<ClippingGroup>(object)){if(key=="clippingPlanes"){g->clippingPlanes=readSharedArray<std::shared_ptr<Plane>>(value);return true;}if(key=="enabled"){g->enabled=value;return true;}if(key=="clipIntersection"){g->clipIntersection=value;return true;}if(key=="clipShadows"){g->clipShadows=value;return true;}}
    return false;
}
template <class T> void primitiveWrite(const std::shared_ptr<T>& object,const std::string& key,const JSValue& value) {
    if (key == "geometry") object->geometry = value.isNull() ? nullptr : payload<BufferGeometry>(value,"three.BufferGeometry");
    else if (key == "material") {
        if (value.isHostObject(arrayTag<std::shared_ptr<Material>>())) {
            object->materials=readSharedArray<std::shared_ptr<Material>>(value); object->materialIsArray=true; object->material=nullptr;
        } else {
            object->material=value.isNull()?nullptr:payload<Material>(value,materialTag);object->materials={};object->materialIsArray=false;
        }
    } else if (key == "morphTargetInfluences") {
        if (value.isUndefined()) object->morphTargetInfluences.reset();
        else object->morphTargetInfluences=readSharedArray<double>(value);
    } else if (key == "morphTargetDictionary") object->morphTargetDictionary=value.isUndefined()?nullptr:payload<Dictionary>(value,"three.MorphDictionary");
}
JSValue objectWrite(const JSValue& self, const Args& args, const Args& capture) {
    auto o = payload<Object3D>(self, objectTag);
    const auto key = capture[0].string(); const auto value = argument(args, 0);
    if(helperWrite(o,key,value))return {};
    if (key == "uuid") o->uuid = value.toString();
    else if (key == "name") o->name = value.toString();
    else if (key == "type") o->type = value.toString();
    else if (key == "visible") o->visible = value.toBoolean();
    else if (key == "matrixWorldNeedsUpdate") o->matrixWorldNeedsUpdate = value.toBoolean();
    else if (key == "matrixAutoUpdate") o->matrixAutoUpdate = value.toBoolean();
    else if (key == "matrixWorldAutoUpdate") o->matrixWorldAutoUpdate = value.toBoolean();
    else if (key == "renderOrder") o->renderOrder = value.toNumber();
    else if (key == "castShadow") o->castShadow = value.toBoolean();
    else if (key == "layers") o->layers = Layers::fromObject(value);
    else if (key == "children") o->children = readSharedArray<std::shared_ptr<Object3D>>(value);
    else if (const auto lod = std::dynamic_pointer_cast<LOD>(o)) {
        if (key == "isLOD") lod->isLOD = value.toBoolean();
        else if (key == "autoUpdate") lod->autoUpdate = value;
        else if (key == "_currentLevel") lod->_currentLevel = value;
    }
    else if (auto mesh = std::dynamic_pointer_cast<Mesh>(o)) {
        if (key == "count") mesh->count = value;
        else if (key == "material") {
            if (value.isHostObject(arrayTag<std::shared_ptr<Material>>())) {
                mesh->materials = readSharedArray<std::shared_ptr<Material>>(value); mesh->materialIsArray = true; mesh->material = nullptr;
            } else {
                mesh->material = value.isNull() ? nullptr : payload<Material>(value, materialTag);
                mesh->materials = {}; mesh->materialIsArray = false;
            }
        } else if (key == "geometry") mesh->geometry = value.isNull() ? nullptr : payload<BufferGeometry>(value, "three.BufferGeometry");
        else if (key == "morphTargetInfluences") {
            if (value.isUndefined()) mesh->morphTargetInfluences.reset();
            else mesh->morphTargetInfluences = readSharedArray<double>(value);
        } else if (key == "morphTargetDictionary") mesh->morphTargetDictionary = value.isUndefined() ? nullptr : payload<Dictionary>(value, "three.MorphDictionary");
    }
    if (auto line = std::dynamic_pointer_cast<Line>(o)) primitiveWrite(line,key,value);
    if (auto points = std::dynamic_pointer_cast<Points>(o)) primitiveWrite(points,key,value);
    if (auto sprite = std::dynamic_pointer_cast<Sprite>(o)) {
        if (key == "material") sprite->material=value.isNull()?nullptr:payload<Material>(value,materialTag);
        else if (key == "geometry") sprite->geometry=value.isNull()?nullptr:payload<BufferGeometry>(value,"three.BufferGeometry");
        else if (key == "count") sprite->count=value;
    }
    if (auto skinned = std::dynamic_pointer_cast<SkinnedMesh>(o)) {
        if (key == "bindMode") skinned->bindMode = value.toString();
        else if (key == "skeleton") skinned->skeleton = value.isUndefined() ? nullptr : payload<Skeleton>(value,"three.Skeleton");
    }
    if (auto light = std::dynamic_pointer_cast<Light>(o)) if (key == "intensity") light->intensity = value.toNumber();
    if (auto light = std::dynamic_pointer_cast<DirectionalLight>(o)) if (key == "target") light->target = payload<Object3D>(value,objectTag);
    if(key=="shadow") {
        const auto shadow=value.isNull()?nullptr:payload<LightShadow>(value,"three.LightShadow");
        if(const auto light=std::dynamic_pointer_cast<DirectionalLight>(o)) {const auto typed=std::dynamic_pointer_cast<DirectionalLightShadow>(shadow);if(shadow&&!typed)throw JSTypeError("DirectionalLight shadow requires matching native type");light->shadow=typed;}
        if(const auto light=std::dynamic_pointer_cast<PointLight>(o)) {const auto typed=std::dynamic_pointer_cast<PointLightShadow>(shadow);if(shadow&&!typed)throw JSTypeError("PointLight shadow requires matching native type");light->shadow=typed;}
        if(const auto light=std::dynamic_pointer_cast<SpotLight>(o)) {const auto typed=std::dynamic_pointer_cast<SpotLightShadow>(shadow);if(shadow&&!typed)throw JSTypeError("SpotLight shadow requires matching native type");light->shadow=typed;}
    }
    if (auto light = std::dynamic_pointer_cast<PointLight>(o)) {
        if (key == "power") light->setPower(value.toNumber());
        else if (key == "distance") light->distance = value.toNumber();
        else if (key == "decay") light->decay = value.toNumber();
    }
    if (auto light = std::dynamic_pointer_cast<SpotLight>(o)) {
        if (key == "power") light->setPower(value.toNumber());
        else if (key == "distance") light->distance = value.toNumber();
        else if (key == "angle") light->angle = value.toNumber();
        else if (key == "penumbra") light->penumbra = value.toNumber();
        else if (key == "decay") light->decay = value.toNumber();
        else if (key == "target") light->target = payload<Object3D>(value,objectTag);
        else if (key == "map") light->map = value.isNull() ? nullptr : payload<Texture>(value,"three.Texture");
    }
    if (auto light = std::dynamic_pointer_cast<RectAreaLight>(o)) {
        if (key == "power") light->setPower(value.toNumber());
        else if (key == "width") light->width = value.toNumber();
        else if (key == "height") light->height = value.toNumber();
    }
    return {};
}
JSValue nativeSkeleton(const std::shared_ptr<Skeleton>& skeleton) {
    if (!skeleton) return nullptr;
    auto result = JSValue::hostObject(skeleton,skeleton.get(),"three.Skeleton");
    if (initialize(result)) {
        static auto hooks = [] {
            auto h = std::make_shared<JSHostObjectHooks>(); h->nativeOwnership = skeletonOwners;
            h->trace = [](const JSValue& self) {
                std::vector<JSValue> result; std::unordered_set<const Object3D*> visited;
                traceNativeSkeleton(self,payload<Skeleton>(self,"three.Skeleton"),result,visited);
                return result;
            };
            return h;
        }();
        result.setHostHooks(hooks);
        accessor(result,"bones",[](const JSValue& self,const Args&,const Args&) { return nativeSharedArray(payload<Skeleton>(self,"three.Skeleton")->bones); },
            [](const JSValue& self,const Args& args,const Args&) { payload<Skeleton>(self,"three.Skeleton")->bones = readSharedArray<std::shared_ptr<Bone>>(argument(args,0)); return JSValue(); });
        accessor(result,"boneMatrices",[](const JSValue& self,const Args&,const Args&) { return nativeArray(payload<Skeleton>(self,"three.Skeleton")->boneMatrices); });
        accessor(result,"boneTexture",[](const JSValue& self,const Args&,const Args&) { return nativeTexture(payload<Skeleton>(self,"three.Skeleton")->boneTexture); });
        accessor(result,"uuid",[](const JSValue& self,const Args&,const Args&) { return JSValue(payload<Skeleton>(self,"three.Skeleton")->uuid); });
        static thread_local JSWeakValue cachedPrototype;
        const auto prototype = cachedModuleValue(cachedPrototype, [] {
            auto p = JSValue::object();
            method(p,"getBoneByName",[](const JSValue& self,const Args& args,const Args&) {
                const auto name = argument(args,0);
                if (name.type() != JSValue::Type::String) return JSValue();
                const auto bone = payload<Skeleton>(self,"three.Skeleton")->getBoneByName(name.string());
                return bone ? nativeObject(bone) : JSValue();
            },"three.Skeleton");
            return p;
        });
        result.setPrototype(prototype);
        installNativeSkeletonRenderingBridge(result,skeleton);
    }
    return result;
}
JSValue nativeJSON(const JsonValue& value);
JsonValue jsonFieldValue(const JSValue& value) {
    if (value.isHostObject("three.JSON")) return payload<JsonValue>(value,"three.JSON")->sharedCopy();
    if (value.isObject() || value.isUndefined()) throw JSTypeError("Native JSON fields require JSON values; arbitrary runtime objects need managed storage");
    return value.toJSON().value_or(JsonValue());
}
JSHostPropertyResult jsonOwn(const JSValue& self,const std::string& key) {
    const auto value = payload<JsonValue>(self,"three.JSON");
    if (value->isObject()) return {true,value->contains(key) ? std::optional(dataDescriptor(nativeJSON(value->at(key)))) : std::nullopt};
    if (key == "length") return {true,dataDescriptor(double(value->size()),true,false,false)};
    const auto index = arrayIndex(key);
    if (!index) return {};
    return {true,*index < value->size() ? std::optional(dataDescriptor(nativeJSON((*value)[*index]))) : std::nullopt};
}
std::optional<bool> jsonDefine(const JSValue& self,const std::string& key,const JSPropertyDescriptor& descriptor) {
    const auto value = payload<JsonValue>(self,"three.JSON");
    const bool length = value->isArray() && key == "length";
    const bool present = value->isObject() ? value->contains(key) : length || (arrayIndex(key) && *arrayIndex(key) < value->size());
    if (!present && !self.isExtensible()) return false;
    if (descriptor.getter || descriptor.setter || !descriptor.writable.value_or(present) ||
        descriptor.enumerable.value_or(present && !length) == length || descriptor.configurable.value_or(present && !length) == length)
        throw JSTypeError("Native JSON fields require ordinary writable data descriptors");
    if (!descriptor.value) return present ? std::optional(true) : std::optional(false);
    if (value->isObject()) (*value)[key] = jsonFieldValue(*descriptor.value);
    else if (length) {
        const auto size = descriptor.value->toNumber();
        if (!std::isfinite(size) || size < 0 || std::trunc(size) != size || size > UINT32_MAX) throw JSRangeError("Invalid array length");
        if (size > double(value->size())) throw JSTypeError("Native JSON arrays cannot represent holes");
        value->get<JsonValue::Array>().resize(std::size_t(size));
    } else {
        const auto index = arrayIndex(key);
        if (!index || *index > value->size()) throw JSTypeError("Native JSON arrays cannot represent sparse or named entries");
        auto incoming = jsonFieldValue(*descriptor.value);
        if (*index == value->size()) value->get<JsonValue::Array>().push_back(std::move(incoming));
        else (*value)[*index] = std::move(incoming);
    }
    return true;
}
std::optional<bool> jsonErase(const JSValue& self,const std::string& key) {
    const auto value = payload<JsonValue>(self,"three.JSON");
    if (value->isObject()) { value->get<JsonValue::Object>().erase(key); return true; }
    if (key == "length") return false;
    if (const auto index = arrayIndex(key); index && *index < value->size()) throw JSTypeError("Native JSON arrays cannot represent holes");
    return true;
}
std::vector<std::string> jsonNames(const JSValue& self) {
    const auto value = payload<JsonValue>(self,"three.JSON"); std::vector<std::string> names;
    if (value->isObject()) for (const auto& [key,entry] : value->get<JsonValue::Object>()) names.push_back(key);
    else { for (std::size_t i = 0; i < value->size(); ++i) names.push_back(std::to_string(i)); names.push_back("length"); }
    return names;
}
JSValue nativeJSON(const JsonValue& value) {
    if (!value.isObject() && !value.isArray()) return JSValue::fromJSON(value);
    auto result = JSValue::hostObject(std::make_shared<JsonValue>(value.sharedCopy()),&value.storage(),"three.JSON",
                                    value.isArray() ? JSValue::Type::Array : JSValue::Type::Object);
    if (initialize(result)) {
        static auto hooks = std::make_shared<JSHostObjectHooks>(JSHostObjectHooks{jsonOwn,jsonDefine,jsonErase,jsonNames});
        result.setHostHooks(hooks);
    }
    return result;
}
JSValue materialRead(const JSValue& self, const Args&, const Args& capture) {
    const auto material = payload<Material>(self,materialTag);
    const auto key = capture[0].string();
    if (key == "id") return double(material->id);
    if (key == "isMaterial") return true;
    if (key == "needsUpdate") return {};
    const auto property = material->property(key);
    if (!property) return material->extraProperties.get(key,self);
    return std::visit([&](auto* field) -> JSValue {
        using T = std::remove_cvref_t<decltype(*field)>;
        if constexpr (std::is_same_v<T,Color>) return nativeColor(std::shared_ptr<Color>(material,field));
        else if constexpr (std::is_same_v<T,Vector2>) return nativeVector(std::shared_ptr<Vector2>(material,field));
        else if constexpr (std::is_same_v<T,Euler>) return nativeEuler(std::shared_ptr<Euler>(material,field));
        else if constexpr (std::is_same_v<T,std::shared_ptr<Texture>>) return nativeTexture(*field);
        else if constexpr (std::is_same_v<T,Material::AlphaTest> || std::is_same_v<T,Material::Reflectivity>) return double(*field);
        else if constexpr (std::is_same_v<T,std::optional<int>> || std::is_same_v<T,std::optional<std::string>>) return *field ? JSValue(**field) : JSValue(nullptr);
        else if constexpr (std::is_same_v<T,JsonValue>) {
            return nativeJSON(*field);
        } else if constexpr (std::is_same_v<T,Material::ClippingPlanes>) {
            if (!*field) return nullptr;
            return nativeSharedArray(**field);
        } else return *field;
    },*property);
}
JSValue materialWrite(const JSValue& self, const Args& args, const Args& capture) {
    const auto material = payload<Material>(self,materialTag); const auto key = capture[0].string(); const auto value = argument(args,0);
    if (key == "needsUpdate") { material->needsUpdate = value; return {}; }
    const auto property = material->property(key);
    if (!property) { material->extraProperties.set(key,value); return {}; }
    std::visit([&](auto* field) {
        using T = std::remove_cvref_t<decltype(*field)>;
        if constexpr (std::is_same_v<T,Color> || std::is_same_v<T,Vector2> || std::is_same_v<T,Euler>) {
            if (!materialRead(self,{},capture).sameValue(value))
                throw JSTypeError("Replacing a native material math object requires shared field storage");
        } else if constexpr (std::is_same_v<T,std::shared_ptr<Texture>>) *field = value.isNull() ? nullptr : payload<Texture>(value,"three.Texture");
        else if constexpr (std::is_same_v<T,bool>) {
            if (value.type() != JSValue::Type::Boolean) throw JSTypeError("Native material boolean field requires a Boolean");
            *field = value.toBoolean();
        } else if constexpr (std::is_same_v<T,int> || std::is_same_v<T,unsigned>) {
            const auto number = value.toNumber();
            if (!std::isfinite(number) || std::trunc(number) != number || number < double(std::numeric_limits<T>::lowest()) || number > double(std::numeric_limits<T>::max()))
                throw JSTypeError("Native material enum field requires a representable integer");
            *field = static_cast<T>(number);
        } else if constexpr (std::is_same_v<T,std::optional<int>>) {
            if (value.isNull()) field->reset();
            else {
                const auto number = value.toNumber();
                if (!std::isfinite(number) || std::trunc(number) != number || number < INT_MIN || number > INT_MAX)
                    throw JSTypeError("Native material enum field requires a representable integer");
                *field = int(number);
            }
        } else if constexpr (std::is_same_v<T,std::optional<std::string>>) *field = value.isNull() ? std::nullopt : std::optional(value.toString());
        else if constexpr (std::is_same_v<T,std::string>) *field = value.toString();
        else if constexpr (std::is_same_v<T,JSValue>) *field = value;
        else if constexpr (std::is_same_v<T,JsonValue>) *field = jsonFieldValue(value);
        else if constexpr (std::is_same_v<T,Material::ClippingPlanes>) {
            if (value.isNull()) field->reset();
            else *field = readSharedArray<std::shared_ptr<Plane>>(value);
        } else *field = value.toNumber();
    },*property);
    return {};
}
bool materialAccessor(const Material::PropertyReference& property) {
    return std::holds_alternative<Material::AlphaTest*>(property) || std::holds_alternative<Material::Reflectivity*>(property);
}
JSHostPropertyResult materialOwn(const JSValue& self,const std::string& key) {
    const auto material = payload<Material>(self,materialTag);
    if (key == "id") return {true,dataDescriptor(double(material->id),false,false,false)};
    if (key == "isMaterial") return {true,dataDescriptor(true)};
    if (const auto property = material->property(key)) {
        if (materialAccessor(*property)) return {};
        return {true,dataDescriptor(materialRead(self,{},{key}))};
    }
    const auto descriptor = material->extraProperties.getOwnPropertyDescriptor(key);
    return descriptor ? JSHostPropertyResult{true,descriptor} : JSHostPropertyResult{};
}
std::optional<bool> materialDefine(const JSValue& self,const std::string& key,const JSPropertyDescriptor& descriptor) {
    const auto material = payload<Material>(self,materialTag);
    if (key == "id" || key == "isMaterial") {
        const auto current = key == "id" ? JSValue(double(material->id)) : JSValue(true);
        if (descriptor.value && !descriptor.value->sameValue(current)) return false;
        return !descriptor.getter && !descriptor.setter && (key != "id" || (!descriptor.writable.value_or(false) && !descriptor.enumerable.value_or(false) && !descriptor.configurable.value_or(false)));
    }
    if (const auto property = material->property(key)) {
        if (materialAccessor(*property)) return {};
        if (descriptor.getter || descriptor.setter || !descriptor.writable.value_or(true) || !descriptor.enumerable.value_or(true) || !descriptor.configurable.value_or(true))
            throw JSTypeError("Native material fields currently require writable enumerable configurable data descriptors");
        if (descriptor.value) materialWrite(self,{*descriptor.value},{key});
        return true;
    }
    if (!material->extraProperties.hasOwn(key) && !self.isExtensible()) return false;
    return material->extraProperties.defineOwnProperty(key,descriptor);
}
std::optional<bool> materialErase(const JSValue& self,const std::string& key) {
    const auto material = payload<Material>(self,materialTag);
    if (key == "id") return false;
    if (key == "isMaterial" || material->property(key)) throw JSTypeError("Native material fields cannot currently be deleted");
    return material->extraProperties.erase(key);
}
std::vector<std::string> materialNames(const JSValue& self) {
    const auto material = payload<Material>(self,materialTag);
    std::vector<std::string> names{"isMaterial","id"};
    material->visitProperties([&](std::string_view key,Material::PropertyReference property) { if (!std::holds_alternative<Material::AlphaTest*>(property)) names.emplace_back(key); });
    for (const auto& key : material->extraProperties.ownPropertyNames()) if (!material->property(key)) names.push_back(key);
    return names;
}
JSValue materialSetValues(const JSValue& self,const Args& args,const Args&) {
    const auto values = argument(args,0);
    if (values.isUndefined() || values.isNull()) return {};
    std::vector<std::string> keys; std::unordered_set<std::string> seen;
    for (auto current = values; current.isObject(); current = current.getPrototype())
        for (const auto& key : current.ownPropertyNames()) if (seen.insert(key).second) {
            const auto descriptor = current.getOwnPropertyDescriptor(key);
            if (descriptor && descriptor->enumerable.value_or(false)) keys.push_back(key);
        }
    for (const auto& key : keys) {
        if (!values.has(key)) continue;
        const auto value = values.get(key);
        if (value.isUndefined()) { std::cerr << "THREE.Material: parameter '" << key << "' has value of undefined.\n"; continue; }
        const auto current = self.get(key);
        if (current.isUndefined()) { std::cerr << "THREE.Material: '" << key << "' is not a property of THREE." << self.get("type").toString() << ".\n"; continue; }
        if (current.toBoolean() && current.get("isColor").toBoolean()) current.get("set").call(current,{value});
        else if ((current.toBoolean() && current.get("isVector2").toBoolean() && value.toBoolean() && value.get("isVector2").toBoolean()) ||
                 (current.toBoolean() && current.get("isEuler").toBoolean() && value.toBoolean() && value.get("isEuler").toBoolean()) ||
                 (current.toBoolean() && current.get("isVector3").toBoolean() && value.toBoolean() && value.get("isVector3").toBoolean()))
            current.get("copy").call(current,{value});
        else { auto target = self; target.set(key,value); }
    }
    return {};
}
JSValue textureRead(const JSValue& self, const Args&, const Args& capture) {
    auto t = payload<Texture>(self, "three.Texture"); const auto& key = capture[0].string();
    if (key == "offset") return nativeVector(std::shared_ptr<Vector2>(t, &t->offset));
    if (key == "repeat") return nativeVector(std::shared_ptr<Vector2>(t, &t->repeat));
    if (key == "center") return nativeVector(std::shared_ptr<Vector2>(t, &t->center));
    if (key == "matrix") {
        auto matrix=nativeMatrix(std::shared_ptr<Matrix3>(t,&t->matrix));
        matrix.setHostSlot("nativeTextureOwner",self);
        matrix.getHostSlot("elements").setHostSlot("nativeTextureOwner",self);
        return matrix;
    }
    if (key == "colorSpace") return t->colorSpace;
    if (key == "rotation") return t->rotation;
    if (key == "uuid") return t->uuid;
    if (key == "name") return t->name;
    if (key == "version") return t->version;
    if (key == "isTexture") return true;
    if (key == "isRenderTargetTexture") return t->isRenderTargetTexture;
    if (key == "matrixAutoUpdate") return t->matrixAutoUpdate;
    if (key == "wrapS") return t->wrapS;
    if (key == "wrapT") return t->wrapT;
    if (key == "channel") return t->channel;
    if (key == "format") return t->format;
    if (key == "magFilter") return t->magFilter;
    if (key == "minFilter") return t->minFilter;
    if (key == "anisotropy") return t->anisotropy;
    if (key == "type") return t->type;
    if (key == "flipY") return t->flipY;
    if (key == "generateMipmaps") return t->generateMipmaps;
    if (key == "mapping") return t->mapping;
    if (key == "isDataArrayTexture") return t->isDataArrayTexture;
    if (key == "isData3DTexture") return t->isData3DTexture;
    if (key == "isCubeTexture") return t->isCubeTexture;
    if (key == "isDepthTexture") return t->isDepthTexture;
    if (key == "isCubeDepthTexture") return t->isCubeDepthTexture;
    if (key == "compareFunction") {const auto depth=std::dynamic_pointer_cast<DepthTexture>(t);if(!depth)throw JSTypeError("DepthTexture receiver required");return depth->compareFunction?JSValue(*depth->compareFunction):JSValue(nullptr);}

    return {};
}
JSValue textureWrite(const JSValue& self, const Args& args, const Args& capture) {
    auto t = payload<Texture>(self, "three.Texture"); const auto& key = capture[0].string(); const auto value = argument(args, 0);
    if (key == "rotation") t->rotation = value.toNumber();
    else if (key == "colorSpace") t->colorSpace = value.toString();
    else if (key == "uuid") t->uuid = value.toString();
    else if (key == "name") t->name = value.toString();
    else if (key == "version") t->version = value.toNumber();
    else if (key == "matrixAutoUpdate") t->matrixAutoUpdate = value.toBoolean();
    else if (key == "wrapS") t->wrapS = value.toNumber();
    else if (key == "magFilter") t->magFilter = value.toNumber();
    else if (key == "minFilter") t->minFilter = value.toNumber();
    else if (key == "anisotropy") t->anisotropy = value.toNumber();
    else if (key == "type") t->type = value.toNumber();
    else if (key == "flipY") t->flipY = value.toBoolean();
    else if (key == "generateMipmaps") t->generateMipmaps = value.toBoolean();
    else if (key == "mapping") t->mapping = value.toNumber();
    else if (key == "wrapT") t->wrapT = value.toNumber();
    else if (key == "channel") t->channel = value.toNumber();
    else if (key == "format") t->format = value.toNumber();
    else if (key == "isDataArrayTexture") t->isDataArrayTexture = value.toBoolean();
    else if (key == "isData3DTexture") t->isData3DTexture = value.toBoolean();
    else if (key == "isCubeTexture") t->isCubeTexture = value.toBoolean();
    else if (key == "isDepthTexture") t->isDepthTexture = value.toBoolean();
    else if (key == "isCubeDepthTexture") t->isCubeDepthTexture = value.toBoolean();
    else if (key == "compareFunction") {const auto depth=std::dynamic_pointer_cast<DepthTexture>(t);if(!depth)throw JSTypeError("DepthTexture receiver required");depth->compareFunction=value.isNull()?std::nullopt:std::optional<double>(value.toNumber());}

    else if (key == "needsUpdate") t->needsUpdate = value;
    return {};
}
void traceNativeTexture(const JSValue& self, const std::shared_ptr<Texture>& texture, std::vector<JSValue>& result) {
    if (!texture) return;
    cachedEdge(self, result, texture.get(), "three.Texture");
    for (const auto* vector : {&texture->offset, &texture->repeat, &texture->center}) cachedEdge(self, result, vector, "three.Vector2");
    cachedEdge(self,result,&texture->matrix,"three.Matrix3");
    cachedEdge(self,result,texture->matrix.elements().data(),"three.MatrixElements9",JSValue::Type::Array);
}
void traceNativeMaterial(const JSValue& self, const std::shared_ptr<Material>& material, std::vector<JSValue>& result) {
    if (!material) return;
    cachedEdge(self, result, material.get(), materialTag);
    if (material->clippingPlanes) {
        cachedEdge(self,result,material->clippingPlanes->storage().get(),
                   arrayTag<std::shared_ptr<Plane>>(),JSValue::Type::Array);
        for (const auto& plane : *material->clippingPlanes) if (plane) {
            cachedEdge(self,result,plane.get(),"three.Plane");
            cachedEdge(self,result,&plane->normal,"three.Vector3");
        }
    }
    if (const auto basic = std::dynamic_pointer_cast<MeshBasicMaterial>(material)) {
        cachedEdge(self, result, &basic->color, "three.Color");
        for (const auto& texture : {basic->map,basic->alphaMap,basic->lightMap,basic->aoMap,basic->specularMap,basic->envMap}) traceNativeTexture(self, texture, result);
    }
}
void traceNativeAttribute(const JSValue& self, const std::shared_ptr<VertexAttribute>& attribute, std::vector<JSValue>& result) {
    if (!attribute) return;
    if(const auto root=attribute->managedObjectRoot()) {result.push_back(*root);return;}
    if(const auto ordinary=std::dynamic_pointer_cast<BufferAttribute>(attribute)) {
        cachedEdge(self,result,ordinary.get(),"three.BufferAttribute");
        std::visit([&](const auto& array) { cachedEdge(self,result,array.identity(),typedTag); },ordinary->array);
    }
}
void traceNativeGeometry(const JSValue& self, const std::shared_ptr<BufferGeometry>& geometry, std::vector<JSValue>& result) {
    if (!geometry) return;
    cachedEdge(self, result, geometry.get(), "three.BufferGeometry");
    if(geometry->animations){cachedEdge(self,result,geometry->animations.get(),"three.AnimationClipArray",JSValue::Type::Array);
        for(const auto& clip:*geometry->animations)if(clip)cachedEdge(self,result,clip.get(),"three.AnimationClip");}
    const auto attribute = [&](const std::shared_ptr<VertexAttribute>& a) { traceNativeAttribute(self,a,result); };
    attribute(geometry->index);
    for (const auto& [name, a] : geometry->attributes) attribute(a);
    for (const auto& [name, list] : geometry->morphAttributes) for (const auto& a : list) attribute(a);
}
void traceNativeSkeleton(const JSValue& self,const std::shared_ptr<Skeleton>& skeleton,
                         std::vector<JSValue>& result,std::unordered_set<const Object3D*>& visited) {
    if (!skeleton) return;
    cachedEdge(self,result,skeleton.get(),"three.Skeleton");
    cachedEdge(self,result,skeleton->bones.storage().get(),arrayTag<std::shared_ptr<Bone>>(),JSValue::Type::Array);
    cachedEdge(self,result,skeleton->boneMatrices.identity(),typedTag);
    traceNativeTexture(self,skeleton->boneTexture,result);
    for (const auto& bone : skeleton->bones) traceNativeObject(self,bone,result,visited);
}
template <class T> void traceNativePrimitive(const JSValue& self,const std::shared_ptr<T>& object,std::vector<JSValue>& result) {
    traceNativeMaterial(self,object->material,result);traceNativeGeometry(self,object->geometry,result);
    cachedEdge(self,result,object->materials.storage().get(),arrayTag<std::shared_ptr<Material>>(),JSValue::Type::Array);
    for (const auto& material : object->materials) traceNativeMaterial(self,material,result);
    if (object->morphTargetInfluences) cachedEdge(self,result,object->morphTargetInfluences->storage().get(),arrayTag<double>(),JSValue::Type::Array);
    if (object->morphTargetDictionary) cachedEdge(self,result,object->morphTargetDictionary.get(),"three.MorphDictionary");
}
void traceShadow(const JSValue& self,const std::shared_ptr<LightShadow>& shadow,std::vector<JSValue>& result,std::unordered_set<const Object3D*>& visited) {
    if(!shadow)return;cachedEdge(self,result,shadow.get(),"three.LightShadow");
    cachedEdge(self,result,&shadow->mapSize,"three.Vector2");cachedEdge(self,result,&shadow->_frameExtents,"three.Vector2");cachedEdge(self,result,&shadow->matrix,"three.Matrix4");cachedEdge(self,result,shadow->matrix.elements().data(),"three.MatrixElements16",JSValue::Type::Array);
    traceNativeObject(self,shadow->camera,result,visited);
    for(const auto& target:{shadow->map,shadow->mapPass})if(target){cachedEdge(self,result,target.get(),"three.RenderTarget");for(const auto& texture:target->textures)traceNativeTexture(self,texture,result);traceNativeTexture(self,target->_depthTexture,result);}
}
void traceNativeObject(const JSValue& self, const std::shared_ptr<Object3D>& object,
                       std::vector<JSValue>& result, std::unordered_set<const Object3D*>& visited) {
    if (!object || !visited.insert(object.get()).second) return;
    cachedEdge(self, result, object.get(), objectTag);
    result.push_back(*object->layers.runtimeRoot());
    for(const auto* matrix:{&object->matrix,&object->matrixWorld,&object->modelViewMatrix}) {
        auto resolved=matrix;
        while(resolved->aliasTarget()){if(const auto owner=resolved->aliasObjectOwner().lock())traceNativeObject(self,owner,result,visited);resolved=resolved->aliasTarget().get();}
        cachedEdge(self,result,resolved,"three.Matrix4");cachedEdge(self,result,resolved->elements().data(),"three.MatrixElements16",JSValue::Type::Array);
    }
    if(const auto camera=std::dynamic_pointer_cast<Camera>(object))for(const auto* matrix:{&camera->matrixWorldInverse,&camera->projectionMatrix,&camera->projectionMatrixInverse}){cachedEdge(self,result,matrix,"three.Matrix4");cachedEdge(self,result,matrix->elements().data(),"three.MatrixElements16",JSValue::Type::Array);}
    cachedEdge(self,result,&object->animations,"three.AnimationClipArray",JSValue::Type::Array);
    for(const auto& clip:object->animations)if(clip)cachedEdge(self,result,clip.get(),"three.AnimationClip");
    for (const auto* vector : {&object->position, &object->scale, &object->up}) cachedEdge(self, result, vector, "three.Vector3");
    cachedEdge(self, result, &object->quaternion, "three.Quaternion"); cachedEdge(self, result, &object->rotation, "three.Euler");
    cachedEdge(self, result, object->children.storage().get(), arrayTag<std::shared_ptr<Object3D>>(), JSValue::Type::Array);
    if (object->parent) traceNativeObject(self, object->parent->weak_from_this().lock(), result, visited);
    for (const auto& child : object->children) traceNativeObject(self, child, result, visited);
    if (const auto mesh = std::dynamic_pointer_cast<Mesh>(object)) {
        traceNativeMaterial(self, mesh->material, result); traceNativeGeometry(self, mesh->geometry, result);
        cachedEdge(self, result, mesh->materials.storage().get(), arrayTag<std::shared_ptr<Material>>(), JSValue::Type::Array);
        for (const auto& material : mesh->materials) traceNativeMaterial(self, material, result);
        if (mesh->morphTargetInfluences) cachedEdge(self, result, mesh->morphTargetInfluences->storage().get(), arrayTag<double>(), JSValue::Type::Array);
        if (mesh->morphTargetDictionary) cachedEdge(self, result, mesh->morphTargetDictionary.get(), "three.MorphDictionary");
    }
    if (const auto line = std::dynamic_pointer_cast<Line>(object)) traceNativePrimitive(self,line,result);
    if (const auto points = std::dynamic_pointer_cast<Points>(object)) traceNativePrimitive(self,points,result);
    if (const auto sprite = std::dynamic_pointer_cast<Sprite>(object)) {
        traceNativeMaterial(self,sprite->material,result);traceNativeGeometry(self,sprite->geometry,result);
        cachedEdge(self,result,&sprite->center,"three.Vector2");
    }
    if (const auto skinned = std::dynamic_pointer_cast<SkinnedMesh>(object)) traceNativeSkeleton(self,skinned->skeleton,result,visited);
    if (const auto instanced = std::dynamic_pointer_cast<InstancedMesh>(object)) {
        traceNativeAttribute(self,instanced->instanceMatrix,result);
        traceNativeAttribute(self,instanced->instanceColor,result);
        traceNativeTexture(self,instanced->morphTexture,result);
    }
    const auto child=[&](const std::shared_ptr<Object3D>& value){traceNativeObject(self,value,result,visited);};
    if(const auto light=std::dynamic_pointer_cast<Light>(object))cachedEdge(self,result,&light->color,"three.Color");
    if(const auto light=std::dynamic_pointer_cast<HemisphereLight>(object))cachedEdge(self,result,&light->groundColor,"three.Color");
    if(const auto light=std::dynamic_pointer_cast<DirectionalLight>(object)){child(light->target);traceShadow(self,light->shadow,result,visited);}
    if(const auto light=std::dynamic_pointer_cast<PointLight>(object))traceShadow(self,light->shadow,result,visited);
    if(const auto light=std::dynamic_pointer_cast<SpotLight>(object)){child(light->target);traceShadow(self,light->shadow,result,visited);traceNativeTexture(self,light->map,result);}
    if(const auto light=std::dynamic_pointer_cast<LightProbe>(object))if(light->sh){cachedEdge(self,result,light->sh.get(),"three.SphericalHarmonics3");for(const auto& coefficient:light->sh->coefficients)cachedEdge(self,result,&coefficient,"three.Vector3");}
    if(const auto h=std::dynamic_pointer_cast<ArrowHelper>(object)){child(h->line);child(h->cone);}
    if(const auto h=std::dynamic_pointer_cast<BoxHelper>(object))child(h->object);
    if(const auto h=std::dynamic_pointer_cast<Box3Helper>(object))if(h->box){cachedEdge(self,result,h->box.get(),"three.Box3");cachedEdge(self,result,&h->box->min,"three.Vector3");cachedEdge(self,result,&h->box->max,"three.Vector3");}
    if(const auto h=std::dynamic_pointer_cast<PlaneHelper>(object))if(h->plane){cachedEdge(self,result,h->plane.get(),"three.Plane");cachedEdge(self,result,&h->plane->normal,"three.Vector3");}
    if(const auto h=std::dynamic_pointer_cast<CameraHelper>(object))child(h->camera);
    if(const auto h=std::dynamic_pointer_cast<SkeletonHelper>(object)){child(h->root);cachedEdge(self,result,h->bones.storage().get(),arrayTag<std::shared_ptr<Object3D>>(),JSValue::Type::Array);for(const auto& bone:h->bones)child(bone);}
    if(const auto h=std::dynamic_pointer_cast<PointLightHelper>(object))child(h->light);
    if(const auto h=std::dynamic_pointer_cast<DirectionalLightHelper>(object)){child(h->light);child(h->lightPlane);child(h->targetLine);}
    if(const auto h=std::dynamic_pointer_cast<HemisphereLightHelper>(object)){child(h->light);traceNativeMaterial(self,h->material,result);}
    if(const auto h=std::dynamic_pointer_cast<SpotLightHelper>(object)){child(h->light);child(h->cone);}
    if(const auto c=std::dynamic_pointer_cast<ArrayCamera>(object)){cachedEdge(self,result,c->cameras.storage().get(),arrayTag<std::shared_ptr<Camera>>(),JSValue::Type::Array);for(const auto& camera:c->cameras)child(camera);}
    if(const auto g=std::dynamic_pointer_cast<ClippingGroup>(object)){cachedEdge(self,result,g->clippingPlanes.storage().get(),arrayTag<std::shared_ptr<Plane>>(),JSValue::Type::Array);for(const auto& plane:g->clippingPlanes)if(plane){cachedEdge(self,result,plane.get(),"three.Plane");cachedEdge(self,result,&plane->normal,"three.Vector3");}}
    if(const auto extension=std::dynamic_pointer_cast<NativeObjectExtension>(object))extension->visitNativeObjectReferences(child);
}
// Describe actual native owning slots, independently of which wrappers have
// already been created. A wrapper's JS properties may point back to its owner.
struct NativeOwners {
    std::vector<JSNativeOwnerNode> nodes;
    std::unordered_set<const void*> visited;
    template <class T> bool enter(const std::shared_ptr<T>& value) { return value && visited.insert(value.get()).second; }
    template <class T> void edge(JSNativeOwnerNode& node, const std::shared_ptr<T>& value) {
        if (value) node.edges.push_back({&value,value});
    }
    template <class T> void typedArray(JSNativeOwnerNode& node, const T& value) {
        auto ownership = nativeArrayOwnership(value);
        node.edges.insert(node.edges.end(),ownership.edges.begin(),ownership.edges.end());
        nodes.insert(nodes.end(),std::make_move_iterator(ownership.nodes.begin()),std::make_move_iterator(ownership.nodes.end()));
    }
    void clips(const std::shared_ptr<void>& owner, const std::vector<std::shared_ptr<AnimationClip>>& values) {
        auto additions = nativeAnimationClipOwners(owner,values);
        nodes.insert(nodes.end(),std::make_move_iterator(additions.begin()),std::make_move_iterator(additions.end()));
    }
    void image(const TextureImage::Ptr& value) {
        if (!enter(value)) return;
        JSNativeOwnerNode node{value,{},{}};
        std::visit([&](const auto& data) {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<T,JSValue>) node.roots.push_back(&data);
            else if constexpr (!std::is_same_v<T,std::nullptr_t>) typedArray(node,data);
        },value->data);
        for (const auto& face : value->faces) { edge(node,face); image(face); }
        edge(node,value->texture); texture(value->texture);
        nodes.push_back(std::move(node));
    }
    void texture(const std::shared_ptr<Texture>& value) {
        if (!enter(value)) return;
        JSNativeOwnerNode node{value,{&value->userData},{}};
        edge(node,value->source);
        if (enter(value->source)) {
            JSNativeOwnerNode source{value->source,{},{}};
            edge(source,value->source->data); image(value->source->data); nodes.push_back(std::move(source));
        }
        edge(node,value->mipmaps);
        if (enter(value->mipmaps)) {
            JSNativeOwnerNode mipmaps{value->mipmaps,{},{}};
            for (const auto& mipmap : *value->mipmaps) { edge(mipmaps,mipmap); image(mipmap); }
            nodes.push_back(std::move(mipmaps));
        }
        nodes.push_back(std::move(node));
    }
    void material(const std::shared_ptr<Material>& value) {
        if (!enter(value)) return;
        JSNativeOwnerNode node{value,{&value->extraProperties},{}};
        value->visitProperties([&](std::string_view,Material::PropertyReference property) {
            std::visit([&](auto* field) {
                using T = std::remove_cvref_t<decltype(*field)>;
                if constexpr (std::is_same_v<T,std::shared_ptr<Texture>>) { edge(node,*field); texture(*field); }
                else if constexpr (std::is_same_v<T,JSValue>) node.roots.push_back(field);
                else if constexpr (std::is_same_v<T,Material::ClippingPlanes>) {
                    if (*field) {
                        const auto storage = (*field)->storage();
                        node.edges.push_back({&**field,storage});
                        array<std::shared_ptr<Plane>>(storage);
                    }
                }
            },property);
        });
        nodes.push_back(std::move(node));
    }
    void attributeStorage(const std::shared_ptr<BufferAttributeStorage>& value) {
        if(!enter(value)) return;
        JSNativeOwnerNode node{value,{&value->meshPerAttribute},{}};
        std::visit([&](const auto& array){typedArray(node,array);},value->array);
        nodes.push_back(std::move(node));
    }
    void attribute(const std::shared_ptr<VertexAttribute>& value) {
        if (!enter(value)) return;
        JSNativeOwnerNode node{value,{},{}};
        if(const auto root=value->managedObjectRoot()) node.roots.push_back(root);
        if(const auto ordinary=std::dynamic_pointer_cast<BufferAttribute>(value)) {
            const auto& storage=ordinary->runtimeStorage();
            node.edges.push_back({&storage,storage});attributeStorage(storage);
        }
        nodes.push_back(std::move(node));
    }
    void target(const std::shared_ptr<RenderTarget>& value) {
        if (!enter(value)) return;
        JSNativeOwnerNode node{value,{},{}};
        const auto storage = value->textures.storage();
        node.edges.push_back({&value->textures,storage});
        if (enter(storage)) {
            JSNativeOwnerNode entries{storage,{},{}};
            for (const auto& entry : storage->values) { edge(entries,entry); texture(entry); }
            nodes.push_back(std::move(entries));
        }
        edge(node,value->_depthTexture); texture(value->_depthTexture); nodes.push_back(std::move(node));
    }
    template <class S> void shadow(const std::shared_ptr<S>& value) {
        if (!enter(value)) return;
        JSNativeOwnerNode node{value,{&value->biasNode},{}};
        edge(node,value->camera); object(value->camera);
        edge(node,value->map); target(value->map); edge(node,value->mapPass); target(value->mapPass);
        nodes.push_back(std::move(node));
    }
    void skeleton(const std::shared_ptr<Skeleton>& value) {
        if (!enter(value)) return;
        JSNativeOwnerNode node{value,{},{}};
        typedArray(node,value->boneMatrices);
        const auto bones = value->bones.storage(); node.edges.push_back({&value->bones,bones}); array<std::shared_ptr<Bone>>(bones);
        const auto inverses = value->boneInverses.storage(); node.edges.push_back({&value->boneInverses,inverses});
        if (enter(inverses)) {
            JSNativeOwnerNode entries{inverses,{},{}};
            for (const auto& entry : inverses->values) { edge(entries,entry); if (enter(entry)) nodes.push_back({entry,{},{}}); }
            nodes.push_back(std::move(entries));
        }
        edge(node,value->boneTexture); texture(value->boneTexture);
        nodes.push_back(std::move(node));
    }
    void geometry(const std::shared_ptr<BufferGeometry>& value) {
        if (!enter(value)) return;
        JSNativeOwnerNode node{value,{},{}};
        if(const auto instanced=std::dynamic_pointer_cast<InstancedBufferGeometry>(value))node.roots.push_back(&instanced->instanceCount);
        edge(node,value->index); attribute(value->index);
        for (const auto& [name,a] : value->attributes) { edge(node,a); attribute(a); }
        for (const auto& [name,list] : value->morphAttributes)
            for (const auto& a : list) { edge(node,a); attribute(a); }
        edge(node,value->animations);
        if (value->animations) clips(value->animations,*value->animations);
        nodes.push_back(std::move(node));
    }
    template <class T> void array(const std::shared_ptr<ArrayStorage<T>>& value) {
        if (!enter(value)) return;
        JSNativeOwnerNode node{value,{},{}};
        if constexpr (!std::is_same_v<T,double>) for (const auto& entry : value->values) {
            edge(node,entry);
            if constexpr (std::is_same_v<T,std::shared_ptr<Object3D>> || std::is_same_v<T,std::shared_ptr<Bone>> || std::is_same_v<T,std::shared_ptr<Camera>>) object(entry);
            else if constexpr (std::is_same_v<T,std::shared_ptr<Plane>>) {if(enter(entry))nodes.push_back({entry,{},{}});}
            else material(entry);
        }
        nodes.push_back(std::move(node));
    }
    template <class T> void primitive(JSNativeOwnerNode& node,const std::shared_ptr<T>& value) {
        edge(node,value->geometry);geometry(value->geometry);
        edge(node,value->material);material(value->material);
        const auto materials=value->materials.storage();
        node.edges.push_back({&value->materials,materials});array<std::shared_ptr<Material>>(materials);
        if (value->morphTargetInfluences) {
            const auto influences=value->morphTargetInfluences->storage();
            node.edges.push_back({&*value->morphTargetInfluences,influences});array<double>(influences);
        }
        edge(node,value->morphTargetDictionary);
        if (enter(value->morphTargetDictionary)) nodes.push_back({value->morphTargetDictionary,{},{}});
    }
    void matrix(JSNativeOwnerNode& node,const Matrix4& value) {
        const auto& target=value.aliasTarget();if(!target)return;edge(node,target);
        if(const auto owner=value.aliasObjectOwner().lock())object(owner);
        else if(enter(target)){JSNativeOwnerNode next{target,{},{}};matrix(next,*target);nodes.push_back(std::move(next));}
    }
    void object(const std::shared_ptr<Object3D>& value) {
        if (!enter(value)) return;
        JSNativeOwnerNode node{value,{value->layers.runtimeRoot()},{}};
        matrix(node,value->matrix);matrix(node,value->matrixWorld);matrix(node,value->modelViewMatrix);
        if(const auto camera=std::dynamic_pointer_cast<Camera>(value)){matrix(node,camera->matrixWorldInverse);matrix(node,camera->projectionMatrix);matrix(node,camera->projectionMatrixInverse);}
        clips(value,value->animations);
        if (const auto lod = std::dynamic_pointer_cast<LOD>(value)) {
            const auto roots = lod->runtimeRoots();
            node.roots.insert(node.roots.end(),roots.begin(),roots.end());
        }
        const auto children = value->children.storage();
        node.edges.push_back({&value->children,children}); array<std::shared_ptr<Object3D>>(children);
        for (const auto* slot : {&value->customDepthMaterial,&value->customDistanceMaterial}) { edge(node,*slot); material(*slot); }
        if (const auto mesh = std::dynamic_pointer_cast<Mesh>(value)) {
            node.roots.push_back(&mesh->count.value);
            edge(node,mesh->geometry); geometry(mesh->geometry);
            edge(node,mesh->material); material(mesh->material);
            const auto materials = mesh->materials.storage();
            node.edges.push_back({&mesh->materials,materials}); array<std::shared_ptr<Material>>(materials);
            if (mesh->morphTargetInfluences) {
                const auto influences = mesh->morphTargetInfluences->storage();
                node.edges.push_back({&*mesh->morphTargetInfluences,influences}); array<double>(influences);
            }
            edge(node,mesh->morphTargetDictionary);
            if (enter(mesh->morphTargetDictionary)) nodes.push_back({mesh->morphTargetDictionary,{},{}});
        }
        if (const auto line = std::dynamic_pointer_cast<Line>(value)) primitive(node,line);
        if (const auto points = std::dynamic_pointer_cast<Points>(value)) primitive(node,points);
        if (const auto sprite = std::dynamic_pointer_cast<Sprite>(value)) {
            node.roots.push_back(&sprite->count.value);
            edge(node,sprite->geometry);geometry(sprite->geometry);
            edge(node,sprite->material);material(sprite->material);
        }
        if (const auto skinned = std::dynamic_pointer_cast<SkinnedMesh>(value)) {
            edge(node,skinned->skeleton); skeleton(skinned->skeleton);
            edge(node,skinned->boundingBox); edge(node,skinned->boundingSphere);
        }
        if (const auto instanced = std::dynamic_pointer_cast<InstancedMesh>(value)) {
            edge(node,instanced->instanceMatrix); attribute(instanced->instanceMatrix);
            edge(node,instanced->instanceColor); attribute(instanced->instanceColor);
            edge(node,instanced->morphTexture); texture(instanced->morphTexture);
            edge(node,instanced->boundingBox); edge(node,instanced->boundingSphere);
        }
        if (const auto light = std::dynamic_pointer_cast<DirectionalLight>(value)) {
            edge(node,light->target); object(light->target); edge(node,light->shadow); shadow(light->shadow);
        }
        if (const auto light = std::dynamic_pointer_cast<PointLight>(value)) { edge(node,light->shadow); shadow(light->shadow); }
        if (const auto light = std::dynamic_pointer_cast<SpotLight>(value)) {
            edge(node,light->target); object(light->target); edge(node,light->shadow); shadow(light->shadow);
            edge(node,light->map); texture(light->map);
        }
        if (const auto light = std::dynamic_pointer_cast<IESSpotLight>(value)) { edge(node,light->iesMap); texture(light->iesMap); }
        if (const auto light = std::dynamic_pointer_cast<LightProbe>(value)) { edge(node,light->sh); if (enter(light->sh)) nodes.push_back({light->sh,{},{}}); }
        if (const auto scene = std::dynamic_pointer_cast<Scene>(value)) {
            edge(node,scene->overrideMaterial); material(scene->overrideMaterial);
            edge(node,scene->environment); texture(scene->environment);
            edge(node,scene->fog); if (enter(scene->fog)) nodes.push_back({scene->fog,{},{}});
        }
        const auto child=[&](const auto& source){edge(node,source);object(source);};
        if(const auto h=std::dynamic_pointer_cast<ArrowHelper>(value)){child(h->line);child(h->cone);}
        if(const auto h=std::dynamic_pointer_cast<BoxHelper>(value))child(h->object);
        if(const auto h=std::dynamic_pointer_cast<Box3Helper>(value)){edge(node,h->box);if(enter(h->box))nodes.push_back({h->box,{},{}});}
        if(const auto h=std::dynamic_pointer_cast<PlaneHelper>(value)){edge(node,h->plane);if(enter(h->plane))nodes.push_back({h->plane,{},{}});}
        if(const auto h=std::dynamic_pointer_cast<CameraHelper>(value)){child(h->camera);node.roots.push_back(&h->pointMap);}
        if(const auto h=std::dynamic_pointer_cast<SkeletonHelper>(value)){child(h->root);const auto storage=h->bones.storage();node.edges.push_back({&h->bones,storage});array<std::shared_ptr<Object3D>>(storage);}
        if(const auto h=std::dynamic_pointer_cast<PointLightHelper>(value)){child(h->light);node.roots.push_back(&h->color);}
        if(const auto h=std::dynamic_pointer_cast<DirectionalLightHelper>(value)){child(h->light);child(h->lightPlane);child(h->targetLine);node.roots.push_back(&h->color);}
        if(const auto h=std::dynamic_pointer_cast<HemisphereLightHelper>(value)){child(h->light);node.roots.push_back(&h->color);edge(node,h->material);material(h->material);}
        if(const auto h=std::dynamic_pointer_cast<SpotLightHelper>(value)){child(h->light);child(h->cone);node.roots.push_back(&h->color);}
        if(const auto c=std::dynamic_pointer_cast<ArrayCamera>(value)){const auto storage=c->cameras.storage();node.edges.push_back({&c->cameras,storage});array<std::shared_ptr<Camera>>(storage);}
        if(const auto g=std::dynamic_pointer_cast<ClippingGroup>(value)){node.roots.insert(node.roots.end(),{&g->enabled,&g->clipIntersection,&g->clipShadows});const auto storage=g->clippingPlanes.storage();node.edges.push_back({&g->clippingPlanes,storage});array<std::shared_ptr<Plane>>(storage);}
        if(const auto extension=std::dynamic_pointer_cast<NativeObjectExtension>(value)) {
            const auto roots=extension->nativeJSReferences();node.roots.insert(node.roots.end(),roots.begin(),roots.end());
            extension->visitNativeObjectReferences(child);
        }
        nodes.push_back(std::move(node));
    }
};
std::vector<JSNativeOwnerNode> animationObjectOwners(const JSValue& self) {
    NativeOwners graph; graph.object(payload<Object3D>(self,objectTag)); return std::move(graph.nodes);
}
std::vector<JSNativeOwnerNode> shadowOwners(const JSValue& self) {NativeOwners graph;graph.shadow(payload<LightShadow>(self,"three.LightShadow"));return std::move(graph.nodes);}
std::vector<JSNativeOwnerNode> targetOwners(const JSValue& self) {NativeOwners graph;graph.target(payload<RenderTarget>(self,"three.RenderTarget"));return std::move(graph.nodes);}
std::vector<JSNativeOwnerNode> geometryOwners(const JSValue& self) {
    NativeOwners graph; graph.geometry(payload<BufferGeometry>(self,"three.BufferGeometry")); return std::move(graph.nodes);
}
std::vector<JSNativeOwnerNode> materialOwners(const JSValue& self) {
    NativeOwners graph; graph.material(payload<Material>(self,materialTag)); return std::move(graph.nodes);
}
std::vector<JSNativeOwnerNode> textureOwners(const JSValue& self) {
    NativeOwners graph; graph.texture(payload<Texture>(self,"three.Texture")); return std::move(graph.nodes);
}
std::vector<JSNativeOwnerNode> attributeOwners(const JSValue& self) {
    const auto state=payload<BufferAttributeRuntime>(self,"three.BufferAttribute");
    NativeOwners graph;
    JSNativeOwnerNode runtime{state,{},{{&state->storage,state->storage}}};
    graph.attributeStorage(state->storage);
    if(const auto owner=state->owner.lock()) graph.attribute(owner);
    graph.nodes.push_back(std::move(runtime));
    return std::move(graph.nodes);
}
std::vector<JSNativeOwnerNode> skeletonOwners(const JSValue& self) {
    NativeOwners graph; graph.skeleton(payload<Skeleton>(self,"three.Skeleton")); return std::move(graph.nodes);
}
template <class T> std::vector<JSNativeOwnerNode> arrayOwners(const JSValue& self) {
    NativeOwners graph; graph.array<T>(payload<ArrayStorage<T>>(self,arrayTag<T>())); return std::move(graph.nodes);
}
} // namespace

JSValue nativePlane(const std::shared_ptr<Plane>& plane) { return wrapPlane(plane); }
JSValue nativePlanePrototype() { return planePrototype(); }
JSValue nativeGeometry(const std::shared_ptr<BufferGeometry>& geometry) {
    return wrapGeometry(geometry);
}

JSValue nativeArray(const AnimationArray& array) {
    return NativeArray_detail::wrap(array);
}
AnimationArray animationArray(const JSValue& value) {
    return NativeArray_detail::unwrap(value);
}
namespace {
JSValue attributeRead(const JSValue& self,const Args&,const Args& capture) {
    const auto attribute=bufferAttributeRuntimeValue(self);const auto& key=capture[0].string();
    if(key=="array") return nativeArray(attribute->getArray());
    if(key=="itemSize"&&self.getHostSlot("rawItemSizePresent").toBoolean()) return self.getHostSlot("rawItemSize");
    if(key=="normalized"&&!self.getHostSlot("rawNormalized").isUndefined()) return self.getHostSlot("rawNormalized");
    return attribute->getProperty(key);
}
JSValue attributeWrite(const JSValue& self,const Args& args,const Args& capture) {
    const auto attribute=bufferAttributeRuntimeValue(self);const auto& key=capture[0].string();const auto value=argument(args,0);
    if(key=="name") attribute->name=value.toString();
    else if(key=="itemSize") {attribute->itemSize=value.toNumber();auto wrapper=self;wrapper.setHostSlot("rawItemSize",value);wrapper.setHostSlot("rawItemSizePresent",true);}
    else if(key=="count") attribute->count=value.toNumber();
    else if(key=="normalized") {attribute->normalized=value.toBoolean();auto wrapper=self;wrapper.setHostSlot("rawNormalized",value);}
    else if(key=="array") {
        const auto array=animationArray(value);attribute->array=std::visit([](const auto& input)->BufferAttribute::Array {
            if constexpr(std::is_same_v<std::decay_t<decltype(input)>,JSValue>) throw JSTypeError("Expected TypedArray");else return input;
        },array.storage());
    }
    return {};
}
JSValue attributeMethod(const JSValue& self,const Args& args,const Args& capture) {
    const auto& name=capture[0].string();
    if(name=="onUploadCallback") return {};
    if(name=="needsUpdate") {
        if(argument(args,0).strictEquals(true)) {const auto next=self.get("version").toNumber()+1;auto receiver=self;receiver.set("version",next);}return {};
    }
    if(name=="setUsage" || name=="onUpload") {auto receiver=self;receiver.set(name=="setUsage"?"usage":"onUploadCallback",argument(args,0));return self;}
    if(name=="addUpdateRange") {
        const auto ranges=self.get("updateRanges"),push=ranges.get("push");
        auto range=JSValue::object();range.defineProperty("start",dataDescriptor(argument(args,0)));range.defineProperty("count",dataDescriptor(argument(args,1)));
        push.call(ranges,{range});return {};
    }
    if(name=="clearUpdateRanges") {auto ranges=self.get("updateRanges");ranges.set("length",0);return {};}
    const auto attribute=bufferAttributeRuntimeValue(self);
    if(name=="toJSON") {auto result=JSValue::fromJSON(attribute->toJSON());result.set("itemSize",self.get("itemSize"));result.set("normalized",self.get("normalized"));return result;}
    if(name=="clone") return nativeAttribute(attribute->clone());
    const auto index=argument(args,0).toNumber();
    if(name=="getX") return attribute->getX(index);
    if(name=="getY") return attribute->getY(index);
    if(name=="getZ") return attribute->getZ(index);
    if(name=="getW") return attribute->getW(index);
    if(name=="setX") attribute->setX(index,argument(args,1).toNumber());
    else if(name=="setY") attribute->setY(index,argument(args,1).toNumber());
    else if(name=="setZ") attribute->setZ(index,argument(args,1).toNumber());
    else if(name=="setW") attribute->setW(index,argument(args,1).toNumber());
    else if(name=="setXY") {const auto x=argument(args,1).toNumber(),y=argument(args,2).toNumber();attribute->setXY(index,x,y);}
    else if(name=="setXYZ") {const auto x=argument(args,1).toNumber(),y=argument(args,2).toNumber(),z=argument(args,3).toNumber();attribute->setXYZ(index,x,y,z);}
    else if(name=="setXYZW") {const auto x=argument(args,1).toNumber(),y=argument(args,2).toNumber(),z=argument(args,3).toNumber(),w=argument(args,4).toNumber();attribute->setXYZW(index,x,y,z,w);}
    else throw JSTypeError("Unknown BufferAttribute method");
    return self;
}
JSValue attributeModule() {
    static thread_local JSWeakValue cachedModule;
    return cachedModuleValue(cachedModule, [] {
        auto constructor=JSValue::constructor([](const JSValue& receiver,const Args& args,const Args&)->JSValue {
            const auto array=animationArray(argument(args,0));const auto itemSize=argument(args,1);const auto size=itemSize.toNumber();
            const auto normalized=argument(args,2).isUndefined()?JSValue(false):argument(args,2);
            const auto attribute=std::visit([&](const auto& input)->std::shared_ptr<BufferAttribute> {
                if constexpr(std::is_same_v<std::decay_t<decltype(input)>,JSValue>) throw JSTypeError("Expected TypedArray");
                else return std::make_shared<BufferAttribute>(input,size,normalized.toBoolean());
            },array.storage());auto result=nativeAttribute(attribute);result.setPrototype(receiver.getPrototype());
            result.setHostSlot("rawItemSize",itemSize);result.setHostSlot("rawItemSizePresent",true);
            result.setHostSlot("rawNormalized",normalized);return result;
        },{},"BufferAttribute",2);auto result=constructor.get("prototype");
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"getX",1},{"getY",1},{"getZ",1},{"getW",1},
            {"setX",2},{"setY",2},{"setZ",2},{"setW",2},{"setXY",3},{"setXYZ",4},{"setXYZW",5},{"clone",0},{"toJSON",0},
            {"onUploadCallback",0},{"setUsage",1},{"addUpdateRange",2},{"clearUpdateRanges",0},{"onUpload",1}}) {
            auto d=dataDescriptor(JSValue::function(attributeMethod,{name},name,arity),true,false,true);result.defineProperty(name,d);
        }
        JSPropertyDescriptor needsUpdate;needsUpdate.setter=JSValue::function(attributeMethod,{"needsUpdate"},"set needsUpdate",1);needsUpdate.configurable=true;
        result.defineProperty("needsUpdate",needsUpdate);
        auto module=JSValue::object();module.set("constructor",constructor);module.set("prototype",result);return module;
    });
}
JSValue attributePrototype() { return attributeModule().get("prototype"); }
std::int32_t halfInt32(const JSValue& value) {
    const auto number=value.toNumber();auto wrapped=std::isfinite(number)?std::fmod(std::trunc(number),4294967296.):0.;
    if(wrapped<0)wrapped+=4294967296.;
    const auto bits=static_cast<std::uint32_t>(wrapped);
    return bits<=INT32_MAX?static_cast<std::int32_t>(bits):static_cast<std::int32_t>(static_cast<std::int64_t>(bits)-4294967296LL);
}
double halfRead(const JSValue& value) {
    // DataUtils uses val twice, with a separate ToInt32 at each bitwise operation.
    const auto m=halfInt32(value)>>10;
    const auto low=static_cast<std::uint32_t>(halfInt32(value))&0x3ff;
    return m<0||m>63?0.:fromHalfFloat(double(static_cast<std::uint32_t>(m)*1024+low));
}
double halfWrite(const JSValue& value) {
    if(std::abs(value.toNumber())>65504.)ThreeUtils::warn({"DataUtils.toHalfFloat(): Value out of range."});
    return toHalfFloat(MathUtils::clamp(value.toNumber(),-65504.,65504.));
}
JSValue normalizedHalfComponent(const JSValue& value,const JSValue& array,bool write) {
    const auto constructor=array.get("constructor");
    static constexpr double scales[]{127.,255.,32767.,65535.,2147483647.,4294967295.,1.,0.,255.};
    for(std::size_t kind=0;kind<9;++kind)if(constructor.strictEquals(NativeArray_detail::constructor(kind))) {
        if(kind==6)return value;
        if(kind==7)break;
        if(write)return MathUtils::round(value.toNumber()*scales[kind]);
        const auto component=value.toNumber()/scales[kind];
        return kind==0||kind==2||kind==4?MathUtils::max(component,-1.):component;
    }
    throw std::runtime_error("THREE.MathUtils: Invalid component type.");
}
JSValue halfAttributeMethod(const JSValue& self,const Args& args,const Args& capture) {
    const auto name=capture[0].string();const auto index=argument(args,0);
    if(name[0]=='g') {
        const auto array=self.get("array");const auto size=self.get("itemSize");
        const auto component=name.back()=='X'?0:name.back()=='Y'?1:name.back()=='Z'?2:3;
        const auto numericIndex=index.toNumber();const auto numericSize=size.toNumber();
        const auto offset=numericIndex*numericSize+component;
        auto value=JSValue(halfRead(array.get(JSValue(offset))));
        if(self.get("normalized").toBoolean())value=normalizedHalfComponent(value,self.get("array"),false);
        return value;
    }
    const auto count=name.size()-3;
    if(count==1) {
        auto value=argument(args,1);
        if(self.get("normalized").toBoolean())value=normalizedHalfComponent(value,self.get("array"),true);
        auto array=self.get("array");const auto size=self.get("itemSize");
        const auto component=name.back()=='X'?0:name.back()=='Y'?1:name.back()=='Z'?2:3;
        const auto numericIndex=index.toNumber();const auto numericSize=size.toNumber();
        const auto offset=numericIndex*numericSize+component;
        array.set(JSValue(offset),halfWrite(value));return self;
    }
    const auto size=self.get("itemSize");const auto numericIndex=index.toNumber();const auto numericSize=size.toNumber();const auto offset=numericIndex*numericSize;
    Args values;for(std::size_t i=0;i<count;++i)values.push_back(argument(args,i+1));
    if(self.get("normalized").toBoolean())for(auto& value:values)value=normalizedHalfComponent(value,self.get("array"),true);
    for(std::size_t i=0;i<count;++i) {auto array=self.get("array");array.set(JSValue(offset+double(i)),halfWrite(values[i]));}
    return self;
}
JSValue halfAttributeModule() {
    static thread_local JSWeakValue cachedModule;
    return cachedModuleValue(cachedModule, [] {
        auto module=JSValue::object();
        auto constructor=JSValue::builtinConstructor([](const JSValue& target,const Args& args,const Args& captures)->JSValue {
            // Derived construction evaluates the Uint16Array before entering
            // the dynamic super constructor and reading newTarget.prototype.
            const auto array=NativeArray_detail::constructor(3).construct({argument(args,0)});
            const auto super=captures[0].get("constructor").getPrototype();
            auto result=super.construct({array,argument(args,1),argument(args,2)},target);
            result.set("isFloat16BufferAttribute",true);
            if(result.isHostObject("three.BufferAttribute")) {
                const auto state=payload<BufferAttributeRuntime>(result,"three.BufferAttribute");
                if(state->owner.expired())static_cast<void>(Float16BufferAttribute::restoreRuntime(state->storage,result));
            }
            return result;
        },{module},"Float16BufferAttribute",3);
        constructor.setPrototype(attributeModule().get("constructor"));
        auto prototype=constructor.get("prototype");prototype.setPrototype(attributePrototype());
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{
            {"getX",1},{"setX",2},{"getY",1},{"setY",2},{"getZ",1},{"setZ",2},{"getW",1},{"setW",2},
            {"setXY",3},{"setXYZ",4},{"setXYZW",5}})
            prototype.defineProperty(name,dataDescriptor(JSValue::function(halfAttributeMethod,{name},name,arity),true,false,true));
        module.set("constructor",constructor);module.set("prototype",prototype);return module;
    });
}
}
JSValue nativeBufferAttributeConstructor() { return attributeModule().get("constructor"); }
JSValue nativeFloat16BufferAttributeConstructor() { return halfAttributeModule().get("constructor"); }
bool instanceOfFloat16BufferAttribute(const JSValue& value) {
    const auto constructor=nativeFloat16BufferAttributeConstructor();
    const auto method=constructor.get(JSValue::wellKnownSymbol("hasInstance"));
    if(!method.isUndefined()&&!method.isNull())return method.call(constructor,{value}).toBoolean();
    if(!value.isObject())return false;
    const auto prototype=constructor.get("prototype");if(!prototype.isObject())throw JSTypeError("Non-object instanceof prototype");
    for(auto current=value.getPrototype();!current.isNull();current=current.getPrototype())if(current.strictEquals(prototype))return true;
    return false;
}
void initializeFloat16BufferAttributeRuntime(BufferAttribute& attribute) {
    auto object=attribute.object();object.setPrototype(halfAttributeModule().get("prototype"));
}
void initializeBufferAttributeRuntime(BufferAttribute& attribute) {
    auto state=std::make_shared<BufferAttributeRuntime>();state->storage=attribute.runtimeStorage();
    auto result=JSValue::hostObject(state,state.get(),"three.BufferAttribute");
    attribute.object_=result;
    result.setPrototype(attributePrototype());
    accessor(result,"isBufferAttribute",attributeRead);
    result.defineProperty("id",dataDescriptor(attribute.id,false,false,false));
    for(const char* key:{"name","array","itemSize","count","normalized"}) accessor(result,key,attributeRead,attributeWrite);
    result.set("usage",StaticDrawUsage);result.set("updateRanges",JSValue::array());
    result.set("gpuType",FloatType);result.set("version",0);
    static auto hooks=[] {auto h=std::make_shared<JSHostObjectHooks>();h->nativeOwnership=attributeOwners;return h;}();
    result.setHostHooks(hooks);
}
namespace {
constexpr const char* instanceDescriptorSlot="nativeInstanceDescriptors";
JSValue instanceDescriptors(const JSValue& self) {
    return self.getHostSlot(instanceDescriptorSlot);
}
JSHostPropertyResult instanceOwn(const JSValue& self,const std::string& key) {
    if(key!="meshPerAttribute")return {};
    auto descriptor=instanceDescriptors(self).getOwnPropertyDescriptor(key);
    if(descriptor&&descriptor->value)descriptor->value=payload<BufferAttributeRuntime>(self,"three.BufferAttribute")->storage->meshPerAttribute;
    return {true,std::move(descriptor)};
}

std::shared_ptr<const JSHostObjectHooks> instanceHooks();
struct InstanceHooksGuard {
    JSValue object;
    explicit InstanceHooksGuard(const JSValue& value):object(value){object.setHostHooks({});}
    ~InstanceHooksGuard(){object.setHostHooks(instanceHooks());}
};
std::optional<bool> instanceDefine(const JSValue& self,const std::string& key,const JSPropertyDescriptor& descriptor) {
    if(key!="meshPerAttribute")return {};
    const auto previous=instanceOwn(self,key).descriptor;
    if(!previous&&!self.isExtensible())return false;
    auto next=JSValue::object(nullptr);if(previous)next.defineProperty(key,*previous);
    if(!next.defineOwnProperty(key,descriptor))return false;
    const auto result=*next.getOwnPropertyDescriptor(key);auto metadata=result;
    if(metadata.value)metadata.value=JSValue();
    if(metadata.getter)metadata.getter=JSValue();
    if(metadata.setter)metadata.setter=JSValue();
    {InstanceHooksGuard guard(self);if(!guard.object.defineOwnProperty(key,metadata))return false;}
    auto stored=JSValue::object(nullptr);auto privateMetadata=result;if(privateMetadata.value)privateMetadata.value=JSValue();stored.defineProperty(key,privateMetadata);
    auto object=self;object.setHostSlot(instanceDescriptorSlot,stored);
    const auto state=payload<BufferAttributeRuntime>(self,"three.BufferAttribute");
    state->storage->meshPerAttribute=result.value?*result.value:JSValue();
    return true;
}
std::optional<bool> instanceErase(const JSValue& self,const std::string& key) {
    if(key!="meshPerAttribute")return {};
    auto fields=instanceDescriptors(self);
    const auto existing=fields.getOwnPropertyDescriptor(key);if(existing&&!existing->configurable.value_or(false))return false;
    {InstanceHooksGuard guard(self);if(!guard.object.erase(key))return false;}
    fields.erase(key);
    const auto state=payload<BufferAttributeRuntime>(self,"three.BufferAttribute");state->storage->meshPerAttribute={};return true;
}
std::shared_ptr<const JSHostObjectHooks> instanceHooks() {
    static auto hooks=[] {auto h=std::make_shared<JSHostObjectHooks>();h->nativeOwnership=attributeOwners;h->getOwnProperty=instanceOwn;h->defineOwnProperty=instanceDefine;h->erase=instanceErase;return h;}();return hooks;
}

void initializeInstanceProtocol(JSValue object,const std::shared_ptr<BufferAttribute>& attribute) {
    if(!std::dynamic_pointer_cast<InstancedBufferAttribute>(attribute)||!object.getHostSlot(instanceDescriptorSlot).isUndefined())return;
    object.defineProperty("isInstancedBufferAttribute",dataDescriptor(true));
    object.defineProperty("meshPerAttribute",dataDescriptor(JSValue()));
    auto fields=JSValue::object(nullptr);fields.defineProperty("meshPerAttribute",dataDescriptor(JSValue()));object.setHostSlot(instanceDescriptorSlot,fields);
    object.setHostHooks(instanceHooks());
}
}
void bindBufferAttributeRuntime(const JSValue& object,const std::shared_ptr<BufferAttribute>& attribute) {
    static_cast<void>(JSWeakValue(object));
    const auto state=payload<BufferAttributeRuntime>(object,"three.BufferAttribute");
    if(state->storage!=attribute->runtimeStorage())throw JSTypeError("BufferAttribute runtime storage mismatch");
    state->owner=attribute;
    initializeInstanceProtocol(object,attribute);
}
std::shared_ptr<BufferAttribute> bufferAttributeRuntimeValue(const JSValue& object) {
    static_cast<void>(JSWeakValue(object));
    const auto state=payload<BufferAttributeRuntime>(object,"three.BufferAttribute");
    if(const auto owner=state->owner.lock())return owner;
    return BufferAttribute::restore(state->storage,object);
}
JSValue nativeAttribute(const std::shared_ptr<VertexAttribute>& attribute) {
    if(!attribute) return nullptr;
    if(const auto ordinary=std::dynamic_pointer_cast<BufferAttribute>(attribute)) return ordinary->object();
    if(const auto root=attribute->managedObjectRoot()) return *root;
    throw JSTypeError("Unsupported native vertex attribute");
}
std::shared_ptr<VertexAttribute> vertexAttribute(const JSValue& value) {
    if(value.isNull()) return nullptr;
    if(value.isHostObject("three.BufferAttribute")) return bufferAttributeRuntimeValue(value);
    if(value.get("isInterleavedBufferAttribute").toBoolean()) return std::make_shared<InterleavedBufferAttribute>(InterleavedBufferAttribute::fromObject(value));
    throw JSTypeError("Expected a vertex attribute");
}
JSValue nativeObject(const std::shared_ptr<Object3D>& object) {
    if (!object) return nullptr;
    auto result = JSValue::hostObject(object, object.get(), objectTag);
    if (initialize(result)) {
        static auto hooks = [] { auto h = std::make_shared<JSHostObjectHooks>(); h->trace = [](const JSValue& self) {
            std::vector<JSValue> result; std::unordered_set<const Object3D*> visited;
            traceNativeObject(self, payload<Object3D>(self, objectTag), result, visited); return result;
        };h->nativeOwnership=animationObjectOwners; return h; }(); result.setHostHooks(hooks);
        for (const char* key : {"uuid", "name", "type", "visible", "matrixWorldNeedsUpdate", "matrixAutoUpdate", "matrixWorldAutoUpdate", "renderOrder", "castShadow", "layers", "children"}) accessor(result, key, objectRead, objectWrite);
        for (const char* key : {"id", "isObject3D", "isMesh", "isLight", "isBone", "isSkinnedMesh", "isInstancedMesh", "parent", "position", "scale", "up", "rotation", "quaternion", "animations"}) accessor(result, key, objectRead);
        for(const auto* key:{"matrix","matrixWorld"})accessor(result,key,objectRead);
        if(std::dynamic_pointer_cast<Camera>(object))for(const auto* key:{"matrixWorldInverse","projectionMatrix","projectionMatrixInverse","coordinateSystem","reversedDepth"})accessor(result,key,objectRead);
        if(std::dynamic_pointer_cast<PerspectiveCamera>(object)||std::dynamic_pointer_cast<OrthographicCamera>(object))for(const auto* key:{"near","far"})accessor(result,key,objectRead);
        const auto helperFields=[&](std::initializer_list<const char*> names){for(const auto* name:names)accessor(result,name,objectRead,objectWrite);};
        if(std::dynamic_pointer_cast<ArrowHelper>(object))helperFields({"line","cone"});
        if(std::dynamic_pointer_cast<BoxHelper>(object))helperFields({"object"});
        if(std::dynamic_pointer_cast<Box3Helper>(object))helperFields({"box"});
        if(std::dynamic_pointer_cast<PlaneHelper>(object))helperFields({"plane","size"});
        if(std::dynamic_pointer_cast<CameraHelper>(object))helperFields({"camera","pointMap"});
        if(std::dynamic_pointer_cast<SkeletonHelper>(object)){helperFields({"root","bones"});accessor(result,"isSkeletonHelper",objectRead);}
        if(std::dynamic_pointer_cast<PointLightHelper>(object))helperFields({"light","color"});
        if(std::dynamic_pointer_cast<DirectionalLightHelper>(object))helperFields({"light","color","lightPlane","targetLine"});
        if(std::dynamic_pointer_cast<HemisphereLightHelper>(object))helperFields({"light","color","material"});
        if(std::dynamic_pointer_cast<SpotLightHelper>(object))helperFields({"light","color","cone"});
        if(std::dynamic_pointer_cast<ArrayCamera>(object)){helperFields({"cameras","isMultiViewCamera"});accessor(result,"isArrayCamera",objectRead);}
        if(std::dynamic_pointer_cast<ClippingGroup>(object)){helperFields({"clippingPlanes","enabled","clipIntersection","clipShadows"});accessor(result,"isClippingGroup",objectRead);}
        if (const auto lod = std::dynamic_pointer_cast<LOD>(object)) {
            JSPropertyDescriptor levels; levels.value = lod->levels; levels.enumerable = true;
            result.defineProperty("levels",levels);
            for (const char* key : {"isLOD","autoUpdate","_currentLevel"}) accessor(result,key,objectRead,objectWrite);
        }
        if (std::dynamic_pointer_cast<Mesh>(object)) for (const char* key : {"count", "material", "geometry", "morphTargetInfluences", "morphTargetDictionary"}) accessor(result, key, objectRead, objectWrite);
        if (std::dynamic_pointer_cast<Line>(object) || std::dynamic_pointer_cast<Points>(object))
            for (const char* key : {"material", "geometry", "morphTargetInfluences", "morphTargetDictionary"}) accessor(result,key,objectRead,objectWrite);
        if (std::dynamic_pointer_cast<Sprite>(object)) {
            for (const char* key : {"material","geometry","count"}) accessor(result,key,objectRead,objectWrite);
            accessor(result,"center",objectRead);
        }
        if (object->isLine) accessor(result,"isLine",objectRead);
        if (object->isLineLoop) accessor(result,"isLineLoop",objectRead);
        if (object->isLineSegments) accessor(result,"isLineSegments",objectRead);
        if (object->isPoints) accessor(result,"isPoints",objectRead);
        if (object->isSprite) accessor(result,"isSprite",objectRead);
        if (std::dynamic_pointer_cast<SkinnedMesh>(object)) {
            for (const char* key : {"skeleton","bindMode"}) accessor(result,key,objectRead,objectWrite);
            for (const char* key : {"bindMatrix","bindMatrixInverse"}) accessor(result,key,objectRead);
        }
        if (std::dynamic_pointer_cast<Light>(object)) { accessor(result,"color",objectRead); accessor(result,"intensity",objectRead,objectWrite); }
        if(std::dynamic_pointer_cast<AmbientLight>(object))result.set("isAmbientLight",true);
        if(std::dynamic_pointer_cast<DirectionalLight>(object))result.set("isDirectionalLight",true);
        if(std::dynamic_pointer_cast<SpotLight>(object))result.set("isSpotLight",true);
        if(std::dynamic_pointer_cast<PointLight>(object))result.set("isPointLight",true);
        if(std::dynamic_pointer_cast<HemisphereLight>(object))result.set("isHemisphereLight",true);
        if(std::dynamic_pointer_cast<RectAreaLight>(object))result.set("isRectAreaLight",true);
        if(std::dynamic_pointer_cast<LightProbe>(object)){result.set("isLightProbe",true);accessor(result,"sh",objectRead);}
        if(std::dynamic_pointer_cast<DirectionalLight>(object)||std::dynamic_pointer_cast<PointLight>(object)||std::dynamic_pointer_cast<SpotLight>(object))accessor(result,"shadow",objectRead,objectWrite);
        if (std::dynamic_pointer_cast<HemisphereLight>(object)) accessor(result,"groundColor",objectRead);
        if (std::dynamic_pointer_cast<DirectionalLight>(object)) accessor(result,"target",objectRead,objectWrite);
        if (std::dynamic_pointer_cast<PointLight>(object)) for (const char* key : {"power","distance","decay"}) accessor(result,key,objectRead,objectWrite);
        if (std::dynamic_pointer_cast<SpotLight>(object)) for (const char* key : {"power","distance","decay","angle","penumbra","target","map"}) accessor(result,key,objectRead,objectWrite);
        if (std::dynamic_pointer_cast<RectAreaLight>(object)) for (const char* key : {"power","width","height"}) accessor(result,key,objectRead,objectWrite);
        if(const auto extension=std::dynamic_pointer_cast<NativeObjectExtension>(object))extension->initializeNativeWrapper(result);
        installNativeObjectRenderingBridge(result,object);
    }
    return result;
}
JSValue nativeMaterial(const std::shared_ptr<Material>& material) {
    if (!material) return nullptr;
    auto result = JSValue::hostObject(material, material.get(), materialTag);
    if (initialize(result)) {
        static auto hooks = [] { auto h = std::make_shared<JSHostObjectHooks>(JSHostObjectHooks{materialOwn,materialDefine,materialErase,materialNames}); h->trace = [](const JSValue& self) {
            std::vector<JSValue> result; traceNativeMaterial(self, payload<Material>(self, materialTag), result); return result;
        }; h->nativeOwnership=materialOwners; return h; }(); result.setHostHooks(hooks);
        static thread_local std::map<std::string,JSWeakValue> prototypes;
        const auto prototype = cachedModuleValue(prototypes[material->type], [&] {
            auto p = JSValue::object();
            accessor(p,"needsUpdate",nullptr,materialWrite,false);
            material->visitProperties([&](std::string_view key,Material::PropertyReference property) {
                if (std::holds_alternative<Material::AlphaTest*>(property)) accessor(p,std::string(key).c_str(),materialRead,materialWrite,false);
            });
            method(p,"setValues",materialSetValues,"three.Material");
            method(p,"clone",[](const JSValue& self,const Args&,const Args&){ return nativeMaterial(payload<Material>(self,materialTag)->clone()); },"three.Material");
            method(p,"toJSON",[](const JSValue& self,const Args&,const Args&){ return JSValue::fromJSON(payload<Material>(self,materialTag)->toJSON()); },"three.Material");
            method(p,"onBeforeCompile",[](const JSValue&,const Args&,const Args&){ return JSValue(); },"three.Material","onBeforeCompile( /* shaderobject, renderer */ ) {}");
            method(p,"customProgramCacheKey",[](const JSValue& self,const Args&,const Args&){ const auto callback = self.get("onBeforeCompile"); return callback.get("toString").call(callback); },"three.Material","customProgramCacheKey() {\r\n\r\n\t\treturn this.onBeforeCompile.toString();\r\n\r\n\t}");
            return p;
        });
        result.setPrototype(prototype);
        material->visitProperties([&](std::string_view key,Material::PropertyReference property) {
            if (std::holds_alternative<Material::Reflectivity*>(property)) {
                JSPropertyDescriptor d; d.getter = JSValue::function(materialRead,{JSValue(std::string(key))});
                d.setter = JSValue::function(materialWrite,{JSValue(std::string(key))});
                d.enumerable = false; d.configurable = false; result.defineProperty(std::string(key),d);
            }
        });
    }
    return result;
}
JSValue nativeTexture(const std::shared_ptr<Texture>& texture) {
    if (!texture) return nullptr;
    auto result = JSValue::hostObject(texture, texture.get(), "three.Texture");
    if (initialize(result)) {
        static auto hooks = [] { auto h = std::make_shared<JSHostObjectHooks>(); h->trace = [](const JSValue& self) {
            std::vector<JSValue> result; traceNativeTexture(self, payload<Texture>(self, "three.Texture"), result); return result;
        }; h->nativeOwnership=textureOwners; return h; }(); result.setHostHooks(hooks);
        static thread_local JSWeakValue cachedPrototype;
        const auto prototype = cachedModuleValue(cachedPrototype, [] {
            auto p = JSValue::object();
            lightingMethod(p,"updateMatrix",[](const JSValue& self,const Args&,const Args&)->JSValue {
                const auto matrix=self.get("matrix"),fn=matrix.get("setUvTransform");
                Args args;
                args.push_back(self.get("offset").get("x"));
                args.push_back(self.get("offset").get("y"));
                args.push_back(self.get("repeat").get("x"));
                args.push_back(self.get("repeat").get("y"));
                args.push_back(self.get("rotation"));
                args.push_back(self.get("center").get("x"));
                args.push_back(self.get("center").get("y"));
                fn.call(matrix,args);return {};
            },"three.Texture",0);
            method(p,"clone",[](const JSValue& self,const Args&,const Args&){ return nativeTexture(payload<Texture>(self,"three.Texture")->clone()); },"three.Texture");
            method(p,"toJSON",[](const JSValue& self,const Args&,const Args&){ return JSValue::fromJSON(payload<Texture>(self,"three.Texture")->toJSON()); },"three.Texture");
            return p;
        });
        result.setPrototype(prototype);
        for (const char* key : {"offset", "repeat", "center", "matrix", "isTexture", "isRenderTargetTexture"}) accessor(result, key, textureRead);
        for (const char* key : {"rotation", "colorSpace", "uuid", "name", "version", "matrixAutoUpdate", "wrapS", "wrapT", "channel", "format", "magFilter", "minFilter", "anisotropy", "type", "flipY", "generateMipmaps", "mapping", "needsUpdate"}) accessor(result, key, textureRead, textureWrite);
        for (const auto& [name,enabled]:std::initializer_list<std::pair<const char*,bool>>{{"isDataArrayTexture",texture->isDataArrayTexture},{"isData3DTexture",texture->isData3DTexture},{"isCubeTexture",texture->isCubeTexture},{"isDepthTexture",texture->isDepthTexture},{"isCubeDepthTexture",texture->isCubeDepthTexture}})
            if(enabled)accessor(result,name,textureRead,textureWrite);
        if(std::dynamic_pointer_cast<DepthTexture>(texture))accessor(result,"compareFunction",textureRead,textureWrite);

    }
    return result;
}
