#include "geometries/LoftGeometry.h"
#include "core/BufferAttribute.h"
#include "extras/ShapeUtils.h"
#include "math/Vector2.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>

namespace {
// The module-level temporary of the pinned source, one per owning thread.
thread_local Vector3 temporary;
constexpr double EPSILON = 2.220446049250313e-16; // Number.EPSILON
}

LoftGeometry::LoftGeometry(std::vector<Section> sections, Options options) {
    type = "LoftGeometry";
    const bool closed = options.closed;
    const bool capStart = options.capStart;
    const bool capEnd = options.capEnd;
    parameters = Parameters{std::move(sections), closed, capStart, capEnd};
    const auto& rows = parameters.sections;
    if (rows.size() < 2) {
        std::cerr << "THREE.LoftGeometry: At least two sections are required.\n";
        return;
    }
    const double rowCount = double(rows.size());
    const double columns = double(rows[0].size());
    for (std::size_t i = 1; i < rows.size(); ++i) {
        if (double(rows[i].size()) != columns) {
            std::cerr << "THREE.LoftGeometry: All sections must have the same number of points.\n";
            return;
        }
    }
    // closed sections repeat their first point so the surface can wrap
    // around with continuous uvs
    const double pointsPerRow = closed ? columns + 1 : columns;
    // buffers
    std::vector<std::uint32_t> indices;
    std::vector<double> vertices, uvs;
    // uvs follow arc length so uneven sections and points map evenly
    std::vector<double> rowU{0};
    for (double i = 1; i < rowCount; i += 1) {
        double distance = 0;
        for (double j = 0; j < columns; j += 1)
            distance += rows[std::size_t(i)][std::size_t(j)].distanceTo(rows[std::size_t(i - 1)][std::size_t(j)]);
        rowU.push_back(rowU[std::size_t(i - 1)] + distance / columns);
    }
    const double totalU = rowU[std::size_t(rowCount - 1)];
    // generate vertices and uvs
    for (double i = 0; i < rowCount; i += 1) {
        const auto& section = rows[std::size_t(i)];
        // distance around the section
        std::vector<double> colV{0};
        for (double j = 1; j < pointsPerRow; j += 1)
            colV.push_back(colV[std::size_t(j - 1)] +
                           section[std::size_t(std::fmod(j, columns))].distanceTo(
                               section[std::size_t(std::fmod(j - 1, columns))]));
        const double totalV = colV[std::size_t(pointsPerRow - 1)];
        for (double j = 0; j < pointsPerRow; j += 1) {
            const auto& point = section[std::size_t(std::fmod(j, columns))];
            vertices.push_back(point.x);
            vertices.push_back(point.y);
            vertices.push_back(point.z);
            uvs.push_back(totalU > 0 ? rowU[std::size_t(i)] / totalU : i / (rowCount - 1));
            uvs.push_back(totalV > 0 ? colV[std::size_t(j)] / totalV : j / (pointsPerRow - 1));
        }
    }
    // generate indices
    for (double i = 0; i < rowCount - 1; i += 1) {
        for (double j = 0; j < pointsPerRow - 1; j += 1) {
            const double a = i * pointsPerRow + j;
            const double b = i * pointsPerRow + j + 1;
            const double c = (i + 1) * pointsPerRow + j + 1;
            const double d = (i + 1) * pointsPerRow + j;
            // faces one and two
            indices.push_back(std::uint32_t(a));
            indices.push_back(std::uint32_t(b));
            indices.push_back(std::uint32_t(d));
            indices.push_back(std::uint32_t(b));
            indices.push_back(std::uint32_t(c));
            indices.push_back(std::uint32_t(d));
        }
    }
    // generate caps
    if (capStart) generateCap(0, rowCount, columns, rows, vertices, uvs, indices);
    if (capEnd) generateCap(rowCount - 1, rowCount, columns, rows, vertices, uvs, indices);
    // build geometry
    setIndex(indices);
    setAttribute("position", std::make_shared<Float32BufferAttribute>(vertices, 3));
    setAttribute("uv", std::make_shared<Float32BufferAttribute>(uvs, 2));
    computeVertexNormals();
    // the seam vertices of closed sections are duplicated so their computed
    // normals must be averaged to achieve smooth shading across the seam
    if (closed) {
        const auto normals = getAttribute("normal");
        for (double i = 0; i < rowCount; i += 1) {
            const double a = i * pointsPerRow;
            const double b = i * pointsPerRow + (pointsPerRow - 1);
            temporary.set(normals->getX(a) + normals->getX(b), normals->getY(a) + normals->getY(b),
                normals->getZ(a) + normals->getZ(b)).normalize();
            normals->setXYZ(a, temporary.x, temporary.y, temporary.z);
            normals->setXYZ(b, temporary.x, temporary.y, temporary.z);
        }
    }
}

