#include "loaders/ObjectLoader.h"

#include "animation/AnimationClip.h"
#include "core/BufferGeometry.h"
#include "core/NativeObject.h"
#include "core/Object3D.h"
#include "loaders/BufferGeometryLoader.h"
#include "loaders/MaterialLoader.h"
#include "objects/Group.h"
#include "objects/Mesh.h"
#include "scenes/Scene.h"
#include <stdexcept>

namespace {
// The maths types read a std::vector, so the JSON array is copied into one. three keeps every matrix array
// in column-major order and so does this conversion: it is a straight copy, no transposition anywhere.
std::vector<double> numbers(const JSValue& value) {
    std::vector<double> out;
    const auto length = static_cast<int>(value.length());
    for (int index = 0; index < length; ++index) out.push_back(value.get(index).toNumber());
    return out;
}
}

ObjectLoader::Result ObjectLoader::parse(const JSValue& json) {
    Result result;
    const auto animations = json.get("animations");
    if (!animations.isUndefined())
        for (double index = 0; index < animations.length(); index += 1) {
            const auto clip = AnimationClip::parse(animations.get(index));
            clips_[clip->uuid.toString()] = clip;
            result.animations.push_back(clip);
        }
    const auto geometries = json.get("geometries");
    if (!geometries.isUndefined())
        for (double index = 0; index < geometries.length(); index += 1) {
            const auto data = geometries.get(index);
            const auto type = data.get("type").toString();
            std::shared_ptr<BufferGeometry> geometry;
            if (type == "BufferGeometry" || type == "InstancedBufferGeometry") {
                BufferGeometryLoader loader;
                geometry = loader.parseGeometry(data);
            } else {
                // Every other type comes from Geometries[type].fromJSON upstream, and none of those are ported.
                throw std::runtime_error("ObjectLoader: unknown geometry type \"" + type + "\"");
            }
            geometry->uuid = data.get("uuid").toString();
            if (!data.get("name").isUndefined()) geometry->name = data.get("name").toString();
            if (const auto userData = data.get("userData").toJSON()) geometry->userData = *userData;
            geometries_[geometry->uuid] = geometry;
        }
    const auto materialList = json.get("materials");
    if (!materialList.isUndefined()) {
        MaterialLoader loader;
        for (double index = 0; index < materialList.length(); index += 1) {
            const auto data = materialList.get(index);
            const auto uuid = data.get("uuid").toString();
            if (materials_.find(uuid) == materials_.end()) materials_[uuid] = loader.parseMaterial(data);
        }
    }
    const auto object = json.get("object");
    if (object.isUndefined()) throw std::runtime_error("ObjectLoader: the document has no object");
    result.object = parseObject(object);
    return result;
}

std::shared_ptr<Object3D> ObjectLoader::parseObject(const JSValue& data) {
    const auto type = data.get("type").toString();
    std::shared_ptr<Object3D> object;
    if (type == "Scene") object = std::make_shared<Scene>();
    else if (type == "Group") object = std::make_shared<Group>();
    else if (type == "Mesh") {
        const auto mesh = std::make_shared<Mesh>();
        const auto geometry = geometries_.find(data.get("geometry").toString());
        if (geometry != geometries_.end()) mesh->geometry = geometry->second;
        const auto material = materials_.find(data.get("material").toString());
        if (material != materials_.end()) mesh->material = material->second;
        object = mesh;
    } else if (type == "Object3D" || type.empty()) object = std::make_shared<Object3D>();
    else if (type == "Line" || type == "LineSegments" || type == "LineLoop" || type == "Points" ||
             type == "Sprite" || type == "LOD" || type == "Bone" || type == "SkinnedMesh" ||
             type == "InstancedMesh" || type == "PerspectiveCamera" || type == "OrthographicCamera" ||
             type == "AmbientLight" || type == "DirectionalLight" || type == "PointLight" ||
             type == "SpotLight" || type == "HemisphereLight")
        throw std::runtime_error("ObjectLoader: object type \"" + type + "\" is not ported yet");
    else if (!type.empty() && type.find("Light") != std::string::npos)
        throw std::runtime_error("ObjectLoader: object type \"" + type + "\" is not ported yet");
    else object = std::make_shared<Object3D>();   // upstream falls back to Object3D for an unknown type

    // The epilogue below is shared by every type and mirrors the upstream tail line for line.
    object->uuid = data.get("uuid").toString();
    if (!data.get("name").isUndefined()) object->name = data.get("name").toString();
    if (!data.get("pivot").isUndefined()) {
        Vector3 pivot;
        pivot.fromArray(numbers(data.get("pivot")));
        object->pivot = pivot;
    }
    const auto matrix = data.get("matrix");
    if (!matrix.isUndefined()) {
        object->matrix.fromArray(numbers(matrix));
        if (!data.get("matrixAutoUpdate").isUndefined())
            object->matrixAutoUpdate = data.get("matrixAutoUpdate").toBoolean();
        if (object->matrixAutoUpdate) {
            object->matrix.decompose(object->position, object->quaternion, object->scale);
            if (object->pivot) {
                object->position.copy(*object->pivot);
                object->position.applyMatrix4(object->matrix);
                object->position.sub(*object->pivot);
            }
        }
    } else {
        if (!data.get("position").isUndefined()) object->position.fromArray(numbers(data.get("position")));
        if (!data.get("rotation").isUndefined()) object->rotation.fromArray(numbers(data.get("rotation")));
        if (!data.get("quaternion").isUndefined()) object->quaternion.fromArray(numbers(data.get("quaternion")));
        if (!data.get("scale").isUndefined()) object->scale.fromArray(numbers(data.get("scale")));
    }
    if (!data.get("up").isUndefined()) object->up.fromArray(numbers(data.get("up")));
    if (!data.get("morphTargetDictionary").isUndefined() || !data.get("morphTargetInfluences").isUndefined())
        throw std::runtime_error("ObjectLoader: morph target dictionaries are not ported yet");
    if (!data.get("castShadow").isUndefined()) object->castShadow = data.get("castShadow").toBoolean();
    if (!data.get("receiveShadow").isUndefined()) object->receiveShadow = data.get("receiveShadow").toBoolean();
    if (!data.get("shadow").isUndefined())
        throw std::runtime_error("ObjectLoader: light shadows are not ported in this slice");
    if (!data.get("visible").isUndefined()) object->visible = data.get("visible").toBoolean();
    if (!data.get("frustumCulled").isUndefined()) object->frustumCulled = data.get("frustumCulled").toBoolean();
    if (!data.get("renderOrder").isUndefined()) object->renderOrder = data.get("renderOrder").toNumber();
    if (!data.get("static").isUndefined()) object->isStatic = data.get("static").toBoolean();
    if (const auto userData = data.get("userData").toJSON()) object->userData = *userData;
    if (!data.get("layers").isUndefined()) object->layers.mask = data.get("layers").toNumber();
    const auto children = data.get("children");
    if (!children.isUndefined())
        for (double index = 0; index < children.length(); index += 1)
            object->add(parseObject(children.get(index)));
    const auto objectAnimations = data.get("animations");
    if (!objectAnimations.isUndefined())
        for (double index = 0; index < objectAnimations.length(); index += 1) {
            const auto found = clips_.find(objectAnimations.get(index).toString());
            if (found != clips_.end()) object->animations.push_back(found->second);
        }
    return object;
}
