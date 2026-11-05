#include "catch.hpp"
#include "core/NativeObject.h"
#include "animation/PropertyBinding.h"
#include "objects/Mesh.h"
#include "objects/Line.h"
#include "objects/Points.h"
#include "objects/SkinnedMesh.h"
#include "lights/DirectionalLight.h"
#include "lights/SpotLight.h"
#include "textures/DataTexture.h"
#include <thread>

TEST_CASE("Native bindings write actual object fields and retain replaced resources", "[native]") {
    auto mesh = std::make_shared<Mesh>(); auto object = nativeObject(mesh);
    PropertyBinding position(object, ".position"); position.setValue(Float64Array({2, 3, 4}), 0);
    REQUIRE(mesh->position.x == 2); REQUIRE(mesh->position.y == 3); REQUIRE(mesh->position.z == 4);
    REQUIRE(mesh->matrixWorldNeedsUpdate);
    auto original = mesh->material; std::weak_ptr<Material> lifetime = original;
    PropertyBinding opacity(object, ".material.opacity"); opacity.setValue(Float64Array({.2}), 0);
    mesh->material = std::make_shared<MeshBasicMaterial>(); original.reset();
    opacity.setValue(Float64Array({.7}), 0);
    REQUIRE_FALSE(lifetime.expired()); REQUIRE(lifetime.lock()->opacity == .7);
    REQUIRE(mesh->material->opacity == 1); REQUIRE(lifetime.lock()->version == 2);
}
TEST_CASE("Native child and morph arrays retain identities across mutations", "[native]") {
    auto root = std::make_shared<Object3D>(); auto object = nativeObject(root);
    auto children = object.get("children"); auto child = std::make_shared<Object3D>();
    child->name = "child"; root->add(child);
    REQUIRE(children.sameValue(object.get("children"))); REQUIRE(children.get("length").number() == 1);
    REQUIRE(PropertyBinding::findNode(object, "child").sameValue(nativeObject(child)));
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->morphAttributes["position"].push_back(std::make_shared<Float32BufferAttribute>(std::vector<float>{0, 0, 0}, 3));
    auto mesh = std::make_shared<Mesh>(geometry); auto view = nativeObject(mesh).get("morphTargetInfluences");
    auto previous = *mesh->morphTargetInfluences;
    mesh->updateMorphTargets(); view.set("0", .8);
    REQUIRE(previous.at(0) == .8); REQUIRE(mesh->morphTargetInfluences->at(0) == 0);
    auto clone = std::dynamic_pointer_cast<Mesh>(mesh->clone());
    REQUIRE_FALSE(clone->morphTargetInfluences->sameIdentity(*mesh->morphTargetInfluences));
}
TEST_CASE("Native math methods retain receiver and destination identities", "[native]") {
    auto a = std::make_shared<Vector3>(), b = std::make_shared<Vector3>();
    auto first = nativeVector(a), second = nativeVector(b); auto array = JSValue::array();
    REQUIRE(first.get("toArray").sameValue(second.get("toArray")));
    REQUIRE_FALSE(first.hasOwn("toArray"));
    REQUIRE(first.get("toArray").call(first, {array}).sameValue(array));
    auto input = InterpolantArray::array({4, 5, 6});
    REQUIRE(first.get("fromArray").call(first, {nativeArray(input)}).sameValue(first));
    REQUIRE(a->x == 4); REQUIRE(a->y == 5); REQUIRE(a->z == 6);
    Float32Array typed({1, 2, 3});
    REQUIRE(nativeArray(typed).sameValue(nativeArray(typed)));
    REQUIRE_FALSE(nativeArray(typed).sameValue(nativeArray(typed.subarray())));
    REQUIRE(animationArray(nativeArray(typed)).sameIdentity(typed));
}
TEST_CASE("Native map binding updates an actual texture transform", "[native]") {
    auto mesh = std::make_shared<Mesh>(); auto basic = std::dynamic_pointer_cast<MeshBasicMaterial>(mesh->material);
    basic->map = std::make_shared<Texture>();
    PropertyBinding offset(nativeObject(mesh), ".map.offset"); offset.setValue(Float64Array({.25, .75}), 0);
    REQUIRE(basic->map->offset.x == .25); REQUIRE(basic->map->offset.y == .75);
    REQUIRE(nativeMaterial(basic).get("map").sameValue(nativeTexture(basic->map)));
}
TEST_CASE("Native GC retains external owners and releases replaced resources after their owners", "[native]") {
    auto root = std::make_shared<Object3D>(), middle = std::make_shared<Object3D>();
    auto mesh = std::make_shared<Mesh>(); root->add(middle); middle->add(mesh);
    auto wrapper = nativeObject(root);
    {
        auto position = wrapper.get("position"); position.set("extra", 7);
        nativeObject(mesh).set("extra", 8);
        auto material = nativeMaterial(mesh->material); material.set("extra", 9);
        auto basic = std::dynamic_pointer_cast<MeshBasicMaterial>(mesh->material); basic->map = std::make_shared<Texture>();
        auto offset = nativeTexture(basic->map).get("offset"); offset.set("extra", 10);
    }
    wrapper.collectGarbage();
    REQUIRE(wrapper.get("position").get("extra").number() == 7);
    REQUIRE(nativeObject(mesh).get("extra").number() == 8);
    REQUIRE(nativeMaterial(mesh->material).get("extra").number() == 9);
    REQUIRE(nativeTexture(std::dynamic_pointer_cast<MeshBasicMaterial>(mesh->material)->map).get("offset").get("extra").number() == 10);
    auto oldMaterial = mesh->material; mesh->material = std::make_shared<MeshBasicMaterial>();
    wrapper.collectGarbage();
    REQUIRE(wrapper.findHostObject(oldMaterial.get(), "three.Material").get("extra").number() == 9);
    std::weak_ptr<Material> oldLifetime = oldMaterial;
    const void* oldIdentity = oldMaterial.get();
    oldMaterial.reset();
    wrapper.collectGarbage();
    REQUIRE(oldLifetime.expired());
    REQUIRE(wrapper.findHostObject(oldIdentity, "three.Material").isUndefined());
}
TEST_CASE("Native array mutators respect descriptors and clone creates writable elements", "[native]") {
    auto parent = std::make_shared<Object3D>(); auto children = nativeObject(parent).get("children");
    JSPropertyDescriptor frozenLength; frozenLength.writable = false;
    children.defineProperty("length", frozenLength);
    auto child = std::make_shared<Object3D>();
    REQUIRE_THROWS_AS(parent->add(child), JSTypeError);
    REQUIRE(child->parent == parent.get()); REQUIRE(parent->children.empty());
    auto material = std::make_shared<MeshBasicMaterial>();
    auto mesh = std::make_shared<Mesh>(std::make_shared<BufferGeometry>(),std::vector<std::shared_ptr<Material>>{material});
    auto array = nativeObject(mesh).get("material");
    JSPropertyDescriptor readonly; readonly.writable = false; readonly.configurable = false;
    array.defineProperty("0",readonly); array.defineProperty("length",frozenLength);
    auto clone = std::dynamic_pointer_cast<Mesh>(mesh->clone()); auto copy = nativeObject(clone).get("material");
    REQUIRE(*copy.getOwnPropertyDescriptor("0")->writable);
    REQUIRE(*copy.getOwnPropertyDescriptor("0")->configurable);
    REQUIRE(*copy.getOwnPropertyDescriptor("length")->writable);
    copy.set("0",nativeMaterial(std::make_shared<MeshBasicMaterial>()));
    REQUIRE(clone->materials[0] != material); REQUIRE(mesh->materials[0] == material);
}
TEST_CASE("Typed native length is inherited and can be shadowed", "[native]") {
    auto array = nativeArray(Float64Array(3)); REQUIRE_FALSE(array.hasOwn("length"));
    JSPropertyDescriptor d; d.value = 7; d.configurable = true;
    array.defineProperty("length",d); REQUIRE(array.get("length").number() == 7);
    REQUIRE(animationArray(array).size() == 3); REQUIRE(array.erase("length"));
    REQUIRE(array.get("length").number() == 3);
}
TEST_CASE("Native remove retains splice partial writes when length is readonly", "[native]") {
    auto parent=std::make_shared<Object3D>(),first=std::make_shared<Object3D>(),second=std::make_shared<Object3D>();
    parent->add(first);parent->add(second);auto children=nativeObject(parent).get("children");
    JSPropertyDescriptor length;length.writable=false;children.defineProperty("length",length);
    REQUIRE_THROWS_AS(parent->remove(first),JSTypeError);
    REQUIRE(first->parent==nullptr);REQUIRE(second->parent==parent.get());
    REQUIRE(children.get("length").number()==2);REQUIRE(children.get("0").sameValue(nativeObject(second)));
    REQUIRE_FALSE(children.hasOwn("1"));
}
TEST_CASE("Native morph dictionary rejects unsupported descriptors explicitly", "[native]") {
    auto mesh=std::make_shared<Mesh>();mesh->morphTargetDictionary=std::make_shared<std::map<std::string,std::size_t>>();
    (*mesh->morphTargetDictionary)["smile"]=0;auto dictionary=nativeObject(mesh).get("morphTargetDictionary");
    JSPropertyDescriptor frozen;frozen.writable=false;REQUIRE_THROWS_AS(dictionary.defineProperty("smile",frozen),JSTypeError);
    JSPropertyDescriptor getter;getter.getter=JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&){return JSValue(3);});
    REQUIRE_THROWS_AS(dictionary.defineProperty("smile",getter),JSTypeError);
    REQUIRE(dictionary.get("smile").number()==0);dictionary.set("smile",2);REQUIRE(mesh->morphTargetDictionary->at("smile")==2);
    dictionary.set("blink",3);REQUIRE(mesh->morphTargetDictionary->at("blink")==3);
}

