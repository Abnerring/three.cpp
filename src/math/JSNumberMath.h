#pragma once

// Binary64 sin/cos/acos from the pinned Node/V8 fdlibm implementation. This is
// native arithmetic; no JavaScript engine or runtime library is required.
namespace JSNumberMath {
double sin(double value);
double cos(double value);
double acos(double value);
}
