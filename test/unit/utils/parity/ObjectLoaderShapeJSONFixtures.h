#pragma once
#include "loaders/ObjectLoaderShapes.h"
#include "core/JSAsyncFunction.h"
#include <algorithm>
#include <cmath>
#include <memory>

namespace ObjectLoaderShapeJSONFixtures {
using Args=std::vector<JSValue>;
inline JSValue object(){return JSValue::object();}
inline JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
inline void push(JSValue a,const JSValue& v){a.set(JSValue(a.length()),v);}
inline JSValue list(const Args& a={}){auto out=JSValue::array();for(const auto& v:a)push(out,v);return out;}
inline JSValue call(const JSValue& self,const char* key,const Args& a={}){return self.get(key).call(self,a);}
inline void getter(JSValue self,const std::string& key,const JSValue& fn,bool enumerable=true){JSPropertyDescriptor d;d.getter=fn;d.enumerable=enumerable;d.configurable=true;self.defineProperty(key,d);}
struct RestoreProperty {
    JSValue self,key;std::optional<JSPropertyDescriptor> previous;
    RestoreProperty(JSValue s,JSValue k):self(s),key(k),previous(s.getOwnPropertyDescriptor(k)){}
    ~RestoreProperty(){if(previous)self.defineProperty(key,*previous);else self.erase(key);}
};
struct RestorePrototype {
    JSValue self,previous;
    explicit RestorePrototype(JSValue s):self(s),previous(s.getPrototype()){}
    ~RestorePrototype(){self.setPrototype(previous);}
};
inline JSValue constructors(){
    auto out=object();const auto publicCurves=nativeObjectLoaderCurveConstructors();
    for(const auto& name:publicCurves.keys())out.set(name,publicCurves.get(name));
    const auto shape=nativeObjectLoaderShapeConstructor(),path=shape.get("prototype").getPrototype().get("constructor"),curvePath=path.get("prototype").getPrototype().get("constructor"),curve=curvePath.get("prototype").getPrototype().get("constructor");
    out.set("Shape",shape);out.set("Path",path);out.set("CurvePath",curvePath);out.set("Curve",curve);return out;
}
inline const std::vector<std::string>& names(){static const std::vector<std::string> value={"Curve","CurvePath","Path","Shape","ArcCurve","CatmullRomCurve3","CubicBezierCurve","CubicBezierCurve3","EllipseCurve","LineCurve","LineCurve3","QuadraticBezierCurve","QuadraticBezierCurve3","SplineCurve"};return value;}
// This is an alias-preserving snapshot of the returned JSON object itself. It
// deliberately does not classify plain {type,arcLengthDivisions} as live curves.
struct Snapshot {
    std::vector<JSValue> seen;
    JSValue value(const JSValue& source){
        if(source.isUndefined()){auto out=object();out.set("$undefined",true);return out;}
        if(source.type()==JSValue::Type::Number&&!std::isfinite(source.number())){auto out=object();out.set("$number",std::isnan(source.number())?"NaN":source.number()<0?"-Infinity":"Infinity");return out;}
        if(source.isCallable()){auto out=object();out.set("$function",true);return out;}
        if(!source.isObject())return source;
        for(std::size_t i=0;i<seen.size();++i)if(seen[i].sameValue(source)){auto out=object();out.set("$ref",i+1);return out;}
        seen.push_back(source);auto out=object();out.set("$id",seen.size());
        if(source.type()==JSValue::Type::Array){out.set("$type","Array");auto items=list();for(double i=0;i<source.length();++i)push(items,value(source.get(JSValue(i))));out.set("items",items);return out;}
        out.set("$type","Object");auto keys=source.keys();std::sort(keys.begin(),keys.end());for(const auto& key:keys)out.set(key,value(source.get(key)));return out;
    }
};
inline JSValue metadata(){
    const auto ctors=constructors();auto out=object(),entries=list();
    for(const auto& name:names()){
        const auto prototype=ctors.get(name).get("prototype");auto owner=prototype;
        while(!owner.hasOwn("toJSON"))owner=owner.getPrototype();
        const auto descriptor=owner.getOwnPropertyDescriptor("toJSON");const auto fn=prototype.get("toJSON");auto row=object();
        row.set("name",name);row.set("own",prototype.hasOwn("toJSON"));row.set("length",fn.get("length"));row.set("source",call(fn,"toString"));row.set("flags",list({descriptor->writable.value_or(false),descriptor->enumerable.value_or(false),descriptor->configurable.value_or(false)}));row.set("constructible",fn.isConstructible());push(entries,row);
    }out.set("entries",entries);return out;
}
inline JSValue errorValue(){const auto error=nativeExceptionValue(std::current_exception());auto out=object();if(error.isObject())out.set("name",error.get("name"));else{out.set("name","ThrownValue");out.set("raw",error);}return out;}

inline JSValue run(const JSValue& request){
    if(request.get("op").strictEquals("metadata"))return metadata();
    const auto ctors=constructors();auto events=list(),output=object();
    try{
        if(request.get("op").strictEquals("serialize")){
            auto self=ctors.get(request.get("curve")).construct();
            if(!request.get("json").isUndefined())call(self,"fromJSON",{request.get("json")});
            if(request.get("curve").strictEquals("Shape"))self.set("uuid","shape");
            Snapshot snapshot;output.set("value",snapshot.value(call(self,"toJSON")));
        }else{
            const auto variant=request.get("variant");const auto curvePrototype=ctors.get("Curve").get("prototype"),linePrototype=ctors.get("LineCurve").get("prototype");
            auto self=object(),token=JSValue{},result=JSValue{};
            std::unique_ptr<RestoreProperty> restore;std::unique_ptr<RestorePrototype> restorePrototype;
            if(variant.strictEquals("superAlias")||variant.strictEquals("homeObject")||variant.strictEquals("constructorIgnored")){
                self=ctors.get("LineCurve").construct();token=object();token.set("marker",1);const auto original=linePrototype.get("toJSON");
                if(variant.strictEquals("constructorIgnored")){
                    token=JSValue{};restore=std::make_unique<RestoreProperty>(linePrototype,"constructor");auto fakeConstructor=object(),fakePrototype=object(),alternate=object();
                    getter(alternate,"toJSON",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSThrownValue("wrong-super");}));
                    fakePrototype.setPrototype(alternate);fakeConstructor.set("prototype",fakePrototype);auto mutablePrototype=linePrototype;mutablePrototype.set("constructor",fakeConstructor);
                }else{
                    const auto parent=variant.strictEquals("homeObject")?object():curvePrototype;
                    if(variant.strictEquals("homeObject")){restorePrototype=std::make_unique<RestorePrototype>(linePrototype);auto mutablePrototype=linePrototype;mutablePrototype.setPrototype(parent);}else restore=std::make_unique<RestoreProperty>(parent,"toJSON");
                    getter(parent,"toJSON",JSValue::function(+[](const JSValue& receiver,const Args&,const Args& c)->JSValue{
                        push(c[0],list({"get:super",receiver.sameValue(c[1])}));return JSValue::function(+[](const JSValue& r,const Args&,const Args& c)->JSValue{push(c[0],list({"call:super",r.sameValue(c[1])}));return c[2];},c);
                    },{events,self,token}));
                }
                result=original.call(self);
            }else if(variant.strictEquals("rawGetters")||variant.strictEquals("getterThrow")){
                token=object();token.set("raw",1);
                getter(self,"arcLengthDivisions",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:arcLengthDivisions");return c[2].strictEquals("getterThrow")?JSValue(17):c[1];},{events,token,variant}));
                getter(self,"type",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:type");if(c[2].strictEquals("getterThrow"))throw JSThrownValue("type");return c[1];},{events,token,variant}));
                result=curvePrototype.get("toJSON").call(self);
            }else if(variant.strictEquals("vectorOrder")||variant.strictEquals("vectorReenter")||variant.strictEquals("vectorGetThrow")||variant.strictEquals("superNull")||variant.strictEquals("superUndefined")){
                self.set("arcLengthDivisions",7);self.set("type","LineCurve");
                if(variant.strictEquals("superNull")||variant.strictEquals("superUndefined")){
                    restore=std::make_unique<RestoreProperty>(curvePrototype,"toJSON");auto mutablePrototype=curvePrototype;
                    mutablePrototype.set("toJSON",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0].strictEquals("superNull")?JSValue(nullptr):JSValue{};},{variant}));
                }
                for(int i=1;i<=2;++i){
                    const auto label="v"+std::to_string(i);auto point=object();
                    getter(point,"toArray",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
                        push(c[0],"get:"+c[1].string()+":toArray");if(c[1].strictEquals("v2")&&c[2].strictEquals("vectorGetThrow"))throw JSThrownValue("toArray:v2");
                        return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
                            push(c[0],"call:"+c[1].string()+":toArray");
                            if(c[1].strictEquals("v1")&&c[2].strictEquals("vectorReenter")){auto inner=object();inner.set("arcLengthDivisions",9);inner.set("type","inner");const auto nested=c[3].get("toJSON").call(inner);push(c[0],list({"reenter",nested.get("type"),nested.get("arcLengthDivisions")}));}
                            return c[1].strictEquals("v1")?list({1,2}):list({3,4});
                        },c);
                    },{events,label,variant,curvePrototype}));
                    getter(self,label,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:"+c[1].string());return c[2];},{events,label,point}));
                }
                result=linePrototype.get("toJSON").call(self);
            }else{
                const bool curves=variant.strictEquals("pushCurves"),holes=variant.strictEquals("pushHoles");const std::string key=curves?"curves":holes?"holes":"points",method=curves||holes?"toJSON":"toArray";
                const auto className=curves?"CurvePath":holes?"Shape":"SplineCurve";const auto prototype=ctors.get(className).get("prototype"),parent=prototype.getPrototype(),original=prototype.get("toJSON"),originalCurveToJSON=curvePrototype.get("toJSON");
                token=object();token.set("marker",1);restore=std::make_unique<RestoreProperty>(parent,"toJSON");auto mutableParent=parent;
                mutableParent.set("toJSON",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{token}));
                JSPropertyDescriptor descriptor;descriptor.configurable=true;descriptor.enumerable=false;
                descriptor.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:result:"+c[1].string());return c[2].get("saved");},{events,key,token});
                descriptor.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{
                    push(c[0],"set:result:"+c[1].string());auto array=arg(a,0);const auto originalPush=array.get("push");
                    getter(array,"push",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
                        push(c[0],"get:push");if(c[1].strictEquals("pushGetThrow"))throw JSThrownValue("push");
                        return JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue{push(c[0],"call:push");return c[2].call(receiver,a);},c);
                    },{c[0],c[3],originalPush}),false);
                    auto mutableToken=c[2];mutableToken.set("saved",array);return {};
                },{events,key,token,variant});token.defineProperty(key,descriptor);
                auto input=object(),length=object(),child=object();
                length.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"coerce:length");if(c[1].strictEquals("lengthThrow"))throw JSThrownValue("length");return 1;},{events,variant}));
                getter(input,"length",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:length");return c[1];},{events,length}));
                getter(child,method,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
                    push(c[0],"get:child:"+c[1].string());if(c[2].strictEquals("childGetThrow"))throw JSThrownValue("child");
                    return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
                        push(c[0],"call:child:"+c[1].string());if(c[2].strictEquals("pushReenter")){auto inner=object();inner.set("arcLengthDivisions",9);inner.set("type","inner");const auto nested=c[3].call(inner);push(c[0],list({"reenter",nested.get("type"),nested.get("arcLengthDivisions")}));}return list({"child"});
                    },c);
                },{events,method,variant,originalCurveToJSON}));input.set("0",child);
                getter(self,key,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:input:"+c[1].string());return c[2];},{events,key,input}));self.set("autoClose",false);self.set("uuid","shape");
                result=original.call(self);
            }
            auto root=object();root.set("result",result);root.set("token",token);Snapshot snapshot;output.set("value",snapshot.value(root));
        }
    }catch(...){output.set("error",errorValue());}
    output.set("events",events);return output;
}
}
