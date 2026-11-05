#include "utils/GeometryUtils.h"

#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace {
void appendRange(std::vector<Vector3>& target, const std::vector<Vector3>& source) {
    target.insert(target.end(), source.begin(), source.end());
}
}  // namespace

std::vector<Vector3> GeometryUtils::hilbert2D(const Vector3& center, double size, int iterations,
                                              int v0, int v1, int v2, int v3) {
    const double half = size / 2;
    const Vector3 corners[4] = {
        Vector3(center.x - half, center.y, center.z - half),
        Vector3(center.x - half, center.y, center.z + half),
        Vector3(center.x + half, center.y, center.z + half),
        Vector3(center.x + half, center.y, center.z - half),
    };
    const Vector3 vec[4] = {corners[v0], corners[v1], corners[v2], corners[v3]};

    // if ( 0 <= -- iterations ) { return [ ...recursive calls ] }
    if (0 <= --iterations) {
        std::vector<Vector3> points;
        appendRange(points, hilbert2D(vec[0], half, iterations, v0, v3, v2, v1));
        appendRange(points, hilbert2D(vec[1], half, iterations, v0, v1, v2, v3));
        appendRange(points, hilbert2D(vec[2], half, iterations, v0, v1, v2, v3));
        appendRange(points, hilbert2D(vec[3], half, iterations, v2, v1, v0, v3));
        return points;
    }
    return {vec[0], vec[1], vec[2], vec[3]};
}

std::vector<Vector3> GeometryUtils::hilbert3D(const Vector3& center, double size, int iterations,
                                              int v0, int v1, int v2, int v3, int v4, int v5,
                                              int v6, int v7) {
    const double half = size / 2;
    const Vector3 corners[8] = {
        Vector3(center.x - half, center.y + half, center.z - half),
        Vector3(center.x - half, center.y + half, center.z + half),
        Vector3(center.x - half, center.y - half, center.z + half),
        Vector3(center.x - half, center.y - half, center.z - half),
        Vector3(center.x + half, center.y - half, center.z - half),
        Vector3(center.x + half, center.y - half, center.z + half),
        Vector3(center.x + half, center.y + half, center.z + half),
        Vector3(center.x + half, center.y + half, center.z - half),
    };
    const Vector3 vec[8] = {corners[v0], corners[v1], corners[v2], corners[v3],
                            corners[v4], corners[v5], corners[v6], corners[v7]};

    // if ( -- iterations >= 0 ) { return [ ...recursive calls ] }
    if (--iterations >= 0) {
        std::vector<Vector3> points;
        appendRange(points, hilbert3D(vec[0], half, iterations, v0, v3, v4, v7, v6, v5, v2, v1));
        appendRange(points, hilbert3D(vec[1], half, iterations, v0, v7, v6, v1, v2, v5, v4, v3));
        appendRange(points, hilbert3D(vec[2], half, iterations, v0, v7, v6, v1, v2, v5, v4, v3));
        appendRange(points, hilbert3D(vec[3], half, iterations, v2, v3, v0, v1, v6, v7, v4, v5));
        appendRange(points, hilbert3D(vec[4], half, iterations, v2, v3, v0, v1, v6, v7, v4, v5));
        appendRange(points, hilbert3D(vec[5], half, iterations, v4, v3, v2, v5, v6, v1, v0, v7));
        appendRange(points, hilbert3D(vec[6], half, iterations, v4, v3, v2, v5, v6, v1, v0, v7));
        appendRange(points, hilbert3D(vec[7], half, iterations, v6, v5, v2, v1, v0, v3, v4, v7));
        return points;
    }
    return {vec[0], vec[1], vec[2], vec[3], vec[4], vec[5], vec[6], vec[7]};
}

std::vector<double> GeometryUtils::gosper(double size) {
    // const gosper = fractalize( { axiom: 'A', steps: 4, rules: { ... } } )
    std::string input = "A";
    const std::pair<char, std::string> rules[] = {
        {'A', "A+BF++BF-FA--FAFA-BF+"},
        {'B', "-FA+BFBF++BF+FA--FA-B"},
    };
    for (int step = 0; step < 4; ++step) {
        std::string output;
        for (const char character : input) {
            bool replaced = false;
            for (const auto& [symbol, replacement] : rules) {
                if (character == symbol) {
                    output += replacement;
                    replaced = true;
                    break;
                }
            }
            if (!replaced) output += character;
        }
        input = std::move(output);
    }

    // toPoints( { fractal, size, angle: Math.PI / 3 } )
    double currentX = 0;
    double currentY = 0;
    double angle = 0;
    std::vector<double> path{0, 0, 0};
    const double turn = std::numbers::pi_v<double> / 3;
    for (const char character : input) {
        if (character == '+') {
            angle += turn;
        } else if (character == '-') {
            angle -= turn;
        } else if (character == 'F') {
            currentX += size * std::cos(angle);
            currentY += -size * std::sin(angle);
            path.push_back(currentX);
            path.push_back(currentY);
            path.push_back(0);
        }
    }
    return path;
}
