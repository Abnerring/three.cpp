#include "objects/Line.h"
#include "core/Raycaster.h"
#include "math/MathUtils.h"
#include "math/Frustum.h"
#include "geometries/ShapeGeometry.h"
#include "geometries/ExtrudeGeometry.h"
#include <cmath>
#include <iostream>

namespace {
thread_local Vector3 vStart,vEnd,intersectPointOnRay,intersectPointOnSegment;
thread_local Matrix4 inverseMatrix;
thread_local Ray ray;
thread_local Sphere sphere;
std::optional<Intersection> checkIntersection(Line& object,const Raycaster& raycaster,double thresholdSq,double a,double b,double index) {
    if(!object.geometry)throw JSTypeError("Cannot read properties of null (reading 'attributes')");
    const auto position=object.geometry->getAttribute("position");
    if(!position)throw JSTypeError("Cannot read properties of undefined (reading 'getX')");
    vStart.x=position->getX(a);vStart.y=position->getY(a);vStart.z=position->getZ(a);
    vEnd.x=position->getX(b);vEnd.y=position->getY(b);vEnd.z=position->getZ(b);
    const auto distanceSq=ray.distanceSqToSegment(vStart,vEnd,&intersectPointOnRay,&intersectPointOnSegment);
    if(distanceSq>thresholdSq)return {};
    intersectPointOnRay.applyMatrix4(object.matrixWorld);
    const auto distance=raycaster.ray.origin.distanceTo(intersectPointOnRay);
    if(distance<raycaster.near||distance>raycaster.far)return {};
    Intersection result;result.distance=distance;result.point=intersectPointOnSegment.clone().applyMatrix4(object.matrixWorld);
    result.index=index;result.object=&object;result.objectOwner=object.weak_from_this().lock();return result;
}
}

Line::Line(std::shared_ptr<BufferGeometry> geometry,std::shared_ptr<Material> material)
    : geometry(std::move(geometry)),material(std::move(material)) { isLine=true;type="Line";updateMorphTargets(); }
Line::Line(std::shared_ptr<BufferGeometry> geometry,std::vector<std::shared_ptr<Material>> materials)
    : Line(std::move(geometry),nullptr) { this->materials=std::move(materials);materialIsArray=true; }
