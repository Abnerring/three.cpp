#include "loaders/STLLoader.h"
#include "core/BufferAttribute.h"
#include "loaders/FileLoader.h"
#include "math/Color.h"
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
bool isSpaceChar(char ch){return ch==' '||ch=='\t'||ch=='\n'||ch=='\r'||ch=='\v'||ch=='\f';}
// JS parseFloat: the longest prefix that forms a decimal literal, NaN when there is none.
double parseFloatJS(const std::string& text) {
    std::size_t index=0;
    while(index<text.size()&&isSpaceChar(text[index]))index+=1;
    const std::size_t start=index;
    const bool negative=index<text.size()&&text[index]=='-';
    if(index<text.size()&&(text[index]=='+'||text[index]=='-'))index+=1;
    if(text.compare(index,8,"Infinity")==0)return negative?-HUGE_VAL:HUGE_VAL;
    bool digits=false,point=false,exponent=false;
    std::size_t exponentMark=std::string::npos;
    std::size_t cursor=index;
    while(cursor<text.size()) {
        const char ch=text[cursor];
        if(ch>='0'&&ch<='9'){digits=true;cursor+=1;continue;}
        if(ch=='.'&&!point&&!exponent){point=true;cursor+=1;continue;}
        if((ch=='e'||ch=='E')&&!exponent&&digits){
            exponent=true;exponentMark=cursor;cursor+=1;
            if(cursor<text.size()&&(text[cursor]=='+'||text[cursor]=='-'))cursor+=1;
            continue;
        }
        break;
    }
    std::size_t end=cursor;
    if(exponent) {
        std::size_t probe=exponentMark+1;
        if(probe<text.size()&&(text[probe]=='+'||text[probe]=='-'))probe+=1;
        if(probe>=text.size()||text[probe]<'0'||text[probe]>'9')end=exponentMark;
    }
    if(!digits)return std::numeric_limits<double>::quiet_NaN();
    try { return std::stod(text.substr(start,end-start)); }
    catch(...) { return std::numeric_limits<double>::quiet_NaN(); }
}
// The pinned ASCII path drives four fixed regular expressions, matched by hand rather than through a
// general engine because what matters is the behaviour of these patterns: a lazy capture between two
// literals, a line bounded name, and a float capture whose every part is optional, so a vertex line
// with nothing after the keyword still matches with an empty capture and parseFloat gives NaN.
std::pair<std::string,std::size_t> matchFloatAt(const std::string& text,std::size_t pos) {
    const std::size_t start=pos;
    std::size_t cursor=pos;
    if(cursor<text.size()&&(text[cursor]=='+'||text[cursor]=='-'))cursor+=1;
    while(cursor<text.size()&&text[cursor]>='0'&&text[cursor]<='9')cursor+=1;
    if(cursor<text.size()&&text[cursor]=='.') {
        cursor+=1;
        while(cursor<text.size()&&text[cursor]>='0'&&text[cursor]<='9')cursor+=1;
    }
    if(cursor<text.size()&&(text[cursor]=='e'||text[cursor]=='E')) {
        std::size_t probe=cursor+1;
        if(probe<text.size()&&(text[probe]=='+'||text[probe]=='-'))probe+=1;
        if(probe<text.size()&&text[probe]>='0'&&text[probe]<='9') {
            probe+=1;
            while(probe<text.size()&&text[probe]>='0'&&text[probe]<='9')probe+=1;
            cursor=probe;
        }
    }
    return {text.substr(start,cursor-start),cursor};
}
bool matchTriple(const std::string& text,const std::string& keyword,std::size_t from,double out[3],std::size_t& next) {
    const auto found=text.find(keyword,from);
    if(found==std::string::npos)return false;
    std::size_t cursor=found+keyword.size();
    double values[3]={0,0,0};
    for(int index=0;index<3;index+=1) {
        if(cursor>=text.size()||!isSpaceChar(text[cursor]))return false;
        while(cursor<text.size()&&isSpaceChar(text[cursor]))cursor+=1;
        const auto [literal,nextCursor]=matchFloatAt(text,cursor);
        values[index]=parseFloatJS(literal);
        cursor=nextCursor;
    }
    out[0]=values[0];out[1]=values[1];out[2]=values[2];next=cursor;
    return true;
}
bool matchSolidName(const std::string& solid,std::string& name) {
    const auto found=solid.find("solid");
    if(found==std::string::npos)return false;
    std::size_t cursor=found+5;
    if(cursor>=solid.size()||!isSpaceChar(solid[cursor]))return false;
    cursor+=1;
    const auto end=solid.find('\n',cursor);
    const std::string rest=solid.substr(cursor,end==std::string::npos?std::string::npos:end-cursor);
    if(rest.empty())return false;
    name=rest;
    return true;
}
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
// The ASCII half. It is pure: no wrapping, no JSValue, and the group names leave through the
// parameter so that the single wrap in parse can write them onto the geometry.
std::shared_ptr<BufferGeometry> parseASCIIText(const std::string& data,std::vector<std::string>& groupNames) {
    auto geometry=std::make_shared<BufferGeometry>();
    std::vector<double> vertices,normals;
    std::vector<double> groupStarts,groupCounts;
    double normal[3]={0,0,0};
    std::size_t endVertex=0;
    std::size_t searchFrom=0;
    while(true) {
        const auto solidStart=data.find("solid",searchFrom);
        if(solidStart==std::string::npos)break;
        const auto solidEnd=data.find("endsolid",solidStart+5);
        if(solidEnd==std::string::npos)break;
        const std::string solid=data.substr(solidStart,solidEnd-solidStart);
        searchFrom=solidEnd+8;
        const double startVertex=static_cast<double>(endVertex);
        std::string name;
        groupNames.push_back(matchSolidName(solid,name)?name:std::string{});
        std::size_t faceFrom=0;
        while(true) {
            const auto facetStart=solid.find("facet",faceFrom);
            if(facetStart==std::string::npos)break;
            const auto facetEnd=solid.find("endfacet",facetStart+5);
            if(facetEnd==std::string::npos)break;
            const std::string text=solid.substr(facetStart,facetEnd-facetStart);
            faceFrom=facetEnd+8;
            std::size_t cursor=0,next=0;
            double triple[3]={0,0,0};
            while(matchTriple(text,"normal",cursor,triple,next)) {
                normal[0]=triple[0];normal[1]=triple[1];normal[2]=triple[2];
                cursor=next;
            }
            cursor=0;
            while(matchTriple(text,"vertex",cursor,triple,next)) {
                vertices.push_back(triple[0]);vertices.push_back(triple[1]);vertices.push_back(triple[2]);
                normals.push_back(normal[0]);normals.push_back(normal[1]);normals.push_back(normal[2]);
                endVertex+=1;
                cursor=next;
            }
            // Upstream logs when a facet does not carry exactly one normal and three vertices; there
            // is no console natively, so those two diagnostics are recorded rather than reproduced.
        }
        groupStarts.push_back(startVertex);
        groupCounts.push_back(static_cast<double>(endVertex)-startVertex);
    }
    const std::vector<float> positions(vertices.begin(),vertices.end()),converted(normals.begin(),normals.end());
    geometry->setAttribute("position",std::make_shared<Float32BufferAttribute>(positions,3));
    geometry->setAttribute("normal",std::make_shared<Float32BufferAttribute>(converted,3));
    for(std::size_t index=0;index<groupStarts.size();index+=1)
        geometry->addGroup(groupStarts[index],groupCounts[index],static_cast<double>(index));
    return geometry;
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
std::shared_ptr<BufferGeometry> STLLoader::parseBytes(const std::vector<std::uint8_t>& data,bool* hasColorsOut,double* alphaOut,std::vector<std::string>* groupNamesOut) {
    if(!isBinary(data)) {
        std::string text;
        text.reserve(data.size());
        for(std::uint8_t byte:data)text.push_back(char(byte));
        std::vector<std::string> names;
        const auto ascii=parseASCIIText(text,names);
        if(groupNamesOut)*groupNamesOut=names;
        return ascii;
    }
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
    if(hasColors) {
        // Upstream marks the geometry itself rather than the caller's copy, so the flag travels
        // with the geometry: it is set here, where the pinned parseBinary sets it, not in load.
        auto object=nativeGeometry(geometry);
        object.set("hasColors",true);
        object.set("alpha",alpha);
    }
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
    std::vector<std::string> groupNames;
    const auto geometry=parseBytes(bytes,&hasColors,&alpha,&groupNames);
    // The one and only wrap. Everything the pinned source writes onto the geometry is written here.
    auto result=nativeGeometry(geometry);
    if(hasColors) {
        result.set("hasColors",true);
        result.set("alpha",alpha);
    }
    if(!groupNames.empty()) {
        auto names=JSValue::array();
        // Entries go in through push: set with an integer index would select the name overload and
        // read the integer as a null pointer.
        for(std::size_t index=0;index<groupNames.size();index+=1)
            names.get("push").call(names, {JSValue(groupNames[index])});
        auto userData=result.get("userData");
        if(userData.isUndefined()||userData.isNull()) {
            result.set("userData",JSValue::object());
            userData=result.get("userData");
        }
        userData.set("groupNames",names);
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
namespace {
// As in XYZLoader the completion callback has to be a plain function, so the callbacks for the
// load in flight wait in a thread local slot; the injected transport answers synchronously.
struct Pending {
    JSValue onLoad{},onError{},manager{},url{};
};
Pending& pending(){static thread_local Pending value;return value;}
JSValue loadedCallback(const JSValue&,const Args& args,const Args&){
    auto& slot=pending();
    try {

        const auto bytes=args.empty()?std::string{}:args[0].toString();
        std::vector<std::uint8_t> data;
        data.reserve(bytes.size());
        for(char ch:bytes)data.push_back(std::uint8_t(std::uint8_t(ch)&0xff));
        STLLoader parser;
        const auto geometry=nativeGeometry(parser.parseBytes(data));
        if(!slot.onLoad.isUndefined())slot.onLoad.call(JSValue{}, {geometry});
    } catch(const std::exception& error) {
        // Upstream logs when no onError is supplied; there is no console natively.
        if(!slot.onError.isUndefined())slot.onError.call(JSValue{}, {JSValue(error.what())});
        if(!slot.manager.isUndefined())slot.manager.get("itemError").call(slot.manager, {slot.url});
    }
    return {};
}
}
JSValue STLLoader::load(JSValue url,JSValue onLoad,JSValue onProgress,JSValue onError){
    auto& slot=pending();
    slot.onLoad=onLoad;slot.onError=onError;slot.url=url;
    slot.manager=object_.get("manager");
    FileLoader fileLoader(slot.manager);
    const auto call=[&](const char* name,const JSValue& value){const auto method=fileLoader.object().get(name);if(!method.isUndefined())method.call(fileLoader.object(),{value});};
    call("setPath",object_.get("path"));
    call("setRequestHeader",object_.get("requestHeader"));
    call("setWithCredentials",object_.get("withCredentials"));
    const auto result=fileLoader.setResponseType("arraybuffer").load(url,JSValue::function(loadedCallback,{}, "onLoad", 1),onProgress,onError);
    slot= Pending{};
    return result;
}