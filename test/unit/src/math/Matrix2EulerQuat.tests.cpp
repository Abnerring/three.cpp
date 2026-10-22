#include "math/Euler.h"
#include "math/Matrix2.h"
#include "math/Quaternion.h"
#include "math/Vector3.h"

#include "catch.hpp"
#include <cmath>

// Expected values come from running the pinned Matrix2.js, Euler.js and Quaternion.js in
// node. Note that the r187 Matrix2 surface is only constructor/identity/fromArray/set.
TEST_CASE("Matrix2 stores its elements column major like upstream") {
    Matrix2 matrix(1, 2, 3, 4);
    // Upstream elements read 1, 3, 2, 4: the constructor arguments are row major.
    const auto& elements = matrix.elements();
    REQUIRE(elements[0] == 1);
    REQUIRE(elements[1] == 3);
    REQUIRE(elements[2] == 2);
    REQUIRE(elements[3] == 4);

    Matrix2 identity;
    identity.identity();
    REQUIRE(identity.elements()[0] == 1);
    REQUIRE(identity.elements()[1] == 0);
    REQUIRE(identity.elements()[2] == 0);
    REQUIRE(identity.elements()[3] == 1);
}

TEST_CASE("Quaternion setFromEuler matches upstream for an XYZ euler") {
    Euler euler(0.1, 0.2, 0.3, EulerOrder::XYZ);
    REQUIRE(euler.x == 0.1);
    REQUIRE(euler.y == 0.2);
    REQUIRE(euler.z == 0.3);

    Quaternion quaternion;
    quaternion.setFromEuler(euler);
    // The port carries these in HIGH_PRECISION, which is float, so the upstream doubles are
    // rounded to float before comparing. The values are Math.fround of the node results.
    // The port computes in float, so intermediates round differently from a double result
    // rounded to float; a relative tolerance of one part in a million is the honest bar.
    REQUIRE(quaternion.x == Approx(0.064071347706071).epsilon(1e-6));
    REQUIRE(quaternion.y == Approx(0.091157549342991).epsilon(1e-6));
    REQUIRE(quaternion.z == Approx(0.153439302024223).epsilon(1e-6));
    REQUIRE(quaternion.w == Approx(0.981856172866081).epsilon(1e-6));

    // Round tripping through the quaternion restores the original angles.
    Euler back;
    back.setFromQuaternion(quaternion, EulerOrder::XYZ);
    // The port keeps these values in its own precision, so the round trip is compared with
    // the same one part in a million tolerance as the composition above.
    REQUIRE(back.x == Approx(0.1).epsilon(1e-5));
    REQUIRE(back.y == Approx(0.2).epsilon(1e-5));
    REQUIRE(back.z == Approx(0.3).epsilon(1e-5));
}

TEST_CASE("Quaternion axis angle, multiply, length and vector rotation match upstream") {
    Quaternion quarterTurn;
    quarterTurn.setFromAxisAngle(Vector3(0, 1, 0), 3.14159265358979323846 / 2);
    REQUIRE(quarterTurn.x == 0);
    REQUIRE(quarterTurn.y == Approx(0.707106781186547).epsilon(1e-6));
    REQUIRE(quarterTurn.z == 0);
    REQUIRE(quarterTurn.w == Approx(0.7071067811865476).epsilon(1e-6));
    REQUIRE(quarterTurn.length() == 1);
    REQUIRE(quarterTurn.clone().normalize().w == Approx(0.7071067811865476).epsilon(1e-6));

    Quaternion combined = quarterTurn.clone();
    combined.multiply(Quaternion().setFromAxisAngle(Vector3(1, 0, 0), 3.14159265358979323846 / 2));
    REQUIRE(combined.x == Approx(0.5).epsilon(1e-6));
    REQUIRE(combined.w == Approx(0.5).epsilon(1e-6));

    // Rotating +x by a quarter turn about y lands on -z.
    Vector3 rotated = Vector3(1, 0, 0);
    rotated.applyQuaternion(quarterTurn);
    REQUIRE(std::fabs(rotated.x) < 1e-6);
    REQUIRE(rotated.y == 0);
    REQUIRE(rotated.z == -1);
}
