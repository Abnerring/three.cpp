#include "catch.hpp"
#include "geometries/ShapeGeometry.h"
#include "geometries/ExtrudeGeometry.h"
#include "scenes/Scene.h"
#include "geometries/BoxGeometry.h"
#include "geometries/PlaneGeometry.h"
#include "core/OrderedMap.h"
#include "core/BufferGeometry.h"
#include "core/InterleavedBufferAttribute.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"
#include <bit>
#include <cmath>
#include <iterator>
#include <limits>

static_assert(std::random_access_iterator<SharedVector<BufferGeometry::Group>::iterator>);
static_assert(std::random_access_iterator<SharedVector<BufferGeometry::Group>::const_iterator>);

namespace {
void requireSpecialNumbers(const JsonValue& values) {
    REQUIRE(values.size() == 4);
    REQUIRE(values[0].get<double>() == 0);
    REQUIRE(std::signbit(values[0].get<double>()));
    REQUIRE(std::isnan(values[1].get<double>()));
    REQUIRE(values[2].get<double>() == std::numeric_limits<double>::infinity());
    REQUIRE_FALSE(std::signbit(values[2].get<double>()));
    REQUIRE(values[3].get<double>() == -std::numeric_limits<double>::infinity());
    REQUIRE(std::signbit(values[3].get<double>()));
}
}

TEST_CASE("Geometry data snapshots retain raw Float32 numbers through attribute clones",
          "[core][geometry][parity][number-sign]") {
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    const auto infinity = std::numeric_limits<float>::infinity();
    BufferGeometry geometry;
    auto attribute = std::make_shared<Float32BufferAttribute>(
        std::vector<float>{-0.0f, nan, infinity, -infinity}, 1);
    geometry.setAttribute("special", attribute);
    geometry.morphAttributes["special"] = {attribute};
    requireSpecialNumbers(attribute->toJSON().at("array"));
    const auto serialized = geometry.toJSON();
    requireSpecialNumbers(serialized.at("data").at("attributes").at("special").at("array"));
    requireSpecialNumbers(serialized.at("data").at("morphAttributes").at("special")[0].at("array"));

    const auto clone = geometry.clone();
    REQUIRE(clone->getAttribute("special") != attribute);
    const auto clonedData = clone->toJSON();
    requireSpecialNumbers(clonedData.at("data").at("attributes").at("special").at("array"));
    requireSpecialNumbers(clonedData.at("data").at("morphAttributes").at("special")[0].at("array"));
    attribute->setX(0, 1);
    REQUIRE(std::signbit(clone->getAttribute("special")->getX(0)));
    REQUIRE(serialized.at("data").at("attributes").at("special").at("array")[0].get<double>() == 0);
    REQUIRE(std::signbit(serialized.at("data").at("attributes").at("special").at("array")[0].get<double>()));
}

TEST_CASE("Detached interleaved JSON retains raw numbers and geometry JSON retains storage bits",
          "[core][geometry][interleaved-attribute][parity][number-sign]") {
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    const auto infinity = std::numeric_limits<float>::infinity();
    Float32Array values({91, -0.0f, 92, nan, 93, infinity, 94, -infinity});
    InterleavedBuffer data(values, 2);
    auto attribute = std::make_shared<InterleavedBufferAttribute>(data, 1, 1);
    const auto detached = attribute->toJSON();
    requireSpecialNumbers(detached.at("array"));
    const auto deinterleaved = attribute->cloneAttribute();
    REQUIRE(std::dynamic_pointer_cast<BufferAttribute>(deinterleaved));
    requireSpecialNumbers(deinterleaved->toJSON().at("array"));

    BufferGeometry geometry;
    geometry.setAttribute("special", attribute);
    const auto clone = geometry.clone();
    const auto clonedAttribute = std::dynamic_pointer_cast<InterleavedBufferAttribute>(clone->getAttribute("special"));
    REQUIRE(clonedAttribute != nullptr);
    REQUIRE_FALSE(clonedAttribute->data.strictEquals(data));
    requireSpecialNumbers(clonedAttribute->toJSON().at("array"));
    for (const auto* candidate : {&geometry, clone.get()}) {
        const auto json = candidate->toJSON().at("data");
        const auto& descriptor = json.at("attributes").at("special");
        REQUIRE(descriptor.at("isInterleavedBufferAttribute").get<bool>());
        const auto& buffer = json.at("interleavedBuffers").at(descriptor.at("data").get<std::string>());
        const auto& words = json.at("arrayBuffers").at(buffer.at("buffer").get<std::string>());
        REQUIRE(words.size() == values.size());
        for (std::size_t i = 0; i < values.size(); ++i)
            REQUIRE(words[i].get<double>() == double(std::bit_cast<std::uint32_t>(values.at(i))));
    }
    attribute->setX(0, 1);
    REQUIRE(std::signbit(clonedAttribute->getX(0)));
    REQUIRE(std::signbit(deinterleaved->getX(0)));
    requireSpecialNumbers(detached.at("array"));
}

