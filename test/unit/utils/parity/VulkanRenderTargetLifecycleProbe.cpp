#include "renderers/VulkanRenderer.h"
#include "core/RenderTarget.h"
#include "core/WebGLCubeRenderTarget.h"
#include "scenes/Scene.h"
#include "cameras/PerspectiveCamera.h"
#include "geometries/BoxGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"
#include "textures/DepthTexture.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>

namespace {
unsigned assertions = 0;
void require(bool condition, const char* message) {
    if (!condition) throw std::logic_error(message);
    ++assertions;
}
template<class Exception = std::logic_error, class F> void rejects(F&& body, const char* message) {
    bool rejected = false;
    try { body(); } catch (const Exception&) { rejected = true; }
    require(rejected, message);
}
bool color(const std::vector<std::uint8_t>& pixels, std::array<unsigned,3> rgb) {
    if (pixels.empty() || pixels.size() % 4) return false;
    for (std::size_t i = 0; i < pixels.size(); i += 4)
        for (unsigned channel = 0; channel < 3; ++channel)
            if (pixels[i + channel] != rgb[channel]) return false;
    return true;
}
void paint(VulkanRenderer& renderer, const std::shared_ptr<RenderTarget>& target,
           const Color& color, std::uint32_t face = 0) {
    renderer.setRenderTarget(target, face);
    renderer.setClearColor(color, 1);
    renderer.clear();
}
void noPixels(VulkanRenderer& renderer, RenderTarget& target, const char* message) {
    rejects([&] { renderer.readRenderTargetPixels(target, 0, 0, 1, 1); }, message);
}
void twoDimensional(VulkanRenderer& renderer) {
    const auto baseline = renderer.getInfo().renderTargets;
    auto first = std::make_shared<RenderTarget>(8, 4);
    auto second = std::make_shared<RenderTarget>(8, 4);
    paint(renderer, first, Color(1,0,0));
    paint(renderer, second, Color(0,0,1));
    require(color(renderer.readPixels(),{0,0,255}), "readPixels did not read the current target's actual Vulkan pixels");
    require(color(renderer.readRenderTargetPixels(*first,0,0,8,4),{255,0,0}), "Parked target lost its actual red GPU pixels");
    first->dispose();
    noPixels(renderer,*first,"Disposed parked target exposed stale pixels");
    require(renderer.getInfo().renderTargets == baseline + 1, "Disposed parked target retained Vulkan storage");
    renderer.setRenderTarget(first);
    noPixels(renderer,*first,"Fresh allocation inherited disposed pixels");
    renderer.setClearColor(Color(0,1,0)); renderer.clear();
    require(color(renderer.readRenderTargetPixels(*first,0,0,8,4),{0,255,0}), "Disposed target did not recreate actual GPU storage");
    const auto sameSizeAllocations=renderer.getInfo().renderTargets;
    first->setSize(8,4);
    require(renderer.getInfo().renderTargets==sameSizeAllocations&&color(renderer.readRenderTargetPixels(*first,0,0,8,4),{0,255,0}),"Same-size setSize incorrectly disposed or replaced live storage");
    first->setSize(5,3);
    noPixels(renderer,*first,"Resized active target exposed pre-resize pixels");
    require(renderer.readPixels().empty(), "readPixels exposed a logically retired active target");
    renderer.setClearColor(Color(0,0,1)); renderer.clear();
    require(renderer.getWidth()==5 && renderer.getHeight()==3, "Clear did not adopt bound target setSize");
    require(color(renderer.readRenderTargetPixels(*first,0,0,5,3),{0,0,255}), "Resized target clear did not reach Vulkan");
    renderer.setRenderTarget(second);
    first->setSize(6,2);
    noPixels(renderer,*first,"Resized parked target exposed old pixels");
    require(color(renderer.readRenderTargetPixels(*second,0,0,8,4),{0,0,255}), "Other target changed during parked resize");
    paint(renderer,first,Color(1,0,0));
    require(renderer.readRenderTargetPixels(*first,0,0,6,2).size()==6*2*4, "Parked resize did not recreate the requested dimensions");
    auto replacement=std::make_shared<RenderTarget>(6,2);
    const auto oldAttachment=first->texture.get();
    first->texture=replacement->texture.get();
    noPixels(renderer,*first,"A replacement color attachment inherited the previous attachment's GPU storage");
    renderer.setClearColor(Color(0,1,0));renderer.clear();oldAttachment->dispose();
    require(color(renderer.readRenderTargetPixels(*first,0,0,6,2),{0,255,0}),"Old attachment disposal changed replacement target pixels");
    rejects<std::out_of_range>([&]{renderer.readRenderTargetPixels(*first,0xffffffffu,0,2,1);},"Readback bounds overflow was accepted");
    renderer.setRenderTarget(nullptr); first.reset(); second.reset();
    require(renderer.getInfo().renderTargets == baseline,"Dropped target owners retained parked GPU storage");
}
void reuseAddress(VulkanRenderer& renderer) {
    const auto baseline=renderer.getInfo().renderTargets;
    alignas(RenderTarget) std::byte memory[sizeof(RenderTarget)];
    auto construct=[&] { return std::shared_ptr<RenderTarget>(new(memory) RenderTarget(4,4),[](RenderTarget* value){value->~RenderTarget();}); };
    auto first=construct(); const auto* address=first.get(); std::weak_ptr<RenderTarget> weak=first;
    paint(renderer,first,Color(1,0,0)); renderer.setRenderTarget(nullptr); first.reset();
    require(weak.expired(),"Parked registration retained its shared owner");
    auto second=construct(); require(second.get()==address,"Placement fixture did not reuse the actual target address");
    noPixels(renderer,*second,"A distinct owner at the same address inherited a prior target's GPU pixels");
    require(renderer.getInfo().renderTargets==baseline,"Owner identity sweep did not retire old-address storage");
    renderer.setRenderTarget(second); noPixels(renderer,*second,"Rebound reused address inherited old pixels");
    renderer.setClearColor(Color(0,0,1)); renderer.clear();
    require(color(renderer.readRenderTargetPixels(*second,0,0,4,4),{0,0,255}),"Reused address did not obtain independent actual GPU storage");
    renderer.setRenderTarget(nullptr); second.reset();
    require(renderer.getInfo().renderTargets==baseline,"Reused-address owner drop retained storage");
}
void retainedBinding(VulkanRenderer& renderer) {
    auto target=std::make_shared<RenderTarget>(4,4);std::weak_ptr<RenderTarget> weak=target;
    renderer.setRenderTarget(target);target.reset();require(!weak.expired(),"Bound target was not retained by its public renderer binding");
    renderer.setRenderTarget(nullptr);require(weak.expired(),"Unbound target retained an owning cache edge");
    renderer.getInfo();
}
void rendererDropDuringDispatch(bool validation) {
    auto target=std::make_shared<RenderTarget>(4,4);
    auto renderer=std::make_unique<VulkanRenderer>(VulkanRendererParameters{8,8,validation});
    bool entered=false;
    const auto listener=target->addEventListener("dispose",[&](Event&){
        if(entered)return;entered=true;renderer.reset();
    });
    paint(*renderer,target,Color(1,0,0));
    target->dispose();
    require(entered&&!renderer,"Actual dispose dispatch did not destroy the old renderer");
    // Its renderer callback is still in EventDispatcher's copied outer list.
    // Calling dispose again also verifies that teardown detached its registration.
    target->dispose();target->removeEventListener("dispose",listener);
}
void dispatchReentry(VulkanRenderer& renderer) {
    auto target=std::make_shared<RenderTarget>(4,4);bool entered=false;
    // Registered first so EventDispatcher copies an old renderer callback into
    // the outer snapshot, then the inner dispose retires that generation.
    const auto listener=target->addEventListener("dispose",[&](Event&){
        if(entered)return;entered=true;target->dispose();
        paint(renderer,target,Color(0,1,0));
    });
    paint(renderer,target,Color(1,0,0));target->dispose();
    require(entered,"Actual dispose callback did not reenter");
    require(color(renderer.readRenderTargetPixels(*target,0,0,4,4),{0,255,0}),"Old dispose dispatch snapshot deleted the newly registered target generation");
    target->removeEventListener("dispose",listener);renderer.setRenderTarget(nullptr);
}
void recordingDispose(VulkanRenderer& renderer, bool abort) {
    auto target=std::make_shared<RenderTarget>(16,16);
    renderer.setRenderTarget(target);renderer.srgbOutput=false;
    auto material=std::make_shared<MeshBasicMaterial>();material->color=Color(1,0,0);
    auto mesh=std::make_shared<Mesh>(std::make_shared<BoxGeometry>(),material);mesh->position.z=-3;
    Scene scene;scene.add(mesh);PerspectiveCamera camera(60,1,.1,100);
    bool entered=false;const auto before=renderer.getInfo().renderTargets;
    mesh->onBeforeRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){
        if(entered)return;entered=true;target->setSize(12,10);
        noPixels(renderer,*target,"Dispose during command recording exposed stale active storage");
        require(renderer.getInfo().renderTargets==before,"In-use Vk images were destroyed before command completion/reset");
        if(abort)throw std::runtime_error("abort recorded target");
    };
    if(abort)rejects<std::runtime_error>([&]{renderer.render(scene,camera);},"Actual render callback did not abort");
    else renderer.render(scene,camera);
    require(entered,"Actual Vulkan draw path did not enter the recording callback");
    noPixels(renderer,*target,"Finished retired command target published stale readback");
    require(renderer.getInfo().renderTargets+1==before,"Retired active target did not release after fence completion/abort");
    mesh->onBeforeRender={};renderer.setClearColor(Color(0,0,1));renderer.clear();
    require(renderer.getWidth()==12&&renderer.getHeight()==10,"Next clear did not recreate callback-resized target");
    require(color(renderer.readRenderTargetPixels(*target,0,0,12,10),{0,0,255}),"Recording dispose recovery did not write actual GPU pixels");
    renderer.setRenderTarget(nullptr);
}
void cubes(VulkanRenderer& renderer) {
    auto source=std::make_shared<WebGLCubeRenderTarget>(8),temporary=std::make_shared<WebGLCubeRenderTarget>(8);
    for(std::uint32_t face=0;face<6;++face)paint(renderer,source,Color(0,1,0),face);
    renderer.blurCube(source,temporary,.1);
    const auto pixels=renderer.readRenderTargetPixels(*source,0,0,8,8);
    require(!pixels.empty()&&pixels[1]>250&&pixels[0]<5&&pixels[2]<5,"Actual cube blur failed to preserve a uniform six-face source");
    renderer.setRenderTarget(nullptr);source->dispose();
    rejects([&]{renderer.blurCube(source,temporary,.1);},"Cube blur sampled a disposed cached source");
    for(std::uint32_t face=0;face<6;++face)paint(renderer,source,Color(0,0,1),face);
    source->setSize(4,4);temporary->setSize(4,4);
    rejects([&]{renderer.blurCube(source,temporary,.1);},"Cube blur sampled pre-resize faces");
    for(std::uint32_t face=0;face<6;++face)paint(renderer,source,Color(1,0,0),face);
    renderer.blurCube(source,temporary,.1);
    require(renderer.readRenderTargetPixels(*source,0,0,4,4).size()==4*4*4,"Recreated cube blur retained old dimensions");
    renderer.setRenderTarget(nullptr);
}
void rejectedBindings(VulkanRenderer& renderer) {
    auto current=std::make_shared<RenderTarget>(4,4);paint(renderer,current,Color(1,0,0));
    auto reject=[&](const std::shared_ptr<RenderTarget>& target,std::uint32_t face=0,std::uint32_t mip=0){
        rejects<std::exception>([&]{renderer.setRenderTarget(target,face,mip);},"Unsupported target binding was silently accepted");
        require(renderer.getRenderTarget()==current&&renderer.getWidth()==4&&renderer.getHeight()==4&&renderer.getActiveCubeFace()==0&&renderer.getActiveMipmapLevel()==0,"Rejected target changed the current binding");
        require(color(renderer.readRenderTargetPixels(*current,0,0,4,4),{255,0,0}),"Rejected target changed existing pixels");
    };
    auto value=std::make_shared<RenderTarget>(4,4);reject(value,0,1);reject(value,1);
    value->samples=4;reject(value);value->samples=0;
    value->depthBuffer=false;reject(value);value->depthBuffer=true;
    value->stencilBuffer=true;reject(value);value->stencilBuffer=false;
    value->depth=2;reject(value);value->depth=1;
    value->multiview=true;reject(value);value->multiview=false;
    value->texture->type=FloatType;reject(value);value->texture->type=UnsignedByteType;
    value->texture->generateMipmaps=true;reject(value);value->texture->generateMipmaps=false;
    value->texture->format=RedFormat;reject(value);value->texture->format=RGBAFormat;
    value->textures.push_back(value->texture.get());reject(value);value->textures.erase(value->textures.end()-1);
    value->depthTexture=std::make_shared<DepthTexture>(4,4);reject(value);value->depthTexture=nullptr;
    value->width=.5;reject(value);value->width=4;
    value->width=std::numeric_limits<double>::quiet_NaN();reject(value);
    value->width=std::numeric_limits<double>::infinity();reject(value);value->width=4;
    value->height=0;reject(value);value->height=4;
    auto cube=std::make_shared<WebGLCubeRenderTarget>(4);reject(cube,6);cube->height=3;reject(cube);
    current->texture->type=FloatType;
    rejects([&]{renderer.clear();},"Clear bypassed unsupported bound target format validation");
    current->texture->type=UnsignedByteType;renderer.clear();
    current->texture->internalFormat="RGBA8";renderer.clear();
    require(color(renderer.readRenderTargetPixels(*current,0,0,4,4),{255,0,0}),"Explicit RGBA8 storage was rejected or changed supported pixels");
    renderer.setRenderTarget(nullptr);
}
}
int main(int argc,char** argv){try{
    const bool validation=argc==2&&std::string(argv[1])=="--validation";
    if(argc>2||(argc==2&&!validation))throw std::invalid_argument("Usage: VulkanRenderTargetLifecycleProbe [--validation]");
    VulkanRenderer renderer({32,24,validation});renderer.srgbOutput=false;
    if(validation)require(renderer.getInfo().validationEnabled,"Requested validation layer is unavailable");
    auto cases=JSValue::array();auto run=[&](const char* name,auto body){body();cases.set(std::to_string(cases.length()),name);};
    run("two-dimensional",[&]{twoDimensional(renderer);});run("same-address-new-owner",[&]{reuseAddress(renderer);});
    run("retained-binding",[&]{retainedBinding(renderer);});run("dispose-dispatch-reentry",[&]{dispatchReentry(renderer);});
    run("renderer-drop-during-dispatch",[&]{rendererDropDuringDispatch(validation);});
    run("recording-dispose-fence",[&]{recordingDispose(renderer,false);});run("recording-dispose-abort",[&]{recordingDispose(renderer,true);});
    run("cube-blur-recreate",[&]{cubes(renderer);});run("rejected-binding-transaction",[&]{rejectedBindings(renderer);});
    renderer.getInfo();const auto messages=renderer.getValidationMessages();
    require(messages.empty(),"Vulkan validation reported lifecycle errors");
    auto result=JSValue::object();result.set("actualGPUExecuted",true);result.set("deviceName",renderer.getInfo().deviceName);result.set("assertions",assertions);result.set("validationEnabled",renderer.getInfo().validationEnabled);result.set("validationMessages",messages.size());
    result.set("renderTargets",renderer.getInfo().renderTargets);result.set("renderTargetImages",renderer.getInfo().renderTargetImages);result.set("cases",cases);std::cout<<result.toJSON()->dump();
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
