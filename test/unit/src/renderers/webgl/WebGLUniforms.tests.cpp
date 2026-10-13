#include "parity/WebGLUniformsFixture.h"
#include "catch.hpp"
#include "core/NativeObject.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "textures/DepthTexture.h"
#include "textures/Texture.h"
#include <thread>
using namespace UniformsFixture;
namespace {
unsigned commands(Fixture& f, const char* name) {
    unsigned n = 0;
    auto trace = f.state.get("log");
    for (unsigned i = 0; i < trace.length(); ++i)
        if (trace.get(std::to_string(i)).get("0").strictEquals(name))
            ++n;
    return n;
}
} // namespace
TEST_CASE("WebGLUniforms scalar caches use strict equality and commit after commands",
          "[renderers][webgl][uniforms]") {
    Fixture f;
    f.set(-0.);
    f.set(0.);
    REQUIRE(commands(f, "uniform1f") == 1);
    f.set(std::nan(""));
    f.set(std::nan(""));
    REQUIRE(commands(f, "uniform1f") == 3);
    f.state.set("throw", "uniform1f");
    REQUIRE_THROWS_AS(f.set(7), JSRangeError);
    REQUIRE(std::isnan(f.leaf().get("cache").get("0").toNumber()));
    f.state.set("throw", "");
    f.set(7);
    REQUIRE(f.leaf().get("cache").get("0").strictEquals(7));
}
TEST_CASE("WebGLUniforms exposes proper class and ordinary setter functions",
          "[renderers][webgl][uniforms]") {
    Fixture f;
    auto constructor = f.ctor(), leaf = f.leaf(), setter = leaf.get("setValue");
    REQUIRE(constructor.get("name").strictEquals("WebGLUniforms"));
    REQUIRE(constructor.get("length").strictEquals(2));
    REQUIRE(constructor.isConstructible());
    REQUIRE_THROWS_AS(constructor.call(), JSTypeError);
    REQUIRE_FALSE(constructor.get("upload").isConstructible());
    REQUIRE_FALSE(f.manager.get("setValue").isConstructible());
    REQUIRE(setter.isConstructible());
    REQUIRE(setter.get("name").strictEquals("setValueV1f"));
    REQUIRE(setter.get("length").strictEquals(2));
    REQUIRE(setter.hasOwn("prototype"));
    REQUIRE(leaf.getPrototype().get("constructor").get("name").strictEquals("SingleUniform"));
}
TEST_CASE("WebGLUniforms accepts actual native vector matrix and typed buffers",
          "[renderers][webgl][uniforms]") {
    Fixture vectorCase({info("u", 0x8b51)});
    auto vector = std::make_shared<Vector3>(1, 2, 3);
    vectorCase.set(nativeVector(vector));
    vectorCase.set(nativeVector(vector));
    REQUIRE(commands(vectorCase, "uniform3f") == 1);
    vector->y = 4;
    vectorCase.set(nativeVector(vector));
    REQUIRE(commands(vectorCase, "uniform3f") == 2);
    REQUIRE(vectorCase.leaf().get("cache").get("1").strictEquals(4));
    Fixture matrixCase({info("u", 0x8b5c)});
    auto matrix = std::make_shared<Matrix4>();
    matrixCase.set(nativeMatrix(matrix));
    matrixCase.set(nativeMatrix(matrix));
    REQUIRE(commands(matrixCase, "uniformMatrix4fv") == 1);
    auto calls = matrixCase.state.get("log");
    auto row = calls.get(std::to_string(calls.length() - 1));
    REQUIRE(row.get("4").get("typed").strictEquals("Float32Array"));
    matrix->elements()[0] = 1.1;
    matrixCase.set(nativeMatrix(matrix));
    REQUIRE(matrixCase.leaf().get("cache").get("0").strictEquals(1.1));
}
TEST_CASE("WebGLUniforms flat arrays and flatten scratch preserve source cache behavior",
          "[renderers][webgl][uniforms]") {
    Fixture f({info("u[0]", 0x8b51, 2)});
    auto a = std::make_shared<Vector3>(1, 2, 3), b = std::make_shared<Vector3>(4, 5, 6);
    auto input = array({nativeVector(a), nativeVector(b)});
    f.set(input);
    f.set(input);
    REQUIRE(commands(f, "uniform3fv") == 2);
    REQUIRE(f.leaf().get("cache").length() == 0);
    auto rows = f.state.get("log"), last = rows.get(std::to_string(rows.length() - 1));
    REQUIRE(last.get("3").get("typed").strictEquals("Float32Array"));
    REQUIRE(last.get("3").get("values").get("5").strictEquals(6));
}
TEST_CASE(
    "WebGLUniforms static filtering includes inherited properties and upload skips only false",
    "[renderers][webgl][uniforms]") {
    Fixture f({info("a"), info("b"), info("c")});
    auto prototype = JSValue::object(), values = JSValue::object(prototype);
    auto a = JSValue::object(), b = JSValue::object(), c = JSValue::object();
    prop(a, "value", 1);
    prop(a, "needsUpdate", false);
    prop(b, "value", 2);
    prop(b, "needsUpdate", 0);
    prop(c, "value", 3);
    prop(values, "a", a);
    prop(prototype, "b", b);
    prop(values, "c", c);
    auto seq = WebGLUniforms::seqWithValue(f.manager.get("seq"), values);
    REQUIRE(seq.length() == 3);
    WebGLUniforms::upload(f.gl, seq, values, f.textures);
    REQUIRE(commands(f, "uniform1f") == 2);
    REQUIRE(f.manager.get("map").get("a").get("cache").length() == 0);
}
TEST_CASE("WebGLUniforms managed graph releases callback cycles without losing shared setters",
          "[renderers][webgl][uniforms][gc]") {
    auto anchor = JSValue::object();
    JSWeakValue manager, seq, leaf, gl, fn;
    {
        Fixture f;
        manager = JSWeakValue(f.manager);
        seq = JSWeakValue(f.manager.get("seq"));
        leaf = JSWeakValue(f.leaf());
        gl = JSWeakValue(f.gl);
        fn = JSWeakValue(f.leaf().get("setValue"));
        f.manager.set("gl", f.gl);
        anchor.collectGarbage();
        REQUIRE_FALSE(manager.expired());
    }
    anchor.collectGarbage();
    REQUIRE(manager.expired());
    REQUIRE(seq.expired());
    REQUIRE(leaf.expired());
    REQUIRE(gl.expired());
    REQUIRE_FALSE(fn.expired());
}
TEST_CASE("WebGLUniforms fallback textures and setter state survive same heap collection",
          "[renderers][webgl][uniforms][gc]") {
    auto anchor = JSValue::object();
    JSWeakValue textureWitness, setterWitness, markerWitness;
    {
        Fixture f({info("u", 0x8b62)});
        prop(f.textures, "setTexture2D",
             JSValue::function(
                 +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
                     auto state = captures[0];
                     state.set("texture", args[0]);
                     return {};
                 },
                 {f.state}));
        f.set(nullptr);
        auto texture = f.state.get("texture"), setter = f.leaf().get("setValue"),
             marker = JSValue::object();
        REQUIRE(texture.get("isDepthTexture").strictEquals(true));
        REQUIRE(texture.get("version").strictEquals(1));
        texture.set("uniformsMarker", marker);
        setter.set("uniformsMarker", marker);
        textureWitness = JSWeakValue(texture);
        setterWitness = JSWeakValue(setter);
        markerWitness = JSWeakValue(marker);
    }
    anchor.collectGarbage();
    REQUIRE_FALSE(textureWitness.expired());
    REQUIRE_FALSE(setterWitness.expired());
    REQUIRE_FALSE(markerWitness.expired());
    auto texture = *textureWitness.lock(), setter = *setterWitness.lock();
    REQUIRE(texture.get("uniformsMarker").sameValue(setter.get("uniformsMarker")));
    texture.erase("uniformsMarker");
    setter.erase("uniformsMarker");
    anchor.collectGarbage();
    REQUIRE(markerWitness.expired());
}
TEST_CASE("WebGLUniforms C++ entry points reject foreign handles before copying",
          "[renderers][webgl][uniforms][thread]") {
    Fixture f;
    auto wrapper = WebGLUniforms::fromObject(f.manager);
    unsigned rejected = 0;
    std::thread worker([&] {
        auto check = [&](auto action) {
            try {
                action();
            } catch (const JSTypeError&) {
                ++rejected;
            }
        };
        check([&] { wrapper.object(); });
        check([&] { WebGLUniforms foreign(f.gl, f.program); });
        check([&] { WebGLUniforms::fromObject(f.manager); });
        check([&] { WebGLUniforms copied(wrapper); });
        check([&] { WebGLUniforms moved(std::move(wrapper)); });
        check([&] { wrapper.setValue(); });
        check([&] { WebGLUniforms::upload(f.gl); });
        check([&] { WebGLUniforms::seqWithValue(f.manager); });
    });
    worker.join();
    REQUIRE(rejected == 8);
    REQUIRE(wrapper.object().sameValue(f.manager));
}
TEST_CASE("WebGLUniforms texture adapter preserves native shadow state and rejects incompatible "
          "receivers",
          "[renderers][webgl][uniforms][native]") {
    auto depth = std::make_shared<DepthTexture>(1, 1);
    auto plain = std::make_shared<Texture>();
    auto wrapper = nativeTexture(depth), other = nativeTexture(plain);
    wrapper.set("compareFunction", 519);
    REQUIRE(depth->compareFunction == 519);
    depth->compareFunction = 512;
    REQUIRE(wrapper.get("compareFunction").strictEquals(512));
    wrapper.set("compareFunction", nullptr);
    REQUIRE_FALSE(depth->compareFunction.has_value());
    const auto descriptor = wrapper.getOwnPropertyDescriptor("compareFunction");
    REQUIRE_THROWS_AS(descriptor->getter->call(other), JSTypeError);
    REQUIRE_THROWS_AS(descriptor->setter->call(other, {519}), JSTypeError);
}
