#include "XYZLoader.h"
#include "core/BufferAttribute.h"
#include "loaders/FileLoader.h"
#include "math/Color.h"
#include <cctype>
#include <cmath>
#include <limits>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
// JS String.prototype.trim removes WhiteSpace and LineTerminator; this covers the ASCII members
// plus the two non-ASCII ones a text file realistically carries.
bool isTrimChar(unsigned char ch) {
    return ch==' '||ch=='\t'||ch=='\n'||ch=='\r'||ch=='\v'||ch=='\f'||ch==0x00a0||ch==0xfeff;
}
std::string trim(const std::string& text) {
    std::size_t first=0,last=text.size();
    while(first<last&&isTrimChar(static_cast<unsigned char>(text[first])))first+=1;
    while(last>first&&isTrimChar(static_cast<unsigned char>(text[last-1])))last-=1;
    return text.substr(first,last-first);
}
// line.split( /\s+/ ): an empty line yields no fields, which is then ignored because it is neither
// three nor six fields long.
std::vector<std::string> splitWhitespace(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t index=0;
    while(index<line.size()) {
        while(index<line.size()&&std::isspace(static_cast<unsigned char>(line[index])))index+=1;
        if(index>=line.size())break;
        const std::size_t start=index;
        while(index<line.size()&&!std::isspace(static_cast<unsigned char>(line[index])))index+=1;
        fields.push_back(line.substr(start,index-start));
    }
    return fields;
}
// JS parseFloat: the longest prefix that forms a decimal literal, NaN when there is none.
// Infinity is accepted, hex prefixes are not, and a second decimal point ends the number.
double parseFloatJS(const std::string& text) {
    std::size_t index=0;
    while(index<text.size()&&std::isspace(static_cast<unsigned char>(text[index])))index+=1;
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
}
std::shared_ptr<BufferGeometry> parseXYZText(const std::string& text) {
    std::vector<double> vertices,colors;
    Color color;
    std::size_t index=0;
    while(index<=text.size()) {
        const std::size_t newline=text.find('\n',index);
        const std::string raw=newline==std::string::npos?text.substr(index):text.substr(index,newline-index);
        index=(newline==std::string::npos)?text.size()+1:newline+1;
        const std::string line=trim(raw);
        if(!line.empty()&&line[0]=='#')continue;
        const auto fields=splitWhitespace(line);
        if(fields.size()==3||fields.size()==6) {
            vertices.push_back(parseFloatJS(fields[0]));
            vertices.push_back(parseFloatJS(fields[1]));
            vertices.push_back(parseFloatJS(fields[2]));
        }
        if(fields.size()==6) {
            const double r=parseFloatJS(fields[3])/255;
            const double g=parseFloatJS(fields[4])/255;
            const double b=parseFloatJS(fields[5])/255;
            color.setRGB(r,g,b,"srgb");
            colors.push_back(color.r);
            colors.push_back(color.g);
            colors.push_back(color.b);
        }
    }
    auto geometry=std::make_shared<BufferGeometry>();
    const std::vector<float> positions(vertices.begin(),vertices.end());
    geometry->setAttribute("position",std::make_shared<Float32BufferAttribute>(positions,3));
    if(!colors.empty()) {
        const std::vector<float> converted(colors.begin(),colors.end());
        geometry->setAttribute("color",std::make_shared<Float32BufferAttribute>(converted,3));
    }
    return geometry;
}
std::shared_ptr<BufferGeometry> XYZLoader::parseText(const std::string& text){return parseXYZText(text);}
JSValue XYZLoader::parse(JSValue text){return nativeGeometry(parseXYZText(text.toString()));}
XYZLoader::XYZLoader(JSValue manager):Loader(JSValue::object(prototype()),Existing{}){initialize(object_,manager);}
JSValue XYZLoader::prototype() {
    static thread_local auto value=[] {
        auto constructor=JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{
            initialize(self,arg(args,0));
            return {};
        },{},"XYZLoader",1);
        constructor.setPrototype(Loader::prototype().get("constructor"));
        auto result=constructor.get("prototype");
        result.setPrototype(Loader::prototype());
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"load",4},{"parse",1}}) {
            JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.writable=true;
            descriptor.value=JSValue::function([](const JSValue& receiver,const Args& args,const Args& captures)->JSValue{
                XYZLoader loader=XYZLoader::fromObject(receiver);
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
// The completion callback has to be a plain function because JSValue::NativeFunction is not a
// std::function. The native transport answers synchronously, so the callbacks for the load in
// flight are parked here for the duration of that call.
struct Pending {
    JSValue onLoad{},onError{},manager{},url{};
};
Pending& pending(){static thread_local Pending value;return value;}
JSValue loadedCallback(const JSValue&,const Args& args,const Args&){
    auto& slot=pending();
    try {
        const auto geometry=nativeGeometry(parseXYZText(args.empty()?std::string{}:args[0].toString()));
        if(!slot.onLoad.isUndefined())slot.onLoad.call(JSValue{}, {geometry});
    } catch(const std::exception& error) {
        // Upstream logs to the console when no onError is supplied; there is no console natively,
        // which is the same approximation FileLoader records for its status 0 warning.
        if(!slot.onError.isUndefined())slot.onError.call(JSValue{}, {JSValue(error.what())});
        if(!slot.manager.isUndefined())slot.manager.get("itemError").call(slot.manager, {slot.url});
    }
    return {};
}
}
JSValue XYZLoader::load(JSValue url,JSValue onLoad,JSValue onProgress,JSValue onError){
    auto& slot=pending();
    slot.onLoad=onLoad;slot.onError=onError;slot.url=url;
    slot.manager=object_.get("manager");
    FileLoader fileLoader(slot.manager);
    const auto call=[&](const char* name,const JSValue& value){const auto method=fileLoader.object().get(name);if(!method.isUndefined())method.call(fileLoader.object(),{value});};
    call("setPath",object_.get("path"));
    call("setRequestHeader",object_.get("requestHeader"));
    call("setWithCredentials",object_.get("withCredentials"));
    fileLoader.setResponseType("text");
    const auto result=fileLoader.load(url,JSValue::function(loadedCallback,{}, "onLoad", 1),onProgress,onError);
    slot= Pending{};
    return result;
}