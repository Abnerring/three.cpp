#include "catch.hpp"
#include "core/NativeObject.h"
#include "math/Color.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Matrix4.h"
namespace {using Args=std::vector<JSValue>;}
TEST_CASE("Native math methods preserve live storage and class method metadata", "[core][native][math]"){
 auto v=std::make_shared<Vector3>(1,2,3);auto object=nativeVector(v);auto scalar=object.get("multiplyScalar");REQUIRE_FALSE(scalar.isConstructible());REQUIRE(scalar.get("name").strictEquals("multiplyScalar"));REQUIRE(scalar.get("length").number()==1);REQUIRE(scalar.call(object,{2}).sameValue(object));REQUIRE(v->x==2);REQUIRE(v->y==4);REQUIRE(v->z==6);
 auto c=std::make_shared<Color>(.2,.4,.6);auto color=nativeColor(c);color.get("multiplyScalar").call(color,{2});REQUIRE(c->r==.4);REQUIRE(c->g==.8);REQUIRE(c->b==1.2);
}
TEST_CASE("Borrowed native math copy uses its prototype components and preserves partial writes", "[core][native][math]"){
 auto fn=nativeColor(std::make_shared<Color>()).get("copy"),receiver=JSValue::object(),source=JSValue::object();source.set("r",2);source.set("g",3);source.set("b",4);REQUIRE(fn.call(receiver,{source}).sameValue(receiver));REQUIRE(receiver.get("r").number()==2);REQUIRE(receiver.get("g").number()==3);REQUIRE(receiver.get("b").number()==4);REQUIRE_FALSE(receiver.has("x"));
 JSPropertyDescriptor d;d.getter=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("g");});d.configurable=true;source.defineProperty("g",d);source.set("r",9);REQUIRE_THROWS_AS(fn.call(receiver,{source}),JSRangeError);REQUIRE(receiver.get("r").number()==9);REQUIRE(receiver.get("g").number()==3);REQUIRE(receiver.get("b").number()==4);
}
TEST_CASE("Native vector normalization dynamically resolves overridden methods", "[core][native][math]"){
 auto receiver=JSValue::object(),trace=JSValue::array();receiver.set("length",JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{auto t=c[0];t.set(std::to_string(t.length()),self.sameValue(c[1]));return 0;},{trace,receiver}));receiver.set("divideScalar",JSValue::function([](const JSValue& self,const Args& a,const Args& c)->JSValue{auto t=c[0];t.set(std::to_string(t.length()),a[0]);return self;},{trace}));auto normalize=nativeVector(std::make_shared<Vector3>()).get("normalize");REQUIRE(normalize.call(receiver).sameValue(receiver));REQUIRE(trace.get("0").strictEquals(true));REQUIRE(trace.get("1").number()==1);
}
TEST_CASE("Native matrix extraction respects dynamic singular and identity protocols", "[core][native][math]"){
 auto receiver=JSValue::object(),source=JSValue::object(),trace=JSValue::array();source.set("determinantAffine",JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{auto t=c[0];t.set(std::to_string(t.length()),self.sameValue(c[1]));return -0.;},{trace,source}));receiver.set("identity",JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{auto t=c[0];t.set(std::to_string(t.length()),self.sameValue(c[1]));return 37;},{trace,receiver}));auto fn=nativeMatrix(std::make_shared<Matrix4>()).get("extractRotation");REQUIRE(fn.call(receiver,{source}).number()==37);REQUIRE(trace.length()==2);REQUIRE(trace.get("0").strictEquals(true));REQUIRE(trace.get("1").strictEquals(true));
}
