#include "geometries/BoxLineGeometry.h"
#include "core/BufferAttribute.h"
#include <cmath>
#include <vector>

BoxLineGeometry::BoxLineGeometry(double width, double height, double depth, double widthSegments,
                                 double heightSegments, double depthSegments) {
    widthSegments = std::floor(widthSegments);
    heightSegments = std::floor(heightSegments);
    depthSegments = std::floor(depthSegments);
    const double widthHalf = width / 2;
    const double heightHalf = height / 2;
    const double depthHalf = depth / 2;
    const double segmentWidth = width / widthSegments;
    const double segmentHeight = height / heightSegments;
    const double segmentDepth = depth / depthSegments;
    std::vector<double> vertices;
    double x = -widthHalf;
    double y = -heightHalf;
    double z = -depthHalf;
    for (double i = 0; i <= widthSegments; i += 1) {
        vertices.push_back(x);
        vertices.push_back(-heightHalf);
        vertices.push_back(-depthHalf);
        vertices.push_back(x);
        vertices.push_back(heightHalf);
        vertices.push_back(-depthHalf);
        vertices.push_back(x);
        vertices.push_back(heightHalf);
        vertices.push_back(-depthHalf);
        vertices.push_back(x);
        vertices.push_back(heightHalf);
        vertices.push_back(depthHalf);
        vertices.push_back(x);
        vertices.push_back(heightHalf);
        vertices.push_back(depthHalf);
        vertices.push_back(x);
        vertices.push_back(-heightHalf);
        vertices.push_back(depthHalf);
        vertices.push_back(x);
        vertices.push_back(-heightHalf);
        vertices.push_back(depthHalf);
        vertices.push_back(x);
        vertices.push_back(-heightHalf);
        vertices.push_back(-depthHalf);
        x += segmentWidth;
    }
    for (double i = 0; i <= heightSegments; i += 1) {
        vertices.push_back(-widthHalf);
        vertices.push_back(y);
        vertices.push_back(-depthHalf);
        vertices.push_back(widthHalf);
        vertices.push_back(y);
        vertices.push_back(-depthHalf);
        vertices.push_back(widthHalf);
        vertices.push_back(y);
        vertices.push_back(-depthHalf);
        vertices.push_back(widthHalf);
        vertices.push_back(y);
        vertices.push_back(depthHalf);
        vertices.push_back(widthHalf);
        vertices.push_back(y);
        vertices.push_back(depthHalf);
        vertices.push_back(-widthHalf);
        vertices.push_back(y);
        vertices.push_back(depthHalf);
        vertices.push_back(-widthHalf);
        vertices.push_back(y);
        vertices.push_back(depthHalf);
        vertices.push_back(-widthHalf);
        vertices.push_back(y);
        vertices.push_back(-depthHalf);
        y += segmentHeight;
    }
    for (double i = 0; i <= depthSegments; i += 1) {
        vertices.push_back(-widthHalf);
        vertices.push_back(-heightHalf);
        vertices.push_back(z);
        vertices.push_back(-widthHalf);
        vertices.push_back(heightHalf);
        vertices.push_back(z);
        vertices.push_back(-widthHalf);
        vertices.push_back(heightHalf);
        vertices.push_back(z);
        vertices.push_back(widthHalf);
        vertices.push_back(heightHalf);
        vertices.push_back(z);
        vertices.push_back(widthHalf);
        vertices.push_back(heightHalf);
        vertices.push_back(z);
        vertices.push_back(widthHalf);
        vertices.push_back(-heightHalf);
        vertices.push_back(z);
        vertices.push_back(widthHalf);
        vertices.push_back(-heightHalf);
        vertices.push_back(z);
        vertices.push_back(-widthHalf);
        vertices.push_back(-heightHalf);
        vertices.push_back(z);
        z += segmentDepth;
    }
    setAttribute("position", std::make_shared<Float32BufferAttribute>(vertices, 3));
}
