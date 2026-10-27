#include "animation/AnimationUtils.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/constants.h"
#include <iostream>

namespace {
using Args=std::vector<JSValue>;
void own(JSValue o,const std::string& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;o.defineProperty(key,d);}
JSValue array(const Args& values={}){auto a=JSValue::array();for(const auto& v:values)own(a,std::to_string(a.length()),v);return a;}
JSValue normal(const JSValue& v){if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";if(!v.isObject())return v;auto o=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();for(const auto& key:v.keys())own(o,key,normal(v.get(key)));return o;}
JSValue error(){try{throw;}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return "#Error";}}
JSValue decode(const JSValue& v){if(v.isObject()&&v.get("tag").strictEquals("undefined"))return {};return v;}
JSValue run(const JSValue& r){auto out=JSValue::object(),events=array();auto types=nativeNodeConstantsBinding("NodeUpdateType");
    if(r.get("observed").toBoolean())for(const auto* key:{"FRAME","RENDER","OBJECT"}){JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{auto events=c[0];own(events,std::to_string(events.length()),c[1]);if(c[1].sameValue(c[2]))throw JSTypeError("enum getter");return "observed-"+c[1].string();},{events,key,r.get("throwAt")},"get enum",0);d.enumerable=d.configurable=true;types.defineProperty(key,d);}
    try{
        Args args;const auto values=r.get("args");for(unsigned i=0;i<values.length();++i)args.push_back(decode(values.get(std::to_string(i))));JSValue node;
        const auto entry=r.get("entry");if(entry.strictEquals("wrapper")){const auto get=[&](unsigned i,const JSValue& fallback){return i<args.size()?args[i]:fallback;};node=UniformGroupNode(get(0,{}),get(1,false),get(2,1),get(3,nullptr)).object();}
        else if(entry.strictEquals("binding"))node=nativeUniformGroupNodeBinding("default").construct(args);
        else node=nativeUniformGroupNodeBinding(entry.string()).call({},args);
        auto module=nativeUniformGroupNodeModule();auto keys=array();for(const auto& key:module.keys())own(keys,std::to_string(keys.length()),key);out.set("keys",keys);
        const auto ctor=nativeUniformGroupNodeBinding("default");auto metadata=JSValue::object();metadata.set("name",ctor.get("name"));metadata.set("length",ctor.get("length"));metadata.set("constructible",ctor.isConstructible());metadata.set("defaultIdentity",module.get("default").sameValue(ctor));out.set("constructor",metadata);
        auto groups=JSValue::object();for(const auto* key:{"frameGroup","renderGroup","objectGroup"})groups.set(key,normal(nativeUniformGroupNodeBinding(key)));out.set("groups",groups);out.set("node",normal(node));
        if(r.get("enumAfter").toBoolean()){for(const auto* key:{"FRAME","RENDER","OBJECT"})own(types,key,"changed");out.set("snapshot",array({nativeFrameGroup().get("updateType"),nativeRenderGroup().get("updateType"),nativeObjectGroup().get("updateType")}));}
        if(r.get("isolate").toBoolean()){const auto frame=nativeFrameGroup();module.set("default",nullptr);module.set("uniformGroup",nullptr);module.set("frameGroup",nullptr);out.set("privateIdentity",nativeUniformGroupNodeBinding("default").sameValue(ctor)&&nativeFrameGroup().sameValue(frame));out.set("afterFacade",normal(UniformGroupNode("afterFacade").object()));}
        if(r.get("poisonConstructor").toBoolean()){auto p=ctor.get("prototype");p.set("constructor",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSTypeError("mutable prototype constructor");}));out.set("afterPrototype",normal(UniformGroupNode("afterPrototype").object()));}
    }catch(...){out.set("error",error());}out.set("events",events);return out;
}
}
int main(){try{const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto r=Animation_detail::parseJSON(input);const auto out=run(r);std::cout<<out.toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
