#include "catch.hpp"
#include "parity/WebGLMaterialsFixture.h"
#include <thread>
using namespace MaterialsFixture;
TEST_CASE("WebGLMaterials writes physical native uniforms and texture transforms",
          "[webgl-materials]") {
    auto m = std::make_shared<MeshPhysicalMaterial>();
    m->color.setRGB(0.25, 0.5, 0.75);
    m->normalScale.set(2, 3);
    m->clearcoatNormalScale.set(4, 5);
    m->side = BackSide;
    m->map = std::make_shared<Texture>();
    m->map->offset.set(0.25, 0.5);
    m->normalMap = m->map;
    m->clearcoatNormalMap = m->map;
    m->clearcoat = 1;
    m->transmission = 0.5;
    auto uniforms = nativeUniforms(), material = nativeMaterial(m),
         properties = WebGLProperties().object();
    WebGLMaterials manager({}, properties);
    auto target = object({{"texture", nativeTexture(m->map)}, {"width", 512}, {"height", 256}});
    REQUIRE(manager.refreshMaterialUniforms(uniforms, material, 1, 100, target).isUndefined());
    auto diffuse = uniforms.get("diffuse").get("value"),
         normal = uniforms.get("normalScale").get("value"),
         clearcoat = uniforms.get("clearcoatNormalScale").get("value");
    REQUIRE(diffuse.get("r").number() == 0.25);
    REQUIRE(normal.get("x").number() == -2);
    REQUIRE(normal.get("y").number() == -3);
    REQUIRE(clearcoat.get("x").number() == -4);
    REQUIRE(clearcoat.get("y").number() == -5);
    REQUIRE(uniforms.get("map").get("value").sameValue(nativeTexture(m->map)));
    REQUIRE(uniforms.get("mapTransform").get("value").get("elements").get("6").number() == 0.25);
    REQUIRE(uniforms.get("transmissionSamplerSize").get("value").get("x").number() == 512);
    REQUIRE(uniforms.get("transmissionSamplerMap").get("value").sameValue(nativeTexture(m->map)));
    REQUIRE(m->normalScale.x == 2);
    REQUIRE(m->clearcoatNormalScale.x == 4);
}
TEST_CASE("WebGLMaterials preserves earlier uniform writes when a later callback throws",
          "[webgl-materials]") {
    auto uniforms = nativeUniforms(),
         material = object({{"isMeshBasicMaterial", true}, {"opacity", 0.25}, {"color", object()}});
    auto diffuse = uniforms.get("diffuse").get("value");
    prop(diffuse, "copy",
         JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue {
             throw JSRangeError("copy");
         }));
    WebGLMaterials manager({}, WebGLProperties().object());
    REQUIRE_THROWS_AS(manager.refreshMaterialUniforms(uniforms, material), JSRangeError);
    REQUIRE(uniforms.get("opacity").get("value").number() == 0.25);
    REQUIRE(uniforms.get("roughness").get("value").number() == -99);
}
TEST_CASE("WebGLMaterials detached closures share constructor parameter lifetime",
          "[webgl-materials][gc]") {
    auto anchor = JSValue::object();
    for (unsigned method = 0; method < 2; ++method)
        for (bool throughInstance : {false, true}) {
            JSValue held;
            JSWeakValue rw, pw, mw, fw;
            {
                auto renderer = object(), properties = object(),
                     manager = WebGLMaterials(renderer, properties).object();
                auto fn = manager.get(method ? "refreshMaterialUniforms" : "refreshFogUniforms");
                rw = JSWeakValue(renderer);
                pw = JSWeakValue(properties);
                mw = JSWeakValue(manager);
                fw = JSWeakValue(fn);
                if (throughInstance) {
                    auto instance = JSValue::object(fn.get("prototype"));
                    held = instance;
                } else
                    held = fn;
            }
            anchor.collectGarbage();
            REQUIRE_FALSE(rw.expired());
            REQUIRE_FALSE(pw.expired());
            REQUIRE_FALSE(fw.expired());
            REQUIRE(mw.expired());
            held = {};
            anchor.collectGarbage();
            REQUIRE(rw.expired());
            REQUIRE(pw.expired());
            REQUIRE(fw.expired());
        }
}
TEST_CASE("WebGLMaterials releases managed cycles from captured native texture values",
          "[webgl-materials][gc]") {
    auto anchor = object();
    JSWeakValue rendererWeak, propertiesWeak, managerWeak, textureWeak;
    std::weak_ptr<Texture> nativeWeak;
    {
        auto texture = std::make_shared<Texture>();
        nativeWeak = texture;
        auto wrapped = nativeTexture(texture), renderer = object({{"texture", wrapped}}),
             properties = object();
        auto manager = WebGLMaterials(renderer, properties).object();
        prop(wrapped, "manager", manager);
        prop(properties, "renderer", renderer);
        rendererWeak = JSWeakValue(renderer);
        propertiesWeak = JSWeakValue(properties);
        managerWeak = JSWeakValue(manager);
        textureWeak = JSWeakValue(wrapped);
    }
    anchor.collectGarbage();
    REQUIRE(rendererWeak.expired());
    REQUIRE(propertiesWeak.expired());
    REQUIRE(managerWeak.expired());
    REQUIRE(textureWeak.expired());
    REQUIRE(nativeWeak.expired());
}
TEST_CASE("WebGLMaterials managers share the dynamic matrix scratch", "[webgl-materials]") {
    nativeWebGLMaterialsConstructor();
    auto prototype = nativeMatrix4Prototype();
    const auto original = *prototype.getOwnPropertyDescriptor("makeRotationFromEuler");
    auto state = object({{"calls", 0}, {"same", true}});
    prop(prototype, "makeRotationFromEuler",
         JSValue::function(
             [](const JSValue& self, const Args& args, const Args& c) {
                 auto s = c[0];
                 const auto calls = s.get("calls").number();
                 if (calls > 0)
                     s.set("same", s.get("same").toBoolean() && s.get("first").sameValue(self));
                 else
                     s.set("first", self);
                 s.set("calls", calls + 1);
                 return c[1].call(self, args);
             },
             {state, *original.value}));
    bool succeeded = false;
    try {
        auto uniforms = nativeUniforms(),
             material = object({{"isMeshBasicMaterial", true}, {"opacity", 1}}),
             properties = WebGLProperties().object();
        auto cache = invoke(properties, "get", {material});
        prop(cache, "envMap", object());
        prop(cache, "envMapRotation", nativeEuler(std::make_shared<Euler>()));
        WebGLMaterials first({}, properties), second({}, properties);
        first.refreshMaterialUniforms(uniforms, material);
        second.refreshMaterialUniforms(uniforms, material);
        succeeded = true;
    } catch (...) {
        prototype.defineProperty("makeRotationFromEuler", original);
        throw;
    }
    prototype.defineProperty("makeRotationFromEuler", original);
    REQUIRE(succeeded);
    REQUIRE(state.get("calls").number() == 2);
    REQUIRE(state.get("same").toBoolean());
}
TEST_CASE("WebGLMaterials validates all wrapper entries before foreign heap copies",
          "[webgl-materials][threads]") {
    auto foreign = object();
    WebGLMaterials owner;
    unsigned rejected = 0, accepted = 0;
    std::thread worker([&] {
        const auto reject = [&](auto action) {
            try {
                action();
                ++accepted;
            } catch (const std::logic_error&) {
                ++rejected;
            }
        };
        auto localValue = object();
        WebGLMaterials local;
        reject([&] { WebGLMaterials bad(foreign); });
        reject([&] { WebGLMaterials bad(localValue, foreign); });
        reject([&] { static_cast<void>(WebGLMaterials::fromObject(foreign)); });
        reject([&] { static_cast<void>(owner.object()); });
        reject([&] { static_cast<void>(static_cast<JSValue>(owner)); });
        reject([&] { WebGLMaterials copy(owner); });
        reject([&] { WebGLMaterials moved(std::move(owner)); });
        reject([&] { local = owner; });
        reject([&] { local = std::move(owner); });
        reject([&] { owner = local; });
        reject([&] { owner = std::move(local); });
        reject([&] { owner.refreshFogUniforms(); });
        reject([&] { owner.refreshMaterialUniforms(); });
        reject([&] { local.refreshFogUniforms(foreign); });
        reject([&] { local.refreshFogUniforms(localValue, foreign); });
        reject([&] { local.refreshMaterialUniforms(foreign); });
        reject([&] { local.refreshMaterialUniforms(localValue, foreign); });
        reject([&] { local.refreshMaterialUniforms(localValue, localValue, foreign); });
        reject([&] { local.refreshMaterialUniforms(localValue, localValue, localValue, foreign); });
        reject([&] {
            local.refreshMaterialUniforms(localValue, localValue, localValue, localValue, foreign);
        });
    });
    worker.join();
    REQUIRE(accepted == 0);
    REQUIRE(rejected == 20);
    REQUIRE(foreign.keys().empty());
    REQUIRE(owner.object().get("refreshMaterialUniforms").isCallable());
}