TEST_CASE("Geometry groups retain the upstream JSON stringify number normalization",
          "[core][geometry][groups][parity][number-sign]") {
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto infinity = std::numeric_limits<double>::infinity();
    BufferGeometry geometry;
    geometry.addGroup(-0.0, nan, infinity);
    geometry.addGroup(-infinity, -0.0, nan);
    const auto clone = geometry.clone();
    REQUIRE(std::signbit(geometry.groups[0].start));
    REQUIRE(std::signbit(clone->groups[0].start));
    REQUIRE(std::isnan(clone->groups[0].count));
    REQUIRE(clone->groups[0].materialIndex == infinity);
    REQUIRE(clone->groups[1].start == -infinity);
    REQUIRE(std::signbit(clone->groups[1].count));
    for (const auto* candidate : {&geometry, clone.get()}) {
        const auto json = candidate->toJSON();
        const auto& groups = json.at("data").at("groups");
        REQUIRE(groups.size() == 2);
        REQUIRE(groups[0].at("start").get<double>() == 0);
        REQUIRE_FALSE(std::signbit(groups[0].at("start").get<double>()));
        REQUIRE(groups[0].at("count").isNull());
        REQUIRE(groups[0].at("materialIndex").isNull());
        REQUIRE(groups[1].at("start").isNull());
        REQUIRE(groups[1].at("count").get<double>() == 0);
        REQUIRE_FALSE(std::signbit(groups[1].at("count").get<double>()));
        REQUIRE(groups[1].at("materialIndex").isNull());
    }
}

TEST_CASE("Parameterized geometry JSON skips invalid unused buffers and reconstructs defaults") {
    BoxGeometry box(2, 3, 4, 2, 1, 3);
    PlaneGeometry plane(5, 6, 2, 3);
    box.attributes["unused"] = nullptr; plane.attributes["unused"] = nullptr;
    auto boxJSON = box.toJSON(), planeJSON = plane.toJSON();
    REQUIRE(!boxJSON.contains("data")); REQUIRE(!planeJSON.contains("data"));
    auto boxCopy = BoxGeometry::fromJSON(boxJSON), boxDefault = BoxGeometry::fromJSON(JsonValue::Object{});
    auto planeCopy = PlaneGeometry::fromJSON(planeJSON);
    REQUIRE(boxCopy->parameters.width == 2); REQUIRE(boxCopy->parameters.depthSegments == 3);
    REQUIRE(boxDefault->parameters.width == 1); REQUIRE(planeCopy->parameters.heightSegments == 3);
}
TEST_CASE("Shape geometries export shared shapes through the scene resource cache") {
    struct CountingShape : Shape {
        mutable int calls = 0;
        JsonValue toJSON() const override { ++calls; return Shape::toJSON(); }
    };
    auto shape = std::make_shared<CountingShape>();
    shape->moveTo(0, 0); shape->lineTo(1, 0); shape->lineTo(0, 1); shape->closePath();
    auto list = std::make_shared<ShapeGeometry::ShapeList>(); list->push_back(shape); list->push_back(shape);
    auto first = std::make_shared<ShapeGeometry>(shape), second = std::make_shared<ShapeGeometry>(list);
    Scene scene; auto material = std::make_shared<MeshBasicMaterial>();
    scene.add(std::make_shared<Mesh>(first, material)); scene.add(std::make_shared<Mesh>(second, material));
    scene.add(std::make_shared<Mesh>(std::make_shared<ExtrudeGeometry>(shape), material));
    auto json = scene.toJSON();
    REQUIRE(shape->calls == 1); REQUIRE(json.at("shapes").size() == 1);
    REQUIRE(json.at("geometries").size() == 3);
    REQUIRE(json.at("shapes")[0].at("uuid").get<std::string>() == shape->uuid);
}

