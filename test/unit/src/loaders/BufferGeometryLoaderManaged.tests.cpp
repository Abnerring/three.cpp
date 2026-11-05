#include "catch.hpp"
#include "loaders/BufferGeometryLoader.h"
#include "loaders/BufferGeometryLoaderSources.inc"
#include "loaders/FileLoader.h"
#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include "animation/AnimationUtils.h"
#include <cmath>
#include <thread>

namespace {
using Args=std::vector<JSValue>;
JSValue json(const char* text){return Animation_detail::parseJSON(text);}
JSValue call(const JSValue& object,const char* name,const Args& args={}){return object.get(name).call(object,args);}
void getter(JSValue object,const char* key,const JSValue& fn){JSPropertyDescriptor d;d.getter=fn;d.enumerable=d.configurable=true;object.defineProperty(key,d);}
struct Restore {JSValue object;const char* key;JSPropertyDescriptor descriptor;~Restore(){object.defineProperty(key,descriptor);}};
}

TEST_CASE("Managed BufferGeometryLoader has pinned class and method metadata", "[loaders][buffergeometryloader][managed]") {
    auto constructor=nativeBufferGeometryLoaderConstructor(),prototype=constructor.get("prototype");
    REQUIRE(call(constructor,"toString").strictEquals(BufferGeometryLoaderSource::classSource));
    REQUIRE(constructor.get("length").strictEquals(1));
    REQUIRE(constructor.getPrototype().sameValue(nativeLoaderConstructor()));
    REQUIRE(call(prototype.get("load"),"toString").strictEquals(BufferGeometryLoaderSource::loadSource));
    REQUIRE(call(prototype.get("parse"),"toString").strictEquals(BufferGeometryLoaderSource::parseSource));
    REQUIRE(prototype.get("load").get("length").strictEquals(4));REQUIRE(prototype.get("parse").get("length").strictEquals(1));
}

TEST_CASE("Managed BufferGeometryLoader keeps raw values original userData and writable geometry fields", "[loaders][buffergeometryloader][managed]") {
    BufferGeometryLoader loader;
    auto input=json(R"({"data":{"attributes":{"position":{"type":"Float32Array","array":[0,0,0,1,2,3],"itemSize":3,"normalized":false}},"index":{"type":"Uint16Array","array":[0,1]},"groups":[{"start":0,"count":2,"materialIndex":4}],"boundingSphere":{"center":[1,2,3],"radius":7}}})");
    auto user=JSValue::object();user.set("self",user);auto name=JSValue::object();input.set("userData",user);input.set("name",name);
    auto geometry=loader.parse(input);geometry.set("uuid","parsed");
    REQUIRE(geometry.get("uuid").strictEquals("parsed"));REQUIRE(geometry.get("userData").sameValue(user));REQUIRE(geometry.get("name").sameValue(name));
    REQUIRE(geometry.getPrototype().sameValue(nativeBufferGeometryConstructor().get("prototype")));
    auto position=geometry.get("attributes").get("position");REQUIRE(position.get("array").instanceOf(nativeGlobalBinding("Float32Array")));
    REQUIRE(position.get("count").strictEquals(2));REQUIRE(geometry.get("index").get("itemSize").strictEquals(1));
    REQUIRE(geometry.get("groups").get("0").get("materialIndex").strictEquals(4));
    REQUIRE(geometry.get("boundingSphere").get("radius").strictEquals(7));REQUIRE(geometry.get("boundingSphere").get("center").get("y").strictEquals(2));
}

TEST_CASE("Managed BufferGeometryLoader repeats data getter at source sites and respects inherited attributes", "[loaders][buffergeometryloader][managed]") {
    BufferGeometryLoader loader;auto input=json(R"({"data":{"attributes":{}}})");auto data=input.get("data");
    auto events=JSValue::array();getter(input,"data",JSValue::function([](const JSValue&,const Args&,const Args& c){call(c[0],"push",{"data"});return c[1];},{events,data}));
    auto user=JSValue::object();getter(input,"userData",JSValue::function([](const JSValue&,const Args&,const Args& c){call(c[0],"push",{"userData"});return c[1];},{events,user}));
    auto proto=json(R"({"inherited":{"type":"Float32Array","array":[1,2,3],"itemSize":3}})");data.get("attributes").setPrototype(proto);
    auto geometry=loader.parse(input);
    REQUIRE(geometry.get("attributes").hasOwn("inherited"));
    REQUIRE(events.length()==10);for(unsigned i=0;i<8;++i)REQUIRE(events.get(JSValue(i)).strictEquals("data"));
    REQUIRE(events.get("8").strictEquals("userData"));REQUIRE(events.get("9").strictEquals("userData"));
    REQUIRE(geometry.get("userData").sameValue(user));
}

