#pragma once
#include "core/JSValue.h"
#include <string>

JSValue nativeNormalModule();
JSValue nativeNormalBinding(const std::string& name);

// Port of the decisions in src/nodes/accessors/Normal.js at the pinned revision. What matters
// here is what the module does when a geometry has no normal, and a difference between two
// tests of the same question eight lines apart.
class Normal {
  public:
    static constexpr const char* kAttributeName = "normal";
    static constexpr const char* kAttributeType = "vec3";

    // The message printed when a geometry has no normal attribute.
    static std::string missingAttributeWarning();

    // The local normal to use when the geometry has none. Rather than failing, the module warns
    // and answers with a vector pointing up, so the shader still compiles and the surface reads
    // as if it faced the sky.
    struct LocalFallback {
        double x = 0, y = 1, z = 0;
    };

    static LocalFallback localFallback();

    // Which of the two sources the view space geometry normal comes from. This test is written
    // loosely upstream: any value that is true in the language's sense picks the flat normal,
    // including one that is merely present.
    static bool usesFlatNormalViewGeometry(bool flatShadingIsTruthy);

    // Whether a value is added for the world space geometry normal, which the module decides
    // with a strict comparison against true, so a builder that answers nothing does not get one.
    static bool addsWorldGeometryVarying(bool flatShadingIsTrue);
};
