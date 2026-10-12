#include "Three.h"
#include "core/NativeObject.h"
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>

namespace {
JsonValue number(double value) {
    if (std::isnan(value))
        return "NaN";
    if (std::isinf(value))
        return value > 0 ? "Infinity" : "-Infinity";
    return value;
}
JsonValue vec(const Vector2& value) {
    return JsonValue::Array{number(value.x), number(value.y)};
}
JsonValue vec(const Vector3& value) {
    return JsonValue::Array{number(value.x), number(value.y), number(value.z)};
}
JsonValue hits(const std::vector<Intersection>& results) {
    JsonValue::Array output;
    for (const auto& hit : results) {
        JsonValue value =
            JsonValue::Object{{"distance", number(hit.distance)},
                              {"point", vec(hit.point)},
                              {"object", hit.object ? JsonValue(hit.object->name) : JsonValue()}};
        if (hit.uv)
            value["uv"] = vec(*hit.uv);
        if (hit.uv1)
            value["uv1"] = vec(*hit.uv1);
        if (hit.normal)
            value["normal"] = vec(*hit.normal);
        if (hit.barycoord)
            value["barycoord"] = vec(*hit.barycoord);
        if (hit.faceIndex)
            value["faceIndex"] = *hit.faceIndex;
        if (hit.face)
            value["face"] = JsonValue::Object{{"a", hit.face->a},
                                              {"b", hit.face->b},
                                              {"c", hit.face->c},
                                              {"normal", vec(hit.face->normal)},
                                              {"materialIndex", hit.face->materialIndex}};
        output.push_back(std::move(value));
    }
    return output;
}
JsonValue run(Mesh& mesh, const Vector3& origin = Vector3(.25, .4, 4),
              const Vector3& direction = Vector3(0, 0, -1), double near = 0,
              double far = INFINITY) {
    mesh.updateWorldMatrix(true, false);
    return hits(Raycaster(origin, direction, near, far).intersectObject(mesh));
}
JsonValue ray(const Raycaster& raycaster) {
    return JsonValue::Object{
        {"origin", vec(raycaster.ray.origin)},
        {"direction", vec(raycaster.ray.direction)},
        {"camera", raycaster.camera ? JsonValue(raycaster.camera->type) : JsonValue()}};
}
class Gate : public Object3D {
  public:
    bool raycast(const Raycaster&, std::vector<Intersection>&) override { return false; }
};
JsonValue morphMissing() {
    JsonValue::Array cases;
    for (const bool relative : {false, true}) for (int mode = 0; mode < 3; ++mode) {
        auto geometry = std::make_shared<BufferGeometry>();
        geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(std::vector<double>{1,2,3},3));
        geometry->morphAttributes["position"] = {
            std::make_shared<Float32BufferAttribute>(std::vector<double>{4,5,6},3)};
        geometry->morphTargetsRelative = relative;
        auto mesh = std::make_shared<Mesh>(geometry);
        auto influences = nativeObject(mesh).get("morphTargetInfluences");
        if (mode == 0) influences.erase("0");
        else if (mode == 1) influences.set("length", 0);
        else { influences.set("length", 3); influences.set("0", .5); }
        Vector3 point;
        mesh->getVertexPosition(0, point);
        const auto copied = std::dynamic_pointer_cast<Mesh>(mesh->clone());
        Vector3 copiedPoint; copied->getVertexPosition(0, copiedPoint);
        cases.emplace_back(JsonValue::Object{{"point", vec(point)}, {"copyPoint",vec(copiedPoint)},
            {"influences",mesh->toJSON()["object"]["morphTargetInfluences"]},
            {"copyInfluences",copied->toJSON()["object"]["morphTargetInfluences"]}});
    }
    return cases;
}
JsonValue nonFiniteSorts() {
    JsonValue::Array cases;
    Object3D empty;
    Raycaster query;
    for (const std::size_t length : {0, 1, 2, 3, 7, 31, 32, 63, 64, 65, 127, 128, 191, 512}) {
        for (std::uint32_t seed = 1; seed <= 3; ++seed) {
            std::uint32_t state = seed;
            std::vector<Intersection> target(length);
            for (std::size_t i = 0; i < length; ++i) {
                state = state * 1664525u + 1013904223u;
                const auto choice = state % 13;
                target[i].distance = choice == 0 ? NAN : choice == 1 ? INFINITY : choice == 2 ? -INFINITY :
                    choice == 3 ? -0.0 : static_cast<double>(static_cast<int>(state % 29) - 14);
                target[i].point.x = static_cast<double>(i);
            }
            query.intersectObject(empty, false, target);
            JsonValue::Array order;
            for (const auto& hit : target) order.emplace_back(hit.point.x);
            cases.emplace_back(std::move(order));
        }
    }
    return cases;
}
JsonValue reentrantChildren(bool inputArray = false) {
    class CallbackObject : public Object3D {
      public:
        std::function<void()> callback;
        bool raycast(const Raycaster&, std::vector<Intersection>&) override { callback(); return true; }
    };
    JsonValue::Array cases;
    for (int mode = 0; mode < 3; ++mode) {
        Object3D root;
        JsonValue::Array trace;
        auto first = std::make_shared<CallbackObject>(), second = std::make_shared<CallbackObject>();
        auto replacement = std::make_shared<CallbackObject>();
        first->callback = [&] {
            trace.emplace_back("first");
            if (mode == 0) root.children = {replacement};
            else if (mode == 1) root.children.clear();
            else root.children[1].reset();
        };
        second->callback = [&] { trace.emplace_back("second"); };
        replacement->callback = [&] { trace.emplace_back("replacement"); };
        root.add(first, second);
        std::string error;
        try {
            if (inputArray) Raycaster().intersectObjects(root.children);
            else Raycaster().intersectObject(root);
        } catch (const JSTypeError&) { error = "TypeError"; }
        cases.emplace_back(JsonValue::Object{{"trace", std::move(trace)}, {"error", error}});
    }
    return cases;
}
} // namespace

