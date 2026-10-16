#include "renderers/vulkan/VulkanShaderCompiler.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <thread>
#include <vector>
#include <atomic>
#include <filesystem>
#include <algorithm>
namespace fs=std::filesystem;
std::string read(const char* path) {
    std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("Missing lifecycle input");
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(int argc,char** argv) {
    if(argc!=4)return 2;
    // <fixture-directory> <output-directory> <existing compile450 validator>
    try {
        const fs::path sources=argv[1],output=argv[2];fs::create_directories(output);
        const VulkanEsslSourcePair source{read((sources/"positive.vert").string().c_str()),read((sources/"positive.frag").string().c_str())};
        for(const auto* name:{"positive","negative","front","back","early","intermediate","positivew","negativew","derivatives"}) {
            const auto r=VulkanShaderCompiler::compileESSL({read((sources/(std::string(name)+".vert")).string().c_str()),read((sources/(std::string(name)+".frag")).string().c_str())});
            require(r.linked,"Builtin fixture rejected");
            for(unsigned i=0;i<2;++i) {
                require(r.modules[i].validated&&!r.modules[i].words.empty(),"Unvalidated emitted fixture");
                std::ofstream file(output/(std::string(name)+(i?".frag.spv":".vert.spv")),std::ios::binary);
                file.write(reinterpret_cast<const char*>(r.modules[i].words.data()),static_cast<std::streamsize>(r.modules[i].words.size()*sizeof(std::uint32_t)));
                require(bool(file),"Fixture output failed");
            }
            for(const auto& mapping:r.backendScalars) {
                require((mapping.usedStages&~mapping.declaredStages)==0,"Private used/declaration stage mask");
                for(unsigned i=0;i<2;++i)if(mapping.declaredStages&(1u<<i)) {
                    const VulkanShaderInterface::Descriptor* block=nullptr;
                    for(const auto& d:r.interfaces[i].descriptors)if(d.set==mapping.set&&d.binding==mapping.binding)block=&d;
                    require(block&&!block->block.members.empty(),"Private member has no complete owning block");
                    const auto found=std::find_if(block->block.members.begin(),block->block.members.end(),[&](const auto& member){return member.name==mapping.name;});
                    require(found!=block->block.members.end()&&found->absoluteOffset==mapping.offset&&found->size==4,"Private scalar mapping mismatch");
                    const auto expected=mapping.value==VulkanEsslResult::BackendScalar::Value::FirstInstance?VulkanShaderInterface::Numeric::Scalar::SignedInteger:VulkanShaderInterface::Numeric::Scalar::FloatingPoint;
                    require(found->numeric.scalar==expected&&found->numeric.width==32,"Private scalar numeric type mismatch");
                }
            }
            require(r.activeAttributes.size()==r.attributeMappings.size(),"Original attribute mapping cardinality");
            for(const auto& mapping:r.attributeMappings)require((mapping.builtIn>=0)==(mapping.location==~0u),"Builtin consumed as a buffer attribute");
            for(const auto& mapping:r.varyingMappings)if(mapping.vertexDeclared&&mapping.fragmentDeclared)require(mapping.vertexLocation==mapping.fragmentLocation,"Linked varying mismatch");
        }
        const VulkanEsslSourcePair structured{
            "#version 300 es\nprecision highp float;in vec3 position;uniform mat4 transform;struct Light{vec3 color;float strength;};uniform Light lights[2];uniform vec2 offsets[2];uniform sampler2D images[2];out vec2 uv;void main(){uv=offsets[1];gl_Position=transform*vec4(position+lights[1].color*lights[1].strength+texture(images[1],uv).xyz,1.);}",
            "#version 300 es\nprecision highp float;uniform sampler2D images[2];in vec2 uv;out vec4 color;void main(){color=texture(images[0],uv)+vec4(gl_FragCoord.y*.001);}"};
        const auto mapped=VulkanShaderCompiler::compileESSL(structured);
        if(!mapped.linked)throw std::runtime_error("Structured mapping compile: "+mapped.phase+" "+mapped.diagnostics);
        bool sawMatrix=false,sawStructArray=false,sawVectorArray=false,sawSampler=false;
        for(const auto& mapping:mapped.uniformMappings) {
            require(mapping.sourceName==mapping.rootName+mapping.memberPath,"Source uniform path lost");
            if(mapping.kind==VulkanEsslResult::UniformMapping::Kind::Sampler) {
                sawSampler=true;require(mapping.set==0&&mapping.binding>0,"Sampler collided with default block");
                for(unsigned i=0;i<2;++i)if(mapping.stages&(1u<<i)) {
                    const auto& ds=mapped.interfaces[i].descriptors;
                    const auto d=std::find_if(ds.begin(),ds.end(),[&](const auto& v){return v.set==mapping.set&&v.binding==mapping.binding;});
                    require(d!=ds.end()&&d->count==2,"Sampler array descriptor count lost");
                }
            } else if(mapping.rootName=="transform") {
                sawMatrix=true;require(mapping.rootLayout.members.empty(),"Matrix rootLayout must be leaf");
                require(mapping.rootLayout.numeric.columns==4&&mapping.rootLayout.numeric.matrixStride==16,"std140 mat4 layout");
            } else if(mapping.rootName=="lights") {
                sawStructArray=true;require(mapping.rootLayout.array.dimensions==std::vector<std::uint32_t>{2},"Struct array extent");
                require(mapping.rootLayout.array.stride==16&&mapping.rootLayout.members.size()==2,"std140 vec3+float struct array stride");
            } else if(mapping.rootName=="offsets") {
                sawVectorArray=true;require(mapping.rootLayout.array.dimensions==std::vector<std::uint32_t>{2}&&mapping.rootLayout.array.stride==16,"std140 vec2 array stride");
            }
        }
        require(sawMatrix&&sawStructArray&&sawVectorArray&&sawSampler,"Structured active paths not covered");
        const auto baseline=VulkanShaderCompiler::compileESSL(source);require(baseline.linked,"Lifecycle baseline rejected");
        const auto invalid=VulkanShaderCompiler::compileESSL({"#version 300 es\nvoid main(){missing=1.;}",source.fragment});
        require(!invalid.linked&&invalid.phase=="parse-original-vertex","Original invalid source must fail");
        auto nul=source;nul.vertex.push_back('\0');require(!VulkanShaderCompiler::compileESSL(nul).linked,"NUL input accepted");
        auto userBlock=source;userBlock.vertex="#version 300 es\nlayout(std140) uniform U{vec4 p;};void main(){gl_Position=p;}";
        userBlock.fragment="#version 300 es\nprecision highp float;out vec4 color;void main(){color=vec4(1.);}";
        auto rejected=VulkanShaderCompiler::compileESSL(userBlock);
        require(!rejected.linked&&rejected.phase=="collect-AST-schema"&&rejected.diagnostics.find("Existing user uniform blocks")!=std::string::npos,"Vertex ShaderMaterial user UBO boundary changed");
        auto fragmentBlock=source;
        fragmentBlock.fragment="#version 300 es\nprecision highp float;layout(std140) uniform UserBlock{vec4 p;};out vec4 color;void main(){color=p;}";
        rejected=VulkanShaderCompiler::compileESSL(fragmentBlock);
        require(!rejected.linked&&rejected.phase=="collect-AST-schema"&&rejected.diagnostics.find("Existing user uniform blocks")!=std::string::npos,"Fragment ShaderMaterial user UBO boundary changed");
        std::atomic<unsigned> matches{0};std::vector<std::thread> workers;
        for(unsigned i=0;i<8;++i)workers.emplace_back([&] {
            for(unsigned repeat=0;repeat<3;++repeat) {
                const auto compiled=VulkanShaderCompiler::compileESSL(source);
                if(compiled.linked&&compiled.modules[0].words==baseline.modules[0].words&&compiled.modules[1].words==baseline.modules[1].words)++matches;
            }
        });
        for(auto& worker:workers)worker.join();require(matches==24,"Serialized cross-thread compilation drift");
        VulkanShaderCompiler cli(argv[3]);
        const auto native450=cli.compile("#version 450\nlayout(location=0) in vec3 position;void main(){gl_Position=vec4(position,1.);}","#version 450\nlayout(location=0) out vec4 color;void main(){color=vec4(1.);}");
        require(native450.linked&&!native450.vertex.empty()&&!native450.fragment.empty(),"Original compile450 regression");
        std::cout<<"9 builtin pairs emitted; structured matrix/array/sampler mappings; 24/24 serialized thread recompiles byte-identical; parse/NUL/user-UBO rejection recovery; original450 CLI linked\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
