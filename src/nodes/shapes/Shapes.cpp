#include "nodes/shapes/Shapes.h"

#include "nodes/accessors/UV.h"
#include "nodes/math/ConditionalNode.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLFunctionRuntime.h"

namespace {
using namespace TSLFunctionRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue body(const JSValue&, const Args& args, const Args& captures) {
    const auto table = captures[0];
    auto coord = arg(args, 0);
    if (coord.isUndefined()) coord = table.get("uv").call();

    const auto len2 = table.get("lengthSq").call({}, {
        invoke(invoke(coord, "mul")({2.0}), "sub")({1.0})
    });

    const auto builder = arg(args, 1);
    const auto renderer = builder.get("renderer");
    const auto material = builder.get("material");

    JSValue alpha;
    if (material.get("alphaToCoverage").toBoolean() && renderer.get("currentSamples").toNumber() > 0.0) {
        const auto dlen = invoke(table.get("float").call({}, {invoke(len2, "fwidth")()}), "toVar")();
        alpha = invoke(table.get("smoothstep").call({}, {
            invoke(dlen, "oneMinus")(),
            invoke(dlen, "add")({1}),
            len2
        }), "oneMinus")();
    } else {
        alpha = table.get("select").call({}, {
            invoke(len2, "greaterThan")({1.0}),
            0,
            1
        });
    }

    return alpha;
}

} // namespace

JSValue nativeShapesModule() {
    using namespace TSLFunctionRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "Shapes module");
    nativeUVModule();
    nativeTSLBase();
    nativeMathNodeModule();
    nativeConditionalNodeModule();

    auto table = JSValue::object(nullptr);
    own(table, "uv", nativeUVBinding("uv"));
    own(table, "float", nativeTSLBaseBinding("float"));
    own(table, "select", nativeConditionalNodeBinding("select"));
    own(table, "lengthSq", nativeMathNodeBinding("lengthSq"));
    own(table, "smoothstep", nativeMathNodeBinding("smoothstep"));

    auto callback = JSValue::function(body, {table}, "", 2);
    callback.setFunctionSourceText(
        "( [ coord = uv() ], { renderer, material } ) => {\r\n\r\n"
        "\tconst len2 = lengthSq( coord.mul( 2 ).sub( 1 ) );\r\n\r\n"
        "\tlet alpha;\r\n\r\n"
        "\tif ( material.alphaToCoverage && renderer.currentSamples > 0 ) {\r\n\r\n"
        "\t\tconst dlen = float( len2.fwidth() ).toVar();\r\n\r\n"
        "\t\talpha = smoothstep( dlen.oneMinus(), dlen.add( 1 ), len2 ).oneMinus();\r\n\r\n"
        "\t} else {\r\n\r\n"
        "\t\talpha = select( len2.greaterThan( 1.0 ), 0, 1 );\r\n\r\n"
        "\t}\r\n\r\n"
        "\treturn alpha;\r\n\r\n"
        "}"
    );
    const auto value = nativeTSLBaseBinding("Fn").call({}, {callback});

    auto bindings = JSValue::object(nullptr);
    own(bindings, "shapeCircle", value);
    auto module = JSValue::object(nullptr);
    own(module, "shapeCircle", value);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeShapesBinding(const std::string& name) {
    const auto keeper = nativeShapesModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeShapeCircle() { return nativeShapesBinding("shapeCircle"); }
