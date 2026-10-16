#include "catch.hpp"
#include "renderers/vulkan/VulkanShaderCompiler.h"
#include "renderers/vulkan/VulkanUniformBuffer.h"
#include <spirv-tools/libspirv.hpp>
#include <array>
#include <cstring>
#include <optional>

namespace {
const VulkanShaderInterface::Block& matrixMember(const VulkanShaderInterface::Block& block) {
    for(const auto& member:block.members)if(member.name=="matrices")return member;
    throw std::runtime_error("Missing matrices member");
}
VulkanShaderInterface::Block matrixBlock(const std::vector<std::uint32_t>& words) {
    auto interface=VulkanShaderInterface::read(words);
    for(const auto& descriptor:interface.descriptors)if(descriptor.set==0&&descriptor.binding==0)return descriptor.block;
    throw std::runtime_error("Missing matrix block");
}
void validateMatrixModule(const std::vector<std::uint32_t>& words) {
    spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_1);std::string diagnostics;
    tools.SetMessageConsumer([&](spv_message_level_t,const char*,const spv_position_t&,const char* message){diagnostics+=message;diagnostics+='\n';});
    const bool valid=tools.Validate(words);INFO(diagnostics);REQUIRE(valid);
}
// Rewrite actual member decorations into their equivalent legal group form.
// No layout value is inferred or synthesized from the matrix's dimensions.
std::vector<std::uint32_t> groupedMatrixLayout(const std::vector<std::uint32_t>& words) {
    constexpr unsigned MemberDecorate=72,Decorate=71,DecorationGroup=73,GroupMemberDecorate=75;
    constexpr unsigned RowMajor=4,ColMajor=5,MatrixStride=7;
    std::optional<std::pair<unsigned,unsigned>> owner;
    for(std::size_t i=5;i<words.size();i+=words[i]>>16)
        if((words[i]&0xffff)==MemberDecorate&&words[i+3]==MatrixStride) {owner={{words[i+1],words[i+2]}};break;}
    REQUIRE(owner);const auto group=words[3];
    std::vector<std::uint32_t> result(words.begin(),words.begin()+5),annotations{(2u<<16)|DecorationGroup,group};
    result[3]=group+1;bool inserted=false;unsigned count=0;
    for(std::size_t i=5;i<words.size();i+=words[i]>>16) {
        const auto length=words[i]>>16,op=words[i]&0xffff;
        if(op==MemberDecorate&&words[i+1]==owner->first&&words[i+2]==owner->second&&
           (words[i+3]==RowMajor||words[i+3]==ColMajor||words[i+3]==MatrixStride)) {
            annotations.push_back(((length-1)<<16)|Decorate);annotations.push_back(group);
            annotations.insert(annotations.end(),words.begin()+i+3,words.begin()+i+length);++count;continue;
        }
        // OpTypeVoid is the first type emitted by the pinned compiler.
        if(op==19&&!inserted) {
            annotations.insert(annotations.end(),{(4u<<16)|GroupMemberDecorate,group,owner->first,owner->second});
            result.insert(result.end(),annotations.begin(),annotations.end());inserted=true;
        }
        result.insert(result.end(),words.begin()+i,words.begin()+i+length);
    }
    REQUIRE(count==2);REQUIRE(inserted);return result;
}
void checkMatrixStorage(const std::vector<std::uint32_t>& words,bool rowMajor,unsigned rows) {
    validateMatrixModule(words);const auto layout=matrixBlock(words);const auto& member=matrixMember(layout);
    REQUIRE(member.numeric.columns==2);REQUIRE(member.numeric.rows==rows);
    REQUIRE(member.numeric.matrixStride==16);REQUIRE(member.numeric.rowMajor==rowMajor);
    REQUIRE(member.array.dimensions==std::vector<std::uint32_t>{2});
    REQUIRE(member.array.strides==std::vector<std::uint32_t>{16*(rowMajor?rows:2)});
    VulkanUniformBuffer buffer(layout);std::vector<float> values(rows*4);
    for(unsigned i=0;i<values.size();++i)values[i]=float(i+1);buffer.write("matrices",std::span<const float>(values));
    std::vector<float> expected(buffer.bytes().size()/sizeof(float),0);
    for(unsigned a=0;a<2;++a)for(unsigned col=0;col<2;++col)for(unsigned row=0;row<rows;++row) {
        const auto offset=member.offset+a*member.array.strides[0]+(rowMajor?row:col)*16+(rowMajor?col:row)*4;
        expected[offset/4]=values[a*rows*2+col*rows+row];
    }
    for(std::size_t i=0;i<expected.size();++i) {float actual;std::memcpy(&actual,buffer.bytes().data()+i*4,4);REQUIRE(actual==expected[i]);}
    auto invalid=layout;for(auto& m:invalid.members)if(m.name=="matrices")m.numeric.matrixStride=0;
    REQUIRE_THROWS_AS(VulkanUniformBuffer(invalid),std::invalid_argument);
}
}
TEST_CASE("ESSL matrix arrays retain actual member MatrixStride through direct and grouped decorations") {
    VulkanEsslSourcePair pair;
    pair.vertex="#version 300 es\nprecision highp float;uniform mat2 matrices[2];void main(){gl_Position=vec4((matrices[0]+matrices[1])*vec2(1.),0.,1.);}";
    pair.fragment="#version 300 es\nprecision highp float;out vec4 color;void main(){color=vec4(1.);}";
    const auto result=VulkanShaderCompiler::compileESSL(pair);INFO(result.diagnostics);REQUIRE(result.linked);
    checkMatrixStorage(result.modules[0].words,false,2);
    checkMatrixStorage(groupedMatrixLayout(result.modules[0].words),false,2);
}
TEST_CASE("Non-square matrix arrays retain row and column major packing through GroupMemberDecorate") {
    const std::string fragment="#version 450\nlayout(location=0) out vec4 color;void main(){color=vec4(1.);}";
    for(const bool rowMajor:{false,true}) {
        const std::string vertex="#version 450\nlayout(set=0,binding=0,std140,"+std::string(rowMajor?"row_major":"column_major")+
            ") uniform Input{mat2x3 matrices[2];} scene;void main(){gl_Position=vec4((scene.matrices[0]+scene.matrices[1])*vec2(1.),1.);}";
        const auto result=VulkanShaderCompiler(THREECPP_GLSLANG_EXECUTABLE).compile(vertex,fragment);INFO(result.diagnostics);REQUIRE(result.linked);
        checkMatrixStorage(result.vertex,rowMajor,3);checkMatrixStorage(groupedMatrixLayout(result.vertex),rowMajor,3);
    }
}
TEST_CASE("Nested struct arrays retain the owning member of double matrix arrays") {
    const auto result=VulkanShaderCompiler(THREECPP_GLSLANG_EXECUTABLE).compile(R"(#version 450
struct Item {dmat2x3 matrices[2];};
layout(set=0,binding=0,std140,row_major) uniform Input {Item items[2];} scene;
void main(){gl_Position=vec4(float(scene.items[1].matrices[1][1].z));}
)","#version 450\nlayout(location=0) out vec4 color;void main(){color=vec4(1.);}");
    INFO(result.diagnostics);REQUIRE(result.linked);
    for(const auto& words:{result.vertex,groupedMatrixLayout(result.vertex)}) {
        validateMatrixModule(words);const auto layout=matrixBlock(words);
        REQUIRE(layout.members.size()==1);const auto& items=layout.members[0];const auto& m=matrixMember(items);
        REQUIRE(items.array.dimensions==std::vector<std::uint32_t>{2});REQUIRE(items.array.strides==std::vector<std::uint32_t>{96});
        REQUIRE(m.numeric.width==64);REQUIRE(m.numeric.columns==2);REQUIRE(m.numeric.rows==3);
        REQUIRE(m.numeric.rowMajor);REQUIRE(m.numeric.matrixStride==16);
        REQUIRE(m.array.dimensions==std::vector<std::uint32_t>{2});REQUIRE(m.array.strides==std::vector<std::uint32_t>{48});
        VulkanUniformBuffer buffer(layout);const std::array<double,12> values{1,2,3,4,5,6,7,8,9,10,11,12};
        buffer.write("items[1].matrices",std::span<const double>(values));
        std::vector<double> expected(buffer.bytes().size()/8,0);
        for(unsigned a=0;a<2;++a)for(unsigned col=0;col<2;++col)for(unsigned row=0;row<3;++row)
            expected[(items.offset+96+m.offset+a*48+row*16+col*8)/8]=values[a*6+col*3+row];
        for(std::size_t i=0;i<expected.size();++i){double actual;std::memcpy(&actual,buffer.bytes().data()+i*8,8);REQUIRE(actual==expected[i]);}
    }
}
TEST_CASE("An empty decoration group is a valid no-op for members and nonmembers") {
    VulkanEsslSourcePair pair;
    pair.vertex="#version 300 es\nprecision highp float;uniform mat2 matrices[2];void main(){gl_Position=vec4((matrices[0]+matrices[1])*vec2(1.),0.,1.);}";
    pair.fragment="#version 300 es\nprecision highp float;out vec4 color;void main(){color=vec4(1.);}";
    const auto result=VulkanShaderCompiler::compileESSL(pair);INFO(result.diagnostics);REQUIRE(result.linked);
    const auto& words=result.modules[0].words;std::optional<std::pair<unsigned,unsigned>> owner;
    for(std::size_t i=5;i<words.size();i+=words[i]>>16)
        if((words[i]&0xffff)==72&&words[i+3]==7){owner={{words[i+1],words[i+2]}};break;}
    REQUIRE(owner);
    for(const bool member:{false,true}) {
        auto edited=words;const auto group=edited[3]++;std::vector<std::uint32_t> annotations{(2u<<16)|73u,group};
        if(member)annotations.insert(annotations.end(),{(4u<<16)|75u,group,owner->first,owner->second});
        else annotations.insert(annotations.end(),{(3u<<16)|74u,group,owner->first});
        bool inserted=false;
        for(std::size_t i=5;i<edited.size();i+=edited[i]>>16)if((edited[i]&0xffff)==19){edited.insert(edited.begin()+i,annotations.begin(),annotations.end());inserted=true;break;}
        REQUIRE(inserted);checkMatrixStorage(edited,false,2);
    }
}
