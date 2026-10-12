#include "catch.hpp"
#include "objects/InstancedMesh.h"
#include "core/Raycaster.h"

TEST_CASE("InstancedBufferAttribute copies bytes and preserves source typed array semantics", "[objects][instancing]") {
    Int16Array values({1,2,3,4}); InstancedBufferAttribute attribute(values,2,true,3);
    attribute.name="instances";attribute.setUsage(DynamicDrawUsage);
    auto clone=std::dynamic_pointer_cast<InstancedBufferAttribute>(attribute.clone());
    REQUIRE(clone);REQUIRE(clone->meshPerAttribute.number()==3);REQUIRE(clone->normalized);REQUIRE(clone->usage.toNumber()==DynamicDrawUsage);
    REQUIRE_FALSE(std::get<Int16Array>(clone->array).sameIdentity(values));
    values.setIndex(0,300);REQUIRE(std::get<Int16Array>(attribute.array)[0]==300);REQUIRE(std::get<Int16Array>(clone->array)[0]==1);
    std::shared_ptr<BufferAttribute> base=clone;REQUIRE(base->toJSON().at("isInstancedBufferAttribute").get<bool>());
    REQUIRE(base->toJSON().at("meshPerAttribute").get<double>()==3);
}
TEST_CASE("InstancedMesh bounds and raycast retain instance identities", "[objects][instancing]") {
    auto geometry=std::make_shared<BufferGeometry>();
    geometry->setAttribute("position",std::make_shared<Float32BufferAttribute>(std::vector<float>{-1,-1,0,1,-1,0,0,1,0},3));
    auto mesh=std::make_shared<InstancedMesh>(geometry,std::make_shared<MeshBasicMaterial>(),2);
    Matrix4 matrix;mesh->setMatrixAt(1,matrix.makeTranslation(4,0,0));mesh->computeBoundingBox();mesh->computeBoundingSphere();
    REQUIRE(mesh->boundingBox->min.equals(Vector3(-1,-1,0)));REQUIRE(mesh->boundingBox->max.equals(Vector3(5,1,0)));
    REQUIRE(mesh->instanceMatrix->version.toNumber()==0);REQUIRE(mesh->type=="Mesh");
    auto hits=Raycaster(Vector3(4,0,3),Vector3(0,0,-1)).intersectObject(*mesh,false);
    REQUIRE(hits.size()==1);REQUIRE(hits[0].instanceId==1);REQUIRE(hits[0].object==mesh.get());REQUIRE(hits[0].objectOwner==mesh);
    Color color;REQUIRE(&mesh->getColorAt(-1,color)==&color);REQUIRE(color.equals(Color(1,1,1)));
    mesh->setColorAt(1,Color(.25,.5,.75));mesh->getColorAt(0,color);REQUIRE(color.equals(Color(1,1,1)));
    mesh->getColorAt(1,color);REQUIRE(color.equals(Color(.25,.5,.75)));REQUIRE(mesh->instanceColor->version.toNumber()==0);
    auto clone=std::dynamic_pointer_cast<InstancedMesh>(mesh->clone());REQUIRE(clone->geometry==geometry);
    REQUIRE(clone->instanceMatrix!=mesh->instanceMatrix);REQUIRE(clone->instanceColor!=mesh->instanceColor);
    REQUIRE(clone->boundingBox!=mesh->boundingBox);REQUIRE(clone->count==2);
}
TEST_CASE("InstancedMesh morph texture cloning shares source and dispose listeners run first", "[objects][instancing]") {
    auto geometry=std::make_shared<BufferGeometry>();
    geometry->morphAttributes["position"]={std::make_shared<Float32BufferAttribute>(std::vector<float>{0,0,0},3)};
    Mesh source(geometry);source.morphTargetInfluences=SharedArray<double>{.25};
    InstancedMesh mesh(geometry,std::make_shared<MeshBasicMaterial>(),2);
    REQUIRE_FALSE(mesh.morphTargetInfluences);REQUIRE_FALSE(mesh.morphTargetDictionary);
    mesh.setMorphAt(1,source);REQUIRE(mesh.morphTexture->version==0);
    REQUIRE(mesh.morphTexture->source->data->width==2);REQUIRE(mesh.morphTexture->source->data->height==2);
    source.morphTargetInfluences->at(0)=0;mesh.getMorphAt(1,source);REQUIRE(source.morphTargetInfluences->at(0)==.25);
    auto clone=std::dynamic_pointer_cast<InstancedMesh>(mesh.clone());
    REQUIRE(clone->morphTexture!=mesh.morphTexture);REQUIRE(clone->morphTexture->source==mesh.morphTexture->source);
    auto texture=mesh.morphTexture;std::vector<std::string> events;
    mesh.addEventListener("dispose",[&](Event&){events.emplace_back("mesh");REQUIRE(mesh.morphTexture==texture);});
    texture->addEventListener("dispose",[&](Event&){events.emplace_back("texture");REQUIRE(mesh.morphTexture==texture);});
    mesh.dispose();REQUIRE(events==std::vector<std::string>{"mesh","texture"});REQUIRE_FALSE(mesh.morphTexture);
}
