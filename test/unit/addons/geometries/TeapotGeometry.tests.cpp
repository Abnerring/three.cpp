#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "geometries/TeapotGeometry.h"
#include "math/Vector3.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// Reference values in this file were produced by running the pinned three.js module
// examples/jsm/geometries/TeapotGeometry.js under the pinned Node oracle.
namespace {

std::shared_ptr<BufferAttribute> attributeOf(const TeapotGeometry& geometry, const std::string& name) {
    return std::dynamic_pointer_cast<BufferAttribute>(geometry.getAttribute(name));
}
// The pinned constructor stores the generated values in a Float32Array; the port uses
// Float32BufferAttribute, so every component is read back as the binary64 conversion of the
// stored float32 value.
std::vector<double> components(const TeapotGeometry& geometry, const std::string& name) {
    const auto attribute = attributeOf(geometry, name);
    REQUIRE(attribute != nullptr);
    const Float32Array& array = std::get<Float32Array>(attribute->array);
    std::vector<double> values;
    for (std::size_t i = 0; i < array.size(); i++) values.push_back(double(array[i]));
    return values;
}
std::vector<double> indexValues(const TeapotGeometry& geometry) {
    REQUIRE(geometry.index != nullptr);
    std::vector<double> values;
    for (std::size_t i = 0; i < std::size_t(geometry.index->count); i++)
        values.push_back(geometry.index->getX(double(i)));
    return values;
}
std::vector<double> head(const std::vector<double>& values, std::size_t count) {
    REQUIRE(values.size() >= count);
    return std::vector<double>(values.begin(), values.begin() + std::ptrdiff_t(count));
}
std::vector<double> tail(const std::vector<double>& values, std::size_t count) {
    REQUIRE(values.size() >= count);
    return std::vector<double>(values.end() - std::ptrdiff_t(count), values.end());
}
std::vector<double> slice(const std::vector<double>& values, std::size_t start, std::size_t count) {
    REQUIRE(values.size() >= start + count);
    return std::vector<double>(values.begin() + std::ptrdiff_t(start),
                               values.begin() + std::ptrdiff_t(start + count));
}
bool allZero(const std::vector<double>& values) {
    return std::all_of(values.begin(), values.end(), [](double value) { return value == 0; });
}
std::size_t distinctValueCount(const std::vector<double>& values) {
    std::vector<double> distinct = values;
    std::sort(distinct.begin(), distinct.end());
    distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());
    return distinct.size();
}
void requireSphere(const TeapotGeometry& geometry, const Vector3& center, double radius) {
    REQUIRE(geometry.boundingSphere.has_value());
    REQUIRE(geometry.boundingSphere->center.x == center.x);
    REQUIRE(geometry.boundingSphere->center.y == center.y);
    REQUIRE(geometry.boundingSphere->center.z == center.z);
    REQUIRE(geometry.boundingSphere->radius == radius);
}

} // namespace

TEST_CASE("TeapotGeometry defaults and attribute layout", "[teapotgeometry]") {
    const TeapotGeometry geometry;

    SECTION("the pinned constructor builds 32 tessellated patches at the default subdivision") {
        // (4 bottom + 8 lid + 20 body) surfaces * (10 + 1)^2 vertices.
        REQUIRE(geometry.type == "BufferGeometry");
        REQUIRE(attributeOf(geometry, "position")->getCount() == 3872.);
        REQUIRE(attributeOf(geometry, "normal")->getCount() == 3872.);
        REQUIRE(attributeOf(geometry, "uv")->getCount() == 3872.);
        REQUIRE(geometry.index != nullptr);
        REQUIRE(geometry.index->getCount() == 18960.);
    }

    SECTION("position and normal use three components, uv two") {
        REQUIRE(attributeOf(geometry, "position")->getItemSize() == 3.);
        REQUIRE(attributeOf(geometry, "normal")->getItemSize() == 3.);
        REQUIRE(attributeOf(geometry, "uv")->getItemSize() == 2.);
        REQUIRE_FALSE(attributeOf(geometry, "position")->getNormalized());
        REQUIRE_FALSE(attributeOf(geometry, "normal")->getNormalized());
        REQUIRE_FALSE(attributeOf(geometry, "uv")->getNormalized());
        REQUIRE(components(geometry, "position").size() == 3872 * 3);
        REQUIRE(components(geometry, "normal").size() == 3872 * 3);
        REQUIRE(components(geometry, "uv").size() == 3872 * 2);
    }

    SECTION("the pinned arrays are float32 attributes and a uint32 index") {
        REQUIRE(std::holds_alternative<Float32Array>(attributeOf(geometry, "position")->array));
        REQUIRE(std::holds_alternative<Float32Array>(attributeOf(geometry, "normal")->array));
        REQUIRE(std::holds_alternative<Float32Array>(attributeOf(geometry, "uv")->array));
        REQUIRE(std::holds_alternative<Uint32Array>(geometry.index->array));
    }

    SECTION("the pinned class names no parameters and no type of its own") {
        const JsonValue json = geometry.toJSON();
        REQUIRE(json.at("type").get<std::string>() == "BufferGeometry");
        for (const auto& name : {"size", "segments", "bottom", "lid", "body", "fitLid", "blinn"})
            REQUIRE_FALSE(json.contains(name));
    }
}