TEST_CASE("Geometry groups retain object identity across storage growth clear and replacement",
          "[core][geometry][groups]") {
    BufferGeometry geometry;
    geometry.addGroup(0, 6, 0);
    auto queued = geometry.groups.owner(geometry.groups[0]);
    REQUIRE(queued != nullptr);
    REQUIRE(queued.get() == &geometry.groups[0]);
    for (std::size_t i = 0; i < 128; ++i)
        geometry.addGroup(i + 6, 3, 1);
    REQUIRE(&geometry.groups[0] == queued.get());
    geometry.groups[0].count = 0;
    REQUIRE(queued->count == 0);
    const auto& constGroups = geometry.groups;
    const auto constOwner = constGroups.owner(constGroups[0]);
    REQUIRE(constOwner.get() == queued.get());
    // A copied array retains references to its original group objects, as a JS slice does.
    auto oldArray = geometry.groups;
    geometry.clearGroups();
    geometry.addGroup(30, 9, 2);
    REQUIRE(geometry.groups.size() == 1);
    REQUIRE(&geometry.groups[0] != queued.get());
    REQUIRE(queued->start == 0);
    REQUIRE(queued->count == 0);
    oldArray[0].count = 3;
    REQUIRE(queued->count == 3);
    REQUIRE(geometry.groups[0].count == 9);
    REQUIRE(geometry.groups.owner(*queued) == nullptr);
    auto replacementOwner = geometry.groups.owner(geometry.groups[0]);
    geometry.groups.replace(0, {40, 12, 3});
    REQUIRE(&geometry.groups[0] != replacementOwner.get());
    REQUIRE(replacementOwner->count == 9);
    REQUIRE(geometry.groups[0].count == 12);
}

TEST_CASE("Geometry copy and nonindexed conversion create independent group objects",
          "[core][geometry][groups]") {
    BufferGeometry source;
    source.setAttribute("position", std::make_shared<Float32BufferAttribute>(
                                        std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0}, 3));
    source.setIndex(std::vector<std::uint32_t>{0, 1, 2});
    source.addGroup(0, 3, 2);
    auto originalOwner = source.groups.owner(source.groups[0]);
    BufferGeometry copy;
    copy.addGroup(20, 6, 4);
    auto previousOwner = copy.groups.owner(copy.groups[0]);
    copy.copy(source);
    REQUIRE(copy.groups.size() == 1);
    REQUIRE(&copy.groups[0] != originalOwner.get());
    REQUIRE(&copy.groups[0] != previousOwner.get());
    REQUIRE(previousOwner->start == 20);
    REQUIRE(copy.groups[0].materialIndex == 2);
    auto converted = source.toNonIndexed();
    REQUIRE(converted->groups.size() == 1);
    REQUIRE(&converted->groups[0] != originalOwner.get());
    source.groups[0].count = 0;
    REQUIRE(originalOwner->count == 0);
    REQUIRE(copy.groups[0].count == 3);
    REQUIRE(converted->groups[0].count == 3);
    copy.groups[0].materialIndex = 5;
    REQUIRE(source.groups[0].materialIndex == 2);
    REQUIRE(converted->groups[0].materialIndex == 2);
}

TEST_CASE("Shared group arrays preserve value access and owner lifetime during edits",
          "[core][geometry][groups]") {
    using Group = BufferGeometry::Group;
    SharedVector<Group> groups{{0, 3, 0}, {3, 6, 1}};
    auto first = groups.owner(groups.front());
    auto second = groups.owner(groups.back());
    auto inserted = groups.insert(groups.begin() + 1, Group{9, 3, 2});
    REQUIRE(inserted->start == 9);
    REQUIRE(groups.end() - groups.begin() == 3);
    REQUIRE((groups.cbegin() + 2)->start == 3);
    for (auto& group : groups)
        group.count = 0;
    REQUIRE(first->count == 0);
    REQUIRE(second->count == 0);
    groups.erase(groups.begin());
    REQUIRE(groups.front().start == 9);
    REQUIRE(first->start == 0);
    groups = {{12, 6, 3}};
    REQUIRE(groups.size() == 1);
    REQUIRE(second->start == 3);
    REQUIRE(groups.owner(*second) == nullptr);
    auto final = groups.owner(groups[0]);
    groups.clear();
    REQUIRE(final->count == 6);
}

TEST_CASE("JsonValue explicit aliases preserve nested references and ordinary value copies",
          "[core][parity]") {
    JsonValue value = JsonValue::Object{{"nested", JsonValue::Object{{"count", 1}}}};
    auto& retained = value["nested"];
    auto alias = value.sharedCopy();
    retained["count"] = 2;
    REQUIRE(alias.at("nested").at("count") == JsonValue(2));
    alias["nested"]["count"] = 3;
    REQUIRE(value.at("nested").at("count") == JsonValue(3));
    auto independent = alias;
    independent["nested"]["count"] = 4;
    REQUIRE(value.at("nested").at("count") == JsonValue(3));
    value = JsonValue::Object{{"replacement", true}};
    REQUIRE(alias.at("nested").at("count") == JsonValue(3));
    REQUIRE_FALSE(alias.contains("replacement"));
    JsonValue moved = std::move(independent);
    REQUIRE(moved.at("nested").at("count") == JsonValue(4));
    independent["reuse"] = true;
    REQUIRE(independent.at("reuse") == JsonValue(true));
}

