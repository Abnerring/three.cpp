#include "catch.hpp"
#include "objects/SkinnedMesh.h"
#include "core/Raycaster.h"

namespace {
std::shared_ptr<BufferGeometry> skinTriangle() {
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{-1,-1,0,1,-1,0,0,1,0}, 3));
    geometry->setAttribute("skinIndex", std::make_shared<Uint16BufferAttribute>(
        std::vector<std::uint16_t>(12, 0), 4));
    geometry->setAttribute("skinWeight", std::make_shared<Float32BufferAttribute>(
        std::vector<float>{1,0,0,0,1,0,0,0,1,0,0,0}, 4));
    return geometry;
}
}
TEST_CASE("Skeleton preserves bone and inverse aliases across init clone and pose", "[objects][skinning]") {
    auto parent = std::make_shared<Bone>(), child = std::make_shared<Bone>();
    parent->position.set(2,3,4); child->position.set(0,5,0); parent->add(child); parent->updateMatrixWorld(true);
    Skeleton::Bones bones{parent,child}; Skeleton::Inverses inverses;
    Skeleton skeleton(bones,inverses);
    REQUIRE_FALSE(skeleton.bones.sameIdentity(bones)); REQUIRE(skeleton.bones[0]==parent);
    REQUIRE(skeleton.boneInverses.sameIdentity(inverses)); REQUIRE(inverses.size()==2);
    auto cloned=skeleton.clone(); REQUIRE(cloned->boneInverses.sameIdentity(inverses));
    REQUIRE_FALSE(cloned->bones.sameIdentity(skeleton.bones));
    REQUIRE_FALSE(cloned->boneMatrices.sameIdentity(skeleton.boneMatrices));
    parent->position.x=9; child->position.y=10; parent->updateMatrixWorld(true); skeleton.pose();
    REQUIRE(parent->position.equals(Vector3(2,3,4))); REQUIRE(child->position.equals(Vector3(0,5,0)));
    REQUIRE(parent->isBone); REQUIRE(parent->clone()->isBone);
}
TEST_CASE("Skeleton bone texture shares float storage and dispose retains callback order", "[objects][skinning]") {
    auto bone=std::make_shared<Bone>(); Skeleton skeleton({bone});
    skeleton.update(); const auto old=skeleton.boneMatrices; skeleton.computeBoneTexture();
    const auto texture=skeleton.boneTexture; const auto& pixels=std::get<Float32Array>(texture->source->data->data);
    REQUIRE(skeleton.boneMatrices.sameIdentity(pixels)); REQUIRE_FALSE(old.sameIdentity(pixels));
    REQUIRE(pixels.size()==64); REQUIRE(texture->version==1); REQUIRE(texture->source->version==1);
    bone->position.x=3;bone->updateMatrixWorld();skeleton.update();REQUIRE(pixels[12]==3);
    REQUIRE(texture->version==2); REQUIRE(texture->source->version==2);
    int events=0; texture->addEventListener("dispose",[&](Event&){++events;REQUIRE(skeleton.boneTexture==texture);});
    skeleton.dispose();skeleton.dispose();REQUIRE(events==1);REQUIRE_FALSE(skeleton.boneTexture);
}
TEST_CASE("SkinnedMesh deforms vertices bounds rays and homogeneous directions", "[objects][skinning]") {
    auto geometry=skinTriangle();auto mesh=std::make_shared<SkinnedMesh>(geometry);auto bone=std::make_shared<Bone>();mesh->add(bone);
    auto skeleton=std::make_shared<Skeleton>(Skeleton::Bones{bone});mesh->bind(skeleton);
    bone->position.x=2;mesh->updateMatrixWorld(true);Vector3 vertex;mesh->getVertexPosition(0,vertex);
    REQUIRE(vertex.equals(Vector3(1,-1,0)));
    Vector4 direction(0,1,0,0);mesh->applyBoneTransform(0,direction);REQUIRE(direction.equals(Vector4(0,1,0,0)));
    Vector4 position(0,1,0,1);mesh->applyBoneTransform(0,position);REQUIRE(position.equals(Vector4(2,1,0,1)));
    mesh->computeBoundingBox();mesh->computeBoundingSphere();REQUIRE(mesh->boundingBox->min.equals(Vector3(1,-1,0)));
    auto hits=Raycaster(Vector3(2,0,3),Vector3(0,0,-1)).intersectObject(*mesh,false);
    REQUIRE(hits.size()==1);REQUIRE(hits[0].object==mesh.get());REQUIRE(hits[0].objectOwner==mesh);REQUIRE(hits[0].distance==3);
    auto cloned=std::dynamic_pointer_cast<SkinnedMesh>(mesh->clone());REQUIRE(cloned->skeleton==skeleton);
    REQUIRE(cloned->boundingBox!=mesh->boundingBox);REQUIRE(cloned->boundingBox->equals(*mesh->boundingBox));
    REQUIRE(cloned->children[0]!=bone);REQUIRE(cloned->children[0]->isBone);
}
TEST_CASE("SkinnedMesh weight normalization handles zero negative and normalized integer data", "[objects][skinning]") {
    auto geometry=skinTriangle();SkinnedMesh mesh(geometry);
    auto weights=std::make_shared<Float32BufferAttribute>(std::vector<float>{0,0,0,0,-2,2,0,0},4);
    geometry->setAttribute("skinWeight",weights);mesh.normalizeSkinWeights();
    REQUIRE(weights->getX(0)==1);REQUIRE(weights->getY(0)==0);REQUIRE(weights->getX(1)==-.5);REQUIRE(weights->getY(1)==.5);
    REQUIRE(weights->version.toNumber()==0);
    auto integer=std::make_shared<Uint8BufferAttribute>(std::vector<std::uint8_t>{10,20,30,40},4,true);
    geometry->setAttribute("skinWeight",integer);mesh.normalizeSkinWeights();
    REQUIRE(integer->getX(0)==Approx(25.0/255));REQUIRE(integer->getW(0)==Approx(102.0/255));
}
