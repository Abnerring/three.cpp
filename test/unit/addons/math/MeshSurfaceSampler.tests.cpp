#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "geometries/BoxGeometry.h"
#include "geometries/SphereGeometry.h"
#include "math/Color.h"
#include "math/MeshSurfaceSampler.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "objects/Mesh.h"
#include <cmath>
#include <memory>
#include <vector>

// The reference values come from the pinned examples/jsm/math/MeshSurfaceSampler.js and are the
// same values the differential probe compares bit for bit.
namespace {
std::vector<double> draws{0.0, 0.5, 1.0, 0.25};
MeshSurfaceSampler makeSampler(const std::shared_ptr<Mesh>& mesh, const std::string& weight = "") {
    MeshSurfaceSampler sampler(mesh);
    sampler.setWeightAttribute(weight);
    sampler.build();
    std::size_t cursor = 0;
    sampler.setRandomGenerator([&cursor]() { return draws[cursor++ % draws.size()]; });
    return sampler;
}
}

TEST_CASE("MeshSurfaceSampler builds the pinned cumulative distribution", "[meshsurfacesampler]") {
    auto mesh = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(1, 1, 1));
    mesh->updateMatrixWorld(true);
    MeshSurfaceSampler sampler(mesh);
    REQUIRE(sampler.indexAttribute != nullptr);
    REQUIRE(sampler.positionAttribute != nullptr);
    REQUIRE(sampler.normalAttribute != nullptr);
    REQUIRE(sampler.uvAttribute != nullptr);
    REQUIRE(sampler.colorAttribute == nullptr);
    REQUIRE(sampler.weightAttribute == nullptr);
    REQUIRE(sampler.distribution.empty());
    sampler.build();
    // A unit box has twelve faces of area 0.5 each, so the cumulative total is 6.
    REQUIRE(sampler.distribution.size() == 12);
    REQUIRE(sampler.distribution[0] == 0.5f);
    REQUIRE(sampler.distribution[11] == 6.0f);
    for (std::size_t i = 1; i < sampler.distribution.size(); ++i)
        REQUIRE(sampler.distribution[i] > sampler.distribution[i - 1]);
}

TEST_CASE("MeshSurfaceSampler samples position, normal, colour and uv", "[meshsurfacesampler]") {
    auto geometry = std::make_shared<BoxGeometry>(1, 1, 1);
    const double count = geometry->getAttribute("position")->getCount();
    std::vector<double> colors;
    for (double i = 0; i < count; i += 1) {
        colors.push_back(std::fmod(i, 7.) / 7.);
        colors.push_back(std::fmod(i * 3., 5.) / 5.);
        colors.push_back(std::fmod(i * 5., 11.) / 11.);
    }
    geometry->setAttribute("color", std::make_shared<Float32BufferAttribute>(colors, 3));
    auto mesh = std::make_shared<Mesh>(geometry);
    mesh->updateMatrixWorld(true);
    auto sampler = makeSampler(mesh);
    Vector3 position, normal;
    Color color;
    Vector2 uv;
    sampler.sample(position, &normal, &color, &uv);
    // The pinned implementation for this draw sequence lands on the +x face; the exact values
    // are the ones the differential probe compares (uv.y is exactly 1 on that corner).
    REQUIRE(position.x == 0.5);
    REQUIRE(position.y == 0.5);
    REQUIRE(position.z == 0.);
    REQUIRE(normal.x == 1.);
    REQUIRE(normal.y == 0.);
    REQUIRE(normal.z == 0.);
    REQUIRE(uv.x == 0.5);
    REQUIRE(uv.y == 1.);
    // Colours come from the attribute, so they are interpolated inside [ 0, 1 ].
    REQUIRE(color.r >= 0.);
    REQUIRE(color.r <= 1.);
    // Sampling without targets only writes the position.
    Vector3 onlyPosition;
    sampler.sample(onlyPosition);
    REQUIRE(std::abs(onlyPosition.x) == 0.5);
    // Out-of-range draws are the pinned index arithmetic, not a crash.
    std::size_t cursor = 0;
    sampler.setRandomGenerator([&cursor]() { return cursor++ == 0 ? -1. : 0.; });
    Vector3 negative;
    sampler.sample(negative);
    REQUIRE(std::abs(negative.x) <= 0.5);
}

TEST_CASE("MeshSurfaceSampler weights, non-indexed sources and the empty geometry guard",
          "[meshsurfacesampler]") {
    auto box = std::make_shared<BoxGeometry>(1, 1, 1);
    const double count = box->getAttribute("position")->getCount();
    std::vector<double> colors;
    for (double i = 0; i < count; i += 1) {
        colors.push_back(std::fmod(i, 7.) / 7.);
        colors.push_back(std::fmod(i * 3., 5.) / 5.);
        colors.push_back(std::fmod(i * 5., 11.) / 11.);
    }
    box->setAttribute("color", std::make_shared<Float32BufferAttribute>(colors, 3));
    auto weighted = std::make_shared<Mesh>(box);
    weighted->updateMatrixWorld(true);
    auto weightedSampler = makeSampler(weighted, "color");
    REQUIRE(weightedSampler.weightAttribute != nullptr);
    // With per-vertex weights the cumulative distribution no longer grows uniformly.
    REQUIRE(weightedSampler.distribution.back() != 6.0f);
    // Unknown weight attribute names keep the unweighted behaviour, like `? : null` upstream.
    auto missing = makeSampler(weighted, "doesNotExist");
    REQUIRE(missing.weightAttribute == nullptr);
    REQUIRE(missing.distribution.back() == 6.0f);
    // A non-indexed geometry uses the position count for the face total.
    auto flat = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(1, 1, 1)->toNonIndexed());
    flat->updateMatrixWorld(true);
    auto flatSampler = makeSampler(flat);
    REQUIRE(flatSampler.indexAttribute == nullptr);
    REQUIRE(flatSampler.distribution.size() == 12);
    // An empty geometry is the pinned TypeError instead of a null dereference.
    auto empty = std::make_shared<Mesh>();
    MeshSurfaceSampler emptySampler(empty);
    REQUIRE(emptySampler.positionAttribute == nullptr);
    REQUIRE_THROWS_AS(emptySampler.build(), JSTypeError);
    // A populated geometry gives every sample a unit-length normal, except for the degenerate
    // face the pinned module also collapses to NaN (sample 2 of this draw sequence, verified
    // against the upstream class and matched bit for bit by the differential probe).
    auto sphere = makeSampler(std::make_shared<Mesh>(std::make_shared<SphereGeometry>(1, 8, 6)));
    for (int i = 0; i < 4; ++i) {
        Vector3 position, normal;
        sphere.sample(position, &normal);
        if (i == 2) {
            REQUIRE(std::isnan(position.x));
            REQUIRE(std::isnan(normal.x));
            continue;
        }
        REQUIRE(std::abs(normal.length() - 1.) < 1e-6);
    }
}
