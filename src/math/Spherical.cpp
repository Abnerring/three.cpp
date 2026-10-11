#include "math/Spherical.h"
#include "math/MathUtils.h"
#include <cmath>

Spherical::Spherical(double radius, double phi, double theta)
    : radius(radius), phi(phi), theta(theta) {}
Spherical& Spherical::set(double radius, double phi, double theta) {
    this->radius = radius;
    this->phi = phi;
    this->theta = theta;
    return *this;
}
Spherical& Spherical::copy(const Spherical& other) {
    return set(other.radius, other.phi, other.theta);
}
Spherical& Spherical::makeSafe() {
    phi = MathUtils::clamp(phi, 0.000001, MATH_PI - 0.000001);
    return *this;
}
Spherical& Spherical::setFromVector3(const Vector3& vector) {
    return setFromCartesianCoords(vector.x(), vector.y(), vector.z());
}
Spherical& Spherical::setFromCartesianCoords(double x, double y, double z) {
    radius = std::sqrt(x * x + y * y + z * z);
    if (radius == 0) {
        theta = 0;
        phi = 0;
    } else {
        theta = std::atan2(x, z);
        phi = std::acos(MathUtils::clamp(y / radius, -1, 1));
    }
    return *this;
}
Spherical Spherical::clone() const {
    return Spherical().copy(*this);
}
