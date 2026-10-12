#pragma once

#include "core/JsonValue.h"
#include "core/JSValue.h"
#include "extras/core/Interpolations.h"
#include "math/Matrix4.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace Curve_detail {
inline double clamp(double value, double minimum, double maximum) {
    if (std::isnan(value)) return value;
    return std::max(minimum, std::min(maximum, value));
}
inline double quadraticBezier(double t, double p0, double p1, double p2) {
    return QuadraticBezier(t, p0, p1, p2);
}
inline double cubicBezier(double t, double p0, double p1, double p2, double p3) {
    return CubicBezier(t, p0, p1, p2, p3);
}
template <class Vector> JsonValue vectorJSON(const Vector& vector) {
    JsonValue::Array result;
    for (const auto component : vector.toArray()) result.emplace_back(component);
    return result;
}
template <class Vector> void vectorFromJSON(Vector& vector, const JsonValue& json) {
    std::vector<double> values;
    for (const auto& component : json.get<JsonValue::Array>())
        values.push_back(component.isNull() ? 0 : component.get<double>());
    constexpr std::size_t dimensions = std::is_same_v<Vector, Vector2> ? 2 : 3;
    if (values.size() < dimensions)
        values.resize(dimensions, std::numeric_limits<double>::quiet_NaN());
    vector.fromArray(values);
}
} // namespace Curve_detail

