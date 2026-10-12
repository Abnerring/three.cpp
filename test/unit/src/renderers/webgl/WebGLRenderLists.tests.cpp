#include "catch.hpp"
#include "renderers/webgl/WebGLRenderLists.h"
#include "core/JSWeakMap.h"
#include <thread>
#include <exception>
namespace {
JSValue listObject(int id){auto value=JSValue::object();value.set("id",id);value.set("renderOrder",0);return value;}
JSValue listMaterial(int id){auto value=listObject(id);value.set("transmission",0);value.set("transparent",false);return value;}
}
TEST_CASE("WebGL render lists retain pooled identities and release inactive item references", "[renderers][webgl][gc]") {
 WebGLRenderList list;auto object=listObject(3),material=listMaterial(9),camera=JSValue::object();
 list.push(object,JSValue::object(),material,0,3,JSValue::object(),camera);list.push(listObject(4),JSValue::object(),material,0,2,JSValue::object(),camera);
 auto first=list.opaque.get("0"),second=list.opaque.get("1"),array=list.opaque.value();
 list.init();REQUIRE(list.opaque.strictEquals(array));list.push(object,{},material,0,4,{},camera);
 REQUIRE(list.opaque.get("0").strictEquals(first));list.finish();REQUIRE(second.get("id").isNull());REQUIRE(second.get("object").isNull());
 REQUIRE(second.get("geometry").isNull());REQUIRE(second.get("material").isNull());REQUIRE(second.get("group").isNull());REQUIRE(second.get("z").number()==2);
}
TEST_CASE("WebGL render-list scene cache is an ephemeron with independent depth entries", "[renderers][webgl][gc]") {
 WebGLRenderLists cache;auto anchor=JSValue::object();JSWeakValue sceneWeak,listWeak;std::weak_ptr<int> ownerWeak;
 {auto owner=std::make_shared<int>(7);ownerWeak=owner;auto scene=JSValue::hostObject(owner,owner.get(),"renderlist.scene");sceneWeak=JSWeakValue(scene);
  auto zero=cache.get(scene,0);listWeak=JSWeakValue(zero);REQUIRE(cache.get(scene,0).strictEquals(zero));auto one=cache.get(scene,1);REQUIRE_FALSE(one.strictEquals(zero));
  zero.set("scene",scene);REQUIRE(cache.get(scene,1).strictEquals(one));anchor.collectGarbage();REQUIRE_FALSE(sceneWeak.expired());}
 anchor.collectGarbage();REQUIRE(sceneWeak.expired());REQUIRE(listWeak.expired());REQUIRE(ownerWeak.expired());
}
TEST_CASE("WebGL render list active item roots preserve resources only through finish", "[renderers][webgl][gc]") {
 WebGLRenderList list;auto anchor=JSValue::object();JSWeakValue objectWeak,geometryWeak,materialWeak,groupWeak;
 {auto object=listObject(2),geometry=JSValue::object(),material=listMaterial(1),group=JSValue::object();
  objectWeak=JSWeakValue(object);geometryWeak=JSWeakValue(geometry);materialWeak=JSWeakValue(material);groupWeak=JSWeakValue(group);
  list.push(object,geometry,material,0,0,group,JSValue::object());}
 list.init();anchor.collectGarbage();REQUIRE_FALSE(objectWeak.expired());list.finish();anchor.collectGarbage();
 REQUIRE(objectWeak.expired());REQUIRE(geometryWeak.expired());REQUIRE(materialWeak.expired());REQUIRE(groupWeak.expired());
}
TEST_CASE("WebGL render list detached closures preserve captured arrays after public reassignment", "[renderers][webgl]") {
 WebGLRenderList list;auto array=list.opaque.value();list.opaque=JSValue::array();auto push=list.object().get("push");
 push.call(nullptr,{listObject(1),{},listMaterial(1),0,2,{},JSValue::object()});REQUIRE(array.length()==1);REQUIRE(list.opaque.value().length()==0);
 list.object().get("init").call(JSValue(17));REQUIRE(array.length()==0);
}

