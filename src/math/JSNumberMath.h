#pragma once

// Binary64 sin/cos/acos from the pinned Node/V8 fdlibm implementation, plus the
// Math.pow entry point of the pinned Node release. This is native arithmetic; no
// JavaScript engine or runtime library is required.
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
double acos(double value);
double pow(double x, double y);
}
