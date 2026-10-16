#include "catch.hpp"
#include "core/JSValue.h"
#include "core/NativeObject.h"
#include "core/JSJobs.h"
#include "core/JSWeakRef.h"
#include "renderers/webgl/WebGLUniforms.h"
#include "textures/Texture.h"
#include <thread>
#include <exception>
#include <functional>
#include <unordered_set>

namespace {
void isolated(const std::function<void()>& task) {
 std::exception_ptr error;
 std::thread worker([&] { try { task(); } catch (...) { error = std::current_exception(); } });
 worker.join(); if(error) std::rethrow_exception(error);
}
void check(bool value) { if(!value) throw std::runtime_error("isolated check failed"); }
struct Native {
 JSValue field = JSValue::object();
 std::shared_ptr<Native> child;
 std::function<void()> finalize;
 ~Native(){ if(finalize) finalize(); }
};
std::vector<JSNativeOwnerNode> nativeOwners(const JSValue& self) {
 auto owner=std::static_pointer_cast<Native>(self.hostPayload("test.NativeExit"));
 std::vector<JSNativeOwnerNode> result;
 std::unordered_set<Native*> seen;
 std::function<void(const std::shared_ptr<Native>&)> visit=[&](const auto& item) {
  if(!item||!seen.insert(item.get()).second)return;
  JSNativeOwnerNode node{item,{&item->field},{}};
  if(item->child)node.edges.push_back({&item->child,item->child});
  result.push_back(std::move(node)); visit(item->child);
 };
 visit(owner); return result;
}
JSValue wrap(const std::shared_ptr<Native>& owner) {
 auto value=JSValue::hostObject(owner,owner.get(),"test.NativeExit");
 auto hooks=std::make_shared<JSHostObjectHooks>();hooks->nativeOwnership=nativeOwners;
 value.setHostHooks(hooks); return value;
}
}
TEST_CASE("Native host safepoint retires actual Texture and intrinsic fallback graph") {
 isolated([] {
  check(!JSValue::hasActiveHeap());
  for(unsigned n=0;n<8;++n) {
   JSWeakValue wrapper,module;
   { auto texture=nativeTexture(std::make_shared<Texture>()); wrapper=JSWeakValue(texture);
     auto uniforms=nativeWebGLUniformsConstructor();module=JSWeakValue(uniforms);
     texture.collectGarbage();check(!wrapper.expired()&&!module.expired()); }
   check(wrapper.expired()&&module.expired());check(!JSValue::hasActiveHeap());
  }
 }); REQUIRE(true);
}
TEST_CASE("External native Texture owner preserves userData and wrapper graph until next checkpoint") {
 isolated([] {
  auto texture=std::make_shared<Texture>();JSWeakValue wrapper,field,module;
  {auto value=nativeTexture(texture); auto data=texture->userData;data.set("answer",42);
   field=JSWeakValue(data);wrapper=JSWeakValue(value); value.set("extra",data);
   auto intrinsic=JSValue::object();intrinsic.retainForHeapLifetime();module=JSWeakValue(intrinsic);}
  check(!wrapper.expired()&&!field.expired()&&!module.expired());
  check(texture->userData.get("answer").number()==42);
  texture.reset();check(wrapper.expired()&&field.expired()&&module.expired());
  check(!JSValue::hasActiveHeap());
 }); REQUIRE(true);
}
TEST_CASE("Native pure C++ cycles and exits keep JS fields until application explicitly breaks ownership") {
 isolated([] {
  auto parent=std::make_shared<Native>(),child=std::make_shared<Native>();
  parent->child=child;child->child=parent;
  JSWeakValue a,b;
  {auto v=wrap(parent);auto w=wrap(child);a=JSWeakValue(v);b=JSWeakValue(w);}
  std::weak_ptr<Native> parentWeak=parent; parent.reset();child.reset();
  check(!a.expired()&&!b.expired());
  {auto retained=parentWeak.lock();check(bool(retained));retained->child->child.reset();retained->child.reset();}
  check(a.expired()&&b.expired());check(parentWeak.expired());check(!JSValue::hasActiveHeap());
 }); REQUIRE(true);
}
TEST_CASE("Finalizer JS escape restores intrinsic roots then later releases realm") {
 isolated([] {
  JSValue escaped;JSWeakValue old,module;unsigned destroyed=0;
  {auto intrinsic=JSValue::object();intrinsic.retainForHeapLifetime();module=JSWeakValue(intrinsic);
   auto owner=std::make_shared<Native>();old=JSWeakValue(wrap(owner));
   owner->finalize=[&]{++destroyed;escaped=JSValue::object();escaped.set("answer",9);};}
  check(old.expired());check(destroyed==1);check(!module.expired());check(escaped.get("answer").number()==9);
  escaped={};check(module.expired());check(!JSValue::hasActiveHeap());
 }); REQUIRE(true);
}
TEST_CASE("Native finalizer external owner escape and new graph are rechecked") {
 isolated([] {
  std::shared_ptr<Native> escaped;JSWeakValue module;unsigned destroyed=0;
  {auto intrinsic=JSValue::object();intrinsic.retainForHeapLifetime();module=JSWeakValue(intrinsic);
   auto owner=std::make_shared<Native>();auto wrapper=wrap(owner);
   owner->finalize=[&]{++destroyed;escaped=std::make_shared<Native>();static_cast<void>(wrap(escaped));};}
  check(JSValue::nativeSafepoint()!=0);check(destroyed==1);check(!module.expired());
  check(bool(escaped));escaped.reset();check(module.expired());check(!JSValue::hasActiveHeap());
 }); REQUIRE(true);
}
TEST_CASE("Pending native microtasks and WeakRef kept targets defer native realm retirement") {
 isolated([] {
  JSWeakValue module,texture;
  {auto m=JSValue::object();m.retainForHeapLifetime();module=JSWeakValue(m);
   auto value=nativeTexture(std::make_shared<Texture>());texture=JSWeakValue(value);
   queueMicrotask(JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&){return JSValue{};},{value}));}
  check(!module.expired()&&!texture.expired());check(drainMicrotasks()==1);
  check(texture.expired()&&module.expired());check(!JSValue::hasActiveHeap());
 }); REQUIRE(true);
}
TEST_CASE("Normal explicit GC retains native module roots while application root lives") {
 isolated([] {
  auto realm=JSValue::object();JSWeakValue module;
  {auto intrinsic=nativeWebGLUniformsConstructor();module=JSWeakValue(intrinsic);}
  for(unsigned i=0;i<8;++i){realm.collectGarbage();check(!module.expired());}
  realm={};check(module.expired());check(!JSValue::hasActiveHeap());
 }); REQUIRE(true);
}
TEST_CASE("Foreign weak checkpoint refuses before inspecting native hooks") {
 isolated([] {
  auto texture=nativeTexture(std::make_shared<Texture>());JSWeakValue weak(texture);
  unsigned rejected=0;std::thread other([&]{try{static_cast<void>(weak.lock());}catch(const JSTypeError&){++rejected;}
   try{static_cast<void>(weak.expired());}catch(const JSTypeError&){++rejected;}});
  other.join();check(rejected==2);check(!weak.expired());
 }); REQUIRE(true);
}

