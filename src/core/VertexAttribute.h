#pragma once
#include "math/Interpolant.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include <memory>

// C++ common protocol. The two JavaScript attribute classes remain unrelated.
class VertexAttribute {
  public:
    virtual ~VertexAttribute() = default;
    virtual JSValue getProperty(const std::string& name) const = 0;
    double getCount() const { return getProperty("count").toNumber(); }
    double getItemSize() const { return getProperty("itemSize").toNumber(); }
    bool getNormalized() const { return getProperty("normalized").toBoolean(); }
    JSValue getName() const { return getProperty("name"); }
    virtual InterpolantArray getArray() const = 0;
    virtual void setNeedsUpdate(JSValue value) = 0;
    virtual double getX(double index) const = 0;
    virtual double getY(double index) const = 0;
    virtual double getZ(double index) const = 0;
    virtual double getW(double index) const = 0;
    virtual VertexAttribute& setX(double index,double value) = 0;
    virtual VertexAttribute& setY(double index,double value) = 0;
    virtual VertexAttribute& setZ(double index,double value) = 0;
    virtual VertexAttribute& setW(double index,double value) = 0;
    virtual VertexAttribute& setXY(double index,double x,double y) = 0;
    virtual VertexAttribute& setXYZ(double index,double x,double y,double z) = 0;
    virtual VertexAttribute& setXYZW(double index,double x,double y,double z,double w) = 0;
    virtual VertexAttribute& applyMatrix4(const Matrix4& matrix) = 0;
    virtual VertexAttribute& applyNormalMatrix(const Matrix3& matrix) = 0;
    virtual VertexAttribute& transformDirection(const Matrix4& matrix) = 0;
    virtual std::shared_ptr<VertexAttribute> cloneAttribute(JSValue metadata = {}) const = 0;
    virtual JSValue serializeAttribute(JSValue metadata = {}) const = 0;
    virtual JsonValue toJSON() const = 0;
    virtual const JSValue* managedObjectRoot() const { return nullptr; }
};
