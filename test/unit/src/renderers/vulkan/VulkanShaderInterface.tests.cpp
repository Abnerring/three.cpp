#include "catch.hpp"
#include "renderers/vulkan/VulkanShaderCompiler.h"
#include "renderers/vulkan/VulkanShaderInterface.h"
#include <algorithm>
#include <future>
#include <fstream>
#include <iterator>
namespace {
VulkanShaderCompiler compiler(){return VulkanShaderCompiler(THREECPP_GLSLANG_EXECUTABLE);}
const char* vertex=R"(#version 450
layout(location=3) in vec3 position;
layout(location=2) out vec2 uv;
uniform mat4 modelViewProjection;
uniform float shift;
void main(){uv=position.xy;gl_Position=modelViewProjection*vec4(position.x+shift,position.yz,1);}
)";
const char* fragment=R"(#version 450
layout(location=2) in vec2 uv;
layout(location=0) out vec4 color;
uniform vec4 diffuse;
uniform sampler2D map;
void main(){color=diffuse*texture(map,uv);}
)";
const VulkanShaderInterface::Descriptor& descriptor(const VulkanShaderInterface& value,unsigned set,unsigned binding) {
    const auto found=std::find_if(value.descriptors.begin(),value.descriptors.end(),[&](const auto& d){return d.set==set&&d.binding==binding;});
    if(found==value.descriptors.end())throw std::runtime_error("Missing descriptor");return *found;
}
const VulkanShaderInterface::Block& member(const VulkanShaderInterface::Block& block,const std::string& name) {
    const auto found=std::find_if(block.members.begin(),block.members.end(),[&](const auto& m){return m.name==name;});
    if(found==block.members.end())throw std::runtime_error("Missing block member "+name);return *found;
}
}
TEST_CASE("Linked uniforms retain the compiler-assigned block and sampler bindings", "[vulkan][reflection]") {
    auto compiled=compiler().compile(vertex,fragment);INFO(compiled.diagnostics);REQUIRE(compiled.linked);
    const auto vs=VulkanShaderInterface::read(compiled.vertex),fs=VulkanShaderInterface::read(compiled.fragment);
    REQUIRE(vs.stage==1);REQUIRE(fs.stage==16);REQUIRE(vs.entryPoint=="main");
    for(const auto* stage:{&vs,&fs}) {
        const auto& uniform=descriptor(*stage,0,0);REQUIRE(uniform.type==6);REQUIRE(uniform.block.paddedSize==96);
        REQUIRE(member(uniform.block,"modelViewProjection").offset==0);
        REQUIRE(member(uniform.block,"modelViewProjection").numeric.matrixStride==16);
        REQUIRE(member(uniform.block,"shift").offset==64);
        REQUIRE(member(uniform.block,"diffuse").offset==80);
    }
    const auto& sampler=descriptor(fs,0,1);REQUIRE(sampler.type==1);REQUIRE(sampler.name=="map");REQUIRE(sampler.count==1);
    const auto p=std::find_if(vs.inputs.begin(),vs.inputs.end(),[](const auto& v){return v.name=="position";});
    REQUIRE(p!=vs.inputs.end());REQUIRE(p->location==3);REQUIRE(p->numeric.components==3);REQUIRE(p->numeric.width==32);
    REQUIRE(p->numeric.scalar==VulkanShaderInterface::Numeric::Scalar::FloatingPoint);
    compiled={};REQUIRE(member(descriptor(fs,0,0).block,"diffuse").offset==80);
}
TEST_CASE("Explicit nested blocks preserve arrays matrix layout and push constants", "[vulkan][reflection]") {
    const auto compiled=compiler().compile(R"(#version 450
struct Light { vec3 color; float intensity; };
layout(set=2,binding=4,std140,row_major) uniform Scene { mat3 matrix; Light lights[2]; float weights[3]; } scene;
layout(push_constant) uniform Push { vec4 offset; } pc;
layout(location=0) in vec3 position;
layout(location=0) out vec2 uv;
void main(){uv=position.xy;gl_Position=vec4(scene.matrix*position+scene.lights[1].color*scene.lights[0].intensity,1)+pc.offset*scene.weights[2];}
)",R"(#version 450
layout(set=1,binding=7) uniform sampler2D maps[3];
layout(location=0) in vec2 uv;
layout(location=0) out vec4 color;
void main(){color=texture(maps[2],uv);}
)");INFO(compiled.diagnostics);REQUIRE(compiled.linked);
    const auto vs=VulkanShaderInterface::read(compiled.vertex),fs=VulkanShaderInterface::read(compiled.fragment);
    const auto& b=descriptor(vs,2,4).block;const auto& m=member(b,"matrix");
    REQUIRE(m.offset==0);REQUIRE(m.numeric.rowMajor);REQUIRE(m.numeric.rows==3);REQUIRE(m.numeric.columns==3);REQUIRE(m.numeric.matrixStride==16);
    const auto& lights=member(b,"lights");REQUIRE(lights.offset==48);REQUIRE(lights.array.dimensions==std::vector<std::uint32_t>{2});REQUIRE(lights.array.stride==16);
    REQUIRE(member(lights,"intensity").offset==12);
    const auto& weights=member(b,"weights");REQUIRE(weights.offset==80);REQUIRE(weights.array.stride==16);REQUIRE(b.paddedSize==128);
    REQUIRE(vs.pushConstants.size()==1);REQUIRE(vs.pushConstants[0].size==16);REQUIRE(member(vs.pushConstants[0],"offset").offset==0);
    const auto& maps=descriptor(fs,1,7);REQUIRE(maps.count==3);REQUIRE(maps.arrayDimensions==std::vector<std::uint32_t>{3});
}
TEST_CASE("Reflection rejects invalid framing and unknown entry points", "[vulkan][reflection]") {
    REQUIRE_THROWS_AS(VulkanShaderInterface::read({}),std::invalid_argument);
    auto compiled=compiler().compile(vertex,fragment);REQUIRE(compiled.linked);
    REQUIRE_THROWS_AS(VulkanShaderInterface::read(compiled.vertex,"missing"),std::invalid_argument);
    REQUIRE_THROWS_AS(VulkanShaderInterface::read(compiled.vertex,std::string("main\0extra",10)),std::invalid_argument);
    auto bad=compiled.vertex;bad[5]&=0xffff;REQUIRE_THROWS_AS(VulkanShaderInterface::read(bad),std::invalid_argument);
    // Cut inside OpCapability, rather than assuming semantic validation.
    bad=compiled.vertex;bad.resize(6);REQUIRE_THROWS_AS(VulkanShaderInterface::read(bad),std::invalid_argument);
    std::vector<std::future<VulkanShaderInterface>> tasks;
    for(unsigned i=0;i<12;++i)tasks.push_back(std::async(std::launch::async,[words=compiled.fragment]{return VulkanShaderInterface::read(words);}));
    for(auto& task:tasks)REQUIRE(descriptor(task.get(),0,1).name=="map");
}

