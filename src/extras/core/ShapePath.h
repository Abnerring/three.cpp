#pragma once
#include "extras/core/Shape.h"
#include "math/Color.h"
#include "core/JSValue.h"

class ShapePath {
  public:
    using Paths = std::vector<std::shared_ptr<Path>>;
    std::string type = "ShapePath";
    std::shared_ptr<Color> color = std::make_shared<Color>();
    std::shared_ptr<Paths> subPaths = std::make_shared<Paths>();
    std::shared_ptr<Path> currentPath;
    JSValue userData = JSValue::object();
    ShapePath& moveTo(double x, double y);
    ShapePath& lineTo(double x, double y);
    ShapePath& quadraticCurveTo(double cpx, double cpy, double x, double y);
    ShapePath& bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y);
    ShapePath& splineThru(const Path::Points& points);
    ShapePath& splineThru(const Path::PointsPtr& points) { return splineThru(*points); }
    std::vector<std::shared_ptr<Shape>> toShapes();
};