TEST_CASE("TeapotGeometry reproduces the pinned bezier output", "[teapotgeometry]") {
    const TeapotGeometry geometry;

    SECTION("the first vertices, normals and uvs of the pinned reference run") {
        const std::vector<double> positions = components(geometry, "position");
        REQUIRE(head(positions, 6) == std::vector<double>{44.44444274902344, 26.190475463867188, 0,
                                                          43.87200164794922, 26.190475463867188,
                                                          7.292444229125977});
        // The third component is -0 in the pinned Float32Array: -trueSize * vert[ 1 ] with vert[ 1 ]
        // positive zero.
        REQUIRE(positions[2] == 0.);
        REQUIRE(std::signbit(positions[2]));
        REQUIRE(head(components(geometry, "normal"), 6) ==
                std::vector<double>{-0.9028605222702026, -0.4299335777759552, 0.,
                                    -0.891532301902771, -0.4301578998565674, -0.14189553260803223});
        REQUIRE(head(components(geometry, "uv"), 6) == std::vector<double>{1, 1, 0.8999999761581421,
                                                                          1, 0.800000011920929, 1});
    }

    SECTION("the last vertex is the end of the last bottom patch") {
        const std::vector<double> positions = components(geometry, "position");
        REQUIRE(tail(positions, 3) == std::vector<double>{47.619049072265625, -45.238094329833984, 0});
        REQUIRE(std::signbit(positions[positions.size() - 1]));
    }

    SECTION("the index keeps the pinned vertex order and drops the degenerate cusps") {
        const std::vector<double> indices = indexValues(geometry);
        REQUIRE(indices.size() == 18960);
        REQUIRE(head(indices, 12) == std::vector<double>{0, 1, 12, 0, 12, 11, 1, 2, 13, 1, 13, 12});
        // Eight cusp vertices are never referenced, and every index stays inside the array.
        REQUIRE(distinctValueCount(indices) == 3864);
        REQUIRE(*std::max_element(indices.begin(), indices.end()) == 3871.);
        REQUIRE(*std::min_element(indices.begin(), indices.end()) == 0.);
    }

    SECTION("computeBoundingSphere of the pinned run") {
        requireSphere(geometry, Vector3(6.888889312744141, 0, 0), 106.03038744060532);
    }
}

TEST_CASE("TeapotGeometry clamps and floors the subdivision", "[teapotgeometry]") {
    const TeapotGeometry one(50, 1);
    const TeapotGeometry two(50, 2);
    const TeapotGeometry fractional(50, 2.9);
    const TeapotGeometry four(50, 4);
    const TeapotGeometry nan(50, std::nan(""));

    SECTION("Math.max( 2, Math.floor( segments ) )") {
        // segments 1 and 2.9 both collapse onto the minimum of two segments per patch.
        REQUIRE(one.getAttribute("position")->getCount() == 288.);
        REQUIRE(fractional.getAttribute("position")->getCount() == 288.);
        REQUIRE(components(one, "position") == components(two, "position"));
        REQUIRE(components(one, "normal") == components(two, "normal"));
        REQUIRE(components(one, "uv") == components(two, "uv"));
        REQUIRE(indexValues(one) == indexValues(two));
        REQUIRE(indexValues(fractional) == indexValues(two));
        REQUIRE(indexValues(two).size() == 720);
    }

    SECTION("the pinned two-segment reference run") {
        REQUIRE(head(components(two, "position"), 6) ==
                std::vector<double>{44.44444274902344, 26.190475463867188, 0, 31.55555534362793,
                                    26.190475463867188, 31.55555534362793});
        REQUIRE(head(indexValues(two), 12) == std::vector<double>{0, 1, 4, 0, 4, 3, 1, 2, 5, 1, 5, 4});
        REQUIRE(head(components(two, "uv"), 6) == std::vector<double>{1, 1, 0.5, 1, 0, 1});
        requireSphere(two, Vector3(6.795635223388672, 0, 0), 105.97861363870518);
    }

    SECTION("four segments per patch") {
        REQUIRE(four.getAttribute("position")->getCount() == 800.);
        REQUIRE(indexValues(four).size() == 2976);
        REQUIRE(head(components(four, "position"), 6) ==
                std::vector<double>{44.44444274902344, 26.190475463867188, 0, 41, 26.190475463867188,
                                    17.44444465637207});
        REQUIRE(head(indexValues(four), 12) == std::vector<double>{0, 1, 6, 0, 6, 5, 1, 2, 7, 1, 7, 6});
    }

    SECTION("NaN segments stay empty, because Math.max propagates the NaN") {
        REQUIRE(nan.getAttribute("position")->getCount() == 0.);
        REQUIRE(nan.getAttribute("normal")->getCount() == 0.);
        REQUIRE(nan.getAttribute("uv")->getCount() == 0.);
        REQUIRE(nan.index != nullptr);
        REQUIRE(nan.index->getCount() == 0.);
        REQUIRE(components(nan, "position").empty());
        requireSphere(nan, Vector3(0, 0, 0), 0.);
    }
}