TEST_CASE("Managed BufferGeometryLoader uses truthy legacy groups and ignores instanced flags on morph attributes", "[loaders][buffergeometryloader][managed]") {
    BufferGeometryLoader loader;
    auto input=json(R"({"isInstancedBufferGeometry":true,"data":{"attributes":{"instances":{"type":"Float32Array","array":[1,2,3],"itemSize":3,"isInstancedBufferAttribute":true,"meshPerAttribute":7}},"groups":null,"drawcalls":[{"start":"0","count":"3"}],"morphAttributes":{"position":[{"type":"Float32Array","array":[1,2,3],"itemSize":3,"isInstancedBufferAttribute":true}]},"morphTargetsRelative":1}})");
    auto geometry=loader.parse(input);
    REQUIRE(geometry.get("isInstancedBufferGeometry").strictEquals(true));REQUIRE(std::isinf(geometry.get("instanceCount").number()));
    REQUIRE(geometry.get("attributes").get("instances").get("isInstancedBufferAttribute").strictEquals(true));
    REQUIRE(geometry.get("attributes").get("instances").get("meshPerAttribute").strictEquals(1));
    REQUIRE(geometry.get("morphAttributes").get("position").get("0").get("isInstancedBufferAttribute").isUndefined());
    REQUIRE(geometry.get("groups").get("0").get("start").strictEquals("0"));REQUIRE(geometry.get("groups").get("0").get("count").strictEquals("3"));
    REQUIRE(geometry.get("morphTargetsRelative").strictEquals(true));
    auto clone=call(geometry,"clone");REQUIRE(clone.get("isInstancedBufferGeometry").strictEquals(true));
    REQUIRE(clone.get("instanceCount").sameValue(geometry.get("instanceCount")));
}

TEST_CASE("Managed BufferGeometryLoader shares interleaved and array buffers across reentrant parsing", "[loaders][buffergeometryloader][managed]") {
    BufferGeometryLoader loader;
    auto input=json(R"({"data":{"arrayBuffers":{"words":[1065353216,1073741824,1077936128]},"interleavedBuffers":{"ib":{"uuid":"ib","buffer":"words","type":"Float32Array","stride":3,"usage":35048}},"attributes":{"position":{"isInterleavedBufferAttribute":true,"data":"ib","itemSize":3,"offset":0},"normal":{"isInterleavedBufferAttribute":true,"data":"ib","itemSize":3,"offset":0}}}})");
    auto nested=json(R"({"data":{"arrayBuffers":{"words":[1091567616,1092616192,1093664768]},"interleavedBuffers":{"ib":{"uuid":"ib","buffer":"words","type":"Float32Array","stride":3}},"attributes":{"position":{"isInterleavedBufferAttribute":true,"data":"ib","itemSize":3,"offset":0}}}})");
    auto events=JSValue::array();auto normal=input.get("data").get("attributes").get("normal");
    getter(normal,"data",JSValue::function([](const JSValue&,const Args&,const Args& c){auto geometry=call(c[0],"parse",{c[1]});call(c[2],"push",{geometry.get("attributes").get("position").get("data").get("array").get("0")});return JSValue("ib");},{loader.object(),nested,events}));
    auto geometry=loader.parse(input),attributes=geometry.get("attributes");auto first=attributes.get("position").get("data"),second=attributes.get("normal").get("data");
    REQUIRE(first.sameValue(second));REQUIRE(first.get("array").get("0").strictEquals(1));REQUIRE(first.get("array").get("2").strictEquals(3));
    REQUIRE(first.get("usage").strictEquals(35048));REQUIRE(events.get("0").strictEquals(9));
    REQUIRE(first.get("array").get("buffer").sameValue(second.get("array").get("buffer")));
}

TEST_CASE("Managed BufferGeometryLoader load closures retain independent requests and catch callback errors", "[loaders][buffergeometryloader][managed]") {
    auto filePrototype=nativeFileLoaderConstructor().get("prototype");Restore restore{filePrototype,"load",*filePrototype.getOwnPropertyDescriptor("load")};
    auto queue=JSValue::array(),events=JSValue::array();
    auto replacement=JSValue::function([](const JSValue&,const Args& a,const Args& c){call(c[0],"push",{a[1]});return JSValue(99);},{queue});
    filePrototype.set("load",replacement);
    auto manager=JSValue::object();manager.set("itemError",JSValue::function([](const JSValue&,const Args& a,const Args& c){call(c[0],"push",{a[0]});return JSValue{};},{events}));
    BufferGeometryLoader loader(manager);auto seen=JSValue::array();
    const auto callback=[&](const char* label){return JSValue::function([](const JSValue& self,const Args& a,const Args& c){call(c[0],"push",{c[1],a[0].get("name"),self.isUndefined()});return JSValue{};},{seen,label});};
    REQUIRE(loader.load("a",callback("a")).isUndefined());REQUIRE(loader.load("b",callback("b")).isUndefined());
    queue.get("1").call({}, {"{\"name\":\"second\",\"data\":{\"attributes\":{}}}"});
    queue.get("0").call({}, {"{\"name\":\"first\",\"data\":{\"attributes\":{}}}"});
    REQUIRE(seen.get("0").strictEquals("b"));REQUIRE(seen.get("1").strictEquals("second"));REQUIRE(seen.get("2").strictEquals(true));
    REQUIRE(seen.get("3").strictEquals("a"));REQUIRE(seen.get("4").strictEquals("first"));
    auto reason=JSValue::object();auto onLoad=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{throw JSThrownValue(c[0]);},{reason});
    auto onError=JSValue::function([](const JSValue&,const Args& a,const Args& c){call(c[0],"push",{a[0].sameValue(c[1])});return JSValue{};},{seen,reason});
    loader.load("c",onLoad,{},onError);queue.get("2").call({}, {"{\"data\":{\"attributes\":{}}}"});
    REQUIRE(seen.get("6").strictEquals(true));REQUIRE(events.get("0").strictEquals("c"));
}
