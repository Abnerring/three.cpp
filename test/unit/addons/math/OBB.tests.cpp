#include "catch.hpp"
#include "math/Box3.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/OBB.h"
#include "math/Plane.h"
#include "math/Ray.h"
#include "math/Sphere.h"
#include "math/Vector3.h"
#include <cmath>

namespace {
// The identity Matrix3 of the constructor default.
Matrix3 identity() { return Matrix3(); }
// `Matrix3.set( 0, 1, 0, -1, 0, 0, 0, 0, 1 )`: local x points down world y and local y
// points along world x.
Matrix3 zQuarterTurn() {
    Matrix3 rotation;
    rotation.set(0, 1, 0, -1, 0, 0, 0, 0, 1);
    return rotation;
}
bool isIdentity(const Matrix3& matrix) {
    const auto e = matrix.elements();
    return e[0] == 1.0 && e[1] == 0.0 && e[2] == 0.0 && e[3] == 0.0 && e[4] == 1.0 && e[5] == 0.0 &&
           e[6] == 0.0 && e[7] == 0.0 && e[8] == 1.0;
}
// The storage order of the matrix above is [ 0, -1, 0, 1, 0, 0, 0, 0, 1 ].
bool isQuarterTurn(const Matrix3& matrix) {
    const auto e = matrix.elements();
    return e[0] == 0.0 && e[1] == -1.0 && e[2] == 0.0 && e[3] == 1.0 && e[4] == 0.0 && e[5] == 0.0 &&
           e[6] == 0.0 && e[7] == 0.0 && e[8] == 1.0;
}
// `makeBasis( ( 0, 1, 0 ), ( -1, 0, 0 ), ( 0, 0, 1 ) )` is its transpose.
bool isInverseQuarterTurn(const Matrix3& matrix) {
    const auto e = matrix.elements();
    return e[0] == 0.0 && e[1] == 1.0 && e[2] == 0.0 && e[3] == -1.0 && e[4] == 0.0 && e[5] == 0.0 &&
           e[6] == 0.0 && e[7] == 0.0 && e[8] == 1.0;
}
}

TEST_CASE("OBB construction, set, copy and clone", "[obb]") {
    SECTION("the constructor defaults are zero center, zero half size and identity") {
        const OBB obb;
        REQUIRE(obb.center.x == 0.0);
        REQUIRE(obb.center.y == 0.0);
        REQUIRE(obb.center.z == 0.0);
        REQUIRE(obb.halfSize.x == 0.0);
        REQUIRE(obb.halfSize.y == 0.0);
        REQUIRE(obb.halfSize.z == 0.0);
        REQUIRE(isIdentity(obb.rotation));
    }

    SECTION("constructor arguments are copied, not aliased") {
        Vector3 center(1.0, 2.0, 3.0), halfSize(0.5, 1.0, 1.5);
        Matrix3 rotation = zQuarterTurn();
        const OBB obb(center, halfSize, rotation);
        center.set(9.0, 9.0, 9.0);
        halfSize.set(9.0, 9.0, 9.0);
        rotation.identity();
        REQUIRE(obb.center.x == 1.0);
        REQUIRE(obb.center.y == 2.0);
        REQUIRE(obb.center.z == 3.0);
        REQUIRE(obb.halfSize.x == 0.5);
        REQUIRE(obb.halfSize.y == 1.0);
        REQUIRE(obb.halfSize.z == 1.5);
        REQUIRE(isQuarterTurn(obb.rotation));
    }

    SECTION("set copies the values and returns this") {
        OBB obb;
        const Vector3 center(4.0, 5.0, 6.0), halfSize(1.0, 2.0, 3.0);
        REQUIRE(&obb.set(center, halfSize, identity()) == &obb);
        REQUIRE(obb.center.x == 4.0);
        REQUIRE(obb.center.y == 5.0);
        REQUIRE(obb.center.z == 6.0);
        REQUIRE(obb.halfSize.x == 1.0);
        REQUIRE(obb.halfSize.y == 2.0);
        REQUIRE(obb.halfSize.z == 3.0);
        REQUIRE(isIdentity(obb.rotation));
    }

    SECTION("copy and clone produce independent value copies") {
        const OBB source(Vector3(1.0, 2.0, 3.0), Vector3(0.5, 1.0, 1.5), zQuarterTurn());
        OBB target;
        REQUIRE(&target.copy(source) == &target);
        REQUIRE(target.equals(source));

        OBB clone = source.clone();
        REQUIRE(clone.equals(source));
        clone.center.set(9.0, 9.0, 9.0);
        clone.halfSize.set(9.0, 9.0, 9.0);
        clone.rotation.identity();
        REQUIRE(source.center.x == 1.0);
        REQUIRE(source.halfSize.x == 0.5);
        REQUIRE(isQuarterTurn(source.rotation));
    }

    SECTION("equals compares center, half size and rotation") {
        const OBB obb(Vector3(1.0, 2.0, 3.0), Vector3(0.5, 1.0, 1.5), zQuarterTurn());
        REQUIRE(obb.equals(obb));
        REQUIRE_FALSE(obb.equals(OBB(Vector3(1.0, 2.0, 3.5), Vector3(0.5, 1.0, 1.5), zQuarterTurn())));
        REQUIRE_FALSE(obb.equals(OBB(Vector3(1.0, 2.0, 3.0), Vector3(0.5, 1.0, 2.0), zQuarterTurn())));
        REQUIRE_FALSE(obb.equals(OBB(Vector3(1.0, 2.0, 3.0), Vector3(0.5, 1.0, 1.5), identity())));
    }

    SECTION("getSize doubles the half size") {
        const OBB obb(Vector3(), Vector3(1.0, 2.0, 3.0), identity());
        Vector3 size;
        REQUIRE(&obb.getSize(size) == &size);
        REQUIRE(size.x == 2.0);
        REQUIRE(size.y == 4.0);
        REQUIRE(size.z == 6.0);
    }
}

