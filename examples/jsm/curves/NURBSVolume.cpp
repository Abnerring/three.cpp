#include "curves/NURBSVolume.h"
#include "core/JSValue.h"
#include <cmath>
#include <cstddef>

namespace {
// `for ( let i = 0; i < bound; ++ i )` runs one time per integral step below the bound,
// which is ceil( bound ) for a positive fractional bound and no iteration otherwise.
std::size_t iterationCount(double bound) {
    return bound > 0 ? std::size_t(std::ceil(bound)) : 0;
}
const Vector4& controlAt(const NURBSUtils::ControlVolume& volume, std::size_t plane, std::size_t row,
                         std::size_t column) {
    if (plane >= volume.size() || row >= volume[plane].size() || column >= volume[plane][row].size())
        throw JSTypeError("Cannot read properties of undefined (reading 'x')");
    return volume[plane][row][column];
}
}

NURBSVolume::NURBSVolume(double degree1, double degree2, double degree3, std::vector<double> knots1,
                         std::vector<double> knots2, std::vector<double> knots3,
                         const NURBSUtils::ControlVolume& controlPoints)
    : degree1(degree1), degree2(degree2), degree3(degree3), knots1(std::move(knots1)), knots2(std::move(knots2)),
      knots3(std::move(knots3)) {
    const double len1 = double(this->knots1.size()) - degree1 - 1;
    const double len2 = double(this->knots2.size()) - degree2 - 1;
    const double len3 = double(this->knots3.size()) - degree3 - 1;
    // ensure Vector4 for control points
    for (std::size_t i = 0; i < iterationCount(len1); ++i) {
        this->controlPoints.emplace_back();
        for (std::size_t j = 0; j < iterationCount(len2); ++j) {
            this->controlPoints[i].emplace_back();
            for (std::size_t k = 0; k < iterationCount(len3); ++k) {
                const Vector4& point = controlAt(controlPoints, i, j, k);
                this->controlPoints[i][j].emplace_back(point.x, point.y, point.z, point.w);
            }
        }
    }
}

void NURBSVolume::getPoint(double t1, double t2, double t3, Vector3& target) const {
    // linear mapping t1 -> u, t2 -> v, t3 -> w
    const double first1 = NURBSUtils::knotValue(knots1, 0);
    const double last1 = NURBSUtils::knotValue(knots1, double(knots1.size()) - 1);
    const double first2 = NURBSUtils::knotValue(knots2, 0);
    const double last2 = NURBSUtils::knotValue(knots2, double(knots2.size()) - 1);
    const double first3 = NURBSUtils::knotValue(knots3, 0);
    const double last3 = NURBSUtils::knotValue(knots3, double(knots3.size()) - 1);
    const double u = first1 + t1 * (last1 - first1);
    const double v = first2 + t2 * (last2 - first2);
    const double w = first3 + t3 * (last3 - first3);
    NURBSUtils::calcVolumePoint(degree1, degree2, degree3, knots1, knots2, knots3, controlPoints, u, v, w, target);
}
