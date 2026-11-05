#include "nodes/functions/BSDF/DFGLUT.h"
#include "../../../utils/parity/BSDFLookupFixtures.h"
#include "constants.h"
#include "catch.hpp"
#include <cstdint>
using namespace BSDFLookupFixtures;
TEST_CASE("DFGLUT retains the pinned untyped inline Fn and source") {
    const auto value=nativeDFGLUT(),shader=value.get("shaderNode"),callback=shader.get("jsFunc");
    REQUIRE(nativeDFGLUTModule().get("default").sameValue(value));
    REQUIRE(value.get("isFn").strictEquals(true));
    REQUIRE(shader.get("nodeType").isNull());
    REQUIRE(shader.get("layout").isNull());
    REQUIRE(callback.get("length").strictEquals(1));
    REQUIRE(call(callback,"toString").toString().find("if ( lut === null )")!=std::string::npos);
}
TEST_CASE("DFGLUT reads both input fields in destructuring order and preserves abrupt completion") {
    const auto ok=observed(false,"");REQUIRE(ok.get("result").strictEquals("success"));
    REQUIRE(ok.get("events").toJSON()->dump()=="[\"roughness\",\"dotNV\"]");
    const auto early=observed(false,"roughness"),late=observed(false,"dotNV");
    REQUIRE(early.get("events").length()==1);REQUIRE(early.get("result").get("1").strictEquals("roughness"));
    REQUIRE(late.get("events").length()==2);REQUIRE(late.get("result").get("1").strictEquals("dotNV"));
    REQUIRE_THROWS_AS(body(false).call({}, {nullptr}),JSTypeError);
    REQUIRE_THROWS_AS(body(false).call({}, {JSValue{}}),JSTypeError);
}
TEST_CASE("DFGLUT materializes all 512 pinned half float words with exact texture settings") {
    StackScope scope;const auto result=body(false).call({}, {inputs(false)}),t=texture(result),image=t.get("image"),data=image.get("data");
    REQUIRE(data.get("length").strictEquals(512));
    // Independently computed from all 512 pinned upstream words, little endian.
    std::uint64_t digest=14695981039346656037ull;
    for(unsigned i=0;i<data.get("length").toNumber();++i){const auto word=static_cast<unsigned>(data.get(JSValue(i)).toNumber());for(const auto byte:{word&255u,word>>8})digest=(digest^byte)*1099511628211ull;}
    REQUIRE(digest==0xc8e4636174fcd84bull);
    REQUIRE(image.get("width").strictEquals(16));REQUIRE(image.get("height").strictEquals(16));
    REQUIRE(t.get("name").strictEquals("DFG_LUT"));REQUIRE(t.get("format").strictEquals(RGFormat));REQUIRE(t.get("type").strictEquals(HalfFloatType));
    REQUIRE(t.get("minFilter").strictEquals(LinearFilter));REQUIRE(t.get("magFilter").strictEquals(LinearFilter));
    REQUIRE(t.get("wrapS").strictEquals(ClampToEdgeWrapping));REQUIRE(t.get("wrapT").strictEquals(ClampToEdgeWrapping));
    REQUIRE(t.get("generateMipmaps").strictEquals(false));REQUIRE(t.get("flipY").strictEquals(false));REQUIRE(t.get("unpackAlignment").strictEquals(1));
    REQUIRE(t.get("version").strictEquals(1));REQUIRE(t.get("source").get("version").strictEquals(1));
    REQUIRE(t.get("source").get("data").sameValue(image));REQUIRE(data.get("byteLength").strictEquals(1024));REQUIRE(data.get("BYTES_PER_ELEMENT").strictEquals(2));
}
TEST_CASE("DFGLUT reuses the same texture image and live typed data across distinct sample nodes") {
    const auto out=reuse();for(const auto* key:{"textureSame","imageSame","dataSame","separateSampleNodes","aliasWriteVisible","thirdSame","versionStable","sourceVersionStable"})REQUIRE(out.get(key).strictEquals(true));
    REQUIRE(out.get("restored").strictEquals(0x30b5));
    const auto outGraph=graph(false);REQUIRE(outGraph.get("objects").length()>10);
}
