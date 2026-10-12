#include "catch.hpp"
#include "renderers/webgl/WebGLInfo.h"
#include <cmath>
#include <limits>

TEST_CASE("Constructed WebGLInfo receivers retain and then collect the closure prototype graph", "[renderers][webgl-info][construct][gc]") {
    auto witness=JSValue::object();JSValue instance;JSWeakValue weakFunction,weakRender,weakPrototype;
    {
        WebGLInfo info(JSValue::object());auto reset=info.object().get("reset");
        info.render.set("calls",9);weakRender=JSWeakValue(info.render.value());
        weakFunction=JSWeakValue(reset);weakPrototype=JSWeakValue(reset.get("prototype"));
        instance=reset.construct();
    }
    witness.collectGarbage();REQUIRE_FALSE(weakFunction.expired());REQUIRE_FALSE(weakPrototype.expired());REQUIRE_FALSE(weakRender.expired());
    REQUIRE(weakRender.lock()->get("calls").number()==0);
    weakRender.lock()->set("calls",7);
    instance.getPrototype().get("constructor").construct();
    REQUIRE(weakRender.lock()->get("calls").number()==0);
    instance=JSValue();witness.collectGarbage();
    REQUIRE(weakFunction.expired());REQUIRE(weakPrototype.expired());REQUIRE(weakRender.expired());
}
TEST_CASE("WebGLInfo closure construction mutates captured counters and obeys newTarget", "[renderers][webgl-info][construct]") {
    WebGLInfo info([] {auto gl=JSValue::object();gl.set("TRIANGLES",4);return gl;}());
    const auto update=info.object().get("update"),reset=info.object().get("reset");
    for(const auto& fn:{update,reset}) {
        REQUIRE(fn.isConstructible()); REQUIRE(fn.ownPropertyNames()==std::vector<std::string>{"length","name","prototype"});
        const auto d=*fn.getOwnPropertyDescriptor("prototype");
        REQUIRE(d.writable==true); REQUIRE(d.enumerable==false); REQUIRE(d.configurable==false);
        REQUIRE(fn.get("prototype").get("constructor").sameValue(fn));
    }
    auto custom=JSValue::object(),events=JSValue::array();
    auto target=JSValue::constructor(+[](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>& c)->JSValue{
        c[0].get("push").call(c[0],{1});return {};
    },{events},"Target",0,JSValue::ConstructorKind::Callable);
    target.set("prototype",custom);
    const auto value=update.construct({12,4,2},target);
    REQUIRE(value.getPrototype().sameValue(custom)); REQUIRE(value.ownPropertyNames().empty()); REQUIRE(events.length()==0);
    REQUIRE(info.render.get("calls").number()==1); REQUIRE(info.render.get("triangles").number()==8);
    // Rejecting a nonconstructor newTarget must precede all captured side effects.
    REQUIRE_THROWS_AS(update.construct({12,4,2},JSValue::function(+[](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&)->JSValue{return {};})),JSTypeError);
    REQUIRE(info.render.get("calls").number()==1);
    target.set("prototype",17); REQUIRE(reset.construct({},target).getPrototype().sameValue(JSValue::object().getPrototype()));
    REQUIRE(info.render.get("calls").number()==0); REQUIRE(info.render.get("triangles").number()==0);
}

namespace {
using Args = std::vector<JSValue>;
JSValue infoGL() {
    auto gl = JSValue::object();
    gl.set("TRIANGLES", 4); gl.set("LINES", 1); gl.set("LINE_STRIP", 3);
    gl.set("LINE_LOOP", 2); gl.set("POINTS", 0); return gl;
}
struct InfoConsoleScope {
    JSValue previous = WebGLInfo::getConsoleFunction();
    ~InfoConsoleScope() { WebGLInfo::setConsoleFunction(previous); }
};
void appendInfoTrace(const JSValue& trace, const JSValue& item) {
    trace.get("push").call(trace, {item});
}
JSValue infoConvertible(JSValue trace, const char* name, JSValue result) {
    auto value = JSValue::object();
    value.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args& args, const Args& captures)->JSValue {
        appendInfoTrace(captures[0], captures[1].toString() + ":" + args[0].toString());
        return captures[2];
    }, {trace, name, result})); return value;
}
struct InfoPrototypeScope {
    JSValue prototype=JSValue::object().getPrototype();
    std::vector<std::pair<std::string,std::optional<JSPropertyDescriptor>>> saved;
    InfoPrototypeScope(JSValue trace,bool readOnly) {
        for(const auto* key:{"geometries","textures","frame","calls","triangles","points","lines",
                            "memory","render","programs","autoReset","reset","update"}) {
            saved.emplace_back(key,prototype.getOwnPropertyDescriptor(key));
            JSPropertyDescriptor descriptor;descriptor.configurable=true;
            if(readOnly){descriptor.value=77;descriptor.writable=false;}
            else {
                descriptor.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
                    appendInfoTrace(c[0],c[1]);return 77;
                },{trace,key});
                descriptor.setter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
                    appendInfoTrace(c[0],c[1]);return {};
                },{trace,key});
            }
            prototype.defineProperty(key,descriptor);
        }
    }
    ~InfoPrototypeScope(){for(const auto& [key,d]:saved)if(d)prototype.defineProperty(key,*d);else prototype.erase(key);}
};
}

