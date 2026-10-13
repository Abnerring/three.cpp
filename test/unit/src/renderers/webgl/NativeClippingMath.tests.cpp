#include "catch.hpp"
#include "parity/NativeClippingMathFixture.h"
using namespace ClippingMathFixture;

TEST_CASE("Native clipping math updates native Matrix3 and Plane storage", "[core][native][clipping]") {
    auto nativeMatrixValue=std::make_shared<Matrix3>();auto m=nativeMatrix(nativeMatrixValue);
    auto transform=matrix({2,0,0,0,0,3,0,0,0,0,4,0,1,2,3,1},4);
    REQUIRE(invoke(m,"getNormalMatrix",{transform}).sameValue(m));
    REQUIRE(nativeMatrixValue->elements()[0]==.5);REQUIRE(nativeMatrixValue->elements()[4]==1./3);REQUIRE(nativeMatrixValue->elements()[8]==.25);
    auto nativePlaneValue=std::make_shared<Plane>(Vector3(0,1,0),-2);auto p=nativePlane(nativePlaneValue),normal=p.get("normal");
    REQUIRE(invoke(p,"applyMatrix4",{transform,m}).sameValue(p));REQUIRE(p.get("normal").sameValue(normal));
    REQUIRE(nativePlaneValue->normal.x==0);REQUIRE(nativePlaneValue->normal.y==1);REQUIRE(nativePlaneValue->normal.z==0);REQUIRE(nativePlaneValue->constant==-8);
}
TEST_CASE("Borrowed Plane copy preserves values until arithmetic consumes them", "[core][native][clipping]") {
    auto state=JSValue::object();data(state,"log",array());data(state,"mode",0);
    auto x=observedNumber(state,"x",3),source=plane(vector({x,2,1}),-2),target=plane(vector(),0);
    invoke(target,"copy",{source});REQUIRE(state.get("log").length()==0);REQUIRE(target.get("normal").get("x").sameValue(x));
    auto point=vector();REQUIRE(invoke(target,"coplanarPoint",{point}).sameValue(point));REQUIRE(state.get("log").length()==1);
    REQUIRE(point.get("x").strictEquals(6));REQUIRE(point.get("y").strictEquals(4));REQUIRE(point.get("z").strictEquals(2));
}
TEST_CASE("Normal matrix chaining honors replacement methods and singular returns", "[core][native][clipping]") {
    const auto chain=chaining(1),singular=chaining(2),setFrom=chaining(0);
    REQUIRE(chain.at("result").at("value").get<std::string>()=="chain end");
    REQUIRE(singular.at("result").at("value").get<std::string>()=="singular result");
    REQUIRE(setFrom.at("result").at("value").get<std::string>()=="self");
}
TEST_CASE("Clipping math methods have class method descriptors", "[core][native][clipping]") {
    const auto m=nativeMatrix(std::make_shared<Matrix3>()),v=nativeVector(std::make_shared<Vector3>()),p=nativePlane(std::make_shared<Plane>());
    for(const auto& receiver:{m,v,p})for(const auto* name:{"setFromMatrix4","getNormalMatrix","invert","transpose","applyMatrix3","dot","coplanarPoint","applyMatrix4"}) {
        const auto fn=receiver.get(name);if(!fn.isCallable())continue;const auto descriptor=receiver.getPrototype().getOwnPropertyDescriptor(name);
        REQUIRE(descriptor);REQUIRE_FALSE(descriptor->enumerable.value_or(true));REQUIRE(descriptor->configurable.value_or(false));REQUIRE(descriptor->writable.value_or(false));REQUIRE_FALSE(fn.isConstructible());REQUIRE(fn.get("name").strictEquals(name));
    }
}
TEST_CASE("Public native Plane wrapper retains identity and collects alias cycles", "[core][native][clipping][gc]") {
    auto anchor=JSValue::object();JSWeakValue planeWeak,normalWeak;std::weak_ptr<Plane> nativeWeak;
    {auto owner=std::make_shared<Plane>();nativeWeak=owner;auto p=nativePlane(owner),normal=p.get("normal");REQUIRE(nativePlane(owner).sameValue(p));normal.set("owner",p);planeWeak=JSWeakValue(p);normalWeak=JSWeakValue(normal);anchor.collectGarbage();REQUIRE_FALSE(planeWeak.expired());}
    anchor.collectGarbage();REQUIRE(planeWeak.expired());REQUIRE(normalWeak.expired());REQUIRE(nativeWeak.expired());
}
TEST_CASE("Plane prototype access does not construct a temporary Plane", "[core][native][clipping]") {
    auto prototype=nativePlanePrototype(),state=JSValue::object();data(state,"calls",0);const auto previous=prototype.getOwnPropertyDescriptor("isPlane");
    JSPropertyDescriptor d;d.configurable=true;d.setter=JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue{auto state=captures[0];state.set("calls",state.get("calls").toNumber()+1);return {};},{state});prototype.defineProperty("isPlane",d);
    const auto same=nativePlanePrototype().sameValue(prototype);const auto countBefore=state.get("calls").number();auto managed=JSValue::object(nativePlanePrototype());managed.set("isPlane",true);const auto countAfter=state.get("calls").number();
    if(previous)prototype.defineProperty("isPlane",*previous);else prototype.erase("isPlane");
    REQUIRE(same);REQUIRE(countBefore==0);REQUIRE(countAfter==1);
}
TEST_CASE("Plane scratch initializes its matrix as an array literal in fresh runtimes", "[core][native][clipping]") {
    for(unsigned mode=0;mode<4;++mode){const auto result=initialization(mode);REQUIRE(result.at("initialized").get<bool>());REQUIRE(result.at("calls").get<double>()==0);REQUIRE(result.at("error").get<std::string>().empty());}
}
