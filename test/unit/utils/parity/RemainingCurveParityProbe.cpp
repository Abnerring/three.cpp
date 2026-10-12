#include "extras/core/Interpolations.h"
#include "extras/curves/Curves.h"
#include "core/JSValue.h"
#include <iostream>
#include <sstream>

namespace {
JsonValue normalize(const JsonValue& value) {
    return std::visit([](const auto& item) -> JsonValue {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, double>) {
            if (std::isnan(item)) return "NaN";
            if (std::isinf(item)) return item < 0 ? "-Infinity" : "Infinity";
            if (item == 0 && std::signbit(item)) return "-0";
            return item;
        } else if constexpr (std::is_same_v<T, JsonValue::Array>) {
            JsonValue::Array result;
            for (const auto& entry : item) result.push_back(normalize(entry));
            return result;
        } else if constexpr (std::is_same_v<T, JsonValue::Object>) {
            JsonValue::Object result;
            for (const auto& [key, entry] : item) result[key] = normalize(entry);
            return result;
        } else return item;
    }, value.storage());
}
template<class V> JsonValue point(const V& v) { return Curve_detail::vectorJSON(v); }
template<class V> JsonValue samples(const std::vector<std::shared_ptr<V>>& values) {
    JsonValue::Array result;
    for (const auto& value : values) result.push_back(point(*value));
    return result;
}
JsonValue numbers(const std::vector<double>& values) {
    JsonValue::Array result; for (const auto value : values) result.emplace_back(value); return result;
}
double number(std::istream& in) {
    std::string token; in >> token;
    if (token == "NaN") return std::numeric_limits<double>::quiet_NaN();
    if (token == "Infinity") return std::numeric_limits<double>::infinity();
    if (token == "-Infinity") return -std::numeric_limits<double>::infinity();
    return std::stod(token);
}
template<class C> typename C::PointsPtr controls(std::istream& in) {
    int length; in >> length;
    if (length < 0) return nullptr;
    auto points = std::make_shared<typename C::Points>();
    for (int i = 0; i < length; ++i) {
        int valid; in >> valid;
        if (!valid) { points->push_back(nullptr); continue; }
        const auto x = number(in), y = number(in);
        if constexpr (std::is_same_v<typename C::VectorType, Vector3>) points->push_back(std::make_shared<Vector3>(x, y, number(in)));
        else points->push_back(std::make_shared<Vector2>(x, y));
    }
    return points;
}
template<class C> JsonValue evaluate(C& curve, double t, int alias) {
    typename C::VectorType target;
    auto* destination = alias < 0 ? &target : curve.points->at(static_cast<std::size_t>(alias)).get();
    const auto& value = curve.getPoint(t, *destination);
    return JsonValue::Object{{"point", point(value)}, {"sameTarget", &value == destination}};
}
JsonValue query(std::istream& in) {
    char kind; in >> kind;
    if (kind == 'I') {
        const auto t = number(in), p0 = number(in), p1 = number(in), p2 = number(in), p3 = number(in);
        return JsonValue::Array{CatmullRom(t, p0, p1, p2, p3), QuadraticBezier(t, p0, p1, p2), CubicBezier(t, p0, p1, p2, p3)};
    }
    if (kind == 'E') {
        const auto x = number(in), y = number(in), rx = number(in), ry = number(in);
        const auto start = number(in), end = number(in); bool clockwise; in >> clockwise;
        const auto rotation = number(in), t = number(in);
        EllipseCurve curve(x, y, rx, ry, start, end, clockwise, rotation);
        Vector2 target;
        auto& value = curve.getPoint(t, target);
        return JsonValue::Object{{"point", point(value)}, {"sameTarget", &value == &target}};
    }
    if (kind == 'A') {
        const auto x = number(in), y = number(in), radius = number(in), start = number(in), end = number(in);
        bool clockwise; in >> clockwise; const auto t = number(in);
        ArcCurve curve(x, y, radius, start, end, clockwise);
        Vector2 target; auto& value = curve.getPoint(t, target);
        return JsonValue::Object{{"point", point(value)}, {"sameTarget", &value == &target}};
    }
    if (kind == 'S') {
        SplineCurve curve(controls<SplineCurve>(in)); const auto t = number(in); int alias; in >> alias;
        return evaluate(curve, t, alias);
    }
    if (kind == 'C') {
        auto points = controls<CatmullRomCurve3>(in); bool closed; std::string curveType; in >> closed >> curveType;
        const auto tension = number(in), t = number(in); int alias; in >> alias;
        CatmullRomCurve3 curve(points, closed, curveType, tension);
        return evaluate(curve, t, alias);
    }
    throw std::invalid_argument("Unknown curve query");
}
template<class C> std::shared_ptr<C> make() {
    using V = typename C::VectorType;
    if constexpr (std::is_same_v<C, EllipseCurve>) return std::make_shared<C>(2, -3, 5, 2, -.4, 7.2, true, .7);
    else if constexpr (std::is_same_v<C, ArcCurve>) return std::make_shared<C>(2, -3, 5, -.4, 7.2, true);
    else {
        auto points = std::make_shared<typename C::Points>();
        for (const auto& xyz : std::vector<std::vector<double>>{{-10,0,1},{-5,15,-2},{20,15,4},{10,0,8}}) {
            if constexpr (std::is_same_v<V, Vector3>) points->push_back(std::make_shared<V>(xyz[0],xyz[1],xyz[2]));
            else points->push_back(std::make_shared<V>(xyz[0],xyz[1]));
        }
        return std::make_shared<C>(points);
    }
}
template<class C> JsonValue scenario() {
    using V = typename C::VectorType;
    auto curve = make<C>();
    JsonValue result = JsonValue::Object{{"defaults", C().toJSON()}, {"json", curve->toJSON()}};
    result["points"] = samples(curve->getPoints(4)); result["spaced"] = samples(curve->getSpacedPoints(4));
    result["length"] = curve->getLength(); result["lengths"] = numbers(*curve->getLengths(4));
    result["pointAt"] = point(curve->getPointAt(.3)); result["tangent"] = point(curve->getTangent(.25));
    result["tangentAt"] = point(curve->getTangentAt(.75));
    V target; result["targets"] = JsonValue::Array{&curve->getPoint(.3,target)==&target, &curve->getPointAt(.3,target)==&target,
        &curve->getTangent(.3,target)==&target, &curve->getTangentAt(.3,target)==&target};
    const auto clone = std::dynamic_pointer_cast<C>(curve->clone());
    result["clone"] = JsonValue::Object{{"type",clone->type},{"cacheNull",!clone->cacheArcLengths},{"json",clone->toJSON()}};
    auto destination = make<C>(); destination->arcLengthDivisions=5; const auto cache = destination->getLengths();
    result["copyIdentity"] = &destination->copy(*curve)==destination.get();
    result["copyCache"] = destination->cacheArcLengths==cache;
    result["copyJson"] = destination->toJSON();
    result["fromIdentity"] = &destination->fromJSON(curve->toJSON())==destination.get();
    result["restored"] = destination->toJSON();
    if constexpr (requires {curve->points;}) {
        const auto originalPoints = curve->points;
        bool independent = clone->points != originalPoints;
        for(std::size_t i=0;i<originalPoints->size();++i) independent = independent && (*clone->points)[i] != (*originalPoints)[i];
        result["independent"] = independent;
        auto alias = make<C>(); const auto shared = alias->points;
        C second(shared);
        (*shared)[0]->setScalar(7); shared->push_back(std::make_shared<V>());
        result["sharedArray"] = alias->points==second.points;
        result["sharedMutation"] = second.toJSON();
        const auto pointAlias = (*shared)[1]; result["pointAlias"] = point(alias->getPoint(.3,*pointAlias));
        result["aliasStored"] = point(*(*second.points)[1]);
        auto self = make<C>(); const auto previous = self->points;
        self->copy(*self); result["selfCopy"] = self->toJSON(); result["selfOldLength"] = previous->size();
        auto duplicated = std::make_shared<typename C::Points>(); auto repeated = std::make_shared<V>();
        duplicated->push_back(repeated); duplicated->push_back(repeated);
        C same(duplicated); const auto unique = std::dynamic_pointer_cast<C>(same.clone());
        result["duplicateCloneIndependent"] = (*unique->points)[0] != (*unique->points)[1];
    }
    if constexpr (std::is_same_v<C, ArcCurve>) {
        curve->yRadius = 8; curve->aRotation=.9;
        result["arcInheritedFields"] = curve->clone()->toJSON();
    }
    return result;
}
}
int main() {
    try {
        JsonValue::Array queries;
        std::string line;
        while (std::getline(std::cin,line)) {
            if (line.empty()) continue;
            std::istringstream input(line);
            try { queries.push_back(query(input)); }
            catch(const JSTypeError&) { queries.push_back(JsonValue::Object{{"error","TypeError"}}); }
        }
        JsonValue result=JsonValue::Object{{"queries",std::move(queries)}};
        result["EllipseCurve"] = scenario<EllipseCurve>(); result["ArcCurve"] = scenario<ArcCurve>();
        result["SplineCurve"] = scenario<SplineCurve>(); result["CatmullRomCurve3"] = scenario<CatmullRomCurve3>();
        std::cout << normalize(result).dump() << '\n';
    } catch(const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
