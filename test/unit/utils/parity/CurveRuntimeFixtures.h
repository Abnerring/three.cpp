#pragma once
#include "ObjectLoaderShapeJSONFixtures.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "utils.h"
#include "core/JSNumber.h"

namespace CurveRuntimeFixtures {
using namespace ObjectLoaderShapeJSONFixtures;
inline JSValue curveConstructor() { return constructors().get("Curve"); }
inline JSValue sampledCurve(double dimensions, JSValue events) {
    auto curve=curveConstructor().construct();curve.set("arcLengthDivisions",4);
    curve.set("getPoint",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
        const auto t=arg(a,0).toNumber();push(c[0],list({"point",arg(a,0),!arg(a,1).isUndefined()}));
        auto target=arg(a,1);if(!target.toBoolean())target=(c[1].strictEquals(2)?nativeVector2Constructor():nativeVector3Constructor()).construct();
        return call(target,"set",{t*t,2*t,t+1});
    },{events,dimensions}));return curve;
}
inline JSValue baseMetadata() {
    const auto proto=curveConstructor().get("prototype");auto output=object(),keys=list(),methods=list();
    for(const auto& key:proto.ownPropertyKeys()) {
        push(keys,key);if(key.strictEquals("constructor"))continue;
        const auto fn=proto.get(key);const auto d=proto.getOwnPropertyDescriptor(key);auto row=object();
        row.set("key",key);row.set("name",fn.get("name"));row.set("length",fn.get("length"));row.set("source",call(fn,"toString"));
        row.set("flags",list({d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)}));
        row.set("constructible",fn.isConstructible());push(methods,row);
    }output.set("keys",keys);output.set("methods",methods);return output;
}
inline JSValue observe(const JSValue& request,JSValue events) {
    const auto variant=request.get("variant"),proto=curveConstructor().get("prototype");auto self=object(),result=JSValue{};
    if(variant.strictEquals("cacheHit")||variant.strictEquals("cacheMiss")||variant.strictEquals("cacheShortCircuit")||variant.strictEquals("cacheThrow")) {
        auto cache=list({0,1,2});
        getter(self,"cacheArcLengths",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"get:cache");if(c[1].strictEquals("cacheThrow"))throw JSThrownValue("cache");return c[2];},{events,variant,cache}));
        getter(self,"needsUpdate",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"get:needsUpdate");return false;},{events}));
        auto divisions=object();divisions.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"coerce:divisions");return c[1].strictEquals("cacheMiss")?3:c[1].strictEquals("cacheShortCircuit")?"2":JSValue(2);},{events,variant}));
        result=proto.get("getLengths").call(self,{divisions});
    } else if(variant.strictEquals("stringLengths")||variant.strictEquals("pointGetThrow")) {
        self.set("cacheArcLengths",nullptr);self.set("needsUpdate",true);self.set("arcLengthDivisions",3);
        getter(self,"getPoint",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
            push(c[0],"get:getPoint");if(c[1].strictEquals("pointGetThrow"))throw JSThrownValue("point");
            return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {push(c[0],list({"call:getPoint",arg(a,0)}));auto point=object();
                getter(point,"distanceTo",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"get:distanceTo");return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"call:distanceTo");return "2";},c);},c));return point;
            },c);
        },{events,variant}));result=proto.get("getLengths").call(self);
    } else if(variant.strictEquals("sampleDivisions")||variant.strictEquals("sampleReenter")||variant.strictEquals("sampleThrow")) {
        auto divisions=object();divisions.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"coerce:divisions");return 2;},{events}));
        getter(self,"getPoint",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
            push(c[0],"get:getPoint");return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
                push(c[0],list({"call:getPoint",arg(a,0)}));if(c[1].strictEquals("sampleThrow")&&arg(a,0).strictEquals(.5))throw JSThrownValue("sample");
                if(c[1].strictEquals("sampleReenter")&&arg(a,0).strictEquals(0)){auto nested=object();nested.set("getPoint",JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue{return arg(a,0);}));push(c[0],c[2].call(nested,{1}));}return arg(a,0);
            },c);
        },{events,variant,proto.get("getPoints")}));result=proto.get("getPoints").call(self,{divisions});
    } else if(variant.strictEquals("pointAt")||variant.strictEquals("pointAtThrow")||variant.strictEquals("tangentDispatch")) {
        self.set("token",object());
        getter(self,"getUtoTmapping",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"get:mapping");return JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue {push(c[0],list({"call:mapping",receiver.sameValue(c[1]),arg(a,0)}));if(c[2].strictEquals("pointAtThrow"))throw JSThrownValue("mapping");return receiver.get("token");},c);},{events,self,variant}));
        const auto key=variant.strictEquals("tangentDispatch")?"getTangent":"getPoint";
        getter(self,key,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"get:sample");return JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue {push(c[0],list({"call:sample",receiver.sameValue(c[1]),arg(a,0).sameValue(receiver.get("token")),arg(a,1).sameValue(c[3])}));return arg(a,1);},c);},{events,self,variant,self}));
        result=proto.get(variant.strictEquals("tangentDispatch")?"getTangentAt":"getPointAt").call(self,{.3,self});
        return list({result.sameValue(self),events});
    } else if(variant.strictEquals("mappingTruthy")||variant.strictEquals("mappingZero")||variant.strictEquals("mappingStringLength")||variant.strictEquals("mappingGetThrow")) {
        auto lengths=object();lengths.set("0",0);lengths.set("1",1);lengths.set("2",4);lengths.set("3",9);
        getter(lengths,"length",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"get:length");return c[1].strictEquals("mappingStringLength")?JSValue("4"):JSValue(4);},{events,variant}));
        for(const auto key:{"0","1","2","3"}){const auto value=lengths.get(key);getter(lengths,key,JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"get:"+c[1].string());if(c[2].strictEquals("mappingGetThrow"))throw JSThrownValue("index");return c[3];},{events,key,variant,value}));}
        self.set("getLengths",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"call:getLengths");return c[1];},{events,lengths}));
        auto distance=object();distance.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"coerce:distance");return 2;},{events}));
        result=proto.get("getUtoTmapping").call(self,{.5,variant.strictEquals("mappingZero")?JSValue(0):distance});
    } else if(variant.strictEquals("copyNull")) {
        auto source=object();getter(source,"arcLengthDivisions",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"get:arcLengthDivisions");return 17;},{events}));result=proto.get("copy").call(nullptr,{source});
    } else if(variant.strictEquals("cloneCustom")) {
        auto ctor=JSValue::constructor(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"construct");auto out=object();getter(out,"copy",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"get:copy");return JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue {push(c[0],list({"call:copy",receiver.sameValue(c[1]),arg(a,0).sameValue(c[2])}));return arg(a,0);},{c[0],c[1],c[2]});},{c[0],out,c[1]}));return out;},{events,self},"Replacement",0,JSValue::ConstructorKind::Callable);
        getter(self,"constructor",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"get:constructor");return c[1];},{events,ctor}));result=proto.get("clone").call(self);return result.sameValue(self);
    } else if(variant.strictEquals("ctorDefault")||variant.strictEquals("ctorProxy")||variant.strictEquals("ctorPrimitive")||variant.strictEquals("ctorSetter")||variant.strictEquals("ctorThrow")) {
        const auto ctor=curveConstructor();auto newTarget=ctor,prototype=JSValue{};
        if(!variant.strictEquals("ctorDefault")) {
            newTarget=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue {return {};},{},"Replacement",0,JSValue::ConstructorKind::Callable);
            prototype=object();
            if(variant.strictEquals("ctorSetter"))for(const auto key:{"type","arcLengthDivisions","needsUpdate","cacheArcLengths"}) {
                JSPropertyDescriptor d;d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {push(c[0],list({"set:"+c[1].string(),arg(a,0)}));return {};},{events,key});d.configurable=true;prototype.defineProperty(key,d);
            }
            newTarget.set("prototype",variant.strictEquals("ctorPrimitive")?JSValue(1):prototype);
            auto handler=object();handler.set("get",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
                if(arg(a,1).strictEquals("prototype")){push(c[0],"get:newTarget:prototype");if(c[1].strictEquals("ctorThrow"))throw JSThrownValue("prototype");}return arg(a,0).get(arg(a,1),arg(a,2));
            },{events,variant}));newTarget=JSValue::proxy(newTarget,handler);
        }
        auto instance=ctor.construct({},newTarget);result=object();result.set("instance",instance);
        result.set("prototype",instance.getPrototype().sameValue(variant.strictEquals("ctorDefault")?ctor.get("prototype"):variant.strictEquals("ctorPrimitive")?JSValue::intrinsicObjectConstructor().get("prototype"):prototype));
    } else if(variant.strictEquals("numberConstants")) {
        auto number=nativeNumberConstructor();result=object();auto entries=list(),keys=list();
        const std::vector<std::string> names={"MAX_VALUE","MIN_VALUE","NaN","NEGATIVE_INFINITY","POSITIVE_INFINITY","MAX_SAFE_INTEGER","MIN_SAFE_INTEGER","EPSILON"};
        for(const auto& key:number.ownPropertyKeys())if(key.type()==JSValue::Type::String&&std::find(names.begin(),names.end(),key.string())!=names.end())push(keys,key);
        for(const auto& name:names){const auto descriptor=number.getOwnPropertyDescriptor(name);auto row=object();row.set("name",name);
            row.set("value",number.get(name));row.set("flags",list({descriptor->writable.value_or(false),descriptor->enumerable.value_or(false),descriptor->configurable.value_or(false)}));
            try{number.set(name,1);row.set("write","success");}catch(...){row.set("write",errorValue());}row.set("delete",number.erase(name));row.set("final",number.get(name));push(entries,row);
        }result.set("keys",keys);result.set("entries",entries);
    } else if(variant.strictEquals("warn")) {
        const auto previous=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {push(c[0],list(a));return {};},{events}));
        try{result=proto.get("getPoint").call(self,{.5});}catch(...){ThreeUtils::setConsoleFunction(previous);throw;}ThreeUtils::setConsoleFunction(previous);
    } else throw std::logic_error("Unknown Curve observable variant");
    return result;
}
inline JSValue runCurve(const JSValue& request) {
    if(request.get("op").strictEquals("metadata"))return baseMetadata();auto events=list(),output=object();
    try {
        JSValue result;
        if(request.get("op").strictEquals("numeric")) {
            auto self=sampledCurve(request.get("dim").toNumber(),events);Args args;const auto input=request.get("args");
            for(double i=0;i<input.length();++i)args.push_back(input.get(JSValue(i)));auto target=JSValue{};
            if(request.get("target").toBoolean()){target=nativeVector3Constructor().construct();args.push_back(target);}
            const auto method=request.get("method");if(method.strictEquals("copy")){auto source=object();source.set("arcLengthDivisions",17);args.push_back(source);}
            result=self.get(method).call(self,args);auto row=object();row.set("result",result);row.set("self",result.sameValue(self));row.set("target",result.sameValue(target));row.set("cache",result.sameValue(self.get("cacheArcLengths")));result=row;
        } else if(request.get("op").strictEquals("cache")) {
            auto self=sampledCurve(2,events),first=call(self,"getLengths");first.set("1","edited");const auto second=call(self,"getLengths");self.set("needsUpdate",true);const auto third=call(self,"getLengths");result=object();result.set("same",first.sameValue(second));result.set("fresh",!third.sameValue(first));result.set("first",first);result.set("third",third);result.set("needsUpdate",self.get("needsUpdate"));
        } else result=observe(request,events);
        Snapshot snapshot;output.set("value",snapshot.value(result));
    } catch(...) { output.set("error",errorValue()); }
    output.set("events",events);return output;
}
}