TEST_CASE("Native light targets preserve pending UUIDs and bind native object identities", "[native][loader]") {
    const auto verify=[](const auto& light){
        auto object=nativeObject(light);
        object.set("target","target-uuid");
        REQUIRE(object.get("target").strictEquals("target-uuid"));
        REQUIRE(light->target==nullptr);
        auto pending=JSValue::object();pending.set("owner",object);
        pending.set("toString",JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&)->JSValue{
            throw JSTypeError("Pending target must not be coerced");
        }));
        object.set("target",pending);REQUIRE(object.get("target").sameValue(pending));
        auto target=std::make_shared<Object3D>();auto targetObject=nativeObject(target);
        targetObject.set("marker",17);object.set("target",targetObject);
        REQUIRE(light->target==target);REQUIRE(object.get("target").sameValue(targetObject));
        object.collectGarbage();REQUIRE(object.get("target").get("marker").strictEquals(17));
        auto replacement=std::make_shared<Object3D>();light->target=replacement;
        REQUIRE(object.get("target").sameValue(nativeObject(replacement)));
        object.set("target",nullptr);REQUIRE(object.get("target").isNull());REQUIRE(light->target==nullptr);
        object.set("target",JSValue{});REQUIRE(object.get("target").isUndefined());
    };
    verify(std::make_shared<DirectionalLight>());verify(std::make_shared<SpotLight>());
}

