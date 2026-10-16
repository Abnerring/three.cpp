#include "CompilerInternal.h"
#include <SPIRV/GlslangToSpv.h>
#include <spirv-tools/libspirv.hpp>
#include <sstream>
#include <stdexcept>
namespace threecppEssl {
VulkanEsslProgram::Module emit(const glslang::TIntermediate& intermediate) {
    VulkanEsslProgram::Module result;
    spv::SpvBuildLogger logger;
    glslang::SpvOptions options;options.disableOptimizer=true;options.generateDebugInfo=false;
    glslang::GlslangToSpv(intermediate,result.words,&logger,&options);
    result.logger=logger.getAllMessages();
    if(result.words.size()<5 || result.words[0]!=0x07230203)throw std::runtime_error("Invalid SPIR-V framing");
    spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_0);
    std::ostringstream diagnostics;
    tools.SetMessageConsumer([&](spv_message_level_t,const char*,const spv_position_t& p,const char* message) {
        diagnostics<<p.index<<": "<<message<<'\n';
    });
    result.validated=tools.Validate(result.words.data(),result.words.size());
    if(!tools.Disassemble(result.words,&result.assembly,SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES|SPV_BINARY_TO_TEXT_OPTION_INDENT))
        throw std::runtime_error("Disassembly failed: "+diagnostics.str());
    result.validator=diagnostics.str();return result;
}
} // namespace threecppEssl
