#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "math/ColorManagement.h"
#include "math/Color.h"
#include "math/Matrix3.h"
#include "math/Vector3.h"
#include "core/NativeObject.h"
#include "core/JSObject.h"
#include "renderers/webgl/WebGLUtils.h"
#include "constants.h"
#include <thread>
namespace {
using Args=std::vector<JSValue>;
JSPropertyDescriptor data(JSValue value,bool writable=true,bool enumerable=true,bool configurable=true){JSPropertyDescriptor d;d.value=value;d.writable=writable;d.enumerable=enumerable;d.configurable=configurable;return d;}
struct Restore{JSValue object;std::vector<std::pair<std::string,JSPropertyDescriptor>> fields;Restore(JSValue o,std::initializer_list<const char*> keys):object(o){for(auto key:keys)fields.emplace_back(key,*o.getOwnPropertyDescriptor(key));}~Restore(){for(auto& [key,descriptor]:fields)object.defineProperty(key,descriptor);}};
JSValue call(const JSValue& o,const char* name,const Args& args={}){return o.get(name).call(o,args);}
}
TEST_CASE("ColorManagement owns the source defaults and shared module matrices") {
    auto cm=nativeColorManagement();REQUIRE(cm.get("enabled").sameValue(true));REQUIRE(cm.get("workingColorSpace").sameValue("srgb-linear"));
    auto spaces=cm.get("spaces"),linear=spaces.get("srgb-linear"),srgb=spaces.get("srgb");
    for(auto key:{"primaries","whitePoint","luminanceCoefficients","toXYZ","fromXYZ"})REQUIRE(linear.get(key).sameValue(srgb.get(key)));
    REQUIRE_FALSE(linear.get("outputColorSpaceConfig").sameValue(srgb.get("outputColorSpaceConfig")));
    REQUIRE_FALSE(srgb.hasOwn("workingColorSpaceConfig"));
    REQUIRE(call(cm,"getTransfer",{""}).sameValue("linear"));REQUIRE(call(cm,"getToneMappingMode",{"srgb"}).sameValue("standard"));
    REQUIRE(call(cm,"_getDrawingBufferColorSpace",{"srgb"}).sameValue("srgb"));REQUIRE(call(cm,"_getUnpackColorSpace").sameValue("srgb"));
    for(auto key:{"enabled","workingColorSpace","spaces","convert","workingToColorSpace","colorSpaceToWorking","getPrimaries","getTransfer","getToneMappingMode","getLuminanceCoefficients","define","_getMatrix","_getDrawingBufferColorSpace","_getUnpackColorSpace"}){
        auto d=*cm.getOwnPropertyDescriptor(key);REQUIRE(d.writable.value());REQUIRE(d.enumerable.value());REQUIRE(d.configurable.value());
    }
    for(auto name:{"convert","workingToColorSpace","colorSpaceToWorking","getPrimaries","getTransfer","getToneMappingMode","getLuminanceCoefficients","define","_getMatrix","_getDrawingBufferColorSpace","_getUnpackColorSpace"}){
        auto method=cm.get(name);REQUIRE(method.isConstructible());REQUIRE(method.get("prototype").get("constructor").sameValue(method));REQUIRE(method.get("name").sameValue(name));
    }
    Restore restore(cm,{"spaces"});JSWeakValue matrix,oldPrimaries;
    {matrix=JSWeakValue(linear.get("toXYZ"));oldPrimaries=JSWeakValue(linear.get("primaries"));}
    cm.set("spaces",JSValue::object());linear={};srgb={};spaces={}; // restore intentionally keeps old spaces alive.
    cm.collectGarbage();REQUIRE(matrix.lock().has_value());REQUIRE(oldPrimaries.lock().has_value());REQUIRE(nativeColorManagement().sameValue(cm));
}
TEST_CASE("ColorManagement live state and copied native fields share one authority") {
    auto cm=nativeColorManagement();Restore restore(cm,{"enabled","workingColorSpace"});
    const auto previous=ColorManagement::workingColorSpace;const auto previousEnabled=ColorManagement::enabled;
    cm.set("workingColorSpace","srgb");REQUIRE(std::string(ColorManagement::workingColorSpace)=="srgb");
    ColorManagement::workingColorSpace="srgb-linear";REQUIRE(cm.get("workingColorSpace").sameValue("srgb-linear"));
    ColorManagement::enabled=0;REQUIRE(cm.get("enabled").sameValue(0));
    auto color=nativeColor(std::make_shared<Color>(.5,.25,.75));call(cm,"convert",{color,"srgb","srgb-linear"});REQUIRE(color.get("r").number()==Approx(SRGBToLinear(.5)));
    cm.set("enabled",false);call(cm,"convert",{color,"srgb-linear","srgb"});REQUIRE(color.get("r").number()==Approx(SRGBToLinear(.5)));
    ColorManagement::workingColorSpace=previous;ColorManagement::enabled=previousEnabled;
    REQUIRE(cm.get("workingColorSpace").sameValue(previous.value()));REQUIRE(cm.get("enabled").sameValue(previousEnabled.value()));
}
TEST_CASE("ColorManagement performs native primary matrices and luminance methods") {
    auto cm=nativeColorManagement();Restore restore(cm,{"spaces"});auto spaces=JSValue::object();
    auto source=JSValue::object(),target=JSValue::object();auto first=nativeMatrix(std::make_shared<Matrix3>(2,0,0,0,3,0,0,0,4)),second=nativeMatrix(std::make_shared<Matrix3>(5,0,0,0,6,0,0,0,7));
    source.set("transfer","linear");target.set("transfer","linear");source.set("primaries",JSValue::array());target.set("primaries",JSValue::array());source.set("toXYZ",first);target.set("fromXYZ",second);
    spaces.set("from",source);spaces.set("to",target);cm.set("spaces",spaces);
    Color color(.2,.3,.4);ColorManagement::convert(color,"from","to");REQUIRE(color.r==Approx(2));REQUIRE(color.g==Approx(5.4));REQUIRE(color.b==Approx(11.2));
    auto matrix=nativeMatrix(std::make_shared<Matrix3>());REQUIRE(call(cm,"_getMatrix",{matrix,"from","to"}).sameValue(matrix));REQUIRE(matrix.get("elements").get("0").number()==10);
    auto coefficients=JSValue::array();coefficients.set("0",1);coefficients.set("1",2);coefficients.set("2",3);source.set("luminanceCoefficients",coefficients);
    auto v=nativeVector(std::make_shared<Vector3>());REQUIRE(call(cm,"getLuminanceCoefficients",{v,"from"}).sameValue(v));REQUIRE(v.get("z").number()==3);
}
TEST_CASE("Native Color calls retire live borrowed views on return and throw") {
    auto cm=nativeColorManagement();Restore restore(cm,{"convert"});auto state=JSValue::object();
    cm.set("convert",JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue {
        auto state=captures[0];state.set("view",args[0]);state.set("receiver",self);auto view=args[0];view.set("r",.875);
        if(captures[0].get("throw").toBoolean())throw JSRangeError("callback");return view;
    },{state}));
    Color color(.1,.2,.3);REQUIRE(&ColorManagement::convert(color,"a","b")==&color);REQUIRE(color.r==.875);REQUIRE(state.get("receiver").sameValue(cm));
    auto oldView=state.get("view");REQUIRE_THROWS_AS(oldView.get("r"),JSTypeError);REQUIRE_THROWS_AS(oldView.set("g",5),JSTypeError);
    state.set("throw",true);REQUIRE_THROWS_AS(ColorManagement::convert(color,"a","b"),JSRangeError);auto newView=state.get("view");REQUIRE_THROWS_AS(newView.get("b"),JSTypeError);REQUIRE_FALSE(oldView.sameValue(newView));
}
TEST_CASE("Object.assign uses intrinsic binding and source ordered partial writes") {
    auto object=JSValue::intrinsicObjectConstructor();auto assign=object.get("assign");REQUIRE_FALSE(assign.isConstructible());REQUIRE(assign.get("length").sameValue(2));
    auto target=JSValue::object(),source=JSValue::object(),trace=JSValue::array();auto symbol=JSValue::symbol("key");
    source.set("first",1);source.set("second",2);source.set(symbol,3);
    JSPropertyDescriptor get;get.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{auto s=self;s.erase("second");s.set("new",4);return 7;});get.enumerable=get.configurable=true;source.defineProperty("first",get);
    REQUIRE(assign.call(object,{target,source}).sameValue(target));REQUIRE(target.get("first").sameValue(7));REQUIRE_FALSE(target.hasOwn("second"));REQUIRE_FALSE(target.hasOwn("new"));REQUIRE(target.get(symbol).sameValue(3));
    auto partial=JSValue::object(),input=JSValue::object();input.set("a",11);input.set("b",12);input.set("c",13);partial.defineProperty("b",data(0,false));
    REQUIRE_THROWS_AS(assign.call(object,{partial,input}),JSTypeError);REQUIRE(partial.get("a").sameValue(11));REQUIRE(partial.get("b").sameValue(0));REQUIRE_FALSE(partial.hasOwn("c"));
    auto stringTarget=assign.call(object,{"abc",JSValue::object()});REQUIRE(stringTarget.get("length").sameValue(3));REQUIRE_THROWS_AS(assign.call(object,{3,input}),JSTypeError);
    auto stringSource=assign.call(object,{JSValue::object(),"a\xf0\x9f\x98\x80"});REQUIRE(stringSource.keys().size()==3);
    auto prototype=JSValue::object().getPrototype();Restore restore(prototype,{"constructor"});prototype.set("constructor",17);REQUIRE(JSValue::intrinsicObjectConstructor().sameValue(object));
    auto cm=nativeColorManagement();Restore cmRestore(cm,{"spaces"});cm.set("spaces",JSValue::object());call(cm,"define",{input});REQUIRE(cm.get("spaces").get("c").sameValue(13));
}
TEST_CASE("Module and Object.assign reject foreign heap operations before effects") {
    auto cm=nativeColorManagement(),object=JSValue::intrinsicObjectConstructor();auto method=cm.get("getTransfer");bool rejected=false;
    std::thread thread([&]{try{method.call(cm,{"srgb"});}catch(const std::exception&){rejected=true;}});thread.join();REQUIRE(rejected);
    bool assignRejected=false;auto assign=object.get("assign");std::thread another([&]{try{assign.call(object,{JSValue::object(),cm});}catch(const std::exception&){assignRejected=true;}});another.join();REQUIRE(assignRejected);
}
TEST_CASE("WebGLUtils uses live ColorManagement before GPU enum lookup") {
    auto cm=nativeColorManagement();Restore restore(cm,{"getTransfer"});auto trace=JSValue::array(),gl=JSValue::object(),extensions=JSValue::object();
    cm.set("getTransfer",JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue{auto trace=captures[0];trace.set("0","transfer");return "linear";},{trace}));
    JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue{auto trace=captures[0];trace.set("1","enum");return 99;},{trace});gl.defineProperty("UNSIGNED_BYTE",d);
    WebGLUtils utils(gl,extensions);REQUIRE(utils.convert(UnsignedByteType).sameValue(99));REQUIRE(trace.get("0").sameValue("transfer"));REQUIRE(trace.get("1").sameValue("enum"));
    cm.set("getTransfer",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("transfer");}));REQUIRE_THROWS_AS(utils.convert(UnsignedByteType),JSRangeError);
}
