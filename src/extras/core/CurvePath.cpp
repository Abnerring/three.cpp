#include "extras/core/CurvePath.h"
#include "extras/curves/ArcCurve.h"
#include "extras/curves/CatmullRomCurve3.h"
#include "extras/curves/CubicBezierCurve.h"
#include "extras/curves/CubicBezierCurve3.h"
#include "extras/curves/EllipseCurve.h"
#include "extras/curves/LineCurve.h"
#include "extras/curves/LineCurve3.h"
#include "extras/curves/QuadraticBezierCurve.h"
#include "extras/curves/QuadraticBezierCurve3.h"
#include "extras/curves/SplineCurve.h"

namespace {
class NullPointResult final : public std::out_of_range {
  public:
    NullPointResult() : std::out_of_range("CurvePath.getPoint returned null; use getPointOrNull") {}
};

template<class Vector> std::shared_ptr<Curve<Vector>> constructCurve(const std::string& type) {
    if constexpr (std::is_same_v<Vector, Vector2>) {
        if (type == "ArcCurve") return std::make_shared<ArcCurve>();
        if (type == "EllipseCurve") return std::make_shared<EllipseCurve>();
        if (type == "LineCurve") return std::make_shared<LineCurve>();
        if (type == "QuadraticBezierCurve") return std::make_shared<QuadraticBezierCurve>();
        if (type == "CubicBezierCurve") return std::make_shared<CubicBezierCurve>();
        if (type == "SplineCurve") return std::make_shared<SplineCurve>();
    } else {
        if (type == "LineCurve3") return std::make_shared<LineCurve3>();
        if (type == "QuadraticBezierCurve3") return std::make_shared<QuadraticBezierCurve3>();
        if (type == "CubicBezierCurve3") return std::make_shared<CubicBezierCurve3>();
        if (type == "CatmullRomCurve3") return std::make_shared<CatmullRomCurve3>();
    }
    throw std::invalid_argument("CurvePath: unsupported curve type for this dimension: " + type);
}
}

