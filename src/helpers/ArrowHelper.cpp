#include "helpers/ArrowHelper.h"
#include "geometries/ConeGeometry.h"
#include "math/MathUtils.h"
#include "math/JSNumberMath.h"
namespace {thread_local Vector3 axis;thread_local std::shared_ptr<BufferGeometry> lineGeometry,coneGeometry;}
ArrowHelper::ArrowHelper(const Vector3& dir,const Vector3& origin,double length,HelperColor color,std::optional<double> headLength,std::optional<double> headWidth) {
    type="ArrowHelper";
    if(!lineGeometry){lineGeometry=Helper_detail::geometry({0,0,0,0,1,0});coneGeometry=std::make_shared<ConeGeometry>(.5,1,5,1);coneGeometry->translate(0,-.5,0);}
    position.copy(origin);line=std::make_shared<Line>(lineGeometry,Helper_detail::lineMaterial(color.value));line->matrixAutoUpdate=false;add(line);
    cone=std::make_shared<Mesh>(coneGeometry,Helper_detail::meshMaterial(color.value));cone->matrixAutoUpdate=false;add(cone);
    setDirection(dir);setLength(length,headLength,headWidth);
}
void ArrowHelper::setDirection(const Vector3& dir) {
    if(dir.y>.99999)quaternion.set(0,0,0,1);
    else if(dir.y<-.99999)quaternion.set(1,0,0,0);
    else{axis.set(dir.z,0,-dir.x).normalize();const double radians=JSNumberMath::acos(dir.y);quaternion.setFromAxisAngle(axis,radians);}
}
void ArrowHelper::setLength(double length,std::optional<double> requestedHeadLength,std::optional<double> requestedHeadWidth) {
    const auto headLength=requestedHeadLength.value_or(length*.2),headWidth=requestedHeadWidth.value_or(headLength*.2);
    line->scale.set(1,MathUtils::max(.0001,length-headLength),1);line->updateMatrix();
    cone->scale.set(headWidth,headLength,headWidth);cone->position.y=length;cone->updateMatrix();
}
void ArrowHelper::setColor(HelperColor color){Helper_detail::setLineColor(line->material,color.value);Helper_detail::setMeshColor(cone->material,color.value);}
ArrowHelper& ArrowHelper::copy(const Object3D& object,bool){Object3D::copy(object,false);const auto* source=dynamic_cast<const ArrowHelper*>(&object);if(!source)throw JSTypeError("ArrowHelper source.line is undefined");line->copy(*source->line);cone->copy(*source->cone);return *this;}
std::shared_ptr<Object3D> ArrowHelper::clone(bool recursive)const{auto result=std::make_shared<ArrowHelper>();result->copy(*this,recursive);return result;}
void ArrowHelper::dispose(){Object3D::dispose();Helper_detail::disposeGeometry(line->geometry);Helper_detail::disposeMaterial(line->material);Helper_detail::disposeGeometry(cone->geometry);Helper_detail::disposeMaterial(cone->material);}
