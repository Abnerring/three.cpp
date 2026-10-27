#include "nodes/accessors/UniformArrayNode.h"
#include "nodes/accessors/BufferNode.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/ArrayElementNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "core/JSRegExp.h"
#include "core/NativeArray.h"
namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
template<class F> void write(const JSValue& target,double index,F rhs){
    // PutValue follows RHS evaluation, also when its base is null or undefined.
    auto writable=target;const auto value=rhs();writable.set(JSValue(index),value);
}
JSValue fallbackZero(const JSValue& value){return value.toBoolean()?value:JSValue(0);}
JSValue updateBuffer(const JSValue& self){
    const auto array=self.get("array"),value=self.get("value"),type=self.get("elementType");
    const bool scalar=type.strictEquals("float")||type.strictEquals("int")||type.strictEquals("uint");
    for(double i=0;less(i,array.get("length"));++i){
        if(scalar){write(value,i*4,[&]{return array.get(JSValue(i));});continue;}
        const auto item=array.get(JSValue(i));
        if(type.strictEquals("color")){
            const auto index=i*4;
            write(value,index,[&]{return item.get("r");});write(value,index+1,[&]{return item.get("g");});write(value,index+2,[&]{return fallbackZero(item.get("b"));});
        }else if(type.strictEquals("mat2")){
            for(int j=0;j<4;++j)write(value,i*4+j,[&]{return item.get("elements").get(JSValue(j));});
        }else if(type.strictEquals("mat3")){
            for(int j=0;j<9;++j)write(value,i*16+(j/3)*4+j%3,[&]{return item.get("elements").get(JSValue(j));});
            write(value,i*16+15,[]{return JSValue(1);});
        }else if(type.strictEquals("mat4")){
            for(double j=0;less(j,item.get("elements").get("length"));++j)write(value,i*16+j,[&]{return item.get("elements").get(JSValue(j));});
        }else{
            const auto index=i*4;
            write(value,index,[&]{return item.get("x");});write(value,index+1,[&]{return item.get("y");});write(value,index+2,[&]{return fallbackZero(item.get("z"));});write(value,index+3,[&]{return fallbackZero(item.get("w"));});
        }
    }
    return {};
}
JSValue derived(const char* name,unsigned arity,const JSValue& base,JSValue::NativeFunction initialize,const JSValue& bindings,
                std::initializer_list<std::pair<const char*,unsigned>> methods,JSValue::NativeFunction method){
    auto holder=JSValue::object(nullptr);auto ctor=JSValue::builtinConstructor(initialize,{holder,bindings},name,arity);holder.set("constructor",ctor);ctor.setPrototype(base);
    auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{name},"get type",0);type.configurable=true;ctor.defineProperty("type",type);
    for(const auto& [key,length]:methods){JSPropertyDescriptor d;d.value=JSValue::function(method,{key,prototype,bindings},key,length);d.writable=d.configurable=true;prototype.defineProperty(key,d);}
    ctor.retainForHeapLifetime();return ctor;
}
JSValue method(const JSValue& receiver,const Args& args,const Args& c){
    auto self=receiver;const auto name=c[0];const auto builder=arg(args,0),bindings=c[2];
    if(name.strictEquals("generateNodeType"))return invoke(self,"getPaddedType")({builder});
    if(name.strictEquals("getElementType"))return self.get("elementType");
    if(name.strictEquals("getPaddedType")){
        const auto type=self.get("elementType");
        if(type.strictEquals("mat2"))return builder.get("renderer").get("backend").get("isWebGLBackend").strictEquals(true)?JSValue("vec4"):JSValue("mat2");
        const auto regexp=createNativeRegExp("mat");if(invoke(regexp,"test")({type}).strictEquals(true))return "mat4";
        if(invoke(type,"charAt")({0}).strictEquals("i"))return "ivec4";
        if(invoke(type,"charAt")({0}).strictEquals("u"))return "uvec4";
        return "vec4";
    }
    if(name.strictEquals("updateBuffer"))return updateBuffer(self);
    if(name.strictEquals("update")){invoke(self,"updateBuffer")();return {};}
    if(name.strictEquals("onUpdate")){
        const auto callback=invoke(arg(args,0),"bind")({self});
        const auto superMethod=c[1].getPrototype().get("onUpdate",self);
        const auto compose=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{c[0].call({}, {arg(args,0),arg(args,1)});invoke(c[1],"updateBuffer")();return {};},{callback,self},"",2);
        return superMethod.call(self,{compose,arg(args,1)});
    }
    if(name.strictEquals("setup")){
        const auto length=self.get("array").get("length"),type=self.get("elementType");
        auto arrayType=bindings.get("Float32Array");const auto paddedType=invoke(self,"getPaddedType")({builder});
        const auto paddedLength=invoke(builder,"getTypeLength")({paddedType});
        if(invoke(type,"charAt")({0}).strictEquals("i"))arrayType=bindings.get("Int32Array");
        if(invoke(type,"charAt")({0}).strictEquals("u"))arrayType=bindings.get("Uint32Array");
        const auto numericLength=length.toNumber();const auto size=numericLength*paddedLength.toNumber();
        self.set("value",arrayType.construct({size}));self.set("bufferCount",length);self.set("bufferType",paddedType);invoke(self,"updateBuffer")();
        return c[1].getPrototype().get("setup",self).call(self,{builder});
    }
    if(name.strictEquals("element")){
        const auto ctor=bindings.get("UniformArrayElementNode");const auto index=bindings.get("nodeObject").call({}, {arg(args,0)});return ctor.construct({self,index});
    }
    throw std::logic_error("Unknown UniformArrayNode method");
}
}
JSValue nativeUniformArrayNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"UniformArrayNode module");
    const auto tsl=nativeTSLBase(),constants=nativeNodeConstants(),utils=nativeNodeUtils(),arrayElement=nativeArrayElementNodeConstructor(),base=nativeBufferNodeConstructor();
    auto bindings=JSValue::object(nullptr);own(bindings,"nodeObject",nativeTSLBaseBinding("nodeObject"));own(bindings,"getValueType",nativeNodeUtilsBinding("getValueType"));own(bindings,"NodeUpdateType",nativeNodeConstantsBinding("NodeUpdateType"));
    own(bindings,"Float32Array",NativeArray_detail::constructor(6));own(bindings,"Int32Array",NativeArray_detail::constructor(4));own(bindings,"Uint32Array",NativeArray_detail::constructor(5));
    const auto element=derived("UniformArrayElementNode",2,arrayElement,+[](const JSValue& target,const Args& args,const Args& c)->JSValue{
        auto self=superConstruct(target,c,{arg(args,0),arg(args,1)});self.set("isArrayBufferElementNode",true);return self;
    },bindings,{{"generate",1}},+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
        const auto builder=arg(args,0);const auto snippet=c[1].getPrototype().get("generate",self).call(self,{builder});
        const auto type=invoke(self,"getNodeType")({builder});const auto padded=invoke(self.get("node"),"getPaddedType")({builder});return invoke(builder,"format")({snippet,padded,type});
    });own(bindings,"UniformArrayElementNode",element);
    const auto ctor=derived("UniformArrayNode",1,base,+[](const JSValue& target,const Args& args,const Args& c)->JSValue{
        auto self=superConstruct(target,c,{nullptr});const auto array=arg(args,0),type=arg(args,1,nullptr);self.set("array",array);
        self.set("elementType",type.isNull()?c[1].get("getValueType").call({}, {array.get("0")}):type);
        self.set("updateType",c[1].get("NodeUpdateType").get("RENDER"));self.set("isArrayBufferNode",true);return self;
    },bindings,{{"generateNodeType",1},{"getElementType",0},{"getPaddedType",1},{"update",0},{"onUpdate",2},{"updateBuffer",0},{"setup",1},{"element",1}},method);
    own(bindings,"default",ctor);own(bindings,"uniformArray",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{return c[0].construct({arg(args,0),arg(args,1)});},{ctor},"uniformArray",2));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);auto module=JSValue::object();own(module,"default",ctor);own(module,"uniformArray",bindings.get("uniformArray"));module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeUniformArrayNodeBinding(const std::string& name){const auto keeper=nativeUniformArrayNodeModule();return lexical().lock()->get(name);}
JSValue nativeUniformArrayNodeConstructor(){return nativeUniformArrayNodeBinding("default");}
JSValue nativeUniformArrayElementNodeConstructor(){return nativeUniformArrayNodeBinding("UniformArrayElementNode");}
UniformArrayNode::UniformArrayNode(const JSValue& array,const JSValue& type):object_(constructor().construct({array,type})){}
JSValue UniformArrayNode::getPaddedType(const JSValue& builder)const{return object_.get("getPaddedType").call(object_,{builder});}
JSValue UniformArrayNode::updateBuffer()const{return object_.get("updateBuffer").call(object_);}
JSValue UniformArrayNode::update(const JSValue& frame)const{return object_.get("update").call(object_,{frame});}
JSValue UniformArrayNode::onUpdate(const JSValue& callback,const JSValue& updateType)const{return object_.get("onUpdate").call(object_,{callback,updateType});}
JSValue UniformArrayNode::setup(const JSValue& builder)const{return object_.get("setup").call(object_,{builder});}
JSValue UniformArrayNode::element(const JSValue& index)const{return object_.get("element").call(object_,{index});}
