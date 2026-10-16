#include "core/Uniform.h"
#include "core/UniformsGroup.h"
#include "core/NativeObject.h"
#include "core/NativeArray.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Color.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "renderers/webgl/WebGLUniformsGroups.h"
#include "utils.h"
#include <iostream>
#include <cmath>
#include <limits>

namespace {
using Args=std::vector<JSValue>;using A=JsonValue::Array;using O=JsonValue::Object;
JSValue call(const JSValue& self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
JSValue array(const Args& values={}){auto out=JSValue::array();for(const auto& v:values)call(out,"push",{v});return out;}
JsonValue encode(const JSValue& v) {
    if(v.isUndefined())return "undefined";if(v.isNull())return nullptr;
    if(v.type()==JSValue::Type::Boolean)return v.toBoolean();
    if(v.type()==JSValue::Type::String)return v.string();
    if(v.type()==JSValue::Type::Number){auto n=v.number();if(std::isnan(n))return "NaN";if(std::isinf(n))return n<0?"-Infinity":"Infinity";if(n==0&&std::signbit(n))return "-0";return n;}
    if(v.hasOwn("tag"))return O{{"ref",v.get("tag").toString()}};
    for(const auto* flag:{"isVector2","isVector3","isVector4","isColor","isMatrix3","isMatrix4"})
        if(v.get(flag).strictEquals(true))return O{{"math",flag},{"values",encode(call(v,"toArray"))}};
    if(v.isHostObject("three.AnimationArray")){
        A values;for(unsigned i=0;i<v.get("length").toNumber();++i)values.push_back(encode(v.get(JSValue(i))));
        return O{{"typed",v.get("constructor").get("name").toString()},{"values",values}};
    }
    if(v.type()==JSValue::Type::Array){A values;for(unsigned i=0;i<v.length();++i)values.push_back(encode(v.get(JSValue(i))));return values;}
    O out;for(const auto& key:v.keys())out[key]=encode(v.get(key));return out;
}
JSValue command(const JSValue& receiver,const Args& args,const Args& captures) {
    auto state=captures[0];const auto name=captures[1].string();
    A row{name,receiver.sameValue(state.get(captures[2].string()))};
    for(const auto& v:args)row.push_back(encode(v));
    call(state.get("trace"),"push",{JSValue::fromJSON(row)});
    if(name=="createBuffer"){
        auto out=JSValue::object();const auto id=state.get("next").number();state.set("next",id+1);out.set("tag","buffer:"+JSValue(id).toString());return out;
    }
    return {};
}
struct Fixture {
    JSValue state=JSValue::object(),gl=JSValue::object(),info=JSValue::object(),binding=JSValue::object(),program=JSValue::object();
    UniformsGroup group;JSValue manager;
    Fixture(){
        state.set("trace",array());state.set("next",0);state.set("gl",gl);state.set("binding",binding);
        gl.set("UNIFORM_BUFFER",35345);auto render=JSValue::object();render.set("frame",0);info.set("render",render);
        group.object().set("tag","group");auto p=JSValue::object();p.set("tag","program");program.set("program",p);
        for(const auto* name:{"createBuffer","bindBuffer","bufferData","bufferSubData","deleteBuffer"})gl.set(name,JSValue::function(command,{state,name,"gl"},name));
        for(const auto* name:{"uniformBlockBinding","updateUBOMapping"})binding.set(name,JSValue::function(command,{state,name,"binding"},name));
        manager=WebGLUniformsGroups(gl,info,{},binding).object();
    }
    void update(){call(manager,"update",{group,program});}
    void frame(double frame){info.get("render").set("frame",frame);}
    JsonValue snapshot(){
        A data;auto list=group.uniforms.value();
        for(unsigned i=0;i<list.length();++i){auto u=list.get(JSValue(i));data.push_back(O{{"offset",encode(u.get("__offset"))},{"data",encode(u.get("__data"))}});}
        auto listeners=group.object().get("_listeners");auto dispose=listeners.isUndefined()?JSValue():listeners.get("dispose");
        return O{{"size",encode(group.object().get("__size"))},{"uniforms",data},{"cache",encode(group.object().get("__cache"))},
                 {"listeners",dispose.isUndefined()?0:dispose.length()},{"trace",encode(state.get("trace"))}};
    }
};
JSValue math(unsigned kind){
    if(kind==1)return nativeVector(std::make_shared<Vector2>(.25,1.25));
    if(kind==2)return nativeVector(std::make_shared<Vector3>(.25,1.25,2.25));
    if(kind==3)return nativeVector(std::make_shared<Vector4>(.25,1.25,2.25,3.25));
    if(kind==4)return nativeColor(std::make_shared<Color>(.25,1.25,2.25));
    auto values=array();for(unsigned i=0;i<(kind==5?9:16);++i)call(values,"push",{.25+i});
    auto value=kind==5?nativeMatrix(std::make_shared<Matrix3>()):nativeMatrix(std::make_shared<Matrix4>());
    call(value,"fromArray",{values});return value;
}
JsonValue scenario(unsigned kind){
    Fixture f;JSValue v=kind==0?JSValue(-0.):kind<7?math(kind):
        kind==7?array({math(2),math(2)}):NativeArray_detail::constructor(5).construct({array({1,2,3,4})});
    Uniform prefix(true),uniform(v),suffix(17);if(kind==8)uniform.object().set("type","uint");
    f.group.add(prefix);f.group.add(uniform);f.group.add(suffix);
    A steps;auto save=[&]{steps.push_back(f.snapshot());};
    f.update();save();f.update();save();f.frame(1);f.update();save();
    if(kind==0)uniform.value=3.5;
    else if(kind<=3)v.set("x",99);
    else if(kind==4)v.set("r",99);
    else if(kind<=6)v.get("elements").set("0",99);
    else if(kind==7)v.get("0").set("x",99);
    else v.set("0",99);
    f.update();save();f.frame(2);f.update();save();
    f.group.object().collectGarbage();call(f.manager,"bind",{f.group,f.program});save();
    f.group.dispose();save();f.frame(3);f.update();save();
    call(f.manager,"dispose");save();f.frame(4);f.update();save();f.group.dispose();save();
    return O{{"kind",kind},{"steps",steps}};
}
}
int main(){
    for(unsigned i=0;i<9;++i){
        try{std::cout<<scenario(i).dump()<<'\n';}
        catch(const std::exception& error){std::cerr<<"scenario "<<i<<": "<<error.what()<<'\n';return 1;}
    }
}
