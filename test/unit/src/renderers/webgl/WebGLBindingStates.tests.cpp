#include "catch.hpp"
#include "parity/WebGLBindingStatesFixture.h"
#include "constants.h"
#include "renderers/webgl/WebGLAttributes.h"
#include "core/GLBufferAttribute.h"
#include <thread>

using namespace BindingFixture;
namespace {
unsigned commands(const Fixture& f,const char* name) {
    unsigned count=0;const auto events=f.state.get("log");
    for(unsigned i=0;i<events.length();++i)if(events.get(std::to_string(i)).get("0").strictEquals(name))++count;
    return count;
}
JSValue command(const Fixture& f,const char* name,unsigned occurrence=0) {
    const auto events=f.state.get("log");
    for(unsigned i=0;i<events.length();++i) {const auto row=events.get(std::to_string(i));if(row.get("0").strictEquals(name) && occurrence--==0)return row;}
    throw std::runtime_error("Missing command");
}
}
TEST_CASE("WebGLBindingStates snapshots inherited enumeration keys before cache getter mutation","[webgl-binding-states]") {
    Fixture f;auto prototype=JSValue::object(),second=JSValue::object(prototype),a=programAttribute();
    JSPropertyDescriptor d;d.enumerable=d.configurable=true;
    d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{property(c[0],"b",programAttribute(2));return c[1];},{prototype,a});second.defineProperty("a",d);
    property(f.state,"calls",0);f.program.set("getAttributes",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        auto state=c[0];const auto n=state.get("calls").toNumber()+1;state.set("calls",n);return n==1?c[1]:c[2];
    },{f.state,f.programAttributes,second}));
    property(f.geometryAttributes,"b",attribute("b"));f.setup();
    REQUIRE(commands(f,"vertexAttribPointer")==2);f.clearLog();f.setup();
    REQUIRE(commands(f,"vertexAttribPointer")==2);f.clearLog();f.setup();
    REQUIRE(commands(f,"vertexAttribPointer")==0);
}
TEST_CASE("WebGLBindingStates caches attribute identity rather than mutable layout fields","[webgl-binding-states]") {
    Fixture f;f.setup();REQUIRE(commands(f,"createVertexArray")==1);REQUIRE(commands(f,"vertexAttribPointer")==1);
    f.clearLog();f.source.set("itemSize",4);f.source.set("normalized",true);property(f.source,"version",2);f.setup();
    REQUIRE(commands(f,"getAttributes")==1);REQUIRE(commands(f,"get")==0);REQUIRE(commands(f,"vertexAttribPointer")==0);
    f.geometryAttributes.set("a",attribute("new"));f.setup();
    REQUIRE(commands(f,"get")==1);REQUIRE(commands(f,"vertexAttribPointer")==1);
    REQUIRE(command(f,"bindBuffer").get("3").strictEquals("buffer:new"));
    REQUIRE(commands(f,"enableVertexAttribArray")==0);
}
TEST_CASE("WebGLBindingStates separates geometry program strict wireframe and instance identities","[webgl-binding-states]") {
    Fixture f;f.setup();f.geometry.set("id",2);f.setup();f.geometry.set("id",1);f.clearLog();f.setup();
    REQUIRE(command(f,"bindVertexArray").get("2").strictEquals("vao:1"));REQUIRE(commands(f,"vertexAttribPointer")==0);
    f.object.set("id",10);f.object.set("isInstancedMesh","true");f.material.set("wireframe","true");f.setup();
    REQUIRE(commands(f,"createVertexArray")==0);
    f.object.set("isInstancedMesh",true);f.setup();f.object.set("id",11);f.setup();
    REQUIRE(commands(f,"createVertexArray")==2);f.material.set("wireframe",true);f.setup();
    REQUIRE(commands(f,"createVertexArray")==3);f.program.set("id",3);f.setup();
    REQUIRE(commands(f,"createVertexArray")==4);
}
TEST_CASE("WebGLBindingStates emits integer matrix columns with interleaved stride and divisor","[webgl-binding-states]") {
    Fixture f;auto pa=f.programAttributes.get("a");pa.set("location",2);pa.set("locationSize",4);
    f.source.set("itemSize",16);property(f.source,"gpuType",IntType);property(f.source,"isInterleavedBufferAttribute",true);
    auto data=JSValue::object();property(data,"stride",20);property(data,"count",5);property(data,"meshPerAttribute",2);property(data,"isInstancedInterleavedBuffer",true);
    property(f.source,"data",data);property(f.source,"offset",1);f.setup();
    REQUIRE(commands(f,"enableVertexAttribArray")==4);REQUIRE(commands(f,"vertexAttribDivisor")==4);
    REQUIRE(commands(f,"vertexAttribIPointer")==4);REQUIRE(commands(f,"vertexAttribPointer")==0);
    for(unsigned i=0;i<4;++i) {const auto row=command(f,"vertexAttribIPointer",i);
        REQUIRE(row.get("2").strictEquals(i+2));REQUIRE(row.get("3").strictEquals(4));
        REQUIRE(row.get("5").strictEquals(80));REQUIRE(row.get("6").strictEquals(4+i*16));}
    REQUIRE(f.geometry.get("_maxInstanceCount").strictEquals(10));
    data.set("meshPerAttribute",3);invoke(f.manager,"reset");f.clearLog();f.setup();
    REQUIRE(commands(f,"enableVertexAttribArray")==0);REQUIRE(commands(f,"vertexAttribDivisor")==4);
    REQUIRE(f.geometry.get("_maxInstanceCount").strictEquals(10));
}
TEST_CASE("WebGLBindingStates applies defaults on cache changes and disables retired columns","[webgl-binding-states]") {
    Fixture f;f.programAttributes.get("a").set("locationSize",2);f.setup();f.clearLog();f.geometryAttributes.erase("a");
    auto defaults=JSValue::object();property(defaults,"a",array({1,2,3,4}));property(f.material,"defaultAttributeValues",defaults);f.setup();
    REQUIRE(commands(f,"vertexAttrib4fv")==1);REQUIRE(commands(f,"disableVertexAttribArray")==2);
    defaults.set("a",array({4,3}));f.clearLog();f.setup();REQUIRE(commands(f,"vertexAttrib2fv")==0);
    invoke(f.manager,"reset");f.setup();REQUIRE(commands(f,"vertexAttrib2fv")==1);
    f.geometryAttributes.set("a",f.source);f.source.set("cache",JSValue{});f.clearLog();f.setup();
    REQUIRE(commands(f,"vertexAttribPointer")==0);REQUIRE(commands(f,"vertexAttrib2fv")==0);
}
TEST_CASE("WebGLBindingStates index updates run each setup while binding waits for cache change","[webgl-binding-states]") {
    Fixture f;auto index=attribute("index");f.setup(index);f.clearLog();f.setup(index);
    REQUIRE(commands(f,"update")==1);REQUIRE(commands(f,"bindBuffer")==0);REQUIRE(commands(f,"get")==0);
    auto replacement=attribute("index2");f.setup(replacement);REQUIRE(commands(f,"update")==2);
    REQUIRE(command(f,"bindBuffer",1).get("2").strictEquals(34963));
    REQUIRE(command(f,"bindBuffer",1).get("3").strictEquals("buffer:index2"));
}
TEST_CASE("WebGLBindingStates manual attribute tracking persists across resets of default state","[webgl-binding-states]") {
    Fixture f;invoke(f.manager,"enableAttribute",{1});invoke(f.manager,"enableAttribute",{1});
    REQUIRE(commands(f,"enableVertexAttribArray")==1);REQUIRE(commands(f,"vertexAttribDivisor")==0);
    invoke(f.manager,"resetDefaultState");invoke(f.manager,"disableUnusedAttributes");REQUIRE(commands(f,"disableVertexAttribArray")==0);
    invoke(f.manager,"initAttributes");invoke(f.manager,"disableUnusedAttributes");REQUIRE(commands(f,"disableVertexAttribArray")==1);
    invoke(f.manager,"enableAttribute",{1});REQUIRE(commands(f,"enableVertexAttribArray")==2);
}
TEST_CASE("WebGLBindingStates interrupted setup preserves saved identity cache until forced reset","[webgl-binding-states]") {
    Fixture f;f.state.set("throw","vertexAttribPointer");REQUIRE_THROWS_AS(f.setup(),JSRangeError);
    f.state.set("throw","");f.clearLog();f.setup();REQUIRE(commands(f,"vertexAttribPointer")==0);
    invoke(f.manager,"reset");f.setup();REQUIRE(commands(f,"vertexAttribPointer")==1);
    f.state.set("throw","deleteVertexArray");REQUIRE_THROWS_AS(invoke(f.manager,"dispose"),JSRangeError);
    f.state.set("throw","");f.clearLog();invoke(f.manager,"dispose");REQUIRE(commands(f,"deleteVertexArray")==1);
}
TEST_CASE("WebGLBindingStates released current state retains attributes until reset","[webgl-binding-states][gc]") {
    auto anchor=JSValue::object();JSWeakValue source;
    auto gl=JSValue::object(),attrs=JSValue::object(),geometry=JSValue::object(),program=JSValue::object(),object=JSValue::object(),material=JSValue::object();
    property(gl,"MAX_VERTEX_ATTRIBS",1);property(gl,"getParameter",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return 1;}));
    for(const char* name:{"createVertexArray","bindVertexArray","deleteVertexArray","bindBuffer","vertexAttribPointer","enableVertexAttribArray","disableVertexAttribArray"})property(gl,name,JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {}; }));
    property(attrs,"get",JSValue::function(+[](const JSValue&,const Args& args,const Args&)->JSValue{return args[0].get("cache");}));
    auto values=JSValue::object(),layout=JSValue::object();property(layout,"a",programAttribute());property(geometry,"id",1);property(geometry,"attributes",values);
    property(program,"id",1);property(program,"getAttributes",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{layout}));
    WebGLBindingStates bindings(gl,attrs);
    {auto attribute=BindingFixture::attribute();source=JSWeakValue(attribute);values.set("a",attribute);bindings.setup(object,material,program,geometry,nullptr);values.erase("a");}
    bindings.releaseStatesOfGeometry(geometry);anchor.collectGarbage();REQUIRE_FALSE(source.expired());
    bindings.reset();anchor.collectGarbage();REQUIRE(source.expired());
}
TEST_CASE("WebGLBindingStates detached constructor instances retain their managed closure graph","[webgl-binding-states][gc]") {
    auto anchor=JSValue::object();JSWeakValue source,manager;JSValue instance;
    {Fixture f;source=JSWeakValue(f.source);manager=JSWeakValue(f.manager);
        instance=f.manager.get("setup").construct({f.object,f.material,f.program,f.geometry,nullptr});}
    anchor.collectGarbage();REQUIRE_FALSE(source.expired());REQUIRE_FALSE(manager.expired());
    instance={};anchor.collectGarbage();REQUIRE(source.expired());REQUIRE(manager.expired());
}
TEST_CASE("WebGLBindingStates preserves declarations and works with actual GLBufferAttribute cache","[webgl-binding-states]") {
    Fixture f;for(const char* name:{"setup","reset","resetDefaultState","dispose","releaseStatesOfGeometry","releaseStatesOfObject","releaseStatesOfProgram","initAttributes","enableAttribute","disableUnusedAttributes"}) {
        const auto fn=f.manager.get(name);REQUIRE(fn.isConstructible());REQUIRE(fn.get("name").strictEquals(name));
        const auto d=fn.getOwnPropertyDescriptor("prototype");REQUIRE(d->writable==true);REQUIRE(d->enumerable==false);REQUIRE(d->configurable==false);
        REQUIRE(fn.get("prototype").get("constructor").sameValue(fn));
    }
    GLBufferAttribute source("opaque",5124,3,4,2,true);auto attributes=WebGLAttributes(f.gl);
    attributes.update(source.object(),34962);f.geometryAttributes.set("a",source.object());
    property(f.state,"attributeManager",attributes.object());WebGLBindingStates bindings(f.gl,attributes.object());f.clearLog();
    bindings.setup(f.object,f.material,f.program,f.geometry,nullptr);
    REQUIRE(command(f,"bindBuffer").get("3").strictEquals("opaque"));REQUIRE(commands(f,"vertexAttribIPointer")==1);
    REQUIRE(command(f,"vertexAttribIPointer").get("5").strictEquals(12));
}
TEST_CASE("WebGLBindingStates rejects foreign threads before copying constructor method or result handles","[webgl-binding-states][threads]") {
    Fixture mainFixture;WebGLBindingStates owner(mainFixture.gl,mainFixture.attributeManager);auto foreign=JSValue::object();
    unsigned rejected=0;bool unexpected=false,unchanged=false;
    std::thread worker([&] {
        auto reject=[&](auto operation) {try {operation();unexpected=true;}catch(const JSTypeError&){++rejected;}catch(...){unexpected=true;}};
        reject([&]{WebGLBindingStates invalid(mainFixture.gl,{});});
        Fixture localFixture;WebGLBindingStates local(localFixture.gl,localFixture.attributeManager);
        const auto logLength=localFixture.state.get("log").length();
        reject([&]{WebGLBindingStates invalid(localFixture.gl,mainFixture.attributeManager);});
        for(unsigned slot=0;slot<5;++slot) {
            auto select=[&](unsigned index,const JSValue& value)->const JSValue& {return slot==index?foreign:value;};
            const JSValue index=nullptr;
            reject([&]{local.setup(select(0,localFixture.object),select(1,localFixture.material),select(2,localFixture.program),select(3,localFixture.geometry),select(4,index));});
        }
        reject([&]{local.releaseStatesOfGeometry(foreign);});reject([&]{local.releaseStatesOfObject(foreign);});
        reject([&]{local.releaseStatesOfProgram(foreign);});reject([&]{local.enableAttribute(foreign);});
        reject([&]{owner.reset();});reject([&]{owner.resetDefaultState();});reject([&]{owner.dispose();});
        reject([&]{owner.initAttributes();});reject([&]{owner.disableUnusedAttributes();});
        reject([&]{owner.object();});reject([&]{static_cast<JSValue>(owner);});reject([&]{owner.setup();});
        unchanged=localFixture.state.get("log").length()==logLength;
    });
    worker.join();REQUIRE_FALSE(unexpected);REQUIRE(rejected==19);REQUIRE(unchanged);
    REQUIRE(mainFixture.state.get("log").length()==2);REQUIRE(foreign.keys().empty());
}
