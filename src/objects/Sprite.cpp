#include "objects/Sprite.h"
#include "math/JSNumberMath.h"
#include "core/InterleavedBuffer.h"
#include "math/JSNumberMath.h"
#include "core/InterleavedBufferAttribute.h"
#include "math/JSNumberMath.h"
#include "core/NativeObject.h"
#include "math/JSNumberMath.h"
#include "core/Raycaster.h"
#include "math/JSNumberMath.h"
#include "cameras/PerspectiveCamera.h"
#include "math/JSNumberMath.h"
#include "math/Triangle.h"
#include "math/JSNumberMath.h"
#include "math/Frustum.h"
#include "math/JSNumberMath.h"
#include <cmath>
#include <iostream>

namespace {
thread_local std::shared_ptr<BufferGeometry> sharedGeometry;
thread_local Vector3 intersectPoint,worldScale,mvPosition,vA,vB,vC;
thread_local Vector2 alignedPosition,rotatedPosition,uvA,uvB,uvC;
thread_local Matrix4 viewWorldMatrix;
void transformVertex(Vector3& vertex,const Vector2& center,const std::optional<double>& sin,const std::optional<double>& cos) {
    alignedPosition.x=(vertex.x-center.x+.5)*worldScale.x;
    alignedPosition.y=(vertex.y-center.y+.5)*worldScale.y;
    if(sin) {
        rotatedPosition.x=*cos*alignedPosition.x-*sin*alignedPosition.y;
        rotatedPosition.y=*sin*alignedPosition.x+*cos*alignedPosition.y;
    } else rotatedPosition.copy(alignedPosition);
    vertex.copy(mvPosition);vertex.x+=rotatedPosition.x;vertex.y+=rotatedPosition.y;
    vertex.applyMatrix4(viewWorldMatrix);
}
}
Sprite::Sprite(std::shared_ptr<Material> material) : material(std::move(material)) {
    isSprite=true;type="Sprite";
    if(!sharedGeometry) {
        sharedGeometry=std::make_shared<BufferGeometry>();
        InterleavedBuffer buffer(Float32Array{-.5f,-.5f,0,0,0,.5f,-.5f,0,1,0,.5f,.5f,0,1,1,-.5f,.5f,0,0,1},5);
        sharedGeometry->setIndex(std::vector<std::uint32_t>{0,1,2,0,2,3});
        sharedGeometry->setAttribute("position",std::make_shared<InterleavedBufferAttribute>(buffer,3,0,false));
        sharedGeometry->setAttribute("uv",std::make_shared<InterleavedBufferAttribute>(buffer,2,3,false));
    }
    geometry=sharedGeometry;
}
Sprite& Sprite::copy(const Object3D& source,bool recursive) {
    const auto& value=dynamic_cast<const Sprite&>(source);
    Object3D::copy(source,recursive);center.copy(value.center);material=value.material;return *this;
}
std::shared_ptr<Object3D> Sprite::clone(bool recursive) const {
    auto result=std::make_shared<Sprite>();result->copy(*this,recursive);return result;
}
bool Sprite::intersectsFrustum(const Frustum& frustum) { return frustum.intersectsSprite(*this); }
bool Sprite::raycast(const Raycaster& raycaster,std::vector<Intersection>& intersects) {
    if(!raycaster.camera)std::cerr<<"THREE.Sprite: \"Raycaster.camera\" needs to be set in order to raycast against sprites.\n";
    worldScale.setFromMatrixScale(matrixWorld);
    if(!raycaster.camera)throw JSTypeError("Cannot read properties of null (reading 'matrixWorld')");
    viewWorldMatrix.copy(raycaster.camera->matrixWorld);
    modelViewMatrix.multiplyMatrices(raycaster.camera->matrixWorldInverse,matrixWorld);
    mvPosition.setFromMatrixPosition(modelViewMatrix);
    if(dynamic_cast<PerspectiveCamera*>(raycaster.camera)&&nativeMaterial(material).get("sizeAttenuation").strictEquals(JSValue(false)))
        worldScale.multiplyScalar(-mvPosition.z);
    const auto rotation=nativeMaterial(material).get("rotation");
    std::optional<double> sin,cos;
    if(!rotation.strictEquals(JSValue(0))) { cos=JSNumberMath::cos(rotation.toNumber());sin=JSNumberMath::sin(rotation.toNumber()); }
    transformVertex(vA.set(-.5,-.5,0),center,sin,cos);
    transformVertex(vB.set(.5,-.5,0),center,sin,cos);
    transformVertex(vC.set(.5,.5,0),center,sin,cos);
    uvA.set(0,0);uvB.set(1,0);uvC.set(1,1);
    auto intersection=raycaster.ray.intersectTriangle(vA,vB,vC,false,intersectPoint);
    if(!intersection) {
        transformVertex(vB.set(-.5,.5,0),center,sin,cos);uvB.set(0,1);
        intersection=raycaster.ray.intersectTriangle(vA,vC,vB,false,intersectPoint);
        if(!intersection)return true;
    }
    const auto distance=raycaster.ray.origin.distanceTo(intersectPoint);
    if(distance<raycaster.near||distance>raycaster.far)return true;
    Intersection result;result.distance=distance;result.point=intersectPoint;
    Vector2 uv;if(Triangle::getInterpolation(intersectPoint,vA,vB,vC,uvA,uvB,uvC,uv))result.uv=uv;
    result.object=this;result.objectOwner=weak_from_this().lock();intersects.push_back(std::move(result));return true;
}
void Sprite::serializeObjectResources(Object3DJSONContext& meta,JsonValue& object) const {
    const auto current=material;
    if(!current)throw JSTypeError("Cannot read properties of null (reading 'uuid')");
    meta.cacheResource("materials",current->uuid,[&]{return current->toJSON(&meta);});object["material"]=current->uuid;
}
std::optional<JsonValue> Sprite::propertyValue(const std::string& property) const {
    if(property=="count")return count.asValue().toJSON();
    return Object3D::propertyValue(property);
}
