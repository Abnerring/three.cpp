#include "objects/Line.h"
#include "objects/LineLoop.h"
#include "objects/LineSegments.h"
#include "objects/Points.h"
#include "objects/Sprite.h"
#include "core/InterleavedBufferAttribute.h"
#include "core/Raycaster.h"
#include "core/NativeObject.h"
#include "animation/PropertyBinding.h"
#include "cameras/PerspectiveCamera.h"
#include "cameras/OrthographicCamera.h"
#include "math/Frustum.h"
#include "math/Box3.h"
#include <iostream>
#include <functional>

namespace {
using Args=std::vector<JSValue>;
JsonValue number(double n){if(std::isnan(n))return "$NaN";if(std::isinf(n))return n>0?"$Infinity":"$-Infinity";if(n==0&&std::signbit(n))return "$-0";return n;}
template<class Values>JsonValue numbers(const Values& a){JsonValue::Array out;for(auto n:a)out.push_back(number(double(n)));return out;}
double readNumber(JSValue value){if(value.type()==JSValue::Type::String){if(value.string()=="$Infinity")return INFINITY;if(value.string()=="$-Infinity")return -INFINITY;if(value.string()=="$NaN")return NAN;if(value.string()=="$-0")return -0.;}return value.toNumber();}
std::vector<double> values(JSValue a){std::vector<double> result;for(std::uint32_t i=0;i<a.length();++i)result.push_back(readNumber(a.get(std::to_string(i))));return result;}
Vector3 vector(JSValue a,const Vector3& fallback={}){return a.isUndefined()?fallback:Vector3(readNumber(a.get("0")),readNumber(a.get("1")),readNumber(a.get("2")));}
std::shared_ptr<BufferGeometry> geometry(const JSValue& r){
    auto g=std::make_shared<BufferGeometry>();g->uuid="geometry";
    if(!r.get("missing").toBoolean())g->setAttribute("position",std::make_shared<Float32BufferAttribute>(values(r.get("positions")),3));
    if(!r.get("indices").isUndefined())g->index=std::make_shared<Float32BufferAttribute>(values(r.get("indices")),1);
    if(!r.get("start").isUndefined())g->drawRange.start=readNumber(r.get("start"));
    if(!r.get("count").isUndefined())g->drawRange.count=readNumber(r.get("count"));
    if(r.get("largeSphere").toBoolean())g->boundingSphere=Sphere(Vector3(),100);
    return g;
}
std::shared_ptr<Object3D> create(const std::string& name,std::shared_ptr<BufferGeometry> g={}){
    if(name=="Sprite")return std::make_shared<Sprite>();if(!g)g=std::make_shared<BufferGeometry>();
    if(name=="LineLoop")return std::make_shared<LineLoop>(g);if(name=="LineSegments")return std::make_shared<LineSegments>(g);
    if(name=="Line")return std::make_shared<Line>(g);return std::make_shared<Points>(g);
}
std::shared_ptr<BufferGeometry>& objectGeometry(Object3D& o){if(auto p=dynamic_cast<Line*>(&o))return p->geometry;if(auto p=dynamic_cast<Points*>(&o))return p->geometry;return dynamic_cast<Sprite&>(o).geometry;}
std::shared_ptr<Material>& objectMaterial(Object3D& o){if(auto p=dynamic_cast<Line*>(&o))return p->material;if(auto p=dynamic_cast<Points*>(&o))return p->material;return dynamic_cast<Sprite&>(o).material;}
JsonValue morph(Object3D& o){
    if(dynamic_cast<Sprite*>(&o))return nullptr;
    const auto influences=dynamic_cast<Line*>(&o)?dynamic_cast<Line&>(o).morphTargetInfluences:dynamic_cast<Points&>(o).morphTargetInfluences;
    const auto dictionary=dynamic_cast<Line*>(&o)?dynamic_cast<Line&>(o).morphTargetDictionary:dynamic_cast<Points&>(o).morphTargetDictionary;
    JsonValue result=JsonValue::Object{{"influences","$undefined"},{"dictionary","$undefined"}};
    if(influences)result["influences"]=numbers(*influences);
    if(dictionary){JsonValue d=JsonValue::Object{};for(const auto& [k,v]:*dictionary)d[k]=v;result["dictionary"]=d;}return result;
}
JsonValue hits(const std::vector<Intersection>& intersections,Object3D& owner){
    JsonValue::Array out;
    for(const auto& h:intersections)out.emplace_back(JsonValue::Object{{"distance",number(h.distance)},{"point",numbers(h.point.toArray())},
        {"index",h.index?number(*h.index):JsonValue("$undefined")},{"distanceToRay",h.distanceToRay?number(*h.distanceToRay):JsonValue("$undefined")},
        {"uv",h.uv?numbers(h.uv->toArray()):JsonValue("$undefined")},{"object",h.object==&owner},{"ownsObject",h.objectOwner.get()==&owner}});
    return out;
}
struct CallbackAttribute:Float32BufferAttribute {
    mutable std::function<void(double)> callback;
    explicit CallbackAttribute(const std::vector<double>& p):Float32BufferAttribute(p,3){}
    double getX(double i)const override{if(callback)callback(i);return Float32BufferAttribute::getX(i);}
};
JsonValue rays(const JSValue& r){
    const auto name=r.get("type").string();auto g=geometry(r);auto object=create(name,g);object->uuid="object";
    object->position.copy(vector(r.get("position")));object->scale.copy(vector(r.get("scale"),Vector3(1,1,1)));
    object->rotation.setFromVector3(vector(r.get("rotation")));
    auto parent=std::make_shared<Object3D>();parent->scale.copy(vector(r.get("parentScale"),Vector3(1,1,1)));parent->add(object);parent->updateMatrixWorld(true);
    Raycaster ray(vector(r.get("origin"),Vector3(0,0,3)),vector(r.get("direction"),Vector3(0,0,-1)),r.get("near").isUndefined()?0:readNumber(r.get("near")),r.get("far").isUndefined()?INFINITY:readNumber(r.get("far")));
    if(!r.get("threshold").isUndefined())ray.params[name=="Points"?"Points":"Line"]["threshold"]=r.get("threshold").toJSON().value();
    std::shared_ptr<Camera> camera;
    if(name=="Sprite") {
        if(r.get("camera").string()!="none") {
            camera=r.get("camera").string()=="orthographic"?std::shared_ptr<Camera>(std::make_shared<OrthographicCamera>(-2,2,2,-2,.1,30)):std::shared_ptr<Camera>(std::make_shared<PerspectiveCamera>(50,1,.1,30));
            camera->position.copy(vector(r.get("cameraPosition")));camera->rotation.setFromVector3(vector(r.get("cameraRotation")));camera->updateMatrixWorld(true);ray.camera=camera.get();
        }
        auto sprite=std::dynamic_pointer_cast<Sprite>(object);sprite->center.set(readNumber(r.get("center").get("0")),readNumber(r.get("center").get("1")));
        auto material=std::dynamic_pointer_cast<SpriteMaterial>(sprite->material);material->rotation=readNumber(r.get("spriteRotation"));material->sizeAttenuation=r.get("attenuation").toBoolean();
    }
    JsonValue::Array log;std::vector<Intersection> nested;
    if(r.get("reenter").toBoolean()||r.get("swap").toBoolean()) {
        auto attribute=std::make_shared<CallbackAttribute>(values(r.get("positions")));g->setAttribute("position",attribute);g->boundingSphere=Sphere(Vector3(),100);
        auto other=create(name,geometry(r));other->position.set(2,1,0);other->updateMatrixWorld(true);
        auto replacement=geometry(r);replacement->setAttribute("position",std::make_shared<Float32BufferAttribute>(std::vector<double>{-1,0,-2,1,0,-2,0,1,-2},3));
        auto entered=std::make_shared<bool>(false);
        attribute->callback=[&,other,replacement,entered](double i){log.emplace_back(number(i));if(!*entered){*entered=true;if(r.get("swap").toBoolean())objectGeometry(*object)=replacement;else other->raycast(ray,nested);}};
    }
    std::vector<Intersection> intersections;std::string error;
    try{object->raycast(ray,intersections);}catch(const JSTypeError&){error="TypeError";}catch(const std::exception&){error="Error";}
    auto out=JsonValue::Object{{"error",error},{"hits",hits(intersections,*object)},{"log",log}};
    if(name=="Sprite")out["modelView"]=numbers(object->modelViewMatrix.elements());
    return out;
}
JsonValue distances(const JSValue& r){
    auto g=geometry(r);auto line=std::dynamic_pointer_cast<Line>(create(r.get("type").string(),g));
    auto old=std::make_shared<Float32BufferAttribute>(std::vector<double>{17},1);if(r.get("old").toBoolean())g->setAttribute("lineDistance",old);
    std::string error;bool self=false;try{self=&line->computeLineDistances()==line.get();}catch(const JSTypeError&){error="TypeError";}
    const auto distance=g->getAttribute("lineDistance");JsonValue::Array data;if(distance)for(double i=0;i<distance->getCount();++i)data.push_back(number(distance->getX(i)));
    return JsonValue::Object{{"error",error},{"self",self},{"present",bool(distance)},{"sameOld",distance==old},{"data",data}};
}
JsonValue copying(const JSValue& r){
    const auto name=r.get("type").string();auto g=std::make_shared<BufferGeometry>();g->uuid="geometry";
    auto a=std::make_shared<Float32BufferAttribute>(std::vector<double>{0,0,0},3);a->name="target";g->morphAttributes["position"]={a};
    auto source=create(name,g);source->uuid="object";source->name="source";
    source->add(std::make_shared<Object3D>());source->children[0]->uuid="child";
    objectMaterial(*source)->uuid="material";
    auto target=create(name,g);target->uuid="target";target->add(std::make_shared<Object3D>());
    if(auto line=dynamic_cast<Line*>(source.get())) {
        (*line->morphTargetInfluences)[0]=.8;(*dynamic_cast<Line&>(*target).morphTargetInfluences)[0]=.3;
        if(r.get("array").toBoolean()){line->materials={line->material,line->material};line->material.reset();line->materialIsArray=true;}
    } else if(auto points=dynamic_cast<Points*>(source.get())) {
        (*points->morphTargetInfluences)[0]=.8;(*dynamic_cast<Points&>(*target).morphTargetInfluences)[0]=.3;
        if(r.get("array").toBoolean()){points->materials={points->material,points->material};points->material.reset();points->materialIsArray=true;}
    } else {
        auto& sprite=dynamic_cast<Sprite&>(*source);sprite.geometry=g;sprite.center.set(.2,.8);sprite.count=9;
        dynamic_cast<Sprite&>(*target).geometry=std::make_shared<BufferGeometry>();dynamic_cast<Sprite&>(*target).count=7;
    }
    const auto targetGeometry=objectGeometry(*target);const auto defaultGeometry=objectGeometry(*create(name));
    const bool recursive=r.get("recursive").toBoolean();target->copy(*source,recursive);auto clone=source->clone(recursive);clone->uuid="clone";
    if(!clone->children.empty())clone->children[0]->uuid="clone-child";
    JsonValue out=JsonValue::Object{{"sourceMorph",morph(*source)},{"targetMorph",morph(*target)},{"cloneMorph",morph(*clone)},
        {"targetGeometrySame",objectGeometry(*target)==objectGeometry(*source)},{"targetGeometryPreserved",objectGeometry(*target)==targetGeometry},
        {"cloneGeometrySame",objectGeometry(*clone)==objectGeometry(*source)},{"cloneDefaultGeometry",objectGeometry(*clone)==defaultGeometry},
        {"cloneType",clone->type},{"targetChildren",target->children.size()},{"cloneChildren",clone->children.size()},
        {"materialShared",objectMaterial(*clone)==objectMaterial(*source)},{"sourceJSON",source->toJSON()},{"cloneJSON",clone->toJSON()}};
    if(auto line=dynamic_cast<Line*>(source.get()))out["arrayCopied"]=!line->materials.sameIdentity(dynamic_cast<Line&>(*clone).materials);
    else if(auto points=dynamic_cast<Points*>(source.get()))out["arrayCopied"]=!points->materials.sameIdentity(dynamic_cast<Points&>(*clone).materials);
    else {out["center"]=numbers(dynamic_cast<Sprite&>(*clone).center.toArray());out["cloneCount"]=double(dynamic_cast<Sprite&>(*clone).count);out["targetCount"]=double(dynamic_cast<Sprite&>(*target).count);}
    return out;
}
JsonValue bounds(const JSValue& r) {
    auto object=create(r.get("type").string(),geometry(r));object->position.copy(vector(r.get("position")));object->scale.copy(vector(r.get("scale"),Vector3(1,1,1)));object->rotation.setFromVector3(vector(r.get("rotation")));
    if(auto sprite=std::dynamic_pointer_cast<Sprite>(object))sprite->center.set(readNumber(r.get("center").get("0")),readNumber(r.get("center").get("1")));
    object->updateMatrixWorld(true);const Frustum frustum=Frustum().setFromProjectionMatrix(Matrix4().makePerspective(-1,1,1,-1,1,10));
    bool visible;if(auto line=std::dynamic_pointer_cast<Line>(object))visible=line->intersectsFrustum(frustum);else if(auto points=std::dynamic_pointer_cast<Points>(object))visible=points->intersectsFrustum(frustum);else visible=std::dynamic_pointer_cast<Sprite>(object)->intersectsFrustum(frustum);
    Box3 box;box.setFromObject(*object,r.get("precise").toBoolean());return JsonValue::Object{{"visible",visible},{"min",numbers(box.min.toArray())},{"max",numbers(box.max.toArray())}};
}
struct SerializationGeometry:BufferGeometry {mutable std::function<void()> callback;JsonValue toJSON()const override{if(callback)callback();return BufferGeometry::toJSON();}};
struct SerializationMaterial:LineBasicMaterial {mutable std::function<void()> callback;JsonValue toJSON(TextureJSONContext* meta=nullptr)const override{if(callback)callback();return LineBasicMaterial::toJSON(meta);}};
JsonValue serialization(const JSValue& r) {
    auto geometry=std::make_shared<SerializationGeometry>();geometry->uuid="geometry";auto object=create(r.get("type").string(),geometry);object->uuid="object";
    auto first=std::make_shared<SerializationMaterial>();first->uuid="first";auto second=std::make_shared<LineBasicMaterial>();second->uuid="second";objectMaterial(*object)=first;
    JsonValue::Array log;
    geometry->callback=[&]{log.emplace_back("geometry");objectMaterial(*object)=second;};
    const auto single=object->toJSON();objectMaterial(*object).reset();geometry->callback={};
    if(auto line=std::dynamic_pointer_cast<Line>(object)){line->materials={first,first};line->materialIsArray=true;first->callback=[&,line]{log.emplace_back("material");line->materials[1]=second;};}
    else {auto points=std::dynamic_pointer_cast<Points>(object);points->materials={first,first};points->materialIsArray=true;first->callback=[&,points]{log.emplace_back("material");points->materials[1]=second;};}
    const auto multiple=object->toJSON();first->callback={};
    return JsonValue::Object{{"single",single},{"multiple",multiple},{"log",log}};
}
JsonValue bindings(const JSValue& r) {
    const auto name=r.get("type").string();auto g=std::make_shared<BufferGeometry>();
    auto a=std::make_shared<Float32BufferAttribute>(std::vector<double>{0,0,0},3);a->name="smile";g->morphAttributes["position"]={a};
    auto object=create(name,g);auto wrapper=nativeObject(object);
    const auto material=wrapper.get("material");PropertyBinding opacity(wrapper,".material.opacity");
    opacity.setValue(AnimationArray::array({.25}),0);
    JsonValue out=JsonValue::Object{{"opacity",material.get("opacity").toNumber()},{"version",material.get("version").toNumber()},
        {"flags",JsonValue::Array{object->isMesh,object->isLine,object->isLineLoop,object->isLineSegments,object->isPoints,object->isSprite}},
        {"type",object->type},{"materialType",objectMaterial(*object)->type}};
    const auto replacement=std::make_shared<SpriteMaterial>();wrapper.set("material",nativeMaterial(replacement));
    opacity.setValue(AnimationArray::array({.5}),0);out["oldOpacity"]=material.get("opacity").toNumber();out["newOpacity"]=replacement->opacity;
    opacity.unbind();opacity.setValue(AnimationArray::array({.75}),0);out["rebound"]=replacement->opacity;
    if(name=="Sprite") {
        const auto center=wrapper.get("center");PropertyBinding binding(wrapper,".center"),count(wrapper,".count");
        binding.setValue(AnimationArray::array({.1,.9}),0);count.setValue(AnimationArray::array({4}),0);
        out["center"]=numbers(std::dynamic_pointer_cast<Sprite>(object)->center.toArray());out["centerIdentity"]=center.sameValue(wrapper.get("center"));out["count"]=wrapper.get("count").toNumber();
    } else {
        const auto influences=wrapper.get("morphTargetInfluences");PropertyBinding binding(wrapper,".morphTargetInfluences[smile]");
        binding.setValue(AnimationArray::array({.8}),0);out["morph"]=influences.get("0").toNumber();out["morphIdentity"]=influences.sameValue(wrapper.get("morphTargetInfluences"));
    }
    out["dirty"]=object->matrixWorldNeedsUpdate;return out;
}
JsonValue run(const JSValue& r){if(r.get("kind").string()=="distance")return distances(r);if(r.get("kind").string()=="copy")return copying(r);if(r.get("kind").string()=="binding")return bindings(r);if(r.get("kind").string()=="bounds")return bounds(r);if(r.get("kind").string()=="serialize")return serialization(r);return rays(r);}
}
int main(){try{const std::string input(std::istreambuf_iterator<char>(std::cin),{});auto requests=Animation_detail::parseJSON(input);JsonValue::Array results;for(std::uint32_t i=0;i<requests.length();++i)results.push_back(run(requests.get(std::to_string(i))));std::cout<<JsonValue(results).dump()<<'\n';}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