TEST_CASE("OBB containsPoint and clampPoint", "[obb]") {
    const OBB obb(Vector3(), Vector3(1.0, 2.0, 3.0), identity());

    SECTION("points inside and on the boundary are contained") {
        REQUIRE(obb.containsPoint(Vector3(0.0, 0.0, 0.0)));
        REQUIRE(obb.containsPoint(Vector3(1.0, 2.0, 3.0)));
        REQUIRE(obb.containsPoint(Vector3(-1.0, -2.0, -3.0)));
        REQUIRE_FALSE(obb.containsPoint(Vector3(1.0, 2.0, 3.0000001)));
        REQUIRE_FALSE(obb.containsPoint(Vector3(2.0, 0.0, 0.0)));
    }

    SECTION("the rotation decides which world axis is compared") {
        const OBB rotated(Vector3(), Vector3(1.0, 2.0, 3.0), zQuarterTurn());
        REQUIRE(rotated.containsPoint(Vector3(2.0, 0.0, 0.0)));
        REQUIRE_FALSE(rotated.containsPoint(Vector3(0.0, 3.0, 0.0)));
        REQUIRE_FALSE(rotated.containsPoint(Vector3(0.0, 2.0, 3.0)));
    }

    SECTION("clampPoint projects the point onto the box axes") {
        Vector3 target;
        REQUIRE(&obb.clampPoint(Vector3(5.0, 0.5, -5.0), target) == &target);
        REQUIRE(target.x == 1.0);
        REQUIRE(target.y == 0.5);
        REQUIRE(target.z == -3.0);

        // A point inside the OBB is returned unchanged.
        obb.clampPoint(Vector3(0.25, -0.5, 0.75), target);
        REQUIRE(target.x == 0.25);
        REQUIRE(target.y == -0.5);
        REQUIRE(target.z == 0.75);

        const OBB rotated(Vector3(), Vector3(1.0, 2.0, 3.0), zQuarterTurn());
        rotated.clampPoint(Vector3(5.0, 0.0, 0.0), target);
        REQUIRE(target.x == 2.0);
        REQUIRE(target.y == 0.0);
        REQUIRE(target.z == 0.0);
    }
}

TEST_CASE("OBB sphere, box and plane intersection tests", "[obb]") {
    const OBB obb(Vector3(), Vector3(1.0, 2.0, 3.0), identity());

    SECTION("intersectsSphere uses the clamped point") {
        REQUIRE(obb.intersectsSphere(Sphere(Vector3(2.0, 0.0, 0.0), 1.0)));
        REQUIRE_FALSE(obb.intersectsSphere(Sphere(Vector3(2.0, 0.0, 0.0), 0.5)));
        REQUIRE(obb.intersectsSphere(Sphere(Vector3(0.0, 0.0, 0.0), 0.0)));
    }

    SECTION("intersectsBox3 converts the AABB through fromBox3") {
        REQUIRE(obb.intersectsBox3(Box3(Vector3(0.5, 1.5, 2.5), Vector3(4.0, 5.0, 6.0))));
        REQUIRE_FALSE(obb.intersectsBox3(Box3(Vector3(2.0, 2.5, -1.0), Vector3(3.0, 3.0, 1.0))));
        REQUIRE(obb.intersectsBox3(Box3(Vector3(-0.5, -0.5, -0.5), Vector3(0.5, 0.5, 0.5))));
    }

    SECTION("intersectsPlane uses the projection radius") {
        REQUIRE(obb.intersectsPlane(Plane(Vector3(1.0, 0.0, 0.0), 1.0)));
        REQUIRE_FALSE(obb.intersectsPlane(Plane(Vector3(1.0, 0.0, 0.0), 1.0000001)));
        REQUIRE(obb.intersectsPlane(Plane(Vector3(0.0, 0.0, 1.0), -3.0)));
        REQUIRE_FALSE(obb.intersectsPlane(Plane(Vector3(0.0, 0.0, 1.0), -3.5)));
    }
}

