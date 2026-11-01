#include "renderers/VulkanRenderer.h"
#include "cameras/OrthographicCamera.h"
#include "geometries/PlaneGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"
#include "scenes/Scene.h"
#include "core/JSPromise.h"
#include "animation/AnimationUtils.h"
#include <iostream>
#include <iterator>
#include <cmath>
#include <stdexcept>
#include <string_view>

namespace {
int assertions=0;
void require(bool condition,const char* message){++assertions;if(!condition)throw std::runtime_error(message);}
void push(JSValue target,JSValue value){target.set(std::to_string(target.length()),std::move(value));}
// GPU_PIXEL_DIAGNOSTIC_BEGIN
void describePixel(const char* stage,const JSValue& request,int frame,bool errorSame,
                   const MeshBasicMaterial& material,const VulkanRenderer& renderer,
                   int uploads,const std::vector<std::uint8_t>& pixels,int expectedRed,int expectedBlue){
    const auto center=(8*16+8)*4;
    std::cerr<<"GPU_PIXEL_DIAGNOSTIC {\"stage\":\""<<stage
             <<"\",\"id\":\""<<request.get("id").string()<<"\",\"frame\":"<<frame
             <<",\"errorSame\":"<<(errorSame?"true":"false")<<",\"side\":"<<material.side
             <<",\"version\":"<<material.version<<",\"calls\":"<<renderer.getInfo().calls
             <<",\"uploads\":"<<uploads<<",\"materialRGB\":["<<material.color.r<<','<<material.color.g<<','<<material.color.b
             <<"],\"actual\":["<<int(pixels[center])<<','<<int(pixels[center+1])<<','<<int(pixels[center+2])<<','<<int(pixels[center+3])
             <<"],\"expected\":["<<expectedRed<<",0,"<<expectedBlue<<",255]}\n";
}
// GPU_PIXEL_DIAGNOSTIC_END
JSValue run(const JSValue& request,VulkanRenderer& renderer){
    auto geometry=std::make_shared<PlaneGeometry>(1.6,1.6);
    auto material=std::make_shared<MeshBasicMaterial>(Color(1,0,0));
    material->transparent=request.get("transparent").toBoolean();
    material->side=static_cast<int>(request.get("side").toNumber());
    material->forceSinglePass=request.get("forceSinglePass").toBoolean();
    auto object=std::make_shared<Mesh>(geometry,material);Scene scene;scene.add(object);
    const bool swapIndex=request.get("swapIndex").toBoolean();
    if(request.get("backFacing").toBoolean())object->rotation.y=std::acos(-1.0);
    OrthographicCamera camera(-1,1,1,-1,.1,20);camera.position.z=5;
    auto events=JSValue::array(),frames=JSValue::array(),reason=JSValue::object();reason.set("originalUploadFailure",true);
    const auto initialVersion=material->version;int frame=0,uploads=0,secondUploads=0;
    auto observations=JSValue::array();
    const auto event=[&](const char* kind){auto entry=JSValue::object();entry.set("frame",frame);entry.set("kind",kind);entry.set("side",material->side);entry.set("versionDelta",material->version-initialVersion);push(events,entry);};
    object->onBeforeRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){event("before");};
    object->onAfterRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){event("after");};
    const auto index=geometry->index;require(bool(index),"Actual indexed geometry is required");
    std::shared_ptr<Uint16BufferAttribute> reversed;
    if(swapIndex)reversed=std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0,1,2,2,1,3},1);
    if(swapIndex)reversed->onUpload([&]{
        ++uploads;++secondUploads;event("upload-second");
        auto entry=JSValue::object();entry.set("frame",frame);entry.set("kind","second-upload");entry.set("side",material->side);entry.set("versionDelta",material->version-initialVersion);entry.set("calls",renderer.getInfo().calls);push(observations,entry);
        require(renderer.getInfo().calls==(frame==0?1:0),"Second real index upload did not observe the issued passes of this frame");
        if(request.get("secondReadback").toBoolean()&&secondUploads==1){
            event("second-readback-enter");const auto pixels=renderer.readPixels();
            require(pixels.size()==16*16*4,"Second upload callback actual GPU readback is missing");const auto center=(8*16+8)*4;
            // GPU_PIXEL_DIAGNOSTIC_BEGIN
            if(!(pixels[center]==255&&pixels[center+1]==0&&pixels[center+2]==0&&pixels[center+3]==255))describePixel("second-upload-readback",request,frame,false,*material,renderer,uploads,pixels,255,0);
            // GPU_PIXEL_DIAGNOSTIC_END
            require(pixels[center]==255&&pixels[center+1]==0&&pixels[center+2]==0&&pixels[center+3]==255,"Second upload did not see the first pass's immutable red GPU snapshot");
            event("readback");event("second-readback-exit");
        }
        if(request.get("throwSecond").toBoolean()&&secondUploads==1)throw JSThrownValue(reason);
    });
    index->onUpload([&]{
        ++uploads;event("upload");
        if(swapIndex&&uploads==1){geometry->setIndex(reversed);material->color.setRGB(0,0,1);}
        if(request.get("reenter").toBoolean()&&uploads==1){
            event("readback-enter");
            const auto pixels=renderer.readPixels();
            require(pixels.size()==16*16*4,"Upload callback readback did not execute the actual GPU copy");
            const auto center=(8*16+8)*4;
            // GPU_PIXEL_DIAGNOSTIC_BEGIN
            if(!(pixels[center]==0&&pixels[center+1]==0&&pixels[center+2]==0&&pixels[center+3]==255))describePixel("first-upload-readback",request,frame,false,*material,renderer,uploads,pixels,0,0);
            // GPU_PIXEL_DIAGNOSTIC_END
            require(pixels[center]==0&&pixels[center+1]==0&&pixels[center+2]==0&&pixels[center+3]==255,"Upload callback readback did not observe the pre-draw background");
            event("readback");event("readback-exit");
        }
        if(request.get("throwFirst").toBoolean()&&uploads==1)throw JSThrownValue(reason);
    });
    for(frame=0;frame<2;++frame){
        bool errorSame=false;
        try{renderer.render(scene,camera);}catch(const JSThrownValue& error){require(error.value.sameValue(reason),"Upload callback throw value identity changed");errorSame=error.value.sameValue(reason);}
        require(errorSame==((request.get("throwFirst").toBoolean()||request.get("throwSecond").toBoolean())&&frame==0),"Upload failure was swallowed or repeated");
        auto state=JSValue::object();state.set("frame",frame);state.set("errorSame",errorSame);state.set("calls",renderer.getInfo().calls);state.set("side",material->side);state.set("versionDelta",material->version-initialVersion);state.set("uploads",uploads);push(frames,state);
        if(!errorSame||swapIndex){
            const auto pixels=renderer.readPixels();require(pixels.size()==16*16*4,"Actual post-draw GPU pixels are missing");const auto center=(8*16+8)*4;
            const auto red=swapIndex?(errorSame?255:0):(material->side==BackSide?0:255);
            // Actual source VAO trace: failed second index upload leaves the new
            // FrontSide position slot uninitialized; unchanged retry skips setup.
            const bool uninitializedPositionRetry=swapIndex&&request.get("throwSecond").toBoolean()&&frame==1;
            const auto blue=swapIndex&&!errorSame&&!uninitializedPositionRetry?255:0;
            // GPU_PIXEL_DIAGNOSTIC_BEGIN
            if(!(pixels[center]==red&&pixels[center+1]==0&&pixels[center+2]==blue&&pixels[center+3]==255))describePixel("post-render",request,frame,errorSame,*material,renderer,uploads,pixels,red,blue);
            // GPU_PIXEL_DIAGNOSTIC_END
            require(pixels[center]==red&&pixels[center+1]==0&&pixels[center+2]==blue&&pixels[center+3]==255,"Actual culling/pass snapshot/exception-preserved/next-frame GPU pixel differs");
        }
    }
    index->onUpload(nullptr);if(reversed)reversed->onUpload(nullptr);object->onBeforeRender={};object->onAfterRender={};
    auto result=JSValue::object();result.set("id",request.get("id"));result.set("events",events);result.set("frames",frames);if(swapIndex)result.set("observations",observations);return result;
}
}
int main(int argc,char** argv){try{
    const bool validation=argc==2&&std::string_view(argv[1])=="--validation";
    if(argc>2||(argc==2&&!validation))throw std::invalid_argument("Usage: VulkanDoubleSideCallbacksProbe [--validation] < fixtures.json");
    auto input=Animation_detail::parseJSON(std::string(std::istreambuf_iterator<char>(std::cin),{}));
    auto entry=JSValue::function(+[](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>& captures)->JSValue{
        VulkanRenderer renderer({16,16,captures[1].toBoolean()});renderer.srgbOutput=false;renderer.setClearColor(Color(0,0,0),1);
        if(captures[1].toBoolean())require(renderer.getInfo().validationEnabled,"Requested actual validation layer is unavailable");
        auto cases=JSValue::array();for(unsigned i=0;i<captures[0].length();++i)push(cases,run(captures[0].get(JSValue(i)),renderer));
        require(renderer.getValidationMessages().empty(),"Actual Vulkan validation reported an error");
        auto result=JSValue::object();result.set("actualGPUExecuted",true);result.set("deviceName",renderer.getInfo().deviceName);result.set("validationEnabled",renderer.getInfo().validationEnabled);result.set("validationMessages",renderer.getValidationMessages().size());result.set("assertions",assertions);result.set("cases",cases);return result;
    },{input,validation});
    std::cout<<entry.call().toJSON()->dump();
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
