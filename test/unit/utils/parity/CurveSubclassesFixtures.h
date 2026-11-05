#pragma once
#include "ObjectLoaderShapeJSONFixtures.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include <tuple>

namespace CurveSubclassesFixtures {
using namespace ObjectLoaderShapeJSONFixtures;
inline const std::vector<std::string>& curveNames(){static const std::vector<std::string> v={"ArcCurve","CatmullRomCurve3","CubicBezierCurve","CubicBezierCurve3","EllipseCurve","LineCurve","LineCurve3","QuadraticBezierCurve","QuadraticBezierCurve3","SplineCurve"};return v;}
inline JSValue create(const JSValue& request){return call(nativeObjectLoaderCurveConstructors().get(request.get("curve")).construct(),"fromJSON",{request.get("json")});}
inline JSValue targetVector(const JSValue& request){return request.get("curve").toString().ends_with('3')?nativeVector3Constructor().construct():nativeVector2Constructor().construct();}
inline JSValue seed(){auto points=list({nativeVector3Constructor().construct({1,2,3}),nativeVector3Constructor().construct({4,1,0}),nativeVector3Constructor().construct({9,5,-2})});return nativeObjectLoaderCurveConstructors().get("CatmullRomCurve3").construct({points,false,"catmullrom",0.7});}
inline JSValue clean(const JSValue& value){
    if(value.isUndefined()){auto out=object();out.set("$undefined",true);return out;}
    if(value.type()==JSValue::Type::Number&&(!std::isfinite(value.number())||(value.number()==0&&std::signbit(value.number())))){auto out=object();out.set("$number",std::isnan(value.number())?"NaN":value.number()==0?"-0":value.number()<0?"-Infinity":"Infinity");return out;}
    if(!value.isObject())return value;
    auto out=value.type()==JSValue::Type::Array?list():object();for(const auto& key:value.keys())out.set(key,clean(value.get(key)));return out;
}
inline JSValue metadata(){
    auto out=list();const auto table=nativeObjectLoaderCurveConstructors();
    for(const auto& name:curveNames()){
        const auto ctor=table.get(name),proto=ctor.get("prototype");auto row=object(),keys=list(),methods=list();row.set("name",name);
        auto ctorInfo=object();ctorInfo.set("name",ctor.get("name"));ctorInfo.set("length",ctor.get("length"));ctorInfo.set("source",call(ctor,"toString"));ctorInfo.set("parent",ctor.getPrototype().get("name"));
        const auto pd=ctor.getOwnPropertyDescriptor("prototype"),cd=proto.getOwnPropertyDescriptor("constructor");
        ctorInfo.set("prototypeFlags",list({pd->writable.value_or(false),pd->enumerable.value_or(false),pd->configurable.value_or(false)}));ctorInfo.set("constructorFlags",list({cd->writable.value_or(false),cd->enumerable.value_or(false),cd->configurable.value_or(false)}));row.set("constructor",ctorInfo);
        for(const auto& key:proto.ownPropertyKeys())if(key.type()==JSValue::Type::String){
            push(keys,key);if(key.strictEquals("constructor"))continue;
            const auto fn=proto.get(key);const auto d=proto.getOwnPropertyDescriptor(key);auto m=object();
            m.set("key",key);m.set("name",fn.get("name"));m.set("length",fn.get("length"));m.set("source",call(fn,"toString"));m.set("flags",list({d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)}));m.set("constructible",fn.isConstructible());push(methods,m);
        }
        row.set("keys",keys);row.set("methods",methods);push(out,row);
    }return out;
}
inline JSValue construct(const JSValue& request,JSValue events){
    auto ctor=nativeObjectLoaderCurveConstructors().get(request.get("curve"));const auto variant=request.get("variant");auto token=object(),marker=object();JSValue result;
    if(variant.strictEquals("newTarget")||variant.strictEquals("newTargetFallback")){
        auto alternate=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Alternate",0,JSValue::ConstructorKind::Callable);alternate.set("prototype",variant.strictEquals("newTargetFallback")?JSValue(7):marker);result=ctor.construct({},alternate);
    }else if(variant.strictEquals("constructorIgnored")){
        auto proto=ctor.get("prototype");RestoreProperty restore(proto,"constructor");proto.set("constructor",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSThrownValue("wrong-constructor");}));result=ctor.construct();
    }else if(variant.strictEquals("superObject")||variant.strictEquals("superThrow")||variant.strictEquals("fieldThrow")){
        RestorePrototype restore(ctor);
        for(const auto* key:{"type","arcLengthDivisions","isArcCurve","isEllipseCurve","isCatmullRomCurve3","isCubicBezierCurve","isCubicBezierCurve3","isLineCurve","isLineCurve3","isQuadraticBezierCurve","isQuadraticBezierCurve3","isSplineCurve","v0","v1","v2","v3","points","closed","curveType","tension","aX","aY","xRadius","yRadius","aStartAngle","aEndAngle","aClockwise","aRotation"}){
            JSPropertyDescriptor d;d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{push(c[0],list({"set:"+c[1].toString(),clean(arg(a,0))}));if(c[2].strictEquals("fieldThrow")&&c[1].strictEquals("type"))throw JSThrownValue("field");return {};},{events,key,variant});d.configurable=true;token.defineProperty(key,d);
        }
        ctor.setPrototype(JSValue::constructor(+[](const JSValue&,const Args& a,const Args& c)->JSValue{push(c[0],list({"super",a.size()}));if(c[1].strictEquals("superThrow"))throw JSThrownValue("super");return c[2];},{events,variant,token},"",0,JSValue::ConstructorKind::Callable));result=ctor.construct();
    }else if(variant.strictEquals("rawRadius"))result=ctor.construct({2,3,token,0,1,false});
    else result=ctor.construct();
    auto fields=list();for(const auto& key:result.keys())push(fields,list({key,clean(result.get(key))}));auto out=object();out.set("same",result.sameValue(token));out.set("prototype",result.getPrototype().sameValue(variant.strictEquals("newTarget")?marker:variant.strictEquals("superObject")||variant.strictEquals("newTargetFallback")?token.getPrototype():ctor.get("prototype")));out.set("fields",fields);
    if(variant.strictEquals("rawRadius"))out.set("radiusAlias",result.get("xRadius").sameValue(token)&&result.get("yRadius").sameValue(token));return out;
}
inline JSValue observable(const JSValue& request,JSValue events){
    auto self=create(request);const auto variant=request.get("variant");auto target=targetVector(request),t=request.get("t"),token=object();auto state=object();
    if(variant.strictEquals("coercion")||variant.strictEquals("coercionThrow")){
        t=object();t.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"coerce:t");if(c[1].strictEquals("coercionThrow"))throw JSThrownValue("coercion");return c[2];},{events,variant,request.get("t")}));
    }
    if(variant.strictEquals("getterOrder")){
        std::vector<std::string> fields;const auto name=request.get("curve").toString();
        if(name=="EllipseCurve"||name=="ArcCurve")fields={"aEndAngle","aStartAngle","aClockwise","aX","aY","xRadius","yRadius","aRotation"};
        else if(name=="SplineCurve"||name=="CatmullRomCurve3")fields={"points","closed","curveType","tension"};
        else for(const auto& key:request.get("json").keys())if(key.size()==2&&key[0]=='v'&&key[1]>='0'&&key[1]<='9')fields.push_back(key);
        for(const auto& key:fields){const auto value=self.get(key);getter(self,key,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:"+c[1].toString());return c[2];},{events,key,value}));}
        if(self.get("points").toBoolean()){const auto points=self.get("points");
            auto input=object();getter(input,"length",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:points.length");return c[1].get("length");},{events,points}));
            for(double i=0;i<points.length();++i)getter(input,JSValue(i).toString(),JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:point:"+c[1].toString());return c[2];},{events,i,points.get(JSValue(i))}));
            getter(self,"points",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:points");return c[1];},{events,input}));
        }
    }
    if(variant.strictEquals("targetSet")||variant.strictEquals("targetReentry")){
        target=object();getter(target,"set",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            push(c[0],"get:set");if(c[1].strictEquals("targetReentry"))push(c[0],list({"inner",call(call(seed(),"getPoint",{0.2}),"toArray")}));
            return JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue{push(c[0],list({"call:set",receiver.sameValue(c[2])}));auto holder=c[3];holder.set("received",list(a));return c[4];},c);
        },{events,variant,target,state,token}));
    }
    if(variant.strictEquals("targetNull"))target=nullptr;
    if(variant.strictEquals("lineChain")){
        target=object();auto stage=object(),stage2=object();
        const std::vector<std::tuple<JSValue,const char*,JSValue>> chains={{target,"copy",stage},{stage,"sub",object()},{target,"multiplyScalar",stage2},{stage2,"add",object()}};
        for(const auto& [obj,key,ret]:chains)getter(obj,key,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            push(c[0],"get:"+c[1].toString());return JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue{
                const auto v=arg(a,0);const auto name=v.sameValue(c[4].get("v1"))?JSValue("v1"):v.sameValue(c[4].get("v2"))?JSValue("v2"):v;push(c[0],list({"call:"+c[1].toString(),receiver.sameValue(c[2]),name}));return c[3];
            },c);
        },{events,key,obj,ret,self}));
        for(const auto* key:{"v1","v2"}){const auto v=self.get(key);getter(self,key,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:"+c[1].toString());return c[2];},{events,key,v}));}
    }
    if(variant.strictEquals("superCopyHome")){
        auto proto=nativeObjectLoaderCurveConstructors().get("LineCurve").get("prototype"),alternate=object();RestorePrototype restore(proto);auto source=create(request);token=source;
        getter(alternate,"copy",JSValue::function(+[](const JSValue& receiver,const Args&,const Args& c)->JSValue{
            push(c[0],list({"get:super",receiver.sameValue(c[1])}));return JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{push(c[0],list({"call:super",r.sameValue(c[1]),arg(a,0).sameValue(c[2])}));return nullptr;},c);
        },{events,self,token}));
        proto.setPrototype(alternate);const auto result=proto.get("copy").call(self,{source});proto.setPrototype(restore.previous);auto out=object();out.set("same",result.sameValue(self));out.set("json",call(self,"toJSON"));return out;
    }
    const auto result=call(self,"getPoint",{t,target});auto out=object();out.set("same",result.sameValue(target));
    if(variant.strictEquals("targetSet")||variant.strictEquals("targetReentry")||variant.strictEquals("lineChain")){out.set("token",result.sameValue(token));out.set("received",state.get("received"));}
    else out.set("point",call(result,"toArray"));return out;
}
inline JSValue run(const JSValue& request){
    if(request.get("op").strictEquals("metadata"))return metadata();auto events=list(),output=object();
    try{
        JSValue value;
        if(request.get("op").strictEquals("construct"))value=construct(request,events);
        else if(request.get("op").strictEquals("sample")){
            const auto self=create(request);if(request.get("seed").toBoolean())call(seed(),"getPoint",{0.2});
            const auto target=request.get("target").toBoolean()?targetVector(request):JSValue{};const auto result=self.get(request.get("method")).call(self,{request.get("t"),target});value=object();value.set("point",call(result,"toArray"));value.set("same",result.sameValue(target));
        }else if(request.get("op").strictEquals("copy")){
            const auto self=nativeObjectLoaderCurveConstructors().get(request.get("curve")).construct(),source=create(request),result=call(self,"copy",{source});value=object();value.set("same",result.sameValue(self));value.set("json",call(self,"toJSON"));auto aliases=list();
            for(const auto& key:request.get("json").keys())if(key.size()==2&&key[0]=='v'&&key[1]>='0'&&key[1]<='9')push(aliases,self.get(key).sameValue(source.get(key)));
            if(source.get("points").toBoolean()){aliases=list({self.get("points").sameValue(source.get("points"))});for(double i=0;i<self.get("points").length();++i)push(aliases,self.get("points").get(JSValue(i)).sameValue(source.get("points").get(JSValue(i))));}value.set("aliases",aliases);
        }else value=observable(request,events);
        output.set("value",clean(value));output.set("events",clean(events));
    }catch(...){output.set("error",errorValue());output.set("events",clean(events));}return output;
}
}
