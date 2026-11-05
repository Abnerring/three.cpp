#include "misc/MorphBlendMesh.h"

#include "core/JSValue.h"
#include "geometries/BoxGeometry.h"
#include "core/BufferAttribute.h"
#include "materials/MeshBasicMaterial.h"

#include <cmath>
#include <iostream>
#include <memory>

namespace {
// The pinned Mesh only owns a morph target dictionary when the geometry has morph attributes, so both sides
// build the geometry with three position morph targets; the dictionary then holds the keys 0, 1 and 2.
std::shared_ptr<MorphBlendMesh> makeMorphMesh() {
    auto geometry = std::make_shared<BoxGeometry>();
    auto& morphs = geometry->morphAttributes["position"];
    for (int i = 0; i < 3; ++i) {
        morphs.push_back(
            std::make_shared<BufferAttribute>(std::vector<float>(72, 0.0f), 3));
    }
    return std::make_shared<MorphBlendMesh>(geometry, std::make_shared<MeshBasicMaterial>());
}

JSValue animationValue(const MorphBlendMesh::Animation& animation) {
    auto value = JSValue::object(nullptr);
    value.set("start", JSValue(animation.start));
    value.set("end", JSValue(animation.end));
    value.set("length", JSValue(animation.length));
    value.set("fps", JSValue(animation.fps));
    value.set("duration", JSValue(animation.duration));
    value.set("lastFrame", JSValue(animation.lastFrame));
    value.set("currentFrame", JSValue(animation.currentFrame));
    value.set("active", JSValue(animation.active));
    value.set("time", JSValue(animation.time));
    value.set("direction", JSValue(static_cast<double>(animation.direction)));
    value.set("weight", JSValue(animation.weight));
    value.set("directionBackwards", JSValue(animation.directionBackwards));
    value.set("mirroredLoop", JSValue(animation.mirroredLoop));
    return value;
}

JSValue influencesValue(const MorphBlendMesh& mesh) {
    auto values = JSValue::array();
    if (!mesh.morphTargetInfluences) return values;
    std::size_t index = 0;
    for (const double influence : mesh.morphTargetInfluences->storage()->values) {
        values.set(index++, JSValue(influence));
    }
    return values;
}
}  // namespace

int main() {
    try {
        auto mesh = makeMorphMesh();
        const auto& defaultAnimation = mesh->animationsMap.at("__default");
        const auto defaultDurationIsNegativeInfinity =
            std::isinf(defaultAnimation.duration) && defaultAnimation.duration < 0;

        mesh->createAnimation("walk", 0, 2, 2);
        mesh->setAnimationFPS("walk", 4);
        const auto durationAfterFps = mesh->getAnimationDuration("walk");
        mesh->setAnimationDuration("walk", 1);
        mesh->setAnimationWeight("walk", 0.5);
        mesh->setAnimationTime("walk", 0.25);
        mesh->setAnimationDirectionBackward("walk");
        const auto backwardDirection = mesh->animationsMap.at("walk").direction;
        const auto backwardFlag = mesh->animationsMap.at("walk").directionBackwards;
        mesh->setAnimationDirectionForward("walk");

        // The pinned Mesh always owns an empty dictionary; this runtime only creates one from morph attributes.
    if (!mesh->morphTargetDictionary) {
        mesh->morphTargetDictionary = std::make_shared<std::map<std::string, std::size_t>>();
    }
    mesh->morphTargetDictionary->clear();
        (*mesh->morphTargetDictionary)["run_01"] = 0;
        (*mesh->morphTargetDictionary)["run_02"] = 1;
        (*mesh->morphTargetDictionary)["walk_01"] = 2;
        (*mesh->morphTargetDictionary)["walk_02"] = 3;
        mesh->autoCreateAnimations(30);

        // The update scenario: three influences and a three frame animation.
        auto updateMesh = makeMorphMesh();
        // The constructor already produced three zero influences.
        updateMesh->createAnimation("walk", 0, 2, 2);
        updateMesh->playAnimation("walk");
        updateMesh->update(0.1);
        const auto influencesAfterFirst = influencesValue(*updateMesh);
        updateMesh->update(0.5);
        const auto influencesAfterSecond = influencesValue(*updateMesh);

        // The mirrored loop scenario.
        auto mirroredMesh = makeMorphMesh();
        // The constructor already produced three zero influences.
        mirroredMesh->createAnimation("walk", 0, 2, 2);
        mirroredMesh->animationsMap.at("walk").mirroredLoop = true;
        mirroredMesh->playAnimation("walk");
        mirroredMesh->setAnimationTime("walk", 0.9);
        mirroredMesh->update(0.2);

        auto output = JSValue::object(nullptr);
        output.set("animationCount", JSValue(static_cast<double>(mesh->animationsMap.size())));
        output.set("listCount", JSValue(static_cast<double>(mesh->animationsList.size())));
        output.set("defaultAnimation", animationValue(defaultAnimation));
        output.set("defaultDurationIsNegativeInfinity", JSValue(defaultDurationIsNegativeInfinity));
        output.set("runAnimation", animationValue(mesh->animationsMap.at("run")));
        output.set("walkAnimation", animationValue(mesh->animationsMap.at("walk")));
        output.set("firstAnimation", JSValue(mesh->firstAnimation));
        output.set("durationAfterFps", JSValue(durationAfterFps));
        output.set("backwardDirection", JSValue(static_cast<double>(backwardDirection)));
        output.set("backwardFlag", JSValue(backwardFlag));
        output.set("influencesAfterFirst", influencesAfterFirst);
        output.set("influencesAfterSecond", influencesAfterSecond);
        output.set("mirroredAnimation", animationValue(mirroredMesh->animationsMap.at("walk")));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
