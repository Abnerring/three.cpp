#include "scenes/Scene.h"
#include "catch.hpp"
#include "geometries/BoxGeometry.h"
#include "geometries/PlaneGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"
#include "scenes/FogExp2.h"
#include <functional>

namespace {
class CountingGeometry : public BufferGeometry {
  public:
    mutable int calls = 0;
    std::string marker;
    std::function<void()> onSerialize;
    explicit CountingGeometry(std::string marker) : marker(std::move(marker)) {}
    JsonValue toJSON() const override {
        ++calls;
        if (onSerialize)
            onSerialize();
        auto output = BufferGeometry::toJSON();
        output["marker"] = marker;
        return output;
    }
};
class CountingMaterial : public MeshBasicMaterial {
  public:
    mutable int calls = 0;
    std::string marker;
    explicit CountingMaterial(std::string marker) : marker(std::move(marker)) {}
    JsonValue toJSON() const override {
        ++calls;
        auto output = MeshBasicMaterial::toJSON();
        output["marker"] = marker;
        return output;
    }
};
class CountingMesh : public Mesh {
  public:
    using Mesh::Mesh;
    using Mesh::toJSON;
    mutable int calls = 0;
    std::vector<std::string>* order = nullptr;
    JsonValue toJSON(Object3DJSONContext* meta = nullptr) const override {
        ++calls;
        if (order)
            order->push_back(name);
        auto output = Mesh::toJSON(meta);
        output["object"]["extension"] = name;
        return output;
    }
};
} // namespace

TEST_CASE("Scene defaults cloning and JSON match supported upstream properties", "[scenes]") {
    Scene scene;
    REQUIRE(scene.isScene);
    REQUIRE(scene.type == "Scene");
    REQUIRE_FALSE(scene.background.has_value());
    REQUIRE(scene.environment == nullptr);
    REQUIRE(scene.fog == nullptr);
    REQUIRE(scene.overrideMaterial == nullptr);
    scene.background = Color(0x336699);
    scene.backgroundBlurriness = 0.4;
    scene.backgroundIntensity = 0.7;
    scene.environmentIntensity = 2;
    scene.backgroundRotation.set(0.1, 0.2, 0.3, EulerOrder::ZYX);
    scene.fog = std::make_shared<Fog>(0x102030, 3, 40);
    scene.overrideMaterial = std::make_shared<MeshBasicMaterial>();
    scene.add(std::make_shared<Object3D>());
    auto copy = std::dynamic_pointer_cast<Scene>(scene.clone());
    REQUIRE(copy != nullptr);
    REQUIRE(copy->background->getHex() == scene.background->getHex());
    REQUIRE(copy->fog != scene.fog);
    REQUIRE(copy->overrideMaterial != scene.overrideMaterial);
    REQUIRE(copy->backgroundRotation.equals(scene.backgroundRotation));
    REQUIRE(copy->children[0]->parent == copy.get());
    copy->background->setHex(0xffffff);
    REQUIRE(scene.background->getHex() == 0x336699);
    const auto json = scene.toJSON();
    REQUIRE(json.at("object").at("background") == JsonValue(0x336699));
    REQUIRE(json.at("object").at("fog").at("near") == JsonValue(3));
    REQUIRE(json.at("object").at("backgroundRotation")[std::size_t{3}] == JsonValue("ZYX"));
    REQUIRE(json.at("object").at("environmentIntensity") == JsonValue(2));
}

TEST_CASE("Fog and FogExp2 clones and serialized color preserve upstream behavior", "[scenes]") {
    Fog fog(0x123456, 2, 80);
    fog.name = "linear";
    const auto linear = std::dynamic_pointer_cast<Fog>(fog.clone());
    REQUIRE(linear != nullptr);
    REQUIRE(linear->color.getHex() == 0x123456);
    REQUIRE(linear->near == 2);
    REQUIRE(linear->far == 80);
    REQUIRE(linear->name.empty()); // Upstream clone reconstructs and omits name.
    REQUIRE(fog.toJSON().at("name") == JsonValue("linear"));
    FogExp2 exp("red", 0.02);
    exp.name = "exp";
    const auto exponential = std::dynamic_pointer_cast<FogExp2>(exp.clone());
    REQUIRE(exponential != nullptr);
    REQUIRE(exponential->density == Approx(0.02));
    REQUIRE(exponential->color.getHex() == 0xff0000);
    REQUIRE(exponential->name.empty());
    REQUIRE(exp.toJSON().at("density") == JsonValue(0.02));
}

