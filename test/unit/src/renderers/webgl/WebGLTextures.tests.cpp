#include "catch.hpp"
#include "../../../utils/parity/WebGLTexturesFixtures.h"
#include "../../../utils/parity/WebGLTexturesEnumFixture.h"
#include "animation/AnimationUtils.h"
#include "core/JSJobs.h"
#include "utils.h"
#include <exception>
#include <thread>

namespace {
using namespace WebGLTexturesFixtures;
JSValue enums(){return Animation_detail::parseJSON(webglTexturesCommandEnums);}
}
TEST_CASE("WebGLTextures preserves factory metadata and all original service entry points", "[renderers][webgl][textures]"){
    auto c=makeContext(object(),enums());const auto value=c.get("manager"),ctor=nativeWebGLTexturesConstructor();
    REQUIRE(ctor.sameValue(nativeWebGLTexturesConstructor()));
    REQUIRE(ctor.get("length").strictEquals(7));
    REQUIRE(ctor.get("name").strictEquals("WebGLTextures"));
    REQUIRE(ctor.nativeFunctionSource().starts_with("function WebGLTextures( _gl, extensions, state, properties, capabilities, utils, info )"));
    const std::vector<std::string> keys={"allocateTextureUnit","resetTextureUnits","getTextureUnits","setTextureUnits","setTexture2D","setTexture2DArray","setTexture3D","setTextureCube","rebindTextures","setupRenderTarget","updateRenderTargetMipmap","updateMultisampleRenderTarget","setupDepthRenderbuffer","setupFrameBufferTexture","useMultisampledRTT","dispose"};
    REQUIRE(value.keys()==keys);
    for(const auto& key:keys){
        const auto descriptor=value.getOwnPropertyDescriptor(key);
        REQUIRE(descriptor->writable==true);REQUIRE(descriptor->enumerable==true);REQUIRE(descriptor->configurable==true);
        REQUIRE(value.get(key).isConstructible());
    }
    REQUIRE(call(value,"allocateTextureUnit").strictEquals(0));
    REQUIRE(call(value,"allocateTextureUnit").strictEquals(1));
    REQUIRE(call(value,"getTextureUnits").strictEquals(2));
    REQUIRE(call(value,"setTextureUnits",{9}).isUndefined());
    REQUIRE(call(value,"allocateTextureUnit").strictEquals(9));
    REQUIRE(call(value,"resetTextureUnits").isUndefined());
    REQUIRE(call(value,"getTextureUnits").strictEquals(0));
}
TEST_CASE("WebGLTextures shares source storage by cache key and releases each native command handle once", "[renderers][webgl][textures]"){
    auto c=makeContext(object(),enums());auto first=makeTexture(c,"data"),second=makeTexture(c,"data");second.set("source",first.get("source"));
    REQUIRE(upload(c,first).isUndefined());
    REQUIRE(upload(c,second).isUndefined());
    const auto firstProperties=call(c.get("properties"),"get",{first}),secondProperties=call(c.get("properties"),"get",{second});
    REQUIRE(firstProperties.get("__webglTexture").sameValue(secondProperties.get("__webglTexture")));
    REQUIRE(c.get("info").get("memory").get("textures").strictEquals(1));
    call(first,"dispose");
    REQUIRE(c.get("info").get("memory").get("textures").strictEquals(1));
    call(second,"dispose");
    REQUIRE(c.get("info").get("memory").get("textures").strictEquals(0));
    double deletions=0;for(double i=0;i<c.get("events").length();++i)if(c.get("events").get(JSValue(i)).get("0").strictEquals("gl.deleteTexture"))++deletions;
    REQUIRE(deletions==1);
    REQUIRE(call(c.get("manager"),"dispose").isUndefined());
    endJSJob();
}
TEST_CASE("WebGLTextures merges actual update range objects and clears the original array", "[renderers][webgl][textures]"){
    auto c=makeContext(object(),enums()),t=makeTexture(c,"data");
    call(t,"addUpdateRange",{8,4});call(t,"addUpdateRange",{0,4});call(t,"addUpdateRange",{3,5});
    const auto original=t.get("updateRanges"),first=original.get("1"),overlap=original.get("2");
    REQUIRE(upload(c,t).isUndefined());
    REQUIRE(t.get("updateRanges").sameValue(original));
    REQUIRE(original.length()==0);
    REQUIRE(first.get("count").strictEquals(12));
    REQUIRE(overlap.get("count").strictEquals(5));
    call(c.get("manager"),"dispose");endJSJob();
}
TEST_CASE("createElementNS invokes the native document host with original raw arguments", "[renderers][webgl][textures][host]"){
    auto global=nativeGlobalObject();const auto saved=global.getOwnPropertyDescriptor("document");auto document=object(),events=list(),token=object();
    document.set("createElementNS",JSValue::function(+[](const JSValue& self,const Args& args,const Args& cap)->JSValue{push(cap[0],list({self.sameValue(cap[1]),arg(args,0),arg(args,1)}));return cap[2];},{events,document,token}));global.set("document",document);
    try{
        REQUIRE(ThreeUtils::createElementNS(token).sameValue(token));
        REQUIRE(events.get("0").get("0").strictEquals(true));
        REQUIRE(events.get("0").get("1").strictEquals("http://www.w3.org/1999/xhtml"));
        REQUIRE(events.get("0").get("2").sameValue(token));
        REQUIRE(ThreeUtils::nativeCreateElementNSFunction().get("name").strictEquals("createElementNS"));
    }catch(...){if(saved)global.defineProperty("document",*saved);else global.erase("document");throw;}
    if(saved)global.defineProperty("document",*saved);else global.erase("document");
}
TEST_CASE("WebGLTextures source WeakRef tracking releases retired native heaps", "[renderers][webgl][textures][gc]"){
    std::exception_ptr failure;std::thread worker([&]{try{
        REQUIRE_FALSE(JSValue::hasActiveHeap());JSWeakValue factory,manager,texture;
        {
            auto keeper=object();
            {
                const auto c=makeContext(object(),enums());auto t=makeTexture(c,"data");factory=JSWeakValue(nativeWebGLTexturesConstructor());manager=JSWeakValue(c.get("manager"));texture=JSWeakValue(t);upload(c,t);call(c.get("manager"),"dispose");
            }
            endJSJob();keeper.collectGarbage();drainFinalizationJobs();keeper.collectGarbage();
            REQUIRE(manager.expired());REQUIRE(texture.expired());REQUIRE_FALSE(factory.expired());
        }
        REQUIRE(factory.expired());REQUIRE_FALSE(JSValue::hasActiveHeap());
    }catch(...){failure=std::current_exception();}});worker.join();if(failure)std::rethrow_exception(failure);
}