TEST_CASE("Existing renderer shaders expose their actual Vulkan resource layout", "[vulkan][reflection]") {
    const auto read=[](const char* relative) {
        std::ifstream file(std::string(THREECPP_SOURCE_DIRECTORY)+relative,std::ios::binary);
        if(!file)throw std::runtime_error("Shader fixture unavailable");
        return std::string(std::istreambuf_iterator<char>(file),{});
    };
    const auto compiled=compiler().compile(read("/src/renderers/shaders/ShaderLib/meshbasic.vert.glsl"),
                                            read("/src/renderers/shaders/ShaderLib/meshbasic.frag.glsl"));
    INFO(compiled.diagnostics);REQUIRE(compiled.linked);
    const auto vs=VulkanShaderInterface::read(compiled.vertex),fs=VulkanShaderInterface::read(compiled.fragment);
    const auto& transforms=descriptor(vs,0,2);REQUIRE(transforms.type==6);REQUIRE(transforms.block.paddedSize==288);
    REQUIRE(member(transforms.block,"alphaTransform").offset==48);
    REQUIRE(member(transforms.block,"modelView").offset==96);
    REQUIRE(member(transforms.block,"inputLayout").offset==208);
    REQUIRE(member(transforms.block,"inputLayout").array.dimensions==std::vector<std::uint32_t>{4});
    REQUIRE(member(transforms.block,"matrixEnabled").offset==272);
    for(unsigned binding:{3u,4u,5u,7u,8u,9u,10u}) {
        const auto& storage=descriptor(vs,0,binding);REQUIRE(storage.type==7);
        const auto& values=member(storage.block,"values");
        REQUIRE(values.array.dimensions==std::vector<std::uint32_t>{0});
        REQUIRE(values.array.stride==(binding==5?16:4));
    }
    const auto& clipping=descriptor(fs,0,11);REQUIRE(clipping.type==7);
    REQUIRE(member(clipping.block,"counts").offset==0);REQUIRE(member(clipping.block,"planes").offset==16);
    REQUIRE(member(clipping.block,"planes").array.stride==16);
    REQUIRE(descriptor(vs,0,6).type==1);REQUIRE(descriptor(fs,0,0).type==1);REQUIRE(descriptor(fs,0,1).type==1);
    for(const auto* stage:{&vs,&fs}) {
        REQUIRE(stage->pushConstants.size()==1);REQUIRE(stage->pushConstants[0].size==128);
        REQUIRE(member(stage->pushConstants[0],"diffuse").offset==64);
        REQUIRE(member(stage->pushConstants[0],"fogParams").offset==112);
    }
}
