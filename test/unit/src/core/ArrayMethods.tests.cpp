#include "catch.hpp"
#include "core/JSValue.h"
TEST_CASE("Array sort observes comparator before receiver and preserves stable undefined ordering", "[runtime][arraysort]") {
 auto array=JSValue::array();auto sort=array.get("sort");REQUIRE(sort.get("length").number()==1);
 REQUIRE_THROWS_AS(sort.call(nullptr,{17}),JSTypeError);array.set("length",5);array.set("1",JSValue());array.set("3",2);array.set("4",1);
 auto comparator=JSValue::function([](const JSValue& self,const std::vector<JSValue>&a,const std::vector<JSValue>&)->JSValue{if(!self.isUndefined())throw JSTypeError("receiver");return a[0].number()-a[1].number();});
 REQUIRE(sort.call(array,{comparator}).strictEquals(array));REQUIRE(array.get("0").number()==1);REQUIRE(array.get("1").number()==2);
 REQUIRE(array.hasOwn("2"));REQUIRE(array.get("2").isUndefined());REQUIRE_FALSE(array.hasOwn("3"));REQUIRE_FALSE(array.hasOwn("4"));
}
TEST_CASE("Array unshift moves inherited indices and preserves sparse slots", "[runtime][arraysort]") {
 auto p=JSValue::object();p.set("1",7);auto o=JSValue::object(p);o.set("length",3);o.set("0",2);
 auto unshift=JSValue::array().get("unshift");REQUIRE(unshift.call(o,{8,9}).number()==5);
 REQUIRE(o.get("0").number()==8);REQUIRE(o.get("1").number()==9);REQUIRE(o.get("2").number()==2);REQUIRE(o.get("3").number()==7);REQUIRE_FALSE(o.hasOwn("4"));
}

TEST_CASE("Array sort keeps snapshot elements rooted through comparator GC and reentry", "[runtime][arraysort][gc]") {
 auto array=JSValue::array();std::vector<JSWeakValue> weak;
 for(int i:{3,1,2}){auto item=JSValue::object();item.set("rank",i);weak.emplace_back(item);array.get("push").call(array,{item});}
 auto state=JSValue::object();state.set("first",true);
 auto callback=JSValue::function([](const JSValue&,const std::vector<JSValue>&a,const std::vector<JSValue>&c)->JSValue{
  auto state=c[1];if(state.get("first").toBoolean()){state.set("first",false);auto o=c[0];o.set("length",0);o.collectGarbage();}
  return a[0].get("rank").number()-a[1].get("rank").number();
 },{array,state});
 array.get("sort").call(array,{callback});REQUIRE(array.length()==3);
 for(unsigned i=0;i<3;++i){REQUIRE(array.get(std::to_string(i)).get("rank").number()==i+1);REQUIRE_FALSE(weak[i].expired());}
 array.set("length",0);array.collectGarbage();for(const auto& value:weak)REQUIRE(value.expired());
}
TEST_CASE("Array sort validates before length and skips element access for short receivers", "[runtime][arraysort]") {
 auto state=JSValue::object();state.set("calls",0);auto o=JSValue::object();JSPropertyDescriptor length;
 length.getter=JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&c)->JSValue{auto s=c[0];s.set("calls",s.get("calls").number()+1);return 1;},{state});o.defineProperty("length",length);
 JSPropertyDescriptor item;item.getter=JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&)->JSValue{throw JSRangeError("unused getter");});o.defineProperty("0",item);
 auto sort=JSValue::array().get("sort");REQUIRE_THROWS_AS(sort.call(o,{17}),JSTypeError);REQUIRE(state.get("calls").number()==0);
 REQUIRE(sort.call(o).strictEquals(o));REQUIRE(state.get("calls").number()==1);
 auto text=sort.call("a");REQUIRE(text.get("0").string()=="a");REQUIRE_THROWS_AS(sort.call("ba"),JSTypeError);
}
