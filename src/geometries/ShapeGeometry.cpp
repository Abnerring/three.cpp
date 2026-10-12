#include "geometries/ShapeGeometry.h"
#include "extras/ShapeUtils.h"

std::shared_ptr<Shape> ShapeGeometry::defaultShape() {
    auto points = std::make_shared<Shape::Points>();
    for (const auto& p : {Vector2(0, .5), Vector2(-.5, -.5), Vector2(.5, -.5)})
        points->push_back(std::make_shared<Vector2>(p));
    return std::make_shared<Shape>(points);
}
ShapeGeometry::ShapeGeometry(Shapes shapes, double curveSegments) : parameters{shapes, curveSegments} {
    type = "ShapeGeometry";
    std::vector<std::uint32_t> indices;
    std::vector<double> vertices, normals, uvs;
    std::size_t groupStart = 0, groupCount = 0;
    const auto addShape = [&](const std::shared_ptr<Shape>& shape) {
        if (!shape) throw JSTypeError("ShapeGeometry shape is null");
        const auto offset = vertices.size() / 3;
        auto points = shape->extractPoints(curveSegments);
        auto& shapeVertices = points.shape;
        auto& holes = points.holes;
        if (!ShapeUtils::isClockWise(shapeVertices)) std::reverse(shapeVertices.begin(), shapeVertices.end());
        for (auto& hole : holes)
            if (ShapeUtils::isClockWise(hole)) std::reverse(hole.begin(), hole.end());
        const auto faces = ShapeUtils::triangulateShape(shapeVertices, holes);
        for (const auto& hole : holes) shapeVertices.insert(shapeVertices.end(), hole.begin(), hole.end());
        for (const auto& vertex : shapeVertices) {
            vertices.insert(vertices.end(), {vertex->x, vertex->y, 0});
            normals.insert(normals.end(), {0, 0, 1});
            uvs.insert(uvs.end(), {vertex->x, vertex->y});
        }
        for (const auto& face : faces) {
            for (const auto index : face) indices.push_back(std::uint32_t(index + offset));
            groupCount += 3;
        }
    };
    if (const auto single = std::get_if<std::shared_ptr<Shape>>(&shapes)) addShape(*single);
    else {
        const auto list = std::get<std::shared_ptr<ShapeList>>(shapes);
        if (!list) throw JSTypeError("ShapeGeometry shapes array is null");
        for (std::size_t i = 0; i < list->size(); ++i) {
            const auto shape = list->at(i);
            addShape(shape); addGroup(groupStart, groupCount, i);
            groupStart += groupCount; groupCount = 0;
        }
    }
    setIndex(indices);
    setAttribute("position", std::make_shared<Float32BufferAttribute>(vertices, 3));
    setAttribute("normal", std::make_shared<Float32BufferAttribute>(normals, 3));
    setAttribute("uv", std::make_shared<Float32BufferAttribute>(uvs, 2));
}
BufferGeometry& ShapeGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source); parameters = dynamic_cast<const ShapeGeometry&>(source).parameters; return *this;
}
std::shared_ptr<BufferGeometry> ShapeGeometry::clone() const {
    auto result = std::make_shared<ShapeGeometry>(); result->copy(*this); return result;
}
JsonValue ShapeGeometry::toJSON() const {
    JsonValue result = JsonValue::Object{
        {"metadata", JsonValue::Object{{"version", 4.7}, {"type", "BufferGeometry"},
                                        {"generator", "BufferGeometry.toJSON"}}},
        {"uuid", uuid}, {"type", type}, {"name", name}};
    if (userData.size()) result["userData"] = userData.sharedCopy();
    if (_transformed) { result = BufferGeometry::toJSON(); result["type"] = "BufferGeometry"; }
    else { result["shapes"] = nullptr; result["curveSegments"] = parameters.curveSegments; }
    JsonValue::Array ids;
    if (const auto shape = std::get_if<std::shared_ptr<Shape>>(&parameters.shapes)) {
        if (!*shape) throw JSTypeError("ShapeGeometry shape is null");
        ids.emplace_back((*shape)->uuid);
    } else {
        const auto list = std::get<std::shared_ptr<ShapeList>>(parameters.shapes);
        if (!list) throw JSTypeError("ShapeGeometry shapes array is null");
        for (const auto& shape : *list) {
            if (!shape) throw JSTypeError("ShapeGeometry shape is null");
            ids.emplace_back(shape->uuid);
        }
    }
    result["shapes"] = std::move(ids); return result;
}
std::shared_ptr<ShapeGeometry> ShapeGeometry::fromJSON(const JsonValue& data, const ShapeMap& shapes) {
    auto list = std::make_shared<ShapeList>();
    for (const auto& id : data.at("shapes").get<JsonValue::Array>()) {
        const auto found = shapes.find(id.get<std::string>());
        list->push_back(found == shapes.end() ? nullptr : found->second);
    }
    return std::make_shared<ShapeGeometry>(list, data.contains("curveSegments") ? data.at("curveSegments").get<double>() : 12);
}
