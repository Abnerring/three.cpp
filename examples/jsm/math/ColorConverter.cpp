#include "math/ColorConverter.h"
#include "math/Color.h"
#include "math/MathUtils.h"
#include "math/Vector3.h"

namespace {

// const _hsl = {}; — one record shared by every getHSV() call, so a concurrent caller never
// sees a second copy of the module state (docs/PORTING.md, module temporaries).
Vector3& hslScratch() {
    static thread_local Vector3 value;
    return value;
}

} // namespace

Color& ColorConverter::setHSV(Color& color, double h, double s, double v) {
    // https://gist.github.com/xpansive/1337890#file-index-js

    h = MathUtils::euclideanModulo(h, 1);
    s = MathUtils::clamp(s, 0, 1);
    v = MathUtils::clamp(v, 0, 1);

    const double l = (2 - s) * v;
    const double saturation = (l == 0 || l == 2) ? 0 : (s * v) / (l < 1 ? l : (2 - l));

    return color.setHSL(h, saturation, l * 0.5);
}

Vector3& ColorConverter::getHSV(const Color& color, Vector3& target) {
    Vector3& hsl = hslScratch();
    color.getHSL(hsl);

    // based on https://gist.github.com/xpansive/1337890#file-index-js
    hsl.y *= (hsl.z < 0.5) ? hsl.z : (1 - hsl.z);
    const double v = hsl.z + hsl.y;

    target.x = hsl.x;
    target.y = v == 0 ? 0 : 2 * hsl.y / v;
    target.z = v;

    return target;
}
