#include "catch.hpp"
#include "renderers/webgl/WebGLBufferRenderer.h"
#include "renderers/webgl/WebGLIndexedBufferRenderer.h"
#include "renderers/webgl/WebGLInfo.h"
#include <cmath>

namespace {
using Args = std::vector<JSValue>;
JSValue drawTuple(const Args& args) { auto a=JSValue::array();a.get("push").call(a,args);return a; }
void drawAppend(const JSValue& trace, JSValue value) {trace.get("push").call(trace,{value});}
JSValue drawCall(JSValue renderer,const char* name,const Args& args={}) {return renderer.get(name).call(renderer,args);}
JSValue drawIndex(JSValue type=5123,JSValue bytes=2) {auto index=JSValue::object();index.set("type",type);index.set("bytesPerElement",bytes);return index;}
struct DrawAdapters {
    JSValue trace=JSValue::array(),gl=JSValue::object(),extension=JSValue::object(),extensions=JSValue::object(),info=JSValue::object();
    JSValue record(const char* name,JSValue receiver) {
        return JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue {
            Args entry{c[1],self.sameValue(c[2])};entry.insert(entry.end(),args.begin(),args.end());
            drawAppend(c[0],drawTuple(entry));return {};
        },{trace,name,receiver});
    }
    DrawAdapters() {
        for(const auto* name:{"drawArrays","drawArraysInstanced","drawElements","drawElementsInstanced"})gl.set(name,record(name,gl));
        for(const auto* name:{"multiDrawArraysWEBGL","multiDrawElementsWEBGL"})extension.set(name,record(name,extension));
        extensions.set("get",JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue {
            drawAppend(c[0],drawTuple({"get",self.sameValue(c[1]),args[0]}));return c[2];
        },{trace,extensions,extension}));
        info.set("update",record("info",info));
    }
    JSValue renderer(bool indexed) {
        auto r=indexed?WebGLIndexedBufferRenderer(gl,extensions,info).object():WebGLBufferRenderer(gl,extensions,info).object();
        drawCall(r,"setMode",{4});if(indexed)drawCall(r,"setIndex",{drawIndex()});return r;
    }
};
struct DrawPrototypeGuard {
    JSValue prototype=JSValue::object().getPrototype();
    std::vector<std::pair<std::string,std::optional<JSPropertyDescriptor>>> saved;
    DrawPrototypeGuard(const JSValue& trace,bool readOnly) {
        for(const auto* key:{"mode","type","bytesPerElement"}) {
            saved.emplace_back(key,prototype.getOwnPropertyDescriptor(key));
            JSPropertyDescriptor descriptor;descriptor.configurable=true;
            if(readOnly) {descriptor.value=77;descriptor.writable=false;}
            else {
                descriptor.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
                    drawAppend(c[0],drawTuple({"get",c[1]}));return 77;
                },{trace,key});
                descriptor.setter=JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {
                    drawAppend(c[0],drawTuple({"set",c[1],args[0]}));return {};
                },{trace,key});
            }
            prototype.defineProperty(key,descriptor);
        }
    }
    ~DrawPrototypeGuard() {
        for(const auto& [key,descriptor]:saved) {
            if(descriptor)prototype.defineProperty(key,*descriptor);else prototype.erase(key);
        }
    }
};
}

