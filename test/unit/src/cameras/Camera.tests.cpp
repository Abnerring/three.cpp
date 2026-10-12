#include "cameras/OrthographicCamera.h"
#include "cameras/PerspectiveCamera.h"
#include "catch.hpp"
#include <array>
#include <cmath>

namespace {
void requireIdentity(const Matrix4& matrix) {
    for (std::size_t i = 0; i < 16; ++i)
        REQUIRE(matrix.elements()[i] == Approx(i % 5 == 0 ? 1.0 : 0.0).margin(1e-10));
}
void requireElements(const Matrix4& matrix, const std::array<double, 16>& elements) {
    for (std::size_t i = 0; i < 16; ++i)
        REQUIRE(matrix.elements()[i] == Approx(elements[i]).margin(1e-10));
}
} // namespace

TEST_CASE("Camera world inverse excludes scale and uses negative Z forward", "[cameras]") {
    Object3D parent;
    parent.position.set(3, -2, 5);
    parent.rotateY(0.6);
    parent.scale.setScalar(2);
    auto camera = std::make_shared<Camera>();
    camera->position.set(1, 4, -3);
    camera->rotateX(-0.3);
    camera->scale.set(2, 3, 4);
    parent.add(camera);
    parent.updateMatrixWorld();
    Vector3 p, s;
    Quaternion q;
    camera->matrixWorld.decompose(p, q, s);
    Matrix4 rigid, product;
    rigid.compose(p, q, Vector3(1, 1, 1));
    product.multiplyMatrices(camera->matrixWorldInverse, rigid);
    requireIdentity(product);
    Vector3 direction, expected(0, 0, -1);
    camera->getWorldDirection(direction);
    expected.applyQuaternion(q);
    REQUIRE(direction.distanceTo(expected) < 1e-10);
    camera->position.set(2, 5, -4);
    camera->updateWorldMatrix(true, false);
    camera->matrixWorld.decompose(p, q, s);
    rigid.compose(p, q, Vector3(1, 1, 1));
    product.multiplyMatrices(camera->matrixWorldInverse, rigid);
    requireIdentity(product);
}

TEST_CASE("PerspectiveCamera physical lens and view bounds", "[cameras]") {
    PerspectiveCamera camera(60, 2, 0.1, 100);
    REQUIRE(camera.getFilmWidth() == Approx(35));
    REQUIRE(camera.getFilmHeight() == Approx(17.5));
    REQUIRE(camera.getFocalLength() == Approx(15.155444566227676));
    Vector2 size, min, max;
    camera.getViewSize(10, size);
    REQUIRE(size.x == Approx(23.094010767585033));
    REQUIRE(size.y == Approx(11.547005383792516));
    camera.getViewBounds(10, min, max);
    REQUIRE(min.x == Approx(-size.x / 2));
    REQUIRE(max.y == Approx(size.y / 2));
    camera.zoom = 2;
    camera.updateProjectionMatrix();
    REQUIRE(camera.getEffectiveFOV() == Approx(32.20422750397203));
    camera.getViewSize(10, size);
    REQUIRE(size.y == Approx(5.773502691896258));
    camera.setFocalLength(50);
    REQUIRE(camera.getFocalLength() == Approx(50));
    camera.aspect = 0.5;
    REQUIRE(camera.getFilmWidth() == Approx(17.5));
    REQUIRE(camera.getFilmHeight() == Approx(35));
}

TEST_CASE("PerspectiveCamera tiled frustum matches pinned three.js r186 fixture",
          "[cameras][parity]") {
    // Generated from local three.js commit 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
    PerspectiveCamera camera(67, 1.6, 0.3, 888);
    camera.zoom = 1.25;
    camera.filmOffset = 3;
    camera.setViewOffset(1920, 1080, 240, 120, 640, 480);
    requireElements(camera.projectionMatrix,
                    {3.186917986531432, 0, 0, 0, 0, 4.249223982041909, 0, 0, -0.9768356011544488,
                     0.75, -1.0006759040216289, -1, 0, 0, -0.6002027712064886, 0});
    Matrix4 product;
    product.multiplyMatrices(camera.projectionMatrix, camera.projectionMatrixInverse);
    requireIdentity(product);
    auto cloned = std::dynamic_pointer_cast<PerspectiveCamera>(camera.clone());
    REQUIRE(cloned != nullptr);
    REQUIRE(cloned->view == camera.view);
    cloned->view->offsetX = 500;
    REQUIRE(camera.view->offsetX == 240);
    const auto json = camera.toJSON();
    REQUIRE(json.at("object").at("filmOffset") == JsonValue(3));
    REQUIRE(json.at("object").at("view").at("offsetY") == JsonValue(120));
    camera.clearViewOffset();
    REQUIRE(camera.view.has_value());
    REQUIRE_FALSE(camera.view->enabled);
    PerspectiveCamera reference(67, 1920.0 / 1080.0, 0.3, 888);
    reference.zoom = camera.zoom;
    reference.filmOffset = camera.filmOffset;
    reference.updateProjectionMatrix();
    for (std::size_t i = 0; i < 16; ++i)
        REQUIRE(camera.projectionMatrix.elements()[i] ==
                Approx(reference.projectionMatrix.elements()[i]));
}

