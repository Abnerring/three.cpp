#pragma once

// Binary64 sin/cos/tan/asin/acos/atan/atan2/exp/log/log2 from the pinned Node/V8 fdlibm
// implementation, plus the Math.pow entry point of the pinned Node release. This is
// native arithmetic; no JavaScript engine or runtime library is required.
//
// `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `exp`, `log` and `log2` are the
// fdlibm entry points of that same file, so they carry fdlibm's own argument reduction and
// polynomial kernels and do not delegate to the platform libm; `atan2` covers the
// sign-of-zero cases of ES (`atan2(-0, -0)` is `-pi`, `atan2(-0, 1)` is `-0`), and `log2`
// keeps the upstream structure, including its `k_log1p` kernel and the `ivln2hi`/`ivln2lo`
// split of 1/ln2. `exp`/`log`/`log2` cover the ES special cases: `log(+-0)` is `-Infinity`,
// `exp(-Infinity)` is `0`, and a negative `log`/`log2` argument is NaN.
//
// `pow` follows Node.js v26.5.0 (V8 14.6.202.34-node.24) with its default
// `--use-std-math-pow` behaviour: the ECMAScript special cases for NaN and infinities
// are explicit, `y === 2` is `x * x` and `y === 0.5` is `sqrt( x + 0 )`, and every other
// input falls through to the platform `std::pow`. The pinned oracle is a Windows build,
// so the general case is bit-identical to the UCRT `pow` on Windows; another platform's
// libm is a different last-bit contract for those inputs.
namespace JSNumberMath {
double sin(double value);
double cos(double value);
double tan(double x);
double asin(double x);
double acos(double value);
double atan(double x);
double atan2(double y, double x);
double exp(double x);
double log(double x);
double log2(double x);
double pow(double x, double y);
}
