#include "nodes/core/PremultiplyAlpha.h"

#include "catch.hpp"

#include <cmath>

using Colour = PremultiplyAlpha::Colour;

TEST_CASE("PremultiplyAlpha multiplies the three parts by the alpha") {
    const Colour colour{1.0, 0.5, 0.25, 0.5};
    const auto out = PremultiplyAlpha::premultiply(colour);
    // Each part is scaled and the alpha is kept.
    REQUIRE(out.r == 0.5);
    REQUIRE(out.g == 0.25);
    REQUIRE(out.b == 0.125);
    REQUIRE(out.a == 0.5);
    // An opaque colour is unchanged, since multiplying by one does nothing.
    const auto opaque = PremultiplyAlpha::premultiply(Colour{0.2, 0.3, 0.4, 1.0});
    REQUIRE(opaque.r == 0.2);
    REQUIRE(opaque.g == 0.3);
    REQUIRE(opaque.b == 0.4);
    REQUIRE(opaque.a == 1.0);
}

TEST_CASE("PremultiplyAlpha needs no guard when multiplying") {
    // An alpha of nought gives nought parts without any special case, which is what makes the
    // two functions asymmetric.
    const auto out = PremultiplyAlpha::premultiply(Colour{1.0, 1.0, 1.0, 0.0});
    REQUIRE(out.r == 0.0);
    REQUIRE(out.g == 0.0);
    REQUIRE(out.b == 0.0);
    REQUIRE(out.a == 0.0);
    // The answer is finite, so nothing went wrong on the way.
    REQUIRE(std::isfinite(out.r));
}

TEST_CASE("PremultiplyAlpha answers with four noughts when the alpha is nought") {
    // Dividing by nought would give an infinity or a not a number, so that case is guarded.
    REQUIRE(PremultiplyAlpha::needsGuard(0.0));
    const auto out = PremultiplyAlpha::unpremultiply(Colour{1.0, 1.0, 1.0, 0.0});
    REQUIRE(out.r == 0.0);
    REQUIRE(out.g == 0.0);
    REQUIRE(out.b == 0.0);
    // The alpha of the answer is nought too, which is the part worth checking: it is four
    // noughts rather than nought parts beside the original alpha.
    REQUIRE(out.a == 0.0);
    // And the answer is finite, which is the point of the guard.
    REQUIRE(std::isfinite(out.r));
}

TEST_CASE("PremultiplyAlpha divides the three parts when the alpha is not nought") {
    const auto out = PremultiplyAlpha::unpremultiply(Colour{0.5, 0.25, 0.125, 0.5});
    REQUIRE(out.r == 1.0);
    REQUIRE(out.g == 0.5);
    REQUIRE(out.b == 0.25);
    REQUIRE(out.a == 0.5);
    // Only exactly nought needs the guard.
    REQUIRE_FALSE(PremultiplyAlpha::needsGuard(1e-300));
    // A tiny alpha gives huge parts rather than noughts, which is what having no guard beyond
    // exact nought means.
    const auto tiny = PremultiplyAlpha::unpremultiply(Colour{1.0, 1.0, 1.0, 1e-300});
    REQUIRE(tiny.r > 1e299);
    REQUIRE(std::isfinite(tiny.r));
}

TEST_CASE("PremultiplyAlpha's two directions are not inverses at nought") {
    // Multiplying by nought and dividing back gives nought, not the colour that went in, which
    // is why the two cannot be undone in that case.
    const Colour colour{1.0, 0.5, 0.25, 0.0};
    const auto round = PremultiplyAlpha::unpremultiply(PremultiplyAlpha::premultiply(colour));
    REQUIRE(round.r == 0.0);
    REQUIRE(round.r != colour.r);
    // But a colour with a real alpha does round trip.
    const Colour real{1.0, 0.5, 0.25, 0.5};
    const auto back = PremultiplyAlpha::unpremultiply(PremultiplyAlpha::premultiply(real));
    REQUIRE(back.r == Approx(real.r).epsilon(1e-12));
    REQUIRE(back.g == Approx(real.g).epsilon(1e-12));
    REQUIRE(back.b == Approx(real.b).epsilon(1e-12));
    REQUIRE(back.a == Approx(real.a).epsilon(1e-12));
}