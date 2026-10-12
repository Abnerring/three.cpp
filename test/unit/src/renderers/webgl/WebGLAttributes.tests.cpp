#include "catch.hpp"
#include "renderers/webgl/WebGLAttributes.h"
#include "core/NativeArray.h"
#include "core/JSWeakMap.h"

namespace {
using Args=std::vector<JSValue>;
void property(JSValue object,const char*name,JSValue value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(name,d);}
JSValue noop(const JSValue&,const Args&,const Args&){return {};}
JSValue gl(){auto object=JSValue::object();property(object,"createBuffer",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return JSValue::object();}));
    for(const char*name:{"bindBuffer","bufferData","bufferSubData","deleteBuffer"})property(object,name,JSValue::function(noop));
    unsigned i=1;for(const char*name:{"FLOAT","HALF_FLOAT","UNSIGNED_SHORT","SHORT","UNSIGNED_INT","INT","BYTE","UNSIGNED_BYTE"})property(object,name,i++);return object;
}
JSValue attribute(){auto object=JSValue::object();property(object,"array",NativeArray_detail::wrap(Float32Array{1,2,3,4}));property(object,"version",0);property(object,"usage",35044);property(object,"updateRanges",JSValue::array());
    property(object,"onUploadCallback",JSValue::function(noop));property(object,"clearUpdateRanges",JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{auto ranges=self.get("updateRanges");ranges.set("length",0);return {};}));return object;
}
JSValue range(double start,double count){auto result=JSValue::object();property(result,"start",start);property(result,"count",count);return result;}
}
TEST_CASE("WebGLAttributes interleaved views share cache only through data identity","[webgl-attributes]"){
    WebGLAttributes attributes(gl());auto source=attribute(),first=JSValue::object(),second=JSValue::object();
    for(auto&view:{first,second}){property(view,"isInterleavedBufferAttribute",true);property(view,"data",source);}
    attributes.update(first,1);auto cached=attributes.get(first);REQUIRE(cached.sameValue(attributes.get(second)));REQUIRE(cached.sameValue(attributes.get(source)));
    source.set("version",1);attributes.update(second,1);REQUIRE(cached.sameValue(attributes.get(source)));REQUIRE(cached.get("version").number()==1);
    attributes.remove(first);REQUIRE(attributes.get(second).isUndefined());attributes.update(second,1);REQUIRE_FALSE(cached.sameValue(attributes.get(source)));
}
TEST_CASE("WebGLAttributes has real supported types and rejects Float64 after upload","[webgl-attributes]"){
    for(unsigned kind=0;kind<9;++kind){WebGLAttributes attributes(gl());auto source=attribute();source.set("array",NativeArray_detail::constructor(kind).construct({4}));
        if(kind==7){REQUIRE_THROWS_AS(attributes.update(source,1),std::runtime_error);REQUIRE(attributes.get(source).isUndefined());}
        else{attributes.update(source,1);REQUIRE(attributes.get(source).get("bytesPerElement").sameValue(source.get("array").get("BYTES_PER_ELEMENT")));}
    }
    WebGLAttributes attributes(gl());auto source=attribute();source.set("array",NativeArray_detail::wrap(Uint16Array{1,2}));property(source,"isFloat16BufferAttribute",true);attributes.update(source,1);REQUIRE(attributes.get(source).get("type").number()==2);
}
TEST_CASE("WebGLAttributes merges sorted ranges in place and clears after command submission","[webgl-attributes]"){
    auto api=gl(),events=JSValue::array();property(api,"bufferSubData",JSValue::function(+[](const JSValue&,const Args&args,const Args&cap)->JSValue{
        auto events=cap[0];auto row=JSValue::array();for(unsigned i=0;i<args.size();++i)row.set(std::to_string(i),args[i]);events.set(std::to_string(events.length()),row);return {};
    },{events}));
    WebGLAttributes attributes(api);auto source=attribute();attributes.update(source,1);auto ranges=source.get("updateRanges");auto first=range(5,1),second=range(0,1),third=range(2,1);ranges.set("0",first);ranges.set("1",second);ranges.set("2",third);source.set("version",1);attributes.update(source,1);
    REQUIRE(events.length()==2);REQUIRE(events.get("0").get("1").number()==0);REQUIRE(events.get("0").get("4").number()==3);REQUIRE(events.get("1").get("1").number()==20);REQUIRE(second.get("count").number()==3);REQUIRE(ranges.length()==0);
}
TEST_CASE("WebGLAttributes upload exceptions leave initial cache absent and old version intact","[webgl-attributes]"){
    WebGLAttributes attributes(gl());auto source=attribute();source.set("onUploadCallback",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("upload");}));
    REQUIRE_THROWS_AS(attributes.update(source,1),JSRangeError);REQUIRE(attributes.get(source).isUndefined());source.set("onUploadCallback",JSValue::function(noop));attributes.update(source,1);auto cached=attributes.get(source);source.set("version",1);
    source.set("onUploadCallback",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("upload");}));REQUIRE_THROWS_AS(attributes.update(source,1),JSRangeError);REQUIRE(cached.get("version").number()==0);
}
TEST_CASE("WebGLAttributes cache values pointing back to attributes do not make weak keys live","[webgl-attributes][gc]"){
    auto witness=JSValue::object();WebGLAttributes attributes(gl());JSWeakValue key,value,buffer;
    {auto source=attribute();attributes.update(source,1);auto cached=attributes.get(source);cached.set("owner",source);key=JSWeakValue(source);value=JSWeakValue(cached);buffer=JSWeakValue(cached.get("buffer"));}
    witness.collectGarbage();REQUIRE(key.expired());REQUIRE(value.expired());REQUIRE(buffer.expired());
}
TEST_CASE("WebGLAttributes unreachable cache cannot retain values even with rooted attribute","[webgl-attributes][gc]"){
    auto witness=JSValue::object(),source=attribute();JSWeakValue value,manager;
    {WebGLAttributes attributes(gl());attributes.update(source,1);auto cached=attributes.get(source);cached.set("manager",attributes.object());value=JSWeakValue(cached);manager=JSWeakValue(attributes.object());}
    witness.collectGarbage();REQUIRE(value.expired());REQUIRE(manager.expired());REQUIRE(source.get("version").number()==0);
}
TEST_CASE("WebGLAttributes GL buffer cache retains raw fields and replaces only on newer version","[webgl-attributes]"){
    WebGLAttributes attributes(gl());auto source=JSValue::object(),buffer=JSValue::object();property(source,"isGLBufferAttribute",true);property(source,"buffer",buffer);property(source,"type","native");property(source,"elementSize","4");property(source,"version","2");
    attributes.update(source,1);auto first=attributes.get(source);REQUIRE(first.get("buffer").sameValue(buffer));REQUIRE(first.get("bytesPerElement").string()=="4");REQUIRE_FALSE(first.hasOwn("size"));
    source.set("version","10");attributes.update(source,1);REQUIRE(first.sameValue(attributes.get(source)));source.set("version",3);attributes.update(source,1);REQUIRE_FALSE(first.sameValue(attributes.get(source)));
}
TEST_CASE("WebGLAttributes returned declarations are ordinary constructible functions","[webgl-attributes]"){
    WebGLAttributes attributes(gl());auto source=attribute();
    for(const char*name:{"get","remove","update"}){const auto method=attributes.object().get(name);REQUIRE(method.isConstructible());REQUIRE(method.ownPropertyNames()==std::vector<std::string>{"length","name","prototype"});
        const auto descriptor=method.getOwnPropertyDescriptor("prototype");REQUIRE(descriptor->writable==true);REQUIRE(descriptor->enumerable==false);REQUIRE(descriptor->configurable==false);REQUIRE(method.get("prototype").get("constructor").sameValue(method));}
    const auto get=attributes.object().get("get"),update=attributes.object().get("update"),remove=attributes.object().get("remove");
    auto empty=get.construct({source});REQUIRE(empty.getPrototype().sameValue(get.get("prototype")));
    auto updated=update.construct({source,1});REQUIRE(updated.getPrototype().sameValue(update.get("prototype")));auto cached=attributes.get(source);REQUIRE(get.construct({source}).sameValue(cached));
    auto deleted=remove.construct({source});REQUIRE(deleted.getPrototype().sameValue(remove.get("prototype")));REQUIRE(attributes.get(source).isUndefined());
}
