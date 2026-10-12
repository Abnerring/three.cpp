#include "objects/Points.h"
#include "core/Raycaster.h"
#include "math/MathUtils.h"
#include "math/Frustum.h"
#include "geometries/ShapeGeometry.h"
#include "geometries/ExtrudeGeometry.h"
#include <cmath>
#include <iostream>

namespace {
thread_local Matrix4 inverseMatrix;
thread_local Ray ray;
thread_local Sphere sphere;
thread_local Vector3 pointPosition;
void testPoint(double index,double thresholdSq,const Matrix4& world,const Raycaster& raycaster,std::vector<Intersection>& intersects,Points& object) {
    const auto distanceSq=ray.distanceSqToPoint(pointPosition);
    if(distanceSq<thresholdSq) {
        Vector3 point;ray.closestPointToPoint(pointPosition,point);point.applyMatrix4(world);
        const auto distance=raycaster.ray.origin.distanceTo(point);
        if(distance<raycaster.near||distance>raycaster.far)return;
        Intersection result;result.distance=distance;result.distanceToRay=std::sqrt(distanceSq);result.point=point;
        result.index=index;result.object=&object;result.objectOwner=object.weak_from_this().lock();intersects.push_back(std::move(result));
    }
}
}

Points::Points(std::shared_ptr<BufferGeometry> geometry,std::shared_ptr<Material> material)
    : geometry(std::move(geometry)),material(std::move(material)) { isPoints=true;type="Points";updateMorphTargets(); }
Points::Points(std::shared_ptr<BufferGeometry> geometry,std::vector<std::shared_ptr<Material>> materials)
    : Points(std::move(geometry),nullptr) { this->materials=std::move(materials);materialIsArray=true; }
Points& Points::copy(const Object3D& source,bool recursive) {
    const auto& value=dynamic_cast<const Points&>(source);
    Object3D::copy(source,recursive);
    if(value.materialIsArray||!value.materials.empty()) { materials=value.materials.slice();material.reset();materialIsArray=true; }
    else { material=value.material;materials=SharedArray<std::shared_ptr<Material>>();materialIsArray=false; }
    geometry=value.geometry;
    return *this;
}
std::shared_ptr<Object3D> Points::clone(bool recursive) const {
    auto result=std::make_shared<Points>();result->copy(*this,recursive);return result;
}
bool Points::intersectsFrustum(const Frustum& frustum) { return frustum.intersectsObject(*this); }
bool Points::raycast(const Raycaster& raycaster,std::vector<Intersection>& intersects) {
    const auto current=geometry;
    if(!current)throw JSTypeError("Cannot read properties of null (reading 'drawRange')");
    const auto& world=matrixWorld;
    const auto threshold=JSValue::fromJSON(raycaster.params).get("Points").get("threshold");
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
    const auto index=current->index;const auto attribute=current->getAttribute("position");
    const auto start=MathUtils::max(0,drawRange.start);
    if(!index&&!attribute)throw JSTypeError("Cannot read properties of undefined (reading 'count')");
    const auto end=MathUtils::min(index?index->count:attribute->getCount(),drawRange.start+drawRange.count);
    for(double i=start;i<end;++i) {
        const auto a=index?index->getX(i):i;
        if(!attribute)throw JSTypeError("Cannot read properties of undefined (reading 'getX')");
        pointPosition.x=attribute->getX(a);pointPosition.y=attribute->getY(a);pointPosition.z=attribute->getZ(a);
        testPoint(a,thresholdSq,world,raycaster,intersects,*this);
    }
    return true;
}
void Points::updateMorphTargets() {
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
void Points::serializeObjectResources(Object3DJSONContext& meta,JsonValue& object) const {
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
