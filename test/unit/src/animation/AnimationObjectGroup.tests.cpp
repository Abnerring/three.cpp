#include "catch.hpp"
#include "animation/AnimationObjectGroup.h"
#include "animation/PropertyBinding.h"

namespace {
JSValue groupRoot(const char* uuid, double value=0) {
    auto root=JSValue::object();root.set("uuid",uuid);root.set("name",uuid);root.set("value",value);root.set("children",JSValue::array());return root;
}
}
TEST_CASE("AnimationObjectGroup swaps active and cached slots with shared bindings", "[animation][AnimationObjectGroup]") {
    auto a=groupRoot("a",1),b=groupRoot("b",2),c=groupRoot("c",3);
    AnimationObjectGroup group({a,b});const auto object=group.object();
    auto values=group.subscribe_(".value");REQUIRE(values.get("length").number()==2);
    REQUIRE(values.get("0").get("rootNode").sameValue(a));REQUIRE(group.subscribe_(".value").sameValue(values));
    group.remove({a});REQUIRE(object.get("nCachedObjects_").number()==1);
    auto other=group.subscribe_(".other");REQUIRE_FALSE(other.hasOwn("0"));
    group.add({c,a});REQUIRE(object.get("nCachedObjects_").number()==0);REQUIRE(values.get("length").number()==3);
    REQUIRE(other.get("0").get("rootNode").sameValue(a));
    group.remove({b});group.uncache({b});REQUIRE(values.get("length").number()==2);REQUIRE(object.get("nCachedObjects_").number()==0);
    REQUIRE(object.get("stats").get("objects").get("inUse").number()==2);
}
TEST_CASE("AnimationObjectGroup preserves stale subscription indices from upstream", "[animation][AnimationObjectGroup]") {
    AnimationObjectGroup group({groupRoot("a")});auto bindings=group.subscribe_(".value");
    group.unsubscribe_(".value");REQUIRE(group.subscribe_(".value").isUndefined());
    REQUIRE(bindings.get("length").number()==1);group.add({groupRoot("b")});REQUIRE(bindings.get("length").number()==1);
    REQUIRE(group.object().get("_bindingsIndicesByPath").get(".value").number()==0);
}
TEST_CASE("AnimationObjectGroup stats capture scope and inUse uses receiver total", "[animation][AnimationObjectGroup]") {
    AnimationObjectGroup group({groupRoot("a"),groupRoot("b")});auto object=group.object(),stats=object.get("stats"),counts=stats.get("objects");
    group.remove({object.get("_objects").get("0")});
    auto receiver=JSValue::object();receiver.set("total",10);
    REQUIRE(counts.getOwnPropertyDescriptor("inUse")->getter->call(receiver).number()==9);
    REQUIRE(counts.getOwnPropertyDescriptor("total")->getter->call(receiver).number()==2);
    REQUIRE(AnimationObjectGroup::fromObject(object).object().sameValue(object));
    AnimationObjectGroup second;REQUIRE(object.get("add").sameValue(second.object().get("add")));REQUIRE_FALSE(object.hasOwn("add"));
}
TEST_CASE("AnimationObjectGroup works through PropertyBinding Composite", "[animation][AnimationObjectGroup]") {
    auto a=groupRoot("a",1),b=groupRoot("b",2);AnimationObjectGroup group({a,b});
    auto binding=PropertyBinding::create(group.object(),".value");binding->setValue(Float64Array({7}),0);
    REQUIRE(a.get("value").number()==7);REQUIRE(b.get("value").number()==7);
    group.remove({a});binding->setValue(Float64Array({8}),0);REQUIRE(a.get("value").number()==7);REQUIRE(b.get("value").number()==8);
}
TEST_CASE("AnimationObjectGroup graph cycles and detached stats remain collectable", "[animation][AnimationObjectGroup]") {
    AnimationObjectGroup warm({groupRoot("warm")});warm.subscribe_(".value");auto witness=JSValue::object();witness.collectGarbage();const auto baseline=witness.heapObjectCount();
    {
        auto root=groupRoot("cycle");AnimationObjectGroup group({root});auto state=group.object();state.set("cycle",state);state.get("stats").set("cycle",state);
        root.set("group",state);group.subscribe_(".value");
    }
    witness.collectGarbage();REQUIRE(witness.heapObjectCount()==baseline);
    JSValue counts;
    {AnimationObjectGroup group({groupRoot("a")});counts=group.object().get("stats").get("objects");}
    counts.collectGarbage();REQUIRE(counts.get("total").number()==1);
}
TEST_CASE("AnimationObjectGroup field proxies remain attached to copied and moved handles", "[animation][AnimationObjectGroup]") {
    AnimationObjectGroup first({groupRoot("a")}), second=first;
    REQUIRE(first==second);second.uuid="shared";REQUIRE(first.uuid.toString()=="shared");
    AnimationObjectGroup third(std::move(second));third.nCachedObjects_=1;REQUIRE(first.nCachedObjects_.toNumber()==1);
    AnimationObjectGroup distinct;third=distinct;third.uuid="new";REQUIRE(distinct.uuid.toString()=="new");REQUIRE(first.uuid.toString()=="shared");
}
