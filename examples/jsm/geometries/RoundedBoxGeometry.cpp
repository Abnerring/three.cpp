#include "geometries/RoundedBoxGeometry.h"
#include "math/MathUtils.h"
#include "core/BufferAttribute.h"
#include "math/MathUtils.h"
#include "math/JSNumberMath.h"
#include "math/MathUtils.h"
#include <cmath>
#include <cstddef>
#include <vector>

namespace {
constexpr double PI = 3.141592653589793;
// The module-level temporary of the pinned source: one shared vector per owning thread.
thread_local Vector3 tempNormal;
// Math.sign, which returns the argument for NaN and keeps the sign of zero.
double jsSign(double value) {
    if (std::isnan(value)) return value;
    if (value == 0) return value;
    return value > 0 ? 1. : -1.;
}
// `_tempNormal[ uvAxis ]` / `_tempNormal[ projectionAxis ]` with 0 = x, 1 = y, 2 = z.
double componentAt(const Vector3& value, int axis) {
    return axis == 0 ? value.x : axis == 1 ? value.y : value.z;
}
void setComponentAt(Vector3& value, int axis, double component) {
    if (axis == 0) value.x = component;
    else if (axis == 1) value.y = component;
    else value.z = component;
}
double getUv(const Vector3& faceDirVector, const Vector3& normal, int uvAxis, int projectionAxis,
             double radius, double sideLength) {
    const double totArcLength = 2 * PI * radius / 4;
    // length of the planes between the arcs on each axis
    const double centerLength = MathUtils::max(sideLength - 2 * radius, 0.);
    const double halfArc = PI / 4;
    // Get the vector projected onto the Y plane
    tempNormal.copy(normal);
    setComponentAt(tempNormal, projectionAxis, 0);
    tempNormal.normalize();
    // total amount of UV space alloted to a single arc
    const double arcUvRatio = 0.5 * totArcLength / (totArcLength + centerLength);
    // the distance along one arc the point is at
    const double arcAngleRatio = 1.0 - (tempNormal.angleTo(faceDirVector) / halfArc);
    if (jsSign(componentAt(tempNormal, uvAxis)) == 1) {
        return arcAngleRatio * arcUvRatio;
    }
    // total amount of UV space alloted to the plane between the arcs
    const double lenUv = centerLength / (totArcLength + centerLength);
    return lenUv + arcUvRatio + arcUvRatio * (1.0 - arcAngleRatio);
}
}

