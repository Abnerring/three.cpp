#pragma once
#include "ObjectLoaderShapeJSONFixtures.h"
#include "core/JSPromise.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include <utility>

namespace CurvePathsFixtures {
using namespace ObjectLoaderShapeJSONFixtures;
inline JSValue classes() {
    auto out=object();const auto shape=nativeObjectLoaderShapeConstructor(),path=shape.getPrototype(),curvePath=path.getPrototype();
    out.set("Shape",shape);out.set("Path",path);out.set("CurvePath",curvePath);return out;
}
inline JSValue clean(const JSValue& value) {
    if(value.isUndefined()){auto out=object();out.set("$undefined",true);return out;}
    if(value.type()==JSValue::Type::Number&&(!std::isfinite(value.number())||(value.number()==0&&std::signbit(value.number())))){
        auto out=object();out.set("$number",std::isnan(value.number())?"NaN":value.number()==0?"-0":value.number()<0?"-Infinity":"Infinity");return out;
    }
    if(value.isCallable()){auto out=object();out.set("$function",true);return out;}
    if(!value.isObject())return value;
    auto out=value.type()==JSValue::Type::Array?list():object();for(const auto& key:value.keys())out.set(key,clean(value.get(key)));return out;
}
inline JSValue v2(double x=0,double y=0){return nativeVector2Constructor().construct({x,y});}
inline JSValue v3(double x=0,double y=0,double z=0){return nativeVector3Constructor().construct({x,y,z});}
inline JSValue points(const JSValue& input) {
    auto out=list();for(double i=0;i<input.length();++i){const auto p=input.get(JSValue(i));push(out,nativeVector2Constructor().construct({p.get("0"),p.get("1")}));}return out;
}
inline JSValue pointValues(const JSValue& value) {
    if(value.type()==JSValue::Type::Array){auto out=list();for(double i=0;i<value.length();++i)push(out,pointValues(value.get(JSValue(i))));return out;}
    if(value.isObject()&&(value.get("isVector2").strictEquals(true)||value.get("isVector3").strictEquals(true)))return call(value,"toArray");return value;
}
inline bool validUUID(const JSValue& value) {
    if(value.type()!=JSValue::Type::String)return false;const auto text=value.string();if(text.size()!=36)return false;
    for(std::size_t i=0;i<text.size();++i){const auto ch=text[i];if(i==8||i==13||i==18||i==23){if(ch!='-')return false;}else if(!((ch>='0'&&ch<='9')||(ch>='a'&&ch<='f')))return false;}return text[14]=='4';
}
inline void traceGet(JSValue self,const std::string& key,JSValue events,const std::string& label,JSValue value) {
    getter(self,key,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],c[1]);return c[2];},{events,label,value}));
}
inline JSValue curvePath(unsigned dim) {
    auto out=classes().get("CurvePath").construct(),table=nativeObjectLoaderCurveConstructors();
    if(dim==3){call(out,"add",{table.get("LineCurve3").construct({v3(0,0,0),v3(3,0,4)})});call(out,"add",{table.get("LineCurve3").construct({v3(3,0,4),v3(3,4,4)})});}
    else{call(out,"add",{table.get("LineCurve").construct({v2(0,0),v2(3,0)})});call(out,"add",{table.get("QuadraticBezierCurve").construct({v2(3,0),v2(4,4),v2(6,2)})});}return out;
}
inline JSValue populated(const std::string& name) {
    if(name=="CurvePath")return curvePath(2);
    auto out=classes().get(name).construct({list({v2(0,0),v2(4,0),v2(4,3)})});
    call(out,"quadraticCurveTo",{2,5,0,3});call(out,"closePath");out.set("autoClose",true);
    if(name=="Shape"){out.set("uuid","source-shape");auto hole=classes().get("Path").construct({list({v2(1,1),v2(1,2),v2(2,1)})});call(hole,"closePath");push(out.get("holes"),hole);}return out;
}
inline JSValue metadata() {
    auto out=list();const auto table=classes();for(const auto* name:{"CurvePath","Path","Shape"}){
        const auto ctor=table.get(name),proto=ctor.get("prototype");auto row=object(),keys=list(),methods=list();row.set("name",name);row.set("length",ctor.get("length"));row.set("source",call(ctor,"toString"));
        for(const auto& key:proto.ownPropertyKeys())if(key.type()==JSValue::Type::String){push(keys,key);if(key.strictEquals("constructor"))continue;const auto fn=proto.get(key);const auto d=proto.getOwnPropertyDescriptor(key);auto m=object();m.set("key",key);m.set("name",fn.get("name"));m.set("length",fn.get("length"));m.set("source",call(fn,"toString"));m.set("flags",list({d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)}));m.set("constructible",fn.isConstructible());push(methods,m);}
        row.set("keys",keys);row.set("methods",methods);push(out,row);
    }return out;
}
inline JSValue construct(const JSValue& request,JSValue events) {
    const auto name=request.get("class").toString();const auto variant=request.get("variant");auto ctor=classes().get(name),token=object(),marker=object(),result=JSValue{};
    if(variant.strictEquals("newTarget")){auto target=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Alternate",0,JSValue::ConstructorKind::Callable);target.set("prototype",marker);result=ctor.construct({},target);}
    else if(variant.strictEquals("constructorIgnored")){auto proto=ctor.get("prototype");RestoreProperty restore(proto,"constructor");proto.set("constructor",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSThrownValue("wrong-constructor");}));result=ctor.construct();}
    else if(variant.strictEquals("superObject")||variant.strictEquals("superThrow")||variant.strictEquals("fieldThrow")){
        RestorePrototype restore(ctor);for(const auto* key:{"type","curves","autoClose","currentPoint","uuid","holes"}){JSPropertyDescriptor d;d.configurable=true;d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{push(c[0],list({"set:"+c[1].toString(),c[1].strictEquals("uuid")?JSValue(validUUID(arg(a,0))):clean(arg(a,0))}));if(c[2].strictEquals("fieldThrow")&&c[1].strictEquals("type"))throw JSThrownValue("field");return {};},{events,key,variant});token.defineProperty(key,d);}
        ctor.setPrototype(JSValue::constructor(+[](const JSValue&,const Args& a,const Args& c)->JSValue{push(c[0],list({"super",a.size()}));if(c[1].strictEquals("superThrow"))throw JSThrownValue("super");return c[2];},{events,variant,token},"",0,JSValue::ConstructorKind::Callable));result=ctor.construct();
    }else if(variant.strictEquals("points"))result=ctor.construct({list({v2(1,2),v2(3,4),v2(-1,5)})});
    else if(variant.strictEquals("emptyPoints"))result=ctor.construct({list()});
    else if(variant.strictEquals("falsePoints"))result=ctor.construct({false});
    else result=ctor.construct();
    auto out=object(),fields=list();for(const auto& key:result.keys())push(fields,list({key,key=="uuid"?JSValue("$uuid"):clean(result.get(key))}));out.set("same",result.sameValue(token));out.set("fields",fields);out.set("prototype",result.getPrototype().sameValue(variant.strictEquals("newTarget")?marker:variant.strictEquals("superObject")?token.getPrototype():ctor.get("prototype")));if(name=="Shape"&&!variant.strictEquals("superObject"))out.set("uuidValid",validUUID(result.get("uuid")));return out;
}
inline JSValue numeric(const JSValue& request) {
    auto self=curvePath(static_cast<unsigned>(request.get("dim").toNumber()));self.set("autoClose",request.get("close").toBoolean());const auto method=request.get("method").toString();Args args;
    if(request.hasOwn("arg"))args.push_back(request.get("arg"));auto target=request.get("dim").strictEquals(3)?v3():v2();if(request.get("target").toBoolean())args.push_back(target);
    const auto result=self.get(method).call(self,args);auto out=object();out.set("value",result.sameValue(self)?JSValue("$self"):pointValues(result));out.set("target",result.sameValue(target));out.set("cache",result.sameValue(self.get("cacheLengths")));out.set("json",call(self,"toJSON"));
    if(result.type()==JSValue::Type::Array&&result.length()>0)out.set("closedAlias",result.get("0").sameValue(result.get(JSValue(result.length()-1))));return out;
}
inline JSValue sequence(const JSValue& request) {
    auto self=classes().get("Path").construct(),returns=list();const auto actions=request.get("actions");for(double i=0;i<actions.length();++i){const auto action=actions.get(JSValue(i));Args args;const auto values=action.get("args");for(double j=0;j<values.length();++j){auto value=values.get(JSValue(j));if(value.isObject()&&value.hasOwn("points"))value=points(value.get("points"));args.push_back(value);}push(returns,self.get(action.get("method")).call(self,args).sameValue(self));}
    auto out=object();out.set("returns",returns);out.set("json",call(self,"toJSON"));out.set("points",pointValues(call(self,"getPoints",{3})));return out;
}
inline JSValue copyOrJSON(const JSValue& request) {
    const auto name=request.get("class").toString();auto source=populated(name),target=classes().get(name).construct();if(name=="Shape")target.set("uuid","target-shape");auto out=object();
    if(request.get("op").strictEquals("roundtrip")){const auto json=call(source,"toJSON");out.set("same",call(target,"fromJSON",{json}).sameValue(target));out.set("json",call(target,"toJSON"));}
    else{out.set("same",call(target,"copy",{source}).sameValue(target));auto aliases=list({target.get("curves").sameValue(source.get("curves"))});for(double i=0;i<source.get("curves").length();++i)push(aliases,target.get("curves").get(JSValue(i)).sameValue(source.get("curves").get(JSValue(i))));if(name!="CurvePath")push(aliases,target.get("currentPoint").sameValue(source.get("currentPoint")));if(name=="Shape"){push(aliases,target.get("holes").sameValue(source.get("holes")));push(aliases,target.get("holes").get("0").sameValue(source.get("holes").get("0")));}out.set("aliases",aliases);out.set("json",call(target,"toJSON"));}
    out.set("points",pointValues(call(target,"getPoints",{3})));return out;
}
inline JSValue observe(const JSValue& request,JSValue events) {
    const auto variant=request.get("variant").toString();const auto table=classes();auto self=object(),token=object(),result=JSValue{};const auto cp=table.get("CurvePath").get("prototype"),pp=table.get("Path").get("prototype"),sp=table.get("Shape").get("prototype");
    if(variant=="addDispatch"){
        auto curves=object();traceGet(self,"curves",events,"get:curves",curves);traceGet(curves,"push",events,"get:push",JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{push(c[0],list({"push",r.sameValue(c[1]),arg(a,0).sameValue(c[2])}));return 13;},{events,curves,token}));result=cp.get("add").call(self,{token});
    }else if(variant=="cacheHit"||variant=="cacheMiss"||variant=="cacheThrow"){
        auto curves=list(),cache=list({7,9});for(int i=0;i<2;++i){auto child=object();traceGet(child,"getLength",events,"get:length:"+std::to_string(i),JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"call:length:"+c[1].toString());return c[2];},{events,i,i==0?JSValue("2"):JSValue(3)}));push(curves,child);}
        traceGet(self,"curves",events,"get:curves",curves);if(variant=="cacheThrow")getter(self,"cacheLengths",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:cache");throw JSThrownValue("cache");},{events}));else traceGet(self,"cacheLengths",events,"get:cache",variant=="cacheHit"?cache:JSValue(nullptr));
        if(variant=="cacheMiss"){JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:cache");return nullptr;},{events});d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{push(c[0],list({"set:cache",arg(a,0)}));return {};},{events});d.configurable=true;self.defineProperty("cacheLengths",d);}
        result=cp.get("getCurveLengths").call(self);auto out=object();out.set("value",result);out.set("same",result.sameValue(cache));return out;
    }else if(variant=="updateDispatch"){
        for(const auto* key:{"needsUpdate","cacheLengths"}){JSPropertyDescriptor d;d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{push(c[0],list({"set:"+c[1].toString(),arg(a,0)}));return {};},{events,key});d.configurable=true;self.defineProperty(key,d);}
        traceGet(self,"getCurveLengths",events,"get:lengths",JSValue::function(+[](const JSValue& r,const Args&,const Args& c)->JSValue{push(c[0],list({"call:lengths",r.sameValue(c[1])}));return 3;},{events,self}));result=cp.get("updateArcLengths").call(self);
    }else if(variant=="lengthDispatch"){
        auto lengths=object();traceGet(lengths,"length",events,"get:length",3);traceGet(lengths,"2",events,"get:2","tail");traceGet(self,"getCurveLengths",events,"get:lengths",JSValue::function(+[](const JSValue& r,const Args&,const Args& c)->JSValue{push(c[0],list({"call:lengths",r.sameValue(c[1])}));return c[2];},{events,self,lengths}));result=cp.get("getLength").call(self);
    }else if(variant=="pointDispatch"||variant=="pointZero"||variant=="pointOutside"||variant=="pointCoercionThrow"){
        const bool zero=variant=="pointZero";auto lengths=list({zero?JSValue(0):JSValue(2),5}),curves=list();for(int i=0;i<2;++i){auto child=object();traceGet(child,"getLength",events,"get:segmentLength",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"call:segmentLength");return c[1];},{events,zero?JSValue(0):JSValue(i==0?2:3)}));traceGet(child,"getPointAt",events,"get:pointAt",JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{push(c[0],list({"call:pointAt",r.sameValue(c[1]),arg(a,0),arg(a,1).sameValue(c[2])}));return c[2];},{events,child,token}));push(curves,child);}
        traceGet(self,"getLength",events,"get:totalLength",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"call:totalLength");return 5;},{events}));traceGet(self,"getCurveLengths",events,"get:lengths",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"call:lengths");return c[1];},{events,lengths}));traceGet(self,"curves",events,"get:curves",curves);auto t=object();t.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"coerce:t");if(c[1].strictEquals("pointCoercionThrow"))throw JSThrownValue("t");return c[1].strictEquals("pointOutside")?JSValue(2):c[1].strictEquals("pointZero")?JSValue(0):JSValue(.6);},{events,variant}));result=cp.get("getPoint").call(self,{t,token});return result.sameValue(token)?JSValue("$token"):result;
    }else if(variant=="spacedDispatch"||variant=="spacedThrow"){
        traceGet(self,"getPoint",events,"get:point",JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{push(c[0],list({"call:point",r.sameValue(c[1]),arg(a,0)}));if(c[2].strictEquals("spacedThrow")&&arg(a,0).strictEquals(.5))throw JSThrownValue("point");return arg(a,0);},{events,self,variant}));traceGet(self,"autoClose",events,"get:autoClose",true);auto divisions=object();divisions.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"coerce:divisions");return 2;},{events}));result=cp.get("getSpacedPoints").call(self,{divisions});
    }else if(variant=="pointsResolution"){
        auto curves=list();const std::vector<std::string> kinds={"ellipse","line","line3","spline","other"};auto a=v2(0,0),b=v2(1,0),c=v2(1,1);for(std::size_t i=0;i<kinds.size();++i){auto child=object();for(const auto* flag:{"isEllipseCurve","isLineCurve","isLineCurve3","isSplineCurve"})traceGet(child,flag,events,"get:"+kinds[i]+":"+flag,(kinds[i]=="ellipse"&&std::string(flag)=="isEllipseCurve")||(kinds[i]=="line"&&std::string(flag)=="isLineCurve")||(kinds[i]=="line3"&&std::string(flag)=="isLineCurve3")||(kinds[i]=="spline"&&std::string(flag)=="isSplineCurve"));child.set("points",list({0,1,2}));traceGet(child,"getPoints",events,"get:"+kinds[i]+":getPoints",JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{push(c[0],list({"call:"+c[1].toString(),r.sameValue(c[2]),arg(a,0)}));return c[3];},{events,kinds[i],child,i==0?list({a,b}):list({b,c})}));push(curves,child);}self.set("curves",curves);self.set("autoClose",true);result=cp.get("getPoints").call(self,{2});auto out=object();out.set("points",pointValues(result));out.set("closedAlias",result.get("0").sameValue(result.get(JSValue(result.length()-1))));return out;
    }else if(variant=="closeEmpty"){
        self.set("curves",list());result=cp.get("closePath").call(self);
    }else if(variant=="close2"||variant=="close3"||variant=="closeEqual"){
        const bool dim3=variant=="close3";auto a=dim3?v3(1,2,3):v2(1,2),b=variant=="closeEqual"?a:dim3?v3(4,5,6):v2(4,5),curves=list();for(int i=0;i<2;++i){auto child=object();traceGet(child,"getPoint",events,"get:point:"+std::to_string(i),JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{push(c[0],list({"call:point",arg(a,0)}));return c[1];},{events,i==0?a:b}));push(curves,child);}self.set("curves",curves);result=cp.get("closePath").call(self);auto out=object();out.set("same",result.sameValue(self));out.set("count",curves.length());if(curves.length()==3){const auto line=curves.get("2");out.set("type",line.get("type"));out.set("aliases",list({line.get("v1").sameValue(b),line.get("v2").sameValue(a)}));}return out;
    }else if(variant=="forwardArc"||variant=="forwardEllipse"||variant=="forwardAbsarc"){
        auto current=object();traceGet(current,"x",events,"get:x",10);traceGet(current,"y",events,"get:y",20);traceGet(self,"currentPoint",events,"get:currentPoint",current);const auto destination=variant=="forwardArc"?"absarc":"absellipse";traceGet(self,destination,events,std::string("get:")+destination,JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{push(c[0],list({"forward",r.sameValue(c[1]),list(a)}));return c[2];},{events,self,token}));
        auto x=object(),y=object();for(auto p:{std::pair{x,"x"},std::pair{y,"y"}})p.first.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"coerce:"+c[1].toString());return c[1].strictEquals("x")?JSValue("A"):JSValue(2);},{events,p.second}));
        Args args=variant=="forwardArc"||variant=="forwardAbsarc"?Args{x,y,3,.2,1,true}:Args{x,y,3,4,.2,1,false,.3};const auto method=variant=="forwardArc"?"arc":variant=="forwardEllipse"?"ellipse":"absarc";result=pp.get(method).call(self,args);return result.sameValue(self);
    }else if(variant=="setFromPointsDispatch"||variant=="setFromPointsThrow"){
        auto input=object();traceGet(input,"length",events,"get:points.length",3);for(int i=0;i<3;++i){auto p=object();traceGet(p,"x",events,"get:x:"+std::to_string(i),i);if(variant=="setFromPointsThrow"&&i==1)getter(p,"y",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get:y:1");throw JSThrownValue("y");},{events}));else traceGet(p,"y",events,"get:y:"+std::to_string(i),i+4);traceGet(input,std::to_string(i),events,"get:point:"+std::to_string(i),p);}for(const auto* key:{"moveTo","lineTo"})traceGet(self,key,events,std::string("get:")+key,JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{push(c[0],list({"call:"+c[1].toString(),r.sameValue(c[2]),list(a)}));return nullptr;},{events,key,self}));result=pp.get("setFromPoints").call(self,{input});return result.sameValue(self);
    }else if(variant=="moveDispatch"||variant=="lineDispatch"||variant=="quadraticDispatch"||variant=="cubicDispatch"||variant=="splineDispatch"||variant=="splineEmpty"){
        auto current=object(),curves=object(),clone=v2(1,2),input=list({v2(3,4),v2(5,6)});if(variant=="splineEmpty")input=list();traceGet(self,"currentPoint",events,"get:currentPoint",current);traceGet(self,"curves",events,"get:curves",curves);
        traceGet(current,"clone",events,"get:clone",JSValue::function(+[](const JSValue& r,const Args&,const Args& c)->JSValue{push(c[0],list({"clone",r.sameValue(c[1])}));return c[2];},{events,current,clone}));
        for(const auto* key:{"set","copy"})traceGet(current,key,events,std::string("get:")+key,JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{push(c[0],list({"call:"+c[1].toString(),r.sameValue(c[2]),c[1].strictEquals("copy")?pointValues(arg(a,0)):list(a)}));if(c[3].strictEquals("splineEmpty")&&arg(a,0).isUndefined())throw JSThrownValue("empty");return nullptr;},{events,key,current,variant}));
        traceGet(curves,"push",events,"get:push",JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{const auto curve=arg(a,0);auto info=list({"push",r.sameValue(c[1]),curve.get("type")});if(curve.get("isSplineCurve").toBoolean())push(info,list({curve.get("points").get("0").sameValue(c[2]),curve.get("points").get("1").sameValue(c[3].get("0"))}));else push(info,curve.get(curve.get("isLineCurve").toBoolean()?"v1":"v0").sameValue(c[2]));push(c[0],info);return 0;},{events,curves,clone,input}));
        const auto method=variant=="moveDispatch"?"moveTo":variant=="lineDispatch"?"lineTo":variant=="quadraticDispatch"?"quadraticCurveTo":variant=="cubicDispatch"?"bezierCurveTo":"splineThru";Args args=variant=="quadraticDispatch"?Args{3,4,5,6}:variant=="cubicDispatch"?Args{3,4,5,6,7,8}:variant=="splineDispatch"||variant=="splineEmpty"?Args{input}:Args{9,10};result=pp.get(method).call(self,args);return result.sameValue(self);
    }else if(variant=="absellipseJoin"||variant=="absellipseNoJoin"||variant=="absellipseEmpty"){
        auto ellipse=nativeObjectLoaderCurveConstructors().get("EllipseCurve").get("prototype");RestoreProperty restore(ellipse,"getPoint");auto start=v2(1,2),end=v2(3,4),current=variant=="absellipseNoJoin"?start:v2(8,9),curves=object();traceGet(curves,"length",events,"get:curves.length",variant=="absellipseEmpty"?0:1);traceGet(self,"curves",events,"get:curves",curves);traceGet(self,"currentPoint",events,"get:currentPoint",current);
        ellipse.set("getPoint",JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{push(c[0],list({"ellipse:point",r.get("type"),arg(a,0)}));return arg(a,0).strictEquals(0)?c[1]:c[2];},{events,start,end}));
        traceGet(self,"lineTo",events,"get:lineTo",JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{push(c[0],list({"lineTo",r.sameValue(c[1]),list(a)}));return {};},{events,self}));traceGet(curves,"push",events,"get:push",JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{push(c[0],list({"push",r.sameValue(c[1]),arg(a,0).get("type")}));return 0;},{events,curves}));
        result=pp.get("absellipse").call(self,{0,0,2,3,0,1,false,0});auto out=object();out.set("same",result.sameValue(self));out.set("point",call(current,"toArray"));return out;
    }else if(variant=="extractDispatch"||variant=="extractThrow"){
        auto shape=list({1}),holes=list({2});for(auto [key,value]:std::vector<std::pair<const char*,JSValue>>{{"getPoints",shape},{"getPointsHoles",holes}})traceGet(self,key,events,std::string("get:")+key,JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{push(c[0],list({"call:"+c[1].toString(),r.sameValue(c[2]),arg(a,0).sameValue(c[3])}));if(c[4].strictEquals("extractThrow"))throw JSThrownValue("extract");return c[5];},{events,key,self,token,variant,value}));result=sp.get("extractPoints").call(self,{token});return list({result.get("shape").sameValue(shape),result.get("holes").sameValue(holes)});
    }else if(variant=="holesDispatch"||variant=="holesReenter"||variant=="holesThrow"){
        auto input=object();traceGet(input,"length",events,"get:holes.length",2);for(int i=0;i<2;++i){auto hole=object();traceGet(hole,"getPoints",events,"get:holePoints:"+std::to_string(i),JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{push(c[0],list({"holePoints",r.sameValue(c[1]),arg(a,0).sameValue(c[2]),c[3]}));if(c[4].strictEquals("holesThrow"))throw JSThrownValue("hole");if(c[4].strictEquals("holesReenter")&&c[3].strictEquals(0)){auto inner=object();inner.set("holes",list());push(c[0],list({"inner",c[5].get("getPointsHoles").call(inner,{1})}));}return list({c[3]});},{events,hole,token,i,variant,sp}));traceGet(input,std::to_string(i),events,"get:hole:"+std::to_string(i),hole);}traceGet(self,"holes",events,"get:holes",input);result=sp.get("getPointsHoles").call(self,{token});
    }else if(variant=="copySuperHome"||variant=="copySuperThrow"){
        const auto name=request.get("class").toString();auto proto=table.get(name).get("prototype"),alternate=object(),source=populated(name);self=table.get(name).construct();if(name=="Shape")self.set("uuid","target-shape");RestorePrototype restore(proto);const auto original=proto.get("copy");traceGet(alternate,"copy",events,"get:superCopy",JSValue::function(+[](const JSValue& r,const Args& a,const Args& c)->JSValue{push(c[0],list({"superCopy",r.sameValue(c[1]),arg(a,0).sameValue(c[2])}));if(c[3].strictEquals("copySuperThrow"))throw JSThrownValue("supercopy");return nullptr;},{events,self,source,variant}));proto.setPrototype(alternate);result=original.call(self,{source});proto.setPrototype(restore.previous);auto out=object();out.set("same",result.sameValue(self));out.set("json",call(self,"toJSON"));return out;
    }else throw std::logic_error("Unknown CurvePaths observable: "+variant);
    return result;
}
inline JSValue run(const JSValue& request) {
    if(request.get("op").strictEquals("metadata"))return metadata();auto output=object(),events=list();try{
        JSValue value;const auto op=request.get("op");if(op.strictEquals("construct"))value=construct(request,events);else if(op.strictEquals("numeric"))value=numeric(request);else if(op.strictEquals("sequence"))value=sequence(request);else if(op.strictEquals("copy")||op.strictEquals("roundtrip"))value=copyOrJSON(request);
        else if(op.strictEquals("shapePoints")){auto self=populated("Shape");Args args;if(request.hasOwn("arg"))args.push_back(request.get("arg"));const auto result=call(self,"extractPoints",args);value=object();value.set("shape",pointValues(result.get("shape")));value.set("holes",pointValues(result.get("holes")));}
        else value=observe(request,events);output.set("value",clean(value));output.set("events",clean(events));
    }catch(...){output.set("error",errorValue());output.set("events",clean(events));}return output;
}
}
