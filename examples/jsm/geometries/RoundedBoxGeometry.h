#pragma once
#include "geometries/BoxGeometry.h"

// Native port of the pinned examples/jsm/geometries/RoundedBoxGeometry.js.
//
// The derived `parameters` member hides BoxGeometry's own one because the upstream class
// replaces the object with { width, height, depth, segments, radius }; toJSON() therefore
// serializes those five fields. Like the pinned class, this port does not override copy(),
// so a clone keeps the default parameters while copying the vertex data.
class RoundedBoxGeometry : public BoxGeometry {
  public:
    struct Parameters {
        double width = 1, height = 1, depth = 1, segments = 2, radius = 0.1;
    };
    Parameters parameters;
    RoundedBoxGeometry(double width = 1, double height = 1, double depth = 1, double segments = 2,
                       double radius = 0.1);
    // `new this.constructor().copy( this )` upstream: the clone is a RoundedBoxGeometry with
    // default parameters whose vertex data is copied.
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<RoundedBoxGeometry> fromJSON(const JsonValue& data);
};
