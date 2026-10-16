// Differential probe for the native examples/jsm/loaders/STLLoader.js binary path.
//
// Each stdin line is one base64 encoded STL document, so the same bytes reach both sides without a
// string round trip that would mangle the NUL bytes a binary STL carries. Output is the geometry
// shape plus the raw float32 bit patterns of the arrays: comparing bits keeps single precision
// unambiguous.
#include "core/JSValue.h"
#include "core/JsonValue.h"
#include "core/NativeObject.h"
#include "loaders/STLLoader.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
namespace {
const char* kAlphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
std::vector<std::uint8_t> decode(const std::string& text) {
    std::vector<std::uint8_t> bytes;
    int buffer=0,bits=0;
    for(char ch:text) {
        if(ch=='=')break;
        const auto index=std::string(kAlphabet).find(ch);
        if(index==std::string::npos)continue;
        buffer=(buffer<<6)|int(index);bits+=6;
        if(bits>=8){bits-=8;bytes.push_back(std::uint8_t((buffer>>bits)&0xff));}
    }
    return bytes;
}
JsonValue patterns(const JSValue& geometry,const char* name) {
    const auto attribute=geometry.get("attributes").get(name);
    if(attribute.isUndefined()||attribute.isNull())return "absent";
    const auto array=attribute.get("array");
    const double length=array.get("length").toNumber();
    std::string buffer;
    for(double index=0;index<length;index+=1) {
        const float single=static_cast<float>(array.get(index).toNumber());
        std::uint32_t bits=0;
        std::memcpy(&bits,&single,sizeof(bits));
        char text[16];
        std::snprintf(text,sizeof(text),"%08x ",bits);
        buffer+=text;
    }
    while(!buffer.empty()&&buffer.back()==' ')buffer.pop_back();
    return buffer;
}
}
int main() {
    const auto realm=JSValue::object();
    static_cast<void>(realm);
    std::string line;
    while(std::getline(std::cin,line)) {
        try {
            STLLoader loader;
            const auto geometry=nativeGeometry(loader.parseBytes(decode(line)));
            const auto alpha=geometry.get("alpha");
            std::cout << JsonValue(JsonValue::Object{
                             {"hasColors",geometry.get("hasColors").toBoolean()},
                             {"alpha",alpha.isUndefined()?-1.:alpha.toNumber()},
                             {"position",patterns(geometry,"position")},
                             {"normal",patterns(geometry,"normal")},
                             {"color",patterns(geometry,"color")}})
                             .dump()
                      << '\n';
        } catch(const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError",error.what()}}).dump() << '\n';
        }
    }
    return 0;
}