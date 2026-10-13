#include "catch.hpp"
#include "renderers/webgl/WebGLCapabilities.h"
#include "renderers/webgl/WebGLExtensions.h"
#include "renderers/webgl/WebGLInfo.h"
#include "core/JSIterator.h"
#include "utils.h"
#include <thread>
#include <sstream>
#include <iostream>

namespace {
using Args=std::vector<JSValue>;
void data(JSValue object,const char* name,JSValue value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(name,d);}
struct Context {
    JSValue gl=JSValue::object(),extension=JSValue::object(),parameters=JSValue::object(),utils=JSValue::object(),state=JSValue::object(nullptr);
    Context(){
        state.set("queries",0);state.set("anisotropy",16);state.set("supported",true);
        gl.set("getParameter",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue {
            REQUIRE(self.sameValue(c[1]));auto s=c[0];s.set("queries",s.get("queries").toNumber()+1);
            return args[0].strictEquals("anisotropy")?s.get("anisotropy"):args[0];
        },{state,gl}));
        gl.set("getShaderPrecisionFormat",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{auto r=JSValue::object();r.set("precision",23);return r;}));
        gl.set("VERTEX_SHADER",1);gl.set("FRAGMENT_SHADER",2);gl.set("HIGH_FLOAT",3);gl.set("MEDIUM_FLOAT",4);
        int index=1;for(const auto* name:{"MAX_TEXTURE_IMAGE_UNITS","MAX_VERTEX_TEXTURE_IMAGE_UNITS","MAX_TEXTURE_SIZE","MAX_CUBE_MAP_TEXTURE_SIZE","MAX_VERTEX_ATTRIBS","MAX_VERTEX_UNIFORM_VECTORS","MAX_VARYING_VECTORS","MAX_FRAGMENT_UNIFORM_VECTORS","MAX_SAMPLES","SAMPLES"})gl.set(name,index++);
        gl.set("IMPLEMENTATION_COLOR_READ_FORMAT",1023);gl.set("IMPLEMENTATION_COLOR_READ_TYPE",1009);
        extension.set("has",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0].get("supported");},{state}));
        extension.set("get",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{auto r=JSValue::object();r.set("MAX_TEXTURE_MAX_ANISOTROPY_EXT","anisotropy");return r;}));
        utils.set("convert",JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue{return a[0];}));
    }
};
struct ConsoleReset { ~ConsoleReset(){ThreeUtils::setConsoleFunction(nullptr);} };
}
TEST_CASE("WebGL extensions retain actual identity and distinguish null from undefined", "[webgl-capabilities]"){
    auto state=JSValue::object(nullptr),gl=JSValue::object(),resource=JSValue::object();state.set("calls",0);state.set("resource",resource);
    gl.set("getExtension",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{REQUIRE(self.sameValue(c[1]));auto s=c[0];s.set("calls",s.get("calls").toNumber()+1);return s.get("resource");},{state,gl}));
    WebGLExtensions extensions(gl);
    REQUIRE(extensions.get("present").sameValue(resource));state.set("resource",nullptr);
    REQUIRE(extensions.get("present").sameValue(resource));REQUIRE(state.get("calls").strictEquals(1));
    REQUIRE_FALSE(extensions.has("absent").toBoolean());REQUIRE_FALSE(extensions.has("absent").toBoolean());REQUIRE(state.get("calls").strictEquals(2));
    state.set("resource",JSValue());REQUIRE(extensions.has("undefined").toBoolean());REQUIRE(extensions.has("undefined").toBoolean());REQUIRE(state.get("calls").strictEquals(4));
    REQUIRE(extensions.get("__proto__").sameValue(JSValue::object().getPrototype()));REQUIRE(state.get("calls").strictEquals(4));
}
TEST_CASE("WebGL capability values preserve strict booleans and lazy anisotropy cache", "[webgl-capabilities]"){
    Context c;c.parameters.set("logarithmicDepthBuffer",1);c.parameters.set("reversedDepthBuffer",true);c.state.set("supported","enabled");
    WebGLCapabilities caps(c.gl,c.extension,c.parameters,c.utils);
    REQUIRE(caps.precision.value().strictEquals("highp"));REQUIRE(caps.logarithmicDepthBuffer.value().strictEquals(false));REQUIRE(caps.reversedDepthBuffer.value().strictEquals("enabled"));
    REQUIRE(c.state.get("queries").strictEquals(10));REQUIRE(caps.getMaxAnisotropy().strictEquals(0));REQUIRE(c.state.get("queries").strictEquals(10));
    c.state.set("supported",true);REQUIRE(caps.getMaxAnisotropy().strictEquals(0));
    WebGLCapabilities other(c.gl,c.extension,c.parameters,c.utils);c.state.set("anisotropy",JSValue());
    REQUIRE(other.getMaxAnisotropy().isUndefined());REQUIRE(other.getMaxAnisotropy().isUndefined());REQUIRE(c.state.get("queries").strictEquals(22));
    c.state.set("anisotropy",nullptr);REQUIRE(other.getMaxAnisotropy().isNull());c.state.set("anisotropy",32);REQUIRE(other.getMaxAnisotropy().isNull());REQUIRE(c.state.get("queries").strictEquals(23));
    REQUIRE(caps.textureFormatReadable(1023).strictEquals(true));REQUIRE(caps.textureTypeReadable(1016).strictEquals(true));
}
TEST_CASE("WebGL source closure methods retain managed cycles only while reachable", "[webgl-capabilities][gc]"){
    auto heap=JSValue::object();JSWeakValue weakGl,weakResult;JSValue keep;
    {
        Context c;auto caps=WebGLCapabilities(c.gl,c.extension,c.parameters,c.utils).object();c.gl.set("owner",caps);
        weakGl=JSWeakValue(c.gl);weakResult=JSWeakValue(caps);keep=caps.get("getMaxAnisotropy");
    }
    heap.collectGarbage();REQUIRE_FALSE(weakGl.expired());REQUIRE_FALSE(weakResult.expired());
    keep={};heap.collectGarbage();REQUIRE(weakGl.expired());REQUIRE(weakResult.expired());
}
TEST_CASE("WebGL wrapper foreign thread validation precedes method calls and moving", "[webgl-capabilities][thread]"){
    Context c;WebGLCapabilities caps(c.gl,c.extension,c.parameters,c.utils);WebGLExtensions ext(c.gl);bool a=false,b=false,d=false;
    std::thread other([&]{try{caps.getMaxPrecision("highp");}catch(const std::logic_error&){a=true;}try{auto copy=WebGLExtensions::fromObject(ext.object());}catch(const std::logic_error&){b=true;}try{WebGLCapabilities moved(std::move(caps));}catch(const std::logic_error&){d=true;}});other.join();
    REQUIRE(a);REQUIRE(b);REQUIRE(d);REQUIRE(caps.getMaxPrecision("highp").strictEquals("highp"));
}
TEST_CASE("Warnings share cache and console state and cache before a throwing callback", "[webgl-capabilities][utils]"){
    ConsoleReset reset;auto state=JSValue::object(nullptr);state.set("count",0);
    WebGLExtensions::setConsoleFunction(JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{REQUIRE(self.isUndefined());REQUIRE(a[0].strictEquals("warn"));auto s=c[0];s.set("count",s.get("count").toNumber()+1);throw JSRangeError("warning-handler");},{state}));
    REQUIRE(WebGLCapabilities::getConsoleFunction().sameValue(WebGLInfo::getConsoleFunction()));
    auto gl=JSValue::object();gl.set("getExtension",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return nullptr;}));
    WebGLExtensions a(gl),b(gl);
    REQUIRE_THROWS_AS(a.get("unit-shared-warning"),JSRangeError);
    REQUIRE(b.get("unit-shared-warning").isNull());
    ThreeUtils::warnOnce({"WebGLRenderer: unit-shared-warning extension not supported."});
    REQUIRE(state.get("count").strictEquals(1));
}
TEST_CASE("Warnings call the live string startsWith method and real array iterator", "[webgl-capabilities][utils]"){
    ConsoleReset reset;auto prototype=nativeStringPrototype(),old=prototype.get("startsWith");
    struct Restore {JSValue prototype,old;~Restore(){prototype.set("startsWith",old);}}restore{prototype,old};
    auto state=JSValue::object(nullptr);state.set("starts",0);state.set("warnings",0);
    prototype.set("startsWith",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{REQUIRE(self.strictEquals("native-warning"));REQUIRE(a[0].strictEquals("TSL:"));auto s=c[0];s.set("starts",s.get("starts").toNumber()+1);return false;},{state}));
    ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{auto s=c[0];s.set("warnings",s.get("warnings").toNumber()+1);REQUIRE(a.size()==4);REQUIRE(a[1].strictEquals("THREE.native-warning"));REQUIRE(a[2].strictEquals(7));REQUIRE(a[3].strictEquals(8));return {};},{state}));
    ThreeUtils::warn({"native-warning",7,8});REQUIRE(state.get("starts").strictEquals(1));REQUIRE(state.get("warnings").strictEquals(1));
}

TEST_CASE("Native warning sink waits until spread arguments have completed", "[webgl-capabilities][utils]"){
    ConsoleReset reset;
    auto prototype=JSValue::array().getPrototype();
    const auto symbol=JSValue::wellKnownSymbol("iterator"),old=prototype.get(symbol);
    std::ostringstream captured;
    struct Restore {
        JSValue prototype,symbol,old;std::streambuf* buffer;
        ~Restore(){prototype.set(symbol,old);std::cerr.rdbuf(buffer);}
    } restore{prototype,symbol,old,std::cerr.rdbuf(captured.rdbuf())};
    ThreeUtils::setConsoleFunction(nullptr);
    prototype.set(symbol,JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("spread-failure");}));
    REQUIRE_THROWS_AS(ThreeUtils::warn({"sink-order",7}),JSRangeError);
    REQUIRE(captured.str().empty());
}
