#include "geometries/ParametricGeometry.h"
#include "core/BufferAttribute.h"
#include "math/JSNumberMath.h"
#include <cstdint>
#include <vector>

namespace {
constexpr double PI = 3.141592653589793;
// The upstream default callback: ( u, v, target ) => target.set( u, v, cos( u ) * sin( v ) ).
void defaultParametricFunction(double u, double v, Vector3& target) {
    target.set(u, v, JSNumberMath::cos(u) * JSNumberMath::sin(v));
}
}

ParametricGeometry::ParametricGeometry(Func func, double slices, double stacks)
    : parameters{func ? std::move(func) : Func(defaultParametricFunction), slices, stacks} {
    type = "ParametricGeometry";
    // buffers
    std::vector<std::uint32_t> indices;
    std::vector<double> vertices, normals, uvs;
    const double EPS = 0.00001;
    Vector3 normal, p1, pu, pv;
    Vector3 p0;
    // generate vertices, normals and uvs
    const double sliceCount = parameters.slices + 1;
    for (double i = 0; i <= parameters.stacks; i += 1) {
        const double v = i / parameters.stacks;
        for (double j = 0; j <= parameters.slices; j += 1) {
            const double u = j / parameters.slices;
            // vertex
            parameters.func(u, v, p0);
            vertices.push_back(p0.x);
            vertices.push_back(p0.y);
            vertices.push_back(p0.z);
            // normal
            // approximate tangent vectors via finite differences
            if (u - EPS >= 0) {
                parameters.func(u - EPS, v, p1);
                pu.subVectors(p0, p1);
            } else {
                parameters.func(u + EPS, v, p1);
                pu.subVectors(p1, p0);
            }
            if (v - EPS >= 0) {
                parameters.func(u, v - EPS, p1);
                pv.subVectors(p0, p1);
            } else {
                parameters.func(u, v + EPS, p1);
                pv.subVectors(p1, p0);
            }
            // cross product of tangent vectors returns surface normal
            normal.crossVectors(pu, pv).normalize();
            normals.push_back(normal.x);
            normals.push_back(normal.y);
            normals.push_back(normal.z);
            // uv
            uvs.push_back(u);
            uvs.push_back(v);
        }
    }
    // generate indices
    for (double i = 0; i < parameters.stacks; i += 1) {
        for (double j = 0; j < parameters.slices; j += 1) {
            const double a = i * sliceCount + j;
            const double b = i * sliceCount + j + 1;
            const double c = (i + 1) * sliceCount + j + 1;
            const double d = (i + 1) * sliceCount + j;
            // faces one and two
            indices.push_back(std::uint32_t(a));
            indices.push_back(std::uint32_t(b));
            indices.push_back(std::uint32_t(d));
            indices.push_back(std::uint32_t(b));
            indices.push_back(std::uint32_t(c));
            indices.push_back(std::uint32_t(d));
        }
    }
    // build geometry
    setIndex(indices);
    setAttribute("position", std::make_shared<Float32BufferAttribute>(vertices, 3));
    setAttribute("normal", std::make_shared<Float32BufferAttribute>(normals, 3));
    setAttribute("uv", std::make_shared<Float32BufferAttribute>(uvs, 2));
}

BufferGeometry& ParametricGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    const auto* parametric = dynamic_cast<const ParametricGeometry*>(&source);
    if (!parametric)
        throw JSTypeError("ParametricGeometry: source is not a ParametricGeometry");
    parameters = parametric->parameters;
    return *this;
}

std::shared_ptr<BufferGeometry> ParametricGeometry::clone() const {
    auto result = std::make_shared<ParametricGeometry>(parameters.func, parameters.slices, parameters.stacks);
    result->copy(*this);
    return result;
}

JsonValue ParametricGeometry::toJSON() const {
    JsonValue result = BufferGeometry::toJSON();
    // `parameters.func` is a function value, which JSON.stringify omits.
    result["slices"] = parameters.slices;
    result["stacks"] = parameters.stacks;
    return result;
}