TEST_CASE("WebGLInfo exposes upstream plain object fields and method metadata", "[renderers][webgl-info]") {
    WebGLInfo info(infoGL());
    REQUIRE(info.object().keys() == std::vector<std::string>{"memory", "render", "programs", "autoReset", "reset", "update"});
    REQUIRE(info.memory.value().keys() == std::vector<std::string>{"geometries", "textures"});
    REQUIRE(info.render.value().keys() == std::vector<std::string>{"frame", "calls", "triangles", "points", "lines"});
    for (const auto* field : {"frame", "calls", "triangles", "points", "lines"}) REQUIRE(info.render.get(field).number() == 0);
    REQUIRE(info.memory.get("geometries").number() == 0); REQUIRE(info.memory.get("textures").number() == 0);
    REQUIRE(info.programs.isNull()); REQUIRE(info.autoReset.strictEquals(true));
    REQUIRE(info.object().get("update").get("name").string() == "update");
    REQUIRE(info.object().get("update").get("length").number() == 3);
    REQUIRE(info.object().get("reset").get("name").string() == "reset");
    REQUIRE(info.object().get("reset").get("length").number() == 0);
}
TEST_CASE("WebGLInfo counts all five draw modes without rounding", "[renderers][webgl-info]") {
    WebGLInfo info(infoGL());
    REQUIRE(info.update(5.5, 4, 2).isUndefined());
    REQUIRE(info.render.get("triangles").number() == 2 * (5.5 / 3));
    info.update(5.5, 1, 2); REQUIRE(info.render.get("lines").number() == 5.5);
    info.update(0, 3, 1); REQUIRE(info.render.get("lines").number() == 4.5);
    info.update(5.5, 2, 2); REQUIRE(info.render.get("lines").number() == 15.5);
    info.update(5.5, 0, 2); REQUIRE(info.render.get("points").number() == 11);
    REQUIRE(info.render.get("calls").number() == 5);
}
TEST_CASE("WebGLInfo retains calls increment coercion and compound string addition", "[renderers][webgl-info]") {
    WebGLInfo info(infoGL()); info.render.set("calls", "2"); info.render.set("triangles", "total:");
    info.update("6", 4, "2");
    REQUIRE(info.render.get("calls").number() == 3);
    REQUIRE(info.render.get("triangles").string() == "total:4");
    info.render.set("points", -0.0); info.update(-0.0, 0, 1);
    REQUIRE(std::signbit(info.render.get("points").number()));
    info.update(JSValue(), 0, 1); REQUIRE(std::isnan(info.render.get("points").number()));
}
TEST_CASE("WebGLInfo reads counters before RHS conversion and preserves arithmetic order", "[renderers][webgl-info]") {
    for (const auto mode : {4, 1, 3, 2, 0}) {
        auto trace = JSValue::array(); WebGLInfo info(infoGL());
        const auto field = mode == 4 ? "triangles" : mode == 0 ? "points" : "lines";
        JSPropertyDescriptor descriptor;
        descriptor.getter = JSValue::function([](const JSValue&, const Args&, const Args& captures)->JSValue {
            appendInfoTrace(captures[0], "get"); return captures[1];
        }, {trace, infoConvertible(trace, "old", "sum:")});
        descriptor.setter = JSValue::function([](const JSValue&, const Args& args, const Args& captures)->JSValue {
            appendInfoTrace(captures[0], args[0]); return {};
        }, {trace}); descriptor.configurable = true;
        info.render.value().defineProperty(field, descriptor);
        info.update(infoConvertible(trace, "count", 6), mode, infoConvertible(trace, "instances", 2));
        const auto order = mode == 2 || mode == 0
            ? std::vector<std::string>{"get", "instances:number", "count:number", "old:default"}
            : std::vector<std::string>{"get", "count:number", "instances:number", "old:default"};
        REQUIRE(trace.length() == 5);
        for (std::size_t i = 0; i < order.size(); ++i) REQUIRE(trace.get(std::to_string(i)).string() == order[i]);
        REQUIRE(trace.get("4").string() == (mode == 4 ? "sum:4" : mode == 1 ? "sum:6" : mode == 3 ? "sum:10" : "sum:12"));
    }
}
TEST_CASE("WebGLInfo methods retain their original render closure after public replacement", "[renderers][webgl-info]") {
    WebGLInfo info(infoGL()); const auto render = info.render.value();
    info.render = nullptr; info.memory = "memory"; info.programs = 17; info.autoReset = "manual";
    info.object().get("update").call(nullptr, {9, 4, 2});
    REQUIRE(render.get("calls").number() == 1); REQUIRE(render.get("triangles").number() == 6);
    auto retained = render; retained.set("frame", 31);
    REQUIRE(info.reset().isUndefined()); REQUIRE(render.get("calls").number() == 0);
    REQUIRE(render.get("frame").number() == 31);
    REQUIRE(info.render.isNull()); REQUIRE(info.memory.toString() == "memory");
    REQUIRE(info.programs.toNumber() == 17); REQUIRE(info.autoReset.toString() == "manual");
    WebGLInfo copy(info); REQUIRE(copy.object().sameValue(info.object()));
    info.object().set("update", JSValue::function([](const JSValue& self, const Args&, const Args&)->JSValue { return self.get("programs"); }));
    REQUIRE(copy.update().number() == 17);
}
TEST_CASE("WebGLInfo enumerants are dynamic strict comparisons with the GL receiver", "[renderers][webgl-info]") {
    auto gl = infoGL(), trace = JSValue::array(), mode = JSValue::object();
    for (const auto* name : {"TRIANGLES", "LINES", "LINE_STRIP", "LINE_LOOP", "POINTS"}) {
        JSPropertyDescriptor descriptor; descriptor.configurable = true;
        descriptor.getter = JSValue::function([](const JSValue& self, const Args&, const Args& captures)->JSValue {
            REQUIRE(self.sameValue(captures[0])); appendInfoTrace(captures[1], captures[2]);
            return captures[2].string() == "LINE_LOOP" ? captures[3] : JSValue("different");
        }, {gl, trace, name, mode}); gl.defineProperty(name, descriptor);
    }
    WebGLInfo info(gl); info.update(6, mode, 2);
    REQUIRE(trace.length() == 4); REQUIRE(trace.get("0").string() == "TRIANGLES");
    REQUIRE(trace.get("3").string() == "LINE_LOOP"); REQUIRE(info.render.get("lines").number() == 12);
    gl = JSValue::object(); WebGLInfo missing(gl); missing.update(6, JSValue(), 1);
    REQUIRE(missing.render.get("triangles").number() == 2);
}
TEST_CASE("WebGLInfo increments calls before mode and count exceptions", "[renderers][webgl-info]") {
    WebGLInfo info;
    REQUIRE_THROWS_AS(info.update(3, 4, 1), JSTypeError); REQUIRE(info.render.get("calls").number() == 1);
    info = WebGLInfo(infoGL()); auto count = JSValue::object();
    count.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args&, const Args&)->JSValue { throw JSRangeError("count"); }));
    REQUIRE_THROWS_AS(info.update(count, 4, 1), JSRangeError);
    REQUIRE(info.render.get("calls").number() == 1); REQUIRE(info.render.get("triangles").number() == 0);
    info.render.set("calls", count);
    REQUIRE_THROWS_AS(info.update(3, 4, 1), JSRangeError);
    REQUIRE(info.render.get("calls").sameValue(count));
}
TEST_CASE("WebGLInfo reset writes the four counters in upstream order", "[renderers][webgl-info]") {
    WebGLInfo info(infoGL()); auto trace = JSValue::array(); info.render.set("frame", 42);
    for (const auto* field : {"calls", "triangles", "points", "lines"}) {
        JSPropertyDescriptor descriptor;
        descriptor.setter = JSValue::function([](const JSValue&, const Args& args, const Args& captures)->JSValue {
            REQUIRE(args[0].number() == 0); appendInfoTrace(captures[0], captures[1]);
            if (captures[1].string() == "points") throw JSTypeError("points"); return {};
        }, {trace, field}); descriptor.configurable = true;
        info.render.value().defineProperty(field, descriptor);
    }
    REQUIRE_THROWS_AS(info.reset(), JSTypeError); REQUIRE(trace.length() == 3);
    REQUIRE(trace.get("0").string() == "calls"); REQUIRE(trace.get("1").string() == "triangles");
    REQUIRE(trace.get("2").string() == "points"); REQUIRE(info.render.get("frame").number() == 42);
}
TEST_CASE("WebGLInfo unknown mode preserves error handler arguments and exceptions", "[renderers][webgl-info]") {
    InfoConsoleScope scope; auto trace = JSValue::array(), mode = JSValue::object();
    mode.setPrimitiveConverter(JSValue::function([](const JSValue&, const Args&, const Args&)->JSValue { throw JSRangeError("Must not coerce mode"); }));
    WebGLInfo::setConsoleFunction(JSValue::function([](const JSValue& self, const Args& args, const Args& captures)->JSValue {
        REQUIRE(self.isUndefined()); REQUIRE(args[0].string() == "error");
        REQUIRE(args[1].string() == "THREE.WebGLInfo: Unknown draw mode:"); REQUIRE(args[2].sameValue(captures[1]));
        appendInfoTrace(captures[0], args[2]); return {};
    }, {trace, mode}));
    WebGLInfo info(infoGL()); REQUIRE(info.update(3, mode, 1).isUndefined());
    REQUIRE(trace.length() == 1); REQUIRE(info.render.get("calls").number() == 1);
    REQUIRE(info.render.get("triangles").number() == 0);
    WebGLInfo::setConsoleFunction(1);
    REQUIRE_THROWS_AS(info.update(3, mode, 1), JSTypeError); REQUIRE(info.render.get("calls").number() == 2);
    WebGLInfo::setConsoleFunction(JSValue::function([](const JSValue&, const Args&, const Args&)->JSValue { throw JSRangeError("console"); }));
    REQUIRE_THROWS_AS(info.update(3, mode, 1), JSRangeError); REQUIRE(info.render.get("calls").number() == 3);
}
TEST_CASE("WebGLInfo native console preserves StackTrace getter exception behavior", "[renderers][webgl-info]") {
    InfoConsoleScope scope; WebGLInfo::setConsoleFunction(nullptr);
    auto mode = JSValue::object(); JSPropertyDescriptor descriptor;
    descriptor.getter = JSValue::function([](const JSValue&, const Args&, const Args&)->JSValue { throw JSRangeError("stackTrace"); });
    mode.defineProperty("isStackTrace", descriptor); WebGLInfo info(infoGL());
    REQUIRE_THROWS_AS(info.update(3, mode, 1), JSRangeError);
    REQUIRE(info.render.get("calls").number() == 1);
}
TEST_CASE("WebGLInfo closures remain callable after handles expire and release cycles", "[renderers][webgl-info][gc]") {
    auto anchor = JSValue::object(); anchor.collectGarbage(); const auto before = anchor.heapObjectCount();
    JSValue method;
    { WebGLInfo info(infoGL()); info.render.set("owner", info.object()); method = info.object().get("update"); }
    anchor.collectGarbage(); REQUIRE(method.call(nullptr, {6, 4, 1}).isUndefined());
    method = {}; anchor.collectGarbage(); REQUIRE(anchor.heapObjectCount() == before);
}
TEST_CASE("WebGLInfo object literals create own data properties under prototype pollution", "[renderers][webgl-info]") {
    for(const bool readOnly:{false,true}) {
        auto gl=infoGL(),trace=JSValue::array();InfoPrototypeScope scope(trace,readOnly);
        WebGLInfo info(gl);const auto object=info.object(),memory=info.memory.value(),render=info.render.value();
        for(const auto& value:{object,memory,render}) {
            REQUIRE(value.getPrototype().sameValue(scope.prototype));
            const auto keys=value.keys();REQUIRE(keys.size()==(value.sameValue(object)?6:value.sameValue(memory)?2:5));
            for(const auto& key:keys) {
                const auto descriptor=value.getOwnPropertyDescriptor(key);REQUIRE(descriptor.has_value());
                REQUIRE(descriptor->value.has_value());REQUIRE(descriptor->writable.value_or(false));
                REQUIRE(descriptor->enumerable.value_or(false));REQUIRE(descriptor->configurable.value_or(false));
                REQUIRE_FALSE(descriptor->getter.has_value());REQUIRE_FALSE(descriptor->setter.has_value());
            }
        }
        REQUIRE(memory.get("geometries").number()==0);REQUIRE(memory.get("textures").number()==0);
        REQUIRE(info.programs.isNull());REQUIRE(info.autoReset.strictEquals(true));
        REQUIRE(info.update(6,4,2).isUndefined());REQUIRE(render.get("calls").number()==1);
        REQUIRE(render.get("triangles").number()==4);REQUIRE(info.reset().isUndefined());
        REQUIRE(render.get("calls").number()==0);REQUIRE(render.get("triangles").number()==0);
        info.programs=9;REQUIRE(info.programs.toNumber()==9);REQUIRE(trace.length()==0);
    }
}
