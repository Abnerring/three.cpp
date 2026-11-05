#pragma once

#include "nodes/lighting/AnalyticLightNode.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"
#include "core/JSPromise.h"

#include <string>
#include <vector>

namespace AnalyticLightScenarios {
using Args = std::vector<JSValue>;
inline JSValue object() { return JSValue::object(nullptr); }
inline JSValue tagged(const char* tag) { auto o=object(); o.set("tag",tag); return o; }
inline JSValue arg(const Args& args, unsigned i) { return i<args.size()?args[i]:JSValue{}; }
inline void push(JSValue events,const JSValue& value) { events.get("push").call(events,{value}); }
inline JSValue tag(const JSValue& value) {
    if(value.isNull()) return "null";
    if(value.isUndefined()) return "undefined";
    if(!value.isObject()) return value;
    if(value.get("tag").isUndefined()) return value.get("isNode").toBoolean()?JSValue("node"):JSValue("object");
    return value.get("tag");
}
inline JSValue invoke(const char* name,const JSValue& self,const Args& args={}) {
    return nativeAnalyticLightNodeConstructor().get("prototype").get(name).call(self,args);
}
inline JSValue snapshot(const JSValue& self) {
    auto out=object();
    for(const auto* key:{"colorNode","baseColorNode","shadowNode","shadowColorNode"}) out.set(key,tag(self.get(key)));
    return out;
}
inline JSValue attempt(const char* method,const JSValue& self,const Args& args={}) {
    try { return invoke(method,self,args).isUndefined()?JSValue("undefined"):JSValue("value"); }
    catch(const JSThrownValue& e) { return tag(e.value); }
    catch(const JSTypeError&) { return "TypeError"; }
    catch(const std::exception& e) { return e.what(); }
}
inline JSValue baseState() {
    auto self=object(); self.set("colorNode",tagged("color")); self.set("baseColorNode",nullptr);
    self.set("shadowNode",nullptr); self.set("shadowColorNode",nullptr); return self;
}
inline JSValue metadata() {
    const auto ctor=nativeAnalyticLightNodeConstructor(), proto=ctor.get("prototype"); auto out=object(), methods=object();
    out.set("name",ctor.get("name")); out.set("length",ctor.get("length")); out.set("type",ctor.get("type"));
    out.set("source",ctor.get("toString").call(ctor));
    auto names=JSValue::array(); for(const auto& name:proto.ownPropertyNames())push(names,name);out.set("prototypeKeys",names);
    const auto getter=ctor.getOwnPropertyDescriptor("type")->getter.value(); out.set("typeSource",getter.get("toString").call(getter));
    for(const auto* name:{"dispose","disposeShadow","getHash","getLightVector","setupDirect","setupDirectRectArea","setupShadowNode","setupShadow","setup","update"}) {
        const auto fn=proto.get(name); auto m=object(); m.set("name",fn.get("name"));m.set("length",fn.get("length")); m.set("source",fn.get("toString").call(fn)); methods.set(name,m);
    }
    out.set("methods",methods); return out;
}
inline JSValue constructorCases() {
    auto out=JSValue::array();
    for(int variant=0;variant<5;++variant) {
        auto light=tagged("light"); light.set("uuid","uuid");
        if(variant==1) light.set("colorNode",tagged("custom"));
        if(variant==2) light.set("colorNode",0);
        if(variant==3) light.set("colorNode",nullptr);
        auto node=nativeAnalyticLightNodeConstructor().construct({variant==4?JSValue(nullptr):light});
        auto row=object(); row.set("type",node.get("type"));row.set("lightSame",node.get("light").sameValue(variant==4?JSValue(nullptr):light));
        row.set("colorTag",tag(node.get("colorNode")));row.set("uniform",node.get("colorNode").get("type").strictEquals("UniformNode"));
        row.set("group",variant==1?JSValue(nullptr):node.get("colorNode").get("groupNode").get("name"));
        row.set("flags",node.get("isAnalyticLightNode").strictEquals(true)&&node.get("isLightingNode").strictEquals(true));
        row.set("baseNull",node.get("baseColorNode").isNull());row.set("shadowNull",node.get("shadowNode").isNull());row.set("shadowColorNull",node.get("shadowColorNode").isNull());
        row.set("updateType",node.get("updateType"));row.set("hasListener",node.hasOwn("_shadowDisposeListener"));push(out,row);
    }
    return out;
}
inline JSValue listenerLifecycle() {
    auto light=tagged("light"), state=object(), events=JSValue::array();light.set("shadow",object());state.set("events",events);
    light.set("addEventListener",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue {
        auto s=c[0]; s.set("listener",arg(a,1)); push(s.get("events"),self.sameValue(c[1])&&arg(a,0).strictEquals("dispose")?JSValue("add:dispose"):JSValue("bad-add"));return {};
    },{state,light}));
    light.set("removeEventListener",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue {
        push(c[0].get("events"),self.sameValue(c[1])&&arg(a,0).strictEquals("dispose")&&arg(a,1).sameValue(c[0].get("listener"))?JSValue("remove:dispose:same"):JSValue("bad-remove"));return {};
    },{state,light}));
    auto node=nativeAnalyticLightNodeConstructor().construct({light});state.set("node",node);
    node.set("disposeShadow",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue {push(c[0].get("events"),self.sameValue(c[0].get("node"))?JSValue("shadow:lexical-self"):JSValue("bad-self"));return {};},{state}));
    node.set("dispatchEvent",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue {push(c[0].get("events"),self.sameValue(c[0].get("node"))&&arg(a,0).get("type").strictEquals("dispose")?JSValue("node:dispose"):JSValue("bad-event"));return {};},{state}));
    state.get("listener").call(object());auto result=attempt("dispose",node);auto out=object();out.set("events",events);out.set("result",result);out.set("listenerSame",node.get("_shadowDisposeListener").sameValue(state.get("listener")));out.set("source",state.get("listener").get("toString").call(state.get("listener")));return out;
}
inline JSValue disposalCases() {
    auto rows=JSValue::array();
    for(int variant=0;variant<4;++variant) {
        auto self=baseState(),events=JSValue::array(),shadow=tagged("shadow"),base=tagged("base");
        self.set("colorNode",tagged("shaded"));self.set("shadowColorNode",tagged("shaded"));self.set("shadowNode",shadow);self.set("baseColorNode",base);
        shadow.set("dispose",JSValue::function(+[](const JSValue& receiver,const Args&,const Args& c)->JSValue {
            push(c[0],receiver.sameValue(c[1])?JSValue("dispose:self"):JSValue("dispose:bad-self"));
            push(c[0],snapshot(c[2]));if(c[3].strictEquals(1))throw JSThrownValue(tagged("dispose-error"));
            if(c[3].strictEquals(2)){auto target=c[2];target.set("baseColorNode",tagged("reentrant-base"));target.set("shadowColorNode",tagged("reentrant-color"));}return {};
        },{events,shadow,self,variant}));
        if(variant==3) {self.set("shadowNode",nullptr); self.set("baseColorNode",nullptr);}
        auto row=object();row.set("result",attempt("disposeShadow",self));row.set("state",snapshot(self));row.set("events",events);push(rows,row);
    }
    return rows;
}
inline JSValue shadowCases() {
    auto rows=JSValue::array();
    for(int variant=0;variant<11;++variant) {
        auto self=baseState(),light=object(),lightShadow=object(),builder=object(),renderer=object(),map=object(),context=object(),events=JSValue::array();
        auto shadow=tagged("shadow"),shaded=tagged("shaded"),color=self.get("colorNode");
        auto custom=nativeTSLBaseBinding("float").call({}, {0.5});
        if(variant==1)lightShadow.set("shadowNode",custom);
        if(variant==9)lightShadow.set("shadowNode",nullptr);
        if(variant==10)lightShadow.set("shadowNode",0);
        light.set("shadow",lightShadow);self.set("light",light);map.set("enabled",variant==0?JSValue(false):variant==8?JSValue(0):JSValue(true));renderer.set("shadowMap",map);builder.set("renderer",renderer);builder.set("context",context);
        color.set("mul",JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue {
            push(c[0],receiver.sameValue(c[1])?JSValue("mul:self"):JSValue("mul:bad-self"));push(c[0],tag(arg(a,0)));
            if(c[3].strictEquals(4))throw JSThrownValue(tagged("mul-error"));return c[2];
        },{events,color,shaded,variant}));
        self.set("setupShadowNode",JSValue::function(+[](const JSValue& receiver,const Args&,const Args& c)->JSValue {push(c[0],receiver.sameValue(c[1])?JSValue("default:self"):JSValue("default:bad-self"));return c[2];},{events,self,shadow}));
        shadow.set("dispose",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"shadow:dispose");return {};},{events}));
        self.set("disposeShadow",nativeAnalyticLightNodeConstructor().get("prototype").get("disposeShadow"));
        if(variant==2){self.set("shadowColorNode",tagged("cached"));self.set("shadowNode",shadow);self.set("baseColorNode",tagged("base"));self.erase("light");}
        if(variant==3||variant==5||variant==6)context.set("getShadow",JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue {
            push(c[0],receiver.sameValue(c[1])&&arg(a,0).sameValue(c[2])&&arg(a,1).sameValue(c[3])?JSValue("getShadow:args"):JSValue("getShadow:bad-args"));
            push(c[0],snapshot(c[2]));if(c[4].strictEquals(5))throw JSThrownValue(tagged("getShadow-error"));
            if(c[4].strictEquals(6))invoke("disposeShadow",c[2]);return tagged("override");
        },{events,context,self,builder,variant}));
        if(variant==7){map.set("enabled",false);context.set("getShadow",true);}
        auto row=object();row.set("result",attempt("setupShadow",self,{builder}));row.set("state",snapshot(self));row.set("events",events);
        row.set("customSame",variant==1?self.get("shadowNode").sameValue(custom):false);push(rows,row);
    }
    return rows;
}
inline JSValue setupCases() {
    auto rows=JSValue::array();
    for(int variant=0;variant<7;++variant) {
        auto self=baseState(),light=object(),builder=object(),object3d=object(),lights=object(),events=JSValue::array(),state=object();
        light.set("castShadow",variant==1||variant==2);object3d.set("receiveShadow",variant==1);self.set("light",light);builder.set("object",object3d);builder.set("lightsNode",lights);
        if(variant==3){self.set("baseColorNode",tagged("base"));auto shadow=tagged("shadow");shadow.set("dispose",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"shadow:dispose");return {};},{events}));self.set("shadowNode",shadow);self.set("shadowColorNode",tagged("shaded"));}
        auto direct=tagged("direct"),rect=tagged("rect");state.set("direct",direct);state.set("rect",rect);
        for(const auto* key:{"setupDirect","setupDirectRectArea","setupShadow"})self.set(key,JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue {
            push(c[0],c[1]);push(c[0],receiver.sameValue(c[2])&&arg(a,0).sameValue(c[3]));
            push(c[0],tag(receiver.get("colorNode")));
            if(c[4].strictEquals(4)&&c[1].strictEquals("setupDirectRectArea"))throw JSThrownValue(tagged("rect-error"));
            if(c[1].strictEquals("setupShadow")){auto target=receiver;target.set("colorNode",tagged("shadowed"));return {};}
            if(c[4].strictEquals(5))return 0;
            return c[5].get(c[1].strictEquals("setupDirect")?"direct":"rect");
        },{events,key,self,builder,variant,state}));
        for(const auto* key:{"setupDirectLight","setupDirectRectAreaLight"})lights.set(key,JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue {
            push(c[0],c[1]);push(c[0],receiver.sameValue(c[2])&&arg(a,0).sameValue(c[3])&&arg(a,1).sameValue(c[4]));push(c[0],tag(arg(a,2)));
            if(c[5].strictEquals(6)&&c[1].strictEquals("setupDirectLight"))throw JSThrownValue(tagged("dispatch-error"));return {};
        },{events,key,lights,builder,self,variant}));
        auto row=object();row.set("result",attempt("setup",self,{builder}));row.set("state",snapshot(self));row.set("events",events);push(rows,row);
    }
    return rows;
}
inline JSValue defaultFactory() {
    auto light=object();light.set("shadow",object());auto self=object();self.set("light",light);
    const auto shadow=invoke("setupShadowNode",self);auto out=object();out.set("type",shadow.get("type"));out.set("isShadowNode",shadow.get("isShadowNode"));out.set("lightSame",shadow.get("light").sameValue(light));out.set("shadowSame",shadow.get("shadow").sameValue(light.get("shadow")));return out;
}
inline JSValue updateCases() {
    auto rows=JSValue::array();
    for(int variant=0;variant<7;++variant){
        auto self=object(),light=object(),source=tagged("light-color"),color=tagged("color"),returned=tagged("copy-result"),events=JSValue::array();
        light.set("color",source);self.set("color",color);self.set("light",variant==3?JSValue(nullptr):light);
        JSPropertyDescriptor intensity;intensity.configurable=true;intensity.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"get:intensity");if(c[1].strictEquals(5))throw JSThrownValue(tagged("intensity-error"));return 3;},{events,variant});light.defineProperty("intensity",intensity);
        color.set("copy",JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue {
            push(c[0],receiver.sameValue(c[1])&&arg(a,0).sameValue(c[2])?JSValue("copy:args"):JSValue("copy:bad-args"));
            if(c[4].strictEquals(2))throw JSThrownValue(tagged("copy-error"));
            if(c[4].strictEquals(4)){auto target=c[5];target.set("color",tagged("replaced-color"));}
            return c[4].strictEquals(1)?JSValue(0):c[3];
        },{events,color,source,returned,variant,self}));
        returned.set("multiplyScalar",JSValue::function(+[](const JSValue& receiver,const Args& a,const Args& c)->JSValue {push(c[0],receiver.sameValue(c[1])?JSValue("multiply:return-value"):JSValue("multiply:bad-self"));push(c[0],arg(a,0));return receiver;},{events,returned}));
        if(variant==6){JSPropertyDescriptor multiply;multiply.configurable=true;multiply.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],"get:multiplyScalar");throw JSThrownValue(tagged("multiply-error"));},{events});returned.defineProperty("multiplyScalar",multiply);}
        auto row=object();row.set("result",attempt("update",self));row.set("events",events);row.set("color",tag(self.get("color")));push(rows,row);
    }
    return rows;
}
inline JSValue all() {
    auto out=object();out.set("metadata",metadata());out.set("constructor",constructorCases());out.set("listener",listenerLifecycle());out.set("disposal",disposalCases());out.set("shadow",shadowCases());out.set("setup",setupCases());out.set("defaultFactory",defaultFactory());out.set("update",updateCases());return out;
}
} // namespace AnalyticLightScenarios