TEST_CASE("Native skinned skeleton fields retain UUIDs until native binding", "[native][loader]") {
    auto mesh=std::make_shared<SkinnedMesh>();auto object=nativeObject(mesh);
    object.set("skeleton","skeleton-uuid");
    REQUIRE(object.get("skeleton").strictEquals("skeleton-uuid"));REQUIRE(mesh->skeleton==nullptr);
    auto source=std::make_shared<SkinnedMesh>();source->skeleton=std::make_shared<Skeleton>();
    auto skeleton=nativeObject(source).get("skeleton");skeleton.set("marker",23);
    object.set("skeleton",skeleton);
    REQUIRE(mesh->skeleton==source->skeleton);REQUIRE(object.get("skeleton").sameValue(skeleton));
    object.collectGarbage();REQUIRE(object.get("skeleton").get("marker").strictEquals(23));
    mesh->skeleton=std::make_shared<Skeleton>();
    REQUIRE_FALSE(object.get("skeleton").sameValue(skeleton));
    object.set("skeleton",nullptr);REQUIRE(object.get("skeleton").isNull());REQUIRE(mesh->skeleton==nullptr);
    object.set("skeleton",JSValue{});REQUIRE(object.get("skeleton").isUndefined());
}

TEST_CASE("Native morph fields accept ordinary objects and sparse arrays without callbacks", "[native][loader]") {
    const auto verify=[](const auto& primitive){
        auto object=nativeObject(primitive);auto array=JSValue::array(3);array.set("0",.25);array.set("2",.75);
        auto dictionary=JSValue::object();dictionary.set("smile",2);dictionary.set("blink",0);
        JSPropertyDescriptor accessor;accessor.getter=JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&)->JSValue{
            throw JSTypeError("Morph assignment/read must not call an element getter");
        });accessor.enumerable=accessor.configurable=true;
        dictionary.defineProperty("accessor",accessor);
        object.set("morphTargetInfluences",array);object.set("morphTargetDictionary",dictionary);
        REQUIRE(object.get("morphTargetInfluences").sameValue(array));
        REQUIRE(object.get("morphTargetDictionary").sameValue(dictionary));
        REQUIRE(primitive->morphTargetInfluences->size()==3);
        REQUIRE(primitive->morphTargetInfluences->at(0)==.25);
        REQUIRE(primitive->morphTargetInfluences->storage()->holes.contains(1));
        REQUIRE(primitive->morphTargetDictionary->at("smile")==2);
        REQUIRE_FALSE(primitive->morphTargetDictionary->contains("accessor"));
        array.set("0",.5);dictionary.set("smile",1);
        REQUIRE(object.get("morphTargetInfluences").sameValue(array));
        REQUIRE(object.get("morphTargetDictionary").sameValue(dictionary));
        REQUIRE(primitive->morphTargetInfluences->at(0)==.5);REQUIRE(primitive->morphTargetDictionary->at("smile")==1);
        array.defineProperty("1",accessor);
        object.set("morphTargetInfluences",array);REQUIRE(object.get("morphTargetInfluences").sameValue(array));
        REQUIRE_THROWS_AS(array.get("1"),JSTypeError);
        REQUIRE_THROWS_AS(dictionary.get("accessor"),JSTypeError);
        object.set("morphTargetInfluences",nullptr);REQUIRE(object.get("morphTargetInfluences").isNull());
        object.set("morphTargetDictionary",JSValue{});REQUIRE(object.get("morphTargetDictionary").isUndefined());
    };
    verify(std::make_shared<Mesh>());verify(std::make_shared<Line>());verify(std::make_shared<Points>());
}

