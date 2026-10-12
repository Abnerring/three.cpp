#include "catch.hpp"
#include "extras/core/Shape.h"

TEST_CASE("Shape copy and JSON preserve upstream UUID and hole semantics", "[extras][Shape]") {
    Shape shape;
    shape.moveTo(0, 0).lineTo(10, 0).lineTo(10, 10).lineTo(0, 10);
    auto hole = std::make_shared<Path>();
    hole->moveTo(2, 2).lineTo(2, 4).lineTo(4, 4).lineTo(4, 2);
    shape.holes->push_back(hole);
    const auto points = shape.extractPoints();
    REQUIRE(points.shape.size() == 4);
    REQUIRE(points.holes.size() == 1);
    REQUIRE(points.holes[0].size() == 4);
    auto cloned = std::dynamic_pointer_cast<Shape>(shape.clone());
    REQUIRE(cloned->uuid != shape.uuid);
    REQUIRE(cloned->holes != shape.holes);
    REQUIRE(cloned->holes->front() != hole);
    REQUIRE(cloned->holes->front()->curves->front() != hole->curves->front());
    Shape copied;
    const auto uuid = copied.uuid;
    REQUIRE(&copied.copy(shape) == &copied);
    REQUIRE(copied.uuid == uuid);
    Shape restored;
    restored.fromJSON(shape.toJSON());
    REQUIRE(restored.uuid == shape.uuid);
    REQUIRE(restored.toJSON() == shape.toJSON());
}

TEST_CASE("Shape extraction leaves omitted divisions to virtual sampling defaults", "[extras][Shape]") {
    class DefaultHole : public Path {
      public:
        Points getPoints() override { return getPoints(7); }
        Points getPoints(double divisions) override { return {std::make_shared<Vector2>(divisions, 0)}; }
    };
    class DefaultShape : public Shape {
      public:
        Points getPoints() override { return getPoints(9); }
        Points getPoints(double divisions) override { return {std::make_shared<Vector2>(divisions, 0)}; }
    } shape;
    shape.holes->push_back(std::make_shared<DefaultHole>());
    Shape& base = shape;
    REQUIRE(base.getPointsHoles()[0][0]->x == 7);
    REQUIRE(base.extractPoints().shape[0]->x == 9);
    REQUIRE(base.extractPoints().holes[0][0]->x == 7);
    REQUIRE(base.extractPoints(3).shape[0]->x == 3);
    REQUIRE(base.extractPoints(3).holes[0][0]->x == 3);
    class CustomHoles : public DefaultShape {
      public:
        std::vector<Points> getPointsHoles() override { return {{std::make_shared<Vector2>(11, 0)}}; }
        std::vector<Points> getPointsHoles(double d) override { return {{std::make_shared<Vector2>(d + 1, 0)}}; }
    } custom;
    REQUIRE(custom.extractPoints().holes[0][0]->x == 11);
    REQUIRE(custom.extractPoints(3).holes[0][0]->x == 4);
}

TEST_CASE("Shape serialization rereads holes and retains an active serializer", "[extras][Shape]") {
    Shape shape;
    class ReplacingHole : public Path {
      public:
        Shape* owner;
        bool* destroyed;
        ~ReplacingHole() override { *destroyed = true; }
        JsonValue toJSON() const override {
            owner->holes = std::make_shared<Shape::Holes>();
            owner->holes->push_back(std::make_shared<Path>());
            auto second = std::make_shared<Path>(); second->lineTo(20, 0);
            owner->holes->push_back(second);
            REQUIRE_FALSE(*destroyed);
            return Path::toJSON();
        }
    };
    bool destroyed = false;
    auto first = std::make_shared<ReplacingHole>(); first->owner = &shape; first->destroyed = &destroyed;
    shape.holes->push_back(first); first.reset();
    auto second = std::make_shared<Path>(); second->lineTo(10, 0); shape.holes->push_back(second);
    const auto json = shape.toJSON();
    REQUIRE(destroyed);
    const auto& holes = json.at("holes").get<JsonValue::Array>();
    REQUIRE(holes.size() == 2);
    const auto& curves = holes[1].at("curves").get<JsonValue::Array>();
    REQUIRE(curves[0].at("v2").get<JsonValue::Array>()[0].get<double>() == 20);
}

TEST_CASE("Shape copy retains clone receiver and captures each push destination", "[extras][Shape]") {
    Shape source, destination;
    class ReplacingHole : public Path {
      public:
        Shape* source;
        Shape* destination;
        bool* destroyed;
        ~ReplacingHole() override { *destroyed = true; }
        std::shared_ptr<Curve<Vector2>> clone() const override {
            source->holes = std::make_shared<Shape::Holes>();
            source->holes->push_back(std::make_shared<Path>());
            auto second = std::make_shared<Path>(); second->lineTo(20, 0); source->holes->push_back(second);
            destination->holes = std::make_shared<Shape::Holes>();
            REQUIRE_FALSE(*destroyed);
            return Path::clone();
        }
    };
    bool destroyed = false;
    auto first = std::make_shared<ReplacingHole>();
    first->source = &source; first->destination = &destination; first->destroyed = &destroyed;
    source.holes->push_back(first); first.reset(); source.holes->push_back(std::make_shared<Path>());
    destination.copy(source);
    REQUIRE(destroyed);
    REQUIRE(destination.holes->size() == 1);
    REQUIRE(destination.holes->front()->currentPoint->x == 20);
}

TEST_CASE("Shape retains holes while virtual sampling replaces its holes array", "[extras][Shape]") {
    class ReplacingHole : public Path {
      public:
        Shape* owner;
        bool* destroyed;
        ~ReplacingHole() override { *destroyed = true; }
        Points getPoints() override { return getPoints(7); }
        Points getPoints(double d) override {
            owner->holes = std::make_shared<Shape::Holes>();
            REQUIRE_FALSE(*destroyed);
            return {std::make_shared<Vector2>(d, 0)};
        }
    };
    for (const bool explicitDivisions : {false, true}) {
        Shape shape;
        bool destroyed = false;
        auto hole = std::make_shared<ReplacingHole>(); hole->owner = &shape; hole->destroyed = &destroyed;
        shape.holes->push_back(hole); hole.reset();
        const auto result = explicitDivisions ? shape.getPointsHoles(3) : shape.getPointsHoles();
        REQUIRE(result[0][0]->x == (explicitDivisions ? 3 : 7));
        REQUIRE(destroyed);
    }
}