int main() {
    JsonValue output = JsonValue::Object{};
    auto geometry = std::make_shared<PlaneGeometry>(2, 2);
    geometry->setAttribute("uv1", std::make_shared<Float32BufferAttribute>(
                                      std::vector<float>{0, 2, 2, 2, 0, 0, 2, 0}, 2));
    auto material = std::make_shared<MeshBasicMaterial>();
    auto plane = std::make_shared<Mesh>(geometry, material);
    plane->name = "plane";
    JsonValue::Array sideCases;
    for (int side : {FrontSide, BackSide, DoubleSide}) {
        material->side = side;
        sideCases.emplace_back(JsonValue::Object{{"above", run(*plane)},
                                                 {"below", run(*plane, {.25, .4, -4}, {0, 0, 1})},
                                                 {"miss", run(*plane, {3, 3, 4})}});
    }
    output["sides"] = std::move(sideCases);
    material->side = FrontSide;
    output["distanceRanges"] = JsonValue::Array{run(*plane, {.25, .4, 4}, {0, 0, -1}, 4, 4),
                                                run(*plane, {.25, .4, 4}, {0, 0, -1}, 4.001, 9),
                                                run(*plane, {.25, .4, 4}, {0, 0, -1}, 0, 3.999),
                                                run(*plane, {.25, .4, 4}, {0, 0, -1}, 0, 4),
                                                run(*plane, {.25, .4, 4}, {0, 0, -1}, -1, 5)};
    auto boxGeometry = std::make_shared<BoxGeometry>(2, 3, 4);
    auto box = std::make_shared<Mesh>(boxGeometry, material);
    box->name = "box";
    boxGeometry->computeBoundingBox();
    output["indexedBox"] = run(*box, {0, 0, 5});
    auto expanded = std::make_shared<Mesh>(boxGeometry->toNonIndexed(), material);
    expanded->name = "expanded";
    output["nonIndexedBox"] = run(*expanded, {0, 0, 5});
    std::vector<std::shared_ptr<Material>> materials;
    for (int i = 0; i < 6; ++i) {
        auto entry = std::make_shared<MeshBasicMaterial>();
        entry->side = i == 4 ? DoubleSide : FrontSide;
        materials.push_back(entry);
    }
    auto grouped = std::make_shared<Mesh>(boxGeometry, materials);
    grouped->name = "grouped";
    output["groupedBox"] = run(*grouped, {.25, .4, 5});
    boxGeometry->setDrawRange(24, 3);
    output["drawRangeBox"] = run(*grouped, {0, 0, 5});
    boxGeometry->setDrawRange(24, 0);
    output["emptyDrawRange"] = run(*grouped, {0, 0, 5});
    boxGeometry->setDrawRange(0, std::numeric_limits<std::size_t>::max());
    auto ungrouped = std::make_shared<Mesh>(geometry, materials);
    ungrouped->name = "ungrouped";
    output["noGroups"] = run(*ungrouped);
    Group parent;
    parent.position.set(1, -2, 3);
    parent.rotation.set(.2, .3, -.1);
    parent.scale.set(-2, .5, 1.5);
    plane->position.set(2, 1, -4);
    plane->rotation.set(-.2, .1, .4);
    parent.add(plane);
    parent.updateMatrixWorld(true);
    const Vector3 local(.25, .4, 0);
    Vector3 point(local), direction(0, 0, -1), origin;
    point.applyMatrix4(plane->matrixWorld);
    direction.transformDirection(plane->matrixWorld);
    origin.copy(point).addScaledVector(direction, -5);
    output["transformedPlane"] = run(*plane, origin, direction);
    parent.remove(plane);
    plane->position.set(0, 0, 0);
    plane->rotation.set(0, 0, 0);
    auto morphGeometry = std::make_shared<PlaneGeometry>(2, 2);
    auto relative = std::make_shared<Float32BufferAttribute>(
        std::vector<float>{0, 0, 2, 0, 0, 2, 0, 0, 2, 0, 0, 2}, 3);
    morphGeometry->morphAttributes["position"] = {relative};
    morphGeometry->morphTargetsRelative = true;
    auto morph = std::make_shared<Mesh>(morphGeometry, material);
    morph->name = "morph";
    (*morph->morphTargetInfluences)[0] = .25;
    output["relativeMorph"] = run(*morph);
    morphGeometry->morphTargetsRelative = false;
    relative->copyArray(std::vector<float>{-1, 1, 2, 1, 1, 2, -1, -1, 2, 1, -1, 2});
    morphGeometry->boundingSphere.reset();
    output["absoluteMorph"] = run(*morph);
    Group root;
    auto farther = std::make_shared<Mesh>(geometry, material);
    farther->name = "farther";
    farther->position.setZ(-2);
    root.add(farther, plane);
    root.layers.set(1);
    root.updateMatrixWorld(true);
    Raycaster query({.25, .4, 4}, {0, 0, -1});
    output["recursive"] = hits(query.intersectObject(root));
    output["notRecursive"] = hits(query.intersectObject(root, false));
    plane->layers.set(1);
    output["layerFiltered"] = hits(query.intersectObject(root));
    plane->layers.set(0);
    plane->visible = false;
    material->visible = false;
    output["invisibleStillPickable"] = hits(query.intersectObject(root));
    plane->visible = true;
    material->visible = true;
    output["objects"] = hits(query.intersectObjects({farther, plane}, false));
    Intersection initial;
    initial.distance = 1;
    initial.point.set(0, 0, 3);
    initial.object = nullptr;
    std::vector<Intersection> target{initial};
    query.intersectObject(*plane, false, target);
    output["appendTarget"] = hits(target);
    Gate gate;
    gate.add(plane);
    gate.updateMatrixWorld(true);
    output["stoppedPropagation"] = hits(query.intersectObject(gate));
    JsonValue::Array cameraRays;
    for (int coordinates : {WebGLCoordinateSystem, WebGPUCoordinateSystem})
        for (bool reversed : {false, true}) {
            auto perspective = std::make_shared<PerspectiveCamera>(55, 1.6, .2, 60);
            auto orthographic = std::make_shared<OrthographicCamera>(-2, 3, 4, -1, .2, 60);
            for (const auto& camera :
                 std::vector<std::shared_ptr<Camera>>{perspective, orthographic}) {
                camera->coordinateSystem = coordinates;
                camera->reversedDepth = reversed;
                if (auto p = dynamic_cast<PerspectiveCamera*>(camera.get()))
                    p->updateProjectionMatrix();
                else
                    dynamic_cast<OrthographicCamera*>(camera.get())->updateProjectionMatrix();
                camera->position.set(2, 3, 7);
                camera->lookAt(0, -1, 0);
                camera->updateMatrixWorld(true);
                query.setFromCamera({.3, -.4}, *camera);
                cameraRays.push_back(ray(query));
            }
        }
    output["cameraRays"] = std::move(cameraRays);
    Object3D controller;
    controller.position.set(1, 2, 3);
    controller.rotation.set(.1, .2, .3);
    controller.scale.set(2, 3, 4);
    controller.updateMatrixWorld(true);
    query.setFromXRController(controller);
    output["xrRay"] = ray(query);
    output["params"] = query.params;
    output["nonFiniteSorts"] = nonFiniteSorts();
    output["reentrantChildren"] = reentrantChildren();
    output["reentrantObjects"] = reentrantChildren(true);
    output["morphMissing"] = morphMissing();
    std::cout << output.dump() << '\n';
}
