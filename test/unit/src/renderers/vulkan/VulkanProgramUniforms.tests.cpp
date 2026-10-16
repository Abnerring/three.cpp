#include "catch.hpp"
#include "renderers/vulkan/VulkanProgramUniforms.h"
#include "renderers/vulkan/VulkanShaderCompiler.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>

namespace {
using Storage = VulkanProgramUniforms;
using Scalar = VulkanShaderInterface::Numeric::Scalar;
VulkanEsslResult compile() {
    const auto result = VulkanShaderCompiler::compileESSL({
        R"(#version 300 es
precision highp float;precision highp int;
in vec3 position;out vec3 v;
uniform mat3 basis;uniform float weights[3];uniform ivec2 pair;
uniform uvec2 mask;uniform bvec2 flags;
struct Light { vec3 color;float intensity; };uniform Light lights[2];
uniform sampler2D maps[2];
void main(){vec3 p=basis*position+lights[0].color*lights[1].intensity;
float n=weights[0]+weights[2]+float(pair.x)+float(mask.y)+(flags.x?1.0:0.0)+float(gl_InstanceID);
gl_Position=vec4(p+n*0.0001,1.0);v=texture(maps[0],vec2(0.5)).rgb;}
)",
        R"(#version 300 es
precision highp float;precision highp int;
in vec3 v;out vec4 color;
uniform mat3 basis;uniform float weights[3];
struct Light { vec3 color;float intensity; };uniform Light lights[2];
uniform sampler2D maps[2];
void main(){color=vec4(v+texture(maps[1],vec2(0.5)).rgb+vec3(weights[1]+basis[0][0]+lights[0].intensity+gl_FragCoord.y*0.0001),1.0);}
)"});
    if (!result.linked) throw std::runtime_error(result.phase + ": " + result.diagnostics);
    return result;
}
template<class T> void write(Storage& s, const char* name, std::initializer_list<T> values) {
    const std::vector<T> owned(values);s.write(name,std::span<const T>(owned));
}
template<class T> T at(const Storage& s, std::size_t offset) {
    const auto bytes=s.bytes({0,0});
    if(offset+sizeof(T)>bytes.size())throw std::runtime_error("Invalid expected offset");
    T value;std::memcpy(&value,bytes.data()+offset,sizeof(T));return value;
}
const auto& member(const Storage& s,const char* name) {
    const auto& members=s.blocks().at({0,0}).layout.members;
    for(const auto& m:members)if(m.name==name)return m;
    throw std::runtime_error("Missing test member");
}
auto snapshot(const Storage& s) { const auto b=s.bytes({0,0});return std::vector<std::byte>(b.begin(),b.end()); }
}

