// Port of three.js Cache.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "loaders/Cache.h"
#include "math/Interpolant.h"
#include <algorithm>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
std::string key(const JSValue& value){return value.toPrimitive(JSValue::PreferredType::String).toString();}
bool isBlobURL(JSValue value) {
    try {
        std::string url;
        if(value.type()==JSValue::Type::String) {
            const auto& text=value.string();const auto colon=text.find(':');url=text.substr(colon==std::string::npos?0:colon+1);
        } else {
            const auto slice=value.get("slice"),indexOf=value.get("indexOf");
            const auto index=indexOf.call(value,{":"});url=slice.call(value,{Interpolant_detail::add(index,1)}).toString();
        }
        // URL parsing strips C0/space at the ends and ASCII tab/newlines anywhere.
        const auto first=url.find_first_not_of(std::string("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f ")+char(0));
        if(first==std::string::npos)return false;
        url.erase(0,first);while(!url.empty()&&static_cast<unsigned char>(url.back())<=0x20)url.pop_back();
        std::erase_if(url,[](char c){return c=='\t'||c=='\n'||c=='\r';});
        if(url.size()<5)return false;
        return (url[0]=='b'||url[0]=='B')&&(url[1]=='l'||url[1]=='L')&&(url[2]=='o'||url[2]=='O')&&(url[3]=='b'||url[3]=='B')&&url[4]==':';
    } catch(...) {return false;}
}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures) {
    auto self=receiver;const auto& name=captures[0].string();const auto argument=arg(args,0);
    if(name=="clear"){self.set("files",JSValue::object());return {};}
    if(name=="remove"){auto files=self.get("files");files.erase(key(argument));return {};}
    if(self.get("enabled").strictEquals(false)||isBlobURL(argument))return {};
    auto files=self.get("files");const auto property=key(argument);
    if(name=="add"){files.set(property,arg(args,1));return {};}
    return files.get(property);
}
}
CacheStore::CacheStore():object_(JSValue::object()) {
    object_.set("enabled",false);object_.set("files",JSValue::object());
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"add",2},{"get",1},{"remove",1},{"clear",0}})
        object_.set(name,JSValue::function(method,{name},name,arity));
}
void CacheStore::add(JSValue key,JSValue file){object_.get("add").call(object_,{key,file});}
JSValue CacheStore::get(JSValue key) const{return object_.get("get").call(object_,{key});}
void CacheStore::remove(JSValue key){object_.get("remove").call(object_,{key});}
void CacheStore::clear(){object_.get("clear").call(object_);}
thread_local CacheStore Cache;