TEST_CASE("TeapotGeometry builds only the requested surfaces", "[teapotgeometry]") {
    const TeapotGeometry noBottom(50, 10, false, true, true, true, true);
    const TeapotGeometry noLid(50, 10, true, false, true, true, true);
    const TeapotGeometry noBody(50, 10, true, true, false, true, true);
    const TeapotGeometry noBottomLid(50, 10, false, false, true, true, true);
    const TeapotGeometry nothing(50, 10, false, false, false, true, true);

    SECTION("skipping bottom, lid or body removes their surfaces") {
        // (8 + 20) * 121, (4 + 20) * 121 and (4 + 8) * 121 vertices.
        REQUIRE(noBottom.getAttribute("position")->getCount() == 3388.);
        REQUIRE(noLid.getAttribute("position")->getCount() == 2904.);
        REQUIRE(noBody.getAttribute("position")->getCount() == 1452.);
        REQUIRE(noBottomLid.getAttribute("position")->getCount() == 2420.);
        REQUIRE(indexValues(noBottom).size() == 16680);
        REQUIRE(indexValues(noLid).size() == 14280);
        REQUIRE(indexValues(noBody).size() == 6960);
        REQUIRE(indexValues(noBottomLid).size() == 12000);
    }

    SECTION("the pinned bottom-less run ends on the lid instead of the bottom patch") {
        const std::vector<double> positions = components(noBottom, "position");
        // The last component is the tiny negative zero-crossing of the fitted lid, not a plain -0.
        REQUIRE(tail(positions, 3) == std::vector<double>{44.4476203918457, 26.190475463867188,
                                                          -7.049034868588126e-15});
        requireSphere(noBottom, Vector3(6.888889312744141, 2.380952835083008, 0), 105.4152503498436);
    }

    SECTION("the pinned lid-less run keeps the bottom and its own bounding sphere") {
        requireSphere(noLid, Vector3(6.888889312744141, -10.342262268066406, 0), 109.26579021838133);
        REQUIRE(tail(components(noLid, "position"), 3) ==
                std::vector<double>{47.619049072265625, -45.238094329833984, 0});
    }

    SECTION("a body-less teapot is only rim, lid and bottom") {
        const std::vector<double> positions = components(noBody, "position");
        REQUIRE(head(positions, 6) == std::vector<double>{0, 50, 0, 0, 50, 0});
        REQUIRE(std::signbit(positions[2]));
        REQUIRE(std::signbit(positions[5]));
        REQUIRE(head(components(noBody, "normal"), 6) == std::vector<double>{0, 1, 0, 0, 1, 0});
        REQUIRE(head(indexValues(noBody), 12) ==
                std::vector<double>{0, 12, 11, 1, 13, 12, 2, 14, 13, 3, 15, 14});
        requireSphere(noBody, Vector3(0, 0, 0), 66.36276620818754);
    }

    SECTION("all-false still installs empty attributes and an empty index") {
        REQUIRE(nothing.getAttribute("position")->getCount() == 0.);
        REQUIRE(nothing.getAttribute("normal")->getCount() == 0.);
        REQUIRE(nothing.getAttribute("uv")->getCount() == 0.);
        REQUIRE(attributeOf(nothing, "position")->getItemSize() == 3.);
        REQUIRE(attributeOf(nothing, "uv")->getItemSize() == 2.);
        REQUIRE(nothing.index != nullptr);
        REQUIRE(nothing.index->getCount() == 0.);
        REQUIRE(indexValues(nothing).empty());
        REQUIRE(nothing.type == "BufferGeometry");
        requireSphere(nothing, Vector3(0, 0, 0), 0.);
    }
}

