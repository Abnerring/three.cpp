#include "catch.hpp"
#include "core/JSPromise.h"
#include "loaders/ObjectLoaderShapes.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include <utility>

namespace {
using Args=std::vector<JSValue>;
JSValue invoke(const JSValue& self,const char* key,const Args& args={}){return self.get(key).call(self,args);}
JSValue pathClass(){return nativeObjectLoaderShapeConstructor().getPrototype();}
JSValue curvePathClass(){return pathClass().getPrototype();}
JSValue array(const Args& values={}){auto out=JSValue::array();for(std::size_t i=0;i<values.size();++i)out.set(JSValue(i),values[i]);return out;}
JSValue vec2(double x=0,double y=0){return nativeVector2Constructor().construct({x,y});}
struct PropertyGuard {
    JSValue object,key;std::optional<JSPropertyDescriptor> descriptor;
    PropertyGuard(JSValue value,JSValue property):object(value),key(property),descriptor(value.getOwnPropertyDescriptor(property)){}
    ~PropertyGuard(){if(descriptor)object.defineProperty(key,*descriptor);else object.erase(key);}
};
}

TEST_CASE("Native CurvePath keeps its length cache until explicit invalidation", "[extras][CurvePath][runtime]") {
    auto path=curvePathClass().construct(),state=JSValue::object(),segment=JSValue::object();state.set("length",3);state.set("calls",0);
    segment.set("getLength",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{auto state=c[0];state.set("calls",state.get("calls").toNumber()+1);return state.get("length");},{state}));
    REQUIRE(invoke(path,"add",{segment}).isUndefined());
    const auto first=invoke(path,"getCurveLengths");REQUIRE(first.get("0").strictEquals(3));REQUIRE(state.get("calls").strictEquals(1));
    state.set("length",9);path.set("needsUpdate",true);
    REQUIRE(invoke(path,"getCurveLengths").sameValue(first));REQUIRE(invoke(path,"getLength").strictEquals(3));REQUIRE(state.get("calls").strictEquals(1));
    REQUIRE(invoke(path,"updateArcLengths").isUndefined());const auto second=invoke(path,"getCurveLengths");
    REQUIRE_FALSE(second.sameValue(first));REQUIRE(second.get("0").strictEquals(9));REQUIRE(state.get("calls").strictEquals(2));REQUIRE(path.get("needsUpdate").strictEquals(true));
}

TEST_CASE("Native CurvePath forwards original targets and handles zero length segments", "[extras][CurvePath][runtime]") {
    auto path=curvePathClass().construct(),segment=JSValue::object(),target=JSValue::object(),state=JSValue::object();
    segment.set("getLength",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return 0;}));
    segment.set("getPointAt",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{auto state=c[0];state.set("receiver",self.sameValue(c[1]));state.set("u",a[0]);return a[1];},{state,segment}));
    invoke(path,"add",{segment});REQUIRE(invoke(path,"getPoint",{.5,target}).sameValue(target));REQUIRE(state.get("receiver").strictEquals(true));REQUIRE(state.get("u").strictEquals(0));
    REQUIRE(invoke(path,"getPoint",{2,target}).sameValue(target));
    auto empty=curvePathClass().construct();REQUIRE(invoke(empty,"getLength").isUndefined());REQUIRE(invoke(empty,"getPoint",{.5}).isNull());
}

TEST_CASE("Native CurvePath closes 3D paths with the sampled endpoint identities", "[extras][CurvePath][runtime]") {
    auto path=curvePathClass().construct(),a=nativeVector3Constructor().construct({1,2,3}),b=nativeVector3Constructor().construct({4,5,6});
    auto line=nativeObjectLoaderCurveConstructors().get("LineCurve3").construct({a,b}),samples=JSValue::object();const auto getPoint=line.get("getPoint");
    line.set("getPoint",JSValue::function(+[](const JSValue& receiver,const Args& args,const Args& captures)->JSValue{
        const auto result=captures[0].call(receiver,args);auto samples=captures[1];samples.set(args[0].strictEquals(0)?"start":"end",result);return result;
    },{getPoint,samples}));invoke(path,"add",{line});
    REQUIRE(invoke(path,"closePath").sameValue(path));const auto curves=path.get("curves"),closing=curves.get("1");
    REQUIRE(curves.length()==2);REQUIRE(closing.get("isLineCurve3").strictEquals(true));
    REQUIRE_FALSE(samples.get("end").sameValue(b));REQUIRE_FALSE(samples.get("start").sameValue(a));
    REQUIRE(closing.get("v1").sameValue(samples.get("end")));REQUIRE(closing.get("v2").sameValue(samples.get("start")));
    invoke(path,"closePath");REQUIRE(curves.length()==2);
}