TEST_CASE("Native updateMorphTargets retires ordinary morph field aliases", "[native][loader]") {
    auto geometry=std::make_shared<BufferGeometry>();
    geometry->morphAttributes["position"].push_back(std::make_shared<Float32BufferAttribute>(std::vector<float>{0,0,0},3));
    auto mesh=std::make_shared<Mesh>(geometry);auto object=nativeObject(mesh);
    auto array=JSValue::array(1);array.set("0",.8);auto dictionary=JSValue::object();dictionary.set("custom",0);
    object.set("morphTargetInfluences",array);object.set("morphTargetDictionary",dictionary);
    mesh->updateMorphTargets();
    REQUIRE_FALSE(object.get("morphTargetInfluences").sameValue(array));
    REQUIRE_FALSE(object.get("morphTargetDictionary").sameValue(dictionary));
    REQUIRE(mesh->morphTargetInfluences->at(0)==0);
    REQUIRE_FALSE(mesh->morphTargetDictionary->contains("custom"));
    REQUIRE(array.get("0").strictEquals(.8));REQUIRE(dictionary.get("custom").strictEquals(0));
}

TEST_CASE("Native pending references and ordinary morph cycles are traced and reclaimed", "[native][loader][gc]") {
    std::vector<bool> evidence;
    std::thread worker([&]{
        JSWeakValue meshWeak,arrayWeak,dictionaryWeak,lightWeak,targetWeak;
        std::weak_ptr<Mesh> meshOwner;std::weak_ptr<DirectionalLight> lightOwner;
        {
            auto mesh=std::make_shared<Mesh>();meshOwner=mesh;auto object=nativeObject(mesh);
            auto array=JSValue::array();array.set("owner",object);auto dictionary=JSValue::object();dictionary.set("owner",object);
            object.set("morphTargetInfluences",array);object.set("morphTargetDictionary",dictionary);
            meshWeak=JSWeakValue(object);arrayWeak=JSWeakValue(array);dictionaryWeak=JSWeakValue(dictionary);
            auto light=std::make_shared<DirectionalLight>();lightOwner=light;auto lightObject=nativeObject(light);
            auto pending=JSValue::object();pending.set("owner",lightObject);lightObject.set("target",pending);
            lightWeak=JSWeakValue(lightObject);targetWeak=JSWeakValue(pending);
            array={};dictionary={};pending={};object.collectGarbage();
            evidence.push_back(!arrayWeak.expired()&&!dictionaryWeak.expired()&&!targetWeak.expired());
            evidence.push_back(object.get("morphTargetInfluences").get("owner").sameValue(object));
            evidence.push_back(lightObject.get("target").get("owner").sameValue(lightObject));
        }
        JSValue::nativeSafepoint();
        evidence.push_back(meshWeak.expired()&&arrayWeak.expired()&&dictionaryWeak.expired());
        evidence.push_back(lightWeak.expired()&&targetWeak.expired()&&meshOwner.expired()&&lightOwner.expired());
    });worker.join();
    REQUIRE(evidence==std::vector<bool>{true,true,true,true,true});
}
