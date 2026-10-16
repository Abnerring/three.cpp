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
JSValue nativeCache() {
    static thread_local JSWeakValue cached;
    if(const auto value=cached.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;
    JSModuleInitializationScope scope(initializing,"Cache");
    auto object=JSValue::object();
    const auto define=[&](const char* name,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(name,d);};
    define("enabled",false);define("files",JSValue::object());
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"add",2},{"get",1},{"remove",1},{"clear",0}})
        define(name,JSValue::function(method,{name},name,arity));
    object.retainForHeapLifetime();cached=JSWeakValue(object);return object;
}
namespace {
void validate(const JSValue& value){static_cast<void>(JSWeakValue(value));}
void requireRealm(){if(!JSValue::hasActiveHeap())throw JSTypeError("Cache mutation requires a live JSValue realm or module handle");}
}
void CacheStore::add(const JSValue& key,const JSValue& file){validate(key);validate(file);requireRealm();const auto cache=object();cache.get("add").call(cache,{key,file});}
JSValue CacheStore::get(const JSValue& key) const{validate(key);const auto cache=object();return cache.get("get").call(cache,{key});}
void CacheStore::remove(const JSValue& key){validate(key);requireRealm();const auto cache=object();cache.get("remove").call(cache,{key});}
void CacheStore::clear(){requireRealm();const auto cache=object();cache.get("clear").call(cache);}
constinit CacheStore Cache;