TEST_CASE("Native Path spline keeps supplied point aliases and its current point identity", "[extras][Path][runtime]") {
    auto path=pathClass().construct(),current=path.get("currentPoint"),a=vec2(3,4),b=vec2(5,6);invoke(path,"moveTo",{1,2});
    REQUIRE(invoke(path,"splineThru",{array({a,b})}).sameValue(path));const auto spline=path.get("curves").get("0"),points=spline.get("points");
    REQUIRE(spline.get("isSplineCurve").strictEquals(true));REQUIRE_FALSE(points.get("0").sameValue(current));REQUIRE(points.get("0").get("x").strictEquals(1));REQUIRE(points.get("0").get("y").strictEquals(2));
    REQUIRE(points.get("1").sameValue(a));REQUIRE(points.get("2").sameValue(b));REQUIRE(path.get("currentPoint").sameValue(current));REQUIRE(current.get("x").strictEquals(5));REQUIRE(current.get("y").strictEquals(6));
    b.set("x",12);REQUIRE(points.get("2").get("x").strictEquals(12));REQUIRE(current.get("x").strictEquals(5));
}

TEST_CASE("Native Path retains an appended ellipse when its endpoint callback throws", "[extras][Path][runtime]") {
    auto path=pathClass().construct();invoke(path,"moveTo",{7,8});auto proto=nativeObjectLoaderCurveConstructors().get("EllipseCurve").get("prototype");PropertyGuard restore(proto,"getPoint");
    proto.set("getPoint",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSThrownValue("endpoint");}));
    REQUIRE_THROWS_AS(invoke(path,"absellipse",{0,0,2,3,0,1,false,0}),JSThrownValue);
    REQUIRE(path.get("curves").length()==1);REQUIRE(path.get("curves").get("0").get("isEllipseCurve").strictEquals(true));REQUIRE(path.get("currentPoint").get("x").strictEquals(7));REQUIRE(path.get("currentPoint").get("y").strictEquals(8));
}

TEST_CASE("Native Shape copies curves and holes independently and preserves target UUID", "[extras][Shape][runtime]") {
    auto source=nativeObjectLoaderShapeConstructor().construct({array({vec2(0,0),vec2(3,0),vec2(3,3)})}),hole=pathClass().construct({array({vec2(1,1),vec2(1,2),vec2(2,1)})});
    invoke(source,"closePath");invoke(hole,"closePath");source.get("holes").set("0",hole);source.set("uuid","source");auto target=nativeObjectLoaderShapeConstructor().construct();target.set("uuid","target");
    REQUIRE(invoke(target,"copy",{source}).sameValue(target));REQUIRE(target.get("uuid").strictEquals("target"));
    REQUIRE_FALSE(target.get("curves").sameValue(source.get("curves")));REQUIRE_FALSE(target.get("curves").get("0").sameValue(source.get("curves").get("0")));REQUIRE_FALSE(target.get("holes").sameValue(source.get("holes")));REQUIRE_FALSE(target.get("holes").get("0").sameValue(hole));
    REQUIRE_FALSE(target.get("holes").get("0").get("curves").get("0").sameValue(hole.get("curves").get("0")));
    REQUIRE(invoke(target,"toJSON").get("holes").length()==1);const auto json=invoke(source,"toJSON");REQUIRE(invoke(target,"fromJSON",{json}).sameValue(target));REQUIRE(target.get("uuid").strictEquals("source"));
}

TEST_CASE("Native Shape extraction preserves dynamic returned arrays and division token", "[extras][Shape][runtime]") {
    auto shape=nativeObjectLoaderShapeConstructor().construct(),token=JSValue::object(),outline=array({1}),holes=array({2}),state=JSValue::object();state.set("calls",array());
    for(auto [key,returned]:std::vector<std::pair<const char*,JSValue>>{{"getPoints",outline},{"getPointsHoles",holes}})shape.set(key,JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
        if(!self.sameValue(c[0])||!a[0].sameValue(c[1]))throw std::logic_error("receiver or divisions identity lost");auto calls=c[2].get("calls");calls.set(JSValue(calls.length()),c[3]);return c[4];
    },{shape,token,state,key,returned}));
    const auto result=invoke(shape,"extractPoints",{token});REQUIRE(result.get("shape").sameValue(outline));REQUIRE(result.get("holes").sameValue(holes));REQUIRE(state.get("calls").get("0").strictEquals("getPoints"));REQUIRE(state.get("calls").get("1").strictEquals("getPointsHoles"));
}