TEST_CASE("TeapotGeometry applies the fitLid and blinn scalings", "[teapotgeometry]") {
    const TeapotGeometry fitted;
    const TeapotGeometry looseLid(50, 10, true, true, true, false, true);
    const TeapotGeometry original(50, 10, true, true, true, true, false);

    SECTION("fitLid stretches only the lid patches in X and Y") {
        const std::vector<double> fittedPositions = components(fitted, "position");
        const std::vector<double> loosePositions = components(looseLid, "position");
        // The first component the 7.7% lid factor changes: vertex 2431, the second lid row.
        REQUIRE(fittedPositions[7293] == 6.653466701507568);
        REQUIRE(loosePositions[7293] == 6.1777777671813965);
        // Vertex 2420 is the lid apex, which stays at ( 0, 50, 0 ) either way.
        REQUIRE(slice(fittedPositions, 7260, 3) == std::vector<double>{0, 50, 0});
        REQUIRE(slice(loosePositions, 7260, 3) == std::vector<double>{0, 50, 0});
    }

    SECTION("blinn=false restores the taller teapot through the Z factor and trueSize") {
        REQUIRE(head(components(original, "position"), 6) ==
                std::vector<double>{34.18803405761719, 26.190475463867188, 0, 33.7476921081543,
                                    26.190475463867188, 5.609572887420654});
        REQUIRE(head(components(original, "normal"), 6) ==
                std::vector<double>{-0.9389874339103699, -0.34395143389701843, 0,
                                    -0.9272454380989075, -0.3441455662250519, -0.1475796103477478});
        REQUIRE(original.getAttribute("position")->getCount() == 3872.);
        REQUIRE(indexValues(original).size() == 18960);
        requireSphere(original, Vector3(5.299144744873047, 0, 0), 83.57075948473224);
    }
}

TEST_CASE("TeapotGeometry scales the model with size", "[teapotgeometry]") {
    const TeapotGeometry unit(1);
    const TeapotGeometry zero(0);

    SECTION("size reaches the vertices only through trueSize") {
        REQUIRE(head(components(unit, "position"), 6) ==
                std::vector<double>{0.8888888955116272, 0.523809552192688, 0, 0.8774399757385254,
                                    0.523809552192688, 0.14584888517856598});
        REQUIRE(head(components(unit, "uv"), 6) ==
                std::vector<double>{1, 1, 0.8999999761581421, 1, 0.800000011920929, 1});
        // A uniform scale leaves the normals untouched.
        REQUIRE(components(unit, "normal") == components(TeapotGeometry(50), "normal"));
        requireSphere(unit, Vector3(0.13777774572372437, 0, 0), 2.1206076560471714);
    }

    SECTION("a zero size collapses every triangle, so no index is stored") {
        REQUIRE(zero.getAttribute("position")->getCount() == 3872.);
        REQUIRE(zero.index != nullptr);
        REQUIRE(zero.index->getCount() == 18960.);
        const std::vector<double> indices = indexValues(zero);
        REQUIRE(indices.size() == 18960);
        REQUIRE(allZero(indices));
        REQUIRE(allZero(components(zero, "position")));
        requireSphere(zero, Vector3(0, 0, 0), 0.);
    }
}

TEST_CASE("TeapotGeometry keeps the pinned clone and JSON behaviour", "[teapotgeometry]") {
    const TeapotGeometry geometry(50, 2);

    SECTION("clone() is the inherited BufferGeometry clone, as upstream does not override it") {
        const std::shared_ptr<BufferGeometry> clone = geometry.clone();
        REQUIRE(clone != nullptr);
        REQUIRE(dynamic_cast<TeapotGeometry*>(clone.get()) == nullptr);
        REQUIRE(clone->getAttribute("position")->getCount() == 288.);
        REQUIRE(clone->type == "BufferGeometry");
    }

    SECTION("the class never assigns a type or parameters, so the JSON keeps the defaults") {
        const JsonValue json = geometry.toJSON();
        REQUIRE(json.at("type").get<std::string>() == "BufferGeometry");
        // The native serializer keeps the geometry payload under `data` (project-wide layout), so
        // only the fields the pinned class could have added are checked here.
        REQUIRE(json.contains("data"));
        REQUIRE_FALSE(json.contains("size"));
        REQUIRE_FALSE(json.contains("segments"));
        REQUIRE_FALSE(json.contains("blinn"));
    }
}
