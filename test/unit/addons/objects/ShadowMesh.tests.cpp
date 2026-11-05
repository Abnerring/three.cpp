#include "objects/ShadowMesh.h"

#include "constants.h"
#include "geometries/BoxGeometry.h"
#include "math/Matrix4.h"
#include "math/Plane.h"
#include "math/Vector4.h"
#include "materials/MeshBasicMaterial.h"

#include "catch.hpp"

#include <memory>

TEST_CASE("ShadowMesh reuses the source geometry with the stencil shadow material") {
    auto source = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(),
                                         std::make_shared<MeshBasicMaterial>());
    ShadowMesh shadow(source);

    REQUIRE(shadow.geometry == source->geometry);
    const auto material = std::dynamic_pointer_cast<MeshBasicMaterial>(shadow.material);
    REQUIRE(material != nullptr);
    REQUIRE(material->color.getHex() == 0x000000);
    REQUIRE(material->transparent == true);
    REQUIRE(material->opacity == Approx(0.6));
    REQUIRE(material->depthWrite == false);
    REQUIRE(material->stencilWrite == true);
    REQUIRE(material->stencilFunc == EqualStencilFunc);
    REQUIRE(material->stencilRef == 0);
    REQUIRE(material->stencilZPass == IncrementStencilOp);

    REQUIRE(shadow.isShadowMesh);
    REQUIRE(shadow.frustumCulled == false);
    REQUIRE(shadow.matrixAutoUpdate == false);
    // The pinned class references the source world matrix rather than copying it.
    REQUIRE(shadow.meshMatrix == &source->matrixWorld);
}

TEST_CASE("ShadowMesh projects onto the plane from the light position") {
    auto source = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(),
                                         std::make_shared<MeshBasicMaterial>());
    ShadowMesh shadow(source);
    const Plane plane(Vector3(0, 1, 0), -2);
    const Vector4 light(4, 5, 6, 1);

    // dot = 0*4 + 1*5 + 0*6 + -(-2)*1 = 7
    shadow.update(plane, light);
    const auto elements = shadow.matrix.elements();
    REQUIRE(elements[0] == Approx(7));    // dot - 4 * 0
    REQUIRE(elements[4] == Approx(-4));   // -4 * 1
    REQUIRE(elements[5] == Approx(2));    // dot - 5 * 1
    REQUIRE(elements[12] == Approx(-8));  // -4 * -(-2) ... -4 * 2
    REQUIRE(elements[15] == Approx(5));   // dot - 1 * 2

    // The source world matrix is applied on the right, so a translated source shifts the projection.
    source->matrixWorld.makeTranslation(1, 2, 3);
    shadow.update(plane, light);
    const auto translated = shadow.matrix.elements();
    // Column 0 stays the first shadow column because the translation only affects the last column.
    REQUIRE(translated[0] == Approx(7));
    REQUIRE(translated[3] == Approx(elements[3]));
}
