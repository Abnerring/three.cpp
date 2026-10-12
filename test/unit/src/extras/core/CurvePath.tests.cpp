#include "catch.hpp"
#include "extras/core/CurvePath.h"
#include "extras/curves/LineCurve.h"
#include "extras/curves/LineCurve3.h"
#include "extras/curves/EllipseCurve.h"

TEST_CASE("CurvePath samples lengths and preserves targets in both dimensions", "[extras][CurvePath]") {
    CurvePath<Vector2> path;
    REQUIRE(path.type == "CurvePath");
    REQUIRE_FALSE(path.autoClose);
    REQUIRE(std::isnan(path.getLength()));
    Vector2 target(9, 8);
    REQUIRE(path.getPointOrNull(.5, target) == nullptr);
    REQUIRE(target.equals(Vector2(9, 8)));
    REQUIRE_THROWS_AS(path.closePath(), std::out_of_range);
    path.add(std::make_shared<LineCurve>(std::make_shared<Vector2>(0, 0), std::make_shared<Vector2>(3, 0)));
    path.add(std::make_shared<LineCurve>(std::make_shared<Vector2>(3, 0), std::make_shared<Vector2>(3, 4)));
    REQUIRE(path.getLength() == 7);
    REQUIRE(&path.getPoint(3.0 / 7, target) == &target);
    REQUIRE(target.equals(Vector2(3, 0)));
    REQUIRE(path.getPointOrNull(2) == nullptr);
    REQUIRE(path.getPointOrNull(-1)->equals(Vector2(-7, 0)));
    Curve<Vector2>& base = path;
    REQUIRE(base.getLength() == 7);
    REQUIRE(base.getPoints().size() == 3);
    REQUIRE(&path.closePath() == &path);
    REQUIRE(path.curves->size() == 3);
    REQUIRE(path.getLength() == Approx(12));
    path.closePath();
    REQUIRE(path.curves->size() == 3);
    CurvePath<Vector3> spatial;
    spatial.add(std::make_shared<LineCurve3>(std::make_shared<Vector3>(1, 2, 3), std::make_shared<Vector3>(4, 6, 3)));
    spatial.closePath();
    REQUIRE(spatial.curves->back()->type == "LineCurve3");
    REQUIRE(spatial.getLength() == Approx(10));
}

TEST_CASE("CurvePath cache invalidation and copy match upstream lazy behavior", "[extras][CurvePath]") {
    CurvePath<Vector2> path;
    auto line = std::make_shared<LineCurve>(std::make_shared<Vector2>(), std::make_shared<Vector2>(3, 0));
    path.add(line);
    const auto cache = path.getCurveLengths();
    REQUIRE(path.getCurveLengths() == cache);
    line->v2->x = 6;
    path.updateArcLengths();
    REQUIRE(path.needsUpdate);
    REQUIRE(path.getCurveLengths() != cache);
    REQUIRE(path.getLength() == 3); // child caches are deliberately not invalidated
    line->updateArcLengths();
    path.updateArcLengths();
    REQUIRE(path.getLength() == 6);
    const auto stale = path.getCurveLengths();
    CurvePath<Vector2> source;
    source.add(std::make_shared<LineCurve>(std::make_shared<Vector2>(), std::make_shared<Vector2>(20, 0)));
    path.copy(source);
    REQUIRE(path.cacheLengths == stale);
    REQUIRE(path.getLength() == 6); // same-length source preserves destination cache
    REQUIRE(path.curves != source.curves);
    REQUIRE(path.curves->front() != source.curves->front());
    path.updateArcLengths();
    REQUIRE(path.getLength() == 20);
    auto selfArray = source.curves;
    source.copy(source);
    REQUIRE(source.curves->empty());
    REQUIRE(selfArray->size() == 1);
}

TEST_CASE("CurvePath automatic closure shares endpoint objects", "[extras][CurvePath]") {
    CurvePath<Vector2> path;
    path.add(std::make_shared<LineCurve>(std::make_shared<Vector2>(), std::make_shared<Vector2>(2, 1)));
    path.autoClose = true;
    auto points = path.getPoints();
    REQUIRE(points.size() == 3);
    REQUIRE(points.front() == points.back());
    points.front()->x = 5;
    REQUIRE(points.back()->x == 5);
    const auto spaced = path.getSpacedPoints(2);
    REQUIRE(spaced.size() == 4);
    REQUIRE(spaced.front() == spaced.back());
    REQUIRE(path.getPoints(0).size() == 3); // line resolution is always one
    auto restored = CurvePath<Vector2>();
    restored.fromJSON(path.toJSON());
    REQUIRE(restored.toJSON() == path.toJSON());
    REQUIRE(restored.curves->front() != path.curves->front());
}

TEST_CASE("CurvePath virtual sampling retains derived default divisions", "[extras][CurvePath]") {
    CurvePath<Vector2> path;
    path.add(std::make_shared<EllipseCurve>());
    Curve<Vector2>& base = path;
    REQUIRE(base.getPoints().size() == 25);
    REQUIRE(base.getSpacedPoints().size() == 41);
    REQUIRE(base.getPoints(3).size() == 7);
    REQUIRE(base.getSpacedPoints(3).size() == 4);
}

