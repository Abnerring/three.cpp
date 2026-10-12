#include "catch.hpp"
#include "extras/core/Interpolations.h"
#include "extras/curves/Curves.h"
#include "core/JSValue.h"

namespace {
template<class V> void expectedPoint(const V& actual, std::initializer_list<double> expected) {
    const auto values = actual.toArray();
    REQUIRE(values.size() == expected.size());
    std::size_t index = 0;
    for (const auto value : expected) REQUIRE(values[index++] == Approx(value).epsilon(5e-12).margin(5e-12));
}
template<class C> typename C::PointsPtr makePoints() {
    using V = typename C::VectorType;
    auto points = std::make_shared<typename C::Points>();
    for (const auto& xyz : std::vector<std::vector<double>>{{-10,0,1},{-5,15,-2},{20,15,4},{10,0,8}}) {
        if constexpr (std::is_same_v<V, Vector3>) points->push_back(std::make_shared<V>(xyz[0], xyz[1], xyz[2]));
        else points->push_back(std::make_shared<V>(xyz[0], xyz[1]));
    }
    return points;
}
template<class C> void sharedPointsContract() {
    using V = typename C::VectorType;
    auto points = makePoints<C>();
    C source(points), alias(points);
    REQUIRE(source.points == points);
    REQUIRE(alias.points == points);
    (*points)[0]->setScalar(7);
    REQUIRE((*source.points)[0]->equals(*(*alias.points)[0]));
    points->push_back(std::make_shared<V>());
    REQUIRE(source.points->size() == 5);
    source.arcLengthDivisions = 9;
    source.getLengths();
    const auto clone = std::dynamic_pointer_cast<C>(source.clone());
    REQUIRE(clone != nullptr);
    REQUIRE(clone->points != points);
    REQUIRE(clone->cacheArcLengths == nullptr);
    REQUIRE(clone->arcLengthDivisions == 9);
    REQUIRE(clone->toJSON() == source.toJSON());
    for (std::size_t i = 0; i < points->size(); ++i) REQUIRE((*clone->points)[i] != (*points)[i]);
    C destination(makePoints<C>());
    const auto old = destination.points;
    const auto cache = destination.getLengths();
    REQUIRE(&destination.copy(source) == &destination);
    REQUIRE(destination.points != old);
    REQUIRE(destination.points != source.points);
    REQUIRE(destination.cacheArcLengths == cache);
    REQUIRE(destination.toJSON() == source.toJSON());
    const auto copied = destination.points;
    REQUIRE(&destination.fromJSON(source.toJSON()) == &destination);
    REQUIRE(destination.points != copied);
    REQUIRE(destination.toJSON() == source.toJSON());
    REQUIRE(&source.getPoint(.3, *(*source.points)[1]) == (*source.points)[1].get());
    REQUIRE((*alias.points)[1]->equals(*(*source.points)[1]));
    const auto previous = source.points;
    REQUIRE(&source.copy(source) == &source);
    REQUIRE(source.points->empty());
    REQUIRE(previous->size() == 5);
    REQUIRE(alias.points == previous);
    auto duplicates = std::make_shared<typename C::Points>();
    auto shared = std::make_shared<V>();
    duplicates->push_back(shared); duplicates->push_back(shared);
    C repeated(duplicates);
    const auto independent = std::dynamic_pointer_cast<C>(repeated.clone());
    REQUIRE((*independent->points)[0] != (*independent->points)[1]);
}
}

TEST_CASE("EllipseCurve follows pinned angular sweep rotation and copy semantics", "[extras][remaining-curves][EllipseCurve]") {
    EllipseCurve defaults;
    REQUIRE(defaults.isEllipseCurve);
    REQUIRE(defaults.type == "EllipseCurve");
    REQUIRE(defaults.aEndAngle == std::numbers::pi * 2);
    REQUIRE(defaults.xRadius == 1);
    REQUIRE(defaults.yRadius == 1);
    REQUIRE_FALSE(defaults.aClockwise);
    EllipseCurve curve(2,-3,5,2,-.4,7.2,true,.7);
    expectedPoint(curve.getPoint(.3), {2.0236314836949267,-5.462995865539153});
    expectedPoint(curve.getTangent(.25), {-0.7828011581565624,-0.6222719235099271});
    REQUIRE(curve.getLength() == Approx(19.44178825161394).epsilon(5e-12));
    Vector2 target;
    REQUIRE(&curve.getPoint(.3, target) == &target);
    auto clone = std::dynamic_pointer_cast<EllipseCurve>(curve.clone());
    REQUIRE(clone->cacheArcLengths == nullptr);
    REQUIRE(clone->toJSON() == curve.toJSON());
    EllipseCurve restored;
    REQUIRE(&restored.fromJSON(curve.toJSON()) == &restored);
    REQUIRE(restored.toJSON() == curve.toJSON());
    EllipseCurve same(0,0,1,1,1,1,true);
    expectedPoint(same.getPoint(.7), {std::cos(1),std::sin(1)});
    EllipseCurve clockwise(0,0,1,1,0,std::numbers::pi*2,true);
    expectedPoint(clockwise.getPoint(.25), {0,-1});
    EllipseCurve wrapped(0,0,1,1,0,-4*std::numbers::pi,false);
    expectedPoint(wrapped.getPoint(.25), {0,1});
    curve.aRotation = std::numeric_limits<double>::quiet_NaN();
    REQUIRE(std::isnan(curve.getPoint(.3).x));
}

