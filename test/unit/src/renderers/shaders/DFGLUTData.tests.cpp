#include "catch.hpp"
#include "renderers/shaders/DFGLUTData.h"
#include "core/JSGlobal.h"
#include "core/JSJobs.h"
#include "constants.h"
#include <thread>

namespace {
using Args=std::vector<JSValue>;
constexpr std::uint16_t expected[]={
#include "nodes/functions/BSDF/DFGLUTData.inc"
};
}
TEST_CASE("Canonical renderer DFG LUT uses exact half float data and its own persistent source cache","[renderers][dfg-lut]"){
    const auto function=nativeGetDFGLUTFunction();REQUIRE(function.sameValue(nativeGetDFGLUTFunction()));
    REQUIRE(function.get("name").strictEquals("getDFGLUT"));REQUIRE(function.get("length").strictEquals(0));REQUIRE(function.isConstructible());
    auto lut=getDFGLUT();REQUIRE(function.call(JSValue::object(),{1,2}).sameValue(lut));REQUIRE(function.construct().sameValue(lut));
    REQUIRE(lut.get("isDataTexture").strictEquals(true));REQUIRE_FALSE(lut.isHostObject("three.Texture"));
    REQUIRE(lut.get("name").strictEquals("DFG_LUT"));REQUIRE(lut.get("format").strictEquals(RGFormat));REQUIRE(lut.get("type").strictEquals(HalfFloatType));
    REQUIRE(lut.get("minFilter").strictEquals(LinearFilter));REQUIRE(lut.get("magFilter").strictEquals(LinearFilter));
    REQUIRE(lut.get("wrapS").strictEquals(ClampToEdgeWrapping));REQUIRE(lut.get("wrapT").strictEquals(ClampToEdgeWrapping));
    REQUIRE(lut.get("generateMipmaps").strictEquals(false));REQUIRE(lut.get("version").strictEquals(1));REQUIRE(lut.get("source").get("version").strictEquals(1));
    const auto image=lut.get("image"),data=image.get("data");REQUIRE(image.get("width").strictEquals(16));REQUIRE(image.get("height").strictEquals(16));
    REQUIRE(data.get("constructor").get("name").strictEquals("Uint16Array"));REQUIRE(data.get("length").strictEquals(512));
    for(unsigned i=0;i<512;++i)REQUIRE(data.get(JSValue(i)).strictEquals(expected[i]));
    lut.set("name","kept mutation");lut.get("dispose").call(lut);REQUIRE(getDFGLUT().sameValue(lut));REQUIRE(getDFGLUT().get("name").strictEquals("kept mutation"));
    REQUIRE(getDFGLUT().get("version").strictEquals(1));lut.set("name","DFG_LUT");
}
TEST_CASE("Renderer DFG module resolves Uint16Array during module initialization and retires its realm","[renderers][dfg-lut][gc]"){
    std::exception_ptr failure;std::thread worker([&]{try{
        JSWeakValue weakFunction,weakTexture;
        {
            auto global=nativeGlobalObject();const auto saved=global.getOwnPropertyDescriptor("Uint16Array");const auto original=global.get("Uint16Array");
            auto events=JSValue::array();auto spy=JSValue::constructor(+[](const JSValue&,const Args& args,const Args& cap)->JSValue{
                auto events=cap[0];events.set(JSValue(events.length()),args.at(0).get("length"));return cap[1].construct(args);
            },{events,original},"Uint16Spy",1,JSValue::ConstructorKind::Callable);global.set("Uint16Array",spy);
            auto function=nativeGetDFGLUTFunction();REQUIRE(events.length()==1);REQUIRE(events.get("0").strictEquals(512));
            auto lut=function.call(nullptr);REQUIRE(events.length()==1);weakFunction=JSWeakValue(function);weakTexture=JSWeakValue(lut);
            if(saved)global.defineProperty("Uint16Array",*saved);else global.erase("Uint16Array");
            function={};lut={};global.collectGarbage();REQUIRE_FALSE(weakFunction.expired());REQUIRE_FALSE(weakTexture.expired());
        }
        JSValue::nativeSafepoint();REQUIRE(weakFunction.expired());REQUIRE(weakTexture.expired());REQUIRE_FALSE(JSValue::hasActiveHeap());
    }catch(...){failure=std::current_exception();}});worker.join();if(failure)std::rethrow_exception(failure);
}
