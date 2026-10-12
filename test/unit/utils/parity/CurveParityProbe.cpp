#include "extras/core/Curve.h"
#include "extras/curves/LineCurve.h"
#include "extras/curves/LineCurve3.h"
#include "extras/curves/CubicBezierCurve.h"
#include "extras/curves/CubicBezierCurve3.h"
#include "extras/curves/QuadraticBezierCurve.h"
#include "extras/curves/QuadraticBezierCurve3.h"
#include <iostream>

namespace {
JsonValue normalized(const JsonValue& value) {
    return std::visit([](const auto& item) -> JsonValue {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, double>) {
            if (std::isnan(item)) return "NaN";
            if (std::isinf(item)) return item < 0 ? "-Infinity" : "Infinity";
            if (item == 0 && std::signbit(item)) return "-0";
            return item;
        } else if constexpr (std::is_same_v<T, JsonValue::Array>) {
            JsonValue::Array result;
            for (const auto& entry : item) result.push_back(normalized(entry));
            return result;
        } else if constexpr (std::is_same_v<T, JsonValue::Object>) {
            JsonValue::Object result;
            for (const auto& [name, entry] : item) result.emplace(name, normalized(entry));
            return result;
        } else return item;
    }, value.storage());
}
template <class T> JsonValue numericArray(const std::vector<T>& values) {
    JsonValue::Array result;
    for (const auto value : values) result.emplace_back(value);
    return result;
}
template <class V> JsonValue point(const V& vector) { return numericArray(vector.toArray()); }
template <class V> JsonValue point(const std::shared_ptr<V>& vector) { return point(*vector); }
template <class V> JsonValue points(const std::vector<V>& vectors) {
    JsonValue::Array result;
    for (const auto& vector : vectors) result.push_back(point(vector));
    return result;
}
template <class C> auto controls(C& curve) {
    using V = typename C::VectorType;
    std::vector<std::shared_ptr<V>> result;
    if constexpr (requires { curve.v0; }) result.push_back(curve.v0);
    result.push_back(curve.v1);
    result.push_back(curve.v2);
    if constexpr (requires { curve.v3; }) result.push_back(curve.v3);
    return result;
}
template <class C> std::shared_ptr<C> makeCurve() {
    using V = typename C::VectorType;
    auto makePoint = [](double x, double y, double z) {
        if constexpr (std::is_same_v<V, Vector3>) return std::make_shared<V>(x, y, z);
        else return std::make_shared<V>(x, y);
    };
    const auto p0 = makePoint(-10, 0, 1), p1 = makePoint(-5, 15, -2);
    const auto p2 = makePoint(20, 15, 4), p3 = makePoint(10, 0, 8);
    if constexpr (std::is_constructible_v<C, std::shared_ptr<V>, std::shared_ptr<V>,
                                        std::shared_ptr<V>, std::shared_ptr<V>>)
        return std::make_shared<C>(p0, p1, p2, p3);
    else if constexpr (std::is_constructible_v<C, std::shared_ptr<V>, std::shared_ptr<V>, std::shared_ptr<V>>)
        return std::make_shared<C>(p0, p1, p2);
    else return std::make_shared<C>(p0, p1);
}
template <class C> JsonValue scenario() {
    using V = typename C::VectorType;
    auto curve = makeCurve<C>();
    JsonValue result;
    result["defaults"] = C().toJSON();
    result["json"] = curve->toJSON();
    result["points"] = points(curve->getPoints(4));
    result["spaced"] = points(curve->getSpacedPoints(4));
    result["length"] = curve->getLength();
    result["lengths"] = numericArray(*curve->getLengths(4));
    result["pointAt"] = point(curve->getPointAt(.3));
    result["tangent"] = point(curve->getTangent(.25));
    result["tangentAt"] = point(curve->getTangentAt(.75));
    V target;
    result["targets"] = JsonValue::Array{&curve->getPoint(.3, target) == &target,
        &curve->getPointAt(.3, target) == &target, &curve->getTangent(.3, target) == &target,
        &curve->getTangentAt(.3, target) == &target};
    const auto copied = std::dynamic_pointer_cast<C>(curve->clone());
    bool independent = true;
    const auto sourceControls = controls(*curve), copiedControls = controls(*copied);
    for (std::size_t i = 0; i < sourceControls.size(); ++i)
        independent = independent && sourceControls[i] != copiedControls[i];
    result["clone"] = JsonValue::Object{{"type", copied->type}, {"independent", independent},
                                        {"cacheNull", copied->cacheArcLengths == nullptr},
                                        {"json", copied->toJSON()}};
    auto alias = makeCurve<C>();
    const auto aliasTarget = controls(*alias)[0];
    result["aliasedTarget"] = point(alias->getPoint(.25, *aliasTarget));
    result["aliasedControl"] = point(*aliasTarget);
    result["pointsZero"] = points(curve->getPoints(0));
    result["pointsNegative"] = points(curve->getPoints(-1));
    result["pointsFractional"] = points(curve->getPoints(2.5));
    result["tangentOutside"] = points(std::vector<V>{curve->getTangent(-.25), curve->getTangent(1.25)});
    result["pointOutside"] = points(std::vector<V>{curve->getPoint(-.25), curve->getPoint(1.25)});
    curve->arcLengthDivisions = 4;
    const auto original = curve->getLengths();
    result["cacheSame"] = curve->getLengths() == original;
    controls(*curve).back()->setScalar(30);
    result["cacheStale"] = numericArray(*curve->getLengths());
    curve->needsUpdate = true;
    const auto updated = curve->getLengths();
    result["cacheUpdated"] = JsonValue::Object{{"newIdentity", updated != original},
        {"needsUpdate", curve->needsUpdate}, {"lengths", numericArray(*updated)},
        {"oldLengths", numericArray(*original)}};
    curve->updateArcLengths();
    result["updateArcLengths"] = JsonValue::Object{{"newIdentity", curve->cacheArcLengths != updated},
        {"needsUpdate", curve->needsUpdate}, {"lengths", numericArray(*curve->getLengths())}};
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    JsonValue::Array mappings;
    for (const double u : {-.25, 0.0, .3, .5, 1.0, 1.25, nan, inf, -inf})
        mappings.emplace_back(curve->getUtoTmapping(u));
    for (const double distance : {0.0, -0.0, nan, 1.0, -1.0, inf})
        mappings.emplace_back(curve->getUtoTmapping(.3, distance));
    result["mappings"] = mappings;
    auto destination = makeCurve<C>();
    destination->arcLengthDivisions = 4;
    controls(*destination).back()->setScalar(50);
    const auto destinationCache = destination->getLengths();
    const auto destinationControls = controls(*destination);
    result["copyIdentity"] = &destination->copy(*curve) == destination.get();
    result["copyControlIdentity"] = controls(*destination) == destinationControls;
    result["copyCacheIdentity"] = destination->getLengths() == destinationCache;
    result["copyJson"] = destination->toJSON();
    const auto restored = makeCurve<C>();
    const auto restoredControls = controls(*restored);
    result["fromJSONIdentity"] = &restored->fromJSON(curve->toJSON()) == restored.get();
    result["fromJSONControls"] = controls(*restored) == restoredControls;
    result["restored"] = restored->toJSON();
    C zero;
    result["zeroMapping"] = zero.getUtoTmapping(.3);
    result["zeroTangent"] = point(zero.getTangent(.3));
    curve->arcLengthDivisions = 0;
    result["zeroLengths"] = numericArray(*curve->getLengths());
    result["zeroDivisionMapping"] = curve->getUtoTmapping(.3);
    if constexpr (std::is_same_v<V, Vector3>) {
        const auto frameCurve = makeCurve<C>();
        for (const bool closed : {false, true}) {
            const auto frames = frameCurve->computeFrenetFrames(4, closed);
            result[closed ? "framesClosed" : "framesOpen"] = JsonValue::Object{
                {"tangents", points(frames.tangents)}, {"normals", points(frames.normals)},
                {"binormals", points(frames.binormals)}};
        }
    }
    return result;
}
class NonlinearCurve : public Curve<Vector2> {
  public:
    int calls = 0;
    using Curve<Vector2>::getPoint;
    Vector2& getPoint(double t, Vector2& target) override { ++calls; return target.set(t * t, 0); }
};
} // namespace

int main() {
    JsonValue result;
    result["LineCurve"] = scenario<LineCurve>();
    result["LineCurve3"] = scenario<LineCurve3>();
    result["CubicBezierCurve"] = scenario<CubicBezierCurve>();
    result["CubicBezierCurve3"] = scenario<CubicBezierCurve3>();
    result["QuadraticBezierCurve"] = scenario<QuadraticBezierCurve>();
    result["QuadraticBezierCurve3"] = scenario<QuadraticBezierCurve3>();
    NonlinearCurve nonlinear;
    nonlinear.arcLengthDivisions = 4;
    const auto cache = nonlinear.getLengths();
    result["base"] = JsonValue::Object{{"json", Curve<Vector2>().toJSON()},
        {"lengths", numericArray(*cache)}, {"calls", nonlinear.calls},
        {"mapping", nonlinear.getUtoTmapping(.3)}, {"pointAt", point(nonlinear.getPointAt(.3))},
        {"tangent", point(nonlinear.getTangent(.5))}};
    std::cout << normalized(result).dump() << '\n';
}
