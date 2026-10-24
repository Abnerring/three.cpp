#include "animation/AnimationUtils.h"
#include "core/NativeArray.h"
#include "math/Matrix2.h"
#include "renderers/common/Binding.h"
#include "renderers/common/Buffer.h"
#include "renderers/common/BufferUtils.h"
#include "renderers/common/UniformBuffer.h"
#include "renderers/common/Uniform.h"
#include "renderers/common/nodes/NodeUniform.h"
#include "nodes/core/UniformNode.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue array(const Args& args={}){auto a=JSValue::array();for(const auto& x:args)a.get("push").call(a,{x});return a;}
JSValue call(const JSValue& self,const char* key,const Args& args={}){return self.get(key).call(self,args);}
JSValue names(const JSValue& value){Args keys;for(const auto& key:value.ownPropertyNames())keys.emplace_back(key);return array(keys);}
JSValue normal(const JSValue& value){
    if(value.isUndefined())return "#undefined";
    if(value.type()==JSValue::Type::Number){const auto n=value.number();if(std::isnan(n))return "#NaN";if(std::isinf(n))return n>0?"#Infinity":"#-Infinity";if(n==0&&std::signbit(n))return "#-0";return value;}
    if(!value.isObject())return value;
    for(const auto* type:{"Vector2","Vector3","Vector4","Color","Matrix2","Matrix3","Matrix4"})if(value.get(std::string("is")+type).strictEquals(true)){
        auto result=JSValue::object();result.set("math",type);Args values;const auto name=std::string(type);
        if(name.starts_with("Matrix")){const auto e=value.get("elements");for(unsigned i=0;i<e.length();++i)values.push_back(normal(e.get(std::to_string(i))));}
        else if(name=="Color"){for(const auto* key:{"r","g","b"})values.push_back(normal(value.get(key)));}
        else{for(const auto* key:{"x","y","z","w"}){if(std::string(key)=="z"&&name=="Vector2")break;if(std::string(key)=="w"&&name!="Vector4")break;values.push_back(normal(value.get(key)));}}
        result.set("values",array(values));return result;
    }
    if(value.type()==JSValue::Type::Array){Args values;for(unsigned i=0;i<value.length();++i)values.push_back(normal(value.get(std::to_string(i))));return array(values);}
    auto result=JSValue::object();for(const auto& key:value.keys())result.set(key,normal(value.get(key)));return result;
}
void row(const std::string& kind,const JSValue& value){auto r=JSValue::object();r.set("kind",kind);r.set("value",value);std::cout<<r.toJSON()->dump()<<'\n';}
JSValue snapshot(const JSValue& value){auto out=JSValue::object();out.set("keys",names(value));for(const auto& key:value.ownPropertyNames())out.set(key,normal(value.get(key)));return out;}
void getter(JSValue object,const std::string& key,const JSValue& fn){JSPropertyDescriptor d;d.getter=fn;d.configurable=d.enumerable=true;object.defineProperty(key,d);}
JSValue attempt(const JSValue& function,const JSValue& receiver,const Args& args){try{return normal(function.call(receiver,args));}catch(const JSTypeError&){return "TypeError";}catch(const JSRangeError&){return "RangeError";}catch(const std::exception&){return "Error";}}
JSValue arrayFromCase(int mode){
    auto state=JSValue::object();state.set("events",array());state.set("count",0);state.set("mode",mode);auto source=JSValue::object();
    auto iterator=JSValue::object();state.set("iterator",iterator);
    getter(iterator,"next",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{
        const auto state=c[0];call(state.get("events"),"push",{"get next"});
        return JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{
            auto state=c[0];const auto n=state.get("count").number();state.set("count",n+1);call(state.get("events"),"push",{array({"next",n})});
            if(state.get("mode").strictEquals(4))throw JSTypeError("step");
            auto result=JSValue::object();getter(result,"done",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0].get("events"),"push",{"done"});return c[1].number()>=2;},{state,n}));
            getter(result,"value",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0].get("events"),"push",{"value"});if(c[0].get("mode").strictEquals(5))throw JSTypeError("value");return c[1];},{state,n}));return result;
        },{state});
    },{state}));
    getter(iterator,"return",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{
        const auto state=c[0];call(state.get("events"),"push",{"get return"});return JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0].get("events"),"push",{"return"});if(c[0].get("mode").strictEquals(3))throw JSRangeError("close");return JSValue::object();},{state});
    },{state}));
    JSPropertyDescriptor descriptor;descriptor.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{
        const auto state=c[0];call(state.get("events"),"push",{"get iterator"});
        if(state.get("mode").strictEquals(6))return nullptr;
        return JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0].get("events"),"push",{"iterator"});return c[0].get("iterator");},{state});
    },{state});descriptor.configurable=true;source.defineProperty(JSValue::wellKnownSymbol("iterator"),descriptor);
    getter(source,"length",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0].get("events"),"push",{"length"});return 3;},{state}));
    getter(source,"0",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0].get("events"),"push",{"get 0"});return 4;},{state}));
    auto map=JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{
        const auto state=c[0];call(state.get("events"),"push",{array({"map",normal(args[0]),args[1],self.strictEquals(state)})});
        if(state.get("mode").number()>=2&&state.get("mode").number()<=3)throw JSTypeError("map");return args[0];
    },{state});
    auto ctor=JSValue::constructor([](const JSValue& receiver,const Args& args,const Args& c)->JSValue{call(c[0].get("events"),"push",{array({"construct",array(args)})});return receiver;},{state},"Target",0);
    const auto intrinsic=JSValue::intrinsicArrayConstructor();auto result=attempt(intrinsic.get("from"),mode==0?intrinsic:ctor,{source,mode==7?JSValue(2):map,state});
    return array({result,state.get("events")});
}
}
int main(){try{
    auto keeper=JSValue::object();
    const auto uniforms=nativeRendererUniforms(),nodes=nativeNodeUniforms();
    for(const auto* type:{"Number","Vector2","Vector3","Vector4","Color","Matrix2","Matrix3","Matrix4"}){
        const auto ctor=uniforms.get(std::string(type)+"Uniform");auto u=ctor.construct();row(std::string(type)+" defaults",snapshot(u));
        auto another=ctor.construct();row(std::string(type)+" independent",u.get("value").isObject()?JSValue(!u.get("value").strictEquals(another.get("value"))):JSValue(true));
        row(std::string(type)+" constructor",array({ctor.get("name"),ctor.get("length"),ctor.getPrototype().get("name"),names(ctor.get("prototype")),names(ctor.getPrototype().get("prototype"))}));
        auto value=JSValue::object();value.set("v",1);row(std::string(type)+" setter return",normal(call(u,"setValue",{value})));row(std::string(type)+" setter identity",call(u,"getValue").strictEquals(value));
        row(std::string(type)+" null",snapshot(ctor.construct({"n",nullptr})));
        auto node=JSValue::object();node.set("name","node");node.set("value",value);node.set("type","test");auto n=nodes.get(std::string(type)+"NodeUniform").construct({node});
        row(std::string(type)+" node initial",array({n.get("name"),n.get("boundary"),n.get("itemSize"),n.get("value").strictEquals(value),call(n,"getValue").strictEquals(value),call(n,"getType")}));
        node.set("value",42);node.set("type","changed");call(n,"setValue",{3});row(std::string(type)+" node live",array({n.get("value"),call(n,"getValue"),call(n,"getType"),n.get("nodeUniform").strictEquals(node)}));
    }
    for(const auto& [name,ctor]:std::initializer_list<std::pair<const char*,JSValue>>{{"Binding",nativeBindingConstructor()},{"Buffer",nativeRendererBufferConstructor()},{"UniformBuffer",nativeUniformBufferConstructor()}}){
        auto instance=ctor.construct();row(std::string(name)+" defaults",snapshot(instance));
        auto clone=call(instance,"clone");row(std::string(name)+" clone",array({snapshot(clone),!clone.strictEquals(instance),clone.getPrototype().strictEquals(instance.getPrototype())}));
        if(std::string(name)!="Binding"){
            const auto buffer=NativeArray_detail::constructor(6).construct({3});auto b=ctor.construct({"test",buffer});call(b,"addUpdateRange",{1,2});call(b,"addUpdateRange",{3,4});auto c=call(b,"clone");
            row(std::string(name)+" buffer",array({b.get("byteLength"),b.get("bytesPerElement"),normal(b.get("updateRanges")),c.get("_buffer").strictEquals(buffer),c.get("updateRanges").strictEquals(b.get("updateRanges")),call(b,"update")}));
            call(b,"clearUpdateRanges");call(b,"release");row(std::string(name)+" released",array({b.get("buffer"),normal(b.get("updateRanges")),c.get("buffer").strictEquals(buffer)}));
        }
    }
    const auto nan=JSValue(std::numeric_limits<double>::quiet_NaN()),inf=JSValue(std::numeric_limits<double>::infinity());
    for(const auto& value:Args{JSValue{},nullptr,false,true,0,-0.0,-1,1,3.5,15,16,17,4294967299.0,"3","","bad",nan,inf,-inf.toNumber()}){
        auto b=nativeBindingConstructor().construct();call(b,"setVisibility",{value});call(b,"setVisibility",{8});row("visibility",normal(b.get("visibility")));
        for(const auto* method:{"getFloatLength","getStrideLength","getVectorLength"})row(method,normal(nativeBufferUtils().get(method).call({}, {value})));
    }
    for(const auto* method:{"getFloatLength","getStrideLength","getVectorLength"}){
        auto state=JSValue::object();state.set("events",array());state.set("count",0);auto object=JSValue::object();
        object.set(JSValue::wellKnownSymbol("toPrimitive"),JSValue::function([](const JSValue&,const Args& a,const Args& c)->JSValue{auto state=c[0];const auto n=state.get("count").number();state.set("count",n+1);call(state.get("events"),"push",{array({a[0],n})});return n==0?JSValue(5):JSValue("7");},{state}));
        const auto result=nativeBufferUtils().get(method).call({}, {object,object});row(std::string(method)+" coercion",array({normal(result),state.get("events")}));
    }
    const auto matrix=nativeMatrix2Constructor();
    for(const auto& args:std::vector<Args>{{},{JSValue{},4},{nullptr},{1,2,3,4},{"a",false,nullptr}}){auto m=matrix.construct(args);row("Matrix2 constructor",normal(m));}
    for(const auto& offset:Args{JSValue{},0,1,"1",-1,0.5,nullptr}){auto m=matrix.construct();auto a=JSValue::object();for(int i=0;i<15;++i)a.set(std::to_string(i),i+10);call(m,"fromArray",{a,offset});row("Matrix2 fromArray",normal(m));}
    auto m=matrix.construct({1,2,3,4});row("Matrix2 identity",array({call(m,"identity").strictEquals(m),normal(m)}));
    row("Matrix2 metadata",array({matrix.get("length"),names(matrix.get("prototype")),matrix.get("prototype").get("fromArray").get("length")}));
    // Literal range fields must bypass inherited setters.
    auto proto=JSValue::object().getPrototype();JSPropertyDescriptor d;d.setter=JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{throw JSTypeError("inherited range setter");});d.configurable=true;proto.defineProperty("start",d);
    auto b=nativeRendererBufferConstructor().construct();call(b,"addUpdateRange",{3,5});proto.erase("start");row("range literal",normal(b.get("updateRanges")));
    const auto intrinsic=JSValue::intrinsicArrayConstructor();
    for(const auto& value:Args{JSValue{},nullptr,false,4,"a\xf0\x9f\x98\x80",array({1,JSValue{},3})})row("Array.from",attempt(intrinsic.get("from"),intrinsic,{value}));
    for(int mode=0;mode<8;++mode)row("Array.from protocol",arrayFromCase(mode));
    for(const auto& value:Args{JSValue{},nullptr,false,3,"a\xf0\x9f\x98\x80",array({1,2})})row("Object.values",attempt(JSValue::intrinsicObjectConstructor().get("values"),{}, {value}));
    auto object=JSValue::object();object.set("a",1);getter(object,"b",JSValue::function([](const JSValue& receiver,const Args&,const Args&)->JSValue{auto self=receiver;self.erase("c");self.set("d",4);return 2;}));object.set("c",3);object.set(JSValue::symbol("skip"),5);row("Object.values mutation",array({call(JSValue::intrinsicObjectConstructor(),"values",{object}),names(object)}));
    for(const auto& value:Args{JSValue{},nullptr,0,true,2.5,"float","bool","vec2","mat2","mat3","mat4"})for(const auto& type:Args{JSValue{},false,"uint"}){
        const auto u=nativeUniformFactory().call({}, {value,type});row("uniform factory",array({normal(u.get("value")),normal(u.get("nodeType")),u.get("groupNode").get("name")}));
    }
    for(int mode=0;mode<4;++mode){
        auto events=array(),value=JSValue::object();value.set("isNode",true);getter(value,"value",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"push",{"value"});return 3;},{events}));
        getter(value,"traverse",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{
            call(c[0],"push",{"get traverse"});return JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{
                call(c[0],"push",{"traverse"});if(c[1].strictEquals(3))throw JSTypeError("traverse");
                for(const auto& flag:Args{false,1,true}){auto node=JSValue::object();node.set("isConstNode",flag);node.set("value",7);args[0].call({}, {node});}return {};
            },c);
        },{events,mode}));
        if(mode==1)value.set("nodeType","vec3");if(mode==2)value.set("convertTo","int");
        JSValue result;try{const auto u=nativeUniformFactory().call({}, {value});result=array({normal(u.get("value")),normal(u.get("nodeType"))});}catch(const JSTypeError&){result="TypeError";}
        row("uniform factory traversal",array({result,events}));
    }
    for(unsigned kind=0;kind<9;++kind)for(const auto* method:{"keys","values","entries"})for(const bool shadow:{false,true}){
        auto typed=NativeArray_detail::constructor(kind).construct({array({1,2,3})});
        if(shadow)getter(typed,"length",JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{throw JSTypeError("length shadow must not be observed");}));
        auto iterator=call(typed,method),result=array();
        for(unsigned i=0;i<5;++i){const auto step=call(iterator,"next");call(result,"push",{normal(step)});if(step.get("done").toBoolean())break;}
        row("TypedArray iterator",result);
        row("TypedArray method metadata",array({typed.get(method).get("name"),typed.get(method).get("length"),!typed.get(method).isConstructible(),typed.get("values").strictEquals(typed.get(JSValue::wellKnownSymbol("iterator")))}));
    }
    auto typed=NativeArray_detail::constructor(1).construct({array({1,2,3})});
    row("TypedArray receiver brand",attempt(typed.get("values"),JSValue::object(),{}));
    getter(typed,"length",JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{throw JSTypeError("length shadow");}));
    auto borrowed=JSValue::array().get("values").call(typed);row("Array iterator typed length",array({normal(call(borrowed,"next")),attempt(intrinsic.get("from"),intrinsic,{typed})}));
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
