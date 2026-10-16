#pragma once
#include "core/BufferGeometry.h"
#include <functional>

// Native port of the pinned examples/jsm/geometries/ParametricGeometry.js.
//
// The upstream `func` is a JavaScript callback that writes into a target vector; the C++
// adaptation is a std::function with the same ( u, v, target ) contract and the same
// default body. `parameters.func` cannot appear in JSON (JSON.stringify drops function
// values), so toJSON() writes only the numeric fields like the serialized upstream object.
class ParametricGeometry : public BufferGeometry {
  public:
    using Func = std::function<void(double u, double v, Vector3& target)>;
    struct Parameters {
        Func func;
        double slices = 8;
        double stacks = 8;
    };
    Parameters parameters;

    explicit ParametricGeometry(Func func = {}, double slices = 8, double stacks = 8);
    BufferGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
};