TEST_CASE("Scene JSON recursively deduplicates shared mesh resources in discovery order",
          "[scenes]") {
    Scene scene;
    auto box = std::make_shared<BoxGeometry>();
    auto plane = std::make_shared<PlaneGeometry>();
    auto material = std::make_shared<MeshBasicMaterial>();
    auto a = std::make_shared<Mesh>(box, material);
    auto b = std::make_shared<Mesh>(box, material);
    auto nested =
        std::make_shared<Mesh>(plane, std::vector<std::shared_ptr<Material>>{material, material});
    a->add(nested);
    scene.add(a).add(b);
    const auto json = scene.toJSON();
    REQUIRE(json.at("geometries").size() == 2);
    REQUIRE(json.at("materials").size() == 1);
    REQUIRE(json.at("geometries")[std::size_t{0}].at("uuid") == JsonValue(box->uuid));
    REQUIRE(json.at("geometries")[std::size_t{1}].at("uuid") == JsonValue(plane->uuid));
    REQUIRE_FALSE(json.at("geometries")[std::size_t{0}].contains("metadata"));
    REQUIRE_FALSE(json.at("materials")[std::size_t{0}].contains("metadata"));
    const auto& nestedJSON =
        json.at("object").at("children")[std::size_t{0}].at("children")[std::size_t{0}];
    REQUIRE(nestedJSON.at("material").size() == 2);
    REQUIRE(nestedJSON.at("material")[std::size_t{0}] == JsonValue(material->uuid));
    Mesh emptyMaterials(box, std::vector<std::shared_ptr<Material>>{});
    REQUIRE(emptyMaterials.toJSON().at("object").at("material").isArray());
    REQUIRE(emptyMaterials.toJSON().at("object").at("material").size() == 0);
    REQUIRE_FALSE(emptyMaterials.toJSON().contains("materials"));
}

TEST_CASE("Object JSON invokes virtual serializers once per UUID and preserves first discovery",
          "[scenes][json]") {
    auto firstGeometry = std::make_shared<CountingGeometry>("first");
    auto collisionGeometry = std::make_shared<CountingGeometry>("collision");
    auto uniqueGeometry = std::make_shared<CountingGeometry>("unique");
    firstGeometry->uuid = collisionGeometry->uuid = "geometry-first";
    uniqueGeometry->uuid = "geometry-second";
    auto firstMaterial = std::make_shared<CountingMaterial>("first");
    auto collisionMaterial = std::make_shared<CountingMaterial>("collision");
    auto uniqueMaterial = std::make_shared<CountingMaterial>("unique");
    firstMaterial->uuid = collisionMaterial->uuid = "material-first";
    uniqueMaterial->uuid = "material-second";
    CountingMesh root(firstGeometry, firstMaterial);
    auto child = std::make_shared<CountingMesh>(
        collisionGeometry,
        std::vector<std::shared_ptr<Material>>{collisionMaterial, uniqueMaterial, uniqueMaterial});
    auto nested = std::make_shared<CountingMesh>(uniqueGeometry, firstMaterial);
    auto sibling = std::make_shared<CountingMesh>(firstGeometry, firstMaterial);
    root.name = "root";
    child->name = "child";
    nested->name = "nested";
    sibling->name = "sibling";
    std::vector<std::string> order;
    root.order = child->order = nested->order = sibling->order = &order;
    child->add(nested);
    root.add(child).add(sibling);
    const auto json = root.toJSON();
    REQUIRE(order == std::vector<std::string>{"root", "child", "nested", "sibling"});
    REQUIRE(root.calls == 1);
    REQUIRE(child->calls == 1);
    REQUIRE(nested->calls == 1);
    REQUIRE(sibling->calls == 1);
    REQUIRE(firstGeometry->calls == 1);
    REQUIRE(collisionGeometry->calls == 0);
    REQUIRE(uniqueGeometry->calls == 1);
    REQUIRE(firstMaterial->calls == 1);
    REQUIRE(collisionMaterial->calls == 0);
    REQUIRE(uniqueMaterial->calls == 1);
    REQUIRE(json.at("geometries").size() == 2);
    REQUIRE(json.at("geometries")[std::size_t{0}].at("marker") == JsonValue("first"));
    REQUIRE(json.at("geometries")[std::size_t{1}].at("uuid") == JsonValue("geometry-second"));
    REQUIRE(json.at("materials").size() == 2);
    REQUIRE(json.at("materials")[std::size_t{0}].at("marker") == JsonValue("first"));
    REQUIRE(json.at("materials")[std::size_t{1}].at("uuid") == JsonValue("material-second"));
    REQUIRE(json.at("object").at("children")[std::size_t{0}].at("extension") == JsonValue("child"));
    REQUIRE_FALSE(json.at("geometries")[std::size_t{0}].contains("metadata"));
    REQUIRE_FALSE(json.at("materials")[std::size_t{0}].contains("metadata"));
    root.toJSON();
    REQUIRE(firstGeometry->calls == 2);
    REQUIRE(firstMaterial->calls == 2);
    REQUIRE(collisionGeometry->calls == 0);
    REQUIRE(collisionMaterial->calls == 0);
}