void LoftGeometry::generateCap(double sectionIndex, double rows, double columns, const std::vector<Section>& sections,
                               std::vector<double>& vertices, std::vector<double>& uvs,
                               std::vector<std::uint32_t>& indices) {
    const auto& section = sections[std::size_t(sectionIndex)];
    // compute the centroid of the section and the normal of its plane
    // via Newell's method
    Vector3 centroid, normal;
    for (double i = 0; i < columns; i += 1) {
        const auto& p = section[std::size_t(i)];
        const auto& q = section[std::size_t(std::fmod(i + 1, columns))];
        centroid.add(p);
        normal.x += (p.y - q.y) * (p.z + q.z);
        normal.y += (p.z - q.z) * (p.x + q.x);
        normal.z += (p.x - q.x) * (p.y + q.y);
    }
    centroid.divideScalar(columns);
    normal.normalize();
    // make sure the cap faces away from the rest of the surface
    const auto& neighbor = sections[std::size_t(sectionIndex == 0 ? 1 : rows - 2)];
    temporary.set(0, 0, 0);
    for (double i = 0; i < columns; i += 1) temporary.add(neighbor[std::size_t(i)]);
    temporary.divideScalar(columns).sub(centroid);
    if (normal.dot(temporary) > 0) normal.negate();
    // project the section onto the cap plane
    Vector3 tangent(1, 0, 0);
    if (std::abs(normal.x) > 0.9) tangent.set(0, 1, 0);
    Vector3 bitangent;
    bitangent.crossVectors(normal, tangent).normalize();
    tangent.crossVectors(bitangent, normal);
    std::vector<Vector2> contour;
    std::vector<Vector3> points = section; // `section.slice()`
    for (double i = 0; i < columns; i += 1) {
        temporary.subVectors(points[std::size_t(i)], centroid);
        contour.emplace_back(temporary.dot(tangent), temporary.dot(bitangent));
    }
    // triangulateShape() expects contours in counterclockwise order
    if (ShapeUtils::isClockWise(contour)) {
        std::reverse(contour.begin(), contour.end());
        std::reverse(points.begin(), points.end());
    }
    std::vector<std::vector<Vector2>> holes;
    const auto faces = ShapeUtils::triangulateShape(contour, holes);
    // compute the bounding box of the contour for uv generation
    Vector2 min(std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
    Vector2 max(-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity());
    for (double i = 0; i < columns; i += 1) {
        // triangulateShape() removes a duplicated end point in place, so a contour whose
        // last point repeats the first one is shorter here; the pinned module then reads
        // `undefined` and throws from Vector2.min.
        if (std::size_t(i) >= contour.size())
            throw JSTypeError("Cannot read properties of undefined (reading 'x')");
        min.min(contour[std::size_t(i)]);
        max.max(contour[std::size_t(i)]);
    }
    const double width = std::max(max.x - min.x, EPSILON);
    const double height = std::max(max.y - min.y, EPSILON);
    // generate vertices, uvs and indices; cap vertices are not shared
    // with the wall so the cap is flat shaded with a hard edge
    const double indexOffset = double(vertices.size()) / 3;
    for (double i = 0; i < columns; i += 1) {
        const auto& point = points[std::size_t(i)];
        vertices.push_back(point.x);
        vertices.push_back(point.y);
        vertices.push_back(point.z);
        uvs.push_back((contour[std::size_t(i)].x - min.x) / width);
        uvs.push_back((contour[std::size_t(i)].y - min.y) / height);
    }
    for (std::size_t i = 0; i < faces.size(); ++i) {
        const auto& face = faces[i];
        indices.push_back(std::uint32_t(indexOffset + double(face[0])));
        indices.push_back(std::uint32_t(indexOffset + double(face[1])));
        indices.push_back(std::uint32_t(indexOffset + double(face[2])));
    }
}

BufferGeometry& LoftGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    const auto* loft = dynamic_cast<const LoftGeometry*>(&source);
    if (!loft) throw JSTypeError("LoftGeometry: source is not a LoftGeometry");
    parameters = loft->parameters;
    return *this;
}

std::shared_ptr<BufferGeometry> LoftGeometry::clone() const {
    // `new this.constructor().copy( this )`: the new instance starts from the defaults and
    // copy() overwrites the parameters, like the pinned Object.assign.
    auto result = std::make_shared<LoftGeometry>();
    result->copy(*this);
    return result;
}

JsonValue LoftGeometry::toJSON() const {
    auto result = BufferGeometry::toJSON();
    // The base toJSON of the pinned module copies every own key of `parameters`, with the
    // section arrays serialized through Vector3#toJSON.
    JsonValue::Array sections;
    for (const auto& section : parameters.sections) {
        JsonValue::Array points;
        for (const auto& point : section) points.push_back(JsonValue::Array{point.x, point.y, point.z});
        sections.push_back(std::move(points));
    }
    result["sections"] = std::move(sections);
    result["closed"] = parameters.closed;
    result["capStart"] = parameters.capStart;
    result["capEnd"] = parameters.capEnd;
    return result;
}
