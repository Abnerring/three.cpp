#include "nodes/core/ToonOutlinePass.h"

#include <cmath>

ToonOutlinePass::Direction ToonOutlinePass::direction(double fromX, double fromY, double fromZ,
                                                     double toX, double toY, double toZ) {
    // The second position taken from the first, in that order.
    Direction out;
    out.x = fromX - toX;
    out.y = fromY - toY;
    out.z = fromZ - toZ;
    const double length = std::sqrt(out.x * out.x + out.y * out.y + out.z * out.z);
    if (length > 0.0) {
        out.x /= length;
        out.y /= length;
        out.z /= length;
    }
    return out;
}

double ToonOutlinePass::push(double thickness, double clipW) {
    // The thickness scaled by the clip space w.
    return thickness * clipW;
}

double ToonOutlinePass::thicknessRatio() {
    // One, with a note that a per vertex ratio is not supported yet.
    return 1.0;
}

bool ToonOutlinePass::outlines(bool isToonMaterial, bool isToonNodeMaterial,
                               bool wireframeIsTrue) {
    // Either toon kind, and not a wireframe.
    if (!(isToonMaterial || isToonNodeMaterial)) return false;
    return !wireframeIsTrue;
}
