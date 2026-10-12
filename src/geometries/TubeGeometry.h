#pragma once
#include "core/BufferGeometry.h"
#include "extras/core/Curve.h"

class TubeGeometry : public BufferGeometry {
  public:
    struct Parameters { std::shared_ptr<Curve<Vector3>> path; double tubularSegments, radius, radialSegments; bool closed; };
    Parameters parameters;
    std::vector<Vector3> tangents, normals, binormals;
    static std::shared_ptr<Curve<Vector3>> defaultPath();
    TubeGeometry(std::shared_ptr<Curve<Vector3>> path = defaultPath(), double tubularSegments = 64,
                 double radius = 1, double radialSegments = 8, bool closed = false);
    BufferGeometry& copy(const BufferGeometry& source) override;
    std::shared_ptr<BufferGeometry> clone() const override;
    JsonValue toJSON() const override;
    static std::shared_ptr<TubeGeometry> fromJSON(const JsonValue& data);
};
