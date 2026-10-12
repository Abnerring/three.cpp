#include "catch.hpp"
#include "core/GLBufferAttribute.h"
#include <cmath>
#include <limits>
#include <thread>

namespace {
using Args = std::vector<JSValue>;
JSValue noop(const JSValue&, const Args&, const Args&) { return {}; }
void push(const JSValue& array, const JSValue& value) { array.get("push").call(array, {value}); }
JSPropertyDescriptor data(const JSValue& value, bool writable = true) {
    JSPropertyDescriptor d; d.value=value; d.writable=writable; d.configurable=d.enumerable=true; return d;
}
}
TEST_CASE("GLBufferAttribute exposes the exact default fields and class descriptors", "[core][glbuffer]") {
    GLBufferAttribute attribute; const auto object=attribute.object(), constructor=attribute.constructor(), prototype=attribute.prototype();
    REQUIRE(object.keys()==std::vector<std::string>{"isGLBufferAttribute","name","buffer","type","itemSize","elementSize","count","normalized","version"});
    REQUIRE(attribute.isGLBufferAttribute.strictEquals(true)); REQUIRE(attribute.name.strictEquals(""));
    for(const auto* name:{"buffer","type","itemSize","elementSize","count"}) REQUIRE(attribute.get(name).isUndefined());
    REQUIRE(attribute.normalized.strictEquals(false)); REQUIRE(attribute.version.strictEquals(0)); REQUIRE(attribute.needsUpdate.isUndefined());
    REQUIRE_FALSE(object.hasOwn("needsUpdate")); REQUIRE_FALSE(object.has("array"));
    for(const auto& name:object.ownPropertyNames()) {const auto d=object.getOwnPropertyDescriptor(name);REQUIRE(d->writable==true);REQUIRE(d->enumerable==true);REQUIRE(d->configurable==true);}
    REQUIRE(constructor.get("length").strictEquals(5)); REQUIRE(constructor.get("name").strictEquals("GLBufferAttribute"));
    const auto pd=constructor.getOwnPropertyDescriptor("prototype"); REQUIRE(pd->writable==false); REQUIRE(pd->enumerable==false); REQUIRE(pd->configurable==false);
    REQUIRE(prototype.ownPropertyNames()==std::vector<std::string>{"constructor","needsUpdate","setBuffer","setType","setItemSize","setCount"});
    auto nd=prototype.getOwnPropertyDescriptor("needsUpdate");REQUIRE(nd->getter->isUndefined());REQUIRE(nd->setter->get("name").strictEquals("set needsUpdate"));REQUIRE(nd->enumerable==false);REQUIRE(nd->configurable==true);
    REQUIRE_THROWS_AS(constructor.call(),JSTypeError); REQUIRE_THROWS_AS(prototype.get("setBuffer").construct(),JSTypeError);
}
TEST_CASE("GLBufferAttribute keeps arbitrary values and shared managed identity", "[core][glbuffer]") {
    auto resource=JSValue::object(), arbitrary=JSValue::array();GLBufferAttribute a(resource,"type",nullptr,-1,arbitrary,{});
    REQUIRE(a.buffer.strictEquals(resource));REQUIRE(a.count.strictEquals(arbitrary));REQUIRE(a.normalized.strictEquals(false));
    GLBufferAttribute b=a; REQUIRE(nativeGLBufferAttribute(a).sameValue(b.object())); b.name="edited";REQUIRE(a.name.strictEquals("edited"));
    GLBufferAttribute c(std::move(b));c.itemSize=arbitrary;REQUIRE(a.itemSize.strictEquals(arbitrary));
    auto alias=GLBufferAttribute::fromObject(a.object());alias.normalized=nullptr;REQUIRE(a.normalized.isNull());
    GLBufferAttribute d;d=a;d.count=17;REQUIRE(a.count.strictEquals(17));GLBufferAttribute e;e=std::move(d);e.type=false;REQUIRE(a.type.strictEquals(false));
    REQUIRE(a.setBuffer(arbitrary).sameValue(a.object()));REQUIRE(a.buffer.strictEquals(arbitrary));
    REQUIRE(a.setType(resource,arbitrary).sameValue(a.object()));REQUIRE(a.type.strictEquals(resource));REQUIRE(a.elementSize.strictEquals(arbitrary));
    REQUIRE(a.setItemSize(resource).sameValue(a.object())); REQUIRE(a.itemSize.strictEquals(resource));
    REQUIRE(a.setCount().sameValue(a.object())); REQUIRE(a.count.isUndefined()); REQUIRE(a.version.strictEquals(0));
    auto method=JSValue::function(+[](const JSValue& self,const Args& args,const Args&)->JSValue {auto value=self;value.set("seen",args[0]);return 71;});
    a.setProperty("setCount",method);REQUIRE(a.setCount(resource).strictEquals(71));REQUIRE(a.get("seen").sameValue(resource));
}
TEST_CASE("GLBufferAttribute needsUpdate checks strict true and performs numeric postfix conversion", "[core][glbuffer]") {
    GLBufferAttribute a; const Args ignored{{},nullptr,false,0,1,"true",JSValue::object(),JSValue::array(),JSValue::function(noop)};
    for(const auto& flag:ignored){a.version="7";a.needsUpdate=flag;REQUIRE(a.version.strictEquals("7"));}
    a.needsUpdate=true;REQUIRE(a.version.strictEquals(8));a.version=-0.0;a.needsUpdate=true;REQUIRE(a.version.strictEquals(1));
    a.version={};a.needsUpdate=true;REQUIRE(std::isnan(a.version.toNumber()));
    a.version=std::numeric_limits<double>::infinity();a.needsUpdate=true;REQUIRE(std::isinf(a.version.toNumber()));
    auto events=JSValue::array(),number=JSValue::object();
    number.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue {push(c[0],args[0]);return "41";},{events}));
    a.version=number;a.needsUpdate=true;REQUIRE(a.version.strictEquals(42));REQUIRE(events.get("0").strictEquals("number"));REQUIRE(events.length()==1);
    auto generic=JSValue::object();generic.set("version","5");const auto setter=a.prototype().getOwnPropertyDescriptor("needsUpdate")->setter.value();
    REQUIRE(setter.call(generic,{true}).isUndefined());REQUIRE(generic.get("version").strictEquals(6));REQUIRE(setter.call(nullptr,{false}).isUndefined());REQUIRE_THROWS_AS(setter.call(nullptr,{true}),JSTypeError);
}
TEST_CASE("GLBufferAttribute accessors keep evaluation and partial-write order", "[core][glbuffer]") {
    GLBufferAttribute a;auto self=a.object(),events=JSValue::array();
    JSPropertyDescriptor type;type.setter=JSValue::function(+[](const JSValue& receiver,const Args& args,const Args& c)->JSValue {push(c[0],"type");auto self=receiver;self.set("seen",args[0]);return {};},{events});type.configurable=true;self.defineProperty("type",type);
    self.defineProperty("elementSize",data(4,false));REQUIRE_THROWS_AS(a.setType(9,8),JSTypeError);REQUIRE(self.get("seen").strictEquals(9));REQUIRE(events.length()==1);REQUIRE(a.elementSize.strictEquals(4));
    JSPropertyDescriptor v;
    auto number=JSValue::object();number.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"convert");c[1].collectGarbage();return 3;},{events,self}));
    v.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"get");return c[1];},{events,number});
    v.setter=JSValue::function(+[](const JSValue& receiver,const Args& args,const Args& c)->JSValue {push(c[0],"set");auto self=receiver;self.set("seenVersion",args[0]);return {};},{events});v.configurable=true;self.defineProperty("version",v);
    a.needsUpdate=true;REQUIRE(events.get("1").strictEquals("get"));REQUIRE(events.get("2").strictEquals("convert"));REQUIRE(events.get("3").strictEquals("set"));REQUIRE(self.get("seenVersion").strictEquals(4));
}
TEST_CASE("GLBufferAttribute construction obeys newTarget and inherited assignment restrictions", "[core][glbuffer]") {
    auto ctor=GLBufferAttribute::constructor(),target=JSValue::constructor(noop,{},"Target",0,JSValue::ConstructorKind::Callable);
    auto custom=JSValue::object(),events=JSValue::array();
    JSPropertyDescriptor d;d.setter=JSValue::function(+[](const JSValue& receiver,const Args& args,const Args& c)->JSValue {push(c[0],"buffer");auto self=receiver;self.set("capturedBuffer",args[0]);return {};},{events});d.configurable=true;custom.defineProperty("buffer",d);
    target.set("prototype",custom);auto buffer=JSValue::object();auto derived=ctor.construct({buffer},target);
    REQUIRE(derived.getPrototype().sameValue(custom));REQUIRE_FALSE(derived.hasOwn("buffer"));REQUIRE(derived.get("capturedBuffer").sameValue(buffer));REQUIRE(events.length()==1);
    custom.defineProperty("type",data(7,false));REQUIRE_THROWS_AS(ctor.construct({buffer},target),JSTypeError);REQUIRE(events.length()==2);
    target.set("prototype",nullptr);auto fallback=ctor.construct({},target);REQUIRE(fallback.getPrototype().sameValue(JSValue::object().getPrototype()));REQUIRE_FALSE(fallback.has("setBuffer"));REQUIRE(fallback.get("isGLBufferAttribute").strictEquals(true));
    REQUIRE_THROWS_AS(ctor.construct({},JSValue::function(noop)),JSTypeError);
}
TEST_CASE("GLBufferAttribute opaque native handles remain shared and unrooted cycles collect", "[core][glbuffer][gc]") {
    auto witness=JSValue::object();auto native=std::make_shared<int>(73);std::weak_ptr<int> resource=native;JSWeakValue weakAttribute,weakBuffer;
    {auto buffer=JSValue::hostObject(native,native.get(),"test.OpaqueBuffer");GLBufferAttribute attribute(buffer);weakAttribute=JSWeakValue(attribute.object());weakBuffer=JSWeakValue(buffer);buffer.set("owner",attribute.object());
     REQUIRE(std::static_pointer_cast<int>(attribute.buffer.value().hostPayload("test.OpaqueBuffer")).get()==native.get());REQUIRE_FALSE(attribute.object().has("array"));}
    native.reset();witness.collectGarbage();REQUIRE(resource.expired());REQUIRE(weakBuffer.expired());REQUIRE(weakAttribute.expired());
}
TEST_CASE("GLBufferAttribute weak constructor cache does not keep an otherwise unused runtime alive", "[core][glbuffer][gc]") {
    bool expired=false,distinct=false;std::thread worker([&]{JSWeakValue weak;{GLBufferAttribute attribute;weak=JSWeakValue(attribute.object());}expired=weak.expired();GLBufferAttribute next;distinct=weak.expired();});worker.join();REQUIRE(expired);REQUIRE(distinct);
}
TEST_CASE("GLBufferAttribute module constructor preserves prototype changes within a live heap", "[core][glbuffer][module][gc]") {
    bool retained=false,expired=false,reset=false;
    std::exception_ptr error;
    std::thread worker([&] {
        try {
            JSWeakValue weakConstructor,weakPrototype;
            {
                auto witness=JSValue::object();
                {
                    auto constructor=GLBufferAttribute::constructor(),prototype=constructor.get("prototype");
                    prototype.set("moduleMarker",42);
                    weakConstructor=JSWeakValue(constructor);weakPrototype=JSWeakValue(prototype);
                }
                witness.collectGarbage();GLBufferAttribute next;
                retained=!weakConstructor.expired() && GLBufferAttribute::constructor().sameValue(*weakConstructor.lock()) &&
                    next.object().getPrototype().sameValue(*weakPrototype.lock()) && next.get("moduleMarker").strictEquals(42);
            }
            expired=weakConstructor.expired() && weakPrototype.expired();
            reset=GLBufferAttribute().get("moduleMarker").isUndefined();
        } catch (...) {error=std::current_exception();}
    });
    worker.join();if(error)std::rethrow_exception(error);
    REQUIRE(retained);REQUIRE(expired);REQUIRE(reset);
}
TEST_CASE("GLBufferAttribute method lookup can reenter and collect without losing receiver or arguments", "[core][glbuffer][gc]") {
    GLBufferAttribute attribute;auto events=JSValue::array(),resource=JSValue::object();
    auto replacement=JSValue::function(+[](const JSValue& receiver,const Args& args,const Args& c)->JSValue {
        push(c[0],"call");auto self=receiver;self.set("buffer",args[0]);self.collectGarbage();return self;
    },{events});
    JSPropertyDescriptor lookup;lookup.getter=JSValue::function(+[](const JSValue& receiver,const Args&,const Args& c)->JSValue {
        push(c[0],"get");auto self=receiver;self.erase("setBuffer");self.collectGarbage();return c[1];
    },{events,replacement});lookup.configurable=true;attribute.object().defineProperty("setBuffer",lookup);
    REQUIRE(attribute.setBuffer(resource).sameValue(attribute.object()));REQUIRE(attribute.buffer.strictEquals(resource));
    REQUIRE(events.get("0").strictEquals("get"));REQUIRE(events.get("1").strictEquals("call"));
    REQUIRE(attribute.object().get("setBuffer").sameValue(attribute.prototype().get("setBuffer")));
}
