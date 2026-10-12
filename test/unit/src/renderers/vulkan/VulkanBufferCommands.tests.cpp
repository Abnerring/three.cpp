#include "catch.hpp"
#include "renderers/vulkan/VulkanBufferCommands.h"
#include "renderers/webgl/WebGLAttributes.h"
#include "core/NativeArray.h"
#include <thread>

namespace {
using Args = std::vector<JSValue>;
struct Memory final : VulkanBufferCommands::Storage {
    std::vector<std::byte> bytes;
    std::size_t writes = 0;
    bool fail = false;
    void setData(std::span<const std::byte> value) override {
        if (fail) throw JSRangeError("Storage write failed");bytes.assign(value.begin(), value.end());++writes;
    }
    void setSubData(std::size_t offset, std::span<const std::byte> value) override {
        if (fail) throw JSRangeError("Storage write failed");
        REQUIRE(offset <= bytes.size());REQUIRE(value.size() <= bytes.size()-offset);
        std::copy(value.begin(), value.end(), bytes.begin()+offset);++writes;
    }
    std::vector<std::byte> read() const override { return bytes; }
    std::size_t size() const override { return bytes.size(); }
};
JSValue call(JSValue gl,const char* name,const Args& args={}) { return gl.get(name).call(gl,args); }
JSValue tuple(const Args& args){auto v=JSValue::array();v.get("push").call(v,args);return v;}
JSValue range(JSValue start,JSValue count){auto v=JSValue::object();v.set("start",start);v.set("count",count);return v;}
JSValue attribute(JSValue array){
    auto a=JSValue::object();a.set("array",array);a.set("usage",35044);a.set("version",0);a.set("updateRanges",JSValue::array());
    a.set("onUploadCallback",JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{auto a=self;a.set("uploads",a.get("uploads").toNumber()+1);return {};}));a.set("uploads",0);
    a.set("clearUpdateRanges",JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{auto ranges=self.get("updateRanges");ranges.set("length",0);return {};}));return a;
}
std::vector<int> numbers(const std::vector<std::byte>& bytes){std::vector<int> out;for(auto b:bytes)out.push_back(std::to_integer<int>(b));return out;}
struct Fixture {
    std::vector<std::weak_ptr<Memory>> created;
    VulkanBufferCommands commands{[this]{auto result=std::make_shared<Memory>();created.emplace_back(result);return result;}};
    JSValue gl=commands.object();
    JSValue create(std::size_t size=8){auto value=call(gl,"createBuffer");call(gl,"bindBuffer",{34962,value});call(gl,"bufferData",{34962,NativeArray_detail::wrap(Uint8Array(size)),35044});return value;}
};
}
TEST_CASE("Vulkan attribute byte commands use real view offsets and retained storage format", "[vulkan-buffer-commands]") {
    Fixture f;auto b=f.create();Uint8Array backing{99,99,1,2,3,4,5,6,7,8,99,99};
    auto source=NativeArray_detail::wrap(backing.subarray(2,10));
    JSPropertyDescriptor fake;fake.value=99;fake.writable=true;fake.configurable=true;
    source.defineProperty("byteOffset",fake);source.defineProperty("byteLength",fake);source.defineProperty("length",fake);source.defineProperty("BYTES_PER_ELEMENT",fake);
    call(f.gl,"bufferData",{34962,source,35044});
    REQUIRE(numbers(f.commands.read(b))==std::vector<int>{1,2,3,4,5,6,7,8});REQUIRE(f.commands.revision(b)==2);
    call(f.gl,"bufferSubData",{34962,1,source,2,2});
    REQUIRE(numbers(f.commands.read(b))==std::vector<int>{1,3,4,4,5,6,7,8});
    REQUIRE(std::uint8_t(backing[0])==99);REQUIRE(std::uint8_t(backing[10])==99);
    Float64Array doubles{1.25};call(f.gl,"bufferData",{34962,NativeArray_detail::wrap(doubles),35044});
    REQUIRE(f.commands.read(b).size()==8);REQUIRE(call(f.gl,"getError").number()==0);
}
TEST_CASE("Vulkan byte commands convert offsets separately and interpret zero length as remainder", "[vulkan-buffer-commands]") {
    Fixture f;auto b=f.create();Uint16Array values{0x0102,0x0304,0x0506,0x0708};auto source=NativeArray_detail::wrap(values);
    call(f.gl,"bufferSubData",{34962,1,source,.5,1});
    REQUIRE(numbers(f.commands.read(b))==std::vector<int>{0,2,1,0,0,0,0,0});
    call(f.gl,"bufferSubData",{34962,4,source,2,0});
    REQUIRE(numbers(f.commands.read(b))==std::vector<int>{0,2,1,0,6,5,8,7});
    const auto rev=f.commands.revision(b);call(f.gl,"bufferSubData",{34962,8,source,4,0});
    REQUIRE(f.commands.revision(b)==rev);REQUIRE(call(f.gl,"getError").number()==0);
    call(f.gl,"bufferSubData",{34962,0,source,0,4294967296.0});REQUIRE(numbers(f.commands.read(b))==std::vector<int>{2,1,4,3,6,5,8,7});
    call(f.gl,"bufferSubData",{34962,0,source,0,-1});REQUIRE(call(f.gl,"getError").number()==1281);
    call(f.gl,"bufferSubData",{34962,-1,source,0,1});REQUIRE(call(f.gl,"getError").number()==1281);
    call(f.gl,"bufferSubData",{34962,0,source,-1,1});REQUIRE(call(f.gl,"getError").number()==1281);
    call(f.gl,"bufferData",{34962,NativeArray_detail::wrap(Uint8Array(0)),35044});REQUIRE(f.commands.read(b).empty());
    call(f.gl,"bufferSubData",{34962,0,NativeArray_detail::wrap(Uint8Array(0)),0,0});REQUIRE(call(f.gl,"getError").number()==0);
}
TEST_CASE("Vulkan byte command API errors are sticky no-ops and distinct from exceptions", "[vulkan-buffer-commands]") {
    Fixture f;auto b=f.create();const auto before=f.commands.read(b);const auto source=NativeArray_detail::wrap(Uint8Array{7});
    call(f.gl,"bufferSubData",{34962,99,source});call(f.gl,"bufferSubData",{34962,-1,source});
    call(f.gl,"bufferSubData",{99,0,source});
    REQUIRE(f.commands.read(b)==before);REQUIRE(f.commands.revision(b)==1);
    REQUIRE(call(f.gl,"getError").number()==1280);REQUIRE(call(f.gl,"getError").number()==1281);REQUIRE(call(f.gl,"getError").number()==0);
    auto bad=JSValue::object();bad.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("conversion");}));
    REQUIRE_THROWS_AS(call(f.gl,"bufferSubData",{34962,0,source,bad,1}),JSRangeError);REQUIRE(call(f.gl,"getError").number()==0);
    call(f.gl,"bindBuffer",{34962,nullptr});call(f.gl,"bufferSubData",{34962,0,source});REQUIRE(call(f.gl,"getError").number()==1282);
    call(f.gl,"bindBuffer",{34962,b});call(f.gl,"bufferData",{34962,source,0});REQUIRE(call(f.gl,"getError").number()==1280);
    REQUIRE(f.commands.read(b)==before);
}
TEST_CASE("Vulkan byte commands resolve binding and source contents after numeric conversion reentry", "[vulkan-buffer-commands]") {
    Fixture f;auto first=f.create(),second=f.create(),events=JSValue::array();Uint8Array values{1,2,3,4};auto source=NativeArray_detail::wrap(values);
    call(f.gl,"bindBuffer",{34962,first});auto offset=JSValue::object(),length=JSValue::object();
    offset.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
        c[3].get("push").call(c[3],{args[0]});call(c[0],"bindBuffer",{34962,c[1]});auto v=c[2];v.set("0",9);return 0;
    },{f.gl,second,source,events}));
    length.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{c[0].collectGarbage();return 1;},{f.gl}));
    call(f.gl,"bufferSubData",{34962,0,source,offset,length});
    REQUIRE(numbers(f.commands.read(first))==std::vector<int>(8,0));REQUIRE(numbers(f.commands.read(second))==std::vector<int>{9,0,0,0,0,0,0,0});
    REQUIRE(events.get("0").string()=="number");
    offset.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"deleteBuffer",{c[1]});return 0;},{f.gl,second}));
    call(f.gl,"bufferSubData",{34962,0,source,offset,1});REQUIRE(call(f.gl,"getError").number()==1282);
    REQUIRE_THROWS_AS(f.commands.read(second),JSTypeError);
}
TEST_CASE("WebGLAttributes keeps successful writes across later API errors and conversion exceptions", "[vulkan-buffer-commands][webgl-attributes]") {
    Fixture f;WebGLAttributes manager(f.gl);Uint8Array values(12);auto a=attribute(NativeArray_detail::wrap(values));manager.update(a,34962);
    auto b=manager.get(a).get("buffer");for(std::size_t i=0;i<12;++i)values[i]=double(i+1);
    // Sorting keeps these separate. A middle invalid range does not stop later
    // writes, clear, callback or version advancement.
    a.set("updateRanges",tuple({range(0,1),range(4,-1),range(8,1)}));a.set("version",1);manager.update(a,34962);
    REQUIRE(numbers(f.commands.read(b))==std::vector<int>{1,0,0,0,0,0,0,0,9,0,0,0});
    REQUIRE(call(f.gl,"getError").number()==1281);REQUIRE(a.get("updateRanges").length()==0);
    REQUIRE(a.get("uploads").number()==2);REQUIRE(manager.get(a).get("version").number()==1);
    auto first=range(0,1),second=range(4,1),start=JSValue::object(),counter=JSValue::object();counter.set("reads",0);
    // A custom sort keeps the fixture focused on upload-time conversion. A start
    // getter switches to a throwing converter only for source-offset conversion.
    auto ranges=tuple({first,second});ranges.set("sort",JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{return self;}));
    JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        auto counter=c[0];auto n=counter.get("reads").number()+1;counter.set("reads",n);return n>=3?c[1]:JSValue(4);
    },{counter,start});d.configurable=true;second.defineProperty("start",d);
    start.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("source offset");}));
    values[0]=11;values[4]=22;a.set("updateRanges",ranges);a.set("version",2);
    REQUIRE_THROWS_AS(manager.update(a,34962),JSRangeError);
    REQUIRE(numbers(f.commands.read(b))==std::vector<int>{11,0,0,0,0,0,0,0,9,0,0,0});
    REQUIRE(a.get("updateRanges").length()==2);REQUIRE(a.get("uploads").number()==2);REQUIRE(manager.get(a).get("version").number()==1);
    REQUIRE(call(f.gl,"getError").number()==0);
}
TEST_CASE("Vulkan command storage exceptions preserve revision and previous bytes", "[vulkan-buffer-commands]") {
    Fixture f;auto b=f.create();auto storage=f.created.back().lock();storage->fail=true;
    REQUIRE_THROWS_AS(call(f.gl,"bufferSubData",{34962,0,NativeArray_detail::wrap(Uint8Array{9})}),JSRangeError);
    REQUIRE(f.commands.revision(b)==1);REQUIRE(numbers(f.commands.read(b))==std::vector<int>(8,0));REQUIRE(call(f.gl,"getError").number()==0);
    storage->fail=false;call(f.gl,"bufferSubData",{34962,0,NativeArray_detail::wrap(Uint8Array{9})});REQUIRE(f.commands.revision(b)==2);
}
TEST_CASE("Vulkan buffer deletion and shutdown invalidate escaped handles without device access", "[vulkan-buffer-commands]") {
    Fixture f;auto first=f.create(),second=f.create();auto weakFirst=f.created[0],weakSecond=f.created[1];
    call(f.gl,"deleteBuffer",{first});REQUIRE(weakFirst.expired());call(f.gl,"deleteBuffer",{first});
    REQUIRE_THROWS_AS(f.commands.revision(first),JSTypeError);REQUIRE(call(f.gl,"getError").number()==0);
    call(f.gl,"bindBuffer",{34962,first});REQUIRE(call(f.gl,"getError").number()==1282);
    auto detached=f.gl.get("bufferSubData");f.commands.shutdown();f.commands.shutdown();REQUIRE(weakSecond.expired());
    REQUIRE_THROWS_AS(detached.call(nullptr,{34962,0,NativeArray_detail::wrap(Uint8Array{1})}),JSTypeError);
    REQUIRE_THROWS_AS(call(f.gl,"createBuffer"),JSTypeError);REQUIRE_THROWS_AS(f.commands.read(second),JSTypeError);
    first=second=JSValue();f.gl.collectGarbage();
}
TEST_CASE("Vulkan command bindings participate in native and managed cycle collection", "[vulkan-buffer-commands][gc]") {
    auto witness=JSValue::object();JSWeakValue weakGl,weakBuffer;std::weak_ptr<Memory> weakStorage;
    {
        VulkanBufferCommands commands([&]{auto storage=std::make_shared<Memory>();weakStorage=storage;return storage;});auto gl=commands.object();
        auto b=call(gl,"createBuffer");call(gl,"bindBuffer",{34962,b});b.set("back",gl);weakGl=JSWeakValue(gl);weakBuffer=JSWeakValue(b);
    }
    witness.collectGarbage();REQUIRE(weakGl.expired());REQUIRE(weakBuffer.expired());REQUIRE(weakStorage.expired());
    Fixture f;JSWeakValue boundHandle;
    {auto b=f.create();boundHandle=JSWeakValue(b);}
    witness.collectGarbage();REQUIRE_FALSE(boundHandle.expired());call(f.gl,"bindBuffer",{34962,nullptr});witness.collectGarbage();REQUIRE(boundHandle.expired());
}
TEST_CASE("Vulkan command contexts reject foreign handles and invalid element binding classes", "[vulkan-buffer-commands]") {
    Fixture f,g;auto first=f.create();call(g.gl,"bindBuffer",{34962,first});REQUIRE(call(g.gl,"getError").number()==1282);
    call(g.gl,"deleteBuffer",{first});REQUIRE(call(g.gl,"getError").number()==1282);REQUIRE(f.commands.read(first).size()==8);
    REQUIRE_THROWS_AS(g.commands.read(first),JSTypeError);
    call(f.gl,"bindBuffer",{34963,first});REQUIRE(call(f.gl,"getError").number()==1282);
    REQUIRE_THROWS_AS(call(f.gl,"bindBuffer",{34962,JSValue::object()}),JSTypeError);
    bool rejected=false,foreignState=false,foreignBuffer=false;
    std::thread worker([&]{
        try{static_cast<void>(f.commands.object());}catch(const JSTypeError&){rejected=true;}
        try{static_cast<void>(f.commands.revision(first));}catch(const JSTypeError&){foreignState=true;}
        VulkanBufferCommands local([]{return std::make_shared<Memory>();});
        try{static_cast<void>(local.read(first));}catch(const JSTypeError&){foreignBuffer=true;}
    });worker.join();REQUIRE(rejected);REQUIRE(foreignState);REQUIRE(foreignBuffer);
}
TEST_CASE("Vulkan buffer create allocation failure records OUT_OF_MEMORY", "[vulkan-buffer-commands]") {
    VulkanBufferCommands commands([]()->std::shared_ptr<VulkanBufferCommands::Storage>{throw std::bad_alloc();});auto gl=commands.object();
    REQUIRE(call(gl,"createBuffer").isNull());REQUIRE(call(gl,"getError").number()==1285);REQUIRE(call(gl,"getError").number()==0);
}
TEST_CASE("Float64 byte upload and callback occur before WebGLAttributes rejects the format", "[vulkan-buffer-commands][webgl-attributes]") {
    Fixture f;WebGLAttributes manager(f.gl);auto a=attribute(NativeArray_detail::wrap(Float64Array{1,2}));
    REQUIRE_THROWS(manager.update(a,34962));REQUIRE(a.get("uploads").number()==1);REQUIRE(manager.get(a).isUndefined());
    REQUIRE(f.created.size()==1);auto storage=f.created[0].lock();REQUIRE(storage);REQUIRE(storage->size()==16);REQUIRE(storage->writes==1);
    REQUIRE(call(f.gl,"getError").number()==0);
}
TEST_CASE("A factory may close its command owner without destroying its active callable", "[vulkan-buffer-commands]") {
    std::unique_ptr<VulkanBufferCommands> commands;std::weak_ptr<Memory> weak;bool returned=false;
    commands=std::make_unique<VulkanBufferCommands>([&]{commands->shutdown();auto value=std::make_shared<Memory>();weak=value;returned=true;return value;});
    auto gl=commands->object();REQUIRE_THROWS_AS(call(gl,"createBuffer"),JSTypeError);REQUIRE(returned);REQUIRE(weak.expired());
    REQUIRE_THROWS_AS(call(gl,"getError"),JSTypeError);
}
