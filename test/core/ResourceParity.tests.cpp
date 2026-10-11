#include "catch.hpp"
#include "common/OrderedMap.h"
#include "core/BufferGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"
#include <cmath>
#include <iterator>
#include <limits>

static_assert(std::random_access_iterator<SharedVector<BufferGeometry::Group>::iterator>);
static_assert(std::random_access_iterator<SharedVector<BufferGeometry::Group>::const_iterator>);

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
    REQUIRE(positions->version == 1);
}

TEST_CASE("Sphere expansion preserves upstream squared-distance overflow behavior",
          "[core][parity]") {
    Sphere sphere(Vector3(0, 0, 0), 1e200);
    sphere.expandByPoint(Vector3(1e300, 0, 0));
    REQUIRE(sphere.center.x() == 0);
    REQUIRE(sphere.center.y() == 0);
    REQUIRE(sphere.center.z() == 0);
    REQUIRE(sphere.radius == 1e200);
    Sphere ordinary(Vector3(0, 0, 0), 2);
    ordinary.expandByPoint(Vector3(5, 0, 0));
    REQUIRE(ordinary.center.x() == Approx(1.5));
    REQUIRE(ordinary.radius == Approx(3.5));
}
