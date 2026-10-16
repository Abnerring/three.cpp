#include "renderers/vulkan/VulkanShaderCompiler.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <atomic>
namespace {
unsigned checks=0;
void check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
unsigned location(const VulkanEsslResult& result,const char* name) {
 const auto it=std::find_if(result.attributeMappings.begin(),result.attributeMappings.end(),[&](const auto& a){return a.sourceName==name;});
 if(it==result.attributeMappings.end())throw std::runtime_error("Missing attribute");return it->location;
}
}
int main(){try {
 const std::string vs="#version 300 es\nprecision highp float;in vec3 position;in vec3 normal;out vec3 unmatched;void main(){unmatched=normal;gl_Position=vec4(position+normal,1.);}";
 const std::string fs="#version 300 es\nprecision highp float;out vec4 color;void main(){color=vec4(1.);}";
 auto stage=VulkanShaderCompiler::validateESSLStage(VulkanEsslStage::Vertex,vs);
 check(stage.compiled&&stage.originalVersion==300&&stage.phase=="complete","Real independent vertex parser failed");
 stage=VulkanShaderCompiler::validateESSLStage(VulkanEsslStage::Fragment,"#version 300 es\nprecision highp float;in vec2 unmatched;out vec4 color;void main(){color=vec4(unmatched,0.,1.);}");
 check(stage.compiled,"Fragment parsing incorrectly depends on a dummy vertex sibling");
 auto pair=VulkanShaderCompiler::compileESSL({vs,"#version 300 es\nprecision highp float;in vec2 unmatched;out vec4 color;void main(){color=vec4(unmatched,0.,1.);}"});
 check(!pair.linked&&pair.phase=="link-original","Stage parsing must not replace actual pair link validation");
 stage=VulkanShaderCompiler::validateESSLStage(VulkanEsslStage::Vertex,"#version 300 es\nvoid main(){missing=1.;}","original.vert");
 check(!stage.compiled&&stage.phase=="parse-original-vertex"&&!stage.diagnostics.empty(),"Invalid original vertex accepted");
 stage=VulkanShaderCompiler::validateESSLStage(VulkanEsslStage::Fragment,"#version 300 es\nvoid main(){gl_Position=vec4(1.);}");
 check(!stage.compiled,"Fragment stage checked with vertex rules");
 stage=VulkanShaderCompiler::validateESSLStage(VulkanEsslStage::Vertex,"#version 450\nvoid main(){gl_Position=vec4(1.);}");
 check(!stage.compiled,"Desktop profile accepted by ESSL stage API");
 stage=VulkanShaderCompiler::validateESSLStage(static_cast<VulkanEsslStage>(99),vs);
 check(!stage.compiled,"Unknown stage accepted");
 auto nul=vs;nul.push_back('\0');check(!VulkanShaderCompiler::validateESSLStage(VulkanEsslStage::Vertex,nul).compiled,"NUL source accepted");
 const std::vector<VulkanEsslAttributeBinding> bindings{{"position",5},{"normal",1}};
 pair=VulkanShaderCompiler::compileESSL({vs,fs},bindings);
 if(!pair.linked)throw std::runtime_error(pair.phase+": "+pair.diagnostics);
 check(location(pair,"position")==5&&location(pair,"normal")==1,"Requested attribute bindings ignored");
 check(pair.modules[0].validated&&pair.modules[1].validated,"Bound pair not SPIR-V validated");
 auto explicitSource=vs;explicitSource.replace(explicitSource.find("in vec3 position"),16,"layout(location=3) in vec3 position");
 pair=VulkanShaderCompiler::compileESSL({explicitSource,fs},bindings);
 check(pair.linked&&location(pair,"position")==3&&location(pair,"normal")==1,"Explicit ESSL location must override requested binding");
 const std::vector<VulkanEsslAttributeBinding> rebound{{"position",2},{"position",4}};
 pair=VulkanShaderCompiler::compileESSL({vs,fs},rebound);
 check(pair.linked&&location(pair,"position")==4,"Last attribute binding must win");
 const std::vector<VulkanEsslAttributeBinding> impossible{{"position",0xffffffffu}};
 check(!VulkanShaderCompiler::compileESSL({vs,fs},impossible).linked,"Unrepresentable frontend attribute binding accepted");
 const auto baseline=VulkanShaderCompiler::compileESSL({vs,fs});
 pair=VulkanShaderCompiler::compileESSL({vs,fs},{});
 check(pair.linked&&pair.modules[0].words==baseline.modules[0].words&&pair.modules[1].words==baseline.modules[1].words,"Empty binding overload changed original pair output");
 std::atomic<unsigned> parsed=0;std::vector<std::thread> threads;
 for(unsigned i=0;i<8;++i)threads.emplace_back([&]{if(VulkanShaderCompiler::validateESSLStage(VulkanEsslStage::Vertex,vs).compiled)++parsed;});
 for(auto& t:threads)t.join();check(parsed==8,"Shared native frontend lifetime/locking failed");
 std::cout<<checks<<" compiler API checks passed; independent stage parsing, real link mismatch, bound/explicit attributes, serialized frontend lifetime\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
