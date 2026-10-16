#include "CompilerInternal.h"
#include <glslang/Public/ResourceLimits.h>
#include <limits>
#include <mutex>
#include <stdexcept>
namespace threecppEssl {
namespace {
struct Process {
    std::mutex mutex;
    Process() { if(!glslang::InitializeProcess()) throw std::runtime_error("glslang initialization failed"); }
    ~Process() { glslang::FinalizeProcess(); }
};
Process& process() { static Process state; return state; }
std::uint32_t stages(EShLanguageMask mask) {
    return ((mask & EShLangVertexMask)?1u:0u) | ((mask & EShLangFragmentMask)?2u:0u);
}
void parse(glslang::TShader& shader,const std::string& source,const std::string& name,EShLanguage stage) {
    if(source.size()>std::size_t(std::numeric_limits<int>::max())) throw std::invalid_argument("ESSL source exceeds native parser length");
    if(source.find('\0')!=std::string::npos || name.find('\0')!=std::string::npos)
        throw std::invalid_argument("Embedded NUL in ESSL source or diagnostic name");
    const char* text=source.data(); const int length=static_cast<int>(source.size()); const char* label=name.c_str();
    shader.setStringsWithLengthsAndNames(&text,&length,&label,1);
    shader.setEnvInput(glslang::EShSourceGlsl,stage,glslang::EShClientNone,0);
    if(!shader.parse(GetDefaultResources(),100,EEsProfile,false,false,EShMsgDefault))
        throw std::runtime_error(std::string(shader.getInfoLog())+shader.getInfoDebugLog());
}
}
VulkanEsslStageResult validateStage(VulkanEsslStage stage,const std::string& source,const std::string& name) {
    VulkanEsslStageResult result;
    try {
        if(stage!=VulkanEsslStage::Vertex&&stage!=VulkanEsslStage::Fragment)throw std::invalid_argument("Unsupported ESSL stage");
        const auto language=stage==VulkanEsslStage::Vertex?EShLangVertex:EShLangFragment;
        auto& state=process();std::lock_guard guard(state.mutex);
        glslang::TShader shader(language);
        result.phase=stage==VulkanEsslStage::Vertex?"parse-original-vertex":"parse-original-fragment";
        parse(shader,source,name,language);
        result.originalVersion=shader.getIntermediate()->getVersion();
        result.originalProfile=shader.getIntermediate()->getProfile();
        if(result.originalProfile!=EEsProfile)throw std::invalid_argument("validateESSLStage requires original ES source profile");
        result.diagnostics=std::string(shader.getInfoLog())+shader.getInfoDebugLog();
        result.compiled=true;result.phase="complete";
    }catch(const std::exception& error){result.diagnostics=error.what();}
    return result;
}
VulkanEsslProgram compile(const VulkanEsslSourcePair& pair,std::span<const VulkanEsslAttributeBinding> attributes) {
    VulkanEsslProgram result;
    try {
        auto& state=process();
        std::lock_guard guard(state.mutex);
        glslang::TShader vertex(EShLangVertex),fragment(EShLangFragment);
        result.phase="parse-original-vertex";parse(vertex,pair.vertex,pair.vertexName,EShLangVertex);
        result.phase="parse-original-fragment";parse(fragment,pair.fragment,pair.fragmentName,EShLangFragment);
        result.phase="link-original";
        glslang::TProgram program;program.addShader(&vertex);program.addShader(&fragment);
        if(!program.link(EShMsgDefault))throw std::runtime_error(std::string(program.getInfoLog())+program.getInfoDebugLog());
        const EShLanguage languages[]{EShLangVertex,EShLangFragment};
        for(int i=0;i<2;++i) {
            result.originalVersions[i]=program.getIntermediate(languages[i])->getVersion();
            result.originalProfiles[i]=program.getIntermediate(languages[i])->getProfile();
            if(result.originalProfiles[i]!=EEsProfile) throw std::invalid_argument("compileESSL requires original ES source profile");
        }
        result.phase="reflect-original-AST";
        if(!program.buildReflection())throw std::runtime_error("Original linked AST reflection failed");
        for(int i=0;i<program.getNumUniformVariables();++i) {
            const auto& u=program.getUniform(i);
            result.activeUniforms.push_back({u.name,u.glDefineType,u.size,u.index,u.offset,u.arrayStride,stages(u.stages)});
        }
        for(int i=0;i<program.getNumPipeInputs();++i) {
            const auto& a=program.getPipeInput(i);result.activeAttributes.push_back({a.name,a.glDefineType,a.size});
        }
        result.phase="collect-AST-schema";mapAst(program,result,attributes);
        result.phase="emit-SPIRV";
        for(int i=0;i<2;++i) {
            result.modules[i]=emit(*program.getIntermediate(languages[i]));
            if(!result.modules[i].validated) throw std::runtime_error("Vulkan SPIR-V validation failed: "+result.modules[i].validator);
        }
        result.linked=true;result.phase="complete";
    } catch(const std::exception& error) {result.diagnostics=error.what();}
    return result;
}
} // namespace threecppEssl
