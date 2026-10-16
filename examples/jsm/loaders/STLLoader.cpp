#include "loaders/STLLoader.h"
#include "core/BufferAttribute.h"
#include "math/Color.h"
#include <cstring>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
std::uint32_t readUint32(const std::vector<std::uint8_t>& data,std::size_t offset,bool littleEndian) {
    if(offset+4>data.size())return 0;
    if(littleEndian)return std::uint32_t(data[offset])|(std::uint32_t(data[offset+1])<<8)|
        (std::uint32_t(data[offset+2])<<16)|(std::uint32_t(data[offset+3])<<24);
    return (std::uint32_t(data[offset])<<24)|(std::uint32_t(data[offset+1])<<16)|
        (std::uint32_t(data[offset+2])<<8)|std::uint32_t(data[offset+3]);
}
std::uint16_t readUint16(const std::vector<std::uint8_t>& data,std::size_t offset) {
    if(offset+2>data.size())return 0;
    return std::uint16_t(std::uint16_t(data[offset])|(std::uint16_t(data[offset+1])<<8));
}
float readFloat32(const std::vector<std::uint8_t>& data,std::size_t offset) {
    const std::uint32_t bits=readUint32(data,offset,true);
    float value=0;
    std::memcpy(&value,&bits,sizeof(value));
    return value;
}
std::uint8_t byteAt(const std::vector<std::uint8_t>& data,std::size_t offset) {
    return offset<data.size()?data[offset]:0;
}
bool matches(const std::vector<std::uint8_t>& data,const std::uint8_t* query,std::size_t length,std::size_t offset) {
    for(std::size_t index=0;index<length;index+=1) if(query[index]!=byteAt(data,offset+index))return false;
    return true;
}
}
bool STLLoader::isBinary(const std::vector<std::uint8_t>& data) {
    constexpr std::size_t faceSize=(32/8*3)+((32/8*3)*3)+(16/8);
    const std::uint32_t faces=readUint32(data,80,true);
    const std::size_t expected=80+(32/8)+std::size_t(faces)*faceSize;
    if(expected==data.size())return true;
    // An ASCII file has to begin with "solid", but up to five bytes of an encoding prefix may come
    // first, which is why the search walks those five offsets.
    const std::uint8_t solid[5]={115,111,108,105,100};
    for(std::size_t off=0;off<5;off+=1) if(matches(data,solid,5,off))return false;
    return true;
}
std::shared_ptr<BufferGeometry> STLLoader::parseBytes(const std::vector<std::uint8_t>& data,bool* hasColorsOut,double* alphaOut) {
    if(!isBinary(data))throw JSTypeError("STLLoader: ASCII parsing is not implemented natively yet");
    const std::uint32_t faces=readUint32(data,80,true);
    bool hasColors=false;
    double defaultR=0,defaultG=0,defaultB=0,alpha=0;
    // The header search reads a big endian word for "COLO" and then the literal bytes for "R=".
    for(std::size_t index=0;index+10<=80;index+=1) {
        if(readUint32(data,index,false)==0x434F4C4Fu&&byteAt(data,index+4)==0x52&&byteAt(data,index+5)==0x3D) {
            hasColors=true;
            defaultR=double(byteAt(data,index+6))/255;
            defaultG=double(byteAt(data,index+7))/255;
            defaultB=double(byteAt(data,index+8))/255;
            alpha=double(byteAt(data,index+9))/255;
        }
    }
    constexpr std::size_t dataOffset=84,faceLength=12*4+2;
    std::vector<float> vertices(std::size_t(faces)*9,0.f),normals(std::size_t(faces)*9,0.f),colors;
    if(hasColors)colors.assign(std::size_t(faces)*9,0.f);
    Color color;
    for(std::uint32_t face=0;face<faces;face+=1) {
        const std::size_t start=dataOffset+std::size_t(face)*faceLength;
        const float normalX=readFloat32(data,start),normalY=readFloat32(data,start+4),normalZ=readFloat32(data,start+8);
        double r=0,g=0,b=0;
        if(hasColors) {
            const std::uint16_t packed=readUint16(data,start+48);
            if((packed&0x8000)==0) {
                r=double(packed&0x1F)/31;
                g=double((packed>>5)&0x1F)/31;
                b=double((packed>>10)&0x1F)/31;
            } else {
                r=defaultR;g=defaultG;b=defaultB;
            }
        }
        for(std::size_t i=1;i<=3;i+=1) {
            const std::size_t vertexStart=start+i*12;
            const std::size_t component=std::size_t(face)*9+(i-1)*3;
            vertices[component]=readFloat32(data,vertexStart);
            vertices[component+1]=readFloat32(data,vertexStart+4);
            vertices[component+2]=readFloat32(data,vertexStart+8);
            normals[component]=normalX;
            normals[component+1]=normalY;
            normals[component+2]=normalZ;
            if(hasColors) {
                color.setRGB(r,g,b,"srgb");
                colors[component]=color.r;
                colors[component+1]=color.g;
                colors[component+2]=color.b;
            }
        }
    }
    auto geometry=std::make_shared<BufferGeometry>();
    geometry->setAttribute("position",std::make_shared<Float32BufferAttribute>(vertices,3));
    geometry->setAttribute("normal",std::make_shared<Float32BufferAttribute>(normals,3));
    if(hasColors)geometry->setAttribute("color",std::make_shared<Float32BufferAttribute>(colors,3));
    if(hasColorsOut)*hasColorsOut=hasColors;
    if(alphaOut)*alphaOut=alpha;
    return geometry;
}
JSValue STLLoader::parse(JSValue data) {
    const auto text=data.toString();
    // ensureBinary: a string is read one code unit at a time and masked to a byte.
    std::vector<std::uint8_t> bytes;
    bytes.reserve(text.size());
    for(char ch:text) bytes.push_back(std::uint8_t(std::uint8_t(ch)&0xff));
    bool hasColors=false;
    double alpha=0;
    const auto geometry=parseBytes(bytes,&hasColors,&alpha);
    auto result=nativeGeometry(geometry);
    if(hasColors) {
        // Upstream marks the geometry and carries the header alpha onto it alongside the colour
        // attribute, which is what a consumer checks before using per face colours.
        result.set("hasColors",true);
        result.set("alpha",alpha);
    }
    return result;
}
STLLoader::STLLoader(JSValue manager):Loader(JSValue::object(prototype()),Existing{}){initialize(object_,manager);}
JSValue STLLoader::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{
            initialize(self,arg(args,0));
            return {};
        },{},"STLLoader",1);
        constructor.setPrototype(Loader::prototype().get("constructor"));
        auto result=constructor.get("prototype");
        result.setPrototype(Loader::prototype());
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"load",4},{"parse",1}}) {
            JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.writable=true;
            descriptor.value=JSValue::function([](const JSValue& receiver,const Args& args,const Args& captures)->JSValue{
                STLLoader loader=STLLoader::fromObject(receiver);
                if(captures[0].string()=="parse")return loader.parse(arg(args,0));
                return loader.load(arg(args,0),arg(args,1),arg(args,2),arg(args,3));
            },{name},name,arity);
            result.defineProperty(name,descriptor);
        }
        return result;
    }();
    return value;
}
JSValue STLLoader::load(JSValue url,JSValue onLoad,JSValue onProgress,JSValue onError){
    throw JSTypeError("STLLoader.load is not implemented natively yet");
}