#include "nodes/utils/SpriteSheetUV.h"

#include "nodes/accessors/UV.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue value;
    return value;
}

JSValue body(const JSValue&, const Args& args, const Args& captures) {
    const auto table = captures[0];
    const auto inputs = arg(args, 0);
    const auto countNode = inputs.get(JSValue(0));
    auto uvNode = inputs.get(JSValue(1));
    if (uvNode.isUndefined()) uvNode = table.get("uv").call();
    auto frameNode = inputs.get(JSValue(2));
    if (frameNode.isUndefined()) frameNode = table.get("float").call({}, {0});

    const auto width = countNode.get("x");
    const auto height = countNode.get("y");

    const auto frameNum = invoke(invoke(frameNode, "mod")({invoke(width, "mul")({height})}), "floor")();
    const auto column = invoke(frameNum, "mod")({width});
    const auto row = invoke(height, "sub")({
        invoke(invoke(invoke(frameNum, "add")({1}), "div")({width}), "ceil")()
    });

    const auto scale = invoke(countNode, "reciprocal")();
    const auto uvFrameOffset = table.get("vec2").call({}, {column, row});
    return invoke(invoke(uvNode, "add")({uvFrameOffset}), "mul")({scale});
}
} // namespace

JSValue nativeSpriteSheetUVModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "SpriteSheetUV module");
    nativeUVModule();
    nativeTSLBase();

    auto table = JSValue::object(nullptr);
    own(table, "uv", nativeUVBinding("uv"));
    own(table, "float", nativeTSLBaseBinding("float"));
    own(table, "vec2", nativeTSLBaseBinding("vec2"));

    auto callback = JSValue::function(body, {table}, "", 1);
    callback.setFunctionSourceText(
        "( [ countNode, uvNode = uv(), frameNode = float( 0 ) ] ) => {\r\n\r\n"
        "\tconst width = countNode.x;\r\n"
        "\tconst height = countNode.y;\r\n\r\n"
        "\tconst frameNum = frameNode.mod( width.mul( height ) ).floor();\r\n\r\n"
        "\tconst column = frameNum.mod( width );\r\n"
        "\tconst row = height.sub( frameNum.add( 1 ).div( width ).ceil() );\r\n\r\n"
        "\tconst scale = countNode.reciprocal();\r\n"
        "\tconst uvFrameOffset = vec2( column, row );\r\n\r\n"
        "\treturn uvNode.add( uvFrameOffset ).mul( scale );\r\n\r\n"
        "}");
    const auto value = nativeTSLBaseBinding("Fn").call({}, {callback});

    auto bindings = JSValue::object(nullptr);
    own(bindings, "spritesheetUV", value);
    auto module = JSValue::object(nullptr);
    own(module, "spritesheetUV", value);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeSpriteSheetUVBinding(const std::string& name) {
    const auto keeper = nativeSpriteSheetUVModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeSpriteSheetUV() {
    return nativeSpriteSheetUVBinding("spritesheetUV");
}
