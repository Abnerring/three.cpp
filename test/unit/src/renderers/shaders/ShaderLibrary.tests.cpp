#include "catch.hpp"
#include "parity/ShaderLibraryFixture.h"
#include "renderers/webgl/WebGLShader.h"
#include <exception>
#include <thread>

namespace {
// These fixtures require module initialization and final heap teardown. A fresh
// worker avoids owning TLS and cached exports installed by earlier unit cases.
template <class Body> void inFreshShaderRealm(Body body) {
    std::exception_ptr error;
    std::thread worker([&] {
        try {
            body();
        } catch (...) {
            error = std::current_exception();
        }
    });
    worker.join();
    if (error)
        std::rethrow_exception(error);
}
}
TEST_CASE("Shader exports share one realm and release it after the last owning value",
          "[shader-library]") {
    inFreshShaderRealm([] {
        JSWeakValue chunks, library, uniforms, colour, payloadWeak;
        std::weak_ptr<int> native;
        const auto facadeCopy = UniformsLib;
        {
            auto realm = JSValue::object();
            JSValue u = UniformsLib, c = ShaderChunk, l = ShaderLib;
            REQUIRE(facadeCopy.object().sameValue(u));
            REQUIRE(ShaderChunk.object().sameValue(c));
            REQUIRE(ShaderLib.object().sameValue(l));
            auto standard = l.get("standard").get("uniforms"),
                 physical = l.get("physical").get("uniforms");
            REQUIRE_FALSE(standard.sameValue(physical));
            REQUIRE_FALSE(
                standard.get("diffuse").get("value").sameValue(physical.get("diffuse").get("value")));
            auto owner = std::make_shared<int>(42);
            native = owner;
            auto p = JSValue::hostObject(owner, owner.get(), "audit.payload");
            owner.reset();
            u.set("marker", p);
            p.set("lib", l);
            chunks = JSWeakValue(c);
            library = JSWeakValue(l);
            uniforms = JSWeakValue(u);
            colour = JSWeakValue(u.get("common").get("diffuse").get("value"));
            payloadWeak = JSWeakValue(p);
            u = {};
            c = {};
            l = {};
            p = {};
            standard = {};
            physical = {};
            realm.collectGarbage();
            REQUIRE_FALSE(native.expired());
            REQUIRE_FALSE(library.expired());
            REQUIRE(UniformsLib.get("marker").isHostObject("audit.payload"));
        }
        REQUIRE(chunks.expired());
        REQUIRE(library.expired());
        REQUIRE(uniforms.expired());
        REQUIRE(colour.expired());
        REQUIRE(payloadWeak.expired());
        REQUIRE(native.expired());
        JSValue fresh = facadeCopy;
        REQUIRE_FALSE(fresh.has("marker"));
        REQUIRE(fresh.keys().size() == 16);
        REQUIRE(ShaderLib.ownPropertyNames().size() == 18);
    });
}
TEST_CASE("Shader module literals create own fields despite inherited setters",
          "[shader-library]") {
    inFreshShaderRealm([] {
        auto realm = JSValue::object(), prototype = realm.getPrototype();
        auto hits = arr();
        for (const auto* k :
             {"common", "diffuse", "value", "basic", "uniforms", "vertexShader", "fragmentShader"})
            accessor(prototype, k, {},
                     JSValue::function(
                         [](const JSValue&, const Args&, const Args& c) -> JSValue {
                             literal(c[0], std::to_string(c[0].length()), c[1]);
                             return {};
                         },
                         {hits, k}));
        for (const auto* k :
             {"common", "diffuse", "value", "basic", "uniforms", "vertexShader", "fragmentShader"}) {
            JSPropertyDescriptor d;
            d.enumerable = false;
            prototype.defineProperty(k, d);
        }
        auto lib = nativeUniformsLib();
        auto chunk = nativeShaderChunk();
        REQUIRE(hits.length() == 0);
        for (const auto* k : {"common", "diffuse", "value"})
            prototype.erase(k);
        auto shader = nativeShaderLib();
        for (const auto* k :
             {"common", "diffuse", "value", "basic", "uniforms", "vertexShader", "fragmentShader"})
            prototype.erase(k);
        REQUIRE(hits.length() == 0);
        REQUIRE(lib.hasOwn("common"));
        REQUIRE(shader.hasOwn("basic"));
        REQUIRE(chunk.hasOwn("meshbasic_vert"));
    });
}
TEST_CASE("Shader module reentry fails explicitly and restores initialization for retry",
          "[shader-library]") {
    inFreshShaderRealm([] {
        auto u = nativeUniformsLib();
        const auto common = *u.getOwnPropertyDescriptor("common");
        accessor(u, "common",
                 JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue {
                     return nativeShaderLib();
                 }));
        REQUIRE_THROWS_AS(nativeShaderLib(), JSModuleInitializationError);
        u.defineProperty("common", common);
        auto result = nativeShaderLib();
        REQUIRE(result.keys().size() == 18);
        REQUIRE(nativeShaderLib().sameValue(result));
    });
}
TEST_CASE("Uniform cloning captures assignment base before observable RHS", "[shader-library]") {
    for (int mode : {0, 2}) {
        const auto result = cloneAssignmentCase(mode);
        REQUIRE(result.at("trace") == JsonValue(JsonValue::Array{"set-u", "get-u", "rhs"}));
        REQUIRE(result.at("first").at("fields").size() == 1);
        REQUIRE(result.at("next").at("fields").size() == 0);
    }
    const auto array = cloneAssignmentCase(1);
    REQUIRE(array.at("trace") == JsonValue(JsonValue::Array{"set-u", "rhs", "get-u"}));
    const auto lhsThrow = cloneAssignmentCase(3);
    REQUIRE(lhsThrow.at("error").get<std::string>() == "RangeError");
    REQUIRE(lhsThrow.at("trace") == JsonValue(JsonValue::Array{"set-u", "get-u"}));
}
TEST_CASE("Shader resolver and creation wrappers reject foreign values before copying",
          "[shader-library]") {
    auto foreign = JSValue::object();
    bool rejected[8] = {};
    std::thread worker([&] {
        auto reject = [&](size_t n, auto action) {
            try {
                action();
            } catch (const std::logic_error&) {
                rejected[n] = true;
            }
        };
        reject(0, [&] { UniformsLib.set("foreign", foreign); });
        reject(1, [&] { ShaderLib.setPrototype(foreign); });
        reject(2, [&] { ShaderChunk.get("x", foreign); });
        reject(3, [&] { ShaderLib.sameValue(foreign); });
        reject(4, [&] { ShaderChunk.reflectSet("x", 3, foreign); });
        reject(5, [&] { WebGLShader(foreign, 1, "x"); });
        reject(6, [&] { WebGLShader({}, foreign, "x"); });
        reject(7, [&] { WebGLShader({}, 1, foreign); });
    });
    worker.join();
    for (bool r : rejected)
        REQUIRE(r);
}
#include "core/NativeObject.h"
#include "materials/MeshPhysicalMaterial.h"
#include "renderers/webgl/WebGLMaterials.h"
#include "renderers/webgl/WebGLProperties.h"
TEST_CASE("ShaderLib uniforms feed the actual native material refresh protocol",
          "[shader-library]") {
    auto lib = nativeShaderLib();
    auto uniforms = cloneUniforms(lib.get("physical").get("uniforms"));
    auto material = std::make_shared<MeshPhysicalMaterial>();
    material->color.setRGB(.3, .4, .5);
    material->roughness = .25;
    material->metalness = .6;
    material->clearcoat = .5;
    auto properties = WebGLProperties().object();
    WebGLMaterials refresh({}, properties);
    refresh.refreshMaterialUniforms(uniforms, nativeMaterial(material), 1, 100, {});
    REQUIRE(uniforms.get("roughness").get("value").number() == .25);
    REQUIRE(uniforms.get("metalness").get("value").number() == .6);
    REQUIRE(uniforms.get("clearcoat").get("value").number() == .5);
    REQUIRE(uniforms.get("diffuse").get("value").get("r").number() == .3);
    REQUIRE(lib.get("physical").get("uniforms").get("diffuse").get("value").get("r").number() == 1);
    REQUIRE(lib.get("physical").get("uniforms").get("roughness").get("value").number() == 1);
}
