#include "catch.hpp"
#include "loaders/LoadingManager.h"
#include "loaders/Cache.h"
#include "loaders/Loader.h"
#include "loaders/AnimationLoader.h"
#include "loaders/BufferGeometryLoader.h"
#include "loaders/MaterialLoader.h"
#include "core/AbortController.h"
#include "core/JSJobs.h"
#include "core/JSPromise.h"
#include "core/InstancedBufferGeometry.h"
#include "core/InterleavedBufferAttribute.h"
#include "core/InstancedBufferAttribute.h"
#include "animation/AnimationUtils.h"

namespace {
using Args=std::vector<JSValue>;
JSValue call(JSValue self,const char* name,Args args={}){return self.get(name).call(self,args);}
JSValue list(std::initializer_list<JSValue> values){auto result=JSValue::array();for(const auto& value:values)call(result,"push",{value});return result;}
}
TEST_CASE("LoadingManager callbacks preserve closure identity reentry and thrown state", "[loaders]") {
    LoadingManager manager;auto state=JSValue::object();state.set("calls",0);const auto scope=manager.object();
    manager.onStart=JSValue::function([](const JSValue& self,const Args& args,const Args& captures)->JSValue {
        auto state=captures[0];state.set("calls",state.get("calls").toNumber()+1);state.set("receiver",self.sameValue(captures[1]));state.set("total",args[2]);throw JSTypeError("start failed");
    },{state,scope});
    REQUIRE_THROWS_AS(manager.itemStart("a"),JSTypeError);REQUIRE_THROWS_AS(manager.itemStart("b"),JSTypeError);
    REQUIRE(state.get("calls").toNumber()==2);REQUIRE(state.get("total").toNumber()==2);REQUIRE(state.get("receiver").toBoolean());
    manager.onStart=JSValue{};manager.itemStart("c");auto foreign=JSValue::object();
    REQUIRE(scope.get("setURLModifier").call(foreign,{JSValue::function([](const JSValue& self,const Args& args,const Args&)->JSValue {REQUIRE(self.isUndefined());return args[0];})}).sameValue(foreign));
    REQUIRE(manager.resolveURL("e\xcc\x81").toString()=="\xc3\xa9");
    manager.onProgress=nullptr;REQUIRE_THROWS_AS(manager.itemEnd("a"),JSTypeError);
}
TEST_CASE("Cache keys and enabled use JS strict boolean and object identity", "[loaders]") {
    auto payload=JSValue::object();Cache.clear();Cache.enabled=false;Cache.add("file:a",payload);REQUIRE(Cache.get("file:a").isUndefined());
    Cache.enabled=0;Cache.add("file:a",payload);REQUIRE(Cache.get("file:a").sameValue(payload));Cache.add("file:blob:a",payload);REQUIRE(Cache.get("file:blob:a").isUndefined());
    auto oldFiles=Cache.files.value();Cache.clear();REQUIRE_FALSE(Cache.files.value().sameValue(oldFiles));REQUIRE(oldFiles.get("file:a").sameValue(payload));Cache.enabled=false;
}
TEST_CASE("Native abort algorithms precede events and composed signals share reasons", "[loaders][abort]") {
    auto first=createNativeAbortController(),second=createNativeAbortController();auto signal=first.get("signal");auto combined=createNativeAbortSignalAny(list({signal,second.get("signal")}));auto trace=JSValue::array(),reason=JSValue::object();
    const auto event=JSValue::function([](const JSValue& self,const Args& args,const Args& captures)->JSValue {
        REQUIRE(captures[1].get("aborted").toBoolean());call(captures[0],"push",{"event"});call(args[0],"stopImmediatePropagation");return {};
    },{trace,combined});
    call(signal,"addEventListener",{"abort",event});
    addNativeAbortAlgorithm(signal,JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{call(captures[0],"push",{"algorithm"});return {};},{trace}));
    call(combined,"addEventListener",{"abort",JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{call(captures[0],"push",{"combined"});return {};},{trace})});
    call(first,"abort",{reason});REQUIRE(combined.get("reason").sameValue(reason));REQUIRE(trace.toJSON()->dump()=="[\"algorithm\",\"event\",\"combined\"]");
    call(second,"abort",{"later"});REQUIRE(combined.get("reason").sameValue(reason));REQUIRE(trace.length()==3);
    try{call(combined,"throwIfAborted");FAIL("must throw");}catch(const JSAbortException& exception){REQUIRE(exception.reason.sameValue(reason));}
    REQUIRE_THROWS_AS(signal.set("aborted",false),JSTypeError);
}
TEST_CASE("Native abort listeners removed during dispatch and once before reentry", "[loaders][abort]") {
    auto controller=createNativeAbortController(),signal=controller.get("signal"),trace=JSValue::array();
    auto second=JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{call(captures[0],"push",{2});return {};},{trace});
    auto first=JSValue::function([](const JSValue& self,const Args&,const Args& captures)->JSValue{call(captures[0],"push",{1});call(self,"removeEventListener",{"abort",captures[1]});return {};},{trace,second});
    call(signal,"addEventListener",{"abort",first});call(signal,"addEventListener",{"abort",second});call(controller,"abort");REQUIRE(trace.length()==1);REQUIRE(trace.get("0").toNumber()==1);
}
TEST_CASE("Native abort reports arbitrary thrown values without interrupting other listeners", "[loaders][abort]") {
    auto controller=createNativeAbortController(),signal=controller.get("signal"),reason=JSValue::object(),trace=JSValue::array();
    call(signal,"addEventListener",{"abort",JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{throw JSThrownValue(captures[0]);},{reason})});
    call(signal,"addEventListener",{"abort",JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{throw captures[0];},{reason})});
    call(signal,"addEventListener",{"abort",JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{call(captures[0],"push",{true});return {};},{trace})});
    call(controller,"abort");const auto errors=signal.getHostSlot("reportedErrors");REQUIRE(errors.length()==2);REQUIRE(errors.get("0").sameValue(reason));REQUIRE(errors.get("1").sameValue(reason));REQUIRE(trace.length()==1);
}
TEST_CASE("Loader loadAsync uses dynamic load and queued native promise reactions", "[loaders][promise]") {
    Loader loader;auto scope=loader.object(),state=JSValue::object();state.set("resolved",false);
    scope.set("load",JSValue::function([](const JSValue& self,const Args& args,const Args& captures)->JSValue{REQUIRE(self.sameValue(captures[0]));args[1].call(JSValue{}, {args[0]});return {};},{scope}));
    const auto promise=loader.loadAsync("native-value");call(promise,"then",{JSValue::function([](const JSValue&,const Args& args,const Args& captures)->JSValue{auto state=captures[0];state.set("resolved",args[0]);return {};},{state})});
    REQUIRE_FALSE(state.get("resolved").toBoolean());drainMicrotasks();REQUIRE(state.get("resolved").toString()=="native-value");
    REQUIRE(loader.manager.value().sameValue(DefaultLoadingManager.object()));REQUIRE(loader.crossOrigin.toString()=="anonymous");
}
TEST_CASE("Loader loadAsync progress precedes queued success rejection and thrown values", "[loaders][promise]") {
    for(int mode=0;mode<4;++mode) {
        Loader loader;auto scope=loader.object(),trace=JSValue::array(),reason=JSValue::object();
        const auto operation=JSValue::function([](const JSValue&,const Args& args,const Args& captures)->JSValue {
            call(captures[0],"push",{"load"});args[2].call(JSValue{},{"progress"});
            if(captures[2].strictEquals(2))throw JSThrownValue(captures[1]);
            if(captures[2].strictEquals(0)){args[1].call(JSValue{}, {captures[1]});args[3].call(JSValue{}, {"late reject"});}
            else args[3].call(JSValue{}, {captures[1]});return {};
        },{trace,reason,mode});
        if(mode==3){JSPropertyDescriptor descriptor;descriptor.getter=JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{throw JSThrownValue(captures[0]);},{reason});scope.defineProperty("load",descriptor);}
        else scope.set("load",operation);
        const auto promise=loader.loadAsync("url",JSValue::function([](const JSValue& self,const Args& args,const Args& captures)->JSValue{REQUIRE(self.isUndefined());call(captures[0],"push",{args[0]});return {};},{trace}));
        auto state=JSValue::object();state.set("settled",false);
        const auto reaction=JSValue::function([](const JSValue&,const Args& args,const Args& captures)->JSValue{auto state=captures[0];state.set("settled",args[0]);call(captures[1],"push",{"reaction"});return {};},{state,trace});
        call(promise,"then",{mode==0?reaction:JSValue{},mode==0?JSValue{}:reaction});
        REQUIRE_FALSE(state.get("settled").toBoolean());REQUIRE(trace.length()==(mode==3?0:2));drainMicrotasks();REQUIRE(state.get("settled").sameValue(reason));REQUIRE(trace.length()==(mode==3?1:3));
    }
}
TEST_CASE("BufferGeometryLoader restores shared interleaved storage and intentionally ignores instance count", "[loaders]") {
    auto json=Animation_detail::parseJSON(R"({"isInstancedBufferGeometry":true,"instanceCount":7,"data":{"attributes":{"position":{"isInterleavedBufferAttribute":true,"data":"ib","itemSize":3,"offset":0},"uv":{"isInterleavedBufferAttribute":true,"data":"ib","itemSize":2,"offset":1}},"interleavedBuffers":{"ib":{"uuid":"ib","buffer":"ab","type":"Float32Array","stride":3,"usage":35048}},"arrayBuffers":{"ab":[1065353216,1073741824,1077936128]}}})");
    BufferGeometryLoader loader;auto geometry=loader.parseGeometry(json);auto instanced=std::dynamic_pointer_cast<InstancedBufferGeometry>(geometry);REQUIRE(instanced);REQUIRE(std::isinf(instanced->instanceCount.toNumber()));
    const auto position=std::dynamic_pointer_cast<InterleavedBufferAttribute>(geometry->getAttribute("position")),uv=std::dynamic_pointer_cast<InterleavedBufferAttribute>(geometry->getAttribute("uv"));
    REQUIRE(position);REQUIRE(uv);REQUIRE(position->data.value().sameValue(uv->data.value()));REQUIRE(position->getZ(0)==3);position->setY(0,9);REQUIRE(uv->getX(0)==9);REQUIRE(position->data.get("usage").toNumber()==35048);
    auto attribute=json.get("data").get("attributes").get("position");attribute.set("usage",35044);REQUIRE_THROWS_AS(loader.parseGeometry(json),JSTypeError);
}
TEST_CASE("MaterialLoader custom registration and per instance creators remain dynamic", "[loaders]") {
    MaterialLoader loader;auto json=JSValue::object();json.set("type","CustomLoaderTest");auto textures=JSValue::object();loader.setTextures(textures);
    MaterialLoader::registerMaterial("CustomLoaderTest",JSValue::constructor([](const JSValue& self,const Args&,const Args&)->JSValue {
        auto result=self;result.set("fromJSON",JSValue::function([](const JSValue& self,const Args& args,const Args&)->JSValue{auto result=self;result.set("json",args[0]);result.set("textures",args[1]);return self;}));return {};
    }));
    const auto material=loader.parse(json);REQUIRE(material.get("json").sameValue(json));REQUIRE(material.get("textures").sameValue(textures));
    auto scope=loader.object();scope.set("createMaterialFromType",JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue{return captures[0];},{material}));
    REQUIRE(loader.parse(json).sameValue(material));
}
TEST_CASE("Loader and manager callback cycles are collectible", "[loaders][gc]") {
    auto root=JSValue::object();{LoadingManager warm;warm.abortController.value();Loader warmLoader;}
    root.collectGarbage();const auto baseline=root.heapObjectCount();
    {LoadingManager manager;manager.onLoad=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return {};},{manager.object()});auto controller=manager.abortController.value();call(controller.get("signal"),"addEventListener",{"abort",JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return {};},{controller,manager.object()})});}
    REQUIRE(root.collectGarbage()>0);REQUIRE(root.heapObjectCount()==baseline);
}
