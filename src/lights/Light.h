#pragma once
#include "core/Object3D.h"
#include "core/JSValue.h"
#include "math/Color.h"

template <class LightType> struct LightPower {
    LightType* owner;
    operator double() const { return owner->getPower(); }
    double operator=(double value) { owner->setPower(value); return value; }
    double operator=(const LightPower& value) { return *this = static_cast<double>(value); }
    double operator+=(double value) { return *this = static_cast<double>(*this) + value; }
    double operator-=(double value) { return *this = static_cast<double>(*this) - value; }
    double operator*=(double value) { return *this = static_cast<double>(*this) * value; }
    double operator/=(double value) { return *this = static_cast<double>(*this) / value; }
};

class Light : public Object3D {
  public:
    Color color;
    double intensity;
    Light(const Color& color = Color(), double intensity = 1);
    static void registerNode(const JSValue& nodeClass);
    virtual JSValue getLightNode() const;
    virtual Object3D* shadowTarget() const { return nullptr; }
    Light& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    using Object3D::toJSON;
    JsonValue toJSON(Object3DJSONContext* meta = nullptr) const override;
  protected:
    static void registerNodeFor(const std::string& className, const JSValue& nodeClass);
    static std::optional<JSValue> registeredNodeFor(const std::string& className);
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