TEST_CASE("WeakRef kept target alone defers intrinsic native heap exit until job ends") {
 isolated([] {
  JSWeakValue target,module;
  {auto texture=nativeTexture(std::make_shared<Texture>());target=JSWeakValue(texture);
   auto ref=createNativeWeakRef(texture);check(JSWeakRef(ref).deref().sameValue(texture));
   auto m=JSValue::object();m.retainForHeapLifetime();module=JSWeakValue(m);}
  check(!target.expired()&&!module.expired());endJSJob();
  check(target.expired()&&module.expired());check(!JSValue::hasActiveHeap());
 }); REQUIRE(true);
}
TEST_CASE("Native checkpoint hook exceptions restore collection guard and do not retire graph") {
 isolated([] {
  bool fail=true;unsigned calls=0;
  auto native=std::make_shared<Native>();auto wrapper=wrap(native);JSWeakValue weak(wrapper);
  auto hooks=std::make_shared<JSHostObjectHooks>();
  hooks->nativeOwnership=+[](const JSValue& self) {
   auto context=self.getHostSlot("testContext").hostPayload("test.Context");
   auto pointers=static_cast<std::pair<bool*,unsigned*>*>(context.get());++*pointers->second;
   if(*pointers->first)throw std::runtime_error("ownership failure");return nativeOwners(self);
  };
  std::pair<bool*,unsigned*> pointers{&fail,&calls};
  auto context=JSValue::hostObject(std::shared_ptr<void>(&pointers,+[](void*){}),&pointers,"test.Context");
  wrapper.setHostSlot("testContext",context);wrapper.setHostHooks(hooks);
  bool caught=false;try{static_cast<void>(weak.expired());}catch(const std::runtime_error&){caught=true;}
  check(caught&&calls==1);fail=false;check(!weak.expired());
  auto pureHooks=std::make_shared<JSHostObjectHooks>();pureHooks->nativeOwnership=nativeOwners;wrapper.setHostHooks(pureHooks);
  context={};wrapper={};native.reset();check(weak.expired());check(!JSValue::hasActiveHeap());
 }); REQUIRE(true);
}
TEST_CASE("Native exit callback weak locks and nested checkpoints do not start another collection") {
 isolated([] {
  JSWeakValue old,module;unsigned finalizers=0;bool read=false;std::size_t nested=99;
  {auto m=JSValue::object();m.retainForHeapLifetime();module=JSWeakValue(m);
   auto owner=std::make_shared<Native>();auto wrapper=wrap(owner);old=JSWeakValue(wrapper);
   owner->finalize=[&]{++finalizers;read=bool(module.lock());nested=JSValue::nativeSafepoint();};}
  check(old.expired());check(finalizers==1&&read&&nested==0);check(module.expired());
  check(!JSValue::hasActiveHeap());
 }); REQUIRE(true);
}

