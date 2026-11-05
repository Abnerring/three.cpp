#include "misc/MorphAnimMesh.h"

#include "animation/AnimationClip.h"
#include "animation/tracks/NumberKeyframeTrack.h"
#include "core/JSValue.h"
#include "geometries/BoxGeometry.h"
#include "materials/MeshBasicMaterial.h"

#include <iostream>
#include <memory>
#include <string>

namespace {
std::shared_ptr<AnimationClip> makeClip(const std::string& name, double duration, int trackCount) {
    auto tracks = std::make_shared<AnimationClip::Tracks>();
    for (int i = 0; i < trackCount; ++i) {
        tracks->push_back(std::make_shared<NumberKeyframeTrack>(
            ".position[x]", std::vector<double>{0, duration}, std::vector<double>{0, 1}));
    }
    return std::make_shared<AnimationClip>(name, duration, tracks);
}
}  // namespace

int main() {
    try {
        auto mesh = MorphAnimMesh::create(std::make_shared<BoxGeometry>(),
                                          std::make_shared<MeshBasicMaterial>());
        const auto initialActiveNull = mesh->activeAction.isNull();
        const auto hasRoot = mesh->mixer->getRoot().toBoolean();

        mesh->setDirectionBackward();
        const double backwardScale = mesh->mixer->timeScale.toNumber();
        mesh->setDirectionForward();
        const double forwardScale = mesh->mixer->timeScale.toNumber();

        mesh->animations.push_back(makeClip("walk", 2.0, 2));
        mesh->playAnimation("walk", 30.0);
        const auto playing = mesh->activeAction.toBoolean();
        auto action = AnimationAction::fromObject(mesh->activeAction);
        const double actionTimeScale = action.timeScale.toNumber();
        const auto running = action.isRunning();

        bool updateThrew = false;
        try {
            mesh->updateAnimation(0.1);
        } catch (...) {
            updateThrew = true;
        }

        bool missingThrew = false;
        std::string missingMessage;
        try {
            mesh->playAnimation("missing", 30.0);
        } catch (const std::exception& error) {
            missingThrew = true;
            missingMessage = error.what();
        }

        auto output = JSValue::object(nullptr);
        output.set("type", JSValue(mesh->type));
        output.set("initialActiveNull", initialActiveNull);
        output.set("hasRoot", hasRoot);
        output.set("backwardScale", JSValue(backwardScale));
        output.set("forwardScale", JSValue(forwardScale));
        output.set("playing", playing);
        output.set("actionTimeScale", JSValue(actionTimeScale));
        output.set("running", running);
        output.set("updateThrew", updateThrew);
        output.set("missingThrew", missingThrew);
        output.set("missingMessageHasPrefix",
                   JSValue(missingMessage.rfind("THREE.MorphAnimMesh: animations[missing]", 0) == 0));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
