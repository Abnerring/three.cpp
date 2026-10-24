#pragma once
#include "core/JSValue.h"
#include <string>

// Port of src/nodes/core/NodeVarying.js at the pinned revision. A varying is what the shader
// passes from one stage to the next: a name, a type, and two things about how it is
// interpolated. It also carries a flag saying whether it needs interpolation at all, and that
// flag is deliberately not derived from the two: a varying may name an interpolation type and
// still say it does not need one, which is what lets a builder treat it as a plain variable.
class NodeVarying {
  public:
    static constexpr bool isNodeVarying = true;

    NodeVarying(std::string name, std::string type, std::string interpolationType = {},
                std::string interpolationSampling = {})
        : name_(std::move(name)),
          type_(std::move(type)),
          interpolationType_(std::move(interpolationType)),
          interpolationSampling_(std::move(interpolationSampling)) {}

    const std::string& name() const { return name_; }
    const std::string& type() const { return type_; }

    // Whether the varying needs interpolating. It starts clear and is set by whoever knows,
    // not worked out from the two names below.
    bool needsInterpolation() const { return needsInterpolation_; }
    void setNeedsInterpolation(bool value) { needsInterpolation_ = value; }

    // The two the source takes as arguments. An absent one is the empty string rather than a
    // default name, since the source's default is null and nothing downstream compares it to a
    // word.
    const std::string& interpolationType() const { return interpolationType_; }
    const std::string& interpolationSampling() const { return interpolationSampling_; }

  private:
    std::string name_;
    std::string type_;
    std::string interpolationType_;
    std::string interpolationSampling_;
    // The flag starts clear, as the source's constructor leaves it.
    bool needsInterpolation_ = false;
};

JSValue nativeNodeVaryingConstructor();