namespace { struct FinalizingTexture : Texture {
 std::function<void()> finalizer;
 ~FinalizingTexture() { if(finalizer) finalizer(); }
}; }
TEST_CASE("Unrelated native finalizer escape preserves actual intrinsic fallback Texture before commit") {
 isolated([] {
  JSValue escaped;JSWeakValue moduleWeak;unsigned fallbackDestroyed=0,ordinaryDestroyed=0;
  {auto a=std::make_shared<FinalizingTexture>(),b=std::make_shared<FinalizingTexture>();
   std::owner_less<void> less;auto first=less(a,b)?a:b,second=less(a,b)?b:a;
   first->finalizer=[&]{++fallbackDestroyed;};
   second->finalizer=[&]{++ordinaryDestroyed;escaped=JSValue::object();escaped.set("rescued",true);};
   auto fallback=nativeTexture(first),ordinary=nativeTexture(second);
   auto module=JSValue::object();module.set("fallback",fallback);module.retainForHeapLifetime();moduleWeak=JSWeakValue(module);}
  JSValue::nativeSafepoint();check(fallbackDestroyed==0&&ordinaryDestroyed==1);
  {auto module=moduleWeak.lock();check(bool(module));check(module->get("fallback").isHostObject("three.Texture"));}
  escaped={};check(moduleWeak.expired());check(fallbackDestroyed==1);check(!JSValue::hasActiveHeap());
 }); REQUIRE(true);
}
TEST_CASE("Intrinsic native destructor escape preserves other payloads and exposes its own retired brand") {
 isolated([] {
  JSValue escaped;JSWeakValue moduleWeak,selfWeak;unsigned selfDestroyed=0,otherDestroyed=0;
  {auto a=std::make_shared<FinalizingTexture>(),b=std::make_shared<FinalizingTexture>();
   std::owner_less<void> less;auto first=less(a,b)?a:b,second=less(a,b)?b:a;
   auto self=nativeTexture(first),other=nativeTexture(second);selfWeak=JSWeakValue(self);
   first->finalizer=[&]{++selfDestroyed;auto value=selfWeak.lock();check(bool(value));escaped=*value;};
   second->finalizer=[&]{++otherDestroyed;};
   auto module=JSValue::object();module.set("self",self);module.set("other",other);module.retainForHeapLifetime();moduleWeak=JSWeakValue(module);}
  JSValue::nativeSafepoint();check(selfDestroyed==1&&otherDestroyed==0);
  check(escaped.isObject()&&!escaped.isHostObject("three.Texture"));
  {auto module=moduleWeak.lock();check(bool(module));check(module->get("self").sameValue(escaped));
   check(module->get("other").isHostObject("three.Texture"));}
  escaped={};check(moduleWeak.expired());check(otherDestroyed==1);check(!JSValue::hasActiveHeap());
 }); REQUIRE(true);
}

