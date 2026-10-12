#include "helpers/Box3Helper.h"
namespace {std::shared_ptr<BufferGeometry> createGeometry(){auto result=Helper_detail::geometry({1,1,1,-1,1,1,-1,-1,1,1,-1,1,1,1,-1,-1,1,-1,-1,-1,-1,1,-1,-1});result->setIndex(std::make_shared<BufferAttribute>(Uint16Array{0,1,1,2,2,3,3,0,4,5,5,6,6,7,7,4,0,4,1,5,2,6,3,7},1));return result;}}
Box3Helper::Box3Helper(std::shared_ptr<Box3> value,HelperColor color):LineSegments(createGeometry(),Helper_detail::lineMaterial(color.value)),box(std::move(value)){type="Box3Helper";geometry->computeBoundingSphere();}
void Box3Helper::updateMatrixWorld(bool force){const auto current=box;if(!current)throw JSTypeError("Cannot read properties of undefined (reading 'isEmpty')");if(current->isEmpty())return;current->getCenter(position);current->getSize(scale);scale.multiplyScalar(.5);LineSegments::updateMatrixWorld(force);}
std::shared_ptr<Object3D> Box3Helper::clone(bool recursive)const{auto result=std::make_shared<Box3Helper>();result->copy(*this,recursive);return result;}
void Box3Helper::dispose(){LineSegments::dispose();Helper_detail::disposeGeometry(geometry);Helper_detail::disposeMaterial(material);}
