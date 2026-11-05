#include "renderers/VulkanRenderer.h"
#include "textures/CubeTexture.h"
#include "textures/DataTexture.h"
#include "textures/Data3DTexture.h"
#include "textures/DataArrayTexture.h"
#include "materials/ShaderMaterial.h"
#include "materials/RawShaderMaterial.h"
#include "materials/MeshBasicMaterial.h"
#include "core/BufferAttribute.h"
#include "core/NativeArray.h"
#include "core/JSJobs.h"
#include "core/JSPromise.h"
#include "scenes/Scene.h"
#include "cameras/OrthographicCamera.h"
#include "geometries/PlaneGeometry.h"
#include "objects/Mesh.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using Args=std::vector<JSValue>;
int assertions=0;
void require(bool value,const char* message){++assertions;if(!value)throw std::runtime_error(message);}
JSValue call(JSValue object,const char* name,const Args& args={}){return object.get(name).call(object,args);}
JSValue list(const Args& values={}){auto result=JSValue::array();for(const auto& v:values)result.set(JSValue(result.length()),v);return result;}
void push(JSValue a,JSValue value){a.set(JSValue(a.length()),value);}
JSValue fields(std::initializer_list<std::pair<const char*,JSValue>> values){auto result=JSValue::object();for(const auto& [key,v]:values)result.set(key,v);return result;}
JSValue bytes(std::initializer_list<std::uint8_t> values){return NativeArray_detail::wrap(Uint8Array(values));}
JSValue camera(){auto result=nativeOrthographicCameraConstructor().construct({-1,1,1,-1,.1,10});result.get("position").set("z",3);return result;}
JSValue mesh(JSValue geometry,JSValue material,double x=0){auto result=nativeMeshConstructor().construct({geometry,material});result.set("frustumCulled",false);result.get("position").set("x",x);return result;}
JSValue scene(const Args& meshes){auto result=nativeSceneConstructor().construct();call(result,"add",meshes);return result;}
void linear(VulkanRenderer& r){r.srgbOutput=false;r.object().set("outputColorSpace",LinearSRGBColorSpace);}
void expect(const VulkanRenderer& r,std::array<int,3> wanted,const char* message,unsigned x=32,unsigned y=32){
    const auto data=r.readPixels();require(data.size()==r.getWidth()*r.getHeight()*4,"complete sampler-shape GPU readback");const auto i=(y*r.getWidth()+x)*4;
    if(std::abs(int(data.at(i))-wanted[0])>2||std::abs(int(data.at(i+1))-wanted[1])>2||std::abs(int(data.at(i+2))-wanted[2])>2){
        std::cerr<<message<<" actual="<<int(data.at(i))<<','<<int(data.at(i+1))<<','<<int(data.at(i+2))<<'\n';require(false,message);
    }require(true,message);
}
void validate(const VulkanRenderer& r){const auto messages=r.getValidationMessages();for(const auto& m:messages)std::cerr<<m<<'\n';require(messages.empty(),"no collected sampler-shape validation errors");}
const char* rawVertex="precision highp float;precision highp int;in vec3 position;uniform mat4 projectionMatrix;uniform mat4 modelViewMatrix;void main(){gl_Position=projectionMatrix*modelViewMatrix*vec4(position,1.);}";
JSValue raw(const char* fragment,JSValue uniforms){return nativeRawShaderMaterialConstructor().construct({fields({{"glslVersion",GLSL3},{"vertexShader",rawVertex},{"fragmentShader",fragment},{"uniforms",uniforms}})});}
JSValue shader(const char* fragment,JSValue uniforms){return nativeShaderMaterialConstructor().construct({fields({{"glslVersion",GLSL3},{"vertexShader","void main(){gl_Position=projectionMatrix*modelViewMatrix*vec4(position,1.);}"},{"fragmentShader",fragment},{"uniforms",uniforms}})});}
JSValue uniform(JSValue value){return fields({{"value",value}});}
struct World {
    JSValue view=camera(),geometry=nativePlaneGeometryConstructor().construct({1.8,1.8}),material,object,world;
    explicit World(JSValue m):material(m),object(mesh(geometry,m)),world(scene({object})){}
};
JSValue cube(){
    auto faces=JSValue::array();for(const auto color:std::array<std::array<std::uint8_t,4>,6>{{{255,0,0,255},{0,255,0,255},{0,0,255,255},{255,255,0,255},{0,255,255,255},{255,0,255,255}}})
        push(faces,nativeDataTextureConstructor().construct({NativeArray_detail::wrap(Uint8Array({color[0],color[1],color[2],color[3]})),1,1}));
    auto result=nativeCubeTextureConstructor().construct({faces});result.set("minFilter",NearestFilter);result.set("magFilter",NearestFilter);result.set("generateMipmaps",false);result.set("needsUpdate",true);return result;
}
JSValue volume(){auto result=nativeData3DTextureConstructor().construct({bytes({255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255}),2,1,2});result.set("needsUpdate",true);return result;}
JSValue arrayTexture(){auto result=nativeDataArrayTextureConstructor().construct({bytes({255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255}),2,1,2});result.set("needsUpdate",true);return result;}

