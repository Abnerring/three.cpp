#pragma once
#include "core/Object3D.h"
#include "materials/SpriteMaterial.h"

class Sprite : public Object3D {
  public:
    std::shared_ptr<BufferGeometry> geometry;
    std::shared_ptr<Material> material;
    Vector2 center{.5,.5};
    struct Count {
        JSValue value{1};
        operator double() const { return value.toNumber(); }
        Count& operator=(const JSValue& next) { value=next;return *this; }
        Count& operator=(const Count& next) { value=next.value;return *this; }
        const JSValue& asValue() const { return value; }
    } count;
    explicit Sprite(std::shared_ptr<Material> material = std::make_shared<SpriteMaterial>());
    Sprite& copy(const Object3D& source,bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    virtual bool intersectsFrustum(const class Frustum& frustum);
    bool raycast(const Raycaster& raycaster,std::vector<Intersection>& intersects) override;
  protected:
    void serializeObjectResources(Object3DJSONContext& meta,JsonValue& object) const override;
    std::optional<JsonValue> propertyValue(const std::string& property) const override;
};