RoundedBoxGeometry::RoundedBoxGeometry(double width, double height, double depth, double segments, double radius)
    : BoxGeometry(1, 1, 1, segments * 2 + 1, segments * 2 + 1, segments * 2 + 1) {
    // calculate total segments needed &
    // ensure it's odd so that we have a plane connecting the rounded corners
    const double totalSegments = segments * 2 + 1;
    // ensure radius isn't bigger than shortest side
    radius = MathUtils::min(MathUtils::min(width / 2, height / 2), MathUtils::min(depth / 2, radius));
    type = "RoundedBoxGeometry";
    parameters = {width, height, depth, segments, radius};
    // if totalSegments is 1, no rounding is needed - return regular box
    if (totalSegments == 1) return;
    const auto geometry2 = toNonIndexed();
    index = nullptr;
    setAttribute("position", geometry2->getAttribute("position"));
    setAttribute("normal", geometry2->getAttribute("normal"));
    setAttribute("uv", geometry2->getAttribute("uv"));
    //
    Vector3 position, normal;
    const Vector3 box = Vector3(width, height, depth).divideScalar(2).subScalar(radius);
    auto positionAttribute = std::dynamic_pointer_cast<BufferAttribute>(getAttribute("position"));
    auto normalAttribute = std::dynamic_pointer_cast<BufferAttribute>(getAttribute("normal"));
    auto uvAttribute = std::dynamic_pointer_cast<BufferAttribute>(getAttribute("uv"));
    if (!positionAttribute || !normalAttribute || !uvAttribute)
        throw JSTypeError("RoundedBoxGeometry: expected position, normal and uv attributes");
    auto& positions = std::get<Float32Array>(positionAttribute->array);
    auto& normals = std::get<Float32Array>(normalAttribute->array);
    auto& uvs = std::get<Float32Array>(uvAttribute->array);
    const double faceTris = double(positions.size()) / 6;
    Vector3 faceDirVector;
    const double halfSegmentSize = 0.5 / totalSegments;
    for (double i = 0, j = 0; i < double(positions.size()); i += 3, j += 2) {
        const std::size_t positionIndex = std::size_t(i);
        const std::size_t uvIndex = std::size_t(j);
        position.set(positions[positionIndex], positions[positionIndex + 1], positions[positionIndex + 2]);
        normal.copy(position);
        normal.x -= jsSign(normal.x) * halfSegmentSize;
        normal.y -= jsSign(normal.y) * halfSegmentSize;
        normal.z -= jsSign(normal.z) * halfSegmentSize;
        normal.normalize();
        positions[positionIndex + 0] = float(box.x * jsSign(position.x) + normal.x * radius);
        positions[positionIndex + 1] = float(box.y * jsSign(position.y) + normal.y * radius);
        positions[positionIndex + 2] = float(box.z * jsSign(position.z) + normal.z * radius);
        normals[positionIndex + 0] = float(normal.x);
        normals[positionIndex + 1] = float(normal.y);
        normals[positionIndex + 2] = float(normal.z);
        const double side = std::floor(i / faceTris);
        switch (int(side)) {
            case 0: // right
                // generate UVs along Z then Y
                faceDirVector.set(1, 0, 0);
                uvs[uvIndex + 0] = float(getUv(faceDirVector, normal, 2, 1, radius, depth));
                uvs[uvIndex + 1] = float(1.0 - getUv(faceDirVector, normal, 1, 2, radius, height));
                break;
            case 1: // left
                // generate UVs along Z then Y
                faceDirVector.set(-1, 0, 0);
                uvs[uvIndex + 0] = float(1.0 - getUv(faceDirVector, normal, 2, 1, radius, depth));
                uvs[uvIndex + 1] = float(1.0 - getUv(faceDirVector, normal, 1, 2, radius, height));
                break;
            case 2: // top
                // generate UVs along X then Z
                faceDirVector.set(0, 1, 0);
                uvs[uvIndex + 0] = float(1.0 - getUv(faceDirVector, normal, 0, 2, radius, width));
                uvs[uvIndex + 1] = float(getUv(faceDirVector, normal, 2, 0, radius, depth));
                break;
            case 3: // bottom
                // generate UVs along X then Z
                faceDirVector.set(0, -1, 0);
                uvs[uvIndex + 0] = float(1.0 - getUv(faceDirVector, normal, 0, 2, radius, width));
                uvs[uvIndex + 1] = float(1.0 - getUv(faceDirVector, normal, 2, 0, radius, depth));
                break;
            case 4: // front
                // generate UVs along X then Y
                faceDirVector.set(0, 0, 1);
                uvs[uvIndex + 0] = float(1.0 - getUv(faceDirVector, normal, 0, 1, radius, width));
                uvs[uvIndex + 1] = float(1.0 - getUv(faceDirVector, normal, 1, 0, radius, height));
                break;
            case 5: // back
                // generate UVs along X then Y
                faceDirVector.set(0, 0, -1);
                uvs[uvIndex + 0] = float(getUv(faceDirVector, normal, 0, 1, radius, width));
                uvs[uvIndex + 1] = float(1.0 - getUv(faceDirVector, normal, 1, 0, radius, height));
                break;
        }
    }
}

std::shared_ptr<BufferGeometry> RoundedBoxGeometry::clone() const {
    // The pinned clone is `new this.constructor().copy( this )`: the new instance starts from
    // the default parameters and copy() does not overwrite them.
    auto result = std::make_shared<RoundedBoxGeometry>();
    result->copy(*this);
    return result;
}

JsonValue RoundedBoxGeometry::toJSON() const {
    auto result = BufferGeometry::toJSON();
    result["width"] = parameters.width;
    result["height"] = parameters.height;
    result["depth"] = parameters.depth;
    result["segments"] = parameters.segments;
    result["radius"] = parameters.radius;
    return result;
}

std::shared_ptr<RoundedBoxGeometry> RoundedBoxGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<RoundedBoxGeometry>(data.at("width").get<double>(), data.at("height").get<double>(),
        data.at("depth").get<double>(), data.at("segments").get<double>(), data.at("radius").get<double>());
}
