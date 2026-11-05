#include "renderers/vulkan/VulkanShaderCompiler.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <sstream>
namespace {
unsigned checks=0;
void check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
unsigned location(const VulkanEsslResult& result,const char* name) {
 const auto it=std::find_if(result.attributeMappings.begin(),result.attributeMappings.end(),[&](const auto& a){return a.sourceName==name;});
 if(it==result.attributeMappings.end())throw std::runtime_error("Missing attribute");return it->location;
}
struct Instruction {std::string result,op;std::vector<std::string> args;};
std::vector<Instruction> instructions(const std::string& assembly) {
 std::vector<Instruction> result;std::istringstream lines(assembly);std::string line;
 while(std::getline(lines,line)){
  std::istringstream words(line);Instruction instruction;std::string first,word;if(!(words>>first)||first.starts_with(';'))continue;
  if(first.starts_with('%')){instruction.result=first;words>>word>>instruction.op;}else instruction.op=first;
  while(words>>word)instruction.args.push_back(word);result.push_back(std::move(instruction));
 }
 return result;
}
const Instruction& definition(const std::vector<Instruction>& code,const std::string& id) {
 const auto found=std::find_if(code.begin(),code.end(),[&](const auto& i){return i.result==id;});
 if(found==code.end())throw std::runtime_error("Missing emitted instruction result: "+id);return *found;
}
const Instruction& operation(const std::vector<Instruction>& code,const char* op) {
 const auto found=std::find_if(code.begin(),code.end(),[&](const auto& i){return i.op==op;});
 if(found==code.end())throw std::runtime_error(std::string("Missing emitted operation: ")+op);return *found;
}
unsigned operations(const std::vector<Instruction>& code,const char* op) {
 return unsigned(std::count_if(code.begin(),code.end(),[&](const auto& i){return i.op==op;}));
}
bool constant(const std::vector<Instruction>& code,const std::string& id,float expected) {
 const auto& instruction=definition(code,id);
 return instruction.op=="OpConstant"&&instruction.args.size()==2&&std::stof(instruction.args[1])==expected;
}
// Trace actual emitted load/component paths, allowing glslang to use either
// pointer access chains or value extraction for a source vector component.
std::string origin(const std::vector<Instruction>& code,const std::string& id) {
 const auto& instruction=definition(code,id);
 if(instruction.op=="OpLoad"||instruction.op=="OpCopyObject")return origin(code,instruction.args.at(1));
 if(instruction.op=="OpAccessChain"||instruction.op=="OpInBoundsAccessChain"||instruction.op=="OpCompositeExtract"){
  auto result=origin(code,instruction.args.at(1));
  for(std::size_t i=2;i<instruction.args.size();++i){
   const auto& index=instruction.args[i];
   result+='[';
   result+=index.starts_with('%')?definition(code,index).args.back():index;result+=']';
  }
  return result;
 }
 return id;
}
const VulkanEsslResult::BackendScalar& heightField(const VulkanEsslResult& result) {
 const auto field=std::find_if(result.backendScalars.begin(),result.backendScalars.end(),[](const auto& b){return b.value==VulkanEsslResult::BackendScalar::Value::FramebufferHeight;});
 if(field==result.backendScalars.end())throw std::runtime_error("Missing private signed-height mapping");return *field;
}
VulkanEsslResult coordinateProgram(const std::string& body,bool varying=false) {
 const std::string vertex=std::string("#version 300 es\nprecision highp float;in vec3 position;")+
  (varying?"out vec4 value;":"")+"void main(){"+(varying?"value=vec4(position,1.);":"")+"gl_Position=vec4(position,1.);}";
 const std::string fragment=std::string("#version 300 es\nprecision highp float;")+(varying?"in vec4 value;":"")+"out vec4 color;"+body;
 auto result=VulkanShaderCompiler::compileESSL({vertex,fragment});
 if(!result.linked)throw std::runtime_error("Coordinate fixture: "+result.phase+": "+result.diagnostics);
 check(result.modules[0].validated&&result.modules[1].validated,"Coordinate fixture emitted invalid SPIR-V");
 return result;
}
void coordinateBackend() {
 const auto frag=coordinateProgram("void main(){color=gl_FragCoord;}");
 const auto code=instructions(frag.modules[1].assembly);const auto& selected=operation(code,"OpSelect");
 check(heightField(frag).usedStages==2,"gl_FragCoord must read the private height in the fragment stage");
 const auto& condition=definition(code,selected.args.at(1));const auto& screenY=definition(code,selected.args.at(3));
 check(condition.op=="OpFOrdLessThan"&&constant(code,condition.args.at(2),0),"Coordinate selection must test signed height < 0");
 check(screenY.op=="OpFSub","Screen gl_FragCoord.y must remain height minus native y");
 check(origin(code,condition.args.at(1))==origin(code,screenY.args.at(1)),"Coordinate comparison and screen subtraction must read the same private height");
 check(origin(code,selected.args.at(2))==origin(code,screenY.args.at(2)),"Render-target gl_FragCoord.y must retain the native y component");
 check(frag.activeUniforms.empty()&&frag.uniformMappings.empty(),"Coordinate backend uniforms leaked into original active reflection");
 for(unsigned width=1;width<=4;++width){
  const auto expression=width==1?"value.y":width==2?"value.xy":width==3?"value.xyz":"value";
  const auto type=width==1?"float":std::string("vec")+std::to_string(width);
  const auto output=width==1?"vec4(derivative,calls,0.,1.)":width==2?"vec4(derivative,calls,1.)":width==3?"vec4(derivative,calls)":"derivative+vec4(calls)";
  const auto derivative=coordinateProgram(std::string("float calls;")+type+" evaluate(){calls+=1.;return "+expression+";}void main(){calls=0.;"+type+" derivative=dFdy(evaluate());color="+output+";}",true);
  const auto& height=heightField(derivative);const auto ir=instructions(derivative.modules[1].assembly);
  check(height.declaredStages==2&&height.usedStages==2,"A derivative-only fragment must declare and use its backend block");
  check(height.size==4&&height.numeric.width==32&&height.numeric.scalar==VulkanShaderInterface::Numeric::Scalar::FloatingPoint,"Derivative backend height lost its scalar float32 layout");
  check(operations(ir,"OpDPdy")==1&&operations(ir,"OpFunctionCall")==1,"dFdy adaptation duplicated its derivative or side-effecting operand");
  const auto& dy=operation(ir,"OpDPdy");const auto& sign=operation(ir,"OpSelect");
  const auto& comparison=definition(ir,sign.args.at(1));
  check(comparison.op=="OpFOrdLessThan"&&constant(ir,comparison.args.at(2),0),"Derivative sign must depend on the actual signed height");
  check(constant(ir,sign.args.at(2),1)&&constant(ir,sign.args.at(3),-1),"Render-target/screen derivatives require signs +1/-1 respectively");
  const auto normalized=std::find_if(ir.begin(),ir.end(),[&](const auto& i){return i.op==(width==1?"OpFMul":"OpVectorTimesScalar")&&i.args.size()==3&&i.args[1]==dy.result&&i.args[2]==sign.result;});
  check(normalized!=ir.end(),"Scalar/vector derivative does not consume the viewport sign exactly once");
  const auto& resultType=definition(ir,dy.args.at(0));
  check(width==1?resultType.op=="OpTypeFloat":resultType.op=="OpTypeVector"&&resultType.args.at(1)==std::to_string(width),"Derivative vector width changed during adaptation");
  check(derivative.activeUniforms.empty()&&derivative.uniformMappings.empty(),"Derivative backend schema leaked into original application uniforms");
 }
 const auto absolute=coordinateProgram("void main(){color=fwidth(value)+dFdx(value);}",true);
 const auto absoluteCode=instructions(absolute.modules[1].assembly);
 check(heightField(absolute).declaredStages==0&&heightField(absolute).usedStages==0,"fwidth/dFdx-only shaders must not acquire a coordinate backend dependency");
 check(operations(absoluteCode,"OpFwidth")==1&&operations(absoluteCode,"OpDPdx")==1&&operations(absoluteCode,"OpSelect")==0,"Absolute derivatives or x derivatives were modified");
 const auto gradients=coordinateProgram("uniform sampler2D map;void main(){color=textureGrad(map,value.xy,vec2(.1,.2),vec2(.3,.4));}",true);
 check(heightField(gradients).usedStages==0,"Explicit textureGrad arguments must not gain an unsupported viewport rewrite");
 check(operations(instructions(gradients.modules[1].assembly),"OpImageSampleExplicitLod")==1,"Original textureGrad operation was lost");
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
 coordinateBackend();
 std::cout<<checks<<" compiler API checks passed; independent stage parsing, real link mismatch, bound/explicit attributes, serialized frontend lifetime, signed-height coordinate/derivative SPIR-V and reflection\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
