#include "loaders/TDSLoader.h"

#include "catch.hpp"

TEST_CASE("TDSLoader converts shininess and transparency") {
    // A raw short of five thousand becomes fifty after the reader divides by a hundred, and the
    // material multiplies it back to fifty.
    // Concretely: a percentage of half a point becomes fifty.
    REQUIRE(TDSLoader::shininessFromPercentage(0.5) == Approx(50.0).epsilon(1e-9));
    // Full shininess passes through as a hundred.
    REQUIRE(TDSLoader::shininessFromPercentage(1.0) == Approx(100.0).epsilon(1e-9));

    // A quarter transparent leaves three quarters opaque.
    REQUIRE(TDSLoader::opacityFromTransparency(0.25) == Approx(0.75).epsilon(1e-12));
    // Fully transparent is invisible.
    REQUIRE(TDSLoader::opacityFromTransparency(1.0) == 0.0);
    // Zero transparency is fully opaque.
    REQUIRE(TDSLoader::opacityFromTransparency(0.0) == 1.0);
}

TEST_CASE("TDSLoader marks a partly transparent material as transparent") {
    // The flag follows the opacity rather than the raw chunk: anything below one is transparent.
    REQUIRE(TDSLoader::opacityFromTransparency(0.5) < 1.0);
    REQUIRE_FALSE(TDSLoader::opacityFromTransparency(0.0) < 1.0);
    // A transparency that rounds away leaves the material opaque.
    REQUIRE(TDSLoader::opacityFromTransparency(0.0) == 1.0);
}

TEST_CASE("TDSLoader doubles a shininess through the percentage scale") {
    // Two successive conversions must undo each other, which is what makes the multiply and
    // the divide a pair.
    for (const double raw : {0.0, 0.25, 0.5, 1.0}) {
        REQUIRE(TDSLoader::shininessFromPercentage(raw) ==
                Approx(raw * 100.0).epsilon(1e-12));
    }
}
