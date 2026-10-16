#include "renderers/vulkan/VulkanShaderCompiler.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
namespace fs=std::filesystem;
std::string read(const fs::path& path) {
    std::ifstream stream(path,std::ios::binary);stream.exceptions(std::ios::badbit|std::ios::failbit);
    std::ostringstream out;out<<stream.rdbuf();return out.str();
}
void write(const fs::path& path,const std::string& text) {
    std::ofstream stream(path,std::ios::binary);stream.exceptions(std::ios::badbit|std::ios::failbit);
    stream.write(text.data(),static_cast<std::streamsize>(text.size()));stream.close();
}
std::string quote(const std::string& text) {
    std::ostringstream out;out<<'"';
    for(unsigned char c:text) {
        if(c=='"'||c=='\\')out<<'\\'<<char(c);
        else if(c=='\n')out<<"\\n";else if(c=='\r')out<<"\\r";else if(c=='\t')out<<"\\t";
        else if(c<32){constexpr char hex[]="0123456789abcdef";out<<"\\u00"<<hex[c>>4]<<hex[c&15];}
        else out<<char(c);
    }out<<'"';return out.str();
}
int main(int argc,char** argv) {
    if(argc!=5||std::string(argv[4])!="--gl-builtins")return 2;
    try {
        const auto r=VulkanShaderCompiler::compileESSL({read(argv[1]),read(argv[2]),argv[1],argv[2]});
        std::ostringstream out;
        if(!r.linked)out<<"{\"ok\":false,\"phase\":"<<quote(r.phase)<<",\"error\":"<<quote(r.diagnostics)<<'}';
        else {
            out<<"{\"ok\":true,\"originalVersions\":["<<r.originalVersions[0]<<','<<r.originalVersions[1]<<"],\"originalProfiles\":["<<r.originalProfiles[0]<<','<<r.originalProfiles[1]<<"],\"originalActiveUniforms\":[";
            bool first=true;
            for(const auto& u:r.activeUniforms){if(!first)out<<',';first=false;out<<"{\"name\":"<<quote(u.name)<<",\"glType\":"<<u.glType<<",\"size\":"<<u.size<<",\"block\":"<<u.block<<",\"offset\":"<<u.offset<<",\"arrayStride\":"<<u.arrayStride<<'}';}
            out<<"],\"originalActiveAttributes\":[";first=true;
            for(const auto& a:r.activeAttributes){if(!first)out<<',';first=false;out<<"{\"name\":"<<quote(a.name)<<",\"glType\":"<<a.glType<<",\"size\":"<<a.size<<'}';}
            out<<"],\"uniforms\":[";first=true;
            for(const auto& u:r.uniforms){if(!first)out<<',';first=false;out<<"{\"name\":"<<quote(u.name)<<",\"type\":"<<quote(u.type)<<'}';}
            out<<"],\"samplers\":[";first=true;
            for(const auto& s:r.samplers){if(!first)out<<',';first=false;out<<"{\"name\":"<<quote(s.name)<<",\"set\":"<<s.set<<",\"binding\":"<<s.binding<<'}';}
            out<<"],\"modules\":[";
            for(unsigned i=0;i<2;++i) {
                const auto& m=r.modules[i];const auto path=std::string(argv[3])+(i?".frag.spv":".vert.spv");
                write(path,std::string(reinterpret_cast<const char*>(m.words.data()),m.words.size()*sizeof(std::uint32_t)));
                write(path+".spvasm",m.assembly);
                if(i)out<<',';out<<"{\"words\":"<<m.words.size()<<",\"validated\":"<<(m.validated?"true":"false")<<",\"validator\":"<<quote(m.validator)<<",\"logger\":"<<quote(m.logger)<<'}';
            }
            out<<"],\"uniformMappings\":[";first=true;
            for(const auto& m:r.uniformMappings){if(!first)out<<',';first=false;out<<"{\"sourceName\":"<<quote(m.sourceName)<<",\"rootName\":"<<quote(m.rootName)<<",\"memberPath\":"<<quote(m.memberPath)<<",\"kind\":"<<int(m.kind)<<",\"set\":"<<m.set<<",\"binding\":"<<m.binding<<",\"stages\":"<<m.stages<<",\"offset\":"<<m.rootLayout.offset<<",\"size\":"<<m.rootLayout.size<<'}';}
            out<<"],\"attributeMappings\":[";first=true;
            for(const auto& m:r.attributeMappings){if(!first)out<<',';first=false;out<<"{\"sourceName\":"<<quote(m.sourceName)<<",\"location\":"<<m.location<<",\"component\":"<<m.component<<",\"builtIn\":"<<m.builtIn<<'}';}
            out<<"],\"backendScalars\":[";first=true;
            for(const auto& m:r.backendScalars){if(!first)out<<',';first=false;out<<"{\"name\":"<<quote(m.name)<<",\"offset\":"<<m.offset<<",\"size\":"<<m.size<<",\"declaredStages\":"<<m.declaredStages<<",\"usedStages\":"<<m.usedStages<<'}';}
            out<<"]}";
        }
        write(std::string(argv[3])+".json",out.str());std::cout<<out.str()<<'\n';return r.linked?0:1;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 3;}
}
