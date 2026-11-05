#include "renderers/VulkanRenderer.h"
#include "textures/DataTexture.h"
#include "textures/Texture.h"
#include "scenes/Scene.h"
#include "cameras/OrthographicCamera.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"
#include "geometries/PlaneGeometry.h"
#include "core/NativeArray.h"
#include "core/JSJobs.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using Args=std::vector<JSValue>;
int assertions=0;
void require(bool value,const char* text){++assertions;if(!value)throw std::runtime_error(text);}
JSValue call(JSValue self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
JSValue bytes(std::initializer_list<std::uint8_t> values){return NativeArray_detail::wrap(Uint8Array(values));}
JSValue texture(JSValue data,int width=1,int height=1,int format=RGBAFormat,int type=UnsignedByteType){
    auto t=nativeDataTextureConstructor().construct({data,width,height,format,type});t.set("needsUpdate",true);return t;
}
JSValue paint(JSValue map={}){auto p=JSValue::object();p.set("color",0xffffff);if(!map.isUndefined())p.set("map",map);return nativeMeshBasicMaterialConstructor().construct({p});}
JSValue camera(){auto c=nativeOrthographicCameraConstructor().construct({-1,1,1,-1,.1,10});c.get("position").set("z",3);return c;}
struct World {
    JSValue scene=nativeSceneConstructor().construct(),view=camera(),geometry=nativePlaneGeometryConstructor().construct({1.8,1.8}),material,mesh;
    explicit World(JSValue map):material(paint(map)),mesh(nativeMeshConstructor().construct({geometry,material})){call(scene,"add",{mesh});}
};
std::array<int,4> pixel(const VulkanRenderer& r,unsigned x=32,unsigned y=32){const auto data=r.readPixels();require(data.size()==r.getWidth()*r.getHeight()*4,"complete raw texture GPU readback");const auto i=(y*r.getWidth()+x)*4;return {data.at(i),data.at(i+1),data.at(i+2),data.at(i+3)};}
void expect(const VulkanRenderer& r,std::array<int,3> wanted,const char* text,unsigned x=32,unsigned y=32){
    const auto actual=pixel(r,x,y);if(std::abs(actual[0]-wanted[0])>2||std::abs(actual[1]-wanted[1])>2||std::abs(actual[2]-wanted[2])>2){std::cerr<<text<<": "<<actual[0]<<','<<actual[1]<<','<<actual[2]<<'\n';require(false,text);}require(true,text);
}
void linear(VulkanRenderer& r){r.srgbOutput=false;r.object().set("outputColorSpace",LinearSRGBColorSpace);}
void validation(const VulkanRenderer& r){const auto errors=r.getValidationMessages();for(const auto& error:errors)std::cerr<<error<<'\n';require(errors.empty(),"raw texture GPU operations emit no collected validation errors");}
void versionAndDisposal(bool enabled){
    VulkanRenderer r({64,64,enabled});linear(r);auto t=nativeDataTextureConstructor().construct({bytes({255,0,0,255}),1,1});World w(t);auto state=JSValue::object();state.set("count",0);
    t.set("onUpdate",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{auto s=c[0];s.set("count",s.get("count").toNumber()+1);s.set("identity",self.sameValue(c[1])&&a.at(0).sameValue(c[1]));return {};},{state,t}));
    r.render(w.scene,w.view);expect(r,{0,0,0},"unuploaded source texture samples the actual WebGLState startup empty texture");require(state.get("count").strictEquals(0),"version zero does not upload or invoke onUpdate");
    require(r.getInfo().textureImages>=4,"WebGLState startup allocates real 2D cube 3D and array images");t.set("needsUpdate",true);
    r.render(w.scene,w.view);expect(r,{255,0,0},"canonical DataTexture and Source upload into a real sampled VkImage");
    require(state.get("count").strictEquals(1)&&state.get("identity").toBoolean(),"source onUpdate preserves receiver and argument identity");
    require(!t.isHostObject("three.Texture"),"raw texture remains a canonical source object");
    auto data=t.get("image").get("data");data.set("0",0);data.set("2",255);r.render(w.scene,w.view);expect(r,{255,0,0},"unversioned CPU pixel edits preserve existing GPU image");
    t.set("needsUpdate",true);r.render(w.scene,w.view);expect(r,{0,0,255},"needsUpdate uploads actual current native pixel bytes");require(state.get("count").strictEquals(2),"source issues one callback for each committed version");
    const auto live=r.getInfo().textureImages;call(t,"dispose");require(r.getInfo().textureImages<live,"raw disposal releases unreferenced actual VkImage storage");
    r.render(w.scene,w.view);expect(r,{0,0,255},"disposed raw texture uploads again through its source resource cache");require(state.get("count").strictEquals(3),"resource reallocation triggers the source upload callback");
    validation(r);
}
void partialRowsAndFlip(bool enabled){
    VulkanRenderer r({64,64,enabled});linear(r);auto t=texture(bytes({255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255}),2,2);World w(t);
    r.render(w.scene,w.view);expect(r,{0,0,255},"source row one maps to the top left UV quadrant",16,16);expect(r,{255,255,255},"top right texture quadrant",48,16);
    expect(r,{255,0,0},"source row zero maps to the bottom left UV quadrant",16,48);expect(r,{0,255,0},"bottom right texture quadrant",48,48);
    auto data=t.get("image").get("data");data.set("1",255);call(t,"addUpdateRange",{0,4});const auto ranges=t.get("updateRanges");t.set("needsUpdate",true);r.render(w.scene,w.view);
    expect(r,{255,255,0},"first partial source range updates the actual lower left texel",16,48);expect(r,{0,0,255},"partial upload preserves untouched VkImage rows",16,16);
    require(t.get("updateRanges").sameValue(ranges)&&ranges.get("length").strictEquals(0),"source clears the original updateRanges object");
    data.set("4",255);data.set("5",0);data.set("6",255);call(t,"addUpdateRange",{4,4});t.set("needsUpdate",true);r.render(w.scene,w.view);
    expect(r,{255,0,255},"partial upload preserves the nonzero VkBufferImageCopy x offset",48,48);expect(r,{255,255,0},"second partial upload preserves the adjacent texel",16,48);
    t.set("flipY",true);t.set("needsUpdate",true);r.render(w.scene,w.view);expect(r,{255,255,0},"raw UNPACK_FLIP_Y_WEBGL reverses complete source rows",16,16);expect(r,{0,0,255},"flipped upload preserves the other row",16,48);
    validation(r);
}
void formatsAndUnpack(bool enabled){
    VulkanRenderer r({64,64,enabled});linear(r);
    auto red=texture(bytes({32,128,255,99,255,128,32}),3,2,RedFormat);red.set("unpackAlignment",4);World w(red);r.render(w.scene,w.view);
    expect(r,{255,0,0},"R8 upload honors the alignment padding before source row one",16,16);expect(r,{32,0,0},"R8 sampling supplies zero green and blue components",16,48);
    auto floating=texture(NativeArray_detail::wrap(Float32Array{0.25f,0.5f,1.f,1.f}),1,1,RGBAFormat,FloatType);w.material.set("map",floating);w.material.set("needsUpdate",true);r.render(w.scene,w.view);
    expect(r,{64,128,255},"raw RGBA32F bytes retain their floating point sampled values");
    auto half=texture(NativeArray_detail::wrap(Uint16Array{0x3400,0x3800,0x3c00,0x3c00}),1,1,RGBAFormat,HalfFloatType);w.material.set("map",half);w.material.set("needsUpdate",true);r.render(w.scene,w.view);
    expect(r,{64,128,255},"raw RGBA16F upload preserves binary16 components");
    auto srgb=texture(bytes({128,128,128,255}));srgb.set("colorSpace",SRGBColorSpace);w.material.set("map",srgb);w.material.set("needsUpdate",true);r.render(w.scene,w.view);
    expect(r,{55,55,55},"SRGB8_ALPHA8 decoding happens in the Vulkan sampled image format");validation(r);
}
void alphaAndPremultiply(bool enabled){
    VulkanRenderer r({64,64,enabled});linear(r);auto white=texture(bytes({255,255,255,255})),alpha=texture(bytes({255,128,255,255}));World w(white);
    w.material.set("alphaMap",alpha);w.material.set("transparent",true);r.render(w.scene,w.view);expect(r,{128,128,128},"source alphaMap samples its green component and blends on the GPU");
    w.material.set("alphaTest",.6);r.render(w.scene,w.view);expect(r,{0,0,0},"source alphaTest discards below the sampled alphaMap threshold");
    w.material.set("alphaTest",.2);r.render(w.scene,w.view);expect(r,{128,128,128},"alphaTest uniform updates retain the actual sampled texture");
    auto premult=texture(bytes({255,0,0,128}));premult.set("premultiplyAlpha",true);w.material.set("map",premult);w.material.set("alphaMap",nullptr);w.material.set("alphaTest",0);w.material.set("transparent",false);w.material.set("needsUpdate",true);
    r.render(w.scene,w.view);expect(r,{128,0,0},"UNPACK_PREMULTIPLY_ALPHA_WEBGL transforms real uploaded bytes before opaque shading");validation(r);
}
void generatedMipmaps(bool enabled){
    VulkanRenderer r({64,64,enabled});linear(r);auto t=texture(bytes({255,0,0,255,0,0,255,255,0,0,255,255,255,0,0,255}),2,2);
    t.set("generateMipmaps",true);t.set("minFilter",LinearMipmapLinearFilter);t.set("wrapS",RepeatWrapping);t.set("wrapT",RepeatWrapping);call(t.get("repeat"),"set",{128,128});World w(t);
    r.render(w.scene,w.view);expect(r,{128,0,128},"actual Vulkan mip generation averages the source image under minification");
    auto data=t.get("image").get("data");for(int i=0;i<16;i+=4){data.set(JSValue(i),0);data.set(JSValue(i+1),255);data.set(JSValue(i+2),0);}t.set("needsUpdate",true);r.render(w.scene,w.view);
    expect(r,{0,255,0},"new source version regenerates lower mip levels from the new GPU base image");validation(r);
}
void recordedImageLifetime(bool enabled){
    VulkanRenderer r({64,64,enabled});linear(r);auto t=texture(bytes({255,0,0,255})),scene=nativeSceneConstructor().construct(),view=camera();
    auto geometry=nativePlaneGeometryConstructor().construct({.8,1.5});auto left=nativeMeshConstructor().construct({geometry,paint(t)}),right=nativeMeshConstructor().construct({geometry,paint(t)});
    left.get("position").set("x",-.5);right.get("position").set("x",.5);left.set("renderOrder",0);right.set("renderOrder",1);call(scene,"add",{left,right});
    left.set("onAfterRender",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        auto data=c[0].get("image").get("data");data.set("0",0);data.set("2",255);auto t=c[0];t.set("needsUpdate",true);return {};
    },{t}));
    r.render(scene,view);expect(r,{255,0,0},"an earlier recorded draw keeps its old sampled image until the frame fence",16,32);expect(r,{0,0,255},"a later source upload uses a new actual VkImage in the same recording",48,32);
    left.set("onAfterRender",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"dispose");return {};},{t}));
    r.render(scene,view);expect(r,{0,0,255},"dispose after a recorded draw retains its image until submission completes",16,32);expect(r,{0,0,255},"the later draw reallocates disposed source storage in the same frame",48,32);validation(r);
}
void mutableOddMipGpu(bool enabled){
    VulkanRenderer r({64,64,enabled});linear(r);const auto gl=call(r.object(),"getContext");auto handle=call(gl,"createTexture");call(gl,"activeTexture",{0x84c0});call(gl,"bindTexture",{0x0de1,handle});
    call(gl,"texParameteri",{0x0de1,0x2801,0x2600});call(gl,"texParameteri",{0x0de1,0x2800,0x2600});
    Uint8Array red(36);for(unsigned i=0;i<36;i+=4){red[i]=255;red[i+3]=255;}
    call(gl,"texImage2D",{0x0de1,0,0x8058,3,3,0,0x1908,0x1401,NativeArray_detail::wrap(red)});
    call(gl,"texImage2D",{0x0de1,1,0x8058,1,1,0,0x1908,0x1401,bytes({0,0,255,255})});
    // Use the original source ExternalTexture protocol over a canonical Texture:
    // sourceTexture is the real gl.createTexture handle allocated above.
    auto external=nativeTextureConstructor().construct();external.set("isExternalTexture",true);external.set("sourceTexture",handle);World w(external);
    r.render(w.scene,w.view);expect(r,{255,0,0},"allocating a 1x1 mip preserves the existing odd 3x3 base image");
    call(gl,"activeTexture",{0x84c0});call(gl,"bindTexture",{0x0de1,handle});call(gl,"texParameteri",{0x0de1,0x2801,0x2700});call(external.get("repeat"),"set",{128,128});
    r.render(w.scene,w.view);expect(r,{0,0,255},"manual mutable mip contents reach the actual Vulkan sampler");
    call(gl,"generateMipmap",{0x0de1});r.render(w.scene,w.view);expect(r,{255,0,0},"GPU mip generation derives the odd-size base image");
    call(gl,"texSubImage2D",{0x0de1,1,0,0,1,1,0x1908,0x1401,bytes({0,255,0,255})});r.render(w.scene,w.view);
    expect(r,{0,255,0},"generated mutable mip storage accepts a later real GPU subimage upload");require(call(gl,"getError").strictEquals(0),"mutable mip GL commands finish without API errors");
    call(gl,"deleteTexture",{handle});validation(r);
}
}
int main(int argc,char** argv){
    bool enabled=false;for(int i=1;i<argc;++i)if(std::string(argv[i])=="--validation")enabled=true;
    try{versionAndDisposal(enabled);partialRowsAndFlip(enabled);formatsAndUnpack(enabled);alphaAndPremultiply(enabled);generatedMipmaps(enabled);recordedImageLifetime(enabled);mutableOddMipGpu(enabled);endJSJob();
        std::cout<<"Vulkan managed textures: "<<assertions<<" assertions passed"<<'\n';return 0;
    }catch(const std::exception& error){std::cerr<<"Vulkan managed textures failure: "<<error.what()<<'\n';return 1;}
}
