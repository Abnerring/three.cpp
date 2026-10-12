#include "objects/LineSegments.h"
#include <iostream>

namespace { thread_local Vector3 start,end; }
LineSegments::LineSegments(std::shared_ptr<BufferGeometry> geometry,std::shared_ptr<Material> material)
    : Line(std::move(geometry),std::move(material)) { isLineSegments=true;type="LineSegments"; }
LineSegments::LineSegments(std::shared_ptr<BufferGeometry> geometry,std::vector<std::shared_ptr<Material>> materials)
    : Line(std::move(geometry),std::move(materials)) { isLineSegments=true;type="LineSegments"; }
LineSegments& LineSegments::computeLineDistances() {
    const auto current=geometry;
    if(!current)throw JSTypeError("Cannot read properties of null (reading 'index')");
    if(!current->index) {
        const auto position=current->getAttribute("position");
        if(!position)throw JSTypeError("Cannot read properties of undefined (reading 'count')");
        std::vector<double> distances;
        for(double i=0,length=position->getCount();i<length;i+=2) {
            start.x=position->getX(i);start.y=position->getY(i);start.z=position->getZ(i);
            end.x=position->getX(i+1);end.y=position->getY(i+1);end.z=position->getZ(i+1);
            const auto previous=i==0?0:distances.back();distances.push_back(previous);distances.push_back(previous+start.distanceTo(end));
        }
        current->setAttribute("lineDistance",std::make_shared<Float32BufferAttribute>(distances,1));
    } else std::cerr<<"THREE.LineSegments.computeLineDistances(): Computation only possible with non-indexed BufferGeometry.\n";
    return *this;
}
std::shared_ptr<Object3D> LineSegments::clone(bool recursive) const {
    auto result=std::make_shared<LineSegments>();result->copy(*this,recursive);return result;
}