TEST_CASE("WebGL render-list closure methods are ordinary constructors with mutable prototypes", "[renderers][webgl][construct]") {
 WebGLRenderList list;WebGLRenderLists cache;auto scene=JSValue::object(),first=cache.get(scene,0);
 auto alternate=JSValue::object();auto target=JSValue::constructor([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&)->JSValue{throw JSRangeError("newTarget body must not execute");},{},"Target",0,JSValue::ConstructorKind::Callable);target.set("prototype",alternate);
 for(const auto* key:{"init","push","unshift","finish","sort","get","dispose"}){
  auto fn=(std::string(key)=="get"||std::string(key)=="dispose"?cache.object():list.object()).get(key);
  REQUIRE(fn.isConstructible());auto d=fn.getOwnPropertyDescriptor("prototype");REQUIRE(d);REQUIRE(d->writable.value());REQUIRE_FALSE(d->enumerable.value());REQUIRE_FALSE(d->configurable.value());REQUIRE(fn.get("prototype").get("constructor").strictEquals(fn));
  std::vector<JSValue> args;if(std::string(key)=="push"||std::string(key)=="unshift")args={listObject(1),{},listMaterial(1),0,1,{},JSValue::object()};if(std::string(key)=="get")args={scene,0};
  auto result=fn.construct(args,target);REQUIRE(result.isObject());if(std::string(key)=="get")REQUIRE(result.strictEquals(first));else REQUIRE(result.getPrototype().strictEquals(alternate));
 }
 REQUIRE_FALSE(JSValue::array().get("sort").isConstructible());REQUIRE_FALSE(JSValue::array().get("unshift").isConstructible());
}

namespace {
JSValue capturedDefaultSort(bool reverse){
 WebGLRenderList list;for(int i=0;i<2;++i)list.push(listObject(i),i,listMaterial(i),0,i,i,JSValue::object());
 auto array=reverse?list.transmissive.value():list.opaque.value();if(reverse){array=list.transparent.value();for(int i=0;i<2;++i){auto m=listMaterial(i);m.set("transparent",true);list.push(listObject(i+3),i,m,0,i,i,JSValue::object());}}
 auto holder=JSValue::object();array.set("sort",JSValue::function([](const JSValue&,const std::vector<JSValue>&a,const std::vector<JSValue>&c)->JSValue{auto value=c[0];value.set("fn",a[0]);return {};},{holder}));list.sort();return holder.get("fn");
}
}
TEST_CASE("WebGL render-list module comparators preserve mutations through GC", "[renderers][webgl][gc]") {
 struct Result{bool mutated[2]{},samePrototype[2]{};std::exception_ptr error;} result;
 std::thread worker([&]{try{
  JSWeakValue weak[2];
  {auto anchor=JSValue::object();
   for(int i=0;i<2;++i){auto fn=capturedDefaultSort(i!=0);weak[i]=JSWeakValue(fn);fn.set("moduleMarker",123);auto prototype=fn.get("prototype");prototype.set("moduleMarker",456);}
   anchor.collectGarbage();
   for(int i=0;i<2;++i){auto fn=capturedDefaultSort(i!=0);result.mutated[i]=fn.get("moduleMarker").strictEquals(JSValue(123));result.samePrototype[i]=fn.get("prototype").get("moduleMarker").strictEquals(JSValue(456));}
  }
  for(int i=0;i<2;++i){auto fn=capturedDefaultSort(i!=0);fn.erase("moduleMarker");auto prototype=fn.get("prototype");prototype.erase("moduleMarker");}
 }catch(...){result.error=std::current_exception();}});worker.join();
 REQUIRE_FALSE(result.error);for(int i=0;i<2;++i){REQUIRE(result.mutated[i]);REQUIRE(result.samePrototype[i]);}
}
