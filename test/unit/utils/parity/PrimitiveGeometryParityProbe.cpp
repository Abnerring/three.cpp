// Differential probe for the primitive geometries: one scenario per stdin line, one JSON object per line
// back. Values are emitted as float32 bit patterns so the comparison is exact and cannot drift through
// decimal printing.
#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "geometries/BoxGeometry.h"
#include "geometries/DodecahedronGeometry.h"
#include "geometries/IcosahedronGeometry.h"
#include "geometries/LatheGeometry.h"
#include "extras/core/Path.h"
#include "geometries/ShapeGeometry.h"
#include "extras/core/Shape.h"
#include "math/Vector2.h"
#include "geometries/OctahedronGeometry.h"
#include "geometries/TetrahedronGeometry.h"
#include "geometries/CircleGeometry.h"
#include "geometries/ConeGeometry.h"
#include "geometries/CylinderGeometry.h"
#include "geometries/PlaneGeometry.h"
#include "geometries/SphereGeometry.h"
#include "geometries/TubeGeometry.h"
#include "extras/curves/CatmullRomCurve3.h"
#include "math/Vector3.h"
#include "geometries/ExtrudeGeometry.h"
#include "core/JSValue.h"
#include "geometries/EdgesGeometry.h"
#include "geometries/WireframeGeometry.h"
#include "geometries/TorusGeometry.h"
#include "geometries/TorusKnotGeometry.h"
#include "geometries/RingGeometry.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::uint32_t bits(float value) {
    std::uint32_t raw = 0;
    std::memcpy(&raw, &value, sizeof(raw));
    return raw;
}

