#pragma once
#include "core/InterleavedBuffer.h"
#include "core/VertexAttribute.h"

class InterleavedBufferAttribute : public VertexAttribute {
  public:
    InterleavedBufferAttribute(JSValue data={},JSValue itemSize={},JSValue offset={},JSValue normalized=false);
    InterleavedBufferAttribute(const InterleavedBufferAttribute& other):object_(other.object_) {}
    InterleavedBufferAttribute(InterleavedBufferAttribute&& other) noexcept:object_(std::move(other.object_)) {}
    InterleavedBufferAttribute& operator=(const InterleavedBufferAttribute& other) {object_=other.object_;return *this;}
    InterleavedBufferAttribute& operator=(InterleavedBufferAttribute&& other) noexcept {object_=std::move(other.object_);return *this;}
    static InterleavedBufferAttribute fromObject(JSValue value) {return InterleavedBufferAttribute(std::move(value),Existing{});}
    static JSValue prototype();
    JSValue object() const {return object_;}
    operator JSValue() const {return object_;}
    JSMember isInterleavedBufferAttribute{&object_,"isInterleavedBufferAttribute"},name{&object_,"name"};
    JSMember data{&object_,"data"},itemSize{&object_,"itemSize"},offset{&object_,"offset"},normalized{&object_,"normalized"};
    JSMember array{&object_,"array"},count{&object_,"count"},needsUpdate{&object_,"needsUpdate"};
    JSValue getProperty(const std::string& name) const override {return object_.get(name);}
    InterpolantArray getArray() const override {return animationArray(object_.get("array"));}
    void setNeedsUpdate(JSValue value) override {object_.set("needsUpdate",value);}
    const JSValue* managedObjectRoot() const override {return &object_;}
    JSValue getComponent(JSValue index,JSValue component) const;
    JSValue setComponent(JSValue index,JSValue component,JSValue value);
    double getX(double index) const override;
    double getY(double index) const override;
    double getZ(double index) const override;
    double getW(double index) const override;
    InterleavedBufferAttribute& setX(double index,double value) override;
    InterleavedBufferAttribute& setY(double index,double value) override;
    InterleavedBufferAttribute& setZ(double index,double value) override;
    InterleavedBufferAttribute& setW(double index,double value) override;
    InterleavedBufferAttribute& setXY(double index,double x,double y) override;
    InterleavedBufferAttribute& setXYZ(double index,double x,double y,double z) override;
    InterleavedBufferAttribute& setXYZW(double index,double x,double y,double z,double w) override;
    // const-reference matrices stay live during the call. A callback retaining
    // the managed matrix or its elements receives a final snapshot on return.
    // Use the JSValue methods with nativeMatrix(shared_ptr) for lasting identity.
    InterleavedBufferAttribute& applyMatrix4(const Matrix4& matrix) override;
    InterleavedBufferAttribute& applyNormalMatrix(const Matrix3& matrix) override;
    InterleavedBufferAttribute& transformDirection(const Matrix4& matrix) override;
    JSValue clone(JSValue metadata={}) const;
    JSValue toJSON(JSValue metadata) const;
    JsonValue toJSON() const override;
    std::shared_ptr<VertexAttribute> cloneAttribute(JSValue metadata={}) const override {return vertexAttribute(clone(metadata));}
    JSValue serializeAttribute(JSValue metadata={}) const override {return toJSON(metadata);}
  private:
    struct Existing {};
    InterleavedBufferAttribute(JSValue value,Existing):object_(std::move(value)) {}
    static void initialize(JSValue self,JSValue data,JSValue itemSize,JSValue offset,JSValue normalized);
    JSValue object_;
};