template<class V> CurvePath<V>::CurvePath() { this->type = "CurvePath"; }
template<class V> void CurvePath<V>::add(const std::shared_ptr<Curve<V>>& curve) { curves->push_back(curve); }
template<class V> CurvePath<V>& CurvePath<V>::closePath() {
    if (curves->empty()) throw std::out_of_range("CurvePath.closePath: empty path");
    const auto first = curves->front();
    auto start = std::make_shared<V>(first->getPoint(0));
    const auto last = curves->back();
    auto end = std::make_shared<V>(last->getPoint(1));
    if (!start->equals(*end)) {
        if constexpr (std::is_same_v<V, Vector2>) curves->push_back(std::make_shared<LineCurve>(end, start));
        else curves->push_back(std::make_shared<LineCurve3>(end, start));
    }
    return *this;
}
template<class V> V* CurvePath<V>::getPointOrNull(double t, V& target) {
    const auto d = t * getLength();
    const auto lengths = getCurveLengths();
    for (std::size_t i = 0; i < lengths->size(); ++i) {
        if ((*lengths)[i] >= d) {
            const auto diff = (*lengths)[i] - d;
            const auto curve = curves->at(i);
            const auto length = curve->getLength();
            const auto u = length == 0 ? 0 : 1 - diff / length;
            return &curve->getPointAt(u, target);
        }
    }
    return nullptr;
}
template<class V> std::shared_ptr<V> CurvePath<V>::getPointOrNull(double t) {
    auto target = std::make_shared<V>();
    return getPointOrNull(t, *target) ? target : nullptr;
}
template<class V> V& CurvePath<V>::getPoint(double t, V& target) {
    if (auto result = getPointOrNull(t, target)) return *result;
    throw NullPointResult();
}
template<class V> double CurvePath<V>::getLength() {
    const auto lengths = getCurveLengths();
    return lengths->empty() ? std::numeric_limits<double>::quiet_NaN() : lengths->back();
}
template<class V> void CurvePath<V>::updateArcLengths() {
    this->needsUpdate = true;
    cacheLengths = nullptr;
    getCurveLengths();
}
template<class V> std::shared_ptr<std::vector<double>> CurvePath<V>::getCurveLengths() {
    if (cacheLengths && cacheLengths->size() == curves->size()) return cacheLengths;
    auto lengths = std::make_shared<std::vector<double>>();
    double sum = 0;
    const auto count = curves->size();
    for (std::size_t i = 0; i < count; ++i) {
        const auto curve = curves->at(i);
        sum += curve->getLength();
        lengths->push_back(sum);
    }
    cacheLengths = lengths;
    return lengths;
}
template<class V> typename CurvePath<V>::Points CurvePath<V>::getSpacedPoints(double divisions) {
    Points points;
    for (double i = 0; i <= divisions; ++i) {
        try {
            points.push_back(std::make_shared<V>(this->getPoint(i / divisions)));
        } catch (const NullPointResult&) {
            points.push_back(nullptr);
        }
    }
    // An empty JS array contributes undefined; nullptr is the nullable point adaptation.
    if (autoClose) points.push_back(points.empty() ? nullptr : points.front());
    return points;
}
template<class V> typename CurvePath<V>::Points CurvePath<V>::getPoints(double divisions) {
    Points points;
    std::shared_ptr<V> last;
    const auto source = curves;
    for (std::size_t i = 0; i < source->size(); ++i) {
        const auto curve = source->at(i);
        double resolution = divisions;
        if constexpr (std::is_same_v<V, Vector2>) {
            if (dynamic_cast<EllipseCurve*>(curve.get())) resolution = divisions * 2;
            else if (dynamic_cast<LineCurve*>(curve.get())) resolution = 1;
            else if (auto spline = dynamic_cast<SplineCurve*>(curve.get())) resolution = divisions * spline->points->size();
        } else if (dynamic_cast<LineCurve3*>(curve.get())) resolution = 1;
        const auto sampled = curve->getPoints(resolution);
        for (const auto& point : sampled) {
            if (last && last->equals(*point)) continue;
            points.push_back(point);
            last = point;
        }
    }
    if (autoClose && points.size() > 1 && !points.back()->equals(*points.front())) points.push_back(points.front());
    return points;
}
template<class V> std::shared_ptr<Curve<V>> CurvePath<V>::clone() const {
    auto result = std::make_shared<CurvePath>(); result->copy(*this); return result;
}
template<class V> CurvePath<V>& CurvePath<V>::copy(const Curve<V>& source) {
    Curve<V>::copy(source);
    const auto& path = dynamic_cast<const CurvePath&>(source);
    curves = std::make_shared<Curves>();
    const auto count = path.curves->size();
    for (std::size_t i = 0; i < count; ++i) {
        const auto curve = path.curves->at(i);
        const auto destination = curves;
        auto cloned = curve->clone();
        destination->push_back(std::move(cloned));
    }
    autoClose = path.autoClose;
    return *this;
}
template<class V> JsonValue CurvePath<V>::toJSON() const {
    auto json = Curve<V>::toJSON();
    json["autoClose"] = autoClose;
    JsonValue::Array entries;
    const auto count = curves->size();
    for (std::size_t i = 0; i < count; ++i) {
        const auto curve = curves->at(i);
        entries.push_back(curve->toJSON());
    }
    json["curves"] = std::move(entries);
    return json;
}
template<class V> CurvePath<V>& CurvePath<V>::fromJSON(const JsonValue& json) {
    Curve<V>::fromJSON(json);
    autoClose = json.at("autoClose").get<bool>();
    curves = std::make_shared<Curves>();
    for (const auto& entry : json.at("curves").get<JsonValue::Array>()) {
        auto curve = constructCurve<V>(entry.at("type").get<std::string>());
        curve->fromJSON(entry);
        curves->push_back(curve);
    }
    return *this;
}
template class CurvePath<Vector2>;
template class CurvePath<Vector3>;
