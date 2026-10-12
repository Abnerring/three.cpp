// Port of three.js LoaderUtils.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "loaders/LoaderUtils.h"
#include "math/Interpolant.h"
#include <regex>

namespace {
bool match(const std::string& value,const char* expression) {
    return std::regex_search(value,std::regex(expression,std::regex::ECMAScript|std::regex::icase));
}
std::size_t lineTerminator(const std::string& value,std::size_t start=0) {
    auto end=value.find_first_of("\r\n",start);end=std::min(end,value.find("\xe2\x80\xa8",start));return std::min(end,value.find("\xe2\x80\xa9",start));
}
}
JSValue LoaderUtils::extractUrlBase(JSValue url) {
    if(url.type()==JSValue::Type::String) {
        const auto index=url.string().rfind('/');return index==std::string::npos?JSValue("./"):JSValue(url.string().substr(0,index+1));
    }
    const auto method=url.get("lastIndexOf"),index=method.call(url,{"/"});
    if(index.strictEquals(-1))return "./";
    const auto slice=url.get("slice");return slice.call(url,{0,Interpolant_detail::add(index,1)});
}
JSValue LoaderUtils::resolveURL(JSValue url,JSValue path) {
    if(url.type()!=JSValue::Type::String||url.string().empty())return "";
    if(match(path.toString(),"^https?://")&&url.string()[0]=='/') {
        if(path.type()==JSValue::Type::String) {
            const auto& value=path.string();const auto scheme=value.find("://");
            const auto end=value.find('/',scheme+3);
            if(end!=scheme+3){const auto line=end==std::string::npos?std::string::npos:lineTerminator(value,end);path=value.substr(0,end)+(line==std::string::npos?"":value.substr(line));}
        } else throw JSTypeError("path.replace is not a function");
    }
    if(match(url.string(),"^(https?:)?//")||(lineTerminator(url.string())==std::string::npos&&(match(url.string(),"^data:.*,.*$")||match(url.string(),"^blob:.*$"))))return url;
    return Interpolant_detail::add(path,url);
}
