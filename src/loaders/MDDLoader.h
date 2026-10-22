#pragma once
#include "animation/AnimationClip.h"
#include "core/BufferAttribute.h"

#include <cstdint>
#include <memory>
#include <vector>

// Port of examples/jsm/loaders/MDDLoader.js at the pinned revision. MDD is a big endian
// binary point cache: a frame count, a vertex count, one float per frame time, then three
// floats per vertex per frame. Only parsing is ported; reading the file is host work.
class MDDLoader {
  public:
    struct Result {
        std::vector<std::shared_ptr<BufferAttribute>> morphTargets;
        std::shared_ptr<AnimationClip> clip;
    };

    // Parses the raw bytes. Throws std::runtime_error when the data ends before the header
    // says it should.
    Result parse(const std::vector<std::uint8_t>& data) const;
};
