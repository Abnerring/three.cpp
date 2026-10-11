#pragma once
#include "math/Vector3.h"

class Spherical {
  public:
    double radius, phi, theta;
    Spherical(double radius = 1, double phi = 0, double theta = 0);
    Spherical& set(double radius, double phi, double theta);
    Spherical& copy(const Spherical& other);
    Spherical& makeSafe();
    Spherical& setFromVector3(const Vector3& vector);
    Spherical& setFromCartesianCoords(double x, double y, double z);
    Spherical clone() const;
};
