#include "../../../utils/parity/MaterialXNativeFixtures.h"
#include "catch.hpp"
using namespace MaterialXNativeFixtures;
namespace {
JSValue spec(const char* type,const Args& a={.371}){auto out=object();out.set("node",type);out.set("args",array(a));return out;}
JSValue request(const char* name,const Args& a,bool build=false){auto out=object();out.set("module","MaterialXNoise");out.set("mode","body");out.set("kind","Fn");out.set("fn",name);out.set("args",array(a));out.set("build",build);return out;}
JSValue property(const JSValue& snapshot,const JSValue& ref,const char* key){const auto row=snapshot.get("objects").get(ref.get("1"));for(const auto& p:values(row.get("properties")))if(p.get("0").strictEquals(key))return p.get("1");return {};}
JSValue rootProperty(const JSValue& snapshot,const char* key){return property(snapshot,snapshot.get("root"),key);}
unsigned count(const JSValue& snapshot,const char* type){unsigned n=0;for(const auto& row:values(snapshot.get("objects")))if(row.get("type").strictEquals(type))++n;return n;}
unsigned length(const JSValue& snapshot,const char* key){return unsigned(snapshot.get("objects").get(rootProperty(snapshot,key).get("1")).get("length").toNumber());}
Args typed(const JSValue& declaration){Args out;for(const auto& field:values(declaration.get("layout"))){if(field.get("0").strictEquals("return"))continue;const auto type=field.get("1").toString();out.push_back(node(type,{type=="bool"?JSValue(true):JSValue(2)}));}return out;}
JSValue find(const JSValue& value,const char* type){Args seen;std::function<JSValue(const JSValue&)> visit=[&](const JSValue& v)->JSValue{if(!v.isObject()||v.isCallable())return {};for(const auto& old:seen)if(old.sameValue(v))return {};seen.push_back(v);if(v.get("type").strictEquals(type))return v;for(const auto& key:v.keys())if(key!="stackTrace"){const auto found=visit(v.get(key));if(!found.isUndefined())return found;}return {};};return visit(value);}
}
TEST_CASE("MaterialXNoise exposes all pinned exports and lexical helper definitions"){
    const auto exports=nativeMaterialXNoiseModule(),all=declarations().get("MaterialXNoise");REQUIRE(exports.keys().size()==74);REQUIRE(all.length()==78);
    unsigned fnCount=0,overloads=0;
    for(const auto& d:values(all)){const auto name=d.get("name").toString();const auto f=nativeMaterialXNoiseBinding(name);REQUIRE(f.isCallable());REQUIRE(f.sameValue(nativeMaterialXNoisePrivate(name)));if(d.get("exported").toBoolean())REQUIRE(exports.get(name).sameValue(f));
        if(d.get("kind").strictEquals("Fn")){++fnCount;const auto shader=f.get("shaderNode"),body=shader.get("jsFunc");REQUIRE(body.get("name").strictEquals(""));REQUIRE(body.get("length").strictEquals(1));REQUIRE(call(body,"toString").toString().find("=>")!=std::string::npos);REQUIRE(call(body,"toString").toString().find("notPorted")==std::string::npos);if(d.get("layout").isNull()||d.get("layout").isUndefined())REQUIRE(shader.get("layout").isNull());else REQUIRE(shader.get("layout").get("inputs").length()+1==d.get("layout").length());}
        if(d.get("kind").strictEquals("overload"))++overloads;
    }
    REQUIRE(fnCount==58);REQUIRE(overloads==17);REQUIRE(nativeMaterialXNoisePrivate("mxBilerpValue").get("length").strictEquals(6));REQUIRE(nativeMaterialXNoisePrivate("mxTrilerpValue").get("length").strictEquals(11));REQUIRE(nativeMaterialXNoisePrivate("mx_perlin_noise_float_scaled").get("length").strictEquals(1));
}
TEST_CASE("MaterialXNoise integer mixing preserves the complete assignment and rotation graphs"){
    const auto mix=run(request("mx_bjmix",{spec("uint",{2}),spec("uint",{3}),spec("uint",{4})}));REQUIRE(mix.get("error").isUndefined());REQUIRE(count(mix,"AssignNode")==18);REQUIRE(count(mix,"OperatorNode")==18);REQUIRE(count(mix,"Node")==6);REQUIRE(length(mix,"stack")==58);
    const auto final=run(request("mx_bjfinal",{spec("uint",{2}),spec("uint",{3}),spec("uint",{4})}));REQUIRE(final.get("error").isUndefined());REQUIRE(count(final,"AssignNode")==14);REQUIRE(count(final,"OperatorNode")==14);REQUIRE(count(final,"Node")==7);REQUIRE(length(final,"stack")==51);
    const auto rotl=run(request("mx_rotl32",{spec("uint",{0x12345678}),spec("int",{13})}));REQUIRE(rotl.get("error").isUndefined());REQUIRE(count(rotl,"OperatorNode")==4);REQUIRE(length(rotl,"stack")==11);
}
TEST_CASE("MaterialXNoise floorfrac writes into the caller variable and returns the fractional subtraction"){
    StackScope scope;const auto x=node("float",{-.371}),integral=call(node("int"),"toVar",{"integral"});const auto fn=nativeMaterialXNoiseBinding("mx_floorfrac").get("shaderNode").get("jsFunc"),result=fn.call({}, {array({x,integral})});
    const auto assignment=find(scope.stack.get("nodes"),"AssignNode");REQUIRE(assignment.get("targetNode").sameValue(integral));REQUIRE(result.get("type").strictEquals("VarNode"));REQUIRE(result.get("node").get("op").strictEquals("-"));REQUIRE(find(result.get("node").get("bNode"),"ConvertNode").get("node").sameValue(integral));
}
TEST_CASE("MaterialXNoise expands real fractal Worley and unified control bodies"){
    const auto f=spec("float"),i=spec("int",{3}),v=spec("vec3",{.173,.346,.519});
    const auto fractal=run(request("mx_fractal_noise_float",{v,i,f,f}));REQUIRE(fractal.get("error").isUndefined());REQUIRE(length(fractal,"controls")==1);REQUIRE(count(fractal,"LoopNode")==1);REQUIRE(count(fractal,"AssignNode")==3);
    const auto worley=run(request("mx_worley_noise_float_3d",{v,f,i}));REQUIRE(worley.get("error").isUndefined());REQUIRE(length(worley,"controls")==6);REQUIRE(count(worley,"LoopNode")==3);REQUIRE(count(worley,"ConditionalNode")==2);REQUIRE(count(worley,"AssignNode")==7);
    const auto unified=run(request("mx_unifiednoise3d",{i,v,v,v,f,f,f,f,i,f,f,i}));REQUIRE(unified.get("error").isUndefined());REQUIRE(length(unified,"controls")==4);REQUIRE(count(unified,"ConditionalNode")==4);REQUIRE(count(unified,"AssignNode")==4);
}
TEST_CASE("MaterialXNoise selects and caches every declared overload candidate"){
    const auto all=declarations().get("MaterialXNoise");unsigned selected=0;StackScope scope;const auto b=builder();
    for(const auto& d:values(all))if(d.get("kind").strictEquals("overload"))for(const auto& candidateName:values(d.get("candidates"))){JSValue candidateDeclaration;for(const auto& candidate:values(all))if(candidate.get("name").strictEquals(candidateName)){candidateDeclaration=candidate;break;}const auto expected=nativeMaterialXNoiseBinding(candidateName.toString()),overload=nativeMaterialXNoiseBinding(d.get("name").toString()).call({},typed(candidateDeclaration));REQUIRE(call(overload,"getCandidateFn",{b}).sameValue(expected));REQUIRE(overload.get("_candidateFn").sameValue(expected));REQUIRE(call(overload,"getNodeType",{b}).strictEquals(expected.get("shaderNode").get("layout").get("type")));const auto setup=call(overload,"setup",{b});REQUIRE(find(setup,"Node").get("shaderNode").sameValue(expected.get("shaderNode")));++selected;}
    REQUIRE(selected==41);
}
TEST_CASE("MaterialXNoise fade and uint normalization execute real NodeBuilder stages"){
    for(const auto* name:{"mx_fade","mx_bits_to_01"}){const auto result=run(request(name,{spec(name==std::string("mx_fade")?"float":"uint")},true));REQUIRE(result.get("error").isUndefined());const auto build=rootProperty(result,"build"),steps=property(result,build,"steps");REQUIRE(result.get("objects").get(steps.get("1")).get("length").strictEquals(3));}
}
