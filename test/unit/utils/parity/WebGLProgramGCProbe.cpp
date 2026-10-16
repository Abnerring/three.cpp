#include "WebGLProgramFixture.h"
#include <iostream>
using namespace ProgramFixture;
int main() {
    try {
        auto realm = JSValue::object();
        JSValue detached, instance;
        std::vector<JSWeakValue> weak;
        {
            auto renderer = JSValue::object(), binding = JSValue::object(), gl = JSValue::object(),
                 handle = JSValue::object();
            auto vertex = JSValue::object(), fragment = JSValue::object(),
                 counter = JSValue::object();
            data(counter, "count", 0);
            auto debug = JSValue::object();
            data(debug, "checkShaderErrors", false);
            data(renderer, "debug", debug);
            data(renderer, "getContext",
                 JSValue::function([](const JSValue&, const Args&, const Args& c) { return c[0]; },
                                   {gl}));
            data(binding, "releaseStatesOfProgram",
                 JSValue::function(
                     [](const JSValue&, const Args&, const Args&) { return JSValue(); }));
            data(gl, "VERTEX_SHADER", 1);
            data(gl, "FRAGMENT_SHADER", 2);
            data(gl, "ACTIVE_UNIFORMS", 3);
            data(gl, "ACTIVE_ATTRIBUTES", 4);
            data(gl, "createProgram",
                 JSValue::function([](const JSValue&, const Args&, const Args& c) { return c[0]; },
                                   {handle}));
            data(gl, "createShader",
                 JSValue::function(
                     [](const JSValue&, const Args& args, const Args& c) {
                         return arg(args, 0).strictEquals(1) ? c[0] : c[1];
                     },
                     {vertex, fragment}));
            for (const auto* key : {"shaderSource", "compileShader", "attachShader", "linkProgram",
                                    "deleteShader", "deleteProgram"})
                data(gl, key, JSValue::function([](const JSValue&, const Args&, const Args&) {
                         return JSValue();
                     }));
            data(gl, "getProgramParameter",
                 JSValue::function(
                     [](const JSValue&, const Args&, const Args&) { return JSValue(0); }));
            const auto parameters = defaults();
            WebGLProgram program(renderer, JSValue::object(), parameters, binding);
            weak = {JSWeakValue(renderer),         JSWeakValue(binding),   JSWeakValue(gl),
                    JSWeakValue(handle),           JSWeakValue(vertex),    JSWeakValue(fragment),
                    JSWeakValue(program.object()), JSWeakValue(parameters)};
            detached = program.object().get("isReady");
        }
        const auto snapshot = [&] {
            realm.collectGarbage();
            JsonValue::Array values;
            for (const auto& w : weak)
                values.emplace_back(!w.expired());
            std::cout << JsonValue(values).dump() << '\n';
        };
        snapshot();
        instance = detached.construct();
        detached = {};
        snapshot();
        instance = {};
        snapshot();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
