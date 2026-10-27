#pragma once
#include <string>

// Port of the decisions in src/nodes/accessors/Tangent.js at the pinned revision. The module
// builds the tangent in three spaces; what matters here is which source it takes the view space
// one from and when it turns it over.
class Tangent {
  public:
    // The tangent is read as four parts, not three, since its fourth carries which way the
    // frame is wound rather than a direction.
    static constexpr const char* kAttributeName = "tangent";
    static constexpr const char* kAttributeType = "vec4";

    // Which of the two sources the view space tangent comes from. A real tangent attribute is
    // used when the build is for the vertex stage or when the geometry holds the attribute;
    // otherwise the frame is worked out from derivatives, which needs no attribute.
    enum class Source { Attribute, Frame };

    static Source tangentSource(bool buildingVertexStage, bool geometryHasTangent);

    // Whether the tangent is turned over on a back facing surface. It is, unless flat shading is
    // exactly on: the test asks whether flat shading is not exactly true, so a builder that
    // answers nothing at all also turns it over.
    static bool negatesOnBackSide(bool flatShadingIsTrue);

    // The weight the local tangent is given when it is lifted into view space. Nought makes it
    // a direction, so the translation part of the matrix does not move it.
    static constexpr double kDirectionWeight = 0.0;
};

#include "core/JSValue.h"
JSValue nativeTangentModule();
JSValue nativeTangentBinding(const std::string& name);
