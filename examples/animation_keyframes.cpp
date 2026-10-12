#include "Three.Core.h"
#include <iostream>

int main() {
    auto object = std::make_shared<Object3D>();
    auto track = std::make_shared<VectorKeyframeTrack>(
        ".position", Float32Array({0, 1}), Float32Array({0, 0, 0, 2, 0, 0}));
    auto tracks = std::make_shared<AnimationClip::Tracks>();
    tracks->push_back(track);
    auto clip = std::make_shared<AnimationClip>("move", 1, tracks);

    AnimationMixer mixer(object);
    auto action = mixer.clipAction(clip);
    action.setLoop(LoopOnce, 1);
    action.clampWhenFinished = true;
    action.play();

    for (int frame = 0; frame < 4; ++frame) {
        mixer.update(0.25);
        std::cout << "time=" << mixer.time.toNumber()
                  << " x=" << object->position.x << '\n';
    }
    action.stop();
    mixer.uncacheClip(clip);
    std::cout << "restored x=" << object->position.x << '\n';
}
