#include "math/Cylindrical.h"
#include <cmath>

Cylindrical::Cylindrical(double radius, double theta, double y)
    : radius(radius), theta(theta), y(y) {}
Cylindrical& Cylindrical::set(double radius, double theta, double y) {
    this->radius = radius;
    this->theta = theta;
    this->y = y;
    return *this;
}
Cylindrical& Cylindrical::copy(const Cylindrical& other) {
    return set(other.radius, other.theta, other.y);
}
Cylindrical& Cylindrical::setFromVector3(const Vector3& vector) {
    return setFromCartesianCoords(vector.x, vector.y, vector.z);
}
Cylindrical& Cylindrical::setFromCartesianCoords(double x, double y, double z) {
    radius = std::sqrt(x * x + z * z);
    theta = std::atan2(x, z);
    this->y = y;
    return *this;
}
Cylindrical Cylindrical::clone() const {
    return Cylindrical().copy(*this);
}
