#include "catch.hpp"
#include "renderers/vulkan/VulkanShaderCompiler.h"
#include "renderers/vulkan/VulkanUniformBuffer.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <vector>
#include <spirv-tools/libspirv.hpp>
namespace {
using Buffer=VulkanUniformBuffer;
const char* fragment=R"(#version 450
layout(location=0) out vec4 color;void main(){color=vec4(1);}
)";
auto compile(const char* source) {
    auto result=VulkanShaderCompiler(THREECPP_GLSLANG_EXECUTABLE).compile(source,fragment);
    if(!result.linked)throw std::runtime_error(result.diagnostics);return result;
}
VulkanShaderInterface::Block reflect(std::span<const std::uint32_t> words) {
    const auto interface=VulkanShaderInterface::read(words);
    for(const auto& d:interface.descriptors)if(d.set==0 && d.binding==0)return d.block;
    throw std::runtime_error("Missing block");
}
const auto& member(const VulkanShaderInterface::Block& block,const char* name) {
    for(const auto& value:block.members)if(value.name==name)return value;
    throw std::runtime_error("Missing member");
}
template<class T>void put(Buffer& b,const char* n,std::initializer_list<T> values) {
    const std::vector<T> copy(values);b.write(n,std::span<const T>(copy));
}
template<class T>T at(const Buffer& buffer,std::size_t offset) {
    if(offset+sizeof(T)>buffer.bytes().size())throw std::runtime_error("Invalid test offset");
    T result;std::memcpy(&result,buffer.bytes().data()+offset,sizeof(T));return result;
}
std::vector<std::byte> snapshot(const Buffer& b){return {b.bytes().begin(),b.bytes().end()};}
const char* nested=R"(#version 450
struct Light {vec3 color;float intensity;};
layout(set=0,binding=0,std140,row_major) uniform Input {
    mat2x3 matrix;Light lights[2];float grid[2][3];
    ivec2 signedValue;uvec2 unsignedValue;bool enabled;
} scene;
void main(){float t=scene.matrix[0].x+scene.lights[0].color.x+scene.lights[1].intensity+
scene.grid[1][2]+float(scene.signedValue.x)+float(scene.unsignedValue.y)+(scene.enabled?1:0);
gl_Position=vec4(t,0,0,1);}
)";
}
TEST_CASE("Compiled std140 arrays structures and non-square row-major matrices encode independently") {
    const auto compiled=compile(nested);const auto layout=reflect(compiled.vertex);
    REQUIRE(member(layout,"grid").array.dimensions==std::vector<std::uint32_t>{2,3});
    REQUIRE(member(layout,"grid").array.strides==std::vector<std::uint32_t>{48,16});
    REQUIRE(member(layout,"grid").offset==80);
    REQUIRE(member(layout,"signedValue").offset==176);
    Buffer buffer(layout);
    put<float>(buffer,"matrix",{1,2,3,4,5,6});
    put<float>(buffer,"lights[0].color",{9,10,11});put<float>(buffer,"lights[0].intensity",{12});
    put<float>(buffer,"lights[1].color",{6,7,8});put<float>(buffer,"lights[1].intensity",{10});
    put<float>(buffer,"grid",{1,2,3,4,5,6});
    put<std::int32_t>(buffer,"signedValue",{-7,19});put<std::uint32_t>(buffer,"unsignedValue",{0x80000000u,0xffffffffu});
    const std::array<std::int32_t,1> boolean{-8};buffer.writeBooleans("enabled",boolean);
    const std::array<float,12> rows{1,4,0,0,2,5,0,0,3,6,0,0};
    for(std::size_t i=0;i<rows.size();++i)REQUIRE(at<float>(buffer,i*4)==rows[i]);
    const std::array<float,8> lights{9,10,11,12,6,7,8,10};
    for(std::size_t i=0;i<lights.size();++i)REQUIRE(at<float>(buffer,48+i*4)==lights[i]);
    for(std::size_t i=0;i<6;++i) {
        REQUIRE(at<float>(buffer,80+i*16)==float(i+1));
        for(std::size_t p=4;p<16;p+=4)REQUIRE(at<float>(buffer,80+i*16+p)==0);
    }
    REQUIRE(at<std::int32_t>(buffer,176)==-7);REQUIRE(at<std::int32_t>(buffer,180)==19);
    REQUIRE(at<std::uint32_t>(buffer,184)==0x80000000u);REQUIRE(at<std::uint32_t>(buffer,188)==0xffffffffu);
    REQUIRE(at<std::uint32_t>(buffer,192)==1);
}
TEST_CASE("Partial arrays preserve padding and rejected writes do not alter storage") {
    const auto compiled=compile(nested);const auto layout=reflect(compiled.vertex);Buffer b(layout);
    put<float>(b,"grid",{1,2,3,4,5,6});put<float>(b,"grid[1][1]",{50,60});
    REQUIRE(at<float>(b,80)==1);REQUIRE(at<float>(b,128)==4);REQUIRE(at<float>(b,144)==50);REQUIRE(at<float>(b,160)==60);
    const auto before=snapshot(b);
    REQUIRE_THROWS_AS(put<float>(b,"matrix",{1,2,3,4,5}),std::invalid_argument);
    REQUIRE_THROWS_AS(put<float>(b,"grid[1][2]",{1,2}),std::invalid_argument);
    REQUIRE_THROWS_AS(put<std::uint32_t>(b,"grid",{1}),std::invalid_argument);
    REQUIRE_THROWS_AS(put<float>(b,"missing",{1}),std::invalid_argument);
    REQUIRE(snapshot(b)==before);
    put<float>(b,"lights[1].color",{6,7,8});put<float>(b,"lights[1].intensity",{10});
    const auto source=b.bytes().subspan(64,24);
    b.writeRaw("grid",Buffer::Scalar::FloatingPoint,32,source);
    const std::array<float,6> expected{6,7,8,10,1,0};
    for(std::size_t i=0;i<expected.size();++i)REQUIRE(at<float>(b,80+i*16)==expected[i]);
    auto bad=layout;member(bad,"grid");bad.members[2].array.strides.clear();
    REQUIRE_THROWS_AS(Buffer(bad),std::invalid_argument);
}
TEST_CASE("Runtime struct arrays require explicit counts and allow zero-length storage") {
    const auto compiled=compile(R"(#version 450
struct Light {vec3 color;float intensity;};
layout(set=0,binding=0,std430) readonly buffer Input {uint tag;Light values[];} scene;
void main(){gl_Position=vec4(scene.values[0].color*scene.values[0].intensity,float(scene.tag));}
)");const auto layout=reflect(compiled.vertex);
    REQUIRE(member(layout,"values").offset==16);REQUIRE(member(layout,"values").array.strides==std::vector<std::uint32_t>{16});
    REQUIRE_THROWS_AS(Buffer(layout,48),std::invalid_argument);
    Buffer b(layout,48,{{"values",{2}}});put<std::uint32_t>(b,"tag",{37});
    put<float>(b,"values[0].color",{2,3,4});put<float>(b,"values[0].intensity",{5});
    put<float>(b,"values[1].color",{6,7,8});put<float>(b,"values[1].intensity",{9});
    REQUIRE(at<std::uint32_t>(b,0)==37);
    const std::array<float,8> expected{2,3,4,5,6,7,8,9};
    for(std::size_t i=0;i<8;++i)REQUIRE(at<float>(b,16+i*4)==expected[i]);
    Buffer empty(layout,16,{{"values",{0}}});put<std::uint32_t>(empty,"tag",{38});
    REQUIRE(at<std::uint32_t>(empty,0)==38);
    REQUIRE_THROWS_AS(Buffer(layout,32,{{"values",{2}}}),std::invalid_argument);
}
TEST_CASE("Actual SPIR-V outer stride decorations retain extra array padding") {
    auto compiled=compile(R"(#version 450
layout(set=0,binding=0,std140) uniform Input {float grid[2][3];} scene;
void main(){gl_Position=vec4(scene.grid[1][2],0,0,1);}
)");unsigned changed=0;
    for(std::size_t i=5;i<compiled.vertex.size();i+=compiled.vertex[i]>>16)
        if((compiled.vertex[i]&0xffff)==71 && compiled.vertex[i]>>16==4 &&
           compiled.vertex[i+2]==6 && compiled.vertex[i+3]==48) {compiled.vertex[i+3]=64;++changed;}
    REQUIRE(changed==1);const auto layout=reflect(compiled.vertex);
    std::string diagnostics;
    spvtools::SpirvTools validator(SPV_ENV_VULKAN_1_1);
    validator.SetMessageConsumer([&](spv_message_level_t,const char*,const spv_position_t&,const char* message){diagnostics+=message;});
    const bool valid=validator.Validate(compiled.vertex);INFO(diagnostics);REQUIRE(valid);
    REQUIRE(member(layout,"grid").array.strides==std::vector<std::uint32_t>{64,16});
    REQUIRE(layout.size==128);REQUIRE(layout.paddedSize>=128);
    Buffer b(layout);put<float>(b,"grid[0][0]",{11,12,13,14,15,16});
    const std::array<std::size_t,6> offsets{0,16,32,64,80,96};
    for(std::size_t i=0;i<6;++i)REQUIRE(at<float>(b,offsets[i])==float(11+i));
    REQUIRE(at<float>(b,48)==0);REQUIRE(at<float>(b,112)==0);
}
TEST_CASE("Floating-point bit patterns and reflected Float64 matrix width survive packing") {
    const auto compiled=compile(R"(#version 450
#extension GL_ARB_gpu_shader_fp64 : require
layout(set=0,binding=0,std140,row_major) uniform Input {dmat2x3 matrix;dvec2 values;} scene;
void main(){gl_Position=vec4(float(scene.matrix[1].z+scene.values.x),0,0,1);}
)");const auto layout=reflect(compiled.vertex);Buffer b(layout);
    REQUIRE(member(layout,"matrix").numeric.width==64);
    REQUIRE(member(layout,"matrix").numeric.matrixStride==16);
    put<double>(b,"matrix",{1,2,3,4,5,6});
    const std::array<double,6> expected{1,4,2,5,3,6};
    for(std::size_t i=0;i<6;++i)REQUIRE(at<double>(b,i*8)==expected[i]);
    const auto valueOffset=member(layout,"values").offset;
    const std::array<double,2> bits{-0.0,std::bit_cast<double>(std::uint64_t{0x7ff8000000000042})};
    b.write("values",std::span<const double>(bits));
    REQUIRE(std::bit_cast<std::uint64_t>(at<double>(b,valueOffset))==0x8000000000000000ull);
    REQUIRE(std::bit_cast<std::uint64_t>(at<double>(b,valueOffset+8))==0x7ff8000000000042ull);
    REQUIRE_THROWS_AS(put<float>(b,"values",{1,2}),std::invalid_argument);
}
TEST_CASE("Block metadata and byte storage have independent lifetimes after construction and copy") {
    auto b=[] {
        const auto compiled=compile(nested);const auto layout=reflect(compiled.vertex);
        Buffer result(layout);put<float>(result,"grid",{1,2,3,4,5,6});return result;
    }();
    auto copy=b;
    put<float>(copy,"grid[1][1]",{20,30});
    REQUIRE(at<float>(b,144)==5);REQUIRE(at<float>(copy,144)==20);
    b=Buffer(reflect(compile(nested).vertex));
    put<float>(copy,"matrix",{1,2,3,4,5,6});
    REQUIRE(at<float>(copy,4)==4);REQUIRE(at<float>(b,4)==0);
}
