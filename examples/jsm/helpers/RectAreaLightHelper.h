#pragma once

#include "math/Color.h"
#include "objects/Line.h"

#include <memory>
#include <optional>

class RectAreaLight;

// Native port of examples/jsm/helpers/RectAreaLightHelper.js from the pinned three.js revision
// (see upstream.json).
//
// The helper draws a rectangle outline plus a filled quad for a RectAreaLight. On a world matrix
// update it scales the outline to half the light size, copies either the explicit colour or the
// light colour scaled by intensity (normalised when a channel exceeds one) onto both materials and
// then places itself at the light world matrix.
class RectAreaLightHelper : public Line {
  public:
    std::shared_ptr<RectAreaLight> light;
    // Upstream assigns the optional constructor colour and tests it for undefined.
    std::optional<Color> color;

    explicit RectAreaLightHelper(const std::shared_ptr<RectAreaLight>& light,
                                 std::optional<Color> color = std::nullopt);

    void updateMatrixWorld(bool force = false) override;
    void dispose() override;
};
