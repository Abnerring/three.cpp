#include "nodes/functions/BSDF/DFGLUT.h"

#include "constants.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "textures/DataTexture.h"
#include "core/JSGlobal.h"

#include <array>
#include <cstdint>

namespace {
using namespace TSLFunctionRuntime;

constexpr std::array<std::uint16_t, 512> DATA = {
#include "DFGLUTData.inc"
};

JSValue lut;
JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue makeLut() {
    if (lut.isObject()) return lut;
    auto data = nativeGlobalBinding("Uint16Array").construct({static_cast<double>(DATA.size())});
    for (std::size_t i = 0; i < DATA.size(); ++i) data.set(JSValue(static_cast<double>(i)), DATA[i]);
    lut = nativeDataTextureConstructor().construct({data, 16, 16, RGFormat, HalfFloatType});
    lut.set("name", "DFG_LUT");
    lut.set("minFilter", LinearFilter);
    lut.set("magFilter", LinearFilter);
    lut.set("wrapS", ClampToEdgeWrapping);
    lut.set("wrapT", ClampToEdgeWrapping);
    lut.set("generateMipmaps", false);
    lut.set("needsUpdate", true);
    return lut;
}

JSValue body(const JSValue&, const Args& args, const Args&) {
    const auto inputs = arg(args, 0);
    const auto uv = nativeTSLBaseBinding("vec2").call({}, {inputs.get("roughness"), inputs.get("dotNV")});
    const auto sampled = nativeTextureNodeBinding("texture").call({}, {makeLut(), uv});
    return sampled.get("rg");
}
} // namespace

JSValue nativeDFGLUTModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "DFGLUT module");
    auto callback = JSValue::function(body, {}, "", 1);
    callback.setFunctionSourceText("( { roughness, dotNV } ) => {\r\n\r\n\tif ( lut === null ) {\r\n\r\n\t\tlut = new DataTexture( DATA, 16, 16, RGFormat, HalfFloatType );\r\n\t\tlut.name = 'DFG_LUT';\r\n\t\tlut.minFilter = LinearFilter;\r\n\t\tlut.magFilter = LinearFilter;\r\n\t\tlut.wrapS = ClampToEdgeWrapping;\r\n\t\tlut.wrapT = ClampToEdgeWrapping;\r\n\t\tlut.generateMipmaps = false;\r\n\t\tlut.needsUpdate = true;\r\n\r\n\t}\r\n\r\n\tconst uv = vec2( roughness, dotNV );\r\n\r\n\treturn texture( lut, uv ).rg;\r\n\r\n}");
    auto layout = JSValue::object();
    own(layout, "roughness", "float");
    own(layout, "dotNV", "float");
    own(layout, "return", "vec2");
    const auto value = nativeTSLBaseBinding("Fn").call({}, {callback, layout});

    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", value);
    auto module = JSValue::object(nullptr);
    own(module, "default", value);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeDFGLUTBinding(const std::string& name) {
    const auto keeper = nativeDFGLUTModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeDFGLUT() {
    return nativeDFGLUTBinding("default");
}
