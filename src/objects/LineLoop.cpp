#include "objects/LineLoop.h"

LineLoop::LineLoop(std::shared_ptr<BufferGeometry> geometry,std::shared_ptr<Material> material)
    : Line(std::move(geometry),std::move(material)) { isLineLoop=true;type="LineLoop"; }
LineLoop::LineLoop(std::shared_ptr<BufferGeometry> geometry,std::vector<std::shared_ptr<Material>> materials)
    : Line(std::move(geometry),std::move(materials)) { isLineLoop=true;type="LineLoop"; }
std::shared_ptr<Object3D> LineLoop::clone(bool recursive) const {
    auto result=std::make_shared<LineLoop>();result->copy(*this,recursive);return result;
}
