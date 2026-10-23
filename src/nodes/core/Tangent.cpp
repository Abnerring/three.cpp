#include "nodes/core/Tangent.h"

Tangent::Source Tangent::tangentSource(bool buildingVertexStage, bool geometryHasTangent) {
    // The attribute is used when the build is for the vertex stage, since the attribute is
    // available there, or when the geometry actually holds one.
    if (buildingVertexStage || geometryHasTangent) return Source::Attribute;
    return Source::Frame;
}

bool Tangent::negatesOnBackSide(bool flatShadingIsTrue) {
    // The source asks whether flat shading is not exactly true, so anything but a plain true
    // turns the tangent over, including a builder that answers nothing.
    return !flatShadingIsTrue;
}
