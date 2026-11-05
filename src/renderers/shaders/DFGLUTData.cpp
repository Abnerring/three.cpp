#include "renderers/shaders/DFGLUTData.h"
#include "textures/DataTexture.h"
#include "core/JSGlobal.h"
#include "core/JSModuleObject.h"
#include "constants.h"

namespace {
using Args=std::vector<JSValue>;
constexpr std::uint16_t data[]={
#include "nodes/functions/BSDF/DFGLUTData.inc"
};
static_assert(sizeof(data)/sizeof(data[0])==512);
// Checked against all 512 uint16 words in the pinned renderer module.
// SHA256 (little-endian words):
// b41ba8e2bd29fa007dfbf034d4d58b853f378062183847846b1f6d228f2c7e79
const char* functionSource="function getDFGLUT() {\r\n\r\n\tif ( lut === null ) {\r\n\r\n\t\tlut = new DataTexture( DATA, 16, 16, RGFormat, HalfFloatType );\r\n\t\tlut.name = 'DFG_LUT';\r\n\t\tlut.minFilter = LinearFilter;\r\n\t\tlut.magFilter = LinearFilter;\r\n\t\tlut.wrapS = ClampToEdgeWrapping;\r\n\t\tlut.wrapT = ClampToEdgeWrapping;\r\n\t\tlut.generateMipmaps = false;\r\n\t\tlut.needsUpdate = true;\r\n\r\n\t}\r\n\r\n\treturn lut;\r\n\r\n}";
JSValue invoke(const JSValue&,const Args&,const Args& captures){
    auto environment=captures[0];auto lut=environment.getInternalSlot("lut");
    if(lut.isNull()){
        const auto ctor=environment.getInternalSlot("DataTexture");
        lut=ctor.construct({environment.getInternalSlot("DATA"),16,16,environment.getInternalSlot("RGFormat"),environment.getInternalSlot("HalfFloatType")});
        // The source commits this binding before the observable setters. A
        // throwing setter or reentrant call must observe the same partial LUT.
        environment.setInternalSlot("lut",lut);
        lut.set("name","DFG_LUT");
        lut.set("minFilter",environment.getInternalSlot("LinearFilter"));
        lut.set("magFilter",environment.getInternalSlot("LinearFilter"));
        lut.set("wrapS",environment.getInternalSlot("ClampToEdgeWrapping"));
        lut.set("wrapT",environment.getInternalSlot("ClampToEdgeWrapping"));
        lut.set("generateMipmaps",false);lut.set("needsUpdate",true);
    }
    return lut;
}
}
JSValue nativeGetDFGLUTFunction(){
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isCallable())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"renderers/shaders/DFGLUTData");
    // Imports evaluate before DATA, which resolves the current global
    // Uint16Array constructor at module initialization, as the source does.
    auto environment=JSValue::object(nullptr);environment.setInternalSlot("DataTexture",nativeDataTextureConstructor());
    for(const char* name:{"RGFormat","HalfFloatType","LinearFilter","ClampToEdgeWrapping"})environment.setInternalSlot(name,nativeConstantsBinding(name));
    auto array=JSValue::array();for(std::size_t i=0;i<512;++i)array.set(JSValue(double(i)),data[i]);
    const auto uint16=nativeGlobalBinding("Uint16Array");environment.setInternalSlot("DATA",uint16.construct({array}));environment.setInternalSlot("lut",nullptr);
    auto function=JSValue::constructor(invoke,{environment},"getDFGLUT",0,JSValue::ConstructorKind::Callable);function.setFunctionSourceText(functionSource);
    function.retainForHeapLifetime();weak=JSWeakValue(function);return function;
}
JSValue getDFGLUT(){return nativeGetDFGLUTFunction().call(nullptr);}
