#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <vector>

// Native port of examples/jsm/math/SimplexNoise.js from the pinned three.js revision (see
// upstream.json).
//
// The source class is constructed with `new SimplexNoise( r = Math )` and reads the
// permutation table from `r.random()`:
//
//     for ( let i = 0; i < 256; i ++ ) this.p[ i ] = Math.floor( r.random() * 256 );
//
// `Math` itself has no native counterpart, so the constructor takes a callable instead:
// `RandomSource` is a function returning one binary64 number in [ 0, 1 ), and an empty
// `std::function` selects the native default source `MathUtils::seededRandom()` (the
// deterministic pseudo-random generator of `src/math/MathUtils.h`). That replacement keeps
// the upstream rule — every table entry is `std::floor( random() * 256 )`, evaluated once
// per index in ascending order — and makes the default instance reproducible in native code
// where `Math.random()` is not. Injecting an explicit generator reproduces an upstream
// instance exactly, which is how the differential test builds both sides from the same
// sequence.
//
// The upstream data members keep their numeric types (`Number` is binary64, and `p` / `perm`
// hold the floored results of the product above):
//
//     grad3   : [ 12 ][ 3 ]  ->  std::array< std::array< double, 3 >, 12 >
//     grad4   : [ 32 ][ 4 ]  ->  std::array< std::array< double, 4 >, 32 >
//     p       : [ 256 ]      ->  std::vector< double >
//     perm    : [ 512 ]      ->  std::vector< double >
//     simplex : [ 64 ][ 4 ]  ->  std::array< std::array< double, 4 >, 64 >
//
// The members stay public with their upstream names, so `perm[ i ] = p[ i & 255 ]` can be
// checked directly. `noise()` / `noise3d()` / `noise4d()` use `std::sqrt()` for `Math.sqrt()`
// (both are correctly rounded, IEEE 754 square root) and keep the source's evaluation order;
// `& 255` applies the ECMAScript ToInt32 conversion to the floored coordinate before the mask,
// which also keeps every table index inside `perm`.
class SimplexNoise {
  public:
    /** A function returning one random number in [ 0, 1 ), or empty for the native default. */
    using RandomSource = std::function<double()>;

    /** A lookup table of the twelve 2D/3D gradients. */
    std::array<std::array<double, 3>, 12> grad3;

    /** A lookup table of the thirty-two 4D gradients. */
    std::array<std::array<double, 4>, 32> grad4;

    /** The permutation table of 256 random values in [ 0, 255 ]. */
    std::vector<double> p;

    /** The doubled permutation table that removes the need for index wrapping. */
    std::vector<double> perm;

    /** A lookup table to traverse the simplex around a given point in 4D. */
    std::array<std::array<double, 4>, 64> simplex;

    /** Constructs a new simplex noise object that fills `p` from the given random source. */
    explicit SimplexNoise(RandomSource random = {});

    /** A 2D simplex noise method. */
    double noise(double xin, double yin) const;

    /** A 3D simplex noise method. */
    double noise3d(double xin, double yin, double zin) const;

    /** A 4D simplex noise method. */
    double noise4d(double x, double y, double z, double w) const;
};
