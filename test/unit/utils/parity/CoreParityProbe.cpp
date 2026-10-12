#include "Three.h"
#include <iostream>

namespace {
JsonValue vec(const Vector3& value) {
    return JsonValue::Array{value.x, value.y, value.z};
}
JsonValue matrix(const Matrix4& value) {
    JsonValue::Array array;
    for (auto element : value.elements())
        array.emplace_back(element);
    return array;
}
JsonValue snapshot(Object3D& object) {
    object.updateWorldMatrix(true, false);
    Vector3 direction, position, scale;
    Quaternion quaternion;
    object.getWorldPosition(position);
    object.getWorldScale(scale);
    object.getWorldQuaternion(quaternion);
    object.getWorldDirection(direction);
    return JsonValue::Object{
        {"position", vec(object.position)},
        {"scale", vec(object.scale)},
        {"rotation", JsonValue::Array{object.rotation.x, object.rotation.y, object.rotation.z,
                                      object.rotation.getOrderString()}},
        {"quaternion", JsonValue::Array{object.quaternion.x, object.quaternion.y,
                                        object.quaternion.z, object.quaternion.w}},
        {"matrix", matrix(object.matrix)},
        {"matrixWorld", matrix(object.matrixWorld)},
        {"worldPosition", vec(position)},
        {"worldScale", vec(scale)},
        {"worldDirection", vec(direction)},
        {"worldQuaternion",
         JsonValue::Array{quaternion.x, quaternion.y, quaternion.z, quaternion.w}}};
}
class CountingGeometry : public BufferGeometry {
  public:
    mutable int calls = 0;
    std::string marker;
    explicit CountingGeometry(std::string marker) : marker(std::move(marker)) {}
    JsonValue toJSON() const override {
        ++calls;
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
    JsonValue toJSON(TextureJSONContext* meta = nullptr) const override {
        ++calls;
        auto output = MeshBasicMaterial::toJSON(meta);
        output["marker"] = marker;
        return output;
    }
};
class CountingMesh : public Mesh {
  public:
    using Mesh::Mesh;
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

int main() {
    JsonValue output = JsonValue::Object{};
    Object3D parent, nextParent;
    auto child = std::make_shared<Object3D>();
    parent.position.set(-3, 2, 7);
    parent.rotateY(0.43);
    parent.scale.setScalar(1.5);
    child->position.set(2, -3, 4);
    child->rotation.set(0.2, 0.8, -0.4, EulerOrder::YZX);
    child->scale.set(1.2, 0.7, 1.4);
    child->pivot = Vector3(1, 2, -1);
    parent.add(child);
    output["initialTransform"] = snapshot(*child);
    child->lookAt(9, 3, -8);
    output["lookAtTransform"] = snapshot(*child);
    nextParent.position.set(4, -1, 2);
    nextParent.rotateZ(-0.6);
    nextParent.scale.setScalar(0.75);
    nextParent.attach(child);
    output["attachedTransform"] = snapshot(*child);
    child->rotateX(0.2).rotateOnWorldAxis(Vector3(0, 1, 0), -0.1).translateZ(2.2);
    output["rotatedTransform"] = snapshot(*child);
    Vector3 local(2, -4, 1);
    child->localToWorld(local);
    output["localToWorld"] = vec(local);
    child->worldToLocal(local);
    output["worldToLocal"] = vec(local);

    JsonValue projections = JsonValue::Array{};
    for (int coordinate : {WebGLCoordinateSystem, WebGPUCoordinateSystem}) {
        for (bool reverse : {false, true}) {
            PerspectiveCamera camera(67, 1.6, 0.3, 888);
            camera.coordinateSystem = coordinate;
            camera.reversedDepth = reverse;
            camera.zoom = 1.25;
            camera.filmOffset = 3;
            camera.setViewOffset(1920, 1080, 240, 120, 640, 480);
            camera.position.set(2, 5, 10);
            camera.scale.set(2, 3, 4);
            camera.lookAt(1, 2, -3);
            camera.updateMatrixWorld();
            Vector2 min, max, size;
            camera.getViewBounds(12, min, max);
            camera.getViewSize(12, size);
            projections.get<JsonValue::Array>().push_back(
                JsonValue::Object{{"projection", matrix(camera.projectionMatrix)},
                                  {"inverse", matrix(camera.projectionMatrixInverse)},
                                  {"viewMatrix", matrix(camera.matrixWorldInverse)},
                                  {"boundsMin", JsonValue::Array{min.x, min.y}},
                                  {"boundsMax", JsonValue::Array{max.x, max.y}},
                                  {"viewSize", JsonValue::Array{size.x, size.y}},
                                  {"focalLength", camera.getFocalLength()},
                                  {"effectiveFOV", camera.getEffectiveFOV()}});
            OrthographicCamera ortho(-4, 6, 5, -3, 0.2, 100);
            ortho.coordinateSystem = coordinate;
            ortho.reversedDepth = reverse;
            ortho.zoom = 1.4;
            ortho.setViewOffset(800, 600, 125, 45, 300, 220);
            projections.get<JsonValue::Array>().push_back(
                JsonValue::Object{{"projection", matrix(ortho.projectionMatrix)},
                                  {"inverse", matrix(ortho.projectionMatrixInverse)}});
        }
    }
    output["projections"] = std::move(projections);

    Scene scene;
    scene.uuid = "scene";
    scene.name = "sample";
    scene.background = Color(0x336699);
    scene.backgroundIntensity = 0.7;
    scene.backgroundRotation.set(0.1, 0.2, 0.3, EulerOrder::ZYX);
    scene.fog = std::make_shared<FogExp2>(0x668899, 0.02);
    auto group = std::make_shared<Group>();
    group->uuid = "group";
    group->name = "nested";
    auto box = std::make_shared<BoxGeometry>(2, 3, 4, 2, 1, 3);
    box->uuid = "box";
    auto plane = std::make_shared<PlaneGeometry>(3, 2, 2, 3);
    plane->uuid = "plane";
    auto red = std::make_shared<MeshBasicMaterial>(Color(0xaa2211));
    red->uuid = "red";
    auto blue = std::make_shared<MeshBasicMaterial>(Color(0x1144aa));
    blue->uuid = "blue";
    blue->opacity = 0.4;
    blue->transparent = true;
    auto a = std::make_shared<Mesh>(box, red);
    a->uuid = "a";
    a->name = "first";
    a->position.set(2, 3, -4);
    a->updateMatrix();
    auto b = std::make_shared<Mesh>(box, red);
    b->uuid = "b";
    b->visible = false;
    b->castShadow = true;
    b->userData["label"] = "shared";
    auto nested = std::make_shared<Mesh>(plane, std::vector<std::shared_ptr<Material>>{red, blue});
    nested->uuid = "nested";
    a->add(nested);
    group->add(a).add(b);
    scene.add(group);
    output["serializedScene"] = scene.toJSON();
    const auto clone = std::dynamic_pointer_cast<Scene>(scene.clone());
    clone->uuid = "scene-copy";
    auto clonedGroup = clone->children[0];
    clonedGroup->uuid = "group-copy";
    clonedGroup->children[0]->uuid = "a-copy";
    clonedGroup->children[1]->uuid = "b-copy";
    clonedGroup->children[0]->children[0]->uuid = "nested-copy";
    output["serializedClone"] = clone->toJSON();
    const auto perspective = std::make_shared<PerspectiveCamera>(62, 1.5, 0.2, 400);
    perspective->uuid = "perspective";
    perspective->zoom = 1.3;
    perspective->filmOffset = 2;
    perspective->setViewOffset(1200, 800, 300, 200, 400, 300);
    output["serializedPerspective"] = perspective->toJSON();
    OrthographicCamera orthographic(-4, 6, 5, -3, 0.2, 100);
    orthographic.uuid = "orthographic";
    orthographic.zoom = 1.4;
    orthographic.setViewOffset(800, 600, 125, 45, 300, 220);
    orthographic.clearViewOffset();
    output["serializedOrthographic"] = orthographic.toJSON();

    {
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
        auto branch = std::make_shared<CountingMesh>(
            collisionGeometry, std::vector<std::shared_ptr<Material>>{
                                   collisionMaterial, uniqueMaterial, uniqueMaterial});
        auto leaf = std::make_shared<CountingMesh>(uniqueGeometry, firstMaterial);
        auto sibling = std::make_shared<CountingMesh>(firstGeometry, firstMaterial);
        root.uuid = root.name = "cache-root";
        branch->uuid = branch->name = "cache-branch";
        leaf->uuid = leaf->name = "cache-leaf";
        sibling->uuid = sibling->name = "cache-sibling";
        std::vector<std::string> order;
        root.order = branch->order = leaf->order = sibling->order = &order;
        branch->add(leaf);
        root.add(branch).add(sibling);
        output["sharedResourceCache"] = root.toJSON();
        const auto calls = [&] {
            return JsonValue::Array{firstGeometry->calls,
                                    collisionGeometry->calls,
                                    uniqueGeometry->calls,
                                    firstMaterial->calls,
                                    collisionMaterial->calls,
                                    uniqueMaterial->calls,
                                    root.calls,
                                    branch->calls,
                                    leaf->calls,
                                    sibling->calls};
        };
        output["sharedResourceCalls"] = calls();
        root.toJSON();
        output["repeatedResourceCalls"] = calls();
        JsonValue::Array traversal;
        for (const auto& name : order)
            traversal.emplace_back(name);
        output["customSerializerTraversal"] = std::move(traversal);
    }
    {
        class EncodedGeometry : public CountingGeometry {
          public:
            using CountingGeometry::CountingGeometry;
            JsonValue toJSON() const override {
                auto result = CountingGeometry::toJSON();
                result["uuid"] = "encoded-uuid";
                return result;
            }
        };
        auto original = std::make_shared<EncodedGeometry>("original");
        auto collision = std::make_shared<CountingGeometry>("collision");
        auto shared = std::make_shared<CountingMaterial>("shared");
        original->uuid = collision->uuid = "identity-uuid";
        shared->uuid = "context-material";
        Mesh first(original, shared), second(collision, shared);
        first.uuid = "context-first";
        second.uuid = "context-second";
        Object3DJSONContext context;
        auto fragment = first.toJSON(&context);
        auto other = second.toJSON(&context);
        output["contextMetadataBeforeExtraction"] =
            context.geometries.at("identity-uuid").contains("metadata");
        output["explicitContextFirst"] = fragment;
        output["explicitContextSecond"] = other;
        context.extractCaches(fragment);
        output["explicitContextDocument"] = fragment;
        output["contextMetadataAfterExtraction"] =
            context.geometries.at("identity-uuid").contains("metadata");
        output["explicitContextCalls"] =
            JsonValue::Array{original->calls, collision->calls, shared->calls};
    }
    std::cout << output.dump() << '\n';
}