Line& Line::copy(const Object3D& source,bool recursive) {
    const auto& value=dynamic_cast<const Line&>(source);
    Object3D::copy(source,recursive);
    if(value.materialIsArray||!value.materials.empty()) { materials=value.materials.slice();material.reset();materialIsArray=true; }
    else { material=value.material;materials=SharedArray<std::shared_ptr<Material>>();materialIsArray=false; }
    geometry=value.geometry;
    return *this;
}
std::shared_ptr<Object3D> Line::clone(bool recursive) const {
    auto result=std::make_shared<Line>();result->copy(*this,recursive);return result;
}
Line& Line::computeLineDistances() {
    const auto current=geometry;
    if(!current)throw JSTypeError("Cannot read properties of null (reading 'index')");
    if(!current->index) {
        const auto position=current->getAttribute("position");
        if(!position)throw JSTypeError("Cannot read properties of undefined (reading 'count')");
        std::vector<double> distances{0};
        for(double i=1,length=position->getCount();i<length;++i) {
            vStart.x=position->getX(i-1);vStart.y=position->getY(i-1);vStart.z=position->getZ(i-1);
            vEnd.x=position->getX(i);vEnd.y=position->getY(i);vEnd.z=position->getZ(i);
            distances.push_back(distances.back()+vStart.distanceTo(vEnd));
        }
        current->setAttribute("lineDistance",std::make_shared<Float32BufferAttribute>(distances,1));
    } else std::cerr<<"THREE.Line.computeLineDistances(): Computation only possible with non-indexed BufferGeometry.\n";
    return *this;
}
bool Line::intersectsFrustum(const Frustum& frustum) { return frustum.intersectsObject(*this); }
bool Line::raycast(const Raycaster& raycaster,std::vector<Intersection>& intersects) {
    const auto current=geometry;
    if(!current)throw JSTypeError("Cannot read properties of null (reading 'drawRange')");
    const auto& world=matrixWorld;
    const auto threshold=JSValue::fromJSON(raycaster.params).get("Line").get("threshold");
    const auto& drawRange=current->drawRange;
    if(!current->boundingSphere)current->computeBoundingSphere();
    if(!current->boundingSphere)throw JSTypeError("Cannot read properties of null (reading 'center')");
    sphere.copy(*current->boundingSphere).applyMatrix4(world);
    const auto primitive=threshold.toPrimitive();
    sphere.radius=primitive.type()==JSValue::Type::String?JSValue(JSValue(sphere.radius).toString()+primitive.string()).toNumber():sphere.radius+primitive.toNumber();
    if(!raycaster.ray.intersectsSphere(sphere))return true;
    inverseMatrix.copy(world).invert();ray.copy(raycaster.ray).applyMatrix4(inverseMatrix);
    const auto localThreshold=threshold.toNumber()/((scale.x+scale.y+scale.z)/3);
    const auto thresholdSq=localThreshold*localThreshold;
    const auto step=isLineSegments?2.:1.;const auto index=current->index;const auto position=current->getAttribute("position");
    const auto start=MathUtils::max(0,drawRange.start);
    if(!index&&!position)throw JSTypeError("Cannot read properties of undefined (reading 'count')");
    const auto end=MathUtils::min(index?index->count:position->getCount(),drawRange.start+drawRange.count);
    for(double i=start,length=end-1;i<length;i+=step) {
        const auto a=index?index->getX(i):i;const auto b=index?index->getX(i+1):i+1;
        if(auto hit=checkIntersection(*this,raycaster,thresholdSq,a,b,i))intersects.push_back(std::move(*hit));
    }
    if(isLineLoop) {
        const auto a=index?index->getX(end-1):end-1;const auto b=index?index->getX(start):start;
        if(auto hit=checkIntersection(*this,raycaster,thresholdSq,a,b,end-1))intersects.push_back(std::move(*hit));
    }
    return true;
}
void Line::updateMorphTargets() {
    if(!geometry)throw JSTypeError("Cannot read properties of null (reading 'morphAttributes')");
    auto names=JSValue::object(nullptr);
    for(const auto& [key,value]:geometry->morphAttributes)names.set(key,true);
    const auto keys=names.keys();
    if(keys.empty())return;
    const auto& attributes=geometry->morphAttributes.at(keys.front());
    morphTargetInfluences=SharedArray<double>();
    morphTargetDictionary=std::make_shared<std::map<std::string,std::size_t>>();
    for(std::size_t i=0,length=attributes.size();i<length;++i) {
        if(i>=attributes.size()||!attributes[i])throw JSTypeError("Cannot read properties of undefined (reading 'name')");
        const auto attribute=attributes[i];const auto name=attribute->getName();
        const auto key=name.toBoolean()?name.toString():std::to_string(i);
        morphTargetInfluences->push_back(0);
        // The inherited __proto__ setter ignores numeric morph indices.
        if(key!="__proto__")(*morphTargetDictionary)[key]=i;
    }
}
void Line::serializeObjectResources(Object3DJSONContext& meta,JsonValue& object) const {
    if(morphTargetDictionary) {
        JsonValue dictionary=JsonValue::Object{};
        for(const auto& [name,index]:*morphTargetDictionary)dictionary[name]=index;
        object["morphTargetDictionary"]=std::move(dictionary);
    }
    if(morphTargetInfluences) {
        JsonValue::Array influences;const auto storage=morphTargetInfluences->storage();
        for(std::size_t i=0;i<storage->values.size();++i)influences.emplace_back(storage->holes.contains(i)?JsonValue():JsonValue(storage->values[i]));
        object["morphTargetInfluences"]=std::move(influences);
    }
    const auto current=geometry;
    if(!current)throw JSTypeError("Cannot read properties of null (reading 'uuid')");
    meta.cacheResource("geometries",current->uuid,[&]{return current->toJSON();});object["geometry"]=current->uuid;
    if(!geometry)throw JSTypeError("Cannot read properties of null (reading 'parameters')");
    std::optional<ShapeGeometry::Shapes> shapes;
    if(const auto shape=std::dynamic_pointer_cast<ShapeGeometry>(geometry))shapes=shape->parameters.shapes;
    else if(const auto extrude=std::dynamic_pointer_cast<ExtrudeGeometry>(geometry))shapes=extrude->parameters.shapes;
    if(shapes) {
        const auto serialize=[&](const std::shared_ptr<Shape>& shape){
            if(!shape)throw JSTypeError("Cannot read properties of null (reading 'uuid')");
            meta.cacheResource("shapes",shape->uuid,[&]{return shape->toJSON();});
        };
        if(const auto single=std::get_if<std::shared_ptr<Shape>>(&*shapes))serialize(*single);
        else {
            const auto list=std::get<std::shared_ptr<ShapeGeometry::ShapeList>>(*shapes);
            if(!list)throw JSTypeError("Cannot read properties of null (reading 'length')");
            for(std::size_t i=0,length=list->size();i<length;++i){
                if(i>=list->size())throw JSTypeError("Cannot read properties of undefined (reading 'uuid')");
                const auto shape=(*list)[i];serialize(shape);
            }
        }
    }
    const auto serializeMaterial=[&](const std::shared_ptr<Material>& entry){
        if(!entry)throw JSTypeError("Cannot read properties of null (reading 'uuid')");
        meta.cacheResource("materials",entry->uuid,[&]{return entry->toJSON(&meta);});return entry->uuid;
    };
    if(materialIsArray||!materials.empty()) {
        JsonValue::Array ids;
        for(std::size_t i=0,length=materials.size();i<length;++i) {
            if(i>=materials.size()||materials.storage()->holes.contains(i))throw JSTypeError("Cannot read properties of undefined (reading 'uuid')");
            const auto entry=materials[i];ids.emplace_back(serializeMaterial(entry));
        }
        object["material"]=std::move(ids);
    } else { const auto entry=material;object["material"]=serializeMaterial(entry); }
}
