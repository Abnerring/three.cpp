#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <iostream>
#include <string>
#include <vector>
struct Case { const char* name; const char* extension; const char* body; bool valid; };
int main() {
    glslang::InitializeProcess();
    const Case cases[]{
        {"user-average-disabled-extension", "", "float average(vec3 x){return dot(x,vec3(0.3333333));} void main(){gl_Position=vec4(average(vec3(1.0)));}", true},
        {"user-average-repeated-prototype", "", "float average(vec3 x); float average(vec3 x); float average(vec3 x){return dot(x,vec3(0.3333333));} void main(){gl_Position=vec4(average(vec3(1.0)));}", true},
        {"user-average-exact-inactive-signature", "", "ivec2 average(ivec2 a,ivec2 b){return a+b;} void main(){gl_Position=vec4(vec2(average(ivec2(1),ivec2(2))),0.0,1.0);}", true},
        {"user-average-enabled-extension-rejected", "#extension GL_INTEL_shader_integer_functions2 : enable\n", "float average(vec3 x){return x.x;} void main(){gl_Position=vec4(1.0);}", false},
        {"user-average-explicitly-disabled-extension", "#extension GL_INTEL_shader_integer_functions2 : disable\n", "float average(vec3 x){return x.x;} void main(){gl_Position=vec4(average(vec3(1.0)));}", true},
        {"core-overload-remains-rejected", "", "float sqrt(vec3 x){return x.x;} void main(){gl_Position=vec4(1.0);}", false},
        {"core-redefinition-remains-rejected", "", "float sqrt(float x){return x;} void main(){gl_Position=vec4(sqrt(1.0));}", false},
        {"inactive-extension-call-remains-rejected", "", "void main(){gl_Position=vec4(float(average(1,2)));}", false},
        {"active-extension-call-accepted", "#extension GL_INTEL_shader_integer_functions2 : enable\n", "void main(){gl_Position=vec4(float(average(1,2)));}", true},
        {"core-usage-accepted", "", "void main(){gl_Position=vec4(sqrt(1.0));}", true}
    };
    unsigned pass=0;
    for (auto stage : {EShLangVertex, EShLangFragment}) for (const auto& item:cases) {
        std::string source="#version 300 es\n";source+=item.extension;source+="precision highp float;precision highp int;\n";source+=item.body;
        if (stage==EShLangFragment) {
            std::size_t position=0;
            while ((position=source.find("gl_Position",position))!=std::string::npos) {source.replace(position,11,"fixtureOutput");position+=13;}
            source.insert(source.find("precision highp float;"),"out highp vec4 fixtureOutput;\n");
        }
        glslang::TShader shader(stage);
        const char* text=source.c_str();shader.setStrings(&text,1);
        shader.setEnvInput(glslang::EShSourceGlsl,stage,glslang::EShClientNone,300);
        const bool parsed=shader.parse(GetDefaultResources(),300,EEsProfile,false,false,EShMsgDefault);
        const bool matches=parsed==item.valid;pass+=matches;
        std::cout<<(stage==EShLangVertex?"vertex":"fragment")<<'/'<<item.name<<" expected="<<item.valid<<" actual="<<parsed<<" pass="<<matches<<'\n';
        if (!matches) std::cout<<shader.getInfoLog();
    }
    std::cout<<"cases="<<std::size(cases)*2<<" passed="<<pass<<'\n';
    glslang::FinalizeProcess();return pass==std::size(cases)*2?0:1;
}
