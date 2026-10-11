#pragma once
#include "math/Vector3.h"

class Cylindrical {
  public:
    double radius, theta, y;
    Cylindrical(double radius = 1, double theta = 0, double y = 0);
    Cylindrical& set(double radius, double theta, double y);
    Cylindrical& copy(const Cylindrical& other);
    Cylindrical& setFromVector3(const Vector3& vector);
    Cylindrical& setFromCartesianCoords(double x, double y, double z);
    Cylindrical clone() const;
};
