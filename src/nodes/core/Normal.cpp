#include "nodes/core/Normal.h"

std::string Normal::missingAttributeWarning() {
    return "TSL: Vertex attribute \"normal\" not found on geometry.";
}

Normal::LocalFallback Normal::localFallback() {
    // Up, so that a surface with no normal reads as facing the sky rather than as failing.
    LocalFallback fallback;
    fallback.x = 0;
    fallback.y = 1;
    fallback.z = 0;
    return fallback;
}

bool Normal::usesFlatNormalViewGeometry(bool flatShadingIsTruthy) {
    // The loose test the module uses for the view space normal.
    return flatShadingIsTruthy;
}

bool Normal::addsWorldGeometryVarying(bool flatShadingIsTrue) {
    // The strict test it uses for the world space one, eight lines later: only exactly true
    // suppresses the varying, so a builder answering nothing still adds it.
    return !flatShadingIsTrue;
}
