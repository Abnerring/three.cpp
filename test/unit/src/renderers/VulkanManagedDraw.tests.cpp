#include "renderers/VulkanRenderer.h"
#include "core/BufferGeometry.h"
#include "core/BufferAttribute.h"
#include "core/JSJobs.h"
#include "scenes/Scene.h"
#include "cameras/OrthographicCamera.h"
#include "materials/MeshBasicMaterial.h"
#include "materials/RawShaderMaterial.h"
#include "objects/Mesh.h"
#include "objects/InstancedMesh.h"
#include "math/Matrix4.h"
#include "math/Color.h"
#include "math/Plane.h"
#include "math/Vector3.h"
#include "loaders/ObjectLoader.h"
#include "animation/AnimationUtils.h"
#include "core/RenderTarget.h"
#include "geometries/PlaneGeometry.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using Args=std::vector<JSValue>;
int assertions=0;
int validationRequested=0,validationEnabled=0,validationMessages=0;
void require(bool value,const char* message){++assertions;if(!value)throw std::runtime_error(message);}
struct ValidationCheck {
    const VulkanRenderer& renderer;bool requested;
    ValidationCheck(const VulkanRenderer& value,bool validation):renderer(value),requested(validation){if(requested)++validationRequested;if(renderer.getInfo().validationEnabled)++validationEnabled;}
    void verify(){const auto messages=renderer.getValidationMessages();validationMessages+=int(messages.size());for(const auto& message:messages)std::cerr<<"Vulkan validation: "<<message<<'\n';require(messages.empty(),"managed draw emits no collected validation errors");}
};
JSValue call(const JSValue& self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
JSValue list(const Args& values={}){auto result=JSValue::array();for(const auto& value:values)result.set(JSValue(result.length()),value);return result;}
void push(JSValue list,const JSValue& value){list.set(JSValue(list.length()),value);}
JSValue geometry(double scale=.8){
    auto value=nativeBufferGeometryConstructor().construct();
    auto positions=nativeFloat32BufferAttributeConstructor().construct({list({-scale,-scale,0,scale,-scale,0,0,scale,0}),3});
    call(value,"setAttribute",{"position",positions});return value;
}
JSValue material(unsigned color=0xff0000){auto p=JSValue::object();p.set("color",color);return nativeMeshBasicMaterialConstructor().construct({p});}
JSValue camera(){auto result=nativeOrthographicCameraConstructor().construct({-1,1,1,-1,.1,10});result.get("position").set("z",3);return result;}
struct World {
    JSValue scene=nativeSceneConstructor().construct(),view=camera(),shape=geometry(),paint=material(),mesh;
    World(){mesh=nativeMeshConstructor().construct({shape,paint});mesh.set("frustumCulled",false);call(scene,"add",{mesh});}
};
std::array<int,4> pixel(const VulkanRenderer& renderer,unsigned x=32,unsigned y=32){
    const auto bytes=renderer.readPixels();const auto i=(y*renderer.getWidth()+x)*4;
    require(bytes.size()==renderer.getWidth()*renderer.getHeight()*4,"complete managed RGBA readback");return {bytes.at(i),bytes.at(i+1),bytes.at(i+2),bytes.at(i+3)};
}
void expectPixel(const VulkanRenderer& renderer,std::array<int,3> expected,const char* message,unsigned x=32,unsigned y=32){
    const auto actual=pixel(renderer,x,y);if(std::abs(actual[0]-expected[0])>2||std::abs(actual[1]-expected[1])>2||std::abs(actual[2]-expected[2])>2){
        std::cerr<<message<<": "<<actual[0]<<','<<actual[1]<<','<<actual[2]<<'\n';require(false,message);
    }require(true,message);
}
void baselineAndVersion(bool validation){
    VulkanRenderer renderer({64,64,validation});renderer.srgbOutput=false;World world;
    ValidationCheck layer(renderer,validation);
    require(!world.scene.isHostObject("three.Object3D"),"scene retains its original raw source identity");
    require(!world.mesh.isHostObject("three.Object3D"),"mesh is not converted into a typed Object3D");
    require(!world.shape.isHostObject("three.BufferGeometry"),"geometry retains its raw source identity");
    require(!world.paint.isHostObject("three.Material"),"material is not converted to a typed Material");
    const auto facade=renderer.object();require(renderer.object().sameValue(facade),"renderer facade identity is stable");
    auto state=JSValue::object();state.set("uploads",0);auto positions=world.shape.get("attributes").get("position");
    call(positions,"onUpload",{JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue {auto state=c[0];state.set("uploads",state.get("uploads").toNumber()+1);state.set("receiver",self.sameValue(c[1]));return {};},{state,positions})});
    require(renderer.render(world.scene,world.view).isUndefined(),"render returns the original undefined result");
    expectPixel(renderer,{255,0,0},"raw native source mesh reaches the Vulkan framebuffer");
    require(state.get("uploads").toNumber()==1&&state.get("receiver").toBoolean(),"real WebGLAttributes upload preserves callback receiver");
    auto array=positions.get("array");for(double i=0;i<array.get("length").toNumber();i+=3)array.set(JSValue(i),array.get(JSValue(i)).toNumber()+3);
    renderer.render(world.scene,world.view);expectPixel(renderer,{255,0,0},"array writes without a version update retain uploaded GPU bytes");
    require(state.get("uploads").toNumber()==1,"unchanged source version does not upload again");
    call(positions,"addUpdateRange",{0,array.get("length")});positions.set("needsUpdate",true);
    renderer.render(world.scene,world.view);expectPixel(renderer,{0,0,0},"needsUpdate and updateRanges update the same managed GPU resource");
    require(state.get("uploads").toNumber()==2,"source update callback runs for the committed range upload");
    require(positions.get("updateRanges").get("length").strictEquals(0),"real upload clears original source update ranges");
    for(double i=0;i<array.get("length").toNumber();i+=3)array.set(JSValue(i),array.get(JSValue(i)).toNumber()-3);
    positions.set("needsUpdate",true);renderer.render(world.scene,world.view);expectPixel(renderer,{255,0,0},"updated raw positions restore the original triangle");
    const auto beforeDispose=renderer.getInfo().attributeUploadBuffers;call(world.shape,"dispose");
    require(renderer.getInfo().attributeUploadBuffers<beforeDispose,"source disposal releases its real Vulkan upload storage");renderer.render(world.scene,world.view);
    expectPixel(renderer,{255,0,0},"source geometry disposal permits a later real re-upload");require(renderer.getInfo().attributeUploadBuffers==beforeDispose,"real Vulkan upload storage is allocated again after disposal");
    require(state.get("uploads").strictEquals(4),"disposal requires a fresh real onUpload callback even without another version change");
    layer.verify();
}
void colorsMirroringAndIndex(bool validation){
    VulkanRenderer renderer({64,64,validation});renderer.srgbOutput=false;World world;call(world.paint.get("color"),"setHex",{0xffffff});world.paint.set("vertexColors",true);
    ValidationCheck layer(renderer,validation);
    const auto colors=nativeFloat32BufferAttributeConstructor().construct({list({0,1,0,0,1,0,0,1,0}),3});call(world.shape,"setAttribute",{"color",colors});
    renderer.render(world.scene,world.view);expectPixel(renderer,{0,255,0},"source vertex color bytes bind to the Basic shader");
    world.mesh.get("scale").set("x",-1);renderer.render(world.scene,world.view);expectPixel(renderer,{0,255,0},"raw determinantAffine preserves mirrored front faces");
    world.paint.set("side",BackSide);world.paint.set("needsUpdate",true);renderer.render(world.scene,world.view);expectPixel(renderer,{0,0,0},"BackSide culls the mirrored front face");
    world.paint.set("side",DoubleSide);world.paint.set("needsUpdate",true);renderer.render(world.scene,world.view);expectPixel(renderer,{0,255,0},"DoubleSide renders mirrored geometry");
    world.mesh.get("scale").set("x",1);world.paint.set("vertexColors",false);world.paint.set("side",FrontSide);call(world.paint.get("color"),"setHex",{0xff0000});world.paint.set("needsUpdate",true);
    const auto square=nativeBufferGeometryConstructor().construct();call(square,"setAttribute",{"position",nativeFloat32BufferAttributeConstructor().construct({list({-.8,-.8,0,.8,-.8,0,.8,.8,0,-.8,.8,0}),3})});
    call(square,"setIndex",{list({0,1,2,2,3,0})});call(square,"setDrawRange",{-3,6});world.mesh.set("geometry",square);
    renderer.render(world.scene,world.view);expectPixel(renderer,{255,0,0},"raw indexed range clamps its start to zero",48,48);expectPixel(renderer,{0,0,0},"clamping preserves the original range end",16,16);
    call(square,"setDrawRange",{0,6});renderer.render(world.scene,world.view);expectPixel(renderer,{255,0,0},"the remaining raw index primitive draws after changing drawRange",16,16);
    auto index=square.get("index");auto indexArray=index.get("array");for(double i=0;i<6;++i)indexArray.set(JSValue(i),0);index.set("needsUpdate",true);
    renderer.render(world.scene,world.view);expectPixel(renderer,{0,0,0},"raw index GPU bytes follow the authoritative upload revision",48,48);
    layer.verify();
}
void instances(bool validation){
    VulkanRenderer renderer({64,64,validation});renderer.srgbOutput=false;auto scene=nativeSceneConstructor().construct(),view=camera(),shape=geometry(.25),paint=material(0xffffff);
    ValidationCheck layer(renderer,validation);
    auto mesh=nativeInstancedMeshConstructor().construct({shape,paint,2});mesh.set("frustumCulled",false);call(scene,"add",{mesh});
    auto matrix=nativeMatrix4Constructor().construct();call(matrix,"makeTranslation",{-.5,0,0});call(mesh,"setMatrixAt",{0,matrix});call(matrix,"makeTranslation",{.5,0,0});call(mesh,"setMatrixAt",{1,matrix});
    call(mesh,"setColorAt",{0,nativeColorConstructor().construct({0xff0000})});call(mesh,"setColorAt",{1,nativeColorConstructor().construct({0x0000ff})});
    renderer.render(scene,view);expectPixel(renderer,{255,0,0},"source instanceMatrix and instanceColor bind for the first instance",16,32);expectPixel(renderer,{0,0,255},"source instance divisors select the second row",48,32);
    mesh.set("count",1);renderer.render(scene,view);expectPixel(renderer,{255,0,0},"raw instance count retains the first instance",16,32);expectPixel(renderer,{0,0,0},"raw instance count omits the second instance",48,32);
    mesh.set("count",0);renderer.render(scene,view);expectPixel(renderer,{0,0,0},"strict zero instance count skips the GPU command",16,32);
    layer.verify();
}
void groupsClippingAndWireframe(bool validation){
    VulkanRenderer renderer({64,64,validation});renderer.srgbOutput=false;World world;
    ValidationCheck layer(renderer,validation);
    auto square=nativeBufferGeometryConstructor().construct();call(square,"setAttribute",{"position",nativeFloat32BufferAttributeConstructor().construct({list({-.8,-.8,0,.8,-.8,0,.8,.8,0,-.8,.8,0}),3})});call(square,"setIndex",{list({0,1,2,2,3,0})});
    call(square,"addGroup",{0,3,0});call(square,"addGroup",{3,3,1});world.mesh.set("geometry",square);world.mesh.set("material",list({world.paint,material(0x00ff00)}));
    auto events=list();const auto groups=square.get("groups");world.mesh.set("onBeforeRender",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
        require(a[5].sameValue(c[1])||a[5].sameValue(c[2]),"draw groups retain their original source object identity");push(c[0],a[5].sameValue(c[1])?0:1);return {};
    },{events,groups.get("0"),groups.get("1")}));
    renderer.render(world.scene,world.view);expectPixel(renderer,{255,0,0},"first indexed group selects its original red material",48,48);expectPixel(renderer,{0,255,0},"second indexed group selects its original green material",16,16);
    require(events.toJSON()->dump()==list({0,1}).toJSON()->dump(),"source group order reaches the raw draw boundary");
    world.mesh.set("onBeforeRender",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};}));world.mesh.set("material",world.paint);world.mesh.set("geometry",world.shape);
    auto plane=nativePlaneConstructor().construct({nativeVector3Constructor().construct({1,0,0}),0});auto planes=list({plane});renderer.clippingPlanes=planes;auto facade=renderer.object();facade.set("clippingPlanes",planes);
    renderer.render(world.scene,world.view);expectPixel(renderer,{0,0,0},"raw global clipping rejects the negative half space",24,32);expectPixel(renderer,{255,0,0},"raw global clipping retains the positive half space",40,32);
    renderer.clippingPlanes=list();facade.set("clippingPlanes",renderer.clippingPlanes);world.paint.set("wireframe",true);world.paint.set("needsUpdate",true);
    renderer.render(world.scene,world.view);expectPixel(renderer,{0,0,0},"source wireframe indices leave triangle interiors empty");
    const auto bytes=renderer.readPixels();bool visibleEdge=false;for(std::size_t i=0;i<bytes.size();i+=4)if(bytes[i]>200&&bytes[i+1]<20&&bytes[i+2]<20)visibleEdge=true;
    require(visibleEdge,"generated source wireframe attribute produces actual GPU line fragments");
    layer.verify();
}
void sharedUUIDs(bool validation){
    VulkanRenderer renderer({64,64,validation});renderer.srgbOutput=false;auto scene=nativeSceneConstructor().construct(),view=camera(),first=geometry(.25),second=geometry(.25);
    ValidationCheck layer(renderer,validation);
    first.set("uuid","shared-geometry-uuid");second.set("uuid","shared-geometry-uuid");first.get("attributes").get("position").set("uuid","shared-attribute-uuid");second.get("attributes").get("position").set("uuid","shared-attribute-uuid");
    auto a=nativeMeshConstructor().construct({first,material(0xff0000)}),b=nativeMeshConstructor().construct({second,material(0x0000ff)});a.set("frustumCulled",false);b.set("frustumCulled",false);a.get("position").set("x",-.5);b.get("position").set("x",.5);call(scene,"add",{a,b});
    renderer.render(scene,view);expectPixel(renderer,{255,0,0},"shared UUIDs preserve the first resource's uploaded bytes",16,32);expectPixel(renderer,{0,0,255},"shared UUIDs cannot alias a distinct managed buffer",48,32);
    layer.verify();
}
void loadedScene(bool validation){
    VulkanRenderer renderer({64,64,validation});renderer.srgbOutput=false;
    ValidationCheck layer(renderer,validation);
    const auto json=Animation_detail::parseJSON(R"JSON({
      "metadata":{"version":4.7,"type":"Object","generator":"Object3D.toJSON"},
      "geometries":[{"uuid":"shared-geometry","type":"BufferGeometry","data":{"attributes":{"position":{"itemSize":3,"type":"Float32Array","array":[-0.25,-0.25,0,0.25,-0.25,0,0,0.25,0],"normalized":false}}}}],
      "materials":[{"uuid":"shared-material","type":"MeshBasicMaterial","color":16711680}],
      "object":{"uuid":"loaded-scene","type":"Scene","children":[
        {"uuid":"loaded-camera","type":"OrthographicCamera","left":-1,"right":1,"top":1,"bottom":-1,"near":0.1,"far":10,"matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0,0,3,1]},
        {"uuid":"left-mesh","type":"Mesh","geometry":"shared-geometry","material":"shared-material","frustumCulled":false,"matrix":[1,0,0,0,0,1,0,0,0,0,1,0,-0.5,0,0,1]},
        {"uuid":"right-mesh","type":"Mesh","geometry":"shared-geometry","material":"shared-material","frustumCulled":false,"matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0.5,0,0,1]}
      ]}
    })JSON");
    const auto loader=nativeObjectLoaderConstructor().construct(),scene=call(loader,"parse",{json}),view=scene.get("children").get("0"),first=scene.get("children").get("1"),second=scene.get("children").get("2");
    require(!scene.isHostObject("three.Object3D"),"ObjectLoader returns the original managed scene graph");
    require(first.get("geometry").sameValue(second.get("geometry")),"ObjectLoader geometry cache identity survives into rendering");
    require(first.get("material").sameValue(second.get("material")),"ObjectLoader material cache identity survives into rendering");
    renderer.render(scene,view);expectPixel(renderer,{255,0,0},"loaded scene renders its first mesh through the original geometry",16,32);expectPixel(renderer,{255,0,0},"loaded scene renders its shared-resource second mesh",48,32);
    call(first.get("material").get("color"),"setHex",{0x00ff00});renderer.render(scene,view);
    expectPixel(renderer,{0,255,0},"mutating the loaded original material changes its first GPU draw",16,32);expectPixel(renderer,{0,255,0},"the second draw observes the same loaded material identity",48,32);
    layer.verify();
}
void callbacks(bool validation){
    VulkanRenderer renderer({64,64,validation});renderer.srgbOutput=false;World world;auto events=list();const auto facade=renderer.object();
    ValidationCheck layer(renderer,validation);
    const auto callback=[&](const char* label,JSValue expected,int count,bool isMaterial=false){
        return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue {
            require(self.sameValue(c[2]),"raw lifecycle callback preserves its original receiver");require(a.size()==std::size_t(c[3].toNumber()),"raw lifecycle callback has the source argument count");
            require(a[0].sameValue(c[4])&&a[1].sameValue(c[5])&&a[2].sameValue(c[6]),"callback renderer/scene/camera aliases are preserved");
            if(a.size()==6){require(a[3].sameValue(c[7]),"callback receives the original geometry");require(a[4].sameValue(c[8]),"callback receives the original object or material");require(a[5].isNull(),"un-grouped draw passes the original null group");}
            push(c[0],c[1]);return {};
        },{events,label,expected,count,facade,world.scene,world.view,world.shape,isMaterial?world.mesh:world.paint});
    };
    world.scene.set("onBeforeRender",callback("scene.before",world.scene,4));world.scene.set("onAfterRender",callback("scene.after",world.scene,3));
    world.mesh.set("onBeforeRender",callback("object.before",world.mesh,6));world.mesh.set("onAfterRender",callback("object.after",world.mesh,6));world.paint.set("onBeforeRender",callback("material.before",world.paint,6,true));
    call(world.shape.get("attributes").get("position"),"onUpload",{JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"upload");return {};},{events})});
    renderer.render(world.scene,world.view);const auto expected=list({"scene.before","upload","object.before","material.before","object.after","scene.after"});
    require(events.toJSON()->dump()==expected.toJSON()->dump(),"scene/upload/object/material callback order matches the source pipeline");expectPixel(renderer,{255,0,0},"lifecycle callbacks feed a real GPU draw");
    layer.verify();
}
void nestedRendering(bool validation){
    VulkanRenderer renderer({64,64,validation});renderer.srgbOutput=false;renderer.autoClear=false;auto facade=renderer.object();facade.set("autoClear",false);renderer.setClearColor(Color(0,0,0));renderer.clear();
    ValidationCheck layer(renderer,validation);
    World outer,inner;call(inner.paint.get("color"),"setHex",{0x0000ff});outer.mesh.get("scale").set("x",.4);outer.mesh.get("scale").set("y",.4);outer.mesh.get("position").set("z",.2);
    auto state=JSValue::object();state.set("calls",0);
    outer.mesh.set("onBeforeRender",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {auto state=c[0];state.set("calls",state.get("calls").toNumber()+1);call(c[1],"render",{c[2],c[3]});return {};},{state,facade,inner.scene,inner.view}));
    renderer.render(outer.scene,outer.view);require(state.get("calls").strictEquals(1),"nested render invokes the outer callback once");
    expectPixel(renderer,{255,0,0},"outer draw resumes after a nested managed render");expectPixel(renderer,{0,0,255},"nested draw remains in the actual Vulkan command stream",20,40);
    layer.verify();
}
void clearAndMetadata(bool validation){
    for(const bool alpha:{false,true}){
        VulkanRenderer renderer({64,64,validation,alpha,true});renderer.srgbOutput=false;auto facade=renderer.object();auto scene=nativeSceneConstructor().construct(),view=camera();
        ValidationCheck layer(renderer,validation);
        renderer.render(scene,view);require(pixel(renderer)[3]==(alpha?0:255),"default null background honors the context alpha capability");
        const auto color=nativeColorConstructor().construct({.2,.4,.6});call(facade,"setClearColor",{color,.5});call(facade,"clear");
        const auto actual=pixel(renderer);require(std::abs(actual[0]-26)<=2&&std::abs(actual[1]-51)<=2&&std::abs(actual[2]-77)<=2,"premultiplied clear color multiplies raw linear RGB by alpha");
        require(std::abs(actual[3]-128)<=1,"explicit setClearColor alpha reaches the Vulkan color attachment");
        layer.verify();
    }
    VulkanRenderer renderer({64,64,validation,true,false});renderer.srgbOutput=false;auto facade=renderer.object();
    ValidationCheck layer(renderer,validation);
    call(facade,"setClearColor",{nativeColorConstructor().construct({.2,.4,.6}),.5});call(facade,"clear");const auto straight=pixel(renderer);
    require(std::abs(straight[0]-51)<=2&&std::abs(straight[1]-102)<=2&&std::abs(straight[2]-153)<=2,"non-premultiplied context preserves straight clear RGB");
    const auto render=facade.get("render");require(render.get("name").strictEquals(""),"render retains the source's anonymous function name");require(render.get("length").strictEquals(2),"render retains the source arity");require(render.isConstructible(),"source function expression retains its constructor capability");
    const auto source=call(render,"toString").toString();require(source.starts_with("function ( scene, camera )"),"render exposes its original function-expression source text");
    auto viewportTarget=JSValue::object(),viewportState=JSValue::object(),viewportToken=JSValue::object();viewportState.set("calls",0);
    viewportTarget.set("copy",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue {
        require(self.sameValue(c[1]),"viewport getter calls the supplied target.copy with the original receiver");require(a.size()==1,"viewport copy receives exactly the original private vector");
        auto state=c[0];state.set("calls",state.get("calls").toNumber()+1);state.set("vector",a[0]);return c[2];
    },{viewportState,viewportTarget,viewportToken}));
    require(call(facade,"getViewport",{viewportTarget}).sameValue(viewportToken),"getViewport returns the original target.copy result unchanged");const auto privateViewport=viewportState.get("vector");
    require(privateViewport.get("x").strictEquals(0)&&privateViewport.get("y").strictEquals(0)&&privateViewport.get("z").strictEquals(64)&&privateViewport.get("w").strictEquals(64),"getViewport passes the actual initial private viewport vector");
    renderer.setViewport(2,3,40,41);require(call(facade,"getViewport",{viewportTarget}).sameValue(viewportToken),"updated viewport retains the raw copy return token");
    require(viewportState.get("vector").sameValue(privateViewport),"C++ setViewport updates the same private raw viewport vector");
    require(privateViewport.get("x").strictEquals(2)&&privateViewport.get("y").strictEquals(3)&&privateViewport.get("z").strictEquals(40)&&privateViewport.get("w").strictEquals(41),"escaped raw viewport aliases observe C++ viewport changes");
    require(call(facade,"getCurrentViewport",{viewportTarget}).sameValue(viewportToken),"getCurrentViewport returns the original target.copy result");const auto privateCurrent=viewportState.get("vector");
    renderer.setViewport(4,5,32,33);call(facade,"getCurrentViewport",{viewportTarget});require(viewportState.get("vector").sameValue(privateCurrent),"current viewport copy receives a stable private vector identity");
    require(privateCurrent.get("x").strictEquals(4)&&privateCurrent.get("y").strictEquals(5)&&privateCurrent.get("z").strictEquals(32)&&privateCurrent.get("w").strictEquals(33),"current viewport alias follows C++ viewport changes");renderer.setViewport(0,0,64,64);
    World world;auto other=JSValue::object(facade),otherInfo=JSValue::object(),otherRender=JSValue::object();otherRender.set("frame",10);otherInfo.set("render",otherRender);otherInfo.set("autoReset",false);other.set("info",otherInfo);
    const auto oldFrame=facade.get("info").get("render").get("frame");auto aliases=JSValue::object();aliases.set("facade",false);
    world.scene.set("onBeforeRender",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{auto state=c[0];state.set("facade",a[0].sameValue(c[1]));return {};},{aliases,facade}));
    require(render.call(other,{world.scene,world.view}).isUndefined(),"borrowed render preserves the original return value");require(otherRender.get("frame").strictEquals(11),"render.call(other) reads this.info from the supplied receiver");
    require(facade.get("info").get("render").get("frame").sameValue(oldFrame),"borrowed render does not increment the original renderer frame");require(aliases.get("facade").toBoolean(),"borrowed render callbacks retain the lexical original renderer alias");expectPixel(renderer,{255,0,0},"borrowed render submits to the lexical Vulkan backend");
    facade.set("autoClear",false);world.mesh.get("position").set("z",1);renderer.render(world.scene,world.view);
    world.paint.set("colorWrite",false);world.paint.set("depthWrite",false);renderer.render(world.scene,world.view);
    call(facade,"clear",{false,true,false});world.paint.set("colorWrite",true);call(world.paint.get("color"),"setHex",{0x0000ff});world.mesh.get("position").set("z",-1);renderer.render(world.scene,world.view);
    expectPixel(renderer,{0,0,255},"explicit depth clear restores the writable depth mask after a depthWrite=false draw");
    layer.verify();
}
void capabilityErrorsAndLifetime(bool validation){
    VulkanRenderer renderer({64,64,validation});renderer.srgbOutput=false;World world;auto facade=renderer.object();
    ValidationCheck layer(renderer,validation);
    // Valid canonical Uint32 geometry and GLSL300 integer vertex inputs now
    // execute on the production path; the sample stays inside the triangle.
    const auto integerGeometry=nativeBufferGeometryConstructor().construct();
    const auto integerPositions=nativeUint32BufferAttributeConstructor().construct({list({0,0,0,1,0,0,0,1,0}),3});
    call(integerGeometry,"setAttribute",{"position",integerPositions});
    auto parameters=JSValue::object();parameters.set("glslVersion","300 es");
    parameters.set("vertexShader","precision highp float;precision highp int;in uvec3 position;uniform mat4 projectionMatrix;uniform mat4 modelViewMatrix;void main(){gl_Position=projectionMatrix*modelViewMatrix*vec4(vec3(position)-vec3(.5,.5,0.),1.);}");
    parameters.set("fragmentShader","precision highp float;out vec4 result;void main(){result=vec4(0.,1.,0.,1.);}");
    const auto integerMaterial=nativeRawShaderMaterialConstructor().construct({parameters});
    world.mesh.set("geometry",integerGeometry);world.mesh.set("material",integerMaterial);renderer.render(world.scene,world.view);
    expectPixel(renderer,{0,255,0},"canonical integer shader input executes through the native Vulkan vertex format",20,40);
    world.mesh.set("geometry",world.shape);world.mesh.set("material",world.paint);
    // The source renderBufferDirect requests an actual width of two for this
    // canonical material. The Vulkan device does not enable the wideLines feature.
    world.paint.set("wireframe",true);world.paint.set("wireframeLinewidth",2);bool rejected=false;
    try{renderer.render(world.scene,world.view);}catch(const std::logic_error& error){rejected=std::string(error.what()).find("wideLines feature")!=std::string::npos;}
    require(rejected,"source wireframe width reaches the explicit unsupported Vulkan wideLines boundary");
    world.paint.set("wireframe",false);world.paint.set("wireframeLinewidth",1);renderer.render(world.scene,world.view);expectPixel(renderer,{255,0,0},"supported geometry and material recover after the native wide-line exception");
    auto target=std::make_shared<RenderTarget>(32,32);renderer.setRenderTarget(target);rejected=false;try{renderer.render(world.scene,world.view);}catch(const std::exception& error){rejected=std::string(error.what()).find("target")!=std::string::npos||std::string(error.what()).find("Target")!=std::string::npos;}
    require(rejected,"raw rendering cannot silently draw into an unrelated typed render target");renderer.setRenderTarget(nullptr);renderer.render(world.scene,world.view);expectPixel(renderer,{255,0,0},"raw screen rendering resumes after restoring the typed target");
    JSValue escaped;{VulkanRenderer temporary({16,16,validation});ValidationCheck temporaryLayer(temporary,validation);escaped=temporary.object();temporaryLayer.verify();}
    bool retired=false;try{call(escaped,"render",{world.scene,world.view});}catch(const JSTypeError&){retired=true;}
    require(retired,"escaped renderer callbacks reject retired native capabilities safely");
    layer.verify();
}
void typedReverseDoesNotLeak(bool validation){
    VulkanRenderer renderer({64,64,validation});renderer.srgbOutput=false;
    ValidationCheck layer(renderer,validation);
    auto typedScene=std::make_shared<Scene>();auto typedPaint=std::make_shared<MeshBasicMaterial>(Color(0,0,1));typedScene->add(std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(1.6,1.6),typedPaint));
    auto typedCamera=std::make_shared<OrthographicCamera>(-1,1,1,-1,.1,10);typedCamera->position.z=3;typedCamera->reversedDepth=true;typedCamera->updateProjectionMatrix();
    renderer.render(*typedScene,*typedCamera);expectPixel(renderer,{0,0,255},"typed reversed-depth draw establishes a different device convention");
    World raw;renderer.render(raw.scene,raw.view);expectPixel(renderer,{255,0,0},"raw renderer normal depth convention is independent of a preceding typed reversed-depth draw");
    layer.verify();
}
void indexUploadUniformOrder(bool validation){
    VulkanRenderer renderer({64,64,validation});renderer.srgbOutput=false;World world;call(world.shape,"setIndex",{list({0,1,2})});
    ValidationCheck layer(renderer,validation);
    auto state=JSValue::object();state.set("uploads",0);auto index=world.shape.get("index");
    call(index,"onUpload",{JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        auto state=c[0];state.set("uploads",state.get("uploads").toNumber()+1);call(c[1].get("color"),"setHex",{0x0000ff});auto paint=c[1];paint.set("depthWrite",false);return {};
    },{state,world.paint})});
    renderer.render(world.scene,world.view);require(state.get("uploads").strictEquals(1),"index upload callback executes at the binding setup boundary");
    expectPixel(renderer,{255,0,0},"index onUpload cannot retroactively change uniforms sampled by setProgram");
    require(world.paint.get("color").get("b").strictEquals(1)&&world.paint.get("depthWrite").strictEquals(false),"index callback mutates the original raw material for later draws");
    renderer.render(world.scene,world.view);expectPixel(renderer,{0,0,255},"the next draw observes material changes made by the index upload callback");
    layer.verify();
}
void directReentry(bool validation){
    VulkanRenderer renderer({64,64,validation});renderer.srgbOutput=false;World world,inner;auto facade=renderer.object();renderer.render(world.scene,world.view);
    ValidationCheck layer(renderer,validation);
    // trace-webgl-renderer-reentry.mjs executes the unchanged pinned function:
    // renderBufferDirect outside render has no currentRenderState and throws.
    bool outsideRejected=false;
    try{call(facade,"renderBufferDirect",{world.view,world.scene,world.shape,world.paint,world.mesh,nullptr});}catch(const JSTypeError&){outsideRejected=true;}
    require(outsideRejected,"standalone renderBufferDirect preserves the pinned missing-render-state TypeError");
    facade.set("autoClear",false);call(inner.paint.get("color"),"setHex",{0x0000ff});auto info=facade.get("info"),originalUpdate=info.get("update"),state=JSValue::object();state.set("active",false);state.set("nested",0);
    info.set("update",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue {
        const auto result=c[0].call(self,a);auto state=c[1];if(!state.get("active").toBoolean()){state.set("active",true);state.set("nested",state.get("nested").toNumber()+1);call(c[2],"render",{c[3],c[4]});state.set("active",false);}return result;
    },{originalUpdate,state,facade,inner.scene,inner.view}));
    renderer.render(world.scene,world.view);info.set("update",originalUpdate);
    require(state.get("nested").strictEquals(1),"source render supports info.update reentry exactly once");expectPixel(renderer,{0,0,255},"nested render from info.update stays in the same live direct command stream");
    call(world.shape,"setIndex",{list({0,1,2})});auto index=world.shape.get("index");auto uploads=JSValue::object();uploads.set("nested",0);
    call(index,"onUpload",{JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {auto state=c[0];state.set("nested",state.get("nested").toNumber()+1);call(c[1],"render",{c[2],c[3]});return {};},{uploads,facade,inner.scene,inner.view})});
    renderer.render(world.scene,world.view);
    require(uploads.get("nested").strictEquals(1),"source draw permits index upload to reenter source render");expectPixel(renderer,{0,0,255},"draw uses the actual current program and uniforms left by index upload reentry");
    layer.verify();
}
}
int main(int argc,char** argv){
    try{bool validation=false;for(int i=1;i<argc;++i)if(std::string(argv[i])=="--validation")validation=true;
        const auto run=[&](const char* name,void(*group)(bool)){std::cout<<"managed group="<<name<<std::endl;group(validation);};
        run("baselineAndVersion",baselineAndVersion);
        run("colorsMirroringAndIndex",colorsMirroringAndIndex);
        run("instances",instances);
        run("groupsClippingAndWireframe",groupsClippingAndWireframe);
        run("sharedUUIDs",sharedUUIDs);
        run("loadedScene",loadedScene);
        run("callbacks",callbacks);
        run("nestedRendering",nestedRendering);
        run("clearAndMetadata",clearAndMetadata);
        run("capabilityErrorsAndLifetime",capabilityErrorsAndLifetime);
        run("typedReverseDoesNotLeak",typedReverseDoesNotLeak);
        run("indexUploadUniformOrder",indexUploadUniformOrder);
        run("directReentry",directReentry);
        std::cout<<"managed Vulkan assertions="<<assertions<<" validation_requested="<<validationRequested<<" validation_enabled="<<validationEnabled<<" validation_messages="<<validationMessages<<'\n';
        if(validation&&validationEnabled==0)std::cout<<"Khronos validation layer unavailable; GPU rendering checks ran without layer validation.\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<"Managed Vulkan draw failed: "<<error.what()<<'\n';return 1;}
}