TEST_CASE("OBB intersectsOBB runs the separating axis test", "[obb]") {
    const OBB a(Vector3(), Vector3(1.0, 1.0, 1.0), identity());

    SECTION("axis aligned overlaps, touching and gaps") {
        REQUIRE(a.intersectsOBB(OBB(Vector3(1.5, 0.0, 0.0), Vector3(1.0, 1.0, 1.0), identity())));
        REQUIRE(a.intersectsOBB(OBB(Vector3(2.0, 0.0, 0.0), Vector3(1.0, 1.0, 1.0), identity())));
        REQUIRE_FALSE(a.intersectsOBB(OBB(Vector3(2.5, 0.0, 0.0), Vector3(1.0, 1.0, 1.0), identity())));
        REQUIRE(a.intersectsOBB(OBB(Vector3(0.0, 0.0, 0.0), Vector3(0.0, 0.0, 0.0), identity())));
    }

    SECTION("rotated boxes exercise the edge cross products") {
        Matrix3 diagonal;
        const double angle = 3.141592653589793 / 4;
        const double cosine = std::cos(angle), sine = std::sin(angle);
        diagonal.set(cosine, -sine, 0, sine, cosine, 0, 0, 0, 1);
        REQUIRE(a.intersectsOBB(OBB(Vector3(2.0, 0.0, 0.0), Vector3(1.0, 1.0, 1.0), diagonal)));
        REQUIRE_FALSE(a.intersectsOBB(OBB(Vector3(3.0, 0.0, 0.0), Vector3(1.0, 1.0, 1.0), diagonal)));
    }

    SECTION("the epsilon argument widens every projected interval") {
        const OBB near(Vector3(2.0 + 1e-10, 0.0, 0.0), Vector3(1.0, 1.0, 1.0), identity());
        REQUIRE_FALSE(a.intersectsOBB(near));
        REQUIRE_FALSE(a.intersectsOBB(near, 2.220446049250313e-16));
        REQUIRE(a.intersectsOBB(near, 1e-9));
        // A zero epsilon still reports the exact touch of the boxes above.
        REQUIRE(a.intersectsOBB(OBB(Vector3(2.0, 0.0, 0.0), Vector3(1.0, 1.0, 1.0), identity()), 0.0));
    }
}

TEST_CASE("OBB intersectRay works in the local space of the box", "[obb]") {
    const OBB obb(Vector3(), Vector3(1.0, 2.0, 3.0), identity());

    SECTION("a hit returns the caller target") {
        Vector3 target;
        Vector3* hit = obb.intersectRay(Ray(Vector3(0.0, 0.0, -5.0), Vector3(0.0, 0.0, 1.0)), target);
        REQUIRE(hit == &target);
        REQUIRE(target.x == 0.0);
        REQUIRE(target.y == 0.0);
        REQUIRE(target.z == -3.0);
        REQUIRE(hit->z == -3.0);
    }

    SECTION("a miss returns nullptr and leaves the target alone") {
        Vector3 target(7.0, 7.0, 7.0);
        REQUIRE(obb.intersectRay(Ray(Vector3(0.0, 0.0, -5.0), Vector3(0.0, 0.0, -1.0)), target) == nullptr);
        REQUIRE(target.x == 7.0);
        REQUIRE(target.y == 7.0);
        REQUIRE(target.z == 7.0);
    }

    SECTION("intersectsRay mirrors the nullable result") {
        REQUIRE(obb.intersectsRay(Ray(Vector3(0.0, 0.0, -5.0), Vector3(0.0, 0.0, 1.0))));
        REQUIRE_FALSE(obb.intersectsRay(Ray(Vector3(0.0, 0.0, -5.0), Vector3(0.0, 0.0, -1.0))));
        REQUIRE(obb.intersectsRay(Ray(Vector3(4.0, 0.0, 0.0), Vector3(-1.0, 0.0, 0.0))));
    }

    SECTION("the rotation moves the ray into the local frame") {
        const OBB rotated(Vector3(), Vector3(1.0, 2.0, 3.0), zQuarterTurn());
        // The box reaches world x = +-2 and world y = +-1 after the quarter turn.
        Vector3 target;
        REQUIRE(rotated.intersectRay(Ray(Vector3(0.0, 0.0, -5.0), Vector3(0.0, 0.0, 1.0)), target) != nullptr);
        REQUIRE(target.z == -3.0);
        REQUIRE(rotated.intersectRay(Ray(Vector3(4.0, 0.0, 0.0), Vector3(-1.0, 0.0, 0.0)), target) != nullptr);
        REQUIRE(target.x == 2.0);
        REQUIRE(rotated.intersectRay(Ray(Vector3(0.0, 4.0, 0.0), Vector3(0.0, -1.0, 0.0)), target) != nullptr);
        REQUIRE(target.y == 1.0);
        REQUIRE_FALSE(rotated.intersectsRay(Ray(Vector3(0.0, 4.0, 0.0), Vector3(0.0, 1.0, 0.0))));
        REQUIRE_FALSE(rotated.intersectsRay(Ray(Vector3(4.0, 0.0, 0.0), Vector3(1.0, 0.0, 0.0))));
    }
}

