#include "catch.hpp"
#include "animation/AnimationMixer.h"
#include "animation/AnimationObjectGroup.h"
#include "animation/AnimationRuntime.h"
#include "animation/AnimationClip.h"
#include "animation/tracks/NumberKeyframeTrack.h"
#include "core/NativeObject.h"
#include "core/Object3D.h"

namespace {
std::shared_ptr<AnimationClip> scalarClip(const char* name="move",double end=10) {
    auto tracks=std::make_shared<AnimationClip::Tracks>();tracks->push_back(std::make_shared<NumberKeyframeTrack>(".position[x]",AnimationArray::array({0,1}),AnimationArray::array({0,end})));
    return std::make_shared<AnimationClip>(name,1,tracks);
}
JSValue item(JSValue array,int i){return array.get(std::to_string(i));}
}
TEST_CASE("AnimationMixer shares binding buffers and restores state after the final action stops", "[animation][mixer]") {
    auto root=std::make_shared<Object3D>();root->position.x=1;AnimationMixer mixer(root);auto first=scalarClip(),second=scalarClip("other",20);
    auto a=mixer.clipAction(first),b=mixer.clipAction(second);REQUIRE(mixer.clipAction(first)==a);
    const auto ba=item(a.get("_propertyBindings"),0),bb=item(b.get("_propertyBindings"),0);
    REQUIRE(ba.sameValue(bb));REQUIRE(ba.get("referenceCount").number()==2);
    REQUIRE(item(a.get("_interpolants"),0).get("resultBuffer").sameValue(ba.get("buffer")));
    a.play();mixer.update(.25);REQUIRE(root->position.x==2.5);
    b.play();mixer.update(.25);REQUIRE(root->position.x==5);
    a.stop();REQUIRE(ba.get("useCount").number()==1);b.stop();REQUIRE(root->position.x==1);
    REQUIRE(mixer.stats.get("bindings").get("inUse").number()==0);
    mixer.uncacheClip(first);REQUIRE(ba.get("referenceCount").number()==1);mixer.uncacheClip(second);REQUIRE(mixer.stats.get("bindings").get("total").number()==0);
}
TEST_CASE("AnimationMixer preserves root action identities and the upstream uncached reactivation error", "[animation][mixer]") {
    auto first=std::make_shared<Object3D>(),second=std::make_shared<Object3D>();auto clip=scalarClip();AnimationMixer mixer(first);
    auto a=mixer.clipAction(clip),b=mixer.clipAction(clip,nativeObject(second));REQUIRE_FALSE(a==b);
    REQUIRE_FALSE(item(a.get("_propertyBindings"),0).sameValue(item(b.get("_propertyBindings"),0)));
    a.play();b.play();mixer.update(.5);REQUIRE(first->position.x==5);REQUIRE(second->position.x==5);
    mixer.uncacheRoot(nativeObject(second));REQUIRE(second->position.x==0);REQUIRE_FALSE(mixer.existingAction(clip,nativeObject(second)));
    // Fixed upstream leaves the removed binding's cache index set. Reactivating
    // this retained action consequently throws after putting it back in the cache.
    REQUIRE_THROWS_AS(b.play(),JSTypeError);REQUIRE(mixer.existingAction(clip,nativeObject(second))==b);
}
TEST_CASE("AnimationMixer fade and warp controls share the upstream module result view", "[animation][mixer]") {
    AnimationMixer one(std::make_shared<Object3D>()),two(std::make_shared<Object3D>());auto clip=scalarClip();auto a=one.clipAction(clip),b=two.clipAction(clip);
    a.fadeIn(2);a.warp(1,2,2);b.fadeOut(3);
    const auto a0=item(one.get("_controlInterpolants"),0),a1=item(one.get("_controlInterpolants"),1),b0=item(two.get("_controlInterpolants"),0);
    REQUIRE(a0.get("resultBuffer").sameValue(a1.get("resultBuffer")));REQUIRE(a0.get("resultBuffer").sameValue(b0.get("resultBuffer")));
    a.stopFading();REQUIRE(one.stats.get("controlInterpolants").get("inUse").number()==1);
    a.stopWarping();REQUIRE(one.stats.get("controlInterpolants").get("inUse").number()==0);
}
TEST_CASE("AnimationMixer event reentry can stop the current action before property application", "[animation][mixer]") {
    auto root=std::make_shared<Object3D>();root->position.x=3;AnimationMixer mixer(root);auto a=mixer.clipAction(scalarClip());a.setLoop(LoopOnce,1);
    auto observed=JSValue::array();auto listener=JSValue::function([](const JSValue& self,const auto& args,const auto& c)->JSValue{
        const auto event=args[0],action=event.get("action");REQUIRE(event.get("target").sameValue(self));
        auto output=c[0];output.set("0",action.get("time"));output.set("1",action.get("enabled"));action.get("stop").call(action);self.collectGarbage();return {};
    },{observed});mixer.addEventListener("finished",listener);a.play();mixer.update(2);
    REQUIRE(observed.get("0").number()==1);REQUIRE_FALSE(observed.get("1").toBoolean());REQUIRE(root->position.x==3);REQUIRE(a.time.toNumber()==0);
}
TEST_CASE("AnimationMixer group updates use one shared original state and skip cached objects", "[animation][mixer]") {
    auto first=std::make_shared<Object3D>(),second=std::make_shared<Object3D>();first->position.x=1;second->position.x=4;
    AnimationObjectGroup group({nativeObject(first),nativeObject(second)});AnimationMixer mixer(group.object());auto clip=scalarClip();auto a=mixer.clipAction(clip);
    a.play();mixer.update(.25);REQUIRE(first->position.x==2.5);REQUIRE(second->position.x==2.5);
    group.remove({nativeObject(first)});mixer.update(.25);REQUIRE(first->position.x==2.5);REQUIRE(second->position.x==5);
    a.stop();REQUIRE(second->position.x==1);REQUIRE(first->position.x==2.5);
    REQUIRE_THROWS_AS(mixer.uncacheClip(clip),JSTypeError);
}
TEST_CASE("AnimationMixer managed action and binding cycles release native roots after collection", "[animation][mixer]") {
    const auto keeper=AnimationMixer::prototype();std::weak_ptr<Object3D> weak;
    {auto root=std::make_shared<Object3D>();weak=root;AnimationMixer mixer(root);auto clip=scalarClip();auto a=mixer.clipAction(clip);a.play();mixer.update(.25);mixer.object().collectGarbage();REQUIRE_FALSE(weak.expired());}
    keeper.collectGarbage();REQUIRE(weak.expired());
}
TEST_CASE("Animation value handles pass between group mixer and action APIs without losing identity", "[animation][mixer]") {
    AnimationObjectGroup group({nativeObject(std::make_shared<Object3D>())});
    AnimationMixer mixer(group);auto clip=scalarClip(),otherClip=scalarClip("other");
    auto a=mixer.clipAction(clip),b=mixer.clipAction(otherClip);
    JSValue groupValue=group,mixerValue=mixer,actionValue=a;
    REQUIRE(groupValue.sameValue(group.object()));REQUIRE(mixerValue.sameValue(mixer.object()));REQUIRE(actionValue.sameValue(a.object()));
    REQUIRE(mixer.getRoot().sameValue(groupValue));REQUIRE(a.crossFadeTo(b,1).sameValue(b.object()));
    REQUIRE(a.syncWith(b).sameValue(a.object()));
    AnimationAction detached(mixer,nativeAnimationClip(clip));REQUIRE(detached.getMixer().sameValue(mixer.object()));
    for(const auto& p:{AnimationAction::prototype(),AnimationObjectGroup::prototype(),AnimationMixer::prototype()}){
        REQUIRE(p.hasOwn("constructor"));REQUIRE(p.get("constructor").get("prototype").sameValue(p));
        REQUIRE_THROWS_AS(p.get("constructor").call(),JSTypeError);
    }
}