TEST_CASE("Geometry copies share userData while material copies own independent JSON",
          "[core][parity]") {
    BufferGeometry geometry;
    geometry.userData["nested"] = JsonValue::Object{{"count", 1}};
    auto clone = geometry.clone();
    geometry.userData["nested"]["count"] = 3;
    REQUIRE(clone->userData.at("nested").at("count") == JsonValue(3));
    clone->userData["nested"]["count"] = 7;
    REQUIRE(geometry.userData.at("nested").at("count") == JsonValue(7));
    REQUIRE(&geometry.userData.storage() == &clone->userData.storage());
    MeshBasicMaterial material;
    material.userData["nested"] = JsonValue::Object{{"count", 1}};
    auto copiedMaterial = material.clone();
    material.userData["nested"]["count"] = 2;
    copiedMaterial->userData["nested"]["count"] = 3;
    REQUIRE(material.userData.at("nested").at("count") == JsonValue(2));
    REQUIRE(copiedMaterial->userData.at("nested").at("count") == JsonValue(3));
    REQUIRE(&material.userData.storage() != &copiedMaterial->userData.storage());
}

TEST_CASE("Geometry attribute maps follow JavaScript insertion and array-index key order",
          "[core][parity]") {
    OrderedMap<int> values;
    values["z"] = 1;
    auto& retained = values["z"];
    for (const auto* key : {"10", "2", "01", "0", "a"})
        values[key] = 2;
    retained = 5;
    REQUIRE(values.at("z") == 5);
    std::vector<std::string> keys;
    for (const auto& [key, value] : values)
        keys.push_back(key);
    REQUIRE(keys == std::vector<std::string>{"0", "2", "10", "z", "01", "a"});
    values["z"] = 9; // Replacing a value does not move the key.
    values.erase("01");
    values["01"] = 4;
    keys.clear();
    for (const auto& [key, value] : values)
        keys.push_back(key);
    REQUIRE(keys == std::vector<std::string>{"0", "2", "10", "z", "a", "01"});
    OrderedMap<int> copied;
    copied = values;
    REQUIRE(copied.begin()->first == "0");
    REQUIRE(copied.at("z") == 9);
    auto geometry = std::make_shared<BufferGeometry>();
    auto position = std::make_shared<Float32BufferAttribute>(std::vector<float>{0, 0, 0}, 3);
    auto normal = std::make_shared<Float32BufferAttribute>(std::vector<float>{0, 0, 1}, 3);
    position->name = "position-shape";
    normal->name = "normal-shape";
    geometry->morphAttributes["position"] = {position};
    geometry->morphAttributes["normal"] = {normal};
    Mesh mesh(geometry);
    REQUIRE(mesh.morphTargetDictionary->contains("position-shape"));
    REQUIRE_FALSE(mesh.morphTargetDictionary->contains("normal-shape"));
    geometry->morphAttributes.clear();
    geometry->morphAttributes["normal"] = {normal};
    geometry->morphAttributes["position"] = {position};
    mesh.updateMorphTargets();
    REQUIRE(mesh.morphTargetDictionary->contains("normal-shape"));
    REQUIRE_FALSE(mesh.morphTargetDictionary->contains("position-shape"));
}

TEST_CASE("setFromPoints uses JavaScript z truthiness for NaN and negative zero",
          "[core][parity]") {
    BufferGeometry geometry;
    geometry.setFromPoints(
        {Vector3(1, 2, std::numeric_limits<double>::quiet_NaN()), Vector3(3, 4, -0.0)});
    auto positions = geometry.getAttribute("position");
    REQUIRE(positions->getZ(0) == 0);
    REQUIRE_FALSE(std::signbit(positions->getZ(1)));
    geometry.setFromPoints({Vector3(5, 6, std::numeric_limits<double>::quiet_NaN()),
                            Vector3(7, 8, std::numeric_limits<double>::infinity())});
    REQUIRE(positions->getZ(0) == 0);
    REQUIRE(std::isinf(positions->getZ(1)));
    REQUIRE(positions->getProperty("version").number() == 1);
}

TEST_CASE("Sphere expansion preserves upstream squared-distance overflow behavior",
          "[core][parity]") {
    Sphere sphere(Vector3(0, 0, 0), 1e200);
    sphere.expandByPoint(Vector3(1e300, 0, 0));
    REQUIRE(sphere.center.x == 0);
    REQUIRE(sphere.center.y == 0);
    REQUIRE(sphere.center.z == 0);
    REQUIRE(sphere.radius == 1e200);
    Sphere ordinary(Vector3(0, 0, 0), 2);
    ordinary.expandByPoint(Vector3(5, 0, 0));
    REQUIRE(ordinary.center.x == Approx(1.5));
    REQUIRE(ordinary.radius == Approx(3.5));
}