// This CPU result is compared against the unchanged pinned JS class by the
// companion script. It exercises dynamic super and observable field order.
JSValue data3DOracle(){
    auto ctor=nativeData3DTextureConstructor();const auto base=nativeTextureConstructor();auto result=JSValue::object();
    result.set("source",call(ctor,"toString"));result.set("copySource",call(ctor.get("prototype").get("copy"),"toString"));
    result.set("name",ctor.get("name"));result.set("length",ctor.get("length"));result.set("copyLength",ctor.get("prototype").get("copy").get("length"));
    result.set("inherits",ctor.getPrototype().sameValue(base)&&ctor.get("prototype").getPrototype().sameValue(base.get("prototype")));
    auto t=ctor.construct();auto initial=JSValue::object();
    for(const auto* name:{"isData3DTexture","magFilter","minFilter","wrapR","generateMipmaps","flipY","unpackAlignment","version"})initial.set(name,t.get(name));
    initial.set("image",t.get("image"));initial.set("alias",t.get("source").get("data").sameValue(t.get("image")));initial.set("sourceVersion",t.get("source").get("version"));result.set("initial",initial);
    auto data=bytes({1,2,3,4}),odd=ctor.construct({data,"2",nullptr,false});result.set("uncoerced",list({odd.get("image").get("data").sameValue(data),odd.get("image").get("width"),odd.get("image").get("height"),odd.get("image").get("depth")}));
    odd.set("wrapR",RepeatWrapping);auto cloned=call(odd,"clone");result.set("clone",list({cloned.getPrototype().sameValue(ctor.get("prototype")),cloned.get("source").sameValue(odd.get("source")),cloned.get("image").sameValue(odd.get("image")),cloned.get("wrapR"),cloned.get("version")}));
    auto cases=JSValue::array();
    for(const auto* fail:{"","isData3DTexture","image","magFilter","minFilter","wrapR","generateMipmaps","flipY","unpackAlignment"}){
        auto events=JSValue::array(),state=fields({{"events",events},{"fail",fail}});
        auto parent=JSValue::builtinConstructor(+[](const JSValue& target,const Args& a,const Args& c)->JSValue{
            auto s=c[0];s.set("newTarget",target.sameValue(c[1]));s.set("superArgs",list(a));auto self=JSValue::object();
            for(const auto* name:{"isData3DTexture","image","magFilter","minFilter","wrapR","generateMipmaps","flipY","unpackAlignment"}){
                JSPropertyDescriptor d;d.configurable=true;d.setter=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
                    push(c[0].get("events"),c[1]);if(c[0].get("fail").sameValue(c[1]))throw JSThrownValue("field sentinel");
                    if(c[1].strictEquals("image")){auto state=c[0];state.set("image",a.at(0));}return {};
                },{s,name});self.defineProperty(name,d);
            }return self;
        },{state,ctor},"Parent",0);
        ctor.setPrototype(parent);bool threw=false;try{ctor.construct({JSValue(),"2",nullptr,false});}catch(const JSThrownValue& e){threw=true;}
        ctor.setPrototype(base);auto row=fields({{"fail",fail},{"events",events},{"newTarget",state.get("newTarget")},{"superArgs",state.get("superArgs")},{"threw",threw}});
        if(state.hasOwn("image"))row.set("image",state.get("image"));push(cases,row);
    }
    result.set("construction",cases);
    auto target=ctor.construct(),proto=ctor.get("prototype"),saved=proto.getPrototype(),trace=JSValue::array(),source=JSValue::object(),parent=JSValue::object();
    JSPropertyDescriptor copy;copy.getter=JSValue::function(+[](const JSValue& receiver,const Args&,const Args& c)->JSValue{
        push(c[0],list({"get super.copy",receiver.sameValue(c[1])}));return JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue{push(c[0],list({"call super.copy",receiver.sameValue(c[1]),a.at(0).sameValue(c[2])}));return 17;},c);
    },{trace,target,source});parent.defineProperty("copy",copy);
    JSPropertyDescriptor wrap;wrap.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get source.wrapR");return "wrap";},{trace});source.defineProperty("wrapR",wrap);
    proto.setPrototype(parent);const auto copied=proto.get("copy").call(target,{source});proto.setPrototype(saved);
    result.set("copy",fields({{"events",trace},{"returnsSelf",copied.sameValue(target)},{"wrapR",target.get("wrapR")}}));return result;
}
void constructorChecks(){
    auto result=data3DOracle();require(result.get("inherits").toBoolean(),"canonical Data3DTexture inherits the original Texture class");
    require(result.get("initial").get("alias").toBoolean(),"canonical volume image is the actual shared Source data");
    require(result.get("initial").get("unpackAlignment").strictEquals(1),"canonical volume keeps source unpack defaults");
    require(result.get("copy").get("returnsSelf").toBoolean(),"Data3DTexture copy returns its original dynamic receiver");
}
void cubeFaces(bool enabled){
    VulkanRenderer r({64,64,enabled});linear(r);auto t=cube(),direction=list({1,0,0});
    require(t.get("images").sameValue(t.get("image")),"CubeTexture images preserves its original image alias");
    auto uniforms=fields({{"sourceMap",uniform(t)},{"direction",uniform(direction)}});
    World w(raw("precision highp float;uniform samplerCube sourceMap;uniform vec3 direction;out vec4 color;void main(){color=texture(sourceMap,direction);}",uniforms));
    const std::array<Args,6> directions{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}};
    const std::array<std::array<int,3>,6> colors{{{255,0,0},{0,255,0},{0,0,255},{255,255,0},{0,255,255},{255,0,255}}};
    for(unsigned i=0;i<6;++i){uniforms.get("direction").set("value",list(directions[i]));r.render(w.world,w.view);expect(r,colors[i],"samplerCube chooses the real independently uploaded cube face");}
    uniforms.get("direction").set("value",list({1,0,0}));auto data=t.get("image").get("0").get("image").get("data");data.set("0",0);data.set("1",255);
    r.render(w.world,w.view);expect(r,{255,0,0},"unversioned cube face edits preserve the uploaded image");t.set("needsUpdate",true);r.render(w.world,w.view);expect(r,{0,255,0},"cube source version uploads the edited face");
    const auto live=r.getInfo().textureImages;call(t,"dispose");require(r.getInfo().textureImages<live,"cube dispose releases its actual VkImage");r.render(w.world,w.view);expect(r,{0,255,0},"cube disposal reuploads through original source cache");validate(r);
}
const char* volumeFragment="precision highp float;precision highp int;uniform highp sampler3D sourceMap;uniform vec3 uvw;uniform ivec3 cell;uniform bool filtered;out vec4 color;void main(){color=filtered?texture(sourceMap,uvw):texelFetch(sourceMap,cell,0);}";
const char* arrayFragment="precision highp float;precision highp int;uniform highp sampler2DArray sourceMap;uniform vec3 uvw;uniform ivec3 cell;uniform bool filtered;out vec4 color;void main(){color=filtered?texture(sourceMap,uvw):texelFetch(sourceMap,cell,0);}";
JSValue shapeUniforms(JSValue t){return fields({{"sourceMap",uniform(t)},{"uvw",uniform(list({.25,.5,.75}))},{"cell",uniform(list({1,0,1}))},{"filtered",uniform(false)}});}
void volumeSampling(bool enabled){
    VulkanRenderer r({64,64,enabled});linear(r);auto t=volume(),u=shapeUniforms(t);World w(raw(volumeFragment,u));
    r.render(w.world,w.view);expect(r,{255,255,255},"sampler3D texelFetch reads the requested x and z coordinates");u.get("filtered").set("value",true);r.render(w.world,w.view);expect(r,{0,0,255},"sampler3D texture uses the actual volume z extent");
    auto data=t.get("image").get("data");data.set("8",255);r.render(w.world,w.view);expect(r,{0,0,255},"unversioned volume bytes preserve the existing VkImage");
    call(t,"addUpdateRange",{0,4});auto ranges=t.get("updateRanges");t.set("needsUpdate",true);r.render(w.world,w.view);expect(r,{255,0,255},"the pinned 3D branch uploads the complete volume, including bytes outside 2D-only ranges");
    require(t.get("updateRanges").sameValue(ranges)&&ranges.length()==1,"the original 3D branch leaves Texture updateRanges untouched");
    call(t,"dispose");r.render(w.world,w.view);expect(r,{255,0,255},"disposed canonical Data3DTexture recreates its actual 3D VkImage");validate(r);
}
void arraySampling(bool enabled){
    VulkanRenderer r({64,64,enabled});linear(r);auto t=arrayTexture(),u=shapeUniforms(t);World w(shader(arrayFragment,u));
    r.render(w.world,w.view);expect(r,{255,255,255},"sampler2DArray texelFetch reads the actual second layer and second texel");
    u.get("filtered").set("value",true);u.get("uvw").set("value",list({.25,.5,1}));r.render(w.world,w.view);expect(r,{0,0,255},"array texture lookup selects an integer array layer");
    auto data=t.get("image").get("data");data.set("1",255);data.set("8",255);data.set("12",0);data.set("13",255);data.set("14",255);
    const auto layers=t.get("layerUpdates");call(t,"addLayerUpdate",{1});t.set("needsUpdate",true);r.render(w.world,w.view);expect(r,{255,0,255},"source layerUpdates uploads the selected Vulkan array layer");
    require(t.get("layerUpdates").sameValue(layers)&&layers.get("size").strictEquals(0),"source clears the original layer update Set");
    u.get("uvw").set("value",list({.25,.5,0}));r.render(w.world,w.view);expect(r,{255,0,0},"partial array upload preserves the unmarked layer despite CPU edits");
    u.get("filtered").set("value",false);r.render(w.world,w.view);expect(r,{0,255,255},"partial array upload also preserves the nonzero x coordinate within its selected layer");
    call(t,"dispose");r.render(w.world,w.view);expect(r,{0,255,255},"array disposal uploads its original canonical Source again");validate(r);
}
enum class Shape{Cube,Volume,Array};
JSValue cowTexture(Shape kind){if(kind==Shape::Cube)return cube();auto data=bytes({255,0,0,255});auto t=(kind==Shape::Volume?nativeData3DTextureConstructor():nativeDataArrayTextureConstructor()).construct({data,1,1,1});t.set("needsUpdate",true);return t;}
JSValue cowMaterial(Shape kind,JSValue t){
    const char* fragment=kind==Shape::Cube?"precision highp float;uniform samplerCube sourceMap;out vec4 color;void main(){color=texture(sourceMap,vec3(1.,0.,0.));}":
        kind==Shape::Volume?"precision highp float;uniform highp sampler3D sourceMap;out vec4 color;void main(){color=texture(sourceMap,vec3(.5));}":
        "precision highp float;uniform highp sampler2DArray sourceMap;out vec4 color;void main(){color=texture(sourceMap,vec3(.5,.5,0.));}";
    return raw(fragment,fields({{"sourceMap",uniform(t)}}));
}
void imageSnapshotsAndReentry(bool enabled){
    VulkanRenderer r({64,64,enabled});linear(r);r.autoClear=false;auto facade=r.object();facade.set("autoClear",false);const auto view=camera(),geometry=nativePlaneGeometryConstructor().construct({.8,1.5});
    for(const auto kind:{Shape::Cube,Shape::Volume,Shape::Array}){
        auto t=cowTexture(kind),leftMaterial=cowMaterial(kind,t),rightMaterial=cowMaterial(kind,t),left=mesh(geometry,leftMaterial,-.5),right=mesh(geometry,rightMaterial,.5);
        left.set("renderOrder",0);right.set("renderOrder",1);auto world=scene({left,right}),inner=scene({mesh(geometry,rightMaterial,.5)}),state=fields({{"nested",0},{"enabled",false},{"updates",0}});
        t.set("onUpdate",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
            require(self.sameValue(c[0])&&a.at(0).sameValue(c[0]),"shape texture onUpdate preserves original receiver and argument");auto state=c[1];state.set("updates",state.get("updates").number()+1);
            if(state.get("enabled").toBoolean()){state.set("enabled",false);state.set("nested",state.get("nested").number()+1);call(c[2],"render",{c[3],c[4]});}return {};
        },{t,state,facade,inner,view}));
        auto data=kind==Shape::Cube?t.get("image").get("0").get("image").get("data"):t.get("image").get("data");
        left.set("onAfterRender",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            auto data=c[0];data.set("0",0);data.set("2",255);auto t=c[1];t.set("needsUpdate",true);auto state=c[2];state.set("enabled",true);return {};
        },{data,t,state}));
        r.clear();r.render(world,view);expect(r,{255,0,0},"earlier shape draw retains its actual image snapshot through later upload and nested render",16);
        expect(r,{0,0,255},"later shape draw samples the COW image after onUpdate reentry",48);require(state.get("nested").strictEquals(1),"shape upload reenters the actual renderer once");require(state.get("updates").strictEquals(2),"nested source bookkeeping does not duplicate GPU upload callbacks");
        left.set("onAfterRender",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"dispose");return {};},{t}));
        r.clear();r.render(world,view);expect(r,{0,0,255},"shape disposal preserves a previously recorded descriptor image until its fence",16);expect(r,{0,0,255},"later shape draw recreates disposed source storage in the same frame",48);
        validate(r);
    }
}
void defaultMorphTexture(bool enabled){
    VulkanRenderer r({64,64,enabled});linear(r);auto geometry=nativePlaneGeometryConstructor().construct({.6,1.2}),position=geometry.get("attributes").get("position");
    Float32Array targetData(std::size_t(position.get("count").number())*3);
    for(unsigned i=0;i<position.get("count").number();++i){targetData[i*3]=call(position,"getX",{i}).number()+1;targetData[i*3+1]=call(position,"getY",{i}).number();targetData[i*3+2]=call(position,"getZ",{i}).number();}
    auto target=nativeFloat32BufferAttributeConstructor().construct({NativeArray_detail::wrap(targetData),3});geometry.get("morphAttributes").set("position",list({target}));
    auto material=nativeMeshBasicMaterialConstructor().construct({fields({{"color",0xff0000}})}),object=mesh(geometry,material,-.5),world=scene({object}),view=camera();
    auto influences=object.get("morphTargetInfluences");require(influences.length()==1,"canonical Mesh discovers its original morph attributes");
    influences.set("0",0);r.render(world,view);expect(r,{255,0,0},"default source morph shader preserves the base geometry at zero weight",16);expect(r,{0,0,0},"zero morph weight leaves the target location clear",48);
    require(r.getInfo().textureImages>=5,"original WebGLMorphtargets allocates a real float DataArrayTexture in addition to startup fallback images");
    influences.set("0",1);r.render(world,view);expect(r,{0,0,0},"full morph weight moves the original position out of the base location",16);expect(r,{255,0,0},"default source vertex shader fetches the real sampler2DArray morph texels",48);
    const auto live=r.getInfo().textureImages;call(geometry,"dispose");require(r.getInfo().textureImages<live,"original geometry disposal retires its cached morph texture");r.render(world,view);expect(r,{255,0,0},"original morph manager recreates its array texture after geometry disposal",48);validate(r);
}
}
int main(int argc,char** argv){
    bool enabled=false;for(int i=1;i<argc;++i){const std::string arg=argv[i];if(arg=="--validation")enabled=true;if(arg=="--data3d-oracle"){try{std::cout<<data3DOracle().toJSON()->dump();endJSJob();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}}
    try{
        auto run=[&](const char* name,auto fn){std::cout<<"Running "<<name<<std::endl;fn(enabled);};
        constructorChecks();run("cubeFaces",cubeFaces);run("volumeSampling",volumeSampling);run("arraySampling",arraySampling);run("imageSnapshotsAndReentry",imageSnapshotsAndReentry);run("defaultMorphTexture",defaultMorphTexture);endJSJob();
        std::cout<<"Vulkan managed sampler shapes: "<<assertions<<" assertions passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<"Vulkan managed sampler shapes failure: "<<e.what()<<'\n';return 1;}
}