TEST_CASE("ArcCurve preserves inherited ellipse properties and concrete clone type", "[extras][remaining-curves][ArcCurve]") {
    ArcCurve curve(2,-3,5,-.4,7.2,true);
    REQUIRE(curve.isArcCurve);
    REQUIRE(curve.isEllipseCurve);
    REQUIRE(curve.type == "ArcCurve");
    expectedPoint(curve.getPoint(.3), {0.43136885550437776,-7.7475674121088955});
    REQUIRE(curve.getLength() == Approx(24.8312150833692).epsilon(5e-12));
    curve.yRadius = 8; curve.aRotation = .9;
    const auto clone = std::dynamic_pointer_cast<ArcCurve>(curve.clone());
    REQUIRE(clone != nullptr);
    REQUIRE(clone->toJSON() == curve.toJSON());
    ArcCurve restored;
    restored.fromJSON(curve.toJSON());
    REQUIRE(restored.toJSON() == curve.toJSON());
    REQUIRE(restored.yRadius == 8);
    REQUIRE(restored.aRotation == .9);
}

TEST_CASE("SplineCurve matches pinned interpolation and shares arrays and point identities", "[extras][remaining-curves][SplineCurve]") {
    SplineCurve curve(makePoints<SplineCurve>());
    REQUIRE(curve.isSplineCurve);
    expectedPoint(curve.getPoint(.3), {-6.3325000000000005,14.04});
    expectedPoint(curve.getTangent(.25), {0.43301839865994945,0.9013850822051435});
    REQUIRE(curve.getLength() == Approx(60.63266182848975).epsilon(5e-12));
    expectedPoint(*curve.getPoints(4).front(), {-10,0});
    expectedPoint(*curve.getPoints(4).back(), {10,0});
    sharedPointsContract<SplineCurve>();
    REQUIRE_THROWS_AS(SplineCurve().getPoint(.5), JSTypeError);
    REQUIRE_THROWS_AS(curve.getPoint(-.1), JSTypeError);
    REQUIRE_THROWS_AS(curve.getPoint(std::numeric_limits<double>::quiet_NaN()), JSTypeError);
    auto singleton = std::make_shared<SplineCurve::Points>(); singleton->push_back(std::make_shared<Vector2>(3,4));
    expectedPoint(SplineCurve(singleton).getPoint(.5), {3,4});
}

TEST_CASE("CatmullRomCurve3 retains centripetal chordal uniform and shared control semantics", "[extras][remaining-curves][CatmullRomCurve3]") {
    auto points = makePoints<CatmullRomCurve3>();
    CatmullRomCurve3 centripetal(points,false,"centripetal",.7);
    REQUIRE(centripetal.isCatmullRomCurve3);
    expectedPoint(centripetal.getPoint(.3), {-6.0287854007945505,14.036675713831034,-1.9771776960633818});
    REQUIRE(centripetal.getLength() == Approx(62.07687760353789).epsilon(5e-12));
    CatmullRomCurve3 chordal(points,false,"chordal",.7);
    expectedPoint(chordal.getPoint(.3), {-5.832074806746069,13.967750847762037,-1.9106681909924268});
    CatmullRomCurve3 uniform(points,false,"catmullrom",.7);
    expectedPoint(uniform.getPoint(.3), {-6.778000000000002,13.918499999999998,-2.1238999999999995});
    CatmullRomCurve3 closed(points,true);
    REQUIRE(closed.getPoint(0).equals(closed.getPoint(1)));
    REQUIRE(closed.getPoint(-.25).equals(closed.getPoint(.75)));
    sharedPointsContract<CatmullRomCurve3>();
    REQUIRE_THROWS_AS(CatmullRomCurve3().getPoint(.5), JSTypeError);
    auto one = std::make_shared<CatmullRomCurve3::Points>(); one->push_back(std::make_shared<Vector3>(3,4,5));
    REQUIRE_THROWS_AS(CatmullRomCurve3(one).getPoint(.5), JSTypeError);
    expectedPoint(CatmullRomCurve3(one,true).getPoint(.5), {3,4,5});
}

TEST_CASE("CatmullRom invalid type reuses module polynomial coefficients as upstream", "[extras][remaining-curves][CatmullRomCurve3]") {
    auto points = makePoints<CatmullRomCurve3>();
    CatmullRomCurve3 valid(points,false,"catmullrom",.7);
    const auto expected = valid.getPoint(.3);
    CatmullRomCurve3 invalid(points,false,"invalid",42);
    REQUIRE(invalid.getPoint(.3).equals(expected));
    invalid.points = std::make_shared<CatmullRomCurve3::Points>();
    invalid.closed = true;
    expectedPoint(invalid.getPoint(.3), {-10,0,1});
    const auto result = invalid.getPoint(std::numeric_limits<double>::infinity());
    REQUIRE(std::isnan(result.x));
    REQUIRE(std::isnan(result.y));
    REQUIRE(std::isnan(result.z));
}

TEST_CASE("Interpolations keep polynomial endpoints and IEEE arithmetic behavior", "[extras][remaining-curves][Interpolations]") {
    REQUIRE(CatmullRom(0,-3,4,7,-2) == 4);
    REQUIRE(CatmullRom(1,-3,4,7,-2) == 7);
    REQUIRE(QuadraticBezier(0,-3,4,7) == -3);
    REQUIRE(QuadraticBezier(1,-3,4,7) == 7);
    REQUIRE(CubicBezier(0,-3,4,7,-2) == -3);
    REQUIRE(CubicBezier(1,-3,4,7,-2) == -2);
    REQUIRE(std::isnan(CubicBezier(0,0,0,0,std::numeric_limits<double>::infinity())));
    REQUIRE(std::isnan(CatmullRom(std::numeric_limits<double>::quiet_NaN(),1,2,3,4)));
}