TEST_CASE("OrthographicCamera tiled frustum matches pinned three.js r186 fixture",
          "[cameras][parity]") {
    OrthographicCamera camera(-4, 6, 5, -3, 0.2, 100);
    camera.zoom = 1.4;
    camera.setViewOffset(800, 600, 125, 45, 300, 220);
    requireElements(camera.projectionMatrix, {0.7466666666666666, 0, 0, 0, 0, 0.9545454545454545, 0,
                                              0, 0, 0, -0.02004008016032064, 0, 0.08666666666666652,
                                              -2.272727272727273, -1.0040080160320641, 1});
    Matrix4 product;
    product.multiplyMatrices(camera.projectionMatrix, camera.projectionMatrixInverse);
    requireIdentity(product);
    auto cloned = std::dynamic_pointer_cast<OrthographicCamera>(camera.clone());
    REQUIRE(cloned != nullptr);
    REQUIRE(cloned->zoom == camera.zoom);
    REQUIRE(cloned->view == camera.view);
    cloned->view->width = 150;
    REQUIRE(camera.view->width == 300);
    camera.clearViewOffset();
    REQUIRE_FALSE(camera.view->enabled);
    REQUIRE(camera.toJSON().at("object").at("left") == JsonValue(-4));
}

TEST_CASE("Camera projection supports WebGL WebGPU and reversed depth endpoints", "[cameras]") {
    PerspectiveCamera perspective(45, 1, 0.2, 10);
    OrthographicCamera orthographic(-2, 2, 2, -2, 0, 10);
    for (int coordinateSystem : {2000, 2001}) {
        for (bool reversed : {false, true}) {
            perspective.coordinateSystem = orthographic.coordinateSystem = coordinateSystem;
            perspective.reversedDepth = orthographic.reversedDepth = reversed;
            perspective.updateProjectionMatrix();
            orthographic.updateProjectionMatrix();
            Vector3 nearPerspective(0, 0, -0.2), farPerspective(0, 0, -10);
            Vector3 nearOrthographic(0, 0, 0), farOrthographic(0, 0, -10);
            nearPerspective.applyMatrix4(perspective.projectionMatrix);
            farPerspective.applyMatrix4(perspective.projectionMatrix);
            nearOrthographic.applyMatrix4(orthographic.projectionMatrix);
            farOrthographic.applyMatrix4(orthographic.projectionMatrix);
            const double nearExpected = reversed ? 1 : (coordinateSystem == 2000 ? -1 : 0);
            const double farExpected = reversed ? 0 : 1;
            REQUIRE(nearPerspective.z == Approx(nearExpected).margin(1e-10));
            REQUIRE(farPerspective.z == Approx(farExpected).margin(1e-10));
            REQUIRE(nearOrthographic.z == Approx(nearExpected).margin(1e-10));
            REQUIRE(farOrthographic.z == Approx(farExpected).margin(1e-10));
        }
    }
}

TEST_CASE("Camera lookAt under a rotated parent preserves negative Z direction", "[cameras]") {
    Object3D parent;
    parent.rotateY(0.6);
    parent.position.set(3, -2, 1);
    auto camera = std::make_shared<PerspectiveCamera>();
    camera->position.set(0, 1, 5);
    parent.add(camera);
    camera->lookAt(7, 2, -4);
    Vector3 p, direction, expected;
    camera->getWorldPosition(p);
    camera->getWorldDirection(direction);
    expected.subVectors(Vector3(7, 2, -4), p);
    expected.normalize();
    REQUIRE(direction.distanceTo(expected) < 1e-10);
}
