#pragma once
#include "renderers/webgl/WebGLBindingStates.h"
#include "core/NativeArray.h"
#include <cmath>
#include <limits>

namespace BindingFixture {
using Args=std::vector<JSValue>;
using A=JsonValue::Array;
using O=JsonValue::Object;
inline void property(JSValue object,const char* name,JSValue value) {
    JSPropertyDescriptor d;d.value=std::move(value);d.writable=d.enumerable=d.configurable=true;object.defineProperty(name,d);
}
inline JSValue array(Args values) {auto out=JSValue::array();for(std::size_t i=0;i<values.size();++i)property(out,std::to_string(i).c_str(),values[i]);return out;}
inline void push(JSValue array,JSValue value) {array.get("push").call(array,{value});}
inline JSValue invoke(JSValue receiver,const char* name,Args args={}) {return receiver.get(name).call(receiver,args);}
inline JsonValue encode(const JSValue& value) {
    if(value.isUndefined())return "undefined";
    if(value.isNull())return nullptr;
    if(value.type()==JSValue::Type::Boolean)return value.toBoolean();
    if(value.type()==JSValue::Type::String)return value.string();
    if(value.type()==JSValue::Type::Number) {
        const auto n=value.number();if(std::isnan(n))return "NaN";if(std::isinf(n))return n<0?"-Infinity":"Infinity";
        if(n==0 && std::signbit(n))return "-0";return n;
    }
    if(value.isCallable())return "function";
    if(value.hasOwn("tag"))return O{{"ref",value.get("tag").toString()}};
    if(value.type()==JSValue::Type::Array) {A out;for(unsigned i=0;i<value.length();++i)out.push_back(encode(value.get(std::to_string(i))));return out;}
    O out;for(const auto& name:value.keys()) {const auto d=value.getOwnPropertyDescriptor(name);out[name]=d&&d->value?encode(*d->value):JsonValue("accessor");}return out;
}
inline JSValue frozen(const JSValue& value) {return JSValue::fromJSON(encode(value));}
inline void log(JSValue state,Args row) {push(state.get("log"),array(std::move(row)));}
template<class F> JsonValue outcome(F action) {
    try{return encode(action());}catch(const JSTypeError&){return O{{"error","TypeError"}};}
    catch(const JSRangeError&){return O{{"error","RangeError"}};}catch(const std::exception& e){return O{{"error","Error"},{"message",e.what()}};}
}
inline JSValue programAttribute(JSValue location=0,JSValue size=1) {
    auto out=JSValue::object();property(out,"location",location);property(out,"locationSize",size);return out;
}
inline JSValue attribute(const char* name="a") {
    auto out=JSValue::object(),cache=JSValue::object();property(out,"tag",std::string("attr:")+name);
    property(out,"normalized",false);property(out,"itemSize",3);property(out,"count",7);
    property(cache,"buffer",std::string("buffer:")+name);property(cache,"type",5126);property(cache,"bytesPerElement",4);
    property(out,"cache",cache);return out;
}
inline bool once(JSValue state) {
    if(state.get("busy").toBoolean() || state.get("acted").toBoolean())return false;
    state.set("acted",true);state.set("busy",true);return true;
}
inline JSValue gpu(const JSValue& self,const Args& args,const Args& captures) {
    auto state=captures[0];const auto name=captures[1].string();
    Args row{name,self.sameValue(state.get("gl"))};for(const auto& arg:args)row.push_back(frozen(arg));log(state,std::move(row));
    if(state.get("throw").strictEquals(name))throw JSRangeError("command");
    if(name=="getParameter")return state.get("max");
    if(name=="createVertexArray") {const auto next=state.get("next").toNumber()+1;state.set("next",next);return "vao:"+JSValue(next).toString();}
    const auto mode=state.get("behavior").toNumber();
    if((mode==1 && name=="bindVertexArray") || (mode==4 && name=="vertexAttribPointer")) {
        if(once(state)) {invoke(state.get("manager"),mode==1?"reset":"releaseStatesOfGeometry",mode==1?Args{}:Args{state.get("geometry")});state.set("busy",false);}
    }
    if(mode==5 && name=="deleteVertexArray" && once(state)) {
        invoke(state.get("manager"),"setup",{state.get("object"),state.get("material"),state.get("program"),state.get("geometry"),nullptr});state.set("busy",false);
    }
    return {};
}
inline JSValue getAttributes(const JSValue& self,const Args&,const Args& captures) {
    auto state=captures[0];log(state,{"getAttributes",self.sameValue(state.get("program"))});
    const auto n=state.get("attributesCalls").toNumber()+1;state.set("attributesCalls",n);
    if(state.get("throw").strictEquals("getAttributes"))throw JSRangeError("program");
    if(state.get("behavior").strictEquals(2) && once(state)) {invoke(state.get("manager"),"reset");state.set("busy",false);}
    if(state.get("behavior").strictEquals(6))return n==2?state.get("alternate"):state.get("programAttributes");
    return state.get("programAttributes");
}
inline JSValue attributeCommand(const JSValue& self,const Args& args,const Args& captures) {
    auto state=captures[0];const auto name=captures[1].string();
    Args row{name,self.sameValue(state.get("attributeManager"))};for(const auto& arg:args)row.push_back(frozen(arg));log(state,std::move(row));
    if(state.get("throw").strictEquals(name))throw JSRangeError("attribute");
    if(name=="get") {
        if(state.get("behavior").strictEquals(3) && once(state)) {invoke(state.get("manager"),"reset");state.set("busy",false);}
        return args[0].get("cache");
    }
    return {};
}
struct Fixture {
    JSValue state=JSValue::object(nullptr),gl=JSValue::object(),attributeManager=JSValue::object(),
            object=JSValue::object(),material=JSValue::object(),program=JSValue::object(),geometry=JSValue::object(),
            programAttributes=JSValue::object(),geometryAttributes=JSValue::object(),source=attribute(),manager;
    Fixture(JSValue max=8) {
        property(state,"log",JSValue::array());property(state,"max",max);property(state,"next",0);
        property(state,"attributesCalls",0);property(state,"busy",false);property(state,"acted",false);
        property(state,"behavior",0);property(state,"throw","");
        for(const auto& [name,value]:std::initializer_list<std::pair<const char*,JSValue>>{
            {"gl",gl},{"attributeManager",attributeManager},{"object",object},{"material",material},
            {"program",program},{"geometry",geometry},{"programAttributes",programAttributes}})property(state,name,value);
        for(const auto* name:{"getParameter","createVertexArray","bindVertexArray","deleteVertexArray","enableVertexAttribArray",
             "disableVertexAttribArray","vertexAttribDivisor","bindBuffer","vertexAttribPointer","vertexAttribIPointer",
             "vertexAttrib1fv","vertexAttrib2fv","vertexAttrib3fv","vertexAttrib4fv"})property(gl,name,JSValue::function(gpu,{state,name}));
        property(gl,"MAX_VERTEX_ATTRIBS",34921);property(gl,"ARRAY_BUFFER",34962);property(gl,"ELEMENT_ARRAY_BUFFER",34963);
        property(gl,"INT",5124);property(gl,"UNSIGNED_INT",5125);
        property(attributeManager,"get",JSValue::function(attributeCommand,{state,"get"}));
        property(attributeManager,"update",JSValue::function(attributeCommand,{state,"update"}));
        property(object,"id",4);property(object,"isInstancedMesh",false);property(material,"wireframe",false);
        property(program,"id",2);property(program,"getAttributes",JSValue::function(getAttributes,{state}));
        property(geometry,"id",1);property(geometry,"attributes",geometryAttributes);
        property(programAttributes,"a",programAttribute());property(geometryAttributes,"a",source);
        manager=WebGLBindingStates(gl,attributeManager).object();property(state,"manager",manager);
    }
    JSValue setup(JSValue index=nullptr) {return invoke(manager,"setup",{object,material,program,geometry,index});}
    void clearLog() {state.get("log").set("length",0);}
    JsonValue trace() const {return encode(state.get("log"));}
    JsonValue maxCount() const {return encode(geometry.get("_maxInstanceCount"));}
};
inline Args scalars() {return {{},nullptr,false,true,0,-0.0,-1,1,2,3,4,1.5,"0","1","2","",std::numeric_limits<double>::quiet_NaN()};}
inline JsonValue attributeCase(unsigned mode,unsigned locationSize,unsigned field,unsigned valueIndex) {
    Fixture f;const auto values=scalars();auto pa=f.programAttributes.get("a");pa.set("locationSize",values[locationSize]);
    if(mode==1 || mode==3) {
        auto data=JSValue::object();property(data,"stride",6);property(data,"count",11);property(data,"meshPerAttribute",2);
        property(data,"isInstancedInterleavedBuffer",mode==3);property(f.source,"data",data);
        property(f.source,"isInterleavedBufferAttribute",true);property(f.source,"offset",1);
    }
    if(mode==2) {property(f.source,"isInstancedBufferAttribute",true);property(f.source,"meshPerAttribute",2);}
    if(mode==4)f.source.set("cache",JSValue{});
    switch(field) {
    case 0:pa.set("location",values[valueIndex]);break;
    case 1:f.source.set("itemSize",values[valueIndex]);break;
    case 2:f.source.set("normalized",values[valueIndex]);break;
    case 3:f.source.get("cache").set("type",values[valueIndex]);break;
    case 4:f.source.get("cache").set("bytesPerElement",values[valueIndex]);break;
    case 5:property(f.source,"gpuType",values[valueIndex]);break;
    case 6:if(mode==2)f.source.set("meshPerAttribute",values[valueIndex]);else if(mode==3)f.source.get("data").set("meshPerAttribute",values[valueIndex]);break;
    case 7:if(mode==1 || mode==3)f.source.set("offset",values[valueIndex]);break;
    }
    const auto result=outcome([&]{return f.setup();});
    const auto repeated=outcome([&]{return f.setup();});
    return O{{"kind","attribute"},{"mode",mode},{"locationSize",locationSize},{"field",field},{"value",valueIndex},
             {"result",result},{"repeated",repeated},{"max",f.maxCount()},{"trace",f.trace()}};
}
inline JsonValue sequenceCase(unsigned variant) {
    Fixture f;A results;auto index=attribute("index");
    for(unsigned step=0;step<6;++step) {
        if(variant==0)f.geometry.set("id",step%2+1);
        if(variant==1)f.program.set("id",step%2+2);
        if(variant==2)f.material.set("wireframe",step%2==1);
        if(variant==3) {f.object.set("isInstancedMesh",true);f.object.set("id",step%2+4);}
        if(variant==4)f.object.set("id",step%2+4);
        if(variant==5) {f.material.set("wireframe",step%2?JSValue("true"):JSValue(false));f.object.set("isInstancedMesh","true");}
        if(variant==6 && step==2)f.geometryAttributes.set("a",attribute("replacement"));
        if(variant==7 && step==2) {property(f.source,"data",JSValue::object());}
        if(variant==8 && step==2) {f.source.set("itemSize",4);f.source.set("normalized",true);property(f.source,"version",7);}
        if(variant==9 && step==2)property(f.programAttributes,"b",programAttribute(3,1));
        if(variant==9 && step==4)f.programAttributes.erase("b");
        if(variant==10 && step==2)f.programAttributes.get("a").set("location",-1);
        if(variant==10 && step==4)f.programAttributes.get("a").set("location",1);
        if(variant==11 && step==2)f.geometryAttributes.erase("a");
        if(variant==11 && step==4)f.geometryAttributes.set("a",f.source);
        if(variant==12 && step==2)invoke(f.manager,"reset");
        if(variant==13 && step==2)invoke(f.manager,"resetDefaultState");
        const auto nextIndex=variant==14?(step%2?JSValue(nullptr):index):variant==15?index:JSValue(nullptr);
        results.push_back(outcome([&]{return f.setup(nextIndex);}));
    }
    return O{{"kind","sequence"},{"variant",variant},{"results",results},{"trace",f.trace()}};
}
inline JsonValue defaultCase(unsigned length,unsigned mode) {
    Fixture f;f.geometryAttributes.erase("a");auto value=JSValue::array();
    for(unsigned i=0;i<length;++i)push(value,i+.25);
    if(mode==1) {value=JSValue::object();property(value,"length",std::to_string(length));property(value,"x",1);}
    if(mode==2)value=nullptr;
    if(mode==3)value=JSValue{};
    auto defaults=JSValue::object();property(defaults,"a",value);property(f.material,"defaultAttributeValues",defaults);
    const auto first=outcome([&]{return f.setup();});defaults.set("a",array({9,8,7,6}));
    const auto second=outcome([&]{return f.setup();});invoke(f.manager,"reset");
    const auto forced=outcome([&]{return f.setup();});
    return O{{"kind","default"},{"length",length},{"mode",mode},{"first",first},{"second",second},{"forced",forced},{"trace",f.trace()}};
}
inline JsonValue releaseCase(unsigned variant) {
    Fixture f;A results;
    for(unsigned g=0;g<2;++g)for(unsigned o=0;o<2;++o)for(unsigned p=0;p<2;++p)for(bool wire:{false,true}) {
        f.geometry.set("id",g+1);f.object.set("id",o+4);f.object.set("isInstancedMesh",true);
        f.program.set("id",p+2);f.material.set("wireframe",wire);f.setup();
    }
    f.clearLog();const char* names[]={"releaseStatesOfGeometry","releaseStatesOfProgram","releaseStatesOfObject","dispose"};
    const auto name=names[variant%4];const auto value=variant%4==0?f.geometry:variant%4==1?f.program:f.object;
    if(variant>=4)f.state.set("behavior",5);
    results.push_back(outcome([&]{return invoke(f.manager,name,variant%4==3?Args{}:Args{value});}));
    results.push_back(outcome([&]{return f.setup();}));
    results.push_back(outcome([&]{return invoke(f.manager,"dispose");}));
    return O{{"kind","release"},{"variant",variant},{"results",results},{"trace",f.trace()}};
}
inline JsonValue behaviorCase(unsigned behavior,unsigned fail) {
    Fixture f;f.state.set("behavior",behavior);
    const char* failures[]={"","createVertexArray","bindVertexArray","getAttributes","get","update","enableVertexAttribArray",
                           "vertexAttribDivisor","bindBuffer","vertexAttribPointer","deleteVertexArray"};
    f.state.set("throw",failures[fail]);
    if(fail==7) {property(f.source,"isInstancedBufferAttribute",true);property(f.source,"meshPerAttribute",2);}
    if(behavior==6) {auto alternate=JSValue::object();property(alternate,"a",programAttribute(-1,1));property(f.state,"alternate",alternate);}
    auto index=fail==5?attribute("index"):JSValue(nullptr);
    const auto first=outcome([&]{return f.setup(index);});f.state.set("throw","");
    const auto second=outcome([&]{return f.setup(index);});
    if(fail==10)f.state.set("throw",failures[fail]);
    const auto release=outcome([&]{return invoke(f.manager,"dispose");});f.state.set("throw","");
    const auto final=outcome([&]{return f.setup();});
    return O{{"kind","behavior"},{"behavior",behavior},{"fail",fail},{"first",first},{"second",second},{"release",release},{"final",final},{"trace",f.trace()}};
}
inline JsonValue manualCase(unsigned max,unsigned index) {
    const auto values=scalars();Fixture f(values[max]);A results;
    for(const auto* name:{"enableAttribute","enableAttribute","disableUnusedAttributes","initAttributes","disableUnusedAttributes","enableAttribute","reset","disableUnusedAttributes"})
        results.push_back(outcome([&]{return invoke(f.manager,name,std::string(name)=="enableAttribute"?Args{values[index]}:Args{});}));
    return O{{"kind","manual"},{"max",max},{"index",index},{"results",results},{"trace",f.trace()}};
}
inline void watch(Fixture& f,JSValue object,const char* name,const char* label,unsigned action=0) {
    const auto value=object.get(name);JSPropertyDescriptor d;d.enumerable=d.configurable=true;
    d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        auto state=c[0];log(state,{"property",c[1],c[2]});const auto action=c[4].toNumber();
        if(action==1)state.get("gl").set("ARRAY_BUFFER",99);
        if(action==2 && once(state)) {invoke(state.get("manager"),"reset");state.set("busy",false);}
        if(action==3)state.get("programAttributes").erase("b");
        return c[3];
    },{f.state,label,name,value,action});object.defineProperty(name,d);
}
inline JSValue numberValue(Fixture& f,const char* label,JSValue result,unsigned action=0) {
    auto object=JSValue::object();property(object,"tag",label);
    object.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue {
        auto state=c[0];log(state,{"convert",c[1],args[0]});
        if(c[3].strictEquals(1))property(state.get("programAttributes").get("a"),"locationSize",2);
        if(c[3].strictEquals(2))throw JSRangeError("coercion");
        if(c[3].strictEquals(3) && once(state)) {invoke(state.get("manager"),"reset");state.set("busy",false);}
        return c[2];
    },{f.state,label,result,action}));return object;
}
inline JsonValue getterCase(unsigned variant) {
    Fixture f;
    if(variant==0) {
        watch(f,f.geometry,"attributes","geometry");watch(f,f.geometry,"id","geometry");watch(f,f.program,"id","program");
        watch(f,f.program,"getAttributes","program");watch(f,f.material,"wireframe","material");watch(f,f.material,"defaultAttributeValues","material");
        watch(f,f.programAttributes,"a","layout");watch(f,f.programAttributes.get("a"),"location","layout-a");watch(f,f.programAttributes.get("a"),"locationSize","layout-a");
        for(const char* name:{"normalized","itemSize","gpuType","data","isInterleavedBufferAttribute","isInstancedBufferAttribute"})watch(f,f.source,name,"source");
        for(const char* name:{"buffer","type","bytesPerElement"})watch(f,f.source.get("cache"),name,"cache");
    }
    if(variant==1)watch(f,f.gl,"bindBuffer","gl",1);
    if(variant==2)watch(f,f.gl,"vertexAttribPointer","gl",2);
    if(variant==3)watch(f,f.attributeManager,"get","attributes",2);
    if(variant==4)watch(f,f.program,"getAttributes","program",2);
    if(variant==5) {f.geometry.set("id",numberValue(f,"geometry-id","1"));f.program.set("id",numberValue(f,"program-id","2"));}
    if(variant==6) {f.programAttributes.get("a").set("location",numberValue(f,"location",1));}
    if(variant==7) {f.source.set("itemSize",numberValue(f,"size","4"));}
    if(variant==8) {f.source.set("itemSize",numberValue(f,"size",4,1));watch(f,f.programAttributes.get("a"),"locationSize","layout-a");}
    if(variant==9) {f.source.set("itemSize",numberValue(f,"size",4,2));}
    if(variant==10) {f.geometry.set("id",numberValue(f,"geometry-id","1",3));}
    if(variant==11) {property(f.source,"isInterleavedBufferAttribute",true);auto data=JSValue::object();property(data,"stride",numberValue(f,"stride","8"));property(data,"count",4);property(f.source,"data",data);property(f.source,"offset",numberValue(f,"offset","1"));}
    if(variant==12) {watch(f,f.gl,"getParameter","gl");watch(f,f.gl,"MAX_VERTEX_ATTRIBS","gl");f.manager=WebGLBindingStates(f.gl,f.attributeManager).object();f.state.set("manager",f.manager);}
    if(variant==13) {property(f.source,"isInstancedBufferAttribute",true);property(f.source,"meshPerAttribute",numberValue(f,"divisor",2));f.object.set("isInstancedMesh","true");}
    const auto first=outcome([&]{return f.setup();});const auto repeated=outcome([&]{return f.setup();});
    return O{{"kind","getter"},{"variant",variant},{"first",first},{"repeated",repeated},{"max",f.maxCount()},{"trace",f.trace()}};
}
inline JsonValue enumerationCase(unsigned variant) {
    Fixture f;auto inherited=JSValue::object();property(inherited,"b",programAttribute(2,1));
    auto b=attribute("b");property(f.geometryAttributes,"b",b);
    if(variant==0)f.programAttributes.setPrototype(inherited);
    if(variant==1) {f.programAttributes.setPrototype(inherited);JSPropertyDescriptor d;d.value=programAttribute(3,1);d.enumerable=false;d.configurable=true;f.programAttributes.defineProperty("b",d);}
    if(variant==2) {property(f.programAttributes,"b",programAttribute(2,1));watch(f,f.programAttributes,"a","layout",3);}
    if(variant==3) {f.programAttributes.setPrototype(inherited);watch(f,inherited,"b","inherited");}
    if(variant==4) {property(f.programAttributes,"2",programAttribute(3,1));property(f.programAttributes,"1",programAttribute(2,1));property(f.geometryAttributes,"1",attribute("1"));property(f.geometryAttributes,"2",attribute("2"));}
    if(variant==5)property(f.programAttributes,"b",nullptr);
    if(variant==6)f.state.set("programAttributes",JSValue{});
    if(variant==7)f.state.set("programAttributes",nullptr);
    if(variant==8)f.state.set("programAttributes","a");
    if(variant==9) {auto values=JSValue::array();push(values,programAttribute());f.state.set("programAttributes",values);property(f.geometryAttributes,"0",f.source);}
    if(variant==10) {auto values=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};});property(values,"a",programAttribute());f.state.set("programAttributes",values);}
    const auto first=outcome([&]{return f.setup();});const auto repeated=outcome([&]{return f.setup();});
    return O{{"kind","enumeration"},{"variant",variant},{"first",first},{"repeated",repeated},{"trace",f.trace()}};
}
inline void dynamicMutation(JSValue object,JSValue prototype,unsigned variant) {
    auto define=[](JSValue target,const char* key,JSValue value,bool enumerable) {
        JSPropertyDescriptor d;d.value=std::move(value);d.writable=d.configurable=true;d.enumerable=enumerable;target.defineProperty(key,d);
    };
    if(variant==0)property(prototype,"b",programAttribute(2));
    if(variant==1)property(object,"d",programAttribute(4));
    if(variant==2||variant==14){JSPropertyDescriptor d;d.enumerable=variant==14;object.defineProperty("b",d);}
    if(variant==3||variant==10){JSPropertyDescriptor d;d.enumerable=variant==10;prototype.defineProperty("b",d);}
    if(variant==4||variant==5||variant==6)object.erase("b");
    if(variant==7)prototype.erase("b");
    if(variant==8)define(object,"b",programAttribute(3),false);
    if(variant==9)property(object,"b",programAttribute(3));
    if(variant==11){auto replacement=JSValue::object();property(replacement,"b",programAttribute(3));property(replacement,"d",programAttribute(4));object.setPrototype(replacement);}
    if(variant==12)object.setPrototype(nullptr);
    if(variant==13){auto replacement=JSValue::object();define(replacement,"b",programAttribute(3),false);object.setPrototype(replacement);}
    if(variant==15){object.erase("b");property(object,"b",programAttribute(3));}
}
inline JsonValue dynamicEnumerationCase(unsigned variant,unsigned phase) {
    Fixture f;auto prototype=JSValue::object(),second=JSValue::object(prototype),a=programAttribute();
    property(prototype,"b",programAttribute(2));property(prototype,"c",programAttribute(3));
    if(variant==0)prototype.erase("b");
    if(variant==2||variant==5||variant==6||variant==15)property(second,"b",programAttribute(2));
    if(variant==4||variant==14){JSPropertyDescriptor d;d.value=programAttribute(2);d.writable=d.configurable=true;d.enumerable=false;second.defineProperty("b",d);}
    if(variant==6||variant==10){JSPropertyDescriptor d;d.enumerable=false;prototype.defineProperty("b",d);}
    property(f.state,"dynamicCalls",0);property(f.state,"dynamicDone",false);
    JSPropertyDescriptor descriptor;descriptor.enumerable=descriptor.configurable=true;
    descriptor.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        log(c[0],{"dynamic-a"});auto state=c[0];
        if(!state.get("dynamicDone").toBoolean()){state.set("dynamicDone",true);dynamicMutation(c[1],c[2],static_cast<unsigned>(c[4].toNumber()));}
        return c[3];
    },{f.state,second,prototype,a,variant});second.defineProperty("a",descriptor);
    // Keep a first in the initial own-key order even when b is already present.
    if(second.hasOwn("b")){const auto d=*second.getOwnPropertyDescriptor("b");second.erase("b");second.defineProperty("b",d);}
    f.program.set("getAttributes",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue {
        auto state=c[0];const auto n=state.get("dynamicCalls").toNumber()+1;state.set("dynamicCalls",n);log(state,{"dynamic-attributes",self.sameValue(c[3]),n});
        return n<c[4].toNumber()?c[1]:c[2];
    },{f.state,f.programAttributes,second,f.program,phase}));
    for(const char* name:{"b","c","d"})property(f.geometryAttributes,name,attribute(name));
    const auto first=outcome([&]{return f.setup();});const auto firstTrace=f.trace();f.clearLog();
    const auto secondResult=outcome([&]{return f.setup();});const auto secondTrace=f.trace();f.clearLog();
    invoke(f.manager,"reset");const auto forced=outcome([&]{return f.setup();});
    return O{{"kind","dynamic-enumeration"},{"variant",variant},{"phase",phase},{"first",first},{"firstTrace",firstTrace},
             {"second",secondResult},{"secondTrace",secondTrace},{"forced",forced},{"forcedTrace",f.trace()}};
}
inline JsonValue invalidCase(unsigned variant) {
    Fixture f;
    if(variant==0)f.geometry.set("attributes",nullptr);
    if(variant==1)f.geometryAttributes.set("a",nullptr);
    if(variant==2)f.source.set("cache",nullptr);
    if(variant==3) {f.geometryAttributes.erase("a");property(f.material,"defaultAttributeValues",nullptr);}
    if(variant==4)f.program.set("getAttributes",0);
    if(variant==5)f.gl.set("vertexAttribPointer",0);
    if(variant==6)f.attributeManager.set("get",0);
    if(variant==7)f.gl.set("bindVertexArray",0);
    const auto first=outcome([&]{return variant==8?invoke(f.manager,"setup"):f.setup(variant==9?JSValue{}:JSValue(nullptr));});
    const auto repeated=outcome([&]{return f.setup();});
    return O{{"kind","invalid"},{"variant",variant},{"first",first},{"repeated",repeated},{"trace",f.trace()}};
}
}