namespace {
struct AllocationHookState {
 bool fired=false, throwAfter=false;
 std::vector<JSValue> roots;
 std::shared_ptr<Native> owner;
 JSWeakValue host;
};
thread_local AllocationHookState* allocationState=nullptr;
void publishRoots() {
 auto& state=*allocationState;if(state.fired)return;state.fired=true;
 for(unsigned i=0;i<512;++i){auto value=JSValue::object();value.set("index",i);state.roots.push_back(value);
  auto owner=std::make_shared<Native>();state.roots.push_back(wrap(owner));}
 state.owner=std::make_shared<Native>();auto host=wrap(state.owner);host.set("extra",42);state.host=JSWeakValue(host);
 if(state.throwAfter)throw std::runtime_error("published root then threw");
}
}
TEST_CASE("Safepoint ownership hook allocations preserve published roots and tolerate host table rehash") {
 isolated([] {
  AllocationHookState state;allocationState=&state;JSWeakValue old,module;
  {auto intrinsic=JSValue::object();intrinsic.retainForHeapLifetime();module=JSWeakValue(intrinsic);
   auto owner=std::make_shared<Native>();auto host=wrap(owner);old=JSWeakValue(host);
   auto hooks=std::make_shared<JSHostObjectHooks>();hooks->nativeOwnership=+[](const JSValue& self){publishRoots();return nativeOwners(self);};host.setHostHooks(hooks);}
  check(!old.expired());check(state.fired&&state.roots.size()==1024);
  check(!module.expired());check(state.host.lock()->get("extra").number()==42);
  state.roots.clear();state.owner.reset();check(old.expired()&&module.expired()&&state.host.expired());
  check(!JSValue::hasActiveHeap());allocationState=nullptr;
 }); REQUIRE(true);
}
TEST_CASE("Normal native ownership and trace hooks settle published ordinary and native graphs before sweep") {
 for(bool trace : {false,true}) isolated([&] {
  AllocationHookState state;allocationState=&state;auto realm=JSValue::object();
  auto owner=std::make_shared<Native>();auto host=wrap(owner);
  auto hooks=std::make_shared<JSHostObjectHooks>();
  if(trace){hooks->nativeOwnership=nativeOwners;hooks->trace=+[](const JSValue&){publishRoots();return std::vector<JSValue>{};};}
  else hooks->nativeOwnership=+[](const JSValue& self){publishRoots();return nativeOwners(self);};
  host.setHostHooks(hooks);realm.collectGarbage();
  check(state.fired&&state.roots.size()==1024);check(state.roots[0].get("index").number()==0);
  check(state.host.lock()->get("extra").number()==42);
  state.roots.clear();state.owner.reset();host={};owner.reset();realm={};check(state.host.expired());
  check(!JSValue::hasActiveHeap());allocationState=nullptr;
 }); REQUIRE(true);
}
TEST_CASE("Safepoint propagates allocation hook error without losing published roots or collection guard") {
 isolated([] {
  AllocationHookState state;state.throwAfter=true;allocationState=&state;JSWeakValue old;
  {auto owner=std::make_shared<Native>();auto host=wrap(owner);old=JSWeakValue(host);
   auto hooks=std::make_shared<JSHostObjectHooks>();hooks->nativeOwnership=+[](const JSValue& self){publishRoots();return nativeOwners(self);};host.setHostHooks(hooks);}
  bool caught=false;try{static_cast<void>(old.lock());}catch(const std::runtime_error& e){caught=std::string(e.what())=="published root then threw";}
  check(caught);check(state.roots.size()==1024);check(state.host.lock()->get("extra").number()==42);
  state.roots.clear();state.owner.reset();check(old.expired());check(!JSValue::hasActiveHeap());allocationState=nullptr;
 }); REQUIRE(true);
}
TEST_CASE("New host allocation retires prior empty native realm and restarts public prototypes") {
 isolated([] {
  for(unsigned n=0;n<8;++n){JSWeakValue old,prototype;
   {auto texture=nativeTexture(std::make_shared<Texture>());old=JSWeakValue(texture);
    auto root=JSValue::object();auto p=root.getPrototype();p.set("exitPoison",n);prototype=JSWeakValue(p);}
   auto fresh=JSValue::object();check(!fresh.has("exitPoison"));check(old.expired()&&prototype.expired());
  }
 }); REQUIRE(true);
}
