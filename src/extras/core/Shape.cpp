#include "extras/core/Shape.h"
#include "math/MathUtils.h"

Shape::Shape(const PointsPtr& points) : Path(points), uuid(MathUtils::generateUUID()) { type = "Shape"; }
std::vector<Shape::Points> Shape::getPointsHoles() {
    std::vector<Points> points;
    const auto count = holes->size();
    for (std::size_t i = 0; i < count; ++i) {
        const auto hole = holes->at(i);
        points.push_back(hole->getPoints());
    }
    return points;
}
std::vector<Shape::Points> Shape::getPointsHoles(double divisions) {
    std::vector<Points> points;
    const auto count = holes->size();
    for (std::size_t i = 0; i < count; ++i) {
        const auto hole = holes->at(i);
        points.push_back(hole->getPoints(divisions));
    }
    return points;
}
Shape::ExtractedPoints Shape::extractPoints() { return {getPoints(), getPointsHoles()}; }
Shape::ExtractedPoints Shape::extractPoints(double divisions) { return {getPoints(divisions), getPointsHoles(divisions)}; }
std::shared_ptr<Curve<Vector2>> Shape::clone() const { auto result = std::make_shared<Shape>(); result->copy(*this); return result; }
Shape& Shape::copy(const Curve<Vector2>& source) {
    Path::copy(source);
    const auto& shape = dynamic_cast<const Shape&>(source);
    holes = std::make_shared<Holes>();
    const auto count = shape.holes->size();
    for (std::size_t i = 0; i < count; ++i) {
        const auto hole = shape.holes->at(i);
        const auto destination = holes;
        auto cloned = std::dynamic_pointer_cast<Path>(hole->clone());
        destination->push_back(std::move(cloned));
    }
    return *this;
}
JsonValue Shape::toJSON() const {
    auto json = Path::toJSON(); json["uuid"] = uuid;
    JsonValue::Array entries;
    const auto count = holes->size();
    for (std::size_t i = 0; i < count; ++i) {
        const auto hole = holes->at(i);
        entries.push_back(hole->toJSON());
    }
    json["holes"] = std::move(entries); return json;
}
Shape& Shape::fromJSON(const JsonValue& json) {
    Path::fromJSON(json); uuid = json.at("uuid").get<std::string>(); holes = std::make_shared<Holes>();
    for (const auto& entry : json.at("holes").get<JsonValue::Array>()) {
        auto hole = std::make_shared<Path>(); hole->fromJSON(entry); holes->push_back(hole);
    }
    return *this;
}
