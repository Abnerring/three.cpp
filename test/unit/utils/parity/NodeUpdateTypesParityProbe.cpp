#include "animation/AnimationUtils.h"
#include "nodes/core/Node.h"
#include "nodes/core/constants.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
void own(JSValue o,const std::string& key,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(key,d);}
JSValue call(const JSValue& o,const char* key,const Args& a={}){return o.get(key).call(o,a);}
JSValue array(const Args& values={}){auto o=JSValue::array();for(const auto& v:values)call(o,"push",{v});return o;}
JSValue normal(const JSValue& v,Args* seen=nullptr){if(v.isUndefined())return "#undefined";if(v.isSymbol())return "#symbol";if(v.isCallable())return "#function";if(v.type()==JSValue::Type::Number){if(std::isnan(v.number()))return "#NaN";if(std::isinf(v.number()))return v.number()>0?"#Infinity":"#-Infinity";if(v.number()==0&&std::signbit(v.number()))return "#-0";}if(!v.isObject())return v;Args local;if(!seen)seen=&local;for(const auto& x:*seen)if(v.sameValue(x))return "#repeat";seen->push_back(v);auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();for(const auto& key:v.keys())own(out,key,normal(v.get(key),seen));seen->pop_back();return out;}
JSValue decode(const JSValue& v){if(!v.isObject())return v;const auto tag=v.get("tag");if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("symbol"))return JSValue::symbol("enum");if(tag.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();if(tag.strictEquals("minusZero"))return -0.;if(tag.strictEquals("function"))return JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"enum",0);return v;}
JSValue failure(std::exception_ptr p){try{std::rethrow_exception(p);}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception& e){return std::string("#NativeError:")+e.what();}}
void event(const JSValue& s,const Args& row){call(s.get("events"),"push",{array(row)});}
JSValue state(const JSValue& r){auto s=JSValue::object();s.set("request",r);s.set("events",array());s.set("reads",0);s.set("partial",JSValue{});return s;}
JSValue selected(const JSValue& r,unsigned read){const auto vs=r.get("values");return vs.isUndefined()?decode(r.get("value")):decode(vs.get(std::to_string((read-1)%vs.length())));}
JSValue constructorCase(const JSValue& r){
    auto s=state(r),o=JSValue::object(),types=nativeNodeConstantsBinding("NodeUpdateType");const auto ctor=nativeNodeConstructor();const auto saved=types.getOwnPropertyDescriptor("NONE");
    auto target=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"CaptureTarget",0);auto proto=target.get("prototype");proto.setPrototype(ctor.get("prototype"));
    for(const auto* key:{"nodeType","updateType","updateBeforeType","updateAfterType"}){JSPropertyDescriptor d;d.configurable=true;d.setter=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{auto s=c[0];const auto key=c[1];if(key.strictEquals("nodeType"))s.set("partial",self);event(s,{"set:"+key.string(),normal(arg(a,0)),self.sameValue(s.get("partial"))});if(s.get("request").get("throwSet").strictEquals(key))throw JSTypeError("setter");own(self,key.string(),arg(a,0));return {};},{s,key},std::string("set ")+key,1);proto.defineProperty(key,d);}
    JSPropertyDescriptor getter;getter.configurable=getter.enumerable=true;getter.getter=JSValue::function(+[](const JSValue& receiver,const Args&,const Args& c)->JSValue{
        auto s=c[0],types=c[1];const auto r=s.get("request");const auto count=static_cast<unsigned>(s.get("reads").toNumber()+1);s.set("reads",count);event(s,{"NONE",count,receiver.sameValue(types),normal(s.get("partial"))});
        if(r.get("throwRead").strictEquals(count))throw JSTypeError("NONE");
        if(r.get("mode").strictEquals("rewrite")&&count==1)own(types,"NONE",decode(r.get("replacement")));
        if(r.get("mode").strictEquals("reenter")&&count==1)s.set("inner",c[2].construct({"inner"}));
        return selected(r,count);
    },{s,types,ctor},"get NONE",0);types.defineProperty("NONE",getter);
    try{o.set("result",normal(ctor.construct({decode(r.get("nodeType"))},target)));}catch(...){o.set("error",failure(std::current_exception()));}
    if(saved)types.defineProperty("NONE",*saved);o.set("partial",normal(s.get("partial")));o.set("inner",normal(s.get("inner")));o.set("reads",s.get("reads"));o.set("events",s.get("events"));o.set("nextId",ctor.construct({"after"}).get("id"));return o;
}
JSValue handler(const JSValue& s,const char* name){return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{const auto s=c[0],r=s.get("request");event(s,{c[1],self.sameValue(s.get("node")),a.size(),arg(a,0).sameValue(s.get("callback")),normal(arg(a,1))});if(r.get("mode").strictEquals("callThrow"))throw JSTypeError("onUpdate");return s.get("returnValue");},{s,name},name,2);}
JSValue methodCase(const JSValue& r){
    auto s=state(r),o=JSValue::object(),types=nativeNodeConstantsBinding("NodeUpdateType");const auto ctor=nativeNodeConstructor();auto node=ctor.construct({"subject"});s.set("node",node);const auto key=r.get("key"),method=r.get("method");const auto saved=types.getOwnPropertyDescriptor(key);auto ret=JSValue::object();ret.set("tag","methodResult");s.set("returnValue",ret);
    auto callback=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{event(c[0],{"callback",self.sameValue(c[0].get("node")),a.size(),arg(a,0)});return "callbackResult";},{s},"callback",1);if(r.has("callback"))callback=decode(r.get("callback"));s.set("callback",callback);s.set("first",handler(s,"first"));s.set("second",handler(s,"second"));
    if(callback.isCallable()&&r.get("mode").strictEquals("bindThrow")){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get:callback.bind"});throw JSTypeError("bind");},{s});callback.defineProperty("bind",d);}
    const auto original=ctor.get("prototype").get("onUpdate");
    JSPropertyDescriptor update;update.configurable=true;update.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        const auto s=c[0],r=s.get("request"),mode=r.get("mode");event(s,{"get:onUpdate"});if(mode.strictEquals("calleeThrow"))throw JSTypeError("callee");
        if(mode.strictEquals("calleeMutatesEnum"))own(c[1],r.get("key").string(),decode(r.get("replacement")));
        if(mode.strictEquals("notCallable"))return 17;if(mode.strictEquals("missing"))return {};
        return mode.strictEquals("actual")||mode.strictEquals("bindThrow")?c[2]:s.get("first");
    },{s,types,original},"get onUpdate",0);node.defineProperty("onUpdate",update);
    JSPropertyDescriptor type;type.configurable=type.enumerable=true;type.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
        auto s=c[0],types=c[1],node=s.get("node");const auto r=s.get("request"),key=r.get("key"),mode=r.get("mode");const auto read=static_cast<unsigned>(s.get("reads").toNumber()+1);s.set("reads",read);event(s,{"enum:"+key.string(),read,self.sameValue(types)});
        if(mode.strictEquals("enumThrow"))throw JSTypeError("enum");
        if(mode.strictEquals("replaceDuringEnum"))own(node,"onUpdate",s.get("second"));
        if(mode.strictEquals("rewrite")&&read==1)own(types,key.string(),decode(r.get("replacement")));
        if(mode.strictEquals("reenter")&&read==1)s.set("innerResult",c[2].call(node,{s.get("callback")}));
        return selected(r,read);
    },{s,types,ctor.get("prototype").get(method)},"get update type",0);types.defineProperty(key,type);
    try{auto results=array();for(unsigned i=0;i<(r.get("repeat").toBoolean()?2:1);++i)call(results,"push",{normal(ctor.get("prototype").get(method).call(node,{callback}))});o.set("results",results);if(r.get("mode").strictEquals("actual"))o.set("callbackResult",normal(call(node,"update",{"frame"})));}catch(...){o.set("error",failure(std::current_exception()));}
    if(saved)types.defineProperty(key,*saved);o.set("innerResult",normal(s.get("innerResult")));o.set("updateType",normal(node.get("updateType")));o.set("hasOwnUpdate",node.hasOwn("update"));o.set("update",normal(node.get("update")));o.set("reads",s.get("reads"));o.set("events",s.get("events"));return o;
}
JSValue freshFacade(bool preload){
    auto facade=nativeNodeConstants(),types=nativeNodeConstantsBinding("NodeUpdateType");JSValue ctor;if(preload)ctor=nativeNodeConstructor();auto o=JSValue::object(),events=array(),poison=JSValue::object();
    for(const auto* key:{"NONE","FRAME","RENDER","OBJECT"}){JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"push",{c[1]});throw JSTypeError("facade was imported");},{events,key});poison.defineProperty(key,d);}
    facade.set("NodeUpdateType",poison);own(types,"NONE","original-none");own(types,"FRAME",17);own(types,"RENDER",nullptr);auto objectType=JSValue::object();objectType.set("tag","object-update");own(types,"OBJECT",objectType);if(!preload)ctor=nativeNodeConstructor();auto n=ctor.construct({"subject"});o.set("initial",normal(n));auto results=array();const auto callback=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{call(c[0],"push",{self.sameValue(c[1])});return "callbackResult";},{events,n},"callback",0);
    for(const auto* name:{"onFrameUpdate","onRenderUpdate","onObjectUpdate"}){call(n,name,{callback});call(results,"push",{array({normal(n.get("updateType")),call(n,"update")})});}
    o.set("updates",results);o.set("poisonAccessesAndReceivers",events);o.set("privateBindingPreserved",nativeNodeConstantsBinding("NodeUpdateType").sameValue(types));o.set("facadeSlotReplaced",facade.get("NodeUpdateType").sameValue(poison));return o;
}
}
int main(int argc,char** argv){try{if(argc==3&&std::string_view(argv[1])=="--fresh-facade"){std::cout<<freshFacade(std::string_view(argv[2])=="post").toJSON()->dump();return 0;}const std::string input((std::istreambuf_iterator<char>(std::cin)),{});const auto requests=Animation_detail::parseJSON(input);nativeNodeConstructor();auto out=array();for(unsigned i=0;i<requests.length();++i){const auto r=requests.get(std::to_string(i));call(out,"push",{r.get("kind").strictEquals("constructor")?constructorCase(r):methodCase(r)});}std::cout<<out.toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
