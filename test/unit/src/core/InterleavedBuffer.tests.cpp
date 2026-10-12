#include "catch.hpp"
#include "core/InterleavedBuffer.h"
#include "core/InstancedInterleavedBuffer.h"

TEST_CASE("InterleavedBuffer preserves typed views and deduplicates only equal byte ranges","[core][interleaved]") {
    Float32Array data({1,2,3,4,5,6,7,8});const auto first=data.subarray(1,5),second=data.subarray(1,5),third=data.subarray(2,6);
    InterleavedBuffer a(first,2),b(second,4),c(third,2);auto metadata=JSValue::object();
    const auto ca=InterleavedBuffer::fromObject(a.clone(metadata)),cb=InterleavedBuffer::fromObject(b.clone(metadata)),cc=InterleavedBuffer::fromObject(c.clone(metadata));
    REQUIRE(a.array.strictEquals(nativeArray(first)));REQUIRE_FALSE(ca.array.strictEquals(cb.array));
    REQUIRE(ca.array.get("buffer").strictEquals(cb.array.get("buffer")));REQUIRE_FALSE(ca.array.get("buffer").strictEquals(cc.array.get("buffer")));
    ca.array.value().set("0",99);REQUIRE(cb.array.get("0").number()==99);REQUIRE(first[0]==2);REQUIRE(ca.array.get("byteOffset").number()==0);
    REQUIRE(metadata.get("arrayBuffers").keys().size()==2);
}
TEST_CASE("InterleavedBuffer follows in-place overlap and keeps destination bookkeeping on copy","[core][interleaved]") {
    Float32Array values({1,2,3,4,5,6});InterleavedBuffer source(values,3);source.copyAt(1.0/3,source,0);
    REQUIRE(values[1]==1);REQUIRE(values[2]==1);REQUIRE(values[3]==1);
    InterleavedBuffer target;target.version=7;target.addUpdateRange("start",nullptr);const auto ranges=target.updateRanges.value(),uuid=target.uuid.value();
    target.copy(source);REQUIRE(target.count.toNumber()==2);REQUIRE(target.version.toNumber()==7);REQUIRE(target.uuid.strictEquals(uuid));REQUIRE(target.updateRanges.strictEquals(ranges));
    REQUIRE_FALSE(target.array.strictEquals(source.array));target.needsUpdate=1;REQUIRE(target.version.toNumber()==7);target.needsUpdate=true;REQUIRE(target.version.toNumber()==8);
    target.clearUpdateRanges();REQUIRE(target.updateRanges.strictEquals(ranges));REQUIRE(ranges.length()==0);
}
TEST_CASE("InterleavedBuffer metadata errors and instanced copy match JavaScript","[core][interleaved]") {
    InterleavedBuffer absent;REQUIRE(absent.count.toNumber()==0);REQUIRE_THROWS_AS(absent.clone(),JSTypeError);REQUIRE_THROWS_AS(absent.toJSON(),JSTypeError);
    REQUIRE_THROWS_AS(InterleavedBuffer(nullptr,2),JSTypeError);
    InterleavedBuffer shortBytes(Uint8Array({1,2,3}),1);auto metadata=JSValue::object();REQUIRE_THROWS_AS(shortBytes.toJSON(metadata),JSRangeError);
    REQUIRE(metadata.get("arrayBuffers").keys().empty());
    InstancedInterleavedBuffer target(Float32Array({1,2,3,4}),2,JSValue::object());InterleavedBuffer source(Float32Array({5,6}),1);target.copy(source);
    REQUIRE(target.meshPerAttribute.isUndefined());const auto json=target.toJSON(JSValue::object());REQUIRE(json.hasOwn("meshPerAttribute"));REQUIRE_FALSE(json.toJSON()->contains("meshPerAttribute"));
}
TEST_CASE("Native typed array properties remain alive with native handles and cycles collect","[core][interleaved][gc]") {
    JSValue root=JSValue::object();
    Float32Array native({1,2,3,4});{auto wrapper=nativeArray(native);wrapper.set("name","retained");wrapper.get("buffer").set("_uuid","buffer");}
    root.collectGarbage();REQUIRE(nativeArray(native).get("name").strictEquals("retained"));REQUIRE(nativeArray(native).get("buffer").get("_uuid").strictEquals("buffer"));
    const auto baseline=root.heapObjectCount();
    for(int i=0;i<10;++i){auto array=nativeArray(Float32Array({1,2,3,4}));auto value=InterleavedBuffer(array,2).object();value.set("self",value);array.set("owner",value);array.get("buffer").set("owner",value);}
    root.collectGarbage();REQUIRE(root.heapObjectCount()==baseline);
}
