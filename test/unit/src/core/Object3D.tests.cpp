#include "core/Object3D.h"
#include "cameras/PerspectiveCamera.h"
#include "catch.hpp"
#include "objects/Group.h"
#include <cmath>

namespace {
void requireVector(const Vector3& actual, const Vector3& expected) {
    REQUIRE(actual.x == Approx(expected.x).margin(1e-10));
    REQUIRE(actual.y == Approx(expected.y).margin(1e-10));
    REQUIRE(actual.z == Approx(expected.z).margin(1e-10));
}
void requireMatrix(const Matrix4& actual, const Matrix4& expected) {
    for (std::size_t i = 0; i < 16; ++i)
        REQUIRE(actual.elements()[i] == Approx(expected.elements()[i]).margin(1e-10));
}
} // namespace

TEST_CASE("Object3D defaults and Euler quaternion synchronization", "[core]") {
    Object3D object, another;
    REQUIRE(another.id > object.id);
    REQUIRE(another.uuid != object.uuid);
    REQUIRE(object.uuid.size() == 36);
    requireVector(object.up, Vector3(0, 1, 0));
    requireVector(object.scale, Vector3(1, 1, 1));
    REQUIRE(object.matrixAutoUpdate);
    REQUIRE(object.matrixWorldAutoUpdate);
    REQUIRE_FALSE(object.matrixWorldNeedsUpdate);
    object.rotation.set(0.3, -0.8, 0.2, EulerOrder::YZX);
    Quaternion expected;
    expected.setFromEuler(object.rotation);
    REQUIRE(object.quaternion.isQuaternionEqual(expected));
    object.quaternion.setFromAxisAngle(Vector3(0, 1, 0), 0.7);
    expected.setFromEuler(object.rotation);
    REQUIRE(object.quaternion.isQuaternionEqual(expected));
    REQUIRE(object.rotation.order == EulerOrder::YZX);
    object.quaternion.setY(0.25);
    Euler expectedRotation;
    expectedRotation.setFromQuaternion(object.quaternion, EulerOrder::YZX);
    REQUIRE(object.rotation.equals(expectedRotation));
}

TEST_CASE("Object3D reparenting delivers upstream event order and retains ownership", "[core]") {
    Object3D a, b;
    auto child = std::make_shared<Object3D>();
    std::vector<std::string> events;
    child->addEventListener("added", [&](Event& event) {
        REQUIRE(event.target == child.get());
        REQUIRE(child->parent != nullptr);
        events.push_back("added");
    });
    child->addEventListener("removed", [&](Event&) {
        REQUIRE(child->parent == nullptr);
        events.push_back("removed");
    });
    a.addEventListener("childadded", [&](Event& event) {
        REQUIRE(event.target == &a);
        REQUIRE(event.child == child.get());
        events.push_back("a.childadded");
    });
    a.addEventListener("childremoved", [&](Event& event) {
        REQUIRE(event.child == child.get());
        events.push_back("a.childremoved");
    });
    b.addEventListener("childadded", [&](Event&) { events.push_back("b.childadded"); });
    a.add(child);
    // The shared_ptr argument may itself refer to the old parent's vector element.
    b.add(a.children.front());
    REQUIRE(a.children.empty());
    REQUIRE(child->parent == &b);
    REQUIRE(b.children.size() == 1);
    REQUIRE(events == std::vector<std::string>{"added", "a.childadded", "removed", "a.childremoved",
                                               "added", "b.childadded"});
    b.remove(child);
    REQUIRE(child->parent == nullptr);
    REQUIRE(b.children.empty());
}

TEST_CASE("Object3D rejects ownership cycles and detaches surviving children", "[core]") {
    auto parent = std::make_shared<Object3D>();
    auto child = std::make_shared<Object3D>();
    parent->add(child);
    REQUIRE_THROWS_AS(parent->add(parent), std::invalid_argument);
    REQUIRE_THROWS_AS(child->add(parent), std::invalid_argument);
    REQUIRE_THROWS_AS(parent->add(nullptr), std::invalid_argument);
    parent.reset();
    REQUIRE(child->parent == nullptr);
}

TEST_CASE("Object3D local transformations and parent world transformations", "[core]") {
    Object3D parent;
    auto child = std::make_shared<Object3D>();
    parent.position.set(10, 20, 30);
    parent.rotateZ(MATH_PI / 2);
    parent.scale.setScalar(2);
    child->position.set(2, 3, 4);
    parent.add(child);
    Vector3 result;
    child->getWorldPosition(result);
    requireVector(result, Vector3(4, 24, 38));
    child->getWorldScale(result);
    requireVector(result, Vector3(2, 2, 2));
    result.set(1, -3, 2);
    const Vector3 local(result);
    child->localToWorld(result);
    child->worldToLocal(result);
    requireVector(result, local);
    child->rotateY(MATH_PI / 2).translateZ(3);
    requireVector(child->position, Vector3(5, 3, 4));
}

TEST_CASE("Object3D lookAt and attach account for rotated parents", "[core]") {
    Object3D oldParent, newParent;
    auto child = std::make_shared<Object3D>();
    oldParent.position.set(3, -2, 5);
    oldParent.rotateY(0.8);
    newParent.position.set(-4, 1, 2);
    newParent.rotateZ(-0.5);
    newParent.scale.setScalar(2);
    child->position.set(1, 2, -3);
    oldParent.add(child);
    child->lookAt(Vector3(7, 6, 8));
    Vector3 direction, position, expected;
    child->getWorldPosition(position);
    expected.subVectors(Vector3(7, 6, 8), position);
    expected.normalize();
    child->getWorldDirection(direction);
    requireVector(direction, expected);
    const Matrix4 world(child->matrixWorld);
    newParent.attach(child);
    REQUIRE(oldParent.children.empty());
    REQUIRE(child->parent == &newParent);
    requireMatrix(child->matrixWorld, world);
}

