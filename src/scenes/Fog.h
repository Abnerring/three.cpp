#pragma once
#include "core/JsonValue.h"
#include "math/Color.h"
#include <memory>
#include <string>

class FogBase {
  public:
    virtual ~FogBase() = default;
    virtual std::shared_ptr<FogBase> clone() const = 0;
    virtual JsonValue toJSON() const = 0;
};

class Fog : public FogBase {
  public:
    const bool isFog = true;
    std::string name;
    Color color;
    HIGH_PRECISION near, far;
    Fog(const Color& color, HIGH_PRECISION near = 1, HIGH_PRECISION far = 1000)
        : color(color), near(near), far(far) {}
    Fog(int color, HIGH_PRECISION near = 1, HIGH_PRECISION far = 1000)
        : Fog(Color(color), near, far) {}
    Fog(const std::string& color, HIGH_PRECISION near = 1, HIGH_PRECISION far = 1000)
        : Fog(Color(color), near, far) {}
    Fog(const char* color, HIGH_PRECISION near = 1, HIGH_PRECISION far = 1000)
        : Fog(std::string(color), near, far) {}
    std::shared_ptr<FogBase> clone() const override {
        return std::make_shared<Fog>(color, near, far);
    }
    JsonValue toJSON() const override {
        return JsonValue::Object{{"type", "Fog"},
                                 {"name", name},
                                 {"color", color.getHex()},
                                 {"near", near},
                                 {"far", far}};
    }
};
