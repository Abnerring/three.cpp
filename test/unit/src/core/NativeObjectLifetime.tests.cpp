#include "core/NativeObject.h"
#include "core/BufferAttribute.h"
#include "materials/MeshBasicMaterial.h"
#include "materials/MeshStandardMaterial.h"
#include "math/Color.h"
#include "math/Euler.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/Plane.h"
#include "math/Quaternion.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "objects/SkinnedMesh.h"
#include "textures/Texture.h"
#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
unsigned checks = 0;
void check(bool condition, const std::string& message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
struct Entry { std::string name; JSValue value; };
std::vector<Entry> moduleObjects() {
    std::vector<Entry> entries;
    const auto add = [&](const char* name, JSValue object) {
        auto prototype = object.getPrototype();
        entries.push_back({name, prototype});
        for (const auto& key : prototype.ownPropertyNames()) {
            const auto descriptor = prototype.getOwnPropertyDescriptor(key);
            if (descriptor && descriptor->value && descriptor->value->isCallable())
                entries.push_back({std::string(name) + "." + key, *descriptor->value});
        }
    };
    add("Vector2", nativeVector(std::make_shared<Vector2>()));
    add("Vector3", nativeVector(std::make_shared<Vector3>()));
    add("Vector4", nativeVector(std::make_shared<Vector4>()));
    add("Quaternion", nativeQuaternion(std::make_shared<Quaternion>()));
    add("Euler", nativeEuler(std::make_shared<Euler>()));
    add("Color", nativeColor(std::make_shared<Color>()));
    add("Matrix3", nativeMatrix(std::make_shared<Matrix3>()));
    add("Matrix4", nativeMatrix(std::make_shared<Matrix4>()));
    add("Plane", nativePlane(std::make_shared<Plane>()));
    add("Material.Basic", nativeMaterial(std::make_shared<MeshBasicMaterial>()));
    add("Material.Standard", nativeMaterial(std::make_shared<MeshStandardMaterial>()));
    add("Texture", nativeTexture(std::make_shared<Texture>()));
    auto mesh = std::make_shared<SkinnedMesh>();
    mesh->skeleton = std::make_shared<Skeleton>();
    // The rendering bridge adds a per-wrapper update layer above the cached
    // Skeleton module prototype; inspect the shared parent of that layer.
    add("Skeleton", nativeObject(mesh).get("skeleton").getPrototype());
    auto attribute = std::make_shared<BufferAttribute>(Float32Array{1, 2, 3}, 3);
    auto wrapped = nativeAttribute(attribute);
    add("BufferAttribute", wrapped);
    // This materializes the separately managed TypedArray/ArrayBuffer modules.
    // Their weak cache fix is an explicit dependency, not a skipped root.
    check(wrapped.get("array").get("length").strictEquals(3), "Array field is unusable");
    return entries;
}
void operations() {
    auto vector = nativeVector(std::make_shared<Vector3>(2, 3, 4));
    auto clone = vector.get("clone").call(vector);
    check(clone.get("x").strictEquals(2), "Vector clone changed");
    check(!clone.sameValue(vector), "Vector clone no longer creates a new wrapper");
    auto matrix = nativeMatrix(std::make_shared<Matrix3>());
    matrix.get("invert").call(matrix);
    check(matrix.get("elements").get("0").strictEquals(1), "Matrix method changed");
    auto plane = nativePlane(std::make_shared<Plane>(Vector3(1, 0, 0), -4));
    plane.get("coplanarPoint").call(plane, {vector});
    check(vector.get("x").strictEquals(4), "Plane method changed");
    auto texture = nativeTexture(std::make_shared<Texture>());
    auto textureClone = texture.get("clone").call(texture);
    check(textureClone.get("isTexture").toBoolean() && !textureClone.sameValue(texture), "Texture clone changed");
    auto material = nativeMaterial(std::make_shared<MeshBasicMaterial>());
    auto materialClone = material.get("clone").call(material);
    check(materialClone.get("type").strictEquals("MeshBasicMaterial"), "Material clone changed");
    auto attribute = nativeAttribute(std::make_shared<BufferAttribute>(Float32Array{1, 2, 3}, 3));
    const auto constructor = attribute.getPrototype().get("constructor");
    auto newAttribute = constructor.construct({NativeArray_detail::wrap(Float32Array{4, 5, 6}), 3});
    check(newAttribute.get("getX").call(newAttribute, {0}).strictEquals(4), "Recreated attribute constructor changed");
}
void nativeFieldCollection() {
    auto anchor = JSValue::object();
    std::weak_ptr<MeshStandardMaterial> nativeMaterialOwner;
    std::weak_ptr<Texture> nativeTextureOwner;
    JSWeakValue materialWrapper, textureWrapper, defines, extraProperties, userData;
    {
        auto material = std::make_shared<MeshStandardMaterial>();
        auto texture = std::make_shared<Texture>();
        nativeMaterialOwner = material;
        nativeTextureOwner = texture;
        auto wrappedMaterial = nativeMaterial(material), wrappedTexture = nativeTexture(texture);
        materialWrapper = JSWeakValue(wrappedMaterial);
        textureWrapper = JSWeakValue(wrappedTexture);
        defines = JSWeakValue(material->defines);
        extraProperties = JSWeakValue(material->extraProperties);
        userData = JSWeakValue(texture->userData);
        // Include actual back edges in the authoritative native field objects.
        material->defines.set("owner", wrappedMaterial);
        material->extraProperties.set("owner", wrappedMaterial);
        texture->userData.set("owner", wrappedTexture);
    }
    check(!nativeMaterialOwner.expired() && !nativeTextureOwner.expired(), "Native graph did not retain its fields before collection");
    anchor.collectGarbage();
    check(nativeMaterialOwner.expired() && nativeTextureOwner.expired(), "Native owner graph was not collected");
    check(materialWrapper.expired() && textureWrapper.expired(), "Native wrappers survived graph collection");
    check(defines.expired() && extraProperties.expired() && userData.expired(), "Native managed fields survived graph collection");
}
void lifetime() {
    std::vector<std::pair<std::string, JSWeakValue>> oldObjects;
    JSWeakValue oldMarker;
    std::weak_ptr<int> nativeMarker;
    {
        auto anchor = JSValue::object();
        nativeFieldCollection();
        auto owner = std::make_shared<int>(42);
        nativeMarker = owner;
        auto marker = JSValue::hostObject(owner, owner.get(), "NativeObjectModuleLifetimeMarker");
        oldMarker = JSWeakValue(marker);
        {
            auto entries = moduleObjects();
            for (auto& [name, value] : entries) {
                check(value.isObject(), name + " module value is not an object");
                value.set("lifetimeMarker", marker);
                oldObjects.emplace_back(name, JSWeakValue(value));
            }
            operations();
        }
        marker = {};
        owner.reset();
        anchor.collectGarbage();
        check(!oldMarker.expired() && !nativeMarker.expired(), "Same-heap GC lost module mutation/payload");
        {
            auto entries = moduleObjects();
            check(entries.size() == oldObjects.size(), "Module property shape changed");
            for (std::size_t i = 0; i < entries.size(); ++i) {
                auto old = oldObjects[i].second.lock();
                check(old && old->sameValue(entries[i].value), entries[i].name + " identity changed after GC");
                check(entries[i].value.get("lifetimeMarker").sameValue(*oldMarker.lock()), entries[i].name + " mutation lost");
            }
        }
        for (const auto& name : {"Material.Basic", "Material.Standard", "Texture", "Vector4", "Skeleton"}) {
            const auto entry = std::find_if(oldObjects.begin(), oldObjects.end(), [&](const auto& item) { return item.first == name; });
            check(entry != oldObjects.end(), std::string(name) + " prototype is missing");
            auto prototype = entry->second.lock();
            check(prototype->erase(std::string(name) == "Skeleton" ? "getBoneByName" : "clone"), "Could not remove cached method");
        }
        // The second factory pass creates new native materials/textures. Their
        // JS fields are traced native-owner edges and retire during collection.
        // Complete that collection before testing the remaining cache roots.
        anchor.collectGarbage();
        check(!oldMarker.expired() && !nativeMarker.expired(), "Final same-heap GC lost module payload");
        for (const auto& name : {"Material.Basic.clone", "Texture.clone", "Vector4.clone", "Skeleton.getBoneByName"}) {
            const auto entry = std::find_if(oldObjects.begin(), oldObjects.end(), [&](const auto& item) { return item.first == name; });
            check(entry != oldObjects.end(), std::string(name) + " cached function is missing");
            check(!entry->second.expired(), std::string(name) + " canonical function cache lost its record");
            check(entry->second.lock()->get("lifetimeMarker").sameValue(*oldMarker.lock()), std::string(name) + " canonical function mutation was lost");
        }
    }
    // Test before the worker exits; TLS destruction is too late.
    for (const auto& [name, weak] : oldObjects)
        check(weak.expired(), name + " cache retains heap after the last application root");
    check(oldMarker.expired() && nativeMarker.expired(), "Native marker survived final heap release");
    JSWeakValue freshHeap;
    {
        auto anchor = JSValue::object();
        freshHeap = JSWeakValue(anchor);
        auto entries = moduleObjects();
        check(entries.size() == oldObjects.size(), "Fresh module shape changed");
        for (std::size_t i = 0; i < entries.size(); ++i) {
            check(entries[i].value.get("lifetimeMarker").isUndefined(), entries[i].name + " inherited old heap mutation");
            check(oldObjects[i].second.expired(), entries[i].name + " old weak handle locked a new object");
        }
        operations();
        anchor.collectGarbage();
    }
    check(freshHeap.expired(), "Fresh heap was retained after its application roots ended");
}
}
int main() {
    std::exception_ptr error;
    std::thread worker([&] { try { lifetime(); } catch (...) { error = std::current_exception(); } });
    worker.join();
    if (error) {
        try { std::rethrow_exception(error); }
        catch (const std::exception& e) { std::cerr << "FAILED after " << checks << " checks: " << e.what() << '\n'; }
        return 2;
    }
    std::cout << "NativeObject intrinsic lifetime: " << checks << " checks passed\n";
}
