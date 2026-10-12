#include "extras/core/CurvePath.h"
#include "extras/core/Path.h"
#include "extras/core/Shape.h"
#include "extras/core/ShapePath.h"
#include "extras/ShapeUtils.h"
#include "extras/curves/LineCurve.h"
#include "extras/curves/LineCurve3.h"
#include "extras/curves/ArcCurve.h"
#include "extras/curves/CatmullRomCurve3.h"
#include "extras/curves/CubicBezierCurve3.h"
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
            JsonValue::Array result; for (const auto& entry : item) result.push_back(normalized(entry)); return result;
        } else if constexpr (std::is_same_v<T, JsonValue::Object>) {
            JsonValue::Object result; for (const auto& [key, entry] : item) result.emplace(key, normalized(entry)); return result;
        } else return item;
    }, value.storage());
}
template<class V> JsonValue point(const V& value) { return Curve_detail::vectorJSON(value); }
template<class V> JsonValue points(const std::vector<std::shared_ptr<V>>& values) {
    JsonValue::Array result; for (const auto& value : values) result.push_back(value ? point(*value) : JsonValue(nullptr)); return result;
}
JsonValue lengths(const std::shared_ptr<std::vector<double>>& values) {
    JsonValue::Array result; for (const auto value : *values) result.emplace_back(value); return result;
}
Path::Points polygon(std::initializer_list<std::array<double, 2>> values) {
    Path::Points result; for (const auto& value : values) result.push_back(std::make_shared<Vector2>(value[0], value[1])); return result;
}
void addPolygon(ShapePath& shape, const Path::Points& points) {
    shape.moveTo(points[0]->x, points[0]->y);
    for (std::size_t i = 1; i < points.size(); ++i) shape.lineTo(points[i]->x, points[i]->y);
}
void rectangle(ShapePath& shape, double lo, double hi, bool reversed = false, double offset = 0) {
    shape.moveTo(lo + offset, lo);
    if (reversed) shape.lineTo(lo + offset, hi).lineTo(hi + offset, hi).lineTo(hi + offset, lo);
    else shape.lineTo(hi + offset, lo).lineTo(hi + offset, hi).lineTo(lo + offset, hi);
    shape.lineTo(lo + offset, lo);
}
template<class V> JsonValue curvePathSamples(CurvePath<V>& path) {
    JsonValue result = JsonValue::Object{};
    result["json"] = path.toJSON(); result["length"] = path.getLength();
    result["lengths"] = lengths(path.getCurveLengths());
    result["points"] = points(path.getPoints(3)); result["spaced"] = points(path.getSpacedPoints(6));
    JsonValue::Array samples;
    for (const auto t : {-0.2, 0.0, 0.15, 0.5, 0.95, 1.0, 1.1}) {
        const auto p = path.getPointOrNull(t); samples.push_back(p ? point(*p) : JsonValue(nullptr));
    }
    result["samples"] = std::move(samples);
    V target; result["targetIdentity"] = path.getPointOrNull(.3, target) == &target;
    result["target"] = point(target);
    Curve<V>& base = path; result["baseLength"] = base.getLength(); result["basePoints"] = points(base.getPoints(3));
    result["baseDefaultPointsCount"] = base.getPoints().size();
    result["baseDefaultSpacedCount"] = base.getSpacedPoints().size();
    path.autoClose = true;
    const auto closed = path.getPoints(3), spaced = path.getSpacedPoints(3);
    result["autoClose"] = JsonValue::Object{{"points", points(closed)}, {"spaced", points(spaced)},
        {"pointAlias", closed.front() == closed.back()}, {"spacedAlias", spaced.front() == spaced.back()}};
    auto copy = std::dynamic_pointer_cast<CurvePath<V>>(path.clone());
    result["clone"] = JsonValue::Object{{"json", copy->toJSON()}, {"arrayDistinct", copy->curves != path.curves},
        {"curveDistinct", copy->curves->front() != path.curves->front()}};
    CurvePath<V> restored; restored.fromJSON(path.toJSON()); result["restored"] = restored.toJSON();
    path.closePath(); result["closedJSON"] = path.toJSON(); result["closedLength"] = path.getLength();
    return result;
}
JsonValue mainPath() {
    Path path;
    const auto originalCurrent = path.currentPoint;
    path.moveTo(-2, 1).lineTo(3, 1).quadraticCurveTo(4, 5, 7, 2).bezierCurveTo(9, -2, 11, 6, 13, 1);
    Path::Points control{std::make_shared<Vector2>(15, 4), std::make_shared<Vector2>(17, 1)};
    path.splineThru(control).absellipse(20, 2, 3, 2, .2, 2.2, false, .3).arc(1, -2, 1.5, .3, 2.5, true);
    auto result = curvePathSamples(path);
    result["currentIdentity"] = path.currentPoint == originalCurrent;
    result["currentPoint"] = point(*path.currentPoint);
    Path restored; const auto current = restored.currentPoint; restored.fromJSON(path.toJSON());
    result["pathRestored"] = restored.toJSON(); result["restoredCurrentIdentity"] = current == restored.currentPoint;
    return result;
}
JsonValue spatialPath() {
    CurvePath<Vector3> path;
    path.add(std::make_shared<LineCurve3>(std::make_shared<Vector3>(0, 0, 0), std::make_shared<Vector3>(3, 0, 4)));
    path.add(std::make_shared<QuadraticBezierCurve3>(std::make_shared<Vector3>(3, 0, 4), std::make_shared<Vector3>(5, 7, -2), std::make_shared<Vector3>(8, 2, 1)));
    path.add(std::make_shared<CubicBezierCurve3>(std::make_shared<Vector3>(8, 2, 1), std::make_shared<Vector3>(9, -3, 4),
        std::make_shared<Vector3>(12, 5, 2), std::make_shared<Vector3>(13, 2, 1)));
    auto controls = std::make_shared<CatmullRomCurve3::Points>();
    for (const auto& v : std::vector<Vector3>{{13,2,1},{15,5,-1},{17,-2,6},{20,1,3}}) controls->push_back(std::make_shared<Vector3>(v));
    path.add(std::make_shared<CatmullRomCurve3>(controls));
    return curvePathSamples(path);
}
JsonValue cacheAndEdges() {
    JsonValue result = JsonValue::Object{};
    CurvePath<Vector2> path;
    auto line = std::make_shared<LineCurve>(std::make_shared<Vector2>(), std::make_shared<Vector2>(3, 0)); path.add(line);
    const auto cache = path.getCurveLengths();
    result["cacheIdentity"] = path.getCurveLengths() == cache;
    line->v2->x = 6; path.updateArcLengths();
    result["pathOnlyUpdate"] = path.getLength(); result["cacheReplaced"] = path.cacheLengths != cache; result["needsUpdate"] = path.needsUpdate;
    line->updateArcLengths(); path.updateArcLengths(); result["bothUpdate"] = path.getLength();
    const auto stale = path.getCurveLengths();
    CurvePath<Vector2> source; source.add(std::make_shared<LineCurve>(std::make_shared<Vector2>(), std::make_shared<Vector2>(20, 0)));
    path.copy(source); result["copyCacheIdentity"] = path.cacheLengths == stale; result["copyStaleLength"] = path.getLength();
    path.updateArcLengths(); result["copyUpdatedLength"] = path.getLength();
    source.copy(source); result["selfCopyCount"] = source.curves->size();
    Path empty; result["emptyPoint"] = empty.getPointOrNull(.5) ? 1 : JsonValue(nullptr);
    result["emptyPoints"] = points(empty.getPoints()); result["emptySpaced"] = points(empty.getSpacedPoints(2));
    result["zeroDivisions"] = points(path.getSpacedPoints(0));
    Path defaultAbs, defaultRelative; defaultAbs.absarc(); defaultRelative.arc();
    result["defaultAbsArc"] = defaultAbs.toJSON(); result["defaultRelativeArc"] = defaultRelative.toJSON();
    Path spline; bool thrown = false; try { spline.splineThru(Path::Points{}); } catch (...) { thrown = true; }
    result["emptySpline"] = JsonValue::Object{{"throws", thrown}, {"count", spline.curves->size()}, {"currentPoint", point(*spline.currentPoint)}};
    CurvePath<Vector2> arcPath; arcPath.add(std::make_shared<ArcCurve>(2, 3, 4, .1, 2.8, true));
    CurvePath<Vector2> restoredArc; restoredArc.fromJSON(arcPath.toJSON()); result["arcRestored"] = restoredArc.toJSON();
    return result;
}
JsonValue shapeScenario() {
    auto input = std::make_shared<Path::Points>(polygon({{0,0},{10,0},{10,10},{0,10},{0,0}}));
    Shape shape(input); shape.uuid = "shape-fixed-uuid";
    auto hole = std::make_shared<Path>(std::make_shared<Path::Points>(polygon({{2,2},{2,4},{4,4},{4,2},{2,2}})));
    shape.holes->push_back(hole);
    const auto extracted = shape.extractPoints(4);
    JsonValue::Array holes; for (const auto& p : extracted.holes) holes.push_back(points(p));
    const auto clone = std::dynamic_pointer_cast<Shape>(shape.clone());
    Shape copy; const auto copyUUID = copy.uuid; copy.copy(shape);
    Shape restored; restored.fromJSON(shape.toJSON());
    return JsonValue::Object{{"json", shape.toJSON()}, {"shape", points(extracted.shape)}, {"holes", std::move(holes)},
        {"cloneUUIDNew", clone->uuid != shape.uuid}, {"cloneHoleNew", clone->holes->front() != hole},
        {"cloneCurveNew", clone->holes->front()->curves->front() != hole->curves->front()},
        {"copyUUIDRetained", copy.uuid == copyUUID}, {"restored", restored.toJSON()}};
}
JsonValue shapePathScenario(int scenario, bool evenOdd) {
    ShapePath path;
    if (scenario == 0) { rectangle(path, 0, 20); rectangle(path, 2, 18); rectangle(path, 4, 16, true); rectangle(path, 6, 14, true); rectangle(path, 8, 12); }
    if (scenario == 1) { rectangle(path, 3, 7, true); rectangle(path, 0, 10); rectangle(path, 0, 10, false, 20); path.moveTo(0,0).lineTo(1,1).lineTo(2,2); }
    if (scenario == 2) {
        addPolygon(path, polygon({{0,0},{8,0},{8,2},{2,2},{2,8},{0,8},{0,0}}));
        addPolygon(path, polygon({{.5,.5},{.5,1.5},{1.5,1.5},{1.5,.5},{.5,.5}}));
        addPolygon(path, polygon({{0,3},{0,4},{1,4},{1,3},{0,3}}));
    }
    if (scenario == 3) { rectangle(path, 0, 10); rectangle(path, 0, 10, true); rectangle(path, 2, 8); }
    if (scenario == 4) { rectangle(path, 0, 10, true); rectangle(path, 2, 8, true); rectangle(path, 3, 7); }
    if (scenario >= 5) {
        const int count = scenario == 5 ? 13 : 96;
        for (int i = 0; i < count; ++i) {
            const double size = i % 7 == 2 ? NAN : i % 11 == 6 ? INFINITY : (i * 17) % 31 + 1;
            rectangle(path, 0, size, i % 3 == 1, i * 100);
        }
    }
    if (evenOdd) { auto style = JSValue::object(); style.set("fillRule", "evenodd"); path.userData.set("style", style); }
    const auto shapes = path.toShapes();
    JsonValue::Array result;
    const auto origin = [&](const auto& curves) {
        for (std::size_t i = 0; i < path.subPaths->size(); ++i) if (path.subPaths->at(i)->curves == curves) return static_cast<int>(i);
        return -1;
    };
    for (const auto& shape : shapes) {
        JsonValue::Array holes;
        for (const auto& hole : *shape->holes) holes.push_back(JsonValue::Object{{"origin", origin(hole->curves)}, {"points", points(hole->getPoints())},
            {"newPath", hole != path.subPaths->at(static_cast<std::size_t>(origin(hole->curves)))}});
        result.push_back(JsonValue::Object{{"origin", origin(shape->curves)}, {"points", points(shape->getPoints())}, {"holes", std::move(holes)},
            {"currentPoint", point(*shape->currentPoint)}, {"autoClose", shape->autoClose}});
    }
    return result;
}
JsonValue triangulation(int scenario) {
    ShapeUtils::Points contour;
    std::vector<ShapeUtils::Points> holes;
    if (scenario == 0) contour = polygon({{0,0},{10,0},{10,10},{0,10},{0,0}});
    if (scenario == 1) { contour = polygon({{0,0},{10,0},{10,10},{0,10},{0,0}}); holes.push_back(polygon({{2,2},{2,4},{4,4},{4,2},{2,2}})); }
    if (scenario == 2) { contour = polygon({{0,0},{12,0},{12,12},{0,12}}); holes.push_back(polygon({{1,1},{1,3},{3,3},{3,1}})); holes.push_back(polygon({{7,7},{7,10},{10,10},{10,7}})); }
    if (scenario == 3) contour = polygon({{0,0},{8,0},{8,2},{2,2},{2,8},{0,8},{0,0}});
    if (scenario == 4) contour = polygon({{0,0},{0,5},{3,5},{3,5},{5,5},{5,0},{0,0}});
    if (scenario == 5) contour = polygon({{0,0},{4,4},{0,4},{4,0}});
    if (scenario == 6) { contour = polygon({{0,0},{10,0},{10,10},{0,10}}); holes.push_back(polygon({{5,5}})); }
    if (scenario == 7) for (int i = 0; i < 120; ++i) { const double a = i * std::numbers::pi * 2 / 120; const double r = i % 2 ? 9 : 10; contour.push_back(std::make_shared<Vector2>(std::cos(a) * r, std::sin(a) * r)); }
    const auto area = ShapeUtils::area(contour); const auto clockwise = ShapeUtils::isClockWise(contour);
    const auto faces = ShapeUtils::triangulateShape(contour, holes);
    JsonValue::Array triangles, holePoints;
    for (const auto& face : faces) triangles.push_back(JsonValue::Array{face[0], face[1], face[2]});
    for (const auto& hole : holes) holePoints.push_back(points(hole));
    return JsonValue::Object{{"area", area}, {"clockwise", clockwise}, {"contour", points(contour)}, {"holes", std::move(holePoints)}, {"triangles", std::move(triangles)}};
}
JsonValue virtualSampling() {
    class DerivedPath : public CurvePath<Vector2> {
      public:
        std::vector<double> calls;
        Vector2& getPoint(double t, Vector2& target) override {
            calls.push_back(t);
            return target.set(99, t);
        }
    } path;
    path.add(std::make_shared<LineCurve>(std::make_shared<Vector2>(), std::make_shared<Vector2>(1, 0)));
    const auto samples = path.getSpacedPoints(2);
    JsonValue::Array calls;
    for (const auto t : path.calls) calls.emplace_back(t);
    path.autoClose = true;
    const auto closed = path.getSpacedPoints(1);
    class ThrowingPath : public CurvePath<Vector2> {
      public:
        Vector2& getPoint(double, Vector2&) override { throw std::out_of_range("derived failure"); }
    } throwing;
    std::string error;
    try { throwing.getSpacedPoints(1); }
    catch (const std::out_of_range& value) { error = value.what(); }
    return JsonValue::Object{{"samples", points(samples)}, {"calls", calls},
        {"closed", points(closed)}, {"identity", closed.front() == closed.back()}, {"error", error}};
}
JsonValue samplingDefaults() {
    class DefaultHole : public Path {
      public:
        Points getPoints() override { return getPoints(7); }
        Points getPoints(double d) override { return {std::make_shared<Vector2>(d, 0)}; }
    };
    class DefaultShape : public Shape {
      public:
        Points getPoints() override { return getPoints(9); }
        Points getPoints(double d) override { return {std::make_shared<Vector2>(d, 0)}; }
    } shape;
    shape.holes->push_back(std::make_shared<DefaultHole>());
    const auto omitted = shape.extractPoints(), explicitValue = shape.extractPoints(3);
    return JsonValue::Array{shape.getPointsHoles()[0][0]->x, omitted.shape[0]->x,
        omitted.holes[0][0]->x, explicitValue.shape[0]->x, explicitValue.holes[0][0]->x};
}
JsonValue replacedSubPaths() {
    ShapePath owner;
    class ReplacingPath : public Path {
      public:
        ShapePath* owner;
        Points getPoints() override {
            owner->subPaths = std::make_shared<ShapePath::Paths>();
            return Path::getPoints();
        }
    };
    auto first = std::make_shared<ReplacingPath>(); first->owner = &owner;
    first->moveTo(0, 0).lineTo(2, 0).lineTo(0, 2);
    auto second = std::make_shared<Path>(); second->moveTo(10, 0).lineTo(12, 0).lineTo(10, 2);
    owner.subPaths->push_back(first); owner.subPaths->push_back(second);
    const auto shapes = owner.toShapes();
    JsonValue::Array result;
    for (const auto& shape : shapes) result.push_back(JsonValue::Object{{"points", points(shape->getPoints())},
        {"first", shape->curves == first->curves}, {"second", shape->curves == second->curves}});
    return JsonValue::Object{{"shapes", result}, {"subPathsCount", owner.subPaths->size()}};
}
JsonValue curveMutation(bool copying) {
    CurvePath<Vector2> source, destination;
    class MutatingCurve : public LineCurve {
      public:
        CurvePath<Vector2>* source;
        CurvePath<Vector2>* destination;
        void replace() const {
            source->curves = std::make_shared<CurvePath<Vector2>::Curves>();
            source->add(std::make_shared<LineCurve>());
            source->add(std::make_shared<LineCurve>(std::make_shared<Vector2>(), std::make_shared<Vector2>(20, 0)));
            if (destination) destination->curves = std::make_shared<CurvePath<Vector2>::Curves>();
        }
        JsonValue toJSON() const override { replace(); return LineCurve::toJSON(); }
        std::shared_ptr<Curve<Vector2>> clone() const override { replace(); return LineCurve::clone(); }
    };
    auto first = std::make_shared<MutatingCurve>(); first->source = &source;
    first->destination = copying ? &destination : nullptr;
    source.add(first); first.reset();
    source.add(std::make_shared<LineCurve>(std::make_shared<Vector2>(), std::make_shared<Vector2>(10, 0)));
    if (copying) {
        destination.copy(source);
        return JsonValue::Array{destination.curves->size(), destination.curves->back()->getPoint(1).x};
    }
    const auto entries = source.toJSON().at("curves").get<JsonValue::Array>();
    return JsonValue::Array{entries.size(), entries[1].at("v2").get<JsonValue::Array>()[0]};
}
JsonValue missingSubPaths() {
    JsonValue::Array cases;
    for (bool duringCallback : {false, true}) {
        ShapePath owner;
        class ReplacingPath : public Path {
          public:
            ShapePath* owner;
            Points getPoints() override { owner->subPaths->at(1).reset(); return Path::getPoints(); }
        };
        if (duringCallback) {
            auto first = std::make_shared<ReplacingPath>(); first->owner = &owner;
            first->moveTo(0, 0).lineTo(2, 0).lineTo(0, 2);
            owner.subPaths->push_back(first); owner.subPaths->push_back(std::make_shared<Path>());
        } else owner.subPaths->push_back(nullptr);
        std::string error;
        try { owner.toShapes(); } catch (const JSTypeError&) { error = "TypeError"; }
        cases.emplace_back(error);
    }
    return cases;
}
JsonValue missingContourPoints() {
    JsonValue::Array cases;
    for (int mode = 0; mode < 4; ++mode) {
        auto contour = polygon({{0,0},{1,0},{0,1}}); contour[0].reset();
        std::string error;
        try {
            if (mode == 0) ShapeUtils::area(contour);
            else if (mode == 1) ShapeUtils::isClockWise(contour);
            else if (mode == 2) { std::vector<ShapeUtils::Points> holes; ShapeUtils::triangulateShape(contour, holes); }
            else {
                class MissingPointPath : public Path {
                  public:
                    Points getPoints() override { return {nullptr, std::make_shared<Vector2>(1,0), std::make_shared<Vector2>(0,1)}; }
                };
                ShapePath owner; owner.subPaths->push_back(std::make_shared<MissingPointPath>()); owner.toShapes();
            }
        } catch (const JSTypeError&) { error = "TypeError"; }
        cases.emplace_back(error);
    }
    return cases;
}
JsonValue holeMutation(bool copying) {
    Shape source, destination;
    class MutatingHole : public Path {
      public:
        Shape* source;
        Shape* destination;
        void replace() const {
            source->holes = std::make_shared<Shape::Holes>();
            source->holes->push_back(std::make_shared<Path>());
            auto second = std::make_shared<Path>(); second->lineTo(20, 0); source->holes->push_back(second);
            if (destination) destination->holes = std::make_shared<Shape::Holes>();
        }
        JsonValue toJSON() const override { replace(); return Path::toJSON(); }
        std::shared_ptr<Curve<Vector2>> clone() const override { replace(); return Path::clone(); }
    };
    auto first = std::make_shared<MutatingHole>(); first->source = &source;
    first->destination = copying ? &destination : nullptr;
    source.holes->push_back(first); first.reset();
    auto second = std::make_shared<Path>(); second->lineTo(10, 0); source.holes->push_back(second);
    if (copying) {
        destination.copy(source);
        return JsonValue::Array{destination.holes->size(), destination.holes->back()->currentPoint->x};
    }
    const auto entries = source.toJSON().at("holes").get<JsonValue::Array>();
    return JsonValue::Array{entries.size(), entries[1].at("currentPoint").get<JsonValue::Array>()[0]};
}
}
int main() {
    try {
        JsonValue result = JsonValue::Object{{"path", mainPath()}, {"spatial", spatialPath()}, {"cacheEdges", cacheAndEdges()}, {"shape", shapeScenario()}};
        JsonValue::Array classifications, triangulations;
        for (int scenario = 0; scenario < 7; ++scenario) for (const bool evenOdd : {false, true}) classifications.push_back(shapePathScenario(scenario, evenOdd));
        for (int scenario = 0; scenario < 9; ++scenario) triangulations.push_back(triangulation(scenario));
        result["classifications"] = std::move(classifications); result["triangulations"] = std::move(triangulations);
        result["virtualSampling"] = virtualSampling();
        result["samplingDefaults"] = samplingDefaults();
        result["replacedSubPaths"] = replacedSubPaths();
        result["missingSubPaths"] = missingSubPaths();
        result["missingContourPoints"] = missingContourPoints();
        result["mutatingContainers"] = JsonValue::Array{curveMutation(false), curveMutation(true), holeMutation(false), holeMutation(true)};
        std::cout << normalized(result).dump() << '\n';
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
