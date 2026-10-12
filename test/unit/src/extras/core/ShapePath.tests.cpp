#include "catch.hpp"
#include "extras/core/ShapePath.h"

namespace {
void rectangle(ShapePath& path, double a, double b, double c, double d, bool reversed) {
    path.moveTo(a, b);
    if (reversed) path.lineTo(a, d).lineTo(c, d).lineTo(c, b);
    else path.lineTo(c, b).lineTo(c, d).lineTo(a, d);
    path.lineTo(a, b);
}
void rule(ShapePath& path, const char* rule) { auto style = JSValue::object(); style.set("fillRule", rule); path.userData.set("style", style); }
}
TEST_CASE("ShapePath classifies nested winding for nonzero and evenodd", "[extras][ShapePath]") {
    ShapePath path;
    REQUIRE(path.type == "ShapePath");
    REQUIRE(path.color->r == 1);
    REQUIRE_FALSE(path.currentPath);
    REQUIRE_THROWS_AS(path.lineTo(1, 2), JSTypeError);
    rectangle(path, 0, 0, 10, 10, false);
    rectangle(path, 2, 2, 8, 8, false);
    rectangle(path, 3, 3, 7, 7, true);
    const auto nonzero = path.toShapes();
    REQUIRE(nonzero.size() == 1);
    REQUIRE(nonzero[0]->holes->empty());
    rule(path, "evenodd");
    const auto evenodd = path.toShapes();
    REQUIRE(evenodd.size() == 2);
    REQUIRE(evenodd[0]->holes->size() == 1);
    REQUIRE(evenodd[1]->holes->empty());
    REQUIRE(evenodd[0]->curves == path.subPaths->at(0)->curves);
    REQUIRE(evenodd[0]->holes->front()->curves == path.subPaths->at(1)->curves);
    REQUIRE(evenodd[0]->holes->front() != path.subPaths->at(1));
    REQUIRE(evenodd[0]->currentPoint->equals(Vector2()));
    REQUIRE(path.currentPath == path.subPaths->back());
    evenodd[0]->lineTo(20, 20);
    REQUIRE(path.subPaths->at(0)->curves->size() == 5);
}
TEST_CASE("ShapePath ignores zero area and resolves holes independent of input order", "[extras][ShapePath]") {
    ShapePath path;
    path.moveTo(0, 0).lineTo(1, 1).lineTo(2, 2);
    rectangle(path, 2, 2, 4, 4, true);
    rectangle(path, 0, 0, 10, 10, false);
    auto shapes = path.toShapes();
    REQUIRE(shapes.size() == 1);
    REQUIRE(shapes[0]->holes->size() == 1);
    REQUIRE(shapes[0]->curves == path.subPaths->at(2)->curves);
    REQUIRE(shapes[0]->holes->front()->curves == path.subPaths->at(1)->curves);
}

TEST_CASE("ShapePath conversion retains the iterated array when sampling replaces subPaths", "[extras][ShapePath]") {
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
    auto second = std::make_shared<Path>();
    second->moveTo(10, 0).lineTo(12, 0).lineTo(10, 2);
    owner.subPaths->push_back(first); owner.subPaths->push_back(second);
    auto shapes = owner.toShapes();
    REQUIRE(shapes.size() == 2);
    REQUIRE(shapes[0]->curves == first->curves);
    REQUIRE(shapes[1]->curves == second->curves);
    REQUIRE(owner.subPaths->empty());
}