TEST_CASE("Linked original uniforms allocate the whole reflected cross-stage block") {
    const auto result=compile();Storage s(result);
    REQUIRE(s.blocks().size()==1);
    const auto& block=s.blocks().at({0,0});
    REQUIRE(block.stages==3);
    REQUIRE(block.storage.bytes().size()==std::max(block.layout.size,block.layout.paddedSize));
    REQUIRE(std::all_of(block.storage.bytes().begin(),block.storage.bytes().end(),[](auto b){return b==std::byte{};}));
    const auto mapping=std::find_if(result.uniformMappings.begin(),result.uniformMappings.end(),[](const auto& u){return u.rootName=="basis";});
    REQUIRE(mapping!=result.uniformMappings.end());
    REQUIRE(mapping->rootLayout.members.empty());
    REQUIRE(s.samplers().at("maps").units==std::vector<std::int32_t>{0,0});
    REQUIRE(s.samplers().at("maps").stages==3);
}
TEST_CASE("Original matrix scalar arrays and structured paths use emitted strides") {
    Storage s(compile());
    write<float>(s,"basis",{1,2,3,4,5,6,7,8,9});
    write<float>(s,"weights[0]",{10,20,30});
    write<float>(s,"weights[1]",{21,31});
    write<float>(s,"lights[0].color",{0.1f,0.2f,0.3f});
    write<float>(s,"lights[1].intensity",{7});
    const auto& matrix=member(s,"basis");
    for(unsigned c=0;c<3;++c)for(unsigned r=0;r<3;++r)
        REQUIRE(at<float>(s,matrix.offset+c*matrix.numeric.matrixStride+r*4)==float(c*3+r+1));
    const auto& weights=member(s,"weights");
    REQUIRE(at<float>(s,weights.offset)==10);
    REQUIRE(at<float>(s,weights.offset+weights.array.strides[0])==21);
    REQUIRE(at<float>(s,weights.offset+2*weights.array.strides[0])==31);
    const auto& lights=member(s,"lights");
    REQUIRE(at<float>(s,lights.offset+lights.members[0].offset)==0.1f);
    REQUIRE(at<float>(s,lights.offset+lights.array.strides[0]+lights.members[1].offset)==7);
}
TEST_CASE("Converted original signed unsigned and boolean fields retain scalar types") {
    Storage s(compile());
    write<std::int32_t>(s,"pair",{-7,11});
    write<std::uint32_t>(s,"mask",{0x80000000u,0xffffffffu});
    const std::array<std::int32_t,2> booleans{-8,0};s.writeBooleans("flags",booleans);
    REQUIRE(at<std::int32_t>(s,member(s,"pair").offset)==-7);
    REQUIRE(at<std::uint32_t>(s,member(s,"mask").offset)==0x80000000u);
    REQUIRE(at<std::uint32_t>(s,member(s,"mask").offset+4)==0xffffffffu);
    REQUIRE(at<std::uint32_t>(s,member(s,"flags").offset)==1);
    REQUIRE(at<std::uint32_t>(s,member(s,"flags").offset+4)==0);
    const auto before=snapshot(s);
    REQUIRE_THROWS_AS(write<float>(s,"mask",{1,2}),std::invalid_argument);
    REQUIRE(snapshot(s)==before);
}
TEST_CASE("Private backend fields use actual draw values and reject application writes") {
    const auto result=compile();Storage s(result);
    s.writeBackend(0x80000003u,73.0f);
    for(const auto& b:result.backendScalars)if(b.declaredStages) {
        if(b.value==VulkanEsslResult::BackendScalar::Value::FirstInstance)
            REQUIRE(at<std::uint32_t>(s,b.offset)==0x80000003u);
        else REQUIRE(at<float>(s,b.offset)==73.0f);
        REQUIRE_THROWS_AS(write<float>(s,b.name.c_str(),{0}),std::invalid_argument);
    }
}
TEST_CASE("Sampler assignments support partial arrays and alias-safe independent copies") {
    Storage s(compile());
    const std::array<std::int32_t,2> first{2,5};s.writeSamplerUnits("maps[0]",first);
    const std::array<std::int32_t,1> second{7};s.writeSamplerUnits("maps[1]",second);
    REQUIRE(s.samplers().at("maps").units==std::vector<std::int32_t>{2,7});
    s.writeSamplerUnits("maps[1]",std::span(s.samplers().at("maps").units).first(1));
    REQUIRE(s.samplers().at("maps").units==std::vector<std::int32_t>{2,2});
    const auto before=s.samplers().at("maps").units;
    const std::array<std::int32_t,1> negative{-1};
    REQUIRE_THROWS_AS(s.writeSamplerUnits("maps",negative),std::invalid_argument);
    REQUIRE_THROWS_AS(s.writeSamplerUnits("maps[2]",second),std::invalid_argument);
    REQUIRE_THROWS_AS(s.writeSamplerUnits("maps[1].x",second),std::invalid_argument);
    const std::array<std::int32_t,3> oversized{1,2,3};
    REQUIRE_THROWS_AS(s.writeSamplerUnits("maps[0]",oversized),std::invalid_argument);
    REQUIRE(s.samplers().at("maps").units==before);
    auto copy=s;copy.writeSamplerUnits("maps[0]",second);
    REQUIRE(s.samplers().at("maps").units==before);
    REQUIRE(copy.samplers().at("maps").units[0]==7);
}
TEST_CASE("Uniform generations outlive compiler data and snapshots are independent") {
    Storage s(compile());write<float>(s,"weights",{10,20,30});
    auto copy=s;write<float>(copy,"weights[1]",{99});
    const auto offset=member(s,"weights").offset+member(s,"weights").array.strides[0];
    REQUIRE(at<float>(s,offset)==20);REQUIRE(at<float>(copy,offset)==99);
    const auto before=snapshot(s);
    REQUIRE_THROWS_AS(write<float>(s,"unknown",{1}),std::invalid_argument);
    REQUIRE_THROWS_AS(write<float>(s,"weights[3]",{1}),std::invalid_argument);
    REQUIRE_THROWS_AS(write<float>(s,"basis",{1,2}),std::invalid_argument);
    REQUIRE(snapshot(s)==before);
}
TEST_CASE("Conflicting complete cross-stage schemas and unlinked inputs are rejected") {
    auto result=compile();
    for(auto& d:result.interfaces[1].descriptors)if(d.set==0 && d.binding==0)
        d.block.members.back().offset+=4;
    REQUIRE_THROWS_AS(Storage(result),std::invalid_argument);
    result=compile();result.linked=false;
    REQUIRE_THROWS_AS(Storage(result),std::invalid_argument);
}
