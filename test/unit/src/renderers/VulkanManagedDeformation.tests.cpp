#include "renderers/VulkanRenderer.h"
#include "core/BufferAttribute.h"
#include "scenes/Scene.h"
#include "cameras/OrthographicCamera.h"
#include "geometries/PlaneGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"
#include "objects/SkinnedMesh.h"
#include "objects/Skeleton.h"
#include "objects/Bone.h"
#include "objects/InstancedMesh.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

// Real canonical source objects -> production VulkanRenderer.render -> pixels.
// trace-webgl-deformation.mjs independently checks the pinned CPU expectations.
namespace {
using Args=std::vector<JSValue>;
int assertions=0,images=0,validationRequested=0,validationEnabled=0,validationMessages=0;
void require(bool value,const char* message){++assertions;if(!value)throw std::runtime_error(message);}
JSValue call(const JSValue& object,const char* name,const Args& args={}){return object.get(name).call(object,args);}
JSValue list(const Args& values={}){auto result=JSValue::array();for(const auto& value:values)result.set(JSValue(result.length()),value);return result;}
void close(double actual,double expected,const char* message){require(std::abs(actual-expected)<1e-6,message);}
JSValue camera(){auto result=nativeOrthographicCameraConstructor().construct({-1,1,1,-1,.1,10});result.get("position").set("z",3);return result;}
JSValue paint(unsigned color=0xff0000){auto p=JSValue::object();p.set("color",color);return nativeMeshBasicMaterialConstructor().construct({p});}
JSValue scene(const Args& objects={}){auto result=nativeSceneConstructor().construct();if(!objects.empty())call(result,"add",objects);return result;}
void linear(VulkanRenderer& renderer){renderer.srgbOutput=false;renderer.object().set("outputColorSpace",LinearSRGBColorSpace);}
void render(VulkanRenderer& renderer,JSValue world,JSValue view){renderer.render(world,view);++images;}
void expect(const VulkanRenderer& renderer,std::array<int,3> color,const char* message,unsigned x=32,unsigned y=32){
    const auto bytes=renderer.readPixels();require(bytes.size()==renderer.getWidth()*renderer.getHeight()*4,"complete deformation RGBA readback");
    const auto offset=(y*renderer.getWidth()+x)*4;
    for(unsigned i=0;i<3;++i)if(std::abs(int(bytes.at(offset+i))-color[i])>2){std::cerr<<message<<": "<<int(bytes.at(offset))<<','<<int(bytes.at(offset+1))<<','<<int(bytes.at(offset+2))<<'\n';require(false,message);}
    require(true,message);
}
struct Validation {
    const VulkanRenderer& renderer;
    Validation(const VulkanRenderer& value,bool requested):renderer(value){if(requested)++validationRequested;if(value.getInfo().validationEnabled)++validationEnabled;}
    void verify(){const auto messages=renderer.getValidationMessages();validationMessages+=int(messages.size());for(const auto& message:messages)std::cerr<<message<<'\n';require(messages.empty(),"deformation GPU work emits no collected validation errors");}
};
JSValue skinnedGeometry(double size=.3,double first=1,double second=0){
    auto result=nativePlaneGeometryConstructor().construct({size,size});Args indices,weights;
    const auto count=result.get("attributes").get("position").get("count").toNumber();
    for(int i=0;i<int(count);++i){indices.insert(indices.end(),{0,1,0,0});weights.insert(weights.end(),{first,second,0,0});}
    call(result,"setAttribute",{"skinIndex",nativeUint16BufferAttributeConstructor().construct({list(indices),4})});
    call(result,"setAttribute",{"skinWeight",nativeFloat32BufferAttributeConstructor().construct({list(weights),4})});return result;
}
struct Skin {
    JSValue geometry,material,object,bone,skeleton,world,view;
    Skin(JSValue shape={},JSValue materialValue={}):geometry(shape.isUndefined()?skinnedGeometry():shape),material(materialValue.isUndefined()?paint():materialValue),
        object(nativeSkinnedMeshConstructor().construct({geometry,material})),bone(nativeBoneConstructor().construct()),
        skeleton(nativeSkeletonConstructor().construct({list({bone})})),world(scene()),view(camera()){
        object.set("frustumCulled",false);call(object,"add",{bone});call(object,"bind",{skeleton});call(world,"add",{object});
    }
};
JSValue observe(JSValue object,const char* name,JSValue state,const char* counter){
    const auto original=object.get(name);
    object.set(name,JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
        require(self.sameValue(c[1])&&args.empty(),"canonical skeleton callback retains receiver and zero arguments");
        auto state=c[2];state.set(c[3],state.get(c[3]).toNumber()+1);return c[0].call(self,args);
    },{original,object,state,counter}));return original;
}
void translatedSkeleton(bool enabled){
    VulkanRenderer renderer({64,64,enabled});linear(renderer);Validation layer(renderer,enabled);Skin skin;
    require(!skin.object.isHostObject("three.Object3D")&&!skin.skeleton.isHostObject("three.Skeleton"),"skinning retains canonical raw identities");
    require(skin.object.get("bindMode").strictEquals(AttachedBindMode)&&skin.skeleton.get("boneTexture").isNull(),"canonical attached binding starts without a bone texture");
    auto state=JSValue::object();state.set("updates",0);state.set("computes",0);state.set("uploads",0);
    observe(skin.skeleton,"update",state,"updates");observe(skin.skeleton,"computeBoneTexture",state,"computes");
    render(renderer,skin.world,skin.view);expect(renderer,{255,0,0},"rest-pose canonical skin reaches production Vulkan");
    auto texture=skin.skeleton.get("boneTexture"),matrices=skin.skeleton.get("boneMatrices");
    require(texture.get("image").get("data").sameValue(matrices)&&matrices.get("length").strictEquals(64),"source computeBoneTexture pads and aliases original float matrix storage");
    require(texture.get("image").get("width").strictEquals(4)&&texture.get("image").get("height").strictEquals(4),"one bone uses the canonical 4 by 4 texture");
    require(texture.get("format").strictEquals(RGBAFormat)&&texture.get("type").strictEquals(FloatType)&&texture.get("minFilter").strictEquals(NearestFilter)&&!texture.get("generateMipmaps").toBoolean(),"bone texture keeps source format and sampler defaults");
    texture.set("onUpdate",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
        require(self.sameValue(c[1])&&args.size()==1&&args[0].sameValue(self),"bone upload callback preserves original texture receiver and argument");
        auto state=c[0];state.set("uploads",state.get("uploads").toNumber()+1);return {};
    },{state,texture}));
    const auto version=texture.get("version").toNumber();skin.bone.get("position").set("x",.65);
    render(renderer,skin.world,skin.view);expect(renderer,{255,0,0},"bone world translation is fetched from the real vertex texture",53,32);expect(renderer,{0,0,0},"bone deformation removes the old rest-pose pixels");
    require(skin.skeleton.get("boneTexture").sameValue(texture)&&skin.skeleton.get("boneMatrices").sameValue(matrices),"ordinary bone updates keep canonical texture and padded matrix identities");
    close(matrices.get("12").toNumber(),.65,"source skeleton matrices contain animated world translation");
    require(texture.get("version").toNumber()==version+1&&state.get("uploads").strictEquals(1)&&state.get("computes").strictEquals(1),"one frame refreshes the existing matrix texture without recomputation");
    call(skin.skeleton,"dispose");require(skin.skeleton.get("boneTexture").isNull(),"canonical skeleton disposal clears its bone texture");
    render(renderer,skin.world,skin.view);expect(renderer,{255,0,0},"disposed bone texture is rebuilt from current matrices",53,32);
    require(!skin.skeleton.get("boneTexture").sameValue(texture)&&state.get("computes").strictEquals(2)&&state.get("updates").strictEquals(3),"source null check lazily recreates the bone texture on a later frame");layer.verify();
}
void weightedBones(bool enabled){
    VulkanRenderer renderer({64,64,enabled});linear(renderer);Validation layer(renderer,enabled);
    Skin skin(skinnedGeometry(.3,3,1));auto other=nativeBoneConstructor().construct();call(skin.object,"add",{other});
    skin.skeleton=nativeSkeletonConstructor().construct({list({skin.bone,other})});call(skin.object,"bind",{skin.skeleton});call(skin.object,"normalizeSkinWeights");
    auto weights=skin.geometry.get("attributes").get("skinWeight");close(call(weights,"getX",{0}).toNumber(),.75,"canonical normalization retains the first weighted bone");close(call(weights,"getY",{0}).toNumber(),.25,"canonical normalization retains the second weighted bone");
    skin.bone.get("position").set("x",.6);other.get("position").set("x",-.2);render(renderer,skin.world,skin.view);
    expect(renderer,{255,0,0},"vertex shader combines two real bone matrices and normalized weights",45,32);expect(renderer,{0,0,0},"weighted skinning leaves the rest position empty");
    const auto rows=weights.get("count").toNumber();for(int i=0;i<int(rows);++i)call(weights,"setXYZW",{i,0,1,0,0});
    render(renderer,skin.world,skin.view);expect(renderer,{255,0,0},"unversioned skinWeight edits preserve the uploaded vertex buffer",45,32);
    weights.set("needsUpdate",true);render(renderer,skin.world,skin.view);expect(renderer,{255,0,0},"versioned weights select the second animated bone",26,32);expect(renderer,{0,0,0},"new weight upload removes prior weighted pixels",45,32);layer.verify();
}
void sharedMaterialSkeletons(bool enabled){
    VulkanRenderer renderer({64,64,enabled});linear(renderer);Validation layer(renderer,enabled);auto material=paint(0x00ff00);Skin left({},material),right({},material);
    left.object.get("position").set("x",-.5);right.object.get("position").set("x",.5);
    // Rebind after placement so each canonical bind matrix describes its rest pose.
    call(left.object,"bind",{left.skeleton});call(right.object,"bind",{right.skeleton});left.object.set("renderOrder",0);right.object.set("renderOrder",1);
    auto world=scene({left.object,right.object});left.bone.get("position").set("y",.4);right.bone.get("position").set("y",-.4);
    render(renderer,world,left.view);expect(renderer,{0,255,0},"first shared-material object samples its own skeleton",16,19);expect(renderer,{0,255,0},"consecutive same-material object binds a different bone texture",48,45);
    expect(renderer,{0,0,0},"second skeleton cannot overwrite the first draw's texture descriptor",16,45);expect(renderer,{0,0,0},"first skeleton cannot leak into the second draw",48,19);
    left.bone.get("position").set("y",-.4);right.bone.get("position").set("y",.4);render(renderer,world,left.view);
    expect(renderer,{0,255,0},"first skeleton animates independently on the next frame",16,45);expect(renderer,{0,255,0},"second skeleton animates independently despite one material",48,19);layer.verify();
}
void sharedSkeletonFrameCache(bool enabled){
    VulkanRenderer renderer({64,64,enabled});linear(renderer);Validation layer(renderer,enabled);
    auto bone=nativeBoneConstructor().construct(),skeleton=nativeSkeletonConstructor().construct({list({bone})});auto shape=skinnedGeometry(),material=paint();
    auto first=nativeSkinnedMeshConstructor().construct({shape,material}),second=nativeSkinnedMeshConstructor().construct({shape,material});
    for(const auto& object:{first,second}){auto value=object;value.set("frustumCulled",false);value.set("bindMode",DetachedBindMode);call(value,"bind",{skeleton,nativeMatrix4Constructor().construct()});}
    first.get("position").set("x",-.5);second.get("position").set("x",.5);first.set("renderOrder",0);second.set("renderOrder",1);
    auto world=scene({bone,first,second}),view=camera(),state=JSValue::object();state.set("updates",0);state.set("callbacks",0);observe(skeleton,"update",state,"updates");
    const auto before=first.get("onBeforeRender");first.set("onBeforeRender",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
        require(self.sameValue(c[0])&&args.size()==6,"deformation callback preserves source object and six render arguments");
        c[1].get("position").set("y",.5);call(c[1],"updateMatrixWorld",{true});auto state=c[2];state.set("callbacks",state.get("callbacks").toNumber()+1);return {};
    },{first,bone,state}));
    render(renderer,world,view);require(state.get("updates").strictEquals(1)&&state.get("callbacks").strictEquals(1),"one skeleton shared by two objects updates exactly once before draw callbacks");
    close(skeleton.get("boneMatrices").get("13").toNumber(),0,"onBeforeRender cannot retroactively refresh cached skeleton matrices");
    expect(renderer,{255,0,0},"first shared skeleton draw uses the projectObject frame snapshot",16,32);expect(renderer,{255,0,0},"second object reuses the same skeleton frame snapshot",48,32);expect(renderer,{0,0,0},"callback bone mutation awaits the next source frame",48,16);
    first.set("onBeforeRender",before);render(renderer,world,view);require(state.get("updates").strictEquals(2),"the next source frame refreshes the skeleton once again");
    expect(renderer,{255,0,0},"next frame commits shared bone animation for the first mesh",16,16);expect(renderer,{255,0,0},"next frame commits shared bone animation for the second mesh",48,16);layer.verify();
}
JSValue morphGeometry(bool relative,bool skin){
    auto shape=skin?skinnedGeometry():nativePlaneGeometryConstructor().construct({.3,.3});auto positions=shape.get("attributes").get("position");Args values;
    for(int i=0;i<int(positions.get("count").toNumber());++i){values.push_back(relative?.7:call(positions,"getX",{i}).toNumber()+.7);values.push_back(relative?0:call(positions,"getY",{i}).toNumber());values.push_back(0);}
    auto target=nativeFloat32BufferAttributeConstructor().construct({list(values),3});target.set("name","shift");shape.get("morphAttributes").set("position",list({target}));shape.set("morphTargetsRelative",relative);return shape;
}
void skinMorphOrder(bool enabled,bool relative){
    VulkanRenderer renderer({64,64,enabled});linear(renderer);Validation layer(renderer,enabled);Skin skin(morphGeometry(relative,true));
    require(skin.object.get("morphTargetDictionary").get("shift").strictEquals(0),"canonical SkinnedMesh initializes the original morph dictionary");
    skin.object.get("morphTargetInfluences").set("0",1);skin.bone.get("rotation").set("z",1.5707963267948966);render(renderer,skin.world,skin.view);
    expect(renderer,{255,0,0},"morph position reaches skinning before the real bone rotation",32,10);expect(renderer,{0,0,0},"morphing after skinning would incorrectly leave pixels on the right",54,32);expect(renderer,{0,0,0},"full morph influence removes the base position");
    auto vertex=nativeVector3Constructor().construct();call(skin.object,"getVertexPosition",{0,vertex});
    close(vertex.get("x").toNumber(),-.15,"source CPU morph then skin vertex x agrees with the GPU fixture");close(vertex.get("y").toNumber(),.55,"source CPU morph then skin vertex y agrees with the GPU fixture");
    skin.object.get("morphTargetInfluences").set("0",0);render(renderer,skin.world,skin.view);expect(renderer,{255,0,0},"a later zero influence restores the rotated base geometry");expect(renderer,{0,0,0},"updated influence removes prior deformed pixels",32,10);layer.verify();
}
void instanceMorphWeights(bool enabled){
    VulkanRenderer renderer({64,64,enabled});linear(renderer);Validation layer(renderer,enabled);auto shape=morphGeometry(false,false),material=paint(0x0000ff);
    auto source=nativeMeshConstructor().construct({shape,material}),object=nativeInstancedMeshConstructor().construct({shape,material,2});object.set("frustumCulled",false);
    call(object,"setMatrixAt",{0,call(nativeMatrix4Constructor().construct(),"makeTranslation",{-.5,0,0})});call(object,"setMatrixAt",{1,call(nativeMatrix4Constructor().construct(),"makeTranslation",{.5,0,0})});object.get("instanceMatrix").set("needsUpdate",true);
    source.get("morphTargetInfluences").set("0",0);require(call(object,"setMorphAt",{0,source}).sameValue(object),"canonical setMorphAt returns its original instance object");source.get("morphTargetInfluences").set("0",1);call(object,"setMorphAt",{1,source});
    auto texture=object.get("morphTexture"),data=texture.get("source").get("data").get("data");
    require(texture.get("version").strictEquals(0)&&texture.get("format").strictEquals(RedFormat)&&texture.get("type").strictEquals(FloatType),"setMorphAt creates canonical R32F influence storage without marking it for upload");
    require(data.get("0").strictEquals(1)&&data.get("1").strictEquals(0)&&data.get("2").strictEquals(0)&&data.get("3").strictEquals(1),"two source texture rows hold distinct base and morph influences");
    texture.set("needsUpdate",true);auto world=scene({object}),view=camera();
    // The shared morph target shifts x by .7; use a y instance translation for
    // the second row so both deformed objects remain inside the camera.
    call(object,"setMatrixAt",{1,call(nativeMatrix4Constructor().construct(),"makeTranslation",{-.2,.5,0})});object.get("instanceMatrix").set("needsUpdate",true);
    render(renderer,world,view);expect(renderer,{0,0,255},"instance zero reads its own base influence row",16,32);expect(renderer,{0,0,255},"instance one reads a distinct morph influence texture row",48,16);expect(renderer,{0,0,0},"unmorphed second instance position stays empty",26,16);
    source.get("morphTargetInfluences").set("0",1);call(object,"setMorphAt",{0,source});source.get("morphTargetInfluences").set("0",0);call(object,"setMorphAt",{1,source});
    render(renderer,world,view);expect(renderer,{0,0,255},"unversioned instance influence edits retain the first uploaded row",16,32);expect(renderer,{0,0,255},"unversioned instance influence edits retain the second uploaded row",48,16);
    texture.set("needsUpdate",true);render(renderer,world,view);expect(renderer,{0,0,255},"versioned instance zero morph moves its actual vertices",38,32);expect(renderer,{0,0,255},"versioned instance one returns to its base position",26,16);expect(renderer,{0,0,0},"new per-instance influence upload removes old instance zero pixels",16,32);
    auto result=nativeMeshConstructor().construct({shape,material});call(object,"getMorphAt",{0,result});require(result.get("morphTargetInfluences").get("0").strictEquals(1),"canonical getMorphAt reads the same original influence data");layer.verify();
}
}
int main(int argc,char** argv){
    try{bool enabled=false;for(int i=1;i<argc;++i)if(std::string(argv[i])=="--validation")enabled=true;
        const auto run=[&](const char* name,void(*group)(bool)){std::cout<<"deformation group="<<name<<std::endl;group(enabled);};
        run("translatedSkeleton",translatedSkeleton);run("weightedBones",weightedBones);run("sharedMaterialSkeletons",sharedMaterialSkeletons);run("sharedSkeletonFrameCache",sharedSkeletonFrameCache);
        std::cout<<"deformation group=skinMorphOrder absolute"<<std::endl;skinMorphOrder(enabled,false);
        std::cout<<"deformation group=skinMorphOrder relative"<<std::endl;skinMorphOrder(enabled,true);run("instanceMorphWeights",instanceMorphWeights);
        std::cout<<"managed deformation assertions="<<assertions<<" images="<<images<<" validation_requested="<<validationRequested<<" validation_enabled="<<validationEnabled<<" validation_messages="<<validationMessages<<'\n';
        if(enabled&&validationEnabled==0)std::cout<<"Khronos validation layer unavailable; GPU checks ran without layer validation.\n";return 0;
    }catch(const std::exception& error){std::cerr<<"Managed Vulkan deformation failed: "<<error.what()<<'\n';return 1;}
}
