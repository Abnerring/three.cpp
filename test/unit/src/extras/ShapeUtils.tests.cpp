#include "catch.hpp"
#include "extras/ShapeUtils.h"

TEST_CASE("ShapeUtils area and triangulation preserve endpoint mutation and index order", "[extras][ShapeUtils]") {
    std::vector<Vector2> contour{{0,0}, {10,0}, {10,10}, {0,10}, {0,0}};
    std::vector<std::vector<Vector2>> holes{{{2,2},{2,4},{4,4},{4,2},{2,2}}};
    REQUIRE(ShapeUtils::area(contour) == 100);
    REQUIRE_FALSE(ShapeUtils::isClockWise(contour));
    REQUIRE(ShapeUtils::isClockWise(holes.front()));
    const auto faces = ShapeUtils::triangulateShape(contour, holes);
    REQUIRE(contour.size() == 4);
    REQUIRE(holes.front().size() == 4);
    REQUIRE(faces.size() == 8);
    auto vertices = contour;
    vertices.insert(vertices.end(), holes[0].begin(), holes[0].end());
    double area = 0;
    for (const auto& face : faces) {
        std::vector<Vector2> triangle{vertices.at(face[0]), vertices.at(face[1]), vertices.at(face[2])};
        area += std::abs(ShapeUtils::area(triangle));
    }
    REQUIRE(area == 96);
    std::vector<Vector2> empty;
    std::vector<std::vector<Vector2>> noHoles;
    REQUIRE(ShapeUtils::area(empty) == 0);
    REQUIRE(ShapeUtils::triangulateShape(empty, noHoles).empty());
}

#include "../../utils/parity/ShapeUtilsFixtures.h"

TEST_CASE("Managed ShapeUtils area retains getter conversion and reentry order", "[extras][ShapeUtils][managed]") {
    namespace F=ShapeUtilsFixtures;
    const auto result=F::run(F::request("areaOrder"));REQUIRE(result.get("result").strictEquals(1));
    REQUIRE(result.get("trace").toJSON()->dump()==R"(["length","get:2x","get:0y","value:2x","value:0y","get:0x","get:2y","value:0x","value:2y","get:0x","get:1y","value:0x","value:1y","get:1x","get:0y","value:1x","value:0y","get:1x","get:2y","value:1x","value:2y","get:2x","get:1y","value:2x","value:1y"])");
    const auto objectLength=F::run(F::request("areaOrder","lengthObject"));REQUIRE(objectLength.get("result").strictEquals(1));unsigned conversions=0;for(unsigned i=0;i<objectLength.get("trace").length();++i)if(objectLength.get("trace").get(i).strictEquals("length:valueOf"))++conversions;REQUIRE(conversions==5);
    const auto thrown=F::run(F::request("areaOrder","throw"));REQUIRE(thrown.get("result").strictEquals("reason"));REQUIRE(thrown.get("trace").toJSON()->dump()==R"(["length","get:2x","get:0y"])");
    const auto reentered=F::run(F::request("areaOrder","reenter"));REQUIRE(reentered.get("result").strictEquals(1));REQUIRE(reentered.get("trace").length()==27);
    REQUIRE(F::run(F::request("dynamicArea")).get("result").strictEquals(true));REQUIRE(F::run(F::request("dynamicArea","throw")).get("result").strictEquals("reason"));
}
TEST_CASE("Managed ShapeUtils preserves endpoint callbacks raw coordinates and triangle slices", "[extras][ShapeUtils][managed]") {
    namespace F=ShapeUtilsFixtures;
    const auto result=F::run(F::request("triangulateOrder"));REQUIRE(result.get("result").toJSON()->dump()==R"([[0,1,2],[3]])");REQUIRE(result.get("length").strictEquals(2));
    REQUIRE(result.get("trace").toJSON()->dump()==R"([["get:equals",true],["equals",true,true],["pop",true],"x:a","y:a","x:b","y:b",["forEach",true,"removeDupEndPts",1],["get:earcut",true],["earcut",true,true,4,0],["slice",0,3],["slice",3,6]])");
    REQUIRE(F::run(F::request("triangulateOrder","keep")).get("length").strictEquals(3));
    for(const auto* variant:{"equalsThrow","forEachThrow","earcutThrow"}){CAPTURE(variant);const auto failure=F::run(F::request("triangulateOrder",variant));REQUIRE(failure.get("result").strictEquals("reason"));REQUIRE(failure.get("length").strictEquals(std::string(variant)=="equalsThrow"?3:2));}
}
TEST_CASE("Managed ShapeUtils uses real Earcut and mutates aliased duplicate endpoint arrays", "[extras][ShapeUtils][managed]") {
    namespace F=ShapeUtilsFixtures;
    auto request=F::request("numeric");request.set("alias",true);request.set("contour",F::list({F::list({0,0}),F::list({10,0}),F::list({10,10}),F::list({0,10}),F::list({0,0})}));request.set("holes",F::list({F::list({F::list({2,2}),F::list({2,4}),F::list({4,4}),F::list({4,2}),F::list({2,2})})}));
    const auto result=F::run(request);REQUIRE(result.get("area").strictEquals(100));REQUIRE(result.get("clockwise").strictEquals(false));REQUIRE(result.get("sameContour").toBoolean());REQUIRE(result.get("faces").length()==8);REQUIRE(result.get("contour").length()==4);REQUIRE(result.get("holes").get("0").length()==4);
    const auto nullish=F::run(F::request("nullish"));REQUIRE(nullish.get("undefined").strictEquals("TypeError"));REQUIRE(nullish.get("null").strictEquals("TypeError"));REQUIRE(nullish.get("empty").strictEquals(0));REQUIRE(nullish.get("missingHoles").strictEquals("TypeError"));
}
