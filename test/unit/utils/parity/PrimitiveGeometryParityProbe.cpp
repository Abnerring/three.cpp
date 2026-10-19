// Differential probe for the primitive geometries: one scenario per stdin line, one JSON object per line
// back. Values are emitted as float32 bit patterns so the comparison is exact and cannot drift through
// decimal printing.
#include "core/BufferAttribute.h"
#include "core/Raycaster.h"
#include "core/BufferGeometry.h"
#include "geometries/BoxGeometry.h"
#include "geometries/DodecahedronGeometry.h"
#include "geometries/IcosahedronGeometry.h"
#include "geometries/LatheGeometry.h"
#include "extras/core/Path.h"
#include "extras/core/CurvePath.h"
#include "extras/core/ShapePath.h"
#include "extras/curves/LineCurve.h"
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
#include "extras/curves/LineCurve3.h"
#include "extras/curves/EllipseCurve.h"
#include "extras/curves/ArcCurve.h"
#include "extras/curves/LineCurve.h"
#include "extras/curves/QuadraticBezierCurve.h"
#include "extras/curves/CubicBezierCurve.h"
#include "extras/curves/SplineCurve.h"
#include "math/interpolants/LinearInterpolant.h"
#include "math/interpolants/DiscreteInterpolant.h"
#include "math/interpolants/CubicInterpolant.h"
#include "math/interpolants/QuaternionLinearInterpolant.h"
#include "animation/tracks/NumberKeyframeTrack.h"
#include "animation/tracks/ColorKeyframeTrack.h"
#include "animation/tracks/VectorKeyframeTrack.h"
#include "animation/tracks/QuaternionKeyframeTrack.h"
#include "animation/tracks/BooleanKeyframeTrack.h"
#include "animation/tracks/StringKeyframeTrack.h"
#include "lights/AmbientLight.h"
#include "lights/DirectionalLight.h"
#include "lights/PointLight.h"
#include "lights/SpotLight.h"
#include "lights/HemisphereLight.h"
#include "lights/LightProbe.h"
#include "lights/RectAreaLight.h"
#include "materials/LineBasicMaterial.h"
#include "materials/LineDashedMaterial.h"
#include "materials/PointsMaterial.h"
#include "materials/SpriteMaterial.h"
#include "materials/ShadowMaterial.h"
#include "materials/MeshDepthMaterial.h"
#include "materials/MeshDistanceMaterial.h"
#include "materials/MeshNormalMaterial.h"
#include "materials/MeshLambertMaterial.h"
#include "materials/MeshPhongMaterial.h"
#include "materials/MeshToonMaterial.h"
#include "materials/MeshMatcapMaterial.h"
#include "materials/MeshPhysicalMaterial.h"
#include "materials/ShaderMaterial.h"
#include "materials/RawShaderMaterial.h"
#include "helpers/AxesHelper.h"
#include "helpers/Box3Helper.h"
#include "helpers/PlaneHelper.h"
#include "helpers/PolarGridHelper.h"
#include "helpers/HemisphereLightHelper.h"
#include "helpers/SpotLightHelper.h"
#include "helpers/CameraHelper.h"
#include "helpers/SkeletonHelper.h"
#include "helpers/BoxHelper.h"
#include "math/Plane.h"
#include "math/Box3.h"
#include "cameras/PerspectiveCamera.h"
#include "cameras/ArrayCamera.h"
#include "cameras/StereoCamera.h"
#include "textures/Texture.h"
#include "scenes/Fog.h"
#include "scenes/FogExp2.h"
#include "math/SphericalHarmonics3.h"
#include "textures/CubeTexture.h"
#include "textures/DataArrayTexture.h"
#include "textures/Data3DTexture.h"
#include "textures/CompressedTexture.h"
#include "textures/CompressedArrayTexture.h"
#include "textures/DataTexture.h"
#include "textures/DepthTexture.h"
#include "cameras/OrthographicCamera.h"
#include "core/RenderTarget.h"
#include "core/RenderTarget3D.h"
#include "objects/Group.h"
#include "objects/Mesh.h"
#include "objects/InstancedMesh.h"
#include "materials/MeshBasicMaterial.h"
#include "geometries/BoxGeometry.h"
#include "objects/Points.h"
#include "objects/Sprite.h"
#include "objects/LOD.h"
#include "scenes/Scene.h"
#include "helpers/GridHelper.h"
#include "helpers/ArrowHelper.h"
#include "helpers/PointLightHelper.h"
#include "helpers/DirectionalLightHelper.h"
#include "objects/Line.h"
#include "extras/curves/QuadraticBezierCurve3.h"
#include "extras/curves/CubicBezierCurve3.h"
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
// A counted protocol can read too few values if the count is smaller than the data; that error would look like a
// divergence in the code under test, so any leftover token on the line is refused.
bool noTrailingTokens(std::istringstream& in) {
    in >> std::ws;
    return in.eof();
}

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
        // A short line leaves numbers unread, which would build a different object than intended and
        // look like a port bug. Refuse it instead.
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
        } else if (kind == "line3") {
            double ax = 0, ay = 0, az = 0, bx = 0, by = 0, bz = 0;
            in >> ax >> ay >> az >> bx >> by >> bz;
            LineCurve3 curve(std::make_shared<Vector3>(ax, ay, az), std::make_shared<Vector3>(bx, by, bz));
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            std::cout << "{\"kind\":\"line3\",\"points\":[";
            for (int i = 0; i <= 16; i += 1) {
                const auto point = curve.getPoint(i / 16.0);
                if (i > 0) std::cout << ",";
                std::cout << "[" << bits(static_cast<float>(point.x)) << "," << bits(static_cast<float>(point.y)) << "," << bits(static_cast<float>(point.z)) << "]";
            }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "quad3") {
            double ax = 0, ay = 0, az = 0, bx = 0, by = 0, bz = 0, cx = 0, cy = 0, cz = 0;
            in >> ax >> ay >> az >> bx >> by >> bz >> cx >> cy >> cz;
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            QuadraticBezierCurve3 curve(std::make_shared<Vector3>(ax, ay, az), std::make_shared<Vector3>(bx, by, bz), std::make_shared<Vector3>(cx, cy, cz));
            std::cout << "{\"kind\":\"quad3\",\"points\":[";
            for (int i = 0; i <= 16; i += 1) {
                const auto point = curve.getPoint(i / 16.0);
                if (i > 0) std::cout << ",";
                std::cout << "[" << bits(static_cast<float>(point.x)) << "," << bits(static_cast<float>(point.y)) << "," << bits(static_cast<float>(point.z)) << "]";
            }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "cubic3") {
            double p[12] = {};
            for (int k = 0; k < 12; k += 1) in >> p[k];
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            CubicBezierCurve3 curve(std::make_shared<Vector3>(p[0], p[1], p[2]), std::make_shared<Vector3>(p[3], p[4], p[5]),
                                    std::make_shared<Vector3>(p[6], p[7], p[8]), std::make_shared<Vector3>(p[9], p[10], p[11]));
            std::cout << "{\"kind\":\"cubic3\",\"points\":[";
            for (int i = 0; i <= 16; i += 1) {
                const auto point = curve.getPoint(i / 16.0);
                if (i > 0) std::cout << ",";
                std::cout << "[" << bits(static_cast<float>(point.x)) << "," << bits(static_cast<float>(point.y)) << "," << bits(static_cast<float>(point.z)) << "]";
            }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "ellipse") {
            double ax = 0, ay = 0, rx = 0, ry = 0, start = 0, end = 0, clockwise = 0, rotation = 0;
            in >> ax >> ay >> rx >> ry >> start >> end >> clockwise >> rotation;
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            EllipseCurve curve(ax, ay, rx, ry, start, end, clockwise != 0.0, rotation);
            std::cout << "{\"kind\":\"ellipse\",\"points\":[";
            for (int i = 0; i <= 4; i += 1) {
                const auto point = curve.getPoint(i / 4.0);
                if (i > 0) std::cout << ",";
                std::cout << "[" << bits(static_cast<float>(point.x)) << "," << bits(static_cast<float>(point.y)) << "]";
            }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "arc") {
            double ax = 0, ay = 0, radius = 0, start = 0, end = 0, clockwise = 0;
            in >> ax >> ay >> radius >> start >> end >> clockwise;
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            ArcCurve curve(ax, ay, radius, start, end, clockwise != 0.0);
            std::cout << "{\"kind\":\"arc\",\"points\":[";
            for (int i = 0; i <= 4; i += 1) {
                const auto point = curve.getPoint(i / 4.0);
                if (i > 0) std::cout << ",";
                std::cout << "[" << bits(static_cast<float>(point.x)) << "," << bits(static_cast<float>(point.y)) << "]";
            }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "line2") {
            double ax = 0, ay = 0, bx = 0, by = 0;
            in >> ax >> ay >> bx >> by;
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            LineCurve curve(std::make_shared<Vector2>(ax, ay), std::make_shared<Vector2>(bx, by));
            std::cout << "{\"kind\":\"line2\",\"points\":[";
            for (int i = 0; i <= 4; i += 1) {
                const auto point = curve.getPoint(i / 4.0);
                if (i > 0) std::cout << ",";
                std::cout << "[" << bits(static_cast<float>(point.x)) << "," << bits(static_cast<float>(point.y)) << "]";
            }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "quad2") {
            double v[6] = {};
            for (int k = 0; k < 6; k += 1) in >> v[k];
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            QuadraticBezierCurve curve(std::make_shared<Vector2>(v[0], v[1]), std::make_shared<Vector2>(v[2], v[3]), std::make_shared<Vector2>(v[4], v[5]));
            std::cout << "{\"kind\":\"quad2\",\"points\":[";
            for (int i = 0; i <= 4; i += 1) { const auto p = curve.getPoint(i / 4.0); if (i > 0) std::cout << ","; std::cout << "[" << bits(static_cast<float>(p.x)) << "," << bits(static_cast<float>(p.y)) << "]"; }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "cubic2") {
            double v[8] = {};
            for (int k = 0; k < 8; k += 1) in >> v[k];
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            CubicBezierCurve curve(std::make_shared<Vector2>(v[0], v[1]), std::make_shared<Vector2>(v[2], v[3]), std::make_shared<Vector2>(v[4], v[5]), std::make_shared<Vector2>(v[6], v[7]));
            std::cout << "{\"kind\":\"cubic2\",\"points\":[";
            for (int i = 0; i <= 4; i += 1) { const auto p = curve.getPoint(i / 4.0); if (i > 0) std::cout << ","; std::cout << "[" << bits(static_cast<float>(p.x)) << "," << bits(static_cast<float>(p.y)) << "]"; }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "spline2") {
            double count = 0;
            in >> count;
            auto points = std::make_shared<SplineCurve::Points>();
            for (int k = 0; k < static_cast<int>(count); k += 1) { double x = 0, y = 0; in >> x >> y; points->push_back(std::make_shared<Vector2>(x, y)); }
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            SplineCurve curve(points);
            std::cout << "{\"kind\":\"spline2\",\"points\":[";
            for (int i = 0; i <= 4; i += 1) { const auto p = curve.getPoint(i / 4.0); if (i > 0) std::cout << ","; std::cout << "[" << bits(static_cast<float>(p.x)) << "," << bits(static_cast<float>(p.y)) << "]"; }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "interp") {
            std::string which;
            double sampleSize = 0, count = 0;
            in >> which >> sampleSize >> count;
            std::vector<double> positions;
            for (int k = 0; k < static_cast<int>(count); k += 1) { double value = 0; in >> value; positions.push_back(value); }
            double sampleCount = 0;
            in >> sampleCount;
            std::vector<double> samples;
            for (int k = 0; k < static_cast<int>(sampleCount); k += 1) { double value = 0; in >> value; samples.push_back(value); }
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            std::shared_ptr<Interpolant> interpolant;
            if (which == "linear") interpolant = std::make_shared<LinearInterpolant>(InterpolantArray(positions), InterpolantArray(samples), sampleSize);
            else if (which == "discrete") interpolant = std::make_shared<DiscreteInterpolant>(InterpolantArray(positions), InterpolantArray(samples), sampleSize);
            else if (which == "cubic") interpolant = std::make_shared<CubicInterpolant>(InterpolantArray(positions), InterpolantArray(samples), sampleSize);
            else if (which == "quaternionLinear") interpolant = std::make_shared<QuaternionLinearInterpolant>(InterpolantArray(positions), InterpolantArray(samples), sampleSize);
            else { std::cout << "{\"error\":\"unknown interpolant\"}" << std::endl; continue; }
            std::cout << "{\"kind\":\"interp\",\"which\":\"" << which << "\",\"results\":[";
            for (int i = 0; i <= 4; i += 1) {
                const auto result = interpolant->evaluate(i / 4.0);
                if (i > 0) std::cout << ",";
                std::cout << "[";
                for (int c = 0; c < static_cast<int>(sampleSize); c += 1) { if (c > 0) std::cout << ","; std::cout << bits(static_cast<float>(result.number(c))); }
                std::cout << "]";
            }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "text") {
            double count = 0;
            in >> count;
            std::vector<double> times;
            for (int k = 0; k < static_cast<int>(count); k += 1) { double value = 0; in >> value; times.push_back(value); }
            std::vector<std::string> words;
            std::string word;
            while (in >> word) words.push_back(word);
            if (words.empty()) { std::cout << "{\"error\":\"short input\"}" << std::endl; continue; }
            StringKeyframeTrack track(JSValue(".s"), InterpolantArray(times), InterpolantArray(words));
            std::cout << "{\"kind\":\"text\",\"valueSize\":" << track.getValueSize() << ",\"results\":[";
            const auto interpolant = track.createInterpolant();
            for (int i = 0; i <= 4; i += 1) {
                interpolant->evaluate(i / 4.0);
                if (i > 0) std::cout << ",";
                std::cout << "\"" << interpolant->resultBuffer.get(0).toString() << "\"";
            }
            std::cout << "]}" << std::endl;
            continue;        } else if (kind == "track") {
            std::string which;
            double count = 0;
            in >> which >> count;
            std::vector<double> times;
            for (int k = 0; k < static_cast<int>(count); k += 1) { double value = 0; in >> value; times.push_back(value); }
            double valueCount = 0;
            in >> valueCount;
            std::vector<double> values;
            for (int k = 0; k < static_cast<int>(valueCount); k += 1) { double value = 0; in >> value; values.push_back(value); }
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            std::shared_ptr<KeyframeTrack> track;
            if (which == "number") track = std::make_shared<NumberKeyframeTrack>(JSValue(".v"), InterpolantArray(times), InterpolantArray(values));
            else if (which == "color") track = std::make_shared<ColorKeyframeTrack>(JSValue(".c"), InterpolantArray(times), InterpolantArray(values));
            else if (which == "vector") track = std::make_shared<VectorKeyframeTrack>(JSValue(".p"), InterpolantArray(times), InterpolantArray(values));
            else if (which == "quaternion") track = std::make_shared<QuaternionKeyframeTrack>(JSValue(".q"), InterpolantArray(times), InterpolantArray(values));
            else if (which == "boolean") track = std::make_shared<BooleanKeyframeTrack>(JSValue(".b"), InterpolantArray(times), InterpolantArray(values));
            else { std::cout << "{\"error\":\"unknown track\"}" << std::endl; continue; }
            std::cout << "{\"kind\":\"track\",\"which\":\"" << which << "\",\"valueSize\":" << track->getValueSize();
            std::cout << ",\"results\":[";
            const auto interpolant = track->createInterpolant();
            for (int i = 0; i <= 4; i += 1) {
                const auto result = interpolant->evaluate(i / 4.0);
                if (i > 0) std::cout << ",";
                std::cout << "[";
                for (int c = 0; c < static_cast<int>(interpolant->resultBuffer.size()); c += 1) { if (c > 0) std::cout << ","; std::cout << bits(static_cast<float>(interpolant->resultBuffer.number(c))); }
                std::cout << "]";
            }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "light") {
            std::string which;
            in >> which;
            std::shared_ptr<Light> light;
            std::cout << "{\"kind\":\"light\",\"which\":\"" << which << "\"";
            double r = 0, g = 0, b = 0, intensity = 0;
            if (which == "ambient" || which == "directional" || which == "point" || which == "spot" || which == "rectarea") {
                in >> r >> g >> b >> intensity;
                if (in.fail()) { std::cout << "{\"error\":\"short input\"}" << std::endl; continue; }
                const Color color(r, g, b);
                if (which == "ambient") light = std::make_shared<AmbientLight>(color, intensity);
                else if (which == "directional") light = std::make_shared<DirectionalLight>(color, intensity);
                else if (which == "point") { double distance = 0, decay = 0; in >> distance >> decay; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; } light = std::make_shared<PointLight>(color, intensity, distance, decay); }
                else if (which == "spot") { double distance = 0, angle = 0, penumbra = 0, decay = 0; in >> distance >> angle >> penumbra >> decay; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; } light = std::make_shared<SpotLight>(color, intensity, distance, angle, penumbra, decay); }
                else { double width = 0, height = 0; in >> width >> height; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; } light = std::make_shared<RectAreaLight>(color, intensity, width, height); }
            } else if (which == "hemisphere") {
                double r2 = 0, g2 = 0, b2 = 0;
                in >> r >> g >> b >> r2 >> g2 >> b2 >> intensity;
                if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
                light = std::make_shared<HemisphereLight>(Color(r, g, b), Color(r2, g2, b2), intensity);
            } else if (which == "probe") {
                in >> intensity;
                if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
                light = std::make_shared<LightProbe>(std::make_shared<SphericalHarmonics3>(), intensity);
            } else { std::cout << "{\"error\":\"unknown light\"}" << std::endl; continue; }
            std::cout << ",\"type\":\"" << light->type << "\"";
            if (!noTrailingTokens(in)) { std::cout << "{\"error\":\"trailing tokens\"}" << std::endl; continue; }
            std::cout << ",\"color\":[" << bits(static_cast<float>(light->color.r)) << "," << bits(static_cast<float>(light->color.g)) << "," << bits(static_cast<float>(light->color.b)) << "]";
            std::cout << ",\"intensity\":" << bits(static_cast<float>(light->intensity));
            if (auto point = std::dynamic_pointer_cast<PointLight>(light)) std::cout << ",\"distance\":" << bits(static_cast<float>(point->distance)) << ",\"decay\":" << bits(static_cast<float>(point->decay));
            if (auto spot = std::dynamic_pointer_cast<SpotLight>(light)) std::cout << ",\"distance\":" << bits(static_cast<float>(spot->distance)) << ",\"angle\":" << bits(static_cast<float>(spot->angle)) << ",\"penumbra\":" << bits(static_cast<float>(spot->penumbra)) << ",\"decay\":" << bits(static_cast<float>(spot->decay));
            if (auto area = std::dynamic_pointer_cast<RectAreaLight>(light)) std::cout << ",\"width\":" << bits(static_cast<float>(area->width)) << ",\"height\":" << bits(static_cast<float>(area->height));
            std::cout << "}" << std::endl;
            continue;
        } else if (kind == "mat") {
            std::string which;
            in >> which;
            std::shared_ptr<Material> material;
            if (which == "linebasic") material = std::make_shared<LineBasicMaterial>();
            else if (which == "linedashed") material = std::make_shared<LineDashedMaterial>();
            else if (which == "points") material = std::make_shared<PointsMaterial>();
            else if (which == "sprite") material = std::make_shared<SpriteMaterial>();
            else if (which == "shadow") material = std::make_shared<ShadowMaterial>();
            else if (which == "depth") material = std::make_shared<MeshDepthMaterial>();
            else if (which == "distance") material = std::make_shared<MeshDistanceMaterial>();
            else if (which == "normal") material = std::make_shared<MeshNormalMaterial>();
            else if (which == "lambert") material = std::make_shared<MeshLambertMaterial>();
            else if (which == "phong") material = std::make_shared<MeshPhongMaterial>();
            else if (which == "toon") material = std::make_shared<MeshToonMaterial>();
            else if (which == "matcap") material = std::make_shared<MeshMatcapMaterial>();
            else if (which == "physical") material = std::make_shared<MeshPhysicalMaterial>();
            else if (which == "shader") material = std::make_shared<ShaderMaterial>();
            else if (which == "rawshader") material = std::make_shared<RawShaderMaterial>();
            else { std::cout << "{\"error\":\"unknown material\"}" << std::endl; continue; }
            std::cout << "{\"kind\":\"mat\",\"which\":\"" << which << "\",\"type\":\"" << material->type << "\"";
            std::cout << ",\"fields\":{";
            std::cout << "\"opacity\":" << bits(static_cast<float>(material->opacity));
            std::cout << ",\"transparent\":" << (material->transparent ? "true" : "false");
            std::cout << ",\"visible\":" << (material->visible ? "true" : "false");
            std::cout << ",\"side\":" << material->side;
            std::cout << ",\"blending\":" << material->blending;
            std::cout << ",\"depthTest\":" << (material->depthTest ? "true" : "false");
            std::cout << ",\"depthWrite\":" << (material->depthWrite ? "true" : "false");
            std::cout << ",\"alphaTest\":" << bits(static_cast<float>(material->alphaTest));
            std::cout << ",\"premultipliedAlpha\":" << (material->premultipliedAlpha ? "true" : "false");
            std::cout << ",\"dithering\":" << (material->dithering ? "true" : "false");
            std::cout << ",\"toneMapped\":" << (material->toneMapped ? "true" : "false");
            std::cout << ",\"vertexColors\":" << (material->vertexColors ? "true" : "false");
            std::cout << "}}" << std::endl;
            continue;
        } else if (kind == "helper") {
            std::string which;
            in >> which;
            std::shared_ptr<Object3D> helper;
            if (which == "axes") { double size = 0; in >> size; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; } helper = std::make_shared<AxesHelper>(size); }
            else if (which == "grid") { double size = 0, divisions = 0; in >> size >> divisions; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; } helper = std::make_shared<GridHelper>(size, divisions); }
            else if (which == "arrow") { double dx = 0, dy = 0, dz = 0, ox = 0, oy = 0, oz = 0, length = 0; in >> dx >> dy >> dz >> ox >> oy >> oz >> length; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; } helper = std::make_shared<ArrowHelper>(Vector3(dx, dy, dz), Vector3(ox, oy, oz), length); }
            else if (which == "pointlight") { double sphereSize = 0; in >> sphereSize; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; } helper = std::make_shared<PointLightHelper>(std::make_shared<PointLight>(), sphereSize); }
            else if (which == "directional") { double size = 0; in >> size; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; } helper = std::make_shared<DirectionalLightHelper>(std::make_shared<DirectionalLight>(), size); }
            else if (which == "hemisphere") { double size = 0; in >> size; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; } helper = std::make_shared<HemisphereLightHelper>(std::make_shared<HemisphereLight>(), size); }
            else if (which == "spotlight") { if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"trailing tokens\"}" << std::endl; continue; } helper = std::make_shared<SpotLightHelper>(std::make_shared<SpotLight>()); }
            else if (which == "camerahelper") { if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"trailing tokens\"}" << std::endl; continue; } helper = std::make_shared<CameraHelper>(std::make_shared<PerspectiveCamera>()); }
            else if (which == "skeletonhelper") { if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"trailing tokens\"}" << std::endl; continue; } helper = std::make_shared<SkeletonHelper>(std::make_shared<Object3D>()); }
            else if (which == "boxhelper") { if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"trailing tokens\"}" << std::endl; continue; }
                auto boxed = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(1, 1, 1, 1, 1, 1), std::make_shared<MeshBasicMaterial>());
                helper = std::make_shared<BoxHelper>(boxed); }
            else if (which == "box3") { double minx = 0, miny = 0, minz = 0, maxx = 0, maxy = 0, maxz = 0; in >> minx >> miny >> minz >> maxx >> maxy >> maxz; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
                helper = std::make_shared<Box3Helper>(std::make_shared<Box3>(Vector3(minx, miny, minz), Vector3(maxx, maxy, maxz))); }
            else if (which == "planehelper") { double nx = 0, ny = 0, nz = 0, constant = 0, size = 0; in >> nx >> ny >> nz >> constant >> size; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
                helper = std::make_shared<PlaneHelper>(std::make_shared<Plane>(Vector3(nx, ny, nz), constant), size); }
            else if (which == "polargrid") { double radius = 0, sectors = 0, rings = 0, divisions = 0; in >> radius >> sectors >> rings >> divisions; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
                helper = std::make_shared<PolarGridHelper>(radius, sectors, rings, divisions); }
            else { std::cout << "{\"error\":\"unknown helper\"}" << std::endl; continue; }
            std::cout << "{\"kind\":\"helper\",\"which\":\"" << which << "\",\"type\":\"" << helper->type << "\"";
            if (auto self = std::dynamic_pointer_cast<Line>(helper)) {
                if (self->geometry) {
                    const auto position = std::dynamic_pointer_cast<BufferAttribute>(self->geometry->getAttribute("position"));
                    std::cout << ",\"selfCount\":" << static_cast<int>(position ? position->count : 0);
                    std::cout << ",\"selfPoints\":[";
                    for (int i = 0; position && i < 6 && i < static_cast<int>(position->count) * 3; i += 1) { if (i > 0) std::cout << ","; std::cout << bits(static_cast<float>(position->getComponent(i / 3, i % 3))); }
                    std::cout << "]";
                }
            }
            std::cout << ",\"children\":[";
            for (std::size_t c = 0; c < helper->children.size(); c += 1) {
                if (c > 0) std::cout << ",";
                std::cout << "{\"type\":\"" << helper->children[c]->type << "\"";
                if (auto line = std::dynamic_pointer_cast<Line>(helper->children[c])) {
                    if (line->geometry) {
                        const auto position = std::dynamic_pointer_cast<BufferAttribute>(line->geometry->getAttribute("position"));
                        std::cout << ",\"count\":" << static_cast<int>(position ? position->count : 0);
                        std::cout << ",\"points\":[";
                        for (int i = 0; position && i < 6 && i < static_cast<int>(position->count) * 3; i += 1) { if (i > 0) std::cout << ","; std::cout << bits(static_cast<float>(position->getComponent(i / 3, i % 3))); }
                        std::cout << "]";
                    }
                }
                std::cout << "}";
            }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "node") {
            std::string which;
            in >> which;
            std::shared_ptr<Object3D> node;
            std::shared_ptr<RenderTarget> target;
            if (which == "perspective") { double fov = 0, aspect = 0, near = 0, far = 0; in >> fov >> aspect >> near >> far; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; } node = std::make_shared<PerspectiveCamera>(fov, aspect, near, far); }
            else if (which == "ortho") { double left = 0, right = 0, top = 0, bottom = 0, near = 0, far = 0; in >> left >> right >> top >> bottom >> near >> far; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; } node = std::make_shared<OrthographicCamera>(left, right, top, bottom, near, far); }
            else if (which == "scene") node = std::make_shared<Scene>();
            else if (which == "group") node = std::make_shared<Group>();
            else if (which == "mesh") node = std::make_shared<Mesh>();
            else if (which == "points") node = std::make_shared<Points>();
            else if (which == "sprite") node = std::make_shared<Sprite>();
            else if (which == "array") { double count = 0; in >> count; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
                SharedArray<std::shared_ptr<Camera>> cameras;
                for (int k = 0; k < static_cast<int>(count); k += 1) cameras.push_back(std::make_shared<PerspectiveCamera>());
                node = std::make_shared<ArrayCamera>(cameras); }
            else if (which == "lod") node = std::make_shared<LOD>();
            else if (which == "rendertarget") { double width = 0, height = 0; in >> width >> height; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; } target = std::make_shared<RenderTarget>(width, height); }
            else if (which == "rendertarget3d") { double width = 0, height = 0, depth = 0; in >> width >> height >> depth; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; } target = std::make_shared<RenderTarget3D>(width, height, depth); }
            else { std::cout << "{\"error\":\"unknown node\"}" << std::endl; continue; }
            std::cout << "{\"kind\":\"node\",\"which\":\"" << which << "\"";
            if (target) {
                std::cout << ",\"width\":" << bits(static_cast<float>(target->width)) << ",\"height\":" << bits(static_cast<float>(target->height));
                if (target->isRenderTarget3D) { if (auto target3d = std::dynamic_pointer_cast<RenderTarget3D>(target)) std::cout << ",\"depth\":" << bits(static_cast<float>(target3d->depth)); }
                std::cout << "}" << std::endl;
                continue;
            }
            std::cout << ",\"type\":\"" << node->type << "\"";
            std::cout << ",\"fields\":{";
            std::cout << "\"visible\":" << (node->visible ? "true" : "false");
            std::cout << ",\"castShadow\":" << (node->castShadow ? "true" : "false");
            std::cout << ",\"receiveShadow\":" << (node->receiveShadow ? "true" : "false");
            std::cout << ",\"frustumCulled\":" << (node->frustumCulled ? "true" : "false");
            std::cout << ",\"renderOrder\":" << bits(static_cast<float>(node->renderOrder));
            std::cout << ",\"matrixAutoUpdate\":" << (node->matrixAutoUpdate ? "true" : "false");
            std::cout << ",\"matrixWorldAutoUpdate\":" << (node->matrixWorldAutoUpdate ? "true" : "false");
            std::cout << ",\"layersMask\":" << node->layers.mask.value().toNumber();
            if (auto arrayCamera = std::dynamic_pointer_cast<ArrayCamera>(node)) std::cout << ",\"camerasCount\":" << static_cast<int>(arrayCamera->cameras.size());
            if (auto perspective = std::dynamic_pointer_cast<PerspectiveCamera>(node)) { std::cout << ",\"fov\":" << bits(static_cast<float>(perspective->fov)) << ",\"aspect\":" << bits(static_cast<float>(perspective->aspect)) << ",\"near\":" << bits(static_cast<float>(perspective->near)) << ",\"far\":" << bits(static_cast<float>(perspective->far)); }
            if (auto ortho = std::dynamic_pointer_cast<OrthographicCamera>(node)) { std::cout << ",\"left\":" << bits(static_cast<float>(ortho->left)) << ",\"right\":" << bits(static_cast<float>(ortho->right)) << ",\"top\":" << bits(static_cast<float>(ortho->top)) << ",\"bottom\":" << bits(static_cast<float>(ortho->bottom)) << ",\"near\":" << bits(static_cast<float>(ortho->near)) << ",\"far\":" << bits(static_cast<float>(ortho->far)); }
            std::cout << "}}" << std::endl;
            continue;
        } else if (kind == "texture") {
            std::string which;
            in >> which;
            std::shared_ptr<Texture> texture;
            if (which == "base") texture = std::make_shared<Texture>();
            else if (which == "data") { double width = 0, height = 0; in >> width >> height; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
                std::vector<std::uint8_t> bytes(static_cast<std::size_t>(width * height * 4), 0);
                texture = std::make_shared<DataTexture>(Uint8Array(bytes), width, height); }
            else if (which == "depth") { double width = 0, height = 0; in >> width >> height; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; } texture = std::make_shared<DepthTexture>(width, height); }
            else if (which == "cube") { double count = 0; in >> count; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
                TextureImage::Faces faces;
                for (int k = 0; k < static_cast<int>(count); k += 1) faces.push_back(std::make_shared<TextureImage>(4.0, 4.0));
                texture = std::make_shared<CubeTexture>(TextureImage::array(faces)); }
            else if (which == "array2d") { double width = 0, height = 0, depth = 0; in >> width >> height >> depth; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
                std::vector<std::uint8_t> bytes(static_cast<std::size_t>(width * height * depth * 4), 0);
                texture = std::make_shared<DataArrayTexture>(Uint8Array(bytes), width, height, depth); }
            else if (which == "data3d") { double width = 0, height = 0, depth = 0; in >> width >> height >> depth; if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
                std::vector<std::uint8_t> bytes(static_cast<std::size_t>(width * height * depth * 4), 0);
                texture = std::make_shared<Data3DTexture>(Uint8Array(bytes), width, height, depth); }
            else if (which == "compressed") { if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"trailing tokens\"}" << std::endl; continue; } texture = std::make_shared<CompressedTexture>(); }
            else if (which == "compressedarray") { if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"trailing tokens\"}" << std::endl; continue; } texture = std::make_shared<CompressedArrayTexture>(); }
            else { std::cout << "{\"error\":\"unknown texture\"}" << std::endl; continue; }
            std::cout << "{\"kind\":\"texture\",\"which\":\"" << which << "\",\"fields\":{";
            std::cout << "\"mapping\":" << bits(static_cast<float>(texture->mapping));
            std::cout << ",\"wrapS\":" << bits(static_cast<float>(texture->wrapS));
            std::cout << ",\"wrapT\":" << bits(static_cast<float>(texture->wrapT));
            std::cout << ",\"magFilter\":" << bits(static_cast<float>(texture->magFilter));
            std::cout << ",\"minFilter\":" << bits(static_cast<float>(texture->minFilter));
            std::cout << ",\"format\":" << bits(static_cast<float>(texture->format));
            std::cout << ",\"type\":" << bits(static_cast<float>(texture->type));
            std::cout << ",\"anisotropy\":" << bits(static_cast<float>(texture->anisotropy));
            std::cout << ",\"flipY\":" << (texture->flipY ? "true" : "false");
            std::cout << ",\"generateMipmaps\":" << (texture->generateMipmaps ? "true" : "false");
            std::cout << ",\"premultiplyAlpha\":" << (texture->premultiplyAlpha ? "true" : "false");
            std::cout << ",\"unpackAlignment\":" << bits(static_cast<float>(texture->unpackAlignment));
            std::cout << ",\"colorSpace\":\"" << texture->colorSpace << "\"";
            std::cout << "}}" << std::endl;
            continue;
        } else if (kind == "fog") {
            std::string which;
            in >> which;
            double r = 0, g = 0, b = 0, density = 0, near = 0, far = 0;
            if (which == "linear") {
                in >> r >> g >> b >> near >> far;
                if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
                Fog fog(Color(r, g, b), near, far);
                std::cout << "{\"kind\":\"fog\",\"which\":\"linear\",\"name\":\"" << fog.name << "\",\"color\":[" << bits(static_cast<float>(fog.color.r)) << "," << bits(static_cast<float>(fog.color.g)) << "," << bits(static_cast<float>(fog.color.b)) << "],\"near\":" << bits(static_cast<float>(fog.near)) << ",\"far\":" << bits(static_cast<float>(fog.far)) << "}" << std::endl;
            } else if (which == "exp2") {
                in >> r >> g >> b >> density;
                if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
                FogExp2 fog(Color(r, g, b), density);
                std::cout << "{\"kind\":\"fog\",\"which\":\"exp2\",\"name\":\"" << fog.name << "\",\"color\":[" << bits(static_cast<float>(fog.color.r)) << "," << bits(static_cast<float>(fog.color.g)) << "," << bits(static_cast<float>(fog.color.b)) << "],\"density\":" << bits(static_cast<float>(fog.density)) << "}" << std::endl;
            } else { std::cout << "{\"error\":\"unknown fog\"}" << std::endl; continue; }
            continue;
        } else if (kind == "sh") {
            double count = 0;
            in >> count;
            std::vector<double> values;
            for (int k = 0; k < static_cast<int>(count); k += 1) { double value = 0; in >> value; values.push_back(value); }
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            SphericalHarmonics3 sh;
            for (std::size_t k = 0; k < values.size() / 3 && k < 9; k += 1) sh.coefficients[k].set(values[k * 3], values[k * 3 + 1], values[k * 3 + 2]);
            std::cout << "{\"kind\":\"sh\",\"coefficients\":[";
            for (int k = 0; k < 9; k += 1) { if (k > 0) std::cout << ","; std::cout << "[" << bits(static_cast<float>(sh.coefficients[k].x)) << "," << bits(static_cast<float>(sh.coefficients[k].y)) << "," << bits(static_cast<float>(sh.coefficients[k].z)) << "]"; }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "instance") {
            double count = 0;
            in >> count;
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            auto geometry = std::make_shared<BoxGeometry>(1, 1, 1, 1, 1, 1);
            auto material = std::make_shared<MeshBasicMaterial>();
            InstancedMesh mesh(geometry, material, count);
            std::cout << "{\"kind\":\"instance\",\"type\":\"" << mesh.type << "\"";
            std::cout << ",\"count\":" << bits(static_cast<float>(mesh.count));
            if (mesh.instanceMatrix) {
                std::cout << ",\"matrixCount\":" << static_cast<int>(mesh.instanceMatrix->count);
                std::cout << ",\"matrixItemSize\":" << static_cast<int>(mesh.instanceMatrix->itemSize);
                std::cout << ",\"matrixFirst\":[";
                for (int i = 0; i < 8 && i < static_cast<int>(mesh.instanceMatrix->count) * 16; i += 1) { if (i > 0) std::cout << ","; std::cout << bits(static_cast<float>(mesh.instanceMatrix->getComponent(i / 16, i % 16))); }
                std::cout << "]";
            } else std::cout << ",\"matrixCount\":null";
            std::cout << ",\"hasColor\":" << (mesh.instanceColor ? "true" : "false");
            std::cout << ",\"frustumCulled\":" << (mesh.frustumCulled ? "true" : "false");
            std::cout << "}" << std::endl;
            continue;
        } else if (kind == "path") {
            double count = 0;
            in >> count;
            std::vector<double> coordinates;
            for (int k = 0; k < static_cast<int>(count) * 2; k += 1) { double value = 0; in >> value; coordinates.push_back(value); }
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            Path path;
            for (int k = 0; k < static_cast<int>(count); k += 1) {
                if (k == 0) path.moveTo(coordinates[0], coordinates[1]);
                else path.lineTo(coordinates[k * 2], coordinates[k * 2 + 1]);
            }
            std::cout << "{\"kind\":\"path\",\"curves\":" << path.curves->size() << ",\"points\":[";
            for (int i = 0; i <= 4; i += 1) { const auto point = path.getPoint(i / 4.0); if (i > 0) std::cout << ","; std::cout << "[" << bits(static_cast<float>(point.x)) << "," << bits(static_cast<float>(point.y)) << "]"; }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "stereo") {
            StereoCamera stereo;
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            std::cout << "{\"kind\":\"stereo\",\"type\":\"" << stereo.type << "\"";
            std::cout << ",\"aspect\":" << bits(static_cast<float>(stereo.aspect)) << ",\"eyeSep\":" << bits(static_cast<float>(stereo.eyeSep));
            std::cout << ",\"hasLeft\":" << (stereo.cameraL ? "true" : "false") << ",\"hasRight\":" << (stereo.cameraR ? "true" : "false");
            if (stereo.cameraL) std::cout << ",\"leftFov\":" << bits(static_cast<float>(stereo.cameraL->fov)) << ",\"leftAspect\":" << bits(static_cast<float>(stereo.cameraL->aspect));
            std::cout << "}" << std::endl;
            continue;
        } else if (kind == "ray") {
            double ox = 0, oy = 0, oz = 0, dx = 0, dy = 0, dz = 0, near = 0, far = 0;
            in >> ox >> oy >> oz >> dx >> dy >> dz >> near >> far;
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            Raycaster raycaster(Vector3(ox, oy, oz), Vector3(dx, dy, dz), near, far);
            std::cout << "{\"kind\":\"ray\",\"origin\":[" << bits(static_cast<float>(raycaster.ray.origin.x)) << "," << bits(static_cast<float>(raycaster.ray.origin.y)) << "," << bits(static_cast<float>(raycaster.ray.origin.z)) << "]";
            std::cout << ",\"direction\":[" << bits(static_cast<float>(raycaster.ray.direction.x)) << "," << bits(static_cast<float>(raycaster.ray.direction.y)) << "," << bits(static_cast<float>(raycaster.ray.direction.z)) << "]";
            std::cout << ",\"near\":" << bits(static_cast<float>(raycaster.near));
            std::cout << ",\"far\":" << bits(static_cast<float>(raycaster.far));
            std::cout << ",\"paramKeys\":" << raycaster.params.size();
            const auto lineThreshold = raycaster.params.contains("Line") ? raycaster.params.at("Line").at("threshold").get<double>() : -1.0;
            const auto pointsThreshold = raycaster.params.contains("Points") ? raycaster.params.at("Points").at("threshold").get<double>() : -1.0;
            std::cout << ",\"lineThreshold\":" << bits(static_cast<float>(lineThreshold));
            std::cout << ",\"pointsThreshold\":" << bits(static_cast<float>(pointsThreshold));
            std::cout << "}" << std::endl;
            continue;
        } else if (kind == "curvepath") {
            double ax = 0, ay = 0, bx = 0, by = 0, cx = 0, cy = 0;
            in >> ax >> ay >> bx >> by >> cx >> cy;
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            CurvePath<Vector2> path;
            path.add(std::make_shared<LineCurve>(std::make_shared<Vector2>(ax, ay), std::make_shared<Vector2>(bx, by)));
            path.add(std::make_shared<LineCurve>(std::make_shared<Vector2>(bx, by), std::make_shared<Vector2>(cx, cy)));
            std::cout << "{\"kind\":\"curvepath\",\"curves\":" << path.curves->size() << ",\"points\":[";
            for (int i = 0; i <= 4; i += 1) { const auto point = path.getPoint(i / 4.0); if (i > 0) std::cout << ","; std::cout << "[" << bits(static_cast<float>(point.x)) << "," << bits(static_cast<float>(point.y)) << "]"; }
            std::cout << "]}" << std::endl;
            continue;
        } else if (kind == "shapath") {
            double count = 0;
            in >> count;
            std::vector<double> coordinates;
            for (int k = 0; k < static_cast<int>(count) * 2; k += 1) { double value = 0; in >> value; coordinates.push_back(value); }
            if (in.fail() || !noTrailingTokens(in)) { std::cout << "{\"error\":\"short input or trailing tokens\"}" << std::endl; continue; }
            ShapePath shapePath;
            for (int k = 0; k < static_cast<int>(count); k += 1) {
                if (k == 0) shapePath.moveTo(coordinates[0], coordinates[1]);
                else shapePath.lineTo(coordinates[k * 2], coordinates[k * 2 + 1]);
            }
            std::cout << "{\"kind\":\"shapath\",\"type\":\"" << shapePath.type << "\",\"subPaths\":" << shapePath.subPaths->size();
            std::cout << ",\"curves\":" << (shapePath.subPaths->empty() ? 0 : static_cast<int>(shapePath.subPaths->front()->curves->size()));
            std::cout << ",\"points\":[";
            if (!shapePath.subPaths->empty()) { for (int i = 0; i <= 4; i += 1) { const auto point = shapePath.subPaths->front()->getPoint(i / 4.0); if (i > 0) std::cout << ","; std::cout << "[" << bits(static_cast<float>(point.x)) << "," << bits(static_cast<float>(point.y)) << "]"; } }
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
