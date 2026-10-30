#include "animation/AnimationUtils.h"
#include "core/NativeArray.h"
#include "core/JSValue.h"
#include <cmath>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue rec(std::initializer_list<std::pair<const char*,JSValue>> fields){auto o=JSValue::object();for(const auto& [k,v]:fields)own(o,k,v);return o;}
JSValue arr(const Args& a={}){auto o=JSValue::array();for(std::size_t i=0;i<a.size();++i)own(o,std::to_string(i),a[i]);return o;}
void push(const JSValue& array,const JSValue& value){array.get("push").call(array,{value});}
JSValue normal(const JSValue& v){if(v.isUndefined())return "#undefined";if(v.isSymbol())return "#symbol";if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "#NaN";if(!std::isfinite(n))return n>0?"#Infinity":"#-Infinity";if(n==0&&std::signbit(n))return "#-0";}return v;}
JSValue read(const JSValue& view,unsigned count){auto out=arr();for(unsigned i=0;i<count;++i)push(out,normal(view.get(JSValue(i))));return out;}
void tick(const JSValue& s,const std::string& name){push(s.get("events"),name);}
JSValue decode(const JSValue&,const JSValue&);
Args decodeArgs(const JSValue& values,const JSValue& s){Args out;for(unsigned i=0;i<values.length();++i)out.push_back(decode(values.get(JSValue(i)),s));return out;}
JSValue coerce(const JSValue& s,const JSValue& definition,const char* event){
    tick(s,event);if(definition.get("mutate").isObject()){const auto change=definition.get("mutate");auto view=s.get("view");view.set(JSValue(change.get("index").toNumber()),decode(change.get("value"),s));}
    if(definition.get("nested").isObject()){const auto nested=definition.get("nested");const auto f=s.get(nested.get("method").strictEquals("fill")?"fill":"copyWithin");const auto result=f.call(s.get("view"),decodeArgs(nested.get("args"),s));push(s.get("events"),arr({"nested.returnSelf",result.sameValue(s.get("view"))}));}
    if(definition.get("gc").toBoolean())s.collectGarbage();
    if(definition.get("throw").toBoolean())throw JSTypeError("fixture conversion");
    if(definition.get("returnObject").toBoolean())return JSValue::object();
    return decode(definition.get("value"),s);
}
JSValue decode(const JSValue& v,const JSValue& s){
    if(!v.isObject())return v;const auto tag=v.get("tag");if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("NaN"))return std::nan("");if(tag.strictEquals("Infinity"))return INFINITY;if(tag.strictEquals("-Infinity"))return -INFINITY;if(tag.strictEquals("-0"))return -0.;if(tag.strictEquals("symbol"))return JSValue::symbol("fixture");
    if(tag.strictEquals("primitive")){
        auto out=JSValue::object();JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            tick(c[0],c[1].get("name").string()+".getToPrimitive");return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{return coerce(c[0],c[1],(c[1].get("name").string()+".toPrimitive."+arg(a,0).string()).c_str());},{c[0],c[1]},"",1);
        },{s,v},"get toPrimitive",0);out.defineProperty(JSValue::wellKnownSymbol("toPrimitive"),d);return out;
    }
    if(tag.strictEquals("fallback")){
        auto out=JSValue::object();for(const auto* name:{"valueOf","toString"}){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{tick(c[0],c[1].get("name").string()+".get."+c[2].string());return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{if(c[2].strictEquals("valueOf")&&c[1].get("objectValueOf").toBoolean()){tick(c[0],c[1].get("name").string()+".valueOf");return JSValue::object();}return coerce(c[0],c[1],(c[1].get("name").string()+"."+c[2].string()).c_str());},{c[0],c[1],c[2]},"",0);},{s,v,name},std::string("get ")+name,0);out.defineProperty(name,d);}return out;
    }
    return v;
}
JSValue metadata(){auto out=arr();const auto first=NativeArray_detail::constructor(0).get("prototype").get("fill"),copy=NativeArray_detail::constructor(0).get("prototype").get("copyWithin");for(std::size_t i=0;i<9;++i){const auto p=NativeArray_detail::constructor(i).get("prototype"),base=p.getPrototype();for(const auto* name:{"fill","copyWithin"}){const auto f=p.get(name);const auto d=base.getOwnPropertyDescriptor(name).value();push(out,rec({{"kind",double(i)},{"method",name},{"name",f.get("name")},{"length",f.get("length")},{"constructible",f.isConstructible()},{"source",f.nativeFunctionSource()},{"shared",f.sameValue(std::string(name)=="fill"?first:copy)},{"ownOnConcretePrototype",p.hasOwn(name)},{"writable",d.writable.value_or(false)},{"enumerable",d.enumerable.value_or(false)},{"configurable",d.configurable.value_or(false)}}));}}return out;}
JSValue run(const JSValue& r){
    if(r.get("kind").strictEquals("metadata"))return metadata();const auto kind=std::size_t(r.get("type").toNumber());const auto ctor=NativeArray_detail::constructor(kind);auto full=ctor.construct({r.get("values")});const auto fullCount=full.get("length").toNumber();const auto elementBytes=ctor.get("BYTES_PER_ELEMENT").toNumber();const auto start=r.get("viewStart").isUndefined()?0:r.get("viewStart").toNumber(),count=r.get("viewLength").isUndefined()?full.get("length").toNumber()-start:r.get("viewLength").toNumber();auto view=ctor.construct({full.get("buffer"),start*elementBytes,count}),bytes=NativeArray_detail::constructor(1).construct({full.get("buffer")});const auto byteCount=bytes.get("length").toNumber();
    if(r.get("rawBytes").isObject())for(unsigned i=0;i<r.get("rawBytes").length();++i)bytes.set(JSValue(i),r.get("rawBytes").get(JSValue(i)));
    auto s=rec({{"events",arr()},{"view",view},{"fill",view.get("fill")},{"copyWithin",view.get("copyWithin")}});const auto method=r.get("method").string();const auto borrowed=r.get("methodType").isUndefined()?ctor:NativeArray_detail::constructor(std::size_t(r.get("methodType").toNumber()));const auto f=borrowed.get("prototype").get(method);JSValue receiver=view;
    if(r.get("receiver").strictEquals("proxy"))receiver=JSValue::proxy(view,JSValue::object());else if(r.get("receiver").strictEquals("fake"))receiver=JSValue::object(ctor.get("prototype"));else if(r.get("receiver").strictEquals("array"))receiver=arr({1,2});else if(r.get("receiver").strictEquals("object"))receiver=rec({{"length",2}});else if(r.get("receiver").strictEquals("buffer"))receiver=full.get("buffer");else if(r.get("receiver").strictEquals("null"))receiver=nullptr;else if(r.get("receiver").strictEquals("undefined"))receiver={};else if(r.get("receiver").strictEquals("number"))receiver=3;
    if(r.get("shadow").toBoolean())for(const auto* key:{"length","byteLength","byteOffset","buffer","constructor"}){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{tick(c[0],"shadow."+c[1].string());throw JSTypeError("must use internal slots");},{s,key},std::string("get ")+key,0);view.defineProperty(key,d);}
    if(r.get("nullPrototype").toBoolean())view.setPrototype(nullptr);if(r.get("preventExtensions").toBoolean())view.preventExtensions();
    const auto args=decodeArgs(r.get("args"),s);JSValue result;bool same=false;try{const auto returned=f.call(receiver,args);same=returned.sameValue(receiver);result=normal(returned.isObject()?JSValue("#object"):returned);}catch(const JSTypeError&){result="#TypeError";}catch(const JSRangeError&){result="#RangeError";}catch(const std::exception&){result="#Error";}
    return rec({{"result",result},{"returnedSelf",same},{"view",read(view,unsigned(count))},{"backing",read(full,unsigned(fullCount))},{"bytes",read(bytes,unsigned(byteCount))},{"events",s.get("events")}});
}
void lifetime(){for(int realm=0;realm<3;++realm){JSWeakValue first,second,viewWeak;{auto ctor=NativeArray_detail::constructor(6),view=ctor.construct({arr({1,2,3,4})}),fill=view.get("fill"),copy=view.get("copyWithin");first=JSWeakValue(fill);second=JSWeakValue(copy);viewWeak=JSWeakValue(view);fill.call(view,{.5,1,3});copy.call(view,{0,1});view.collectGarbage();if(!fill.sameValue(ctor.get("prototype").get("fill")))throw std::logic_error("fill intrinsic identity changed");}JSValue::nativeSafepoint();if(!first.expired()||!second.expired()||!viewWeak.expired()||JSValue::hasActiveHeap())throw std::logic_error("typed method heap retained");}std::cout<<"3 typed-method realm lifetimes passed\n";}
}
int main(int argc,char** argv){try{if(argc>1&&std::string(argv[1])=="--lifetime"){lifetime();return 0;}const std::string text(std::istreambuf_iterator<char>(std::cin),{});const auto input=Animation_detail::parseJSON(text);// Run the fixture in one native call; explicit fixture GC remains observable.
const auto execute=JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue{const auto input=captures[0];auto out=arr();for(unsigned i=0;i<input.length();++i)push(out,run(input.get(JSValue(i))));return out.toJSON()->dump();},{input});std::cout<<execute.call().string();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
