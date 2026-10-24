#include "nodes/core/NodeBuilder.h"
#include "nodes/core/ConstNode.h"
#include "nodes/core/constants.h"
#include "core/JSPromise.h"
#include "core/JSIterator.h"
#include "core/JSRegExp.h"
#include "catch.hpp"

namespace {
using Args=std::vector<JSValue>;
JSValue invoke(JSValue self,const char* key,const Args& args={}) {return self.get(key).call(self,args);}
JSValue builder() {
    auto renderer=JSValue::object(),context=JSValue::object();
    context.set("isContextNode",true);
    context.set("getFlowContextData",JSValue::function(+[](const JSValue&,const Args&,const Args&){return JSValue::object();}));
    renderer.set("contextNode",context);
    auto b=NodeBuilder(nullptr,renderer,nullptr).object();
    b.set("compute",nativeConstNodeConstructor().construct({7}));
    b.set("buildCode",JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{
        auto b=self;b.set("computeShader",invoke(b,"getFlowData",{b.get("compute")}).get("result"));return {};
    }));return b;
}
JSValue observe(JSValue promise) {
    auto status=JSValue::object();status.set("state","pending");
    invoke(promise,"then",{JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{
        auto s=c[0];s.set("state","fulfilled");s.set("value",a[0]);return {};
    },{status}),JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{
        auto s=c[0];s.set("state","rejected");s.set("value",a[0]);return {};
    },{status})});return status;
}
}
TEST_CASE("NodeBuilder default async build yields nine separate native tasks", "[NodeBuilder][async]") {
    drainMicrotasks();REQUIRE_FALSE(hasPendingHostTasks());
    auto b=builder();auto status=observe(invoke(b,"buildAsync"));
    REQUIRE(status.get("state").strictEquals("pending"));
    REQUIRE(b.get("buildStage").strictEquals("setup"));
    REQUIRE(b.get("shaderStage").strictEquals("fragment"));
    for(unsigned i=0;i<9;++i) {
        REQUIRE(hasPendingHostTasks());REQUIRE(drainHostTasks()==1);
        REQUIRE(status.get("state").strictEquals(i==8?"fulfilled":"pending"));
    }
    REQUIRE_FALSE(hasPendingHostTasks());REQUIRE(status.get("value").strictEquals(b));
    REQUIRE(b.get("computeShader").strictEquals("7.0"));
    REQUIRE(b.get("buildStage").isNull());REQUIRE(b.get("shaderStage").isNull());
}
TEST_CASE("NodeBuilder async accepts plain yields and preserves rejected stage", "[NodeBuilder][async]") {
    auto b=builder(),counter=JSValue::object();counter.set("n",0);
    auto yield=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        auto n=c[0];auto count=n.get("n").number()+1;n.set("n",count);
        if(count==4)throw JSThrownValue("stop");return 0;
    },{counter});
    const auto status=observe(invoke(b,"buildAsync",{yield}));
    REQUIRE(counter.get("n").strictEquals(1));REQUIRE(status.get("state").strictEquals("pending"));
    drainMicrotasks();REQUIRE(status.get("state").strictEquals("rejected"));
    REQUIRE(status.get("value").strictEquals("stop"));
    REQUIRE(b.get("buildStage").strictEquals("analyze"));REQUIRE(b.get("shaderStage").strictEquals("fragment"));
}
TEST_CASE("Intrinsic Await ignores a native promise own then getter", "[runtime][promise][async]") {
    auto promise=resolveNativePromise(31),state=JSValue::object();state.set("calls",0);
    JSPropertyDescriptor descriptor;
    descriptor.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSThrownValue("must not read then");});
    descriptor.configurable=true;promise.defineProperty("then",descriptor);
    awaitNativePromise(promise,JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{
        auto s=c[0];s.set("calls",1);s.set("value",a[0]);return {};
    },{state}),{});
    REQUIRE(state.get("calls").strictEquals(0));drainMicrotasks();
    REQUIRE(state.get("calls").strictEquals(1));REQUIRE(state.get("value").strictEquals(31));
}
TEST_CASE("Suspended NodeBuilder retains its graph and releases it after rejection", "[NodeBuilder][async][gc]") {
    drainMicrotasks();
    const auto deferred=createNativePromiseCapability();
    auto collector=JSValue::object();
    JSValue status;
    std::weak_ptr<int> lifetime;
    {
        auto b=builder();
        const auto payload=std::make_shared<int>(17);lifetime=payload;
        b.set("lifetimePayload",JSValue::hostObject(payload,payload.get(),"async.builder.lifetime"));
        const auto yield=JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue {
            return captures[0];
        },{deferred.promise});
        status=observe(invoke(b,"buildAsync",{yield}));
    }
    collector.collectGarbage();
    REQUIRE_FALSE(lifetime.expired());
    REQUIRE(status.get("state").strictEquals("pending"));
    deferred.reject.call({}, {"stop"});drainMicrotasks();
    REQUIRE(status.get("state").strictEquals("rejected"));
    REQUIRE(status.get("value").strictEquals("stop"));
    collector.collectGarbage();
    REQUIRE(lifetime.expired());
}
TEST_CASE("Host task pumping finishes the previous checkpoint and prevents nested pumping", "[runtime][promise][async]") {
    auto events=JSValue::array();
    queueMicrotask(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {invoke(c[0],"push",{"micro"});return {};},{events}));
    queueHostTask(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        invoke(c[0],"push",{"task"});invoke(c[0],"push",{drainHostTasks()});
        queueMicrotask(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{invoke(c[0],"push",{"after"});return {};},{c[0]}));return {};
    },{events}));
    REQUIRE(drainHostTasks()==1);REQUIRE(invoke(events,"join",{","}).strictEquals("micro,task,0,after"));
}
TEST_CASE("String parser prerequisites preserve match protocols and UTF16 substring", "[runtime][string]") {
    REQUIRE(invoke("abcdef","substring",{4,1}).strictEquals("bcd"));
    REQUIRE(invoke("\xF0\x9F\x98\x80","substring",{1,2}).strictEquals("\xED\xB8\x80"));
    REQUIRE(invoke("void test(float x)","match",{createNativeRegExp("(test)")}).get("1").strictEquals("test"));
    auto pattern=JSValue::object(),receiver=JSValue::object();
    pattern.set(JSValue::wellKnownSymbol("match"),JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{return a[0].strictEquals(c[0]);},{receiver}));
    REQUIRE(JSValue("x").get("match").call(receiver,{pattern}).strictEquals(true));
}
