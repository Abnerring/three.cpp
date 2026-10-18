#include "catch.hpp"
#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "core/JSValue.h"
#include <memory>
#include <vector>

// setIndirect records the attribute and the offset and returns the geometry; getIndirect hands back what was
// recorded and is null until something is set. No rendering is involved in either method, so the port can
// carry them exactly as upstream does.
TEST_CASE("BufferGeometry.setIndirect records the attribute and the offset", "[buffergeometry]") {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    auto geometry = std::make_shared<BufferGeometry>();
    REQUIRE(geometry->getIndirect() == nullptr);
    REQUIRE(geometry->indirectOffset == 0.0);

    auto attribute = std::make_shared<Float32BufferAttribute>(std::vector<float>{1.0f, 2.0f}, 2);
    const auto& returned = geometry->setIndirect(attribute);
    REQUIRE(&returned == geometry.get());
    REQUIRE(geometry->getIndirect() == attribute);
    REQUIRE(geometry->indirectOffset == 0.0);

    geometry->setIndirect(attribute, 4.0);
    REQUIRE(geometry->indirectOffset == 4.0);
    geometry->setIndirect(nullptr);
    REQUIRE(geometry->getIndirect() == nullptr);
}