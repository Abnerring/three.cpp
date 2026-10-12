#include "objects/SkinnedMesh.h"
#include "objects/InstancedMesh.h"
#include "core/Raycaster.h"
#include "math/Frustum.h"
#include "animation/AnimationMixer.h"
#include "animation/AnimationClip.h"
#include "animation/tracks/VectorKeyframeTrack.h"
#include "animation/tracks/QuaternionKeyframeTrack.h"
#include "animation/tracks/NumberKeyframeTrack.h"
#include "core/NativeObject.h"
#include <iostream>

namespace {
template<class Values> JsonValue numbers(const Values& values) {
    JsonValue::Array result; for (const auto value : values) result.emplace_back(double(value)); return result;
}
JsonValue matrix(const Matrix4& value) { return numbers(value.elements()); }
JsonValue box(const std::shared_ptr<Box3>& value) {
    return value ? JsonValue(JsonValue::Array{numbers(value->min.toArray()),numbers(value->max.toArray())}) : JsonValue();
}
JsonValue sphere(const std::shared_ptr<Sphere>& value) {
    return value ? JsonValue(JsonValue::Array{numbers(value->center.toArray()),value->radius}) : JsonValue();
}
JsonValue objectBounds(Object3D& value) {
    auto conservative=std::make_shared<Box3>(),precise=std::make_shared<Box3>();
    conservative->setFromObject(value);precise->setFromObject(value,true);
    JsonValue::Array cull;
    for(double shift:{-12.,-2.,0.,2.,12.}){
        Matrix4 projection,view,combined;projection.makePerspective(-2,2,2,-2,.1,50);view.makeTranslation(shift,0,-5);combined.multiplyMatrices(projection,view);
        Frustum frustum;frustum.setFromProjectionMatrix(combined);cull.emplace_back(frustum.intersectsObject(value));
    }
    return JsonValue::Object{{"conservative",box(conservative)},{"precise",box(precise)},{"frustum",cull}};
}
JsonValue pixels(const std::shared_ptr<DataTexture>& texture) {
    if(!texture)return nullptr;
    return JsonValue::Object{{"width",texture->source->data->width},{"height",texture->source->data->height},
        {"data",numbers(std::get<Float32Array>(texture->source->data->data))},
        {"version",texture->version},{"sourceVersion",texture->source->version},
        {"format",texture->format},{"type",texture->type},{"minFilter",texture->minFilter},
        {"magFilter",texture->magFilter},{"flipY",texture->flipY},{"generateMipmaps",texture->generateMipmaps},
        {"unpackAlignment",texture->unpackAlignment}};
}
JsonValue skeletonState(const Skeleton& skeleton) {
    JsonValue::Array inverses,world,local,positions;
    for(const auto& inverse:skeleton.boneInverses)inverses.push_back(matrix(*inverse));
    for(const auto& bone:skeleton.bones){
        world.push_back(bone?matrix(bone->matrixWorld):JsonValue());
        local.push_back(bone?matrix(bone->matrix):JsonValue());
        positions.push_back(bone?JsonValue(JsonValue::Array{numbers(bone->position.toArray()),numbers(bone->quaternion.toArray()),numbers(bone->scale.toArray())}):JsonValue());
    }
    return JsonValue::Object{{"inverses",inverses},{"world",world},{"local",local},{"positions",positions},
        {"matrices",numbers(skeleton.boneMatrices)},{"texture",pixels(skeleton.boneTexture)}};
}
JsonValue skeletonCase(int n,int mode) {
    auto parent=std::make_shared<Object3D>();parent->position.set(2,-3,4);parent->rotation.set(.1,.2,-.3);
    Skeleton::Bones bones;
    for(int i=0;i<n;++i){auto bone=std::make_shared<Bone>();bone->name="bone"+std::to_string(i);bone->uuid=bone->name;
        bone->position.set(i*.5,i+1,-i*.25);bone->rotation.set(i*.02,i*.03,-i*.01);bone->scale.set(1+i*.05,1,1-i*.01);
        if(mode==1&&i>0)bones[std::size_t(i-1)]->add(bone);else if(mode==2)parent->add(bone);
        bones.push_back(bone);
    }
    parent->updateMatrixWorld(true);for(const auto& bone:bones)if(!bone->parent)bone->updateMatrixWorld(true);
    if(mode==3&&n>0)bones[std::size_t(n-1)].reset();
    Skeleton::Inverses supplied;
    if(mode==4)for(int i=0;i<n+1;++i)supplied.push_back(std::make_shared<Matrix4>());
    Skeleton value(bones,supplied);value.uuid="skeleton";
    JsonValue result=JsonValue::Object{{"inputBonesDistinct",!value.bones.sameIdentity(bones)},
        {"inverseInputSame",value.boneInverses.sameIdentity(supplied)}, {"initial",skeletonState(value)}};
    const auto clone=value.clone();result["clone"]=JsonValue::Object{{"bonesDistinct",!clone->bones.sameIdentity(value.bones)},
        {"inversesSame",clone->boneInverses.sameIdentity(value.boneInverses)},
        {"matricesDistinct",!clone->boneMatrices.sameIdentity(value.boneMatrices)},{"textureNull",!clone->boneTexture}};
    for(std::size_t i=0;i<bones.size();++i)if(bones[i]){bones[i]->position.x+=.25*(double(i)+1);bones[i]->rotation.y+=.3;}
    parent->updateMatrixWorld(true);for(const auto& bone:bones)if(bone&&!bone->parent)bone->updateMatrixWorld(true);
    value.update();result["updated"]=skeletonState(value);
    const auto old=value.boneMatrices;value.computeBoneTexture();
    result["textureIdentity"]=JsonValue::Array{!old.sameIdentity(value.boneMatrices),value.boneMatrices.sameIdentity(std::get<Float32Array>(value.boneTexture->source->data->data))};
    value.update();result["textured"]=skeletonState(value);value.pose();result["posed"]=skeletonState(value);
    if(mode!=3||n==0)result["json"]=value.toJSON();
    if(mode!=3||n==0){const auto found=value.getBoneByName("bone0");result["find"]=JsonValue::Array{bool(found),bool(value.getBoneByName("absent"))};}
    int disposed=0;const auto texture=value.boneTexture;texture->addEventListener("dispose",[&](Event&){++disposed;});
    value.dispose();value.dispose();result["disposed"]=JsonValue::Array{disposed,!value.boneTexture};
    return result;
}
std::shared_ptr<BufferGeometry> triangle(int seed,bool skinned) {
    auto g=std::make_shared<BufferGeometry>();g->uuid="geometry";
    g->setAttribute("position",std::make_shared<Float32BufferAttribute>(std::vector<float>{-1,-1,0,1,-1,0,0,1,0},3));
    g->setAttribute("normal",std::make_shared<Float32BufferAttribute>(std::vector<float>{0,0,1,0,0,1,0,0,1},3));
    g->setAttribute("uv",std::make_shared<Float32BufferAttribute>(std::vector<float>{0,0,1,0,.5,1},2));
    if(seed%2)g->setIndex({0,1,2});
    if(seed%3==1){g->morphAttributes["position"]={std::make_shared<Float32BufferAttribute>(std::vector<float>{0,0,1,0,0,2,0,0,3},3)};g->morphTargetsRelative=bool(seed%2);}
    if(skinned){
        g->setAttribute("skinIndex",std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0,1,0,1,1,0,0,1,0,1,0,1},4));
        const std::vector<std::vector<float>> sets{{1,0,0,0,.5f,.5f,0,0,0,1,0,0},{.2f,.8f,0,0,.7f,.3f,0,0,.1f,.9f,0,0},
            {0,0,0,0,0,0,0,0,0,0,0,0},{-2,1,0,0,1,-2,0,0,-1,-1,0,0},{2,3,0,0,3,2,0,0,4,1,0,0}};
        g->setAttribute("skinWeight",std::make_shared<Float32BufferAttribute>(sets[std::size_t(seed%5)],4));
    }
    return g;
}
JsonValue hits(const std::vector<Intersection>& values,const Object3D* owner) {
    JsonValue::Array result;
    for(const auto& h:values){JsonValue v=JsonValue::Object{{"distance",h.distance},{"point",numbers(h.point.toArray())},
        {"object",h.object==owner},{"instanceId",h.instanceId?JsonValue(*h.instanceId):JsonValue()},
        {"faceIndex",h.faceIndex?JsonValue(*h.faceIndex):JsonValue()},
        {"uv",h.uv?numbers(h.uv->toArray()):JsonValue()}, {"normal",h.normal?numbers(h.normal->toArray()):JsonValue()},
        {"barycoord",h.barycoord?numbers(h.barycoord->toArray()):JsonValue()}};
        if(h.face)v["face"]=JsonValue::Array{h.face->a,h.face->b,h.face->c,numbers(h.face->normal.toArray()),h.face->materialIndex};
        result.push_back(v);
    }return result;
}
JsonValue skinCase(int seed) {
    auto g=triangle(seed,true);auto material=std::make_shared<MeshBasicMaterial>();material->uuid="material";material->side=DoubleSide;
    auto mesh=std::make_shared<SkinnedMesh>(g,material);mesh->uuid="mesh";mesh->position.set(.5,-.5,.25);mesh->scale.set(1,1.25,.75);
    mesh->bindMode=seed%2?DetachedBindMode:AttachedBindMode;
    auto b0=std::make_shared<Bone>(),b1=std::make_shared<Bone>();b0->uuid="bone0";b1->uuid="bone1";b0->name="first";b1->name="second";
    b0->position.set(.25,.5,0);b1->position.set(0,1,.25);b0->add(b1);mesh->add(b0);mesh->updateMatrixWorld(true);
    auto skeleton=std::make_shared<Skeleton>(Skeleton::Bones{b0,b1});skeleton->uuid="skeleton";
    if(seed%3==0){Matrix4 bind;bind.makeTranslation(.5,0,-.25);mesh->bind(skeleton,bind);}else mesh->bind(skeleton);
    if(seed%4==0)mesh->normalizeSkinWeights();if(mesh->morphTargetInfluences)mesh->morphTargetInfluences->at(0)=.25;
    b0->position.x+=.5;b1->rotation.z=.1*seed;b1->position.y+=.25;mesh->position.x+=.25;mesh->updateMatrixWorld(true);skeleton->update();
    JsonValue::Array vertices,directions;
    std::vector<Vector3> points;
    for(std::size_t i=0;i<3;++i){Vector3 v;mesh->getVertexPosition(i,v);vertices.push_back(numbers(v.toArray()));points.push_back(v);
        Vector4 d(1,2,3,seed%2?0:2);mesh->applyBoneTransform(double(i),d);directions.push_back(numbers(d.toArray()));}
    mesh->computeBoundingBox();mesh->computeBoundingSphere();auto clone=std::dynamic_pointer_cast<SkinnedMesh>(mesh->clone());
    Vector3 center;for(const auto& p:points)center.add(p);center.divideScalar(3).applyMatrix4(mesh->matrixWorld);
    Vector3 a=points[0],b=points[1],c=points[2];a.applyMatrix4(mesh->matrixWorld);b.applyMatrix4(mesh->matrixWorld);c.applyMatrix4(mesh->matrixWorld);
    Vector3 normal;normal.subVectors(b,a).cross(Vector3().subVectors(c,a)).normalize();Vector3 origin;origin.copy(center).addScaledVector(normal,3);
    auto intersections=Raycaster(origin,Vector3().copy(normal).negate()).intersectObject(*mesh,false);
    JsonValue result=JsonValue::Object{{"vertices",vertices},{"directions",directions},{"box",box(mesh->boundingBox)},
        {"sphere",sphere(mesh->boundingSphere)},{"bind",matrix(mesh->bindMatrix)},{"inverse",matrix(mesh->bindMatrixInverse)},
        {"matrices",numbers(skeleton->boneMatrices)},{"weights",g->getAttribute("skinWeight")->toJSON()},
        {"hits",hits(intersections,mesh.get())},{"clone",JsonValue::Array{clone->skeleton==skeleton,clone->geometry==g,clone->material==material,
            clone->boundingBox!=mesh->boundingBox,clone->boundingSphere!=mesh->boundingSphere,clone->children[0]!=b0,clone->children[0]->isBone}},
        {"json",mesh->toJSON()},{"objectBounds",objectBounds(*mesh)}};
    return result;
}
JsonValue instanceCase(int seed) {
    auto g=triangle(seed,false);auto material=std::make_shared<MeshBasicMaterial>();material->uuid="material";
    const double count=std::vector<double>{0,1,2,7,1.5}[std::size_t(seed%5)];
    auto mesh=std::make_shared<InstancedMesh>(g,material,count);mesh->uuid="mesh";mesh->position.set(.5,0,.25);mesh->updateMatrixWorld();
    Matrix4 local;for(double i=0;i<count;++i){local.makeTranslation(i*3,i*.25,-i*.5);local.scale(Vector3(1+i*.1,1,1));mesh->setMatrixAt(i,local);}
    Color color;mesh->getColorAt(-1,color);const auto defaultColor=numbers(color.toArray());
    mesh->setColorAt(1,Color(.25,.5,.75));mesh->setColorAt(.5,Color(.1,.2,.3));mesh->getColorAt(0,color);
    Matrix4 read;mesh->getMatrixAt(-.125,read);
    Mesh source;source.morphTargetInfluences=SharedArray<double>{.2,-.3,.7};
    std::string error;
    try{mesh->setMorphAt(0,source);}catch(const JSRangeError&){error="RangeError";}
    source.morphTargetInfluences=SharedArray<double>{0,0,0};if(mesh->morphTexture)mesh->getMorphAt(0,source);
    mesh->computeBoundingBox();mesh->computeBoundingSphere();const auto cloned=std::dynamic_pointer_cast<InstancedMesh>(mesh->clone());
    JsonValue::Array rayResults;if(seed%5!=4)for(int i=0;i<3;++i){const auto h=Raycaster(Vector3(.5+i*3,i*.25,5),Vector3(0,0,-1)).intersectObject(*mesh,false);rayResults.push_back(hits(h,mesh.get()));}
    JsonValue result=JsonValue::Object{{"type",mesh->type},{"morphAbsent",!mesh->morphTargetInfluences},
        {"defaultColor",defaultColor},{"color0",numbers(color.toArray())},{"matrixOffset",matrix(read)},
        {"instanceMatrix",mesh->instanceMatrix->toJSON()},{"instanceColor",mesh->instanceColor->toJSON()},
        {"versions",JsonValue::Array{mesh->instanceMatrix->version.toNumber(),mesh->instanceColor->version.toNumber()}},
        {"box",box(mesh->boundingBox)},{"sphere",sphere(mesh->boundingSphere)},{"texture",pixels(mesh->morphTexture)},
        {"morph",numbers(*source.morphTargetInfluences)},{"morphError",error},{"rays",rayResults},
        {"clone",JsonValue::Array{cloned->instanceMatrix!=mesh->instanceMatrix,cloned->instanceColor!=mesh->instanceColor,
            cloned->morphTexture!=mesh->morphTexture,cloned->morphTexture->source==mesh->morphTexture->source,
            cloned->boundingBox!=mesh->boundingBox,cloned->boundingSphere!=mesh->boundingSphere}},
        {"json",mesh->toJSON()},{"objectBounds",objectBounds(*mesh)}};
    JsonValue::Array events;const auto texture=mesh->morphTexture;
    mesh->addEventListener("dispose",[&](Event&){events.emplace_back("mesh");});texture->addEventListener("dispose",[&](Event&){events.emplace_back("texture");});
    mesh->dispose();result["dispose"]=JsonValue::Object{{"events",events},{"textureNull",!mesh->morphTexture}};
    return result;
}
JsonValue fromJSONCase() {
    auto bone=std::make_shared<Bone>();bone->uuid="known";bone->name="retained";
    Skeleton original({bone});original.uuid="source";auto json=original.toJSON();
    json["bones"].get<JsonValue::Array>().emplace_back("missing");json["boneInverses"].get<JsonValue::Array>().push_back(matrix(Matrix4().makeTranslation(1,2,3)));
    Skeleton result({bone});result.fromJSON(json,{{"known",bone}});auto clone=result.clone();
    return JsonValue::Object{{"length",result.bones.size()},{"uuid",result.uuid},{"alias",result.bones[0]==result.bones[1]},
        {"missingBone",result.bones[2]->isBone},{"inverses",skeletonState(result).at("inverses")},
        {"cloneInverseAlias",clone->boneInverses.sameIdentity(result.boneInverses)},
        {"sourceName",result.bones[1]->name}};
}
JsonValue edgeCases() {
    JsonValue result=JsonValue::Object{};
    InstancedMesh omitted;
    result["omittedCount"]=JsonValue::Object{{"undefined",omitted.count.asValue().isUndefined()},
        {"length",omitted.instanceMatrix->count},{"jsonHasCount",omitted.toJSON().at("object").contains("count")}};
    auto object=JSValue::object();object.set("repeat",2);
    InstancedBufferAttribute attribute(Int16Array({1,2,3,4}),2,true,object);
    auto clone=std::dynamic_pointer_cast<InstancedBufferAttribute>(attribute.clone());
    result["attributeAlias"]=JsonValue::Array{clone->meshPerAttribute.sameValue(object),clone->toJSON()};
    BufferAttribute plain(Float64Array({.25,.75}),1);
    attribute.copy(plain);result["attributePlain"]=JsonValue::Array{attribute.meshPerAttribute.isUndefined(),attribute.toJSON()};
    auto bone=std::make_shared<Bone>();Skeleton skeleton({bone});skeleton.update();skeleton.computeBoneTexture();
    const auto texture=skeleton.boneTexture;const auto oldMatrices=skeleton.boneMatrices;int events=0;
    texture->addEventListener("dispose",[&](Event&){++events;});skeleton.computeBoneTexture();
    result["retexture"]=JsonValue::Array{events,texture!=skeleton.boneTexture,!oldMatrices.sameIdentity(skeleton.boneMatrices),pixels(texture),pixels(skeleton.boneTexture)};
    const auto retained=skeleton.boneTexture;skeleton.init();bone->matrixWorld.makeTranslation(1,2,3);skeleton.update();
    result["initRetainsTexture"]=JsonValue::Object{{"textureSame",retained==skeleton.boneTexture},
        {"dataDetached",!skeleton.boneMatrices.sameIdentity(std::get<Float32Array>(retained->source->data->data))},
        {"matrices",numbers(skeleton.boneMatrices)},{"texture",pixels(retained)}};
    auto g=triangle(0,true);SkinnedMesh mesh(g);mesh.bind(std::make_shared<Skeleton>(Skeleton::Bones{bone}),Matrix4());
    g->getAttribute("skinIndex")->setX(0,99);Vector3 target(1,2,3);std::string error;
    try{mesh.applyBoneTransform(0,target);}catch(const JSTypeError&){error="TypeError";}
    result["invalidBone"]=JsonValue::Array{error,numbers(target.toArray())};
    mesh.bindMatrixInverse.makeTranslation(3,4,5);mesh.bindMode="unknown";mesh.updateMatrixWorld(true);
    result["unknownBind"]=matrix(mesh.bindMatrixInverse);
    JsonValue::Array counts;
    for(const auto& count:std::vector<JSValue>{JSValue(),nullptr,false,true,"2",-.1,-1,1.5,std::numeric_limits<double>::quiet_NaN()}){
        try{InstancedMesh value(std::make_shared<BufferGeometry>(),std::make_shared<MeshBasicMaterial>(),count);
            counts.emplace_back(JsonValue::Object{{"undefined",value.count.asValue().isUndefined()},
                {"value",value.count.asValue().toJSON().value_or(JsonValue())},{"data",value.instanceMatrix->toJSON()}});
        }catch(const JSRangeError&){counts.emplace_back("RangeError");}
    }
    result["counts"]=counts;
    return result;
}
class ReentrantSkin : public SkinnedMesh {
  public:
    using SkinnedMesh::SkinnedMesh;
    std::shared_ptr<SkinnedMesh> nested;
    Vector3& getVertexPosition(std::size_t i, Vector3& target) const override {
        SkinnedMesh::getVertexPosition(i,target);
        if(i==0&&nested)nested->computeBoundingBox();
        return target;
    }
};
JsonValue reentryCase() {
    auto bone=std::make_shared<Bone>();auto skeleton=std::make_shared<Skeleton>(Skeleton::Bones{bone,bone});
    auto nested=std::make_shared<SkinnedMesh>(triangle(0,true));nested->bind(skeleton,Matrix4());
    const auto pos=nested->geometry->getAttribute("position");for(int i=0;i<3;++i)pos->setX(i,pos->getX(i)+100);
    ReentrantSkin outer(triangle(0,true));outer.bind(skeleton,Matrix4());outer.nested=nested;outer.computeBoundingBox();outer.computeBoundingSphere();
    auto first=std::make_shared<Bone>(),second=std::make_shared<Bone>();first->position.x=1;second->position.x=2;first->updateMatrixWorld();second->updateMatrixWorld();
    Skeleton pose({first,second});first->position.x=11;second->position.x=12;first->updateMatrixWorld();second->updateMatrixWorld();int calls=0;
    first->quaternion._onChange([&]{++calls;pose.bones.clear();});pose.pose();
    return JsonValue::Object{{"box",box(outer.boundingBox)},{"sphere",sphere(outer.boundingSphere)},
        {"pose",JsonValue::Array{calls,pose.bones.size(),numbers(first->position.toArray()),numbers(second->position.toArray()),matrix(second->matrixWorld)}}};
}
JsonValue animationCase(int mode) {
    auto mesh=std::make_shared<SkinnedMesh>(triangle(1,true));auto first=std::make_shared<Bone>(),second=std::make_shared<Bone>();
    first->name="first";second->name="second";second->position.y=1;first->add(second);
    if(mode!=4)mesh->add(first);else{mesh->bindMode=DetachedBindMode;first->updateMatrixWorld(true);}mesh->updateMatrixWorld(true);
    auto skeleton=std::make_shared<Skeleton>(Skeleton::Bones{first,second});mesh->bind(skeleton);skeleton->computeBoneTexture();
    auto tracks=std::make_shared<AnimationClip::Tracks>();
    tracks->push_back(std::make_shared<VectorKeyframeTrack>(mode>=3?"second.position":".bones[second].position",Float32Array({0,1}),Float32Array({0,1,0,0,3,1})));
    tracks->push_back(std::make_shared<QuaternionKeyframeTrack>(mode>=3?"first.quaternion":".bones[first].quaternion",Float32Array({0,1}),Float32Array(std::vector<double>{0,0,0,1,0,0,std::sqrt(.5),std::sqrt(.5)})));
    tracks->push_back(std::make_shared<NumberKeyframeTrack>(mode>=3?"second.scale[y]":".bones[second].scale[y]",Float32Array({0,1}),Float32Array({1,2})));
    auto clip=std::make_shared<AnimationClip>("bones",1,tracks);AnimationMixer mixer(mesh);auto action=mixer.clipAction(clip);
    action.setLoop(mode==0?LoopOnce:mode==1?LoopRepeat:LoopPingPong,2);action.clampWhenFinished=true;action.play();
    const auto object=nativeObject(mesh),nativeSkeleton=object.get("skeleton");
    JsonValue result=JsonValue::Object{{"identity",JsonValue::Array{nativeSkeleton.sameValue(object.get("skeleton")),
        nativeSkeleton.get("bones").get("0").sameValue(nativeObject(first)),nativeSkeleton.get("bones").get("1").sameValue(nativeObject(second)),
        nativeSkeleton.get("boneMatrices").sameValue(nativeSkeleton.get("boneMatrices"))}}};
    JsonValue::Array samples;
    for(double delta:{.25,.5,.75,1.,.5}){
        mixer.update(delta);mesh->updateMatrixWorld(true);if(mode==4)first->updateMatrixWorld(true);skeleton->update();Vector3 vertex;mesh->getVertexPosition(0,vertex);mixer.object().collectGarbage();
        samples.emplace_back(JsonValue::Object{{"time",action.time.toNumber()},{"enabled",action.enabled.toBoolean()},{"paused",action.paused.toBoolean()},
            {"position",numbers(second->position.toArray())},{"quaternion",numbers(first->quaternion.toArray())},{"scale",numbers(second->scale.toArray())},
            {"vertex",numbers(vertex.toArray())},{"matrices",numbers(skeleton->boneMatrices)},{"textureVersion",skeleton->boneTexture->version}});
    }
    action.stop();mesh->updateMatrixWorld(true);if(mode==4)first->updateMatrixWorld(true);skeleton->update();
    result["samples"]=samples;result["restored"]=JsonValue::Array{numbers(second->position.toArray()),numbers(first->quaternion.toArray()),numbers(second->scale.toArray())};
    return result;
}
class BoundsOverrideSkin : public SkinnedMesh {
  public:
    int calls=0;
    int boxCalls=0;
    bool provideSphere=true;
    bool provideBox=true;
    void computeBoundingSphere() override {
        ++calls;
        boundingSphere=provideSphere?std::make_shared<Sphere>(Vector3(),1):nullptr;
    }
    void computeBoundingBox() override {
        ++boxCalls;
        boundingBox=provideBox?std::make_shared<Box3>(Vector3(),Vector3(1,2,3)):nullptr;
    }
};
JsonValue boundsEdgeCase() {
    JsonValue result=JsonValue::Object{};
    Frustum frustum;
    const auto attempt=[](const std::function<JsonValue()>& call)->JsonValue {
        try{return call();}catch(const JSTypeError&){return "TypeError";}
        catch(const std::invalid_argument&){return "invalid_argument";}
    };
    for(bool skin:{true,false}) {
        std::shared_ptr<Mesh> mesh=skin?std::static_pointer_cast<Mesh>(std::make_shared<SkinnedMesh>()):
            std::static_pointer_cast<Mesh>(std::make_shared<InstancedMesh>());
        mesh->geometry.reset();
        if(skin)std::static_pointer_cast<SkinnedMesh>(mesh)->boundingSphere=std::make_shared<Sphere>(Vector3(),1);
        else std::static_pointer_cast<InstancedMesh>(mesh)->boundingSphere=std::make_shared<Sphere>(Vector3(),1);
        result[skin?"skinCached":"instanceCached"]=attempt([&]{return frustum.intersectsObject(static_cast<Object3D&>(*mesh));});
        if(skin)result["skinDirect"]=attempt([&]{return frustum.intersectsObject(*std::static_pointer_cast<SkinnedMesh>(mesh));});
        else result["instanceDirect"]=attempt([&]{return frustum.intersectsObject(*std::static_pointer_cast<InstancedMesh>(mesh));});
    }
    BoundsOverrideSkin computed;computed.geometry.reset();
    result["computed"]=attempt([&]{return frustum.intersectsObject(static_cast<Object3D&>(computed));});
    result["computedCalls"]=computed.calls;
    BoundsOverrideSkin invalid;invalid.provideSphere=false;
    result["invalidComputed"]=attempt([&]{return frustum.intersectsObject(static_cast<Object3D&>(invalid));});
    result["invalidComputedCalls"]=invalid.calls;
    BoundsOverrideSkin customBox;
    auto customBoxResult=std::make_shared<Box3>();
    result["computedBox"]=attempt([&]{customBoxResult->setFromObject(customBox);return box(customBoxResult);});
    result["computedBoxCalls"]=customBox.boxCalls;
    customBox.provideBox=false;customBox.boundingBox.reset();
    result["invalidComputedBox"]=attempt([&]{customBoxResult->setFromObject(customBox);return box(customBoxResult);});
    result["invalidComputedBoxCalls"]=customBox.boxCalls;
    SkinnedMesh absent;absent.geometry.reset();
    result["absent"]=attempt([&]{return frustum.intersectsObject(static_cast<Object3D&>(absent));});
    result["absentAllocated"]=bool(absent.boundingSphere);
    InstancedMesh noGeometry;noGeometry.geometry.reset();
    result["instanceAbsent"]=attempt([&]{return frustum.intersectsObject(static_cast<Object3D&>(noGeometry));});
    result["instanceAbsentAllocated"]=bool(noGeometry.boundingSphere);
    auto geometry=triangle(0,true);auto bone=std::make_shared<Bone>();
    auto skeleton=std::make_shared<Skeleton>(Skeleton::Bones{bone,bone});SkinnedMesh mesh(geometry);mesh.bind(skeleton,Matrix4());
    bone->matrixWorld.makeTranslation(10,0,0);mesh.isMesh=false;
    auto value=std::make_shared<Box3>();value->setFromObject(mesh,true);result["falseMeshFlag"]=box(value);
    mesh.isMesh=true;value->setFromObject(mesh,true);result["trueMeshFlag"]=box(value);
    mesh.geometry.reset();mesh.boundingBox=std::make_shared<Box3>(Vector3(),Vector3(1,1,1));
    result["boxNullGeometry"]=attempt([&]{value->setFromObject(mesh);return box(value);});
    return result;
}
}
int main() {
    try{
        JsonValue output=JsonValue::Object{};
        for(int n:{0,1,2,5,17})for(int mode=0;mode<5;++mode)output["skeleton:"+std::to_string(n)+":"+std::to_string(mode)]=skeletonCase(n,mode);
        for(int seed=0;seed<30;++seed)output["skin:"+std::to_string(seed)]=skinCase(seed);
        for(int seed=0;seed<15;++seed)output["instance:"+std::to_string(seed)]=instanceCase(seed);
        output["fromJSON"]=fromJSONCase();output["edges"]=edgeCases();output["reentry"]=reentryCase();
        for(int mode=0;mode<5;++mode)output["animation:"+std::to_string(mode)]=animationCase(mode);
        output["boundsEdges"]=boundsEdgeCase();
        std::cout<<output.dump()<<'\n';
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
