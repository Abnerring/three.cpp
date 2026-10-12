#pragma once
#include "helpers/HelperUtils.h"
#include "objects/LineSegments.h"
#include "core/NativeObjectExtension.h"

class VertexNormalsHelper : public LineSegments, public NativeObjectExtension {
  public:
    std::shared_ptr<Object3D> object;
    JSValue size;
    bool isVertexNormalsHelper = true;

    explicit VertexNormalsHelper(std::shared_ptr<Object3D> object = nullptr,
                                 JSValue size = 1, HelperColor color = 0xff0000);
    virtual void update();
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    void dispose() override;
    void initializeNativeWrapper(JSValue wrapper) override;
    void visitNativeObjectReferences(const ObjectVisitor& visitor) const override;
    std::vector<const JSValue*> nativeJSReferences() const override;

  private:
    VertexNormalsHelper(std::shared_ptr<Object3D> object, JSValue size, HelperColor color,
                        std::shared_ptr<BufferGeometry> geometry);
};