TEST_CASE("Object3D pivot composition and application match upstream", "[core]") {
    Object3D object;
    object.position.set(2, 3, 4);
    object.pivot = Vector3(1, -1, 2);
    object.rotation.set(0, 0, MATH_PI / 2);
    object.scale.set(2, 3, 4);
    object.updateMatrix();
    Vector3 mapped(*object.pivot);
    mapped.applyMatrix4(object.matrix);
    requireVector(mapped, Vector3(3, 2, 6));
    Matrix4 transform;
    transform.makeTranslation(7, 8, 9);
    object.applyMatrix4(transform);
    requireVector(object.position, Vector3(9, 11, 13));
    object.updateMatrix();
    mapped.copy(*object.pivot);
    mapped.applyMatrix4(object.matrix);
    requireVector(mapped, Vector3(10, 10, 15));
}

TEST_CASE("Object3D world matrix controls and force propagation", "[core]") {
    Object3D parent;
    auto child = std::make_shared<Object3D>();
    parent.add(child);
    parent.matrixAutoUpdate = child->matrixAutoUpdate = false;
    parent.matrix.makeTranslation(1, 2, 3);
    child->matrix.makeTranslation(4, 5, 6);
    parent.updateMatrixWorld();
    requireMatrix(parent.matrixWorld, Matrix4());
    parent.updateMatrixWorld(true);
    REQUIRE(child->matrixWorld.elements()[12] == Approx(5));
    REQUIRE(child->matrixWorld.elements()[13] == Approx(7));
    parent.matrixWorldAutoUpdate = false;
    parent.matrixWorld.makeTranslation(10, 20, 30);
    parent.updateWorldMatrix(false, true, true);
    REQUIRE(parent.matrixWorld.elements()[12] == Approx(10));
    REQUIRE(child->matrixWorld.elements()[12] == Approx(14));
    REQUIRE_FALSE(parent.matrixWorldNeedsUpdate);
    child->matrix.makeTranslation(8, 9, 10);
    child->matrixWorldNeedsUpdate = true;
    child->updateWorldMatrix();
    REQUIRE(child->matrixWorld.elements()[12] == Approx(18));
}

TEST_CASE("Object3D traversal search clone and JSON retain graph semantics", "[core]") {
    Group group;
    group.name = "root";
    auto hidden = std::make_shared<Group>();
    auto camera = std::make_shared<PerspectiveCamera>();
    hidden->name = "branch";
    hidden->visible = false;
    camera->name = "camera";
    hidden->add(camera);
    group.add(hidden);
    std::vector<std::string> visited;
    group.traverse([&](Object3D& node) { visited.push_back(node.name); });
    REQUIRE(visited == std::vector<std::string>{"root", "branch", "camera"});
    visited.clear();
    group.traverseVisible([&](Object3D& node) { visited.push_back(node.name); });
    REQUIRE(visited == std::vector<std::string>{"root"});
    visited.clear();
    camera->traverseAncestors([&](Object3D& node) { visited.push_back(node.name); });
    REQUIRE(visited == std::vector<std::string>{"branch", "root"});
    REQUIRE(group.getObjectById(camera->id) == camera.get());
    REQUIRE(group.getObjectByName("camera") == camera.get());
    REQUIRE(group.getObjectByName("missing") == nullptr);
    REQUIRE(group.getObjectByProperty("isCamera", true) == camera.get());
    REQUIRE(group.getObjectByProperty("isCamera", false) == nullptr);
    REQUIRE(group.getObjectByProperty("fov", 50) == camera.get());
    std::vector<Object3D*> groups;
    group.getObjectsByProperty("isGroup", true, groups);
    REQUIRE(groups == std::vector<Object3D*>{&group, hidden.get()});
    group.userData["nested"] = JsonValue::Object{{"values", JsonValue::Array{1, 2, 3}}};
    auto cloned = group.clone();
    REQUIRE(cloned->isGroup);
    REQUIRE(cloned->uuid != group.uuid);
    REQUIRE(cloned->children[0]->parent == cloned.get());
    REQUIRE(cloned->children[0]->children[0]->isCamera);
    cloned->userData["nested"]["values"][std::size_t{0}] = 99;
    REQUIRE(group.userData.at("nested").at("values")[std::size_t{0}] == JsonValue(1));
    cloned->rotation.setX(0.5);
    REQUIRE(group.rotation.x == 0);
    REQUIRE(group.clone(false)->children.empty());
    const auto json = group.toJSON();
    REQUIRE(json.at("metadata").at("version") == JsonValue(4.7));
    REQUIRE(json.at("object").at("children")[std::size_t{0}].at("type") == JsonValue("Group"));
    REQUIRE(json.at("object").at("matrix").size() == 16);
    REQUIRE(json.dump().find("\"PerspectiveCamera\"") != std::string::npos);
}

TEST_CASE("Object3D clear and dispose dispatch lifecycle events", "[core]") {
    Object3D parent;
    auto a = std::make_shared<Object3D>(), b = std::make_shared<Object3D>();
    parent.add(a).add(b);
    int removed = 0, disposed = 0;
    parent.addEventListener("childremoved", [&](Event&) { ++removed; });
    a->addEventListener("dispose", [&](Event& event) {
        REQUIRE(event.target == a.get());
        ++disposed;
    });
    parent.clear();
    REQUIRE(removed == 2);
    REQUIRE(parent.children.empty());
    REQUIRE(a->parent == nullptr);
    REQUIRE(b->parent == nullptr);
    a->dispose();
    REQUIRE(disposed == 1);
}
