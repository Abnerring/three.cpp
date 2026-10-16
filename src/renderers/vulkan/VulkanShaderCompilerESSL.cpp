#include "renderers/vulkan/VulkanShaderCompiler.h"
#include "essl/NativeCompile.h"
#include <algorithm>
#include <map>
#include <stdexcept>
#include <utility>
namespace {
using Interface=VulkanShaderInterface;
const Interface::Descriptor* descriptor(const Interface& source,std::uint32_t binding) {
    for(const auto& d:source.descriptors) if(d.set==0 && d.binding==binding)return &d;
    return nullptr;
}
const Interface::Block* member(const Interface::Descriptor* d,const std::string& name) {
    if(d)for(const auto& m:d->block.members)if(m.name==name)return &m;
    return nullptr;
}
std::string rootName(const std::string& name) {return name.substr(0,name.find_first_of(".["));}
}
VulkanEsslStageResult VulkanShaderCompiler::validateESSLStage(VulkanEsslStage stage,const std::string& source,const std::string& name) {
    return threecppEssl::validateStage(stage,source,name);
}
VulkanShaderCompiler::ESSLResult VulkanShaderCompiler::compileESSL(const VulkanEsslSourcePair& pair) {
    return compileESSL(pair,{});
}
VulkanShaderCompiler::ESSLResult VulkanShaderCompiler::compileESSL(const VulkanEsslSourcePair& pair,
        std::span<const VulkanEsslAttributeBinding> attributes) {
    ESSLResult result;
    static_cast<VulkanEsslProgram&>(result)=threecppEssl::compile(pair,attributes);
    if(!result.linked)return result;
    result.linked=false;result.phase="reflect-SPIRV-interface";
    try {
        for(std::size_t i=0;i<2;++i)result.interfaces[i]=Interface::read(result.modules[i].words);
        result.phase="link-owned-mappings";
        for(const auto& u:result.activeUniforms) {
            ESSLResult::UniformMapping mapped;mapped.sourceName=u.name;mapped.rootName=rootName(u.name);
            mapped.memberPath=u.name.substr(mapped.rootName.size());
            const auto sampler=std::find_if(result.samplers.begin(),result.samplers.end(),[&](const auto& s){return s.name==mapped.rootName;});
            if(sampler!=result.samplers.end()) {
                mapped.kind=ESSLResult::UniformMapping::Kind::Sampler;mapped.binding=sampler->binding;
                for(unsigned i=0;i<2;++i)if(descriptor(result.interfaces[i],mapped.binding))mapped.stages|=1u<<i;
            } else {
                for(unsigned i=0;i<2;++i)if(const auto m=member(descriptor(result.interfaces[i],0),mapped.rootName)) {
                    if(!mapped.stages)mapped.rootLayout=*m;
                    else if(mapped.rootLayout.offset!=m->offset || mapped.rootLayout.size!=m->size)
                        throw std::runtime_error("Cross-stage reflected uniform layout mismatch: "+u.name);
                    mapped.stages|=1u<<i;
                }
            }
            if(!mapped.stages)throw std::runtime_error("Original active uniform has no emitted mapping: "+u.name);
            result.uniformMappings.push_back(std::move(mapped));
        }
        for(const auto& a:result.activeAttributes) {
            const auto& inputs=result.interfaces[0].inputs;
            auto found=std::find_if(inputs.begin(),inputs.end(),[&](const auto& v){return v.name==a.name && (v.location!=~0u || v.builtIn>=0);});
            if(found==inputs.end())throw std::runtime_error("Original active attribute has no emitted location: "+a.name);
            result.attributeMappings.push_back({a.name,a.glType,a.size,found->location,found->component,found->builtIn,found->numeric,found->array});
        }
        std::map<std::string,ESSLResult::VaryingMapping> varyings;
        for(const auto& v:result.interfaces[0].outputs)if(v.location!=~0u) {
            auto& mapping=varyings[v.name];mapping.name=v.name;mapping.vertexLocation=v.location;mapping.vertexDeclared=true;
        }
        for(const auto& v:result.interfaces[1].inputs)if(v.location!=~0u) {
            auto& mapping=varyings[v.name];mapping.name=v.name;mapping.fragmentLocation=v.location;mapping.fragmentDeclared=true;
            if(mapping.vertexDeclared && mapping.vertexLocation!=mapping.fragmentLocation)
                throw std::runtime_error("Linked varying location mismatch: "+v.name);
        }
        for(auto& [name,mapping]:varyings)result.varyingMappings.push_back(std::move(mapping));
        for(unsigned scalar=0;scalar<2;++scalar) {
            ESSLResult::BackendScalar mapped;
            mapped.value=scalar?ESSLResult::BackendScalar::Value::FramebufferHeight:ESSLResult::BackendScalar::Value::FirstInstance;
            mapped.name=scalar?"threecppBackendFramebufferHeight":"threecppBackendBaseInstance";
            for(unsigned i=0;i<2;++i)if(const auto m=member(descriptor(result.interfaces[i],0),mapped.name)) {
                const auto expected=scalar?Interface::Numeric::Scalar::FloatingPoint:Interface::Numeric::Scalar::SignedInteger;
                if(m->numeric.scalar!=expected || m->numeric.width!=32 || m->size!=4 || !m->array.dimensions.empty())
                    throw std::runtime_error("Private backend scalar type mismatch: "+mapped.name);
                if(mapped.declaredStages && (mapped.offset!=m->absoluteOffset || mapped.size!=m->size))
                    throw std::runtime_error("Cross-stage private backend offset mismatch: "+mapped.name);
                mapped.offset=m->absoluteOffset;mapped.size=m->size;mapped.numeric=m->numeric;
                mapped.declaredStages|=1u<<i;if(m->used)mapped.usedStages|=1u<<i;
            }
            result.backendScalars.push_back(std::move(mapped));
        }
        result.linked=true;result.phase="complete";
    }catch(const std::exception& error){result.diagnostics=error.what();}
    return result;
}
