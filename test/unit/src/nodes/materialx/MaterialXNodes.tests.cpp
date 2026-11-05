#include "../../../utils/parity/MaterialXNativeFixtures.h"
#include "catch.hpp"
using namespace MaterialXNativeFixtures;
namespace {
JSValue spec(const char* type,const Args& a={.371}){auto out=object();out.set("node",type);out.set("args",array(a));return out;}
JSValue request(const char* name,const Args& args,const char* kind="function"){auto out=object();out.set("module","MaterialXNodes");out.set("mode","body");out.set("kind",kind);out.set("fn",name);out.set("args",array(args));return out;}
unsigned count(const JSValue& snapshot,const char* type){unsigned n=0;for(const auto& row:values(snapshot.get("objects")))if(row.get("type").strictEquals(type))++n;return n;}
JSValue get(const JSValue& snapshot,const JSValue& ref,const char* key){for(const auto& p:values(snapshot.get("objects").get(ref.get("1")).get("properties")))if(p.get("0").strictEquals(key))return p.get("1");return {};}
unsigned controlsCount(const JSValue& snapshot){return unsigned(snapshot.get("objects").get(get(snapshot,snapshot.get("root"),"controls").get("1")).get("length").toNumber());}
}
TEST_CASE("MaterialXNodes exports the complete pinned namespace and preserves imported identities"){
    const auto m=nativeMaterialXNodesModule();REQUIRE(m.keys().size()==49);REQUIRE(declarations().get("MaterialXNodes").length()==49);
    for(const auto* name:{"mx_rotate2d","mx_rotate3d"})REQUIRE(m.get(name).sameValue(nativeMaterialXCoreBinding(name)));
    for(const auto* name:{"mx_hsvtorgb","mx_rgbtohsv"})REQUIRE(m.get(name).sameValue(nativeMaterialXColorBinding(name)));
    REQUIRE(m.get("mx_srgb_texture_to_lin_rec709").sameValue(nativeMaterialXColorTransformBinding("mx_srgb_texture_to_lin_rec709")));
    REQUIRE(m.get("mx_noise_float").get("length").strictEquals(0));REQUIRE(m.get("mx_unifiednoise3d").get("length").strictEquals(1));REQUIRE(m.get("mx_place2d").get("length").strictEquals(1));
    REQUIRE(nativeMaterialXNodesPrivate("_ramp").get("length").strictEquals(4));REQUIRE(nativeMaterialXNodesPrivate("_split").get("length").strictEquals(5));
}
TEST_CASE("MaterialXNodes separate implements channel aliases numeric indices and unchanged fallback identity"){
    StackScope scope;const auto input=node("vec4",{.1,.2,.3,.4}),f=nativeMaterialXNodesBinding("mx_separate");
    for(const auto& [channel,index]:std::vector<std::pair<JSValue,int>>{{"outR",0},{"x",0},{"outg",1},{"Y",1},{"outZ",2},{"b",2},{"outW",3},{"A",3},{0,0},{7,7},{-1,-1}}){const auto result=f.call({}, {input,channel});REQUIRE(result.get("node").sameValue(input));REQUIRE(result.get("indexNode").get("value").strictEquals(index));}
    for(const auto& channel:Args{nullptr,"invalid","OUTX","out",JSValue{}})REQUIRE(f.call({}, {input,channel}).sameValue(input));
}
TEST_CASE("MaterialXNodes place2d uses numeric epsilon and emits dynamic operation order selection"){
    const auto evaluate=[](const JSValue& order){StackScope scope;const auto result=nativeMaterialXNodesBinding("mx_place2d").call({}, {node("vec2",{.2,.3}),node("vec2",{.1,.2}),node("vec2",{2.,3.}),node("float",{.5}),node("vec2",{.4,.5}),order});return Graph()(result).toJSON()->dump();};
    const auto zero=evaluate(0),nonzero=evaluate(1);REQUIRE(zero!=nonzero);REQUIRE(evaluate(std::numeric_limits<double>::epsilon())==zero);REQUIRE(evaluate(std::numeric_limits<double>::epsilon()*2)==nonzero);REQUIRE(evaluate(-1)==nonzero);REQUIRE(evaluate(std::numeric_limits<double>::quiet_NaN())==zero);
    const auto dynamic=run(request("mx_place2d",{spec("vec2"),spec("vec2"),spec("vec2"),spec("float"),spec("vec2"),spec("int",{1})}));REQUIRE(dynamic.get("error").isUndefined());REQUIRE(count(dynamic,"MathNode")>=5);
}
TEST_CASE("MaterialXNodes place2d consumes the immutable Number EPSILON intrinsic"){
    const auto number=nativeGlobalBinding("Number");REQUIRE(number.get("EPSILON").strictEquals(std::numeric_limits<double>::epsilon()));
    const auto descriptor=number.getOwnPropertyDescriptor("EPSILON");REQUIRE(descriptor);REQUIRE(descriptor->value->strictEquals(std::numeric_limits<double>::epsilon()));REQUIRE(descriptor->writable==false);REQUIRE(descriptor->enumerable==false);REQUIRE(descriptor->configurable==false);
    const auto successor=1.+number.get("EPSILON").toNumber();REQUIRE(successor==std::nextafter(1.,2.));REQUIRE(1.+number.get("EPSILON").toNumber()/2==1.);
}
TEST_CASE("MaterialXNodes place2d preserves observable operand lookup conversion and reentry order"){
    const auto success=globalOrder("success");REQUIRE(success.get("result").strictEquals("success"));REQUIRE(success.get("events").toJSON()->dump()=="[[\"get:abs\",true],[\"call:abs\",true,1],[\"get:EPSILON\",true]]");
    const auto coerce=globalOrder("coerce");REQUIRE(coerce.get("result").strictEquals("success"));REQUIRE(coerce.get("events").toJSON()->dump()=="[[\"get:abs\",true],[\"call:abs\",true,1],[\"get:EPSILON\",true],\"get:left:valueOf\",\"call:left:valueOf\",\"get:right:valueOf\",\"call:right:valueOf\"]");
    for(const auto& [variant,reason]:std::vector<std::pair<const char*,const char*>>{{"getAbsThrow","get:abs"},{"callAbsThrow","call:abs"},{"epsilonThrow","get:EPSILON"},{"leftCoerceThrow","left:valueOf"},{"rightCoerceThrow","right:valueOf"}}){const auto out=globalOrder(variant);REQUIRE(out.get("result").get("0").strictEquals("throw"));REQUIRE(out.get("result").get("1").strictEquals(reason));REQUIRE(out.get("graph").isUndefined());}
    const auto reentered=globalOrder("reenter");REQUIRE(reentered.get("result").strictEquals("success"));REQUIRE(reentered.get("events").get("2").strictEquals("reenter"));REQUIRE(reentered.get("events").get("3").get("0").strictEquals("get:EPSILON"));
}
TEST_CASE("MaterialXNodes smoothstep and height conversion preserve full shader algorithms"){
    const auto smooth=run(request("mx_smoothstep",{spec("float"),spec("float"),spec("float")}));REQUIRE(smooth.get("error").isUndefined());REQUIRE(count(smooth,"MathNode")==6);REQUIRE(count(smooth,"OperatorNode")==7);
    const auto normal=run(request("mx_heighttonormal",{spec("float"),spec("float"),spec("vec2",{.2,.3})}));REQUIRE(normal.get("error").isUndefined());REQUIRE(count(normal,"MathNode")==11);REQUIRE(count(normal,"JoinNode")==5);REQUIRE(count(normal,"SplitNode")==9);
}
TEST_CASE("MaterialXNodes exposes real color conversions and every deferred color branch"){
    const auto input=spec("vec3",{.173,.346,.519});
    const auto hsv=run(request("mx_hsvtorgb",{input},"Fn"));REQUIRE(hsv.get("error").isUndefined());REQUIRE(controlsCount(hsv)==8);REQUIRE(count(hsv,"AssignNode")==7);REQUIRE(count(hsv,"ConditionalNode")==6);
    const auto rgb=run(request("mx_rgbtohsv",{input},"Fn"));REQUIRE(rgb.get("error").isUndefined());REQUIRE(controlsCount(rgb)==8);REQUIRE(count(rgb,"AssignNode")==9);REQUIRE(count(rgb,"ConditionalNode")==5);
    const auto transform=run(request("mx_srgb_texture_to_lin_rec709",{input},"Fn"));REQUIRE(transform.get("error").isUndefined());REQUIRE(count(transform,"MathNode")==3);REQUIRE(count(transform,"OperatorNode")==4);
    for(const auto* name:{"mx_hsvtorgb","mx_rgbtohsv","mx_srgb_texture_to_lin_rec709"})REQUIRE(nativeMaterialXNodesBinding(name).get("shaderNode").get("jsFunc").get("name").strictEquals(""));
}
