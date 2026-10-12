#include "helpers/PointLightHelper.h"
#include "geometries/SphereGeometry.h"
namespace{std::shared_ptr<MeshBasicMaterial> createMaterial(){auto result=Helper_detail::meshMaterial();result->wireframe=true;result->fog=false;return result;}}
PointLightHelper::PointLightHelper(std::shared_ptr<PointLight> light,double sphereSize,HelperColor color):Mesh(std::make_shared<SphereGeometry>(sphereSize,4,2),createMaterial()),light(std::move(light)),color(std::move(color.value)){type="PointLightHelper";if(!this->light)throw JSTypeError("Cannot read properties of undefined (reading 'matrixWorld')");matrix.aliasTo(std::shared_ptr<Matrix4>(this->light,&this->light->matrixWorld),this->light);matrixAutoUpdate=false;update();}
void PointLightHelper::update(){matrixWorldNeedsUpdate=true;if(!light)throw JSTypeError("light is undefined");{const auto retained=light;retained->updateWorldMatrix(true,false);}if(!color.isUndefined())Helper_detail::setMeshColor(material,color);else Helper_detail::meshColor(material).copy(light->color);}
std::shared_ptr<Object3D> PointLightHelper::clone(bool)const{return std::make_shared<PointLightHelper>(nullptr);}
void PointLightHelper::dispose(){Mesh::dispose();Helper_detail::disposeGeometry(geometry);Helper_detail::disposeMaterial(material);}
