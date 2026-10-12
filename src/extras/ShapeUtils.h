#pragma once
#include "math/Vector2.h"
#include <array>
#include <memory>
#include <vector>

class ShapeUtils {
  public:
    using Points = std::vector<std::shared_ptr<Vector2>>;
    using Faces = std::vector<std::array<std::size_t, 3>>;
    static double area(const Points& contour);
    static bool isClockWise(const Points& points);
    static Faces triangulateShape(Points& contour, std::vector<Points>& holes);
    static double area(const std::vector<Vector2>& contour);
    static bool isClockWise(const std::vector<Vector2>& points);
    static Faces triangulateShape(std::vector<Vector2>& contour, std::vector<std::vector<Vector2>>& holes);
};
