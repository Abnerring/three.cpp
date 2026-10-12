#include "catch.hpp"
#include "animation/AnimationAction.h"
#include "constants.h"

namespace {
JSValue emptyClip() { auto clip=JSValue::object();clip.set("duration",2);clip.set("blendMode",NormalAnimationBlendMode);clip.set("tracks",JSValue::array());return clip; }
JSValue emptyMixer() { auto mixer=JSValue::object();mixer.set("time",0);mixer.set("_root",JSValue::object());return mixer; }
void property(JSValue object,const char* key,JSValue getter){JSPropertyDescriptor d;d.getter=getter;d.configurable=true;object.defineProperty(key,d);}
void append(JSValue array,JSValue value){array.set(std::to_string(array.length()),value);}
}
TEST_CASE("AnimationAction is a managed identity handle with shared methods and sparse bindings", "[animation][action]") {
    auto mixer=emptyMixer(),clip=emptyClip(),tracks=JSValue::array();
    for(int i=0;i<2;++i){auto track=JSValue::object();track.set("createInterpolant",JSValue::function([](const JSValue&,const auto& args,const auto&)->JSValue{
        REQUIRE(args[0].isNull());return JSValue::object();
    }));tracks.set(std::to_string(i),track);}clip.set("tracks",tracks);
    AnimationAction a(mixer,clip),b(mixer,emptyClip());auto alias=AnimationAction::fromObject(a.object());
    REQUIRE(a==alias);REQUIRE_FALSE(a==b);REQUIRE_FALSE(AnimationAction::fromObject(nullptr));
    alias.set("weight",.25);REQUIRE(a.get("weight").number()==.25);
    REQUIRE(a.get("play").sameValue(b.get("play")));REQUIRE_FALSE(a.object().hasOwn("play"));
    REQUIRE(a.get("_propertyBindings").length()==2);REQUIRE_FALSE(a.get("_propertyBindings").hasOwn("0"));
    const auto interpolants=a.get("_interpolants");
    REQUIRE(interpolants.get("0").get("settings").sameValue(interpolants.get("1").get("settings")));
    REQUIRE(interpolants.get("0").get("settings").sameValue(a.get("_interpolantSettings")));
    REQUIRE(a.get("crossFadeFrom").get("length").number()==2);
}
TEST_CASE("AnimationAction honors method overrides and exact short circuit return values", "[animation][action]") {
    auto mixer=emptyMixer();AnimationAction a(mixer,emptyClip());
    a.set("enabled","");REQUIRE(a.isRunning().strictEquals(""));
    a.set("stopFading",JSValue::function([](const JSValue&,const auto&,const auto&)->JSValue{return "override";}));
    REQUIRE(a.setEffectiveWeight(.5).strictEquals("override"));REQUIRE(a.get("weight").number()==.5);
    auto other=JSValue::object();other.set("crossFadeFrom",JSValue::function([](const JSValue& self,const auto& args,const auto& c)->JSValue{
        REQUIRE(self.sameValue(c[0]));REQUIRE(args[0].sameValue(c[1]));REQUIRE(args[2].strictEquals(false));return 23;
    },{other,a.object()}));
    REQUIRE(a.crossFadeTo(other,2,JSValue{}).number()==23);
}
TEST_CASE("AnimationAction dispatches finished after state mutation and preserves local time across reentry", "[animation][action]") {
    auto mixer=emptyMixer(),trace=JSValue::array();AnimationAction a(mixer,emptyClip());a.set("loop",LoopOnce);
    mixer.set("dispatchEvent",JSValue::function([](const JSValue&,const auto& args,const auto& c)->JSValue{
        auto action=args[0].get("action");REQUIRE(args[0].get("type").strictEquals("finished"));
        append(c[0],action.get("time"));append(c[0],action.get("enabled"));action.set("time",99);c[0].collectGarbage();return {};
    },{trace}));
    REQUIRE(a._updateTime(3).number()==2);REQUIRE(a.get("time").number()==99);
    REQUIRE(trace.toJSON()->dump()=="[2,false]");
}
TEST_CASE("AnimationAction resolves methods before coercing callback arguments", "[animation][action]") {
    auto mixer=emptyMixer(),trace=JSValue::array();AnimationAction a(mixer,emptyClip());
    property(a.object(),"warp",JSValue::function([](const JSValue&,const auto&,const auto& c)->JSValue{
        append(c[0],"method");return c[1];
    },{trace,JSValue::function([](const JSValue&,const auto& args,const auto& c)->JSValue{append(c[0],args[0]);return {};},{trace})}));
    property(a.object(),"_effectiveTimeScale",JSValue::function([](const JSValue&,const auto&,const auto& c)->JSValue{append(c[0],"scale");return 7;},{trace}));
    a.halt(1);REQUIRE(trace.toJSON()->dump()=="[\"method\",\"scale\",7]");
}
TEST_CASE("AnimationAction mixer cycles are reclaimed after the last external handle", "[animation][action]") {
    const auto prototype=AnimationAction::prototype();prototype.collectGarbage();const auto baseline=prototype.heapObjectCount();
    {auto mixer=emptyMixer();AnimationAction a(mixer,emptyClip());mixer.set("action",a.object());a.object().collectGarbage();REQUIRE(a.getMixer().get("action").sameValue(a.object()));}
    REQUIRE(prototype.collectGarbage()>0);REQUIRE(prototype.heapObjectCount()==baseline);
}
TEST_CASE("AnimationAction field proxies follow their own handle after copy move and assignment", "[animation][action]") {
    AnimationAction original(emptyMixer(),emptyClip());original.time=3;
    AnimationAction copied=original;copied.weight=.25;REQUIRE(original.weight.toNumber()==.25);
    auto moved=std::move(copied);moved.time=4;REQUIRE(original.time.toNumber()==4);
    AnimationAction replacement(emptyMixer(),emptyClip());replacement.time=8;
    moved=replacement;moved.time=9;REQUIRE(replacement.time.toNumber()==9);REQUIRE(original.time.toNumber()==4);
    original=std::move(replacement);original.timeScale=2;REQUIRE(moved.timeScale.toNumber()==2);
}