TEST_CASE("Explicit object JSON contexts retain metadata until extraction and use original UUIDs",
          "[scenes][json]") {
    class EncodedGeometry : public CountingGeometry {
      public:
        using CountingGeometry::CountingGeometry;
        JsonValue toJSON() const override {
            auto output = CountingGeometry::toJSON();
            output["uuid"] = "encoded-uuid";
            return output;
        }
    };
    auto original = std::make_shared<EncodedGeometry>("original");
    auto collision = std::make_shared<CountingGeometry>("collision");
    original->uuid = collision->uuid = "identity-uuid";
    auto material = std::make_shared<CountingMaterial>("shared");
    Mesh first(original, material), second(collision, material);
    Object3DJSONContext context;
    auto fragment = first.toJSON(&context);
    auto other = second.toJSON(&context);
    REQUIRE_FALSE(fragment.contains("metadata"));
    REQUIRE_FALSE(fragment.contains("geometries"));
    REQUIRE_FALSE(other.contains("materials"));
    REQUIRE(fragment.at("object").at("geometry") == JsonValue("identity-uuid"));
    REQUIRE(original->calls == 1);
    REQUIRE(collision->calls == 0);
    REQUIRE(material->calls == 1);
    REQUIRE(context.geometries.at("identity-uuid").contains("metadata"));
    context.extractCaches(fragment);
    REQUIRE_FALSE(context.geometries.at("identity-uuid").contains("metadata"));
    REQUIRE(fragment.at("geometries")[std::size_t{0}].at("uuid") == JsonValue("encoded-uuid"));
    const auto compatibility = first.toJSON(false);
    REQUIRE_FALSE(compatibility.contains("metadata"));
    REQUIRE(compatibility.contains("geometries"));
    REQUIRE(first.toJSON(true).contains("metadata"));
}

TEST_CASE("Reentrant object JSON documents use independent resource contexts", "[scenes][json]") {
    auto outerGeometry = std::make_shared<CountingGeometry>("outer");
    auto innerGeometry = std::make_shared<CountingGeometry>("inner");
    auto outerMaterial = std::make_shared<CountingMaterial>("outer");
    auto innerMaterial = std::make_shared<CountingMaterial>("inner");
    outerGeometry->uuid = innerGeometry->uuid = "same-geometry";
    outerMaterial->uuid = innerMaterial->uuid = "same-material";
    Mesh outer(outerGeometry, outerMaterial), inner(innerGeometry, innerMaterial);
    JsonValue innerJSON;
    outerGeometry->onSerialize = [&] { innerJSON = inner.toJSON(); };
    const auto outerJSON = outer.toJSON();
    REQUIRE(outerGeometry->calls == 1);
    REQUIRE(innerGeometry->calls == 1);
    REQUIRE(outerMaterial->calls == 1);
    REQUIRE(innerMaterial->calls == 1);
    REQUIRE(outerJSON.at("geometries")[std::size_t{0}].at("marker") == JsonValue("outer"));
    REQUIRE(innerJSON.at("geometries")[std::size_t{0}].at("marker") == JsonValue("inner"));
    REQUIRE(outerJSON.at("materials")[std::size_t{0}].at("marker") == JsonValue("outer"));
    REQUIRE(innerJSON.at("materials")[std::size_t{0}].at("marker") == JsonValue("inner"));
}

TEST_CASE("Object JSON cache extraction matches JavaScript order for numeric UUID keys",
          "[scenes][json]") {
    auto first = std::make_shared<CountingGeometry>("two");
    auto second = std::make_shared<CountingGeometry>("one");
    first->uuid = "2";
    second->uuid = "1";
    auto material = std::make_shared<MeshBasicMaterial>();
    Scene scene;
    scene.add(std::make_shared<Mesh>(first, material))
        .add(std::make_shared<Mesh>(second, material));
    const auto json = scene.toJSON();
    REQUIRE(json.at("geometries")[std::size_t{0}].at("uuid") == JsonValue("1"));
    REQUIRE(json.at("geometries")[std::size_t{1}].at("uuid") == JsonValue("2"));
}