// The template is the C++ static adaptation of Curve's Vector2/Vector3 return type.
// Supplied targets are returned by reference; omitted targets are returned by value.
template <class Vector = Vector3> class Curve {
    static_assert(std::is_same_v<Vector, Vector2> || std::is_same_v<Vector, Vector3>);
  public:
    using VectorType = Vector;
    using Points = std::vector<std::shared_ptr<Vector>>;
    struct FrenetFrames {
        std::vector<Vector3> tangents, normals, binormals;
    };

    std::string type = "Curve";
    double arcLengthDivisions = 200;
    bool needsUpdate = false;
    std::shared_ptr<std::vector<double>> cacheArcLengths;

    virtual ~Curve() = default;
    virtual Vector& getPoint(double, Vector&) {
        // Upstream returns undefined for this abstract operation; typed C++ cannot.
        throw std::logic_error("Curve: .getPoint() not implemented.");
    }
    Vector getPoint(double t) {
        Vector target;
        return getPoint(t, target);
    }
    virtual Vector& getPointAt(double u, Vector& target) {
        return getPoint(getUtoTmapping(u), target);
    }
    Vector getPointAt(double u) {
        Vector target;
        return getPointAt(u, target);
    }
    virtual Points getPoints() { return getPoints(5); }
    virtual Points getPoints(double divisions) {
        Points points;
        for (double d = 0; d <= divisions; ++d) points.push_back(std::make_shared<Vector>(getPoint(d / divisions)));
        return points;
    }
    virtual Points getSpacedPoints() { return getSpacedPoints(5); }
    virtual Points getSpacedPoints(double divisions) {
        Points points;
        for (double d = 0; d <= divisions; ++d) points.push_back(std::make_shared<Vector>(getPointAt(d / divisions)));
        return points;
    }
    virtual double getLength() {
        const auto lengths = getLengths();
        return lengths->empty() ? std::numeric_limits<double>::quiet_NaN() : lengths->back();
    }
    std::shared_ptr<std::vector<double>> getLengths() {
        return getLengths(arcLengthDivisions);
    }
    std::shared_ptr<std::vector<double>> getLengths(double divisions) {
        if (cacheArcLengths && static_cast<double>(cacheArcLengths->size()) == divisions + 1 &&
            !needsUpdate) return cacheArcLengths;
        needsUpdate = false;
        auto cache = std::make_shared<std::vector<double>>();
        Vector last = getPoint(0);
        double sum = 0;
        cache->push_back(0);
        for (double p = 1; p <= divisions; ++p) {
            Vector current = getPoint(p / divisions);
            sum += current.distanceTo(last);
            cache->push_back(sum);
            last = current;
        }
        cacheArcLengths = cache;
        return cache;
    }
    virtual void updateArcLengths() {
        needsUpdate = true;
        getLengths();
    }
    double getUtoTmapping(double u, std::optional<double> distance = std::nullopt) {
        const auto arcLengths = getLengths();
        const double il = static_cast<double>(arcLengths->size());
        // Reading an absent JS array entry yields undefined, which becomes NaN in arithmetic.
        const auto lengthAt = [&](double index) {
            return index >= 0 && index < il
                       ? (*arcLengths)[static_cast<std::size_t>(index)]
                       : std::numeric_limits<double>::quiet_NaN();
        };
        // JS tests distance by truthiness: zero and NaN use u, including negative zero.
        const double targetArcLength = distance && *distance != 0 && !std::isnan(*distance)
                                           ? *distance : u * lengthAt(il - 1);
        double i = 0, low = 0, high = il - 1;
        while (low <= high) {
            i = std::floor(low + (high - low) / 2);
            const double comparison = (*arcLengths)[static_cast<std::size_t>(i)] - targetArcLength;
            if (comparison < 0) low = i + 1;
            else if (comparison > 0) high = i - 1;
            else { high = i; break; }
        }
        i = high;
        if (lengthAt(i) == targetArcLength) return i / (il - 1);
        const double lengthBefore = lengthAt(i);
        const double lengthAfter = lengthAt(i + 1);
        const double segmentLength = lengthAfter - lengthBefore;
        const double segmentFraction = (targetArcLength - lengthBefore) / segmentLength;
        return (i + segmentFraction) / (il - 1);
    }
    virtual Vector& getTangent(double t, Vector& target) {
        const double delta = 0.0001;
        double t1 = t - delta, t2 = t + delta;
        if (t1 < 0) t1 = 0;
        if (t2 > 1) t2 = 1;
        const Vector pt1 = getPoint(t1), pt2 = getPoint(t2);
        return target.copy(pt2).sub(pt1).normalize();
    }
    Vector getTangent(double t) {
        Vector target;
        return getTangent(t, target);
    }
    virtual Vector& getTangentAt(double u, Vector& target) {
        return getTangent(getUtoTmapping(u), target);
    }
    Vector getTangentAt(double u) {
        Vector target;
        return getTangentAt(u, target);
    }
    virtual FrenetFrames computeFrenetFrames(double segments, bool closed = false) {
        if constexpr (!std::is_same_v<Vector, Vector3>) {
            throw JSTypeError("The native Frenet frame adapter requires a spatial curve");
        } else {
        if (segments == std::numeric_limits<double>::infinity())
            throw JSRangeError("Curve infinite Frenet subdivision does not terminate");
        FrenetFrames result;
        Vector3 normal, vec;
        Matrix4 mat;
        for (std::size_t i = 0; i <= segments; ++i)
            result.tangents.push_back(getTangentAt(static_cast<double>(i) / segments));
        result.normals.emplace_back();
        result.binormals.emplace_back();
        if (result.tangents.empty())
            throw JSTypeError("Curve initial Frenet tangent is undefined");
        double minimum = std::numeric_limits<double>::max();
        const double tx = std::abs(result.tangents[0].x);
        const double ty = std::abs(result.tangents[0].y);
        const double tz = std::abs(result.tangents[0].z);
        if (tx <= minimum) { minimum = tx; normal.set(1, 0, 0); }
        if (ty <= minimum) { minimum = ty; normal.set(0, 1, 0); }
        if (tz <= minimum) normal.set(0, 0, 1);
        vec.crossVectors(result.tangents[0], normal).normalize();
        result.normals[0].crossVectors(result.tangents[0], vec);
        result.binormals[0].crossVectors(result.tangents[0], result.normals[0]);
        for (std::size_t i = 1; i <= segments; ++i) {
            result.normals.push_back(result.normals[i - 1]);
            result.binormals.push_back(result.binormals[i - 1]);
            vec.crossVectors(result.tangents[i - 1], result.tangents[i]);
            if (vec.length() > std::numeric_limits<double>::epsilon()) {
                vec.normalize();
                const double theta = std::acos(Curve_detail::clamp(
                    result.tangents[i - 1].dot(result.tangents[i]), -1, 1));
                result.normals[i].applyMatrix4(mat.makeRotationAxis(vec, theta));
            }
            result.binormals[i].crossVectors(result.tangents[i], result.normals[i]);
        }
        if (closed) {
            if (std::trunc(segments) != segments || segments >= double(result.normals.size()))
                throw JSTypeError("Curve final Frenet normal is undefined");
            const auto last = static_cast<std::size_t>(segments);
            double theta = std::acos(Curve_detail::clamp(
                result.normals[0].dot(result.normals[last]), -1, 1));
            theta /= segments;
            if (result.tangents[0].dot(vec.crossVectors(result.normals[0], result.normals[last])) > 0)
                theta = -theta;
            for (std::size_t i = 1; i <= segments; ++i) {
                result.normals[i].applyMatrix4(mat.makeRotationAxis(result.tangents[i], theta * i));
                result.binormals[i].crossVectors(result.tangents[i], result.normals[i]);
            }
        }
        return result;
        }
    }
    virtual std::shared_ptr<Curve> clone() const {
        auto result = std::make_shared<Curve>();
        result->copy(*this);
        return result;
    }
    virtual Curve& copy(const Curve& source) {
        arcLengthDivisions = source.arcLengthDivisions;
        return *this;
    }
    virtual JsonValue toJSON() const {
        return JsonValue::Object{
            {"metadata", JsonValue::Object{{"version", 4.7}, {"type", "Curve"},
                                           {"generator", "Curve.toJSON"}}},
            {"arcLengthDivisions", arcLengthDivisions}, {"type", type}};
    }
    virtual Curve& fromJSON(const JsonValue& json) {
        arcLengthDivisions = json.at("arcLengthDivisions").get<double>();
        return *this;
    }
};

extern template class Curve<Vector2>;
extern template class Curve<Vector3>;
