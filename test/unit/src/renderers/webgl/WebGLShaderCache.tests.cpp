#include "catch.hpp"
#include "parity/WebGLShaderCacheFixture.h"
#include "core/NativeObject.h"
#include "materials/ShaderMaterial.h"
#include "materials/RawShaderMaterial.h"
#include <atomic>
#include <thread>
using namespace ShaderCacheFixture;

TEST_CASE("WebGLShaderCache shares code stages and module ids", "[webgl-shader-cache]") {
    WebGLShaderCache first,second;auto material=object({{"vertexShader","same"},{"fragmentShader","same"}});
    const auto vertex=first.getVertexShaderStage(material),fragment=first.getFragmentShaderStage(material);
    REQUIRE(vertex.sameValue(fragment));REQUIRE(vertex.get("code").string()=="same");REQUIRE(vertex.get("usedTimes").number()==0);
    const auto independent=second.getVertexShaderStage(material);
    REQUIRE_FALSE(vertex.sameValue(independent));REQUIRE(independent.get("id").number()==vertex.get("id").number()+1);
    REQUIRE(first.update(material,vertex,fragment).sameValue(first.object()));REQUIRE(vertex.get("usedTimes").number()==1);
    auto other=object();first.update(other,vertex,fragment);REQUIRE(vertex.get("usedTimes").number()==2);
    REQUIRE(first.remove(material).sameValue(first.object()));REQUIRE(vertex.get("usedTimes").number()==1);
    REQUIRE(first.object().get("shaderCache").get("size").number()==1);
    first.remove(other);REQUIRE(vertex.get("usedTimes").number()==0);
    REQUIRE(first.object().get("shaderCache").get("size").number()==0);
    REQUIRE_THROWS_AS(first.remove(material),JSTypeError);
    const auto replacement=first.getVertexShaderStage(material);REQUIRE_FALSE(vertex.sameValue(replacement));
    REQUIRE(replacement.get("id").number()==independent.get("id").number()+1);
    first.update(material,replacement,replacement);REQUIRE(first.dispose().isUndefined());
    REQUIRE(replacement.get("usedTimes").number()==1);REQUIRE(first.object().get("materialCache").get("size").number()==0);
    REQUIRE(first.object().get("shaderCache").get("size").number()==0);
    auto copied=first;REQUIRE(copied.object().sameValue(first.object()));auto moved=std::move(copied);
    REQUIRE(moved.object().sameValue(first.object()));REQUIRE(copied.object().isUndefined());
}
TEST_CASE("WebGLShaderCache reads authoritative native shader material strings", "[webgl-shader-cache]") {
    auto shader=std::make_shared<ShaderMaterial>();auto raw=std::make_shared<RawShaderMaterial>();
    shader->vertexShader="vertex-a";shader->fragmentShader="fragment-a";raw->vertexShader=shader->vertexShader;raw->fragmentShader=shader->fragmentShader;
    auto material=nativeMaterial(shader),rawMaterial=nativeMaterial(raw);WebGLShaderCache cache;
    const auto vertex=cache.getVertexShaderStage(material),fragment=cache.getFragmentShaderStage(material);
    REQUIRE(vertex.get("code").string()==shader->vertexShader);REQUIRE(fragment.get("code").string()==shader->fragmentShader);
    REQUIRE(cache.getVertexShaderStage(rawMaterial).sameValue(vertex));REQUIRE(cache.getFragmentShaderStage(rawMaterial).sameValue(fragment));
    cache.update(material,vertex,fragment);cache.update(rawMaterial,vertex,fragment);
    REQUIRE(vertex.get("usedTimes").number()==2);REQUIRE(fragment.get("usedTimes").number()==2);
    shader->vertexShader="vertex-b";const auto changed=cache.getVertexShaderStage(material);
    REQUIRE(changed.get("code").string()=="vertex-b");REQUIRE_FALSE(changed.sameValue(vertex));
    material.set("fragmentShader","fragment-b");REQUIRE(shader->fragmentShader=="fragment-b");
    const auto changedFragment=cache.getFragmentShaderStage(material);REQUIRE(changedFragment.get("code").string()=="fragment-b");
    REQUIRE(material.sameValue(nativeMaterial(shader)));
    cache.update(material,changed,changedFragment);REQUIRE(changed.get("usedTimes").number()==1);
    cache.remove(material);REQUIRE(vertex.get("usedTimes").number()==1);REQUIRE(fragment.get("usedTimes").number()==1);
    REQUIRE(changed.get("usedTimes").number()==0);REQUIRE(changedFragment.get("usedTimes").number()==0);
    cache.remove(rawMaterial);REQUIRE(vertex.get("usedTimes").number()==0);REQUIRE(fragment.get("usedTimes").number()==0);
    REQUIRE(cache.object().get("shaderCache").get("size").number()==0);
}
TEST_CASE("WebGLShaderCache native material ownership follows strong collection edges", "[webgl-shader-cache]") {
    const auto realm=JSValue::object();WebGLShaderCache cache;std::weak_ptr<ShaderMaterial> weakNative;JSWeakValue weakMaterial,weakStage;
    {
        auto native=std::make_shared<ShaderMaterial>();weakNative=native;auto material=nativeMaterial(native);
        const auto stage=cache.getVertexShaderStage(material);cache.update(material,stage,stage);weakMaterial=JSWeakValue(material);weakStage=JSWeakValue(stage);
    }
    realm.collectGarbage();REQUIRE_FALSE(weakNative.expired());REQUIRE_FALSE(weakMaterial.expired());REQUIRE_FALSE(weakStage.expired());
    {const auto material=weakMaterial.lock();REQUIRE(material.has_value());cache.remove(*material);}
    realm.collectGarbage();REQUIRE(weakMaterial.expired());REQUIRE(weakStage.expired());REQUIRE(weakNative.expired());
    {
        auto native=std::make_shared<ShaderMaterial>();weakNative=native;auto material=nativeMaterial(native),stage=cache.getVertexShaderStage(material);
        cache.update(material,stage,stage);weakMaterial=JSWeakValue(material);weakStage=JSWeakValue(stage);
    }
    cache.dispose();realm.collectGarbage();REQUIRE(weakMaterial.expired());REQUIRE(weakStage.expired());REQUIRE(weakNative.expired());
    JSWeakValue weakCycle;
    {
        WebGLShaderCache cycle;auto native=std::make_shared<ShaderMaterial>();weakNative=native;
        auto material=nativeMaterial(native),stage=cycle.getVertexShaderStage(material);native->extraProperties.set("cache",cycle.object());
        cycle.update(material,stage,stage);weakCycle=JSWeakValue(cycle.object());weakMaterial=JSWeakValue(material);
    }
    realm.collectGarbage();REQUIRE(weakCycle.expired());REQUIRE(weakMaterial.expired());REQUIRE(weakNative.expired());
}
TEST_CASE("WebGLShaderCache rejects cross-thread wrapper access before copying handles", "[webgl-shader-cache]") {
    WebGLShaderCache cache;auto material=object(),stage=cache._getShaderStage("code");std::atomic<unsigned> rejected=0;
    std::thread worker([&]{
        const auto check=[&](auto action){try{action();}catch(const JSTypeError&){++rejected;}};
        check([&]{cache.object();});check([&]{WebGLShaderCache copy(cache);});check([&]{WebGLShaderCache copy(std::move(cache));});
        check([&]{cache.update(material,stage,stage);});check([&]{cache.remove(material);});
        check([&]{cache.getVertexShaderStage(material);});check([&]{cache.getFragmentShaderStage(material);});
        check([&]{cache.dispose();});check([&]{cache._getShaderCacheForMaterial(material);});check([&]{cache._getShaderStage("code");});
        check([&]{WebGLShaderCache::fromObject(cache.object());});
    });worker.join();REQUIRE(rejected==11);REQUIRE(cache.object().isObject());REQUIRE(cache._getShaderStage("code").sameValue(stage));
}
