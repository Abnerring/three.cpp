#include "renderers/VulkanRenderer.h"
#include "core/RenderTarget.h"
#include "core/WebGLCubeRenderTarget.h"
#include "cameras/OrthographicCamera.h"
#include "geometries/PlaneGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"
#include "scenes/Scene.h"
#include "textures/DataTexture.h"
#include "extras/DataUtils.h"
#include <array>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
unsigned assertions=0;
JSValue clearSamples=JSValue::array();
void require(bool value,const char* message){if(!value)throw std::logic_error(message);++assertions;}
template<class Exception=std::logic_error,class F> void rejects(F&& body,const char* message){bool rejected=false;try{body();}catch(const Exception&){rejected=true;}require(rejected,message);}
std::shared_ptr<RenderTarget> half(unsigned width=8,unsigned height=4){auto target=std::make_shared<RenderTarget>(width,height);target->texture->type=HalfFloatType;return target;}
std::shared_ptr<WebGLCubeRenderTarget> cube(unsigned size=4){auto target=std::make_shared<WebGLCubeRenderTarget>(size);target->texture->type=HalfFloatType;return target;}
void paint(VulkanRenderer& renderer,const std::shared_ptr<RenderTarget>& target,const Color& rgb,double alpha=1,unsigned face=0){renderer.setRenderTarget(target,face);renderer.setClearColor(rgb,alpha);renderer.clear();}
bool uniform(const std::vector<std::uint16_t>& pixels,std::array<std::uint16_t,4> expected){if(pixels.empty()||pixels.size()%4)return false;for(std::size_t i=0;i<pixels.size();++i)if(pixels[i]!=expected[i%4])return false;return true;}
void noPixels(VulkanRenderer& renderer,RenderTarget& target,const char* message){rejects([&]{renderer.readRenderTargetHalfFloatPixels(target,0,0,1,1);},message);}
struct World {
    Scene scene;OrthographicCamera camera{-1,1,1,-1,.1,10};
    std::shared_ptr<MeshBasicMaterial> material=std::make_shared<MeshBasicMaterial>(Color(2,.5,.25));
    std::shared_ptr<Mesh> mesh=std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(2,2),material);
    World(){camera.position.z=2;mesh->frustumCulled=false;scene.add(mesh);}
};
void clearReadback(VulkanRenderer& renderer){
    auto target=half();renderer.setRenderTarget(target);
    require(renderer.readPixelsHalfFloat().empty(),"Fresh HalfFloat target invented GPU readback");
    noPixels(renderer,*target,"Fresh HalfFloat target exposed pixels before actual GPU completion");
    const std::array<std::array<double,4>,4> inputs{{{2,.5,-.25,1.5},{4,.125,.25,1},{16,2,4,.5},{65504,.0009765625,-2,1}}};
    const std::array<std::array<std::uint16_t,4>,4> expected{{{0x4000,0x3800,0xb400,0x3e00},{0x4400,0x3000,0x3400,0x3c00},{0x4c00,0x4000,0x4400,0x3800},{0x7bff,0x1400,0xc000,0x3c00}}};
    renderer.srgbOutput=true;
    for(std::size_t i=0;i<inputs.size();++i){
        const auto& c=inputs[i];paint(renderer,target,Color(c[0],c[1],c[2]),c[3]);
        const auto pixels=renderer.readRenderTargetHalfFloatPixels(*target,0,0,8,4);
        require(pixels.size()==8*4*4&&uniform(pixels,expected[i]),"Actual RGBA16F clear bits differ or HDR/negative values were clamped");
        require(renderer.readPixelsHalfFloat()==pixels,"Current HalfFloat readback differs from target GPU copy");
        auto item=JSValue::object(),values=JSValue::array(),bits=JSValue::array();
        for(unsigned channel=0;channel<4;++channel){values.set(std::to_string(channel),c[channel]);bits.set(std::to_string(channel),pixels[channel]);}
        item.set("values",values);item.set("bits",bits);clearSamples.set(std::to_string(clearSamples.length()),item);
    }
    rejects<std::invalid_argument>([&]{renderer.readPixels();},"HalfFloat current readback silently normalized to RGBA8");
    rejects<std::invalid_argument>([&]{renderer.readRenderTargetPixels(*target,0,0,8,4);},"HalfFloat target readback silently returned Uint8 bytes");
    Uint16Array backing(std::vector<std::uint16_t>(8*4*4+8,0x55aa));
    Uint16Array view(backing.buffer,8,8*4*4);renderer.readRenderTargetPixels(*target,0,0,8,4,view);
    for(unsigned i=0;i<4;++i)require(backing[i]==0x55aa&&backing[backing.size()-1-i]==0x55aa,"Uint16 destination byteOffset or length guard was overwritten");
    for(std::size_t i=0;i<view.size();++i)require(view[i]==expected.back()[i%4],"Uint16 view did not retain actual raw half component bits");
    Uint16Array small(8*4*4-1);rejects<std::out_of_range>([&]{renderer.readRenderTargetPixels(*target,0,0,8,4,small);},"Too-small Uint16 destination was accepted");
    rejects<std::out_of_range>([&]{renderer.readRenderTargetHalfFloatPixels(*target,0xffffffffu,0,2,1);},"HalfFloat row bounds overflow was accepted");
    renderer.setRenderTarget(nullptr);
}
void interleave(VulkanRenderer& renderer){
    auto byte=std::make_shared<RenderTarget>(8,8),floating=half(8,8);World world;renderer.srgbOutput=false;renderer.setClearColor(Color(0,0,0));
    renderer.setRenderTarget(byte);renderer.render(world.scene,world.camera);
    auto bytes=renderer.readRenderTargetPixels(*byte,0,0,8,8);const auto center=(4*8+4)*4;
    require(bytes[center]==255&&std::abs(int(bytes[center+1])-128)<=1&&std::abs(int(bytes[center+2])-64)<=1,"Existing RGBA8 pipeline behavior changed");
    rejects<std::invalid_argument>([&]{renderer.readRenderTargetHalfFloatPixels(*byte,0,0,1,1);},"RGBA8 was reinterpreted as half components");
    renderer.srgbOutput=true;renderer.setRenderTarget(floating);renderer.render(world.scene,world.camera);
    auto raw=renderer.readRenderTargetHalfFloatPixels(*floating,0,0,8,8);
    require(raw[center]==0x4000&&raw[center+1]==0x3800&&raw[center+2]==0x3400&&raw[center+3]==0x3c00,"HalfFloat draw reused an incompatible RGBA8 pipeline or encoded HDR as sRGB");
    world.material->transparent=true;world.material->opacity=.5;renderer.setClearColor(Color(0,0,0),0);renderer.render(world.scene,world.camera);
    raw=renderer.readRenderTargetHalfFloatPixels(*floating,0,0,8,8);
    require(raw[center]==0x3c00&&raw[center+1]==0x3400&&raw[center+2]==0x3000&&raw[center+3]==0x3800,"Actual HalfFloat attachment blending failed");
    world.material->transparent=false;world.material->opacity=1;renderer.srgbOutput=false;renderer.setRenderTarget(byte);renderer.render(world.scene,world.camera);
    bytes=renderer.readRenderTargetPixels(*byte,0,0,8,8);require(bytes[center]==255&&bytes[center+1]>=127,"Switching back to RGBA8 reused a HalfFloat-incompatible pipeline");
    renderer.setRenderTarget(nullptr);
}
void textureRows(VulkanRenderer& renderer){
    const Uint16Array data{0x4000,0x3400,0x3800,0x3c00,0x4400,0x3400,0x3800,0x3c00,0x4800,0x3400,0x3800,0x3c00,0x4c00,0x3400,0x3800,0x3c00};
    auto texture=std::make_shared<DataTexture>(data,2,2,RGBAFormat,HalfFloatType);texture->needsUpdate=true;
    auto target=half(8,8);World world;world.material->color=Color(1,1,1);world.material->map=texture;renderer.srgbOutput=true;
    renderer.setRenderTarget(target);renderer.setClearColor(Color(0,0,0));renderer.render(world.scene,world.camera);
    const auto full=renderer.readRenderTargetHalfFloatPixels(*target,0,0,8,8);std::set<std::uint16_t> reds;
    for(std::size_t i=0;i<full.size();i+=4){reds.insert(full[i]);require(full[i+1]==0x3400&&full[i+2]==0x3800&&full[i+3]==0x3c00,"HalfFloat texture staging changed raw HDR components");}
    require(reds==std::set<std::uint16_t>{0x4000,0x4400,0x4800,0x4c00},"HalfFloat texture-to-target path clamped or lost HDR texels");
    const auto region=renderer.readRenderTargetHalfFloatPixels(*target,2,1,3,5);
    for(unsigned row=0;row<5;++row)for(unsigned x=0;x<3;++x)for(unsigned c=0;c<4;++c)
        require(region[(row*3+x)*4+c]==full[((row+1)*8+x+2)*4+c],"HalfFloat cropped row stride or component width is wrong");
    renderer.setRenderTarget(nullptr);
}
void equirectangular(VulkanRenderer& renderer){
    std::vector<std::uint16_t> data;for(unsigned i=0;i<8;++i)data.insert(data.end(),{0x4000,0x3800,0x3400,0x3c00});
    auto texture=std::make_shared<DataTexture>(Uint16Array(data),4,2,RGBAFormat,HalfFloatType);texture->needsUpdate=true;
    auto target=cube();renderer.renderEquirectangularToCube(texture,target);
    for(unsigned face=0;face<6;++face){renderer.setRenderTarget(target,face);renderer.clear(false,false);require(uniform(renderer.readRenderTargetHalfFloatPixels(*target,0,0,4,4),{0x4000,0x3800,0x3400,0x3c00}),"Equirectangular HalfFloat pipeline/view/readback failed for an actual cube face");}
    renderer.setRenderTarget(nullptr);
}
void blur(VulkanRenderer& renderer){
    auto source=cube(8),temporary=cube(8);for(unsigned face=0;face<6;++face)paint(renderer,source,Color(4,.5,.125),1,face);
    renderer.blurCube(source,temporary,.1);
    for(unsigned face=0;face<6;++face){
        renderer.setRenderTarget(source,face);renderer.clear(false,false);const auto pixels=renderer.readRenderTargetHalfFloatPixels(*source,0,0,8,8);
        for(std::size_t i=0;i<pixels.size();i+=4)require(std::abs(fromHalfFloat(pixels[i])-4)<.005&&std::abs(fromHalfFloat(pixels[i+1])-.5)<.001&&std::abs(fromHalfFloat(pixels[i+2])-.125)<.0003,"HalfFloat cube blur sampled or wrote clamped/incompatible storage");
    }
    renderer.setRenderTarget(nullptr);source->dispose();rejects([&]{renderer.blurCube(source,temporary,.1);},"HalfFloat blur sampled logically retired source storage");
}
void lifecycle(VulkanRenderer& renderer){
    auto first=half(4,4),second=half(4,4);paint(renderer,first,Color(2,.5,.25));paint(renderer,second,Color(4,.25,.5));
    first->dispose();noPixels(renderer,*first,"Disposed parked HalfFloat target retained old pixels");
    paint(renderer,first,Color(2,.5,.25));first->setSize(3,2);noPixels(renderer,*first,"Resized HalfFloat target exposed old pixel width");
    paint(renderer,first,Color(4,.25,.5));require(uniform(renderer.readRenderTargetHalfFloatPixels(*first,0,0,3,2),{0x4400,0x3400,0x3800,0x3c00}),"HalfFloat resize did not recreate 8-byte storage");
    first->texture->type=UnsignedByteType;noPixels(renderer,*first,"Color format change exposed previous HalfFloat storage");
    renderer.srgbOutput=false;paint(renderer,first,Color(1,0,0));require(renderer.readRenderTargetPixels(*first,0,0,3,2).size()==3*2*4,"Same target did not genuinely replace HalfFloat storage with RGBA8");
    first->texture->type=HalfFloatType;paint(renderer,first,Color(2,.5,.25));require(uniform(renderer.readRenderTargetHalfFloatPixels(*first,0,0,3,2),{0x4000,0x3800,0x3400,0x3c00}),"Same-address format replacement reused stale RGBA8 allocation");
    require(uniform(renderer.readRenderTargetHalfFloatPixels(*second,0,0,4,4),{0x4400,0x3400,0x3800,0x3c00}),"Other parked HalfFloat target lost its actual bytes");
    renderer.setRenderTarget(nullptr);
}
void recording(VulkanRenderer& renderer,bool abort){
    auto target=half(8,8);renderer.setRenderTarget(target);World world;bool entered=false;const auto before=renderer.getInfo().renderTargets;
    world.mesh->onBeforeRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){if(entered)return;entered=true;target->setSize(5,3);noPixels(renderer,*target,"Recording HalfFloat dispose exposed stale pixels");require(renderer.getInfo().renderTargets==before,"In-use HalfFloat Vk resources were destroyed before completion/reset");if(abort)throw std::runtime_error("half abort");};
    if(abort)rejects<std::runtime_error>([&]{renderer.render(world.scene,world.camera);},"HalfFloat recorded callback did not abort");else renderer.render(world.scene,world.camera);
    require(entered,"Actual HalfFloat draw did not reach its recording callback");require(renderer.getInfo().renderTargets+1==before,"Retired HalfFloat allocation did not release after fence/reset");
    world.mesh->onBeforeRender={};paint(renderer,target,Color(2,.5,.25));require(uniform(renderer.readRenderTargetHalfFloatPixels(*target,0,0,5,3),{0x4000,0x3800,0x3400,0x3c00}),"HalfFloat recording retirement did not recover with resized actual GPU pixels");renderer.setRenderTarget(nullptr);
}
void unsupported(VulkanRenderer& renderer){
    auto current=half(4,4);paint(renderer,current,Color(2,.5,.25));auto target=half(4,4);
    auto reject=[&]{rejects<std::exception>([&]{renderer.setRenderTarget(target);},"Unsupported HalfFloat target configuration was silently accepted");require(renderer.getRenderTarget()==current&&uniform(renderer.readRenderTargetHalfFloatPixels(*current,0,0,4,4),{0x4000,0x3800,0x3400,0x3c00}),"Rejected HalfFloat request changed binding or GPU data");};
    target->texture->generateMipmaps=true;reject();target->texture->generateMipmaps=false;
    target->samples=4;reject();target->samples=0;target->depthBuffer=false;reject();target->depthBuffer=true;
    target->texture->internalFormat="RGBA8";reject();target->texture->internalFormat="RGBA16F";
    target->texture->colorSpace=SRGBColorSpace;reject();target->texture->colorSpace=NoColorSpace;
    target->texture->type=FloatType;reject();target->texture->type=HalfFloatType;
    rejects([&]{renderer.setRenderTarget(target,0,1);},"HalfFloat mip binding was silently accepted");
    renderer.setRenderTarget(target);renderer.setClearColor(Color(2,.5,.25));renderer.clear();require(uniform(renderer.readRenderTargetHalfFloatPixels(*target,0,0,4,4),{0x4000,0x3800,0x3400,0x3c00}),"Explicit matching RGBA16F storage did not work");renderer.setRenderTarget(nullptr);
}
}
int main(int argc,char** argv){try{
    const bool validation=argc==2&&std::string(argv[1])=="--validation";if(argc>2||(argc==2&&!validation))throw std::invalid_argument("Usage: VulkanHalfFloatTargetProbe [--validation]");
    VulkanRenderer renderer({32,24,validation});const auto baseline=renderer.getInfo().renderTargets;
    if(validation)require(renderer.getInfo().validationEnabled,"Requested actual validation layer is unavailable");
    auto cases=JSValue::array();auto run=[&](const char* name,auto body){body();cases.set(std::to_string(cases.length()),name);};
    run("half-clear-readback",[&]{clearReadback(renderer);});run("rgba8-interleave",[&]{interleave(renderer);});run("half-texture-rows",[&]{textureRows(renderer);});
    run("equirect-six-faces",[&]{equirectangular(renderer);});run("half-blur",[&]{blur(renderer);});run("lifecycle-format-change",[&]{lifecycle(renderer);});
    run("recording-fence",[&]{recording(renderer,false);});run("recording-abort",[&]{recording(renderer,true);});run("unsupported-transaction",[&]{unsupported(renderer);});
    require(renderer.getInfo().renderTargets==baseline,"Dropped HalfFloat/RGBA8 target owners retained Vulkan allocations");require(renderer.getValidationMessages().empty(),"Vulkan validation reported HalfFloat errors");
    auto result=JSValue::object();result.set("actualGPUExecuted",true);result.set("deviceName",renderer.getInfo().deviceName);result.set("assertions",assertions);result.set("validationEnabled",renderer.getInfo().validationEnabled);result.set("validationMessages",renderer.getValidationMessages().size());result.set("format","VK_FORMAT_R16G16B16A16_SFLOAT");result.set("bytesPerPixel",8);result.set("clearSamples",clearSamples);result.set("cases",cases);std::cout<<result.toJSON()->dump();
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