TEST_CASE("Buffer renderers retain instance method and constructor metadata", "[renderers][webgl-buffer]") {
    for(const bool indexed:{false,true}) {
        DrawAdapters adapters;const auto r=adapters.renderer(indexed);
        REQUIRE(r.keys()==(indexed?std::vector<std::string>{"setMode","setIndex","render","renderInstances","renderMultiDraw"}
            :std::vector<std::string>{"setMode","render","renderInstances","renderMultiDraw"}));
        REQUIRE(r.get("constructor").get("name").string()==(indexed?"WebGLIndexedBufferRenderer":"WebGLBufferRenderer"));
        REQUIRE(r.get("constructor").get("length").number()==3);
        for(const auto* name:{"renderInstances","renderMultiDraw"})REQUIRE(r.get(name).get("length").number()==3);
        REQUIRE(r.get("render").get("length").number()==2);REQUIRE(r.get("setMode").get("length").number()==1);
        REQUIRE(drawCall(r,"render",{2,6}).isUndefined());
        REQUIRE(adapters.trace.get("0").get("1").toBoolean());REQUIRE(adapters.trace.get("1").get("1").toBoolean());
        const auto constructed=r.get("constructor").construct({adapters.gl,adapters.extensions,adapters.info});
        REQUIRE(constructed.getPrototype().sameValue(r.getPrototype()));
        REQUIRE(constructed.get("render").get("name").string()=="render");
    }
}
TEST_CASE("Draw commands forward raw arguments and fetch info after the GPU callback", "[renderers][webgl-buffer]") {
    for(const bool indexed:{false,true}) {
        DrawAdapters a;auto r=a.renderer(indexed),count=JSValue::object();
        const auto name=indexed?"drawElements":"drawArrays";
        a.gl.set(name,JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue {
            REQUIRE(self.sameValue(c[0]));REQUIRE(args[0].number()==4);
            REQUIRE(args[c[3].toBoolean()?1:2].sameValue(c[2]));drawCall(c[1],"setMode",{1});return {};
        },{a.gl,r,count,indexed}));
        JSPropertyDescriptor descriptor;descriptor.configurable=true;
        descriptor.getter=JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue {
            REQUIRE(self.sameValue(c[0]));drawCall(c[1],"setMode",{3});return c[2];
        },{a.info,r,a.info.get("update")});a.info.defineProperty("update",descriptor);
        REQUIRE(r.get("render").call(nullptr,{2,count}).isUndefined());
        REQUIRE(a.trace.length()==1);const auto info=a.trace.get("0");
        REQUIRE(info.get("2").sameValue(count));REQUIRE(info.get("3").number()==3);REQUIRE(info.get("4").number()==1);
    }
}
TEST_CASE("Instanced zero skips by strict equality before every adapter access", "[renderers][webgl-buffer]") {
    for(const bool indexed:{false,true}) {
        auto r=indexed?WebGLIndexedBufferRenderer().object():WebGLBufferRenderer().object();
        REQUIRE(drawCall(r,"renderInstances",{JSValue(),JSValue(),0}).isUndefined());
        REQUIRE(drawCall(r,"renderInstances",{JSValue(),JSValue(),-0.0}).isUndefined());
        REQUIRE(drawCall(r,"renderMultiDraw",{JSValue(),JSValue(),0}).isUndefined());
        REQUIRE_THROWS_AS(drawCall(r,"renderInstances",{0,3,"0"}),JSTypeError);
        DrawAdapters a;r=a.renderer(indexed);
        for(const auto& count:Args{"0",false,nullptr,JSValue()})drawCall(r,"renderInstances",{2,6,count});
        REQUIRE(a.trace.length()==8);
        REQUIRE(a.trace.get("0").get(indexed?"6":"5").string()=="0");
    }
}
TEST_CASE("Indexed mode type and byte size retain upstream capture and conversion order", "[renderers][webgl-buffer]") {
    DrawAdapters a;auto r=a.renderer(true),index=drawIndex();
    drawCall(r,"setIndex",{index});index.set("type",99);index.set("bytesPerElement",8);
    drawCall(r,"render",{3,6});REQUIRE(a.trace.get("0").get("4").number()==5123);REQUIRE(a.trace.get("0").get("5").number()==6);
    auto start=JSValue::object();start.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {
        REQUIRE(args[0].string()=="number");drawCall(c[0],"setMode",{1});drawCall(c[0],"setIndex",{drawIndex(99,8)});return 3;
    },{r}));drawCall(r,"render",{start,6});
    REQUIRE(a.trace.get("2").get("2").number()==4);REQUIRE(a.trace.get("2").get("4").number()==5123);
    REQUIRE(a.trace.get("2").get("5").number()==6);REQUIRE(a.trace.get("3").get("3").number()==1);
    drawCall(r,"render",{3,6});REQUIRE(a.trace.get("4").get("4").number()==99);REQUIRE(a.trace.get("4").get("5").number()==24);
    WebGLIndexedBufferRenderer unset(a.gl,a.extensions,a.info);unset.render(3,6);
    REQUIRE(a.trace.get("6").get("2").isUndefined());REQUIRE(a.trace.get("6").get("4").isUndefined());
    REQUIRE(std::isnan(a.trace.get("6").get("5").number()));
}
TEST_CASE("Indexed setIndex writes type before a failing bytes getter", "[renderers][webgl-buffer]") {
    DrawAdapters a;auto r=a.renderer(true),index=JSValue::object();index.set("type",99);
    JSPropertyDescriptor descriptor;descriptor.getter=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue {throw JSRangeError("bytes");});
    index.defineProperty("bytesPerElement",descriptor);
    REQUIRE_THROWS_AS(drawCall(r,"setIndex",{index}),JSRangeError);drawCall(r,"render",{3,6});
    REQUIRE(a.trace.get("0").get("4").number()==99);REQUIRE(a.trace.get("0").get("5").number()==6);
}
TEST_CASE("Indexed method lookup precedes offset coercion and callability follows it", "[renderers][webgl-buffer]") {
    DrawAdapters a;auto r=a.renderer(true),trace=JSValue::array(),start=JSValue::object();
    start.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {drawAppend(c[0],"start");return 2;},{trace}));
    JSPropertyDescriptor descriptor;descriptor.configurable=true;
    descriptor.getter=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue {throw JSRangeError("draw");});
    a.gl.defineProperty("drawElements",descriptor);
    REQUIRE_THROWS_AS(drawCall(r,"render",{start,6}),JSRangeError);REQUIRE(trace.length()==0);
    a.gl.defineProperty("drawElements",JSPropertyDescriptor{.value=JSValue(),.writable=true,.configurable=true});
    REQUIRE_THROWS_AS(drawCall(r,"render",{start,6}),JSTypeError);REQUIRE(trace.length()==1);REQUIRE(a.trace.length()==0);
}
TEST_CASE("Multi draw retains extension receivers live counts and string addition", "[renderers][webgl-buffer]") {
    for(const bool indexed:{false,true}) {
        DrawAdapters a;auto r=a.renderer(indexed),starts=drawTuple({0,3,9}),counts=drawTuple({1,"2",3});
        drawCall(r,"renderMultiDraw",{starts,counts,2.5});REQUIRE(a.trace.length()==3);
        REQUIRE(a.trace.get("0").get("2").string()=="WEBGL_multi_draw");REQUIRE(a.trace.get("1").get("1").toBoolean());
        REQUIRE(a.trace.get("1").get(indexed?"3":"5").sameValue(counts));
        REQUIRE(a.trace.get("1").get(indexed?"6":"3").sameValue(starts));
        REQUIRE(a.trace.get("2").get("2").string()=="123");
        REQUIRE(a.trace.get("2").get("3").number()==4);REQUIRE(a.trace.get("2").get("4").number()==1);
        a.trace.set("length",0);counts=drawTuple({1,2,3});
        const auto command=indexed?"multiDrawElementsWEBGL":"multiDrawArraysWEBGL";
        a.extension.set(command,JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {
            auto counts=args[c[1].toBoolean()?1:3];counts.set("0",10);drawCall(c[0],"setMode",{1});return {};
        },{r,indexed}));drawCall(r,"renderMultiDraw",{starts,counts,3});
        REQUIRE(a.trace.get("1").get("2").number()==15);REQUIRE(a.trace.get("1").get("3").number()==1);
    }
}
TEST_CASE("Multi draw conditions convert every iteration while counts use default hints", "[renderers][webgl-buffer]") {
    for(const bool indexed:{false,true}) {
        DrawAdapters a;auto r=a.renderer(indexed),trace=JSValue::array(),drawCount=JSValue::object(),counts=JSValue::object();
        drawCount.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {
            REQUIRE(args[0].string()=="number");drawAppend(c[0],"condition");return 2.5;
        },{trace}));
        for(int i=0;i<3;++i) {auto value=JSValue::object();value.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {
            REQUIRE(args[0].string()=="default");drawAppend(c[0],c[1]);return 1;
        },{trace,i}));counts.set(std::to_string(i),value);}
        drawCall(r,"renderMultiDraw",{JSValue::object(),counts,drawCount});
        REQUIRE(trace.length()==7);REQUIRE(trace.get("0").string()=="condition");REQUIRE(trace.get("1").number()==0);
        REQUIRE(trace.get("6").string()=="condition");REQUIRE(a.trace.get("2").get("2").number()==3);
    }
}
TEST_CASE("Missing multi draw extension throws without fallback or info updates", "[renderers][webgl-buffer]") {
    for(const bool indexed:{false,true}) {
        DrawAdapters a;auto r=a.renderer(indexed);
        a.extensions.set("get",JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue {return nullptr;}));
        REQUIRE_THROWS_AS(drawCall(r,"renderMultiDraw",{drawTuple({0}),drawTuple({3}),1}),JSTypeError);
        REQUIRE(a.trace.length()==0);
        REQUIRE(drawCall(r,"renderMultiDraw",{nullptr,nullptr,0}).isUndefined());
        REQUIRE_THROWS_AS(drawCall(r,"renderMultiDraw",{nullptr,nullptr,"0"}),JSTypeError);
    }
}
TEST_CASE("GPU command exceptions prevent info and successful draws integrate WebGLInfo", "[renderers][webgl-buffer]") {
    for(const bool indexed:{false,true}) {
        DrawAdapters a;auto r=a.renderer(indexed);
        a.gl.set(indexed?"drawElements":"drawArrays",JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue {throw JSRangeError("gpu");}));
        REQUIRE_THROWS_AS(drawCall(r,"render",{0,6}),JSRangeError);REQUIRE(a.trace.length()==0);
        auto gl=JSValue::object();gl.set("TRIANGLES",4);WebGLInfo info(gl);
        DrawAdapters commands;r=indexed?WebGLIndexedBufferRenderer(commands.gl,commands.extensions,info).object():WebGLBufferRenderer(commands.gl,commands.extensions,info).object();
        drawCall(r,"setMode",{4});if(indexed)drawCall(r,"setIndex",{drawIndex()});
        drawCall(r,"render",{0,6});drawCall(r,"renderInstances",{0,6,3});
        drawCall(r,"renderMultiDraw",{drawTuple({0,6}),drawTuple({6,3}),2});
        REQUIRE(info.render.get("calls").number()==3);REQUIRE(info.render.get("triangles").number()==11);
    }
}
TEST_CASE("Renderer lexical state bypasses inherited accessors", "[renderers][webgl-buffer]") {
    for(const bool indexed:{false,true}) {
        DrawAdapters a;const auto index=drawIndex(5125,4),pollution=JSValue::array();
        DrawPrototypeGuard guard(pollution,false);
        auto r=indexed?WebGLIndexedBufferRenderer(a.gl,a.extensions,a.info).object():WebGLBufferRenderer(a.gl,a.extensions,a.info).object();
        drawCall(r,"render",{3,6});
        REQUIRE(a.trace.get("0").get("2").isUndefined());
        if(indexed) {REQUIRE(a.trace.get("0").get("4").isUndefined());REQUIRE(std::isnan(a.trace.get("0").get("5").number()));}
        drawCall(r,"setMode",{4});if(indexed)drawCall(r,"setIndex",{index});
        drawCall(r,"render",{3,6});drawCall(r,"setMode",{1});drawCall(r,"renderInstances",{5,8,2});
        drawCall(r,"renderMultiDraw",{drawTuple({0,3}),drawTuple({6,2}),2});
        REQUIRE(a.trace.get("2").get("2").number()==4);
        REQUIRE(a.trace.get("4").get("2").number()==1);
        REQUIRE(a.trace.get("7").get("2").number()==1);
        REQUIRE(a.trace.get("8").get("2").number()==8);
        if(indexed) {REQUIRE(a.trace.get("2").get("4").number()==5125);REQUIRE(a.trace.get("2").get("5").number()==12);
            REQUIRE(a.trace.get("4").get("5").number()==20);REQUIRE(a.trace.get("7").get("5").number()==5125);}
        REQUIRE(pollution.length()==0);
    }
}
TEST_CASE("Renderer closure construction retains its original state and draw receivers", "[renderers][webgl-buffer][construct]") {
    for(const bool indexed:{false,true}) {
        DrawAdapters a;auto r=a.renderer(indexed),custom=JSValue::object();
        auto target=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{throw std::runtime_error("newTarget body executed");},{},"Target",0,JSValue::ConstructorKind::Callable);
        target.set("prototype",custom);
        for(const auto& name:r.keys()) {
            const auto fn=r.get(name); REQUIRE(fn.isConstructible());
            REQUIRE(fn.ownPropertyNames()==std::vector<std::string>{"length","name","prototype"});
            const auto d=*fn.getOwnPropertyDescriptor("prototype");
            REQUIRE(d.writable==true);REQUIRE(d.enumerable==false);REQUIRE(d.configurable==false);
            REQUIRE(fn.get("prototype").get("constructor").sameValue(fn));
        }
        const auto instance=r.get("setMode").construct({1},target);
        REQUIRE(instance.getPrototype().sameValue(custom)); REQUIRE(instance.ownPropertyNames().empty());
        if(indexed) {
            auto input=drawIndex(5125,4);JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("bytes");});d.configurable=true;
            input.defineProperty("bytesPerElement",d);
            REQUIRE_THROWS_AS(r.get("setIndex").construct({input},target),JSRangeError);
        }
        REQUIRE(r.get("render").construct({3,6},target).getPrototype().sameValue(custom));
        REQUIRE(a.trace.length()==2); REQUIRE(a.trace.get("0").get("1").toBoolean());
        REQUIRE(a.trace.get("0").get("2").number()==1); REQUIRE(a.trace.get("1").get("2").number()==6);
        if(indexed){REQUIRE(a.trace.get("0").get("4").number()==5125);REQUIRE(a.trace.get("0").get("5").number()==6);}
        target.set("prototype",nullptr);
        REQUIRE(r.get("renderInstances").construct({3,6,0},target).getPrototype().sameValue(JSValue::object().getPrototype()));
        REQUIRE(a.trace.length()==2);
    }
}
TEST_CASE("Renderer lexical state bypasses inherited readonly properties", "[renderers][webgl-buffer]") {
    for(const bool indexed:{false,true}) {
        DrawAdapters a;const auto index=drawIndex(5125,4),pollution=JSValue::array();
        DrawPrototypeGuard guard(pollution,true);
        auto r=indexed?WebGLIndexedBufferRenderer(a.gl,a.extensions,a.info).object():WebGLBufferRenderer(a.gl,a.extensions,a.info).object();
        drawCall(r,"setMode",{4});if(indexed)drawCall(r,"setIndex",{index});
        drawCall(r,"render",{3,6});drawCall(r,"setMode",{1});drawCall(r,"renderInstances",{5,8,2});
        drawCall(r,"renderMultiDraw",{drawTuple({0,3}),drawTuple({6,2}),2});
        REQUIRE(a.trace.get("0").get("2").number()==4);REQUIRE(a.trace.get("2").get("2").number()==1);
        REQUIRE(a.trace.get("5").get("2").number()==1);REQUIRE(a.trace.get("6").get("2").number()==8);
        if(indexed) {REQUIRE(a.trace.get("0").get("4").number()==5125);REQUIRE(a.trace.get("0").get("5").number()==12);
            REQUIRE(a.trace.get("2").get("5").number()==20);REQUIRE(a.trace.get("5").get("5").number()==5125);}
        REQUIRE(pollution.length()==0);
    }
}
