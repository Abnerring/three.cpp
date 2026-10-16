#include "catch.hpp"
#include "parity/WebGLProgramFixture.h"
#include <thread>
using namespace ProgramFixture;
namespace {
struct Commands {
    JSValue state = JSValue::object(), gl = JSValue::object(), renderer = JSValue::object(),
            bindings = JSValue::object();
    Commands() {
        data(state, "deletes", 0);
        data(state, "links", 0);
        data(state, "uniformQueries", 0);
        data(state, "attributeQueries", 0);
        data(state, "released", 0);
        data(state, "deletePrograms", 0);
        data(state, "failAttributes", false);
        data(state, "ready", false);
        data(state, "readyQueries", 0);
        for (const auto* key :
             {"VERTEX_SHADER", "FRAGMENT_SHADER", "ACTIVE_ATTRIBUTES", "ACTIVE_UNIFORMS"})
            data(gl, key, key);
        data(gl, "createProgram", JSValue::function([](const JSValue&, const Args&, const Args&) {
                 return JSValue::object();
             }));
        data(gl, "createShader", JSValue::function([](const JSValue&, const Args&, const Args&) {
                 return JSValue::object();
             }));
        for (const auto* key :
             {"shaderSource", "compileShader", "attachShader", "bindAttribLocation"})
            data(gl, key, JSValue::function([](const JSValue&, const Args&, const Args&) {
                     return JSValue();
                 }));
        for (const auto& [method, count] :
             std::vector<std::pair<std::string, std::string>>{{"linkProgram", "links"},
                                                              {"deleteShader", "deletes"},
                                                              {"deleteProgram", "deletePrograms"}})
            data(gl, method,
                 JSValue::function(
                     [](const JSValue&, const Args&, const Args& c) {
                         auto s = c[0];
                         s.set(c[1], s.get(c[1]).toNumber() + 1);
                         return JSValue();
                     },
                     {state, count}));
        data(gl, "getProgramParameter",
             JSValue::function(
                 [](const JSValue&, const Args& a, const Args& c) -> JSValue {
                     auto s = c[0];
                     if (a[1].strictEquals("ACTIVE_UNIFORMS")) {
                         s.set("uniformQueries", s.get("uniformQueries").toNumber() + 1);
                         return 0;
                     }
                     if (a[1].strictEquals("ACTIVE_ATTRIBUTES")) {
                         s.set("attributeQueries", s.get("attributeQueries").toNumber() + 1);
                         if (s.get("failAttributes").toBoolean())
                             throw JSRangeError("attributes");
                         return 0;
                     }
                     s.set("readyQueries", s.get("readyQueries").toNumber() + 1);
                     return s.get("ready");
                 },
                 {state}));
        data(bindings, "releaseStatesOfProgram",
             JSValue::function(
                 [](const JSValue&, const Args&, const Args& c) {
                     auto s = c[0];
                     s.set("released", s.get("released").toNumber() + 1);
                     return JSValue();
                 },
                 {state}));
        data(renderer, "getContext",
             JSValue::function([](const JSValue&, const Args&, const Args& c) { return c[0]; },
                               {gl}));
        auto debug = JSValue::object();
        data(debug, "checkShaderErrors", false);
        data(renderer, "debug", debug);
    }
};
} // namespace
TEST_CASE("WebGLProgram caches one actual uniforms object and one attribute map",
          "[webgl-program]") {
    Commands commands;
    WebGLProgram program(commands.renderer, "key", defaults(), commands.bindings);
    REQUIRE(commands.state.get("links").number() == 1);
    auto first = program.getUniforms(), attributes = program.getAttributes();
    REQUIRE(first.sameValue(program.getUniforms()));
    REQUIRE(attributes.sameValue(program.getAttributes()));
    REQUIRE(first.get("seq").length() == 0);
    REQUIRE(commands.state.get("deletes").number() == 2);
    REQUIRE(commands.state.get("uniformQueries").number() == 1);
    REQUIRE(commands.state.get("attributeQueries").number() == 1);
    program.destroy();
    REQUIRE(program.object().get("program").isUndefined());
    REQUIRE(commands.state.get("released").number() == 1);
    REQUIRE(commands.state.get("deletePrograms").number() == 1);
    REQUIRE(first.sameValue(program.getUniforms()));
    program.destroy();
    REQUIRE(commands.state.get("released").number() == 2);
    REQUIRE(commands.state.get("deletePrograms").number() == 2);
}
TEST_CASE("WebGLProgram preserves uniforms initialization when later attribute "
          "discovery throws",
          "[webgl-program]") {
    Commands c;
    data(c.state, "failAttributes", true);
    WebGLProgram p(c.renderer, "key", defaults(), c.bindings);
    REQUIRE_THROWS_AS(p.getAttributes(), JSRangeError);
    const auto first = p.getUniforms();
    REQUIRE(first.isObject());
    REQUIRE(c.state.get("uniformQueries").number() == 1);
    REQUIRE(c.state.get("deletes").number() == 2);
    data(c.state, "failAttributes", false);
    REQUIRE(p.getAttributes().isObject());
    REQUIRE_FALSE(first.sameValue(p.getUniforms()));
    REQUIRE(c.state.get("uniformQueries").number() == 2);
    REQUIRE(c.state.get("deletes").number() == 4);
}
TEST_CASE("WebGLProgram readiness caches values other than strict false", "[webgl-program]") {
    Commands c;
    auto parameters = defaults();
    data(parameters, "rendererExtensionParallelShaderCompile", true);
    WebGLProgram p(c.renderer, "key", parameters, c.bindings);
    REQUIRE(p.isReady().strictEquals(false));
    data(c.state, "ready", nullptr);
    REQUIRE(p.isReady().isNull());
    data(c.state, "ready", true);
    REQUIRE(p.isReady().isNull());
    REQUIRE(c.state.get("readyQueries").number() == 2);
}
TEST_CASE("WebGLProgram escaped methods retain shared native source "
          "environment and release cycles",
          "[webgl-program]") {
    auto realm = JSValue::object();
    JSValue detached;
    JSWeakValue renderer, bindings, handle;
    {
        Commands c;
        WebGLProgram p(c.renderer, "key", defaults(), c.bindings);
        renderer = JSWeakValue(c.renderer);
        bindings = JSWeakValue(c.bindings);
        handle = JSWeakValue(p.object().get("program"));
        detached = p.object().get("isReady");
    }
    realm.collectGarbage();
    REQUIRE_FALSE(renderer.expired());
    REQUIRE_FALSE(bindings.expired());
    REQUIRE_FALSE(handle.expired());
    REQUIRE(detached.call().strictEquals(true));
    detached = {};
    realm.collectGarbage();
    REQUIRE(renderer.expired());
    REQUIRE(bindings.expired());
    REQUIRE(handle.expired());
}
TEST_CASE("WebGLProgram wrapper validates foreign values before copying "
          "managed handles",
          "[webgl-program]") {
    Commands c;
    WebGLProgram p(c.renderer, "key", defaults(), c.bindings);
    unsigned rejected = 0;
    std::thread other([&] {
        const auto attempt = [&](auto operation) {
            try {
                operation();
            } catch (const JSTypeError&) {
                ++rejected;
            }
        };
        attempt([&] { WebGLProgram invalid(c.renderer, {}, {}, {}); });
        attempt([&] { WebGLProgram invalid({}, c.state, {}, {}); });
        attempt([&] { WebGLProgram invalid({}, {}, c.state, {}); });
        attempt([&] { WebGLProgram invalid({}, {}, {}, c.state); });
        attempt([&] { (void)p.object(); });
        attempt([&] { (void)p.getUniforms(); });
        attempt([&] { (void)p.getAttributes(); });
        attempt([&] { (void)p.isReady(); });
        attempt([&] { (void)p.destroy(); });
        attempt([&] { (void)WebGLProgram::fromObject(c.state); });
    });
    other.join();
    REQUIRE(rejected == 10);
}
TEST_CASE("WebGLProgram module roots follow application heap lifetime and "
          "new heaps restart the source counter",
          "[webgl-program][gc]") {
    std::string failure;
    std::thread isolated([&] {
        try {
            for (unsigned iteration = 0; iteration < 8; ++iteration) {
                JSWeakValue constructor, program;
                {
                    auto realm = JSValue::object();
                    constructor = JSWeakValue(nativeWebGLProgramConstructor());
                    Commands c;
                    WebGLProgram p(c.renderer, "key", defaults(), c.bindings);
                    program = JSWeakValue(p.object());
                    if (p.object().get("id").number() != 0)
                        throw std::runtime_error("A new heap inherited the old module counter");
                    realm.collectGarbage();
                    if (constructor.expired() || program.expired())
                        throw std::runtime_error("Live application roots lost their module");
                }
                if (!constructor.expired() || !program.expired())
                    throw std::runtime_error("A module cache retained a dead heap");
            }
        } catch (const std::exception& e) {
            failure = e.what();
        }
    });
    isolated.join();
    INFO(failure);
    REQUIRE(failure.empty());
}