void emitAttribute(const std::shared_ptr<BufferGeometry>& geometry, const char* name, int itemSize) {
    const auto attribute = std::dynamic_pointer_cast<BufferAttribute>(geometry->getAttribute(name));
    if (!attribute) {
        std::cout << "\"" << name << "\":null";
        return;
    }
    const auto count = static_cast<int>(attribute->count);
    std::cout << "\"" << name << "\":{\"count\":" << count << ",\"itemSize\":" << itemSize << ",\"bits\":[";
    const int limit = count < 6 ? count : 6;
    for (int i = 0; i < limit; i += 1) {
        for (int c = 0; c < itemSize; c += 1) {
            if (i > 0 || c > 0) std::cout << ",";
            std::cout << bits(static_cast<float>(attribute->getComponent(i, c)));
        }
    }
    std::cout << "]}";
}
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind;
        if (!(in >> kind)) continue;
        std::shared_ptr<BufferGeometry> geometry;
        if (kind == "box") {
            double width = 0, height = 0, depth = 0, ws = 0, hs = 0, ds = 0;
            in >> width >> height >> depth >> ws >> hs >> ds;
            geometry = std::make_shared<BoxGeometry>(width, height, depth, ws, hs, ds);
        } else if (kind == "plane") {
            double width = 0, height = 0, ws = 0, hs = 0;
            in >> width >> height >> ws >> hs;
            geometry = std::make_shared<PlaneGeometry>(width, height, ws, hs);
        } else if (kind == "circle") {
            double radius = 0, segments = 0, thetaStart = 0, thetaLength = 0;
            in >> radius >> segments >> thetaStart >> thetaLength;
            geometry = std::make_shared<CircleGeometry>(radius, segments, thetaStart, thetaLength);
        } else if (kind == "ring") {
            double inner = 0, outer = 0, thetaSegments = 0, phiSegments = 0, thetaStart = 0, thetaLength = 0;
            in >> inner >> outer >> thetaSegments >> phiSegments >> thetaStart >> thetaLength;
            geometry = std::make_shared<RingGeometry>(inner, outer, thetaSegments, phiSegments, thetaStart, thetaLength);
        } else if (kind == "cone") {
            double radius = 0, height = 0, radialSegments = 0, heightSegments = 0, openEnded = 0, thetaStart = 0, thetaLength = 0;
            in >> radius >> height >> radialSegments >> heightSegments >> openEnded >> thetaStart >> thetaLength;
            geometry = std::make_shared<ConeGeometry>(radius, height, radialSegments, heightSegments, openEnded != 0.0, thetaStart, thetaLength);
        } else if (kind == "cylinder") {
            double radiusTop = 0, radiusBottom = 0, height = 0, radialSegments = 0, heightSegments = 0, openEnded = 0, thetaStart = 0, thetaLength = 0;
            in >> radiusTop >> radiusBottom >> height >> radialSegments >> heightSegments >> openEnded >> thetaStart >> thetaLength;
            geometry = std::make_shared<CylinderGeometry>(radiusTop, radiusBottom, height, radialSegments, heightSegments, openEnded != 0.0, thetaStart, thetaLength);
        } else if (kind == "sphere") {
            double radius = 0, widthSegments = 0, heightSegments = 0, phiStart = 0, phiLength = 0, thetaStart = 0, thetaLength = 0;
            in >> radius >> widthSegments >> heightSegments >> phiStart >> phiLength >> thetaStart >> thetaLength;
            geometry = std::make_shared<SphereGeometry>(radius, widthSegments, heightSegments, phiStart, phiLength, thetaStart, thetaLength);
        } else if (kind == "torus") {
            double radius = 0, tube = 0, radialSegments = 0, tubularSegments = 0, arc = 0;
            in >> radius >> tube >> radialSegments >> tubularSegments >> arc;
            geometry = std::make_shared<TorusGeometry>(radius, tube, radialSegments, tubularSegments, arc);
        } else if (kind == "torusknot") {
            double radius = 0, tube = 0, tubularSegments = 0, radialSegments = 0, p = 0, q = 0;
            in >> radius >> tube >> tubularSegments >> radialSegments >> p >> q;
            geometry = std::make_shared<TorusKnotGeometry>(radius, tube, tubularSegments, radialSegments, p, q);
        } else if (kind == "polyhedron") {
            std::string which;
            double radius = 0, detail = 0;
            in >> which >> radius >> detail;
            if (which == "dodecahedron") geometry = std::make_shared<DodecahedronGeometry>(radius, detail);
            else if (which == "icosahedron") geometry = std::make_shared<IcosahedronGeometry>(radius, detail);
            else if (which == "octahedron") geometry = std::make_shared<OctahedronGeometry>(radius, detail);
            else if (which == "tetrahedron") geometry = std::make_shared<TetrahedronGeometry>(radius, detail);
            else {
                std::cout << "{\"error\":\"unknown polyhedron\"}" << std::endl;
                continue;
            }
        } else if (kind == "lathe") {
            double segments = 0, phiStart = 0, phiLength = 0, count = 0;
            in >> segments >> phiStart >> phiLength >> count;
            Path::PointsPtr points = std::make_shared<Path::Points>();
            for (int i = 0; i < static_cast<int>(count); i += 1) {
                double x = 0, y = 0;
                in >> x >> y;
                points->push_back(std::make_shared<Vector2>(x, y));
            }
            geometry = std::make_shared<LatheGeometry>(points, segments, phiStart, phiLength);
        } else if (kind == "shape") {
            double curveSegments = 0, count = 0;
            in >> curveSegments >> count;
            auto shape = std::make_shared<Shape>();
            for (int i = 0; i < static_cast<int>(count); i += 1) {
                double x = 0, y = 0;
                in >> x >> y;
                if (i == 0) shape->moveTo(x, y);
                else shape->lineTo(x, y);
            }
            ShapeGeometry::Shapes shapes{std::shared_ptr<Shape>(shape)};
            geometry = std::make_shared<ShapeGeometry>(shapes, curveSegments);
        } else if (kind == "edges") {
            double width = 0, height = 0, depth = 0, threshold = 0;
            in >> width >> height >> depth >> threshold;
            auto source = std::make_shared<BoxGeometry>(width, height, depth, 1, 1, 1);
            geometry = std::make_shared<EdgesGeometry>(source, threshold);
        } else if (kind == "wireframe") {
            double width = 0, height = 0, depth = 0;
            in >> width >> height >> depth;
            auto source = std::make_shared<BoxGeometry>(width, height, depth, 1, 1, 1);
            geometry = std::make_shared<WireframeGeometry>(source);
        } else if (kind == "extrude") {
            double depth = 0, bevel = 0, steps = 0, curveSegments = 0, count = 0;
            in >> depth >> bevel >> steps >> curveSegments >> count;
            auto shape = std::make_shared<Shape>();
            for (int i = 0; i < static_cast<int>(count); i += 1) {
                double x = 0, y = 0;
                in >> x >> y;
                if (i == 0) shape->moveTo(x, y);
                else shape->lineTo(x, y);
            }
            auto options = std::make_shared<ExtrudeGeometry::Options>();
            options->set("depth", JSValue(depth));
            options->set("bevelEnabled", JSValue(bevel != 0.0));
            options->set("steps", JSValue(steps));
            options->set("curveSegments", JSValue(curveSegments));
            ExtrudeGeometry::Shapes shapes{std::shared_ptr<Shape>(shape)};
            geometry = std::make_shared<ExtrudeGeometry>(shapes, options);
        } else if (kind == "tube") {
            double tubularSegments = 0, radius = 0, radialSegments = 0, closed = 0, count = 0;
            in >> tubularSegments >> radius >> radialSegments >> closed >> count;
            auto points = std::make_shared<CatmullRomCurve3::Points>();
            for (int i = 0; i < static_cast<int>(count); i += 1) {
                double x = 0, y = 0, z = 0;
                in >> x >> y >> z;
                points->push_back(std::make_shared<Vector3>(x, y, z));
            }
            auto path = std::make_shared<CatmullRomCurve3>(points, closed != 0.0);
            geometry = std::make_shared<TubeGeometry>(path, tubularSegments, radius, radialSegments, closed != 0.0);
        } else if (kind == "frames") {
            double segments = 0, closed = 0, count = 0;
            in >> segments >> closed >> count;
            auto points = std::make_shared<CatmullRomCurve3::Points>();
            for (int i = 0; i < static_cast<int>(count); i += 1) {
                double x = 0, y = 0, z = 0;
                in >> x >> y >> z;
                points->push_back(std::make_shared<Vector3>(x, y, z));
            }
            CatmullRomCurve3 curve(points, closed != 0.0);
            const auto frames = curve.computeFrenetFrames(segments, closed != 0.0);
            std::cout << "{\"kind\":\"frames\",\"normals\":[";
            for (std::size_t i = 0; i < frames.normals.size() && i < 3; i += 1) {
                const auto& n = frames.normals[i];
                if (i > 0) std::cout << ",";
                std::cout << "[" << bits(static_cast<float>(n.x)) << "," << bits(static_cast<float>(n.y)) << "," << bits(static_cast<float>(n.z)) << "]";
            }
            std::cout << "],\"binormals\":[";
            for (std::size_t i = 0; i < frames.binormals.size() && i < 3; i += 1) {
                const auto& b = frames.binormals[i];
                if (i > 0) std::cout << ",";
                std::cout << "[" << bits(static_cast<float>(b.x)) << "," << bits(static_cast<float>(b.y)) << "," << bits(static_cast<float>(b.z)) << "]";
            }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "curve") {
            double closed = 0, count = 0;
            in >> closed >> count;
            auto points = std::make_shared<CatmullRomCurve3::Points>();
            for (int i = 0; i < static_cast<int>(count); i += 1) {
                double x = 0, y = 0, z = 0;
                in >> x >> y >> z;
                points->push_back(std::make_shared<Vector3>(x, y, z));
            }
            CatmullRomCurve3 curve(points, closed != 0.0);
            std::cout << "{\"kind\":\"curve\",\"points\":[";
            for (int i = 0; i <= 4; i += 1) {
                const double t = i / 4.0;
                const auto point = curve.getPoint(t);
                if (i > 0) std::cout << ",";
                std::cout << "[" << bits(static_cast<float>(point.x)) << "," << bits(static_cast<float>(point.y)) << "," << bits(static_cast<float>(point.z)) << "]";
            }
            std::cout << "],\"tangents\":[";
            for (int i = 0; i <= 4; i += 1) {
                const double t = i / 4.0;
                const auto tangent = curve.getTangent(t);
                if (i > 0) std::cout << ",";
                std::cout << "[" << bits(static_cast<float>(tangent.x)) << "," << bits(static_cast<float>(tangent.y)) << "," << bits(static_cast<float>(tangent.z)) << "]";
            }
            std::cout << "],\"lengths\":[";
            const auto lengths = curve.getLengths();
            for (std::size_t j = 0; lengths && j < lengths->size(); j += 1) {
                if (j > 0) std::cout << ",";
                std::cout << bits(static_cast<float>((*lengths)[j]));
            }
            std::cout << "],\"utoT\":[";
            for (int i = 0; i <= 4; i += 1) {
                if (i > 0) std::cout << ",";
                std::cout << bits(static_cast<float>(curve.getUtoTmapping(i / 4.0)));
            }
            std::cout << "],\"pointAt\":[";
            for (int i = 0; i <= 4; i += 1) {
                const auto pointAt = curve.getPointAt(i / 4.0);
                if (i > 0) std::cout << ",";
                std::cout << "[" << bits(static_cast<float>(pointAt.x)) << "," << bits(static_cast<float>(pointAt.y)) << "," << bits(static_cast<float>(pointAt.z)) << "]";
            }
            std::cout << "]}" << std::endl;
            continue;
        } else {
            std::cout << "{\"error\":\"unknown kind\"}" << std::endl;
            continue;
        }
        std::cout << "{\"kind\":\"" << kind << "\",";
        emitAttribute(geometry, "position", 3);
        std::cout << ",";
        emitAttribute(geometry, "normal", 3);
        std::cout << ",";
        emitAttribute(geometry, "uv", 2);
        std::cout << ",";
        const auto index = geometry->getIndex();
        if (index) {
            std::cout << "\"index\":{\"count\":" << static_cast<int>(index->count) << ",\"first\":[";
            const int indexCount = static_cast<int>(index->count);
            const int limit = indexCount < 8 ? indexCount : 8;
            for (int i = 0; i < limit; i += 1) {
                if (i > 0) std::cout << ",";
                std::cout << static_cast<int>(index->getX(i));
            }
            std::cout << "]}";
        } else {
            std::cout << "\"index\":null";
        }
        std::cout << ",\"groups\":" << static_cast<int>(geometry->groups.size());
        std::cout << "}" << std::endl;
    }
    return 0;
}