TEST_CASE("OBB fromBox3 and applyMatrix4", "[obb]") {
    SECTION("fromBox3 takes center, half size and an identity rotation") {
        OBB obb(Vector3(9.0, 9.0, 9.0), Vector3(9.0, 9.0, 9.0), zQuarterTurn());
        REQUIRE(&obb.fromBox3(Box3(Vector3(-1.0, -2.0, -3.0), Vector3(1.0, 2.0, 3.0))) == &obb);
        REQUIRE(obb.center.x == 0.0);
        REQUIRE(obb.center.y == 0.0);
        REQUIRE(obb.center.z == 0.0);
        REQUIRE(obb.halfSize.x == 1.0);
        REQUIRE(obb.halfSize.y == 2.0);
        REQUIRE(obb.halfSize.z == 3.0);
        REQUIRE(isIdentity(obb.rotation));
    }

    SECTION("applyMatrix4 composes the scale into the half size") {
        OBB obb(Vector3(), Vector3(1.0, 1.0, 1.0), identity());
        Matrix4 matrix;
        matrix.makeScale(2.0, 3.0, 4.0);
        matrix.setPosition(1.0, 2.0, 3.0);
        REQUIRE(&obb.applyMatrix4(matrix) == &obb);
        REQUIRE(obb.center.x == 1.0);
        REQUIRE(obb.center.y == 2.0);
        REQUIRE(obb.center.z == 3.0);
        REQUIRE(obb.halfSize.x == 2.0);
        REQUIRE(obb.halfSize.y == 3.0);
        REQUIRE(obb.halfSize.z == 4.0);
        REQUIRE(isIdentity(obb.rotation));
    }

    SECTION("a negative determinant keeps the upstream sign on the half size") {
        OBB obb(Vector3(), Vector3(1.0, 1.0, 1.0), identity());
        Matrix4 matrix;
        matrix.makeScale(-2.0, 1.0, 1.0);
        obb.applyMatrix4(matrix);
        REQUIRE(obb.halfSize.x == -2.0);
        REQUIRE(obb.halfSize.y == 1.0);
        REQUIRE(obb.halfSize.z == 1.0);
        REQUIRE(isIdentity(obb.rotation));
    }

    SECTION("an exact rotation is copied into the rotation matrix") {
        OBB obb(Vector3(), Vector3(1.0, 2.0, 3.0), identity());
        Matrix4 matrix;
        matrix.makeBasis(Vector3(0.0, 1.0, 0.0), Vector3(-1.0, 0.0, 0.0), Vector3(0.0, 0.0, 1.0));
        matrix.setPosition(1.0, 2.0, 3.0);
        obb.applyMatrix4(matrix);
        REQUIRE(obb.center.x == 1.0);
        REQUIRE(obb.center.y == 2.0);
        REQUIRE(obb.center.z == 3.0);
        REQUIRE(obb.halfSize.x == 1.0);
        REQUIRE(obb.halfSize.y == 2.0);
        REQUIRE(obb.halfSize.z == 3.0);
        REQUIRE(isInverseQuarterTurn(obb.rotation));
        REQUIRE(obb.containsPoint(Vector3(3.0, 2.0, 3.0)));
        REQUIRE_FALSE(obb.containsPoint(Vector3(1.0, 5.0, 3.0)));
    }
}