TEST_CASE("CurvePath spaced samples call the derived point implementation", "[extras][CurvePath]") {
    class DerivedPath : public CurvePath<Vector2> {
      public:
        std::vector<double> calls;
        Vector2& getPoint(double t, Vector2& target) override {
            calls.push_back(t);
            return target.set(99, t);
        }
    } path;
    path.add(std::make_shared<LineCurve>(std::make_shared<Vector2>(), std::make_shared<Vector2>(1, 0)));
    Curve<Vector2>& base = path;
    const auto samples = base.getSpacedPoints(2);
    REQUIRE(path.calls == std::vector<double>{0, .5, 1});
    REQUIRE(samples[0]->equals(Vector2(99, 0)));
    REQUIRE(samples[1]->equals(Vector2(99, .5)));
    REQUIRE(samples[2]->equals(Vector2(99, 1)));
    REQUIRE(base.getSpacedPoints().size() == 41);
    path.autoClose = true;
    const auto closed = base.getSpacedPoints(1);
    REQUIRE(closed.front() == closed.back());
}

TEST_CASE("CurvePath spaced sampling only maps its own null result to a missing point", "[extras][CurvePath]") {
    CurvePath<Vector2> empty;
    const auto samples = empty.getSpacedPoints(2);
    REQUIRE(samples == CurvePath<Vector2>::Points{nullptr, nullptr, nullptr});
    Vector2 target;
    REQUIRE_THROWS_AS(empty.getPoint(.5, target), std::out_of_range);
    class ThrowingPath : public CurvePath<Vector2> {
      public:
        Vector2& getPoint(double, Vector2&) override { throw std::out_of_range("derived failure"); }
    } throwing;
    REQUIRE_THROWS_WITH(throwing.getSpacedPoints(1), "derived failure");
}

TEST_CASE("CurvePath serialization rereads curves and retains an active serializer", "[extras][CurvePath]") {
    CurvePath<Vector2> path;
    class ReplacingCurve : public LineCurve {
      public:
        CurvePath<Vector2>* owner;
        bool* destroyed;
        ~ReplacingCurve() override { *destroyed = true; }
        JsonValue toJSON() const override {
            owner->curves = std::make_shared<CurvePath<Vector2>::Curves>();
            owner->add(std::make_shared<LineCurve>());
            owner->add(std::make_shared<LineCurve>(std::make_shared<Vector2>(), std::make_shared<Vector2>(20, 0)));
            REQUIRE_FALSE(*destroyed);
            return LineCurve::toJSON();
        }
    };
    bool destroyed = false;
    auto first = std::make_shared<ReplacingCurve>(); first->owner = &path; first->destroyed = &destroyed;
    path.add(first); first.reset();
    path.add(std::make_shared<LineCurve>(std::make_shared<Vector2>(), std::make_shared<Vector2>(10, 0)));
    const auto json = path.toJSON();
    REQUIRE(destroyed);
    const auto& entries = json.at("curves").get<JsonValue::Array>();
    REQUIRE(entries.size() == 2);
    REQUIRE(entries[1].at("v2").get<JsonValue::Array>()[0].get<double>() == 20);
}

TEST_CASE("CurvePath copy retains clone receiver and captures each push destination", "[extras][CurvePath]") {
    CurvePath<Vector2> source, destination;
    class ReplacingCurve : public LineCurve {
      public:
        CurvePath<Vector2>* source;
        CurvePath<Vector2>* destination;
        bool* destroyed;
        ~ReplacingCurve() override { *destroyed = true; }
        std::shared_ptr<Curve<Vector2>> clone() const override {
            source->curves = std::make_shared<CurvePath<Vector2>::Curves>();
            source->add(std::make_shared<LineCurve>());
            source->add(std::make_shared<LineCurve>(std::make_shared<Vector2>(), std::make_shared<Vector2>(20, 0)));
            destination->curves = std::make_shared<CurvePath<Vector2>::Curves>();
            REQUIRE_FALSE(*destroyed);
            return LineCurve::clone();
        }
    };
    bool destroyed = false;
    auto first = std::make_shared<ReplacingCurve>();
    first->source = &source; first->destination = &destination; first->destroyed = &destroyed;
    source.add(first); first.reset(); source.add(std::make_shared<LineCurve>());
    destination.copy(source);
    REQUIRE(destroyed);
    REQUIRE(destination.curves->size() == 1);
    REQUIRE(destination.curves->front()->getPoint(1).x == 20);
}

TEST_CASE("CurvePath length and closure retain curves during virtual callbacks", "[extras][CurvePath]") {
    class ReplacingCurve : public LineCurve {
      public:
        CurvePath<Vector2>* owner;
        bool* destroyed;
        ~ReplacingCurve() override { *destroyed = true; }
        void replace() {
            owner->curves = std::make_shared<CurvePath<Vector2>::Curves>();
            owner->add(std::make_shared<LineCurve>(std::make_shared<Vector2>(), std::make_shared<Vector2>(2, 0)));
            REQUIRE_FALSE(*destroyed);
        }
        double getLength() override { replace(); return 3; }
        Vector2& getPoint(double t, Vector2& target) override { replace(); return target.set(t, 0); }
    };
    for (const bool close : {false, true}) {
        CurvePath<Vector2> path;
        bool destroyed = false;
        auto curve = std::make_shared<ReplacingCurve>(); curve->owner = &path; curve->destroyed = &destroyed;
        path.add(curve); curve.reset();
        if (close) { path.closePath(); REQUIRE(path.curves->size() == 2); }
        else REQUIRE(path.getLength() == 3);
        REQUIRE(destroyed);
    }
}
