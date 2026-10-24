#include "core/BufferAttribute.h"
#include "core/NativeObject.h"
#include "core/NativeArray.h"
#include "core/InterleavedBufferAttribute.h"
#include "nodes/core/NodeBuilder.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue array(const Args& values={}) {auto out=JSValue::array();for(const auto& value:values)out.get("push").call(out,{value});return out;}
JSValue call(const JSValue& self,const char* key,const Args& args={}) {return self.get(key).call(self,args);}
void push(const JSValue& events,const Args& values) {call(events,"push",{array(values)});}
void getter(JSValue object,const JSValue& key,const JSValue& callback) {JSPropertyDescriptor d;d.getter=callback;d.configurable=d.enumerable=true;object.defineProperty(key,d);}
JSValue normal(const JSValue& value) {
    if(value.isUndefined())return "#undefined";
    if(value.type()==JSValue::Type::Number) {
        if(std::isnan(value.number()))return "#NaN";if(std::isinf(value.number()))return value.number()>0?"#Infinity":"#-Infinity";
        if(value.number()==0&&std::signbit(value.number()))return "#-0";
    }
    if(!value.isObject())return value;
    auto out=value.type()==JSValue::Type::Array?array():JSValue::object();
    for(const auto& key:value.keys())out.set(key,normal(value.get(key)));return out;
}
template<class F> JSValue attempt(F callback) {try{return normal(callback());}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception&){return "#Error";}}
void row(const char* name,const JSValue& value) {auto out=JSValue::object();out.set("name",name);out.set("value",normal(value));std::cout<<out.toJSON()->dump()<<'\n';}
JSValue values(const JSValue& typed) {auto out=array();const auto length=NativeArray_detail::unwrap(typed).size();for(std::size_t i=0;i<length;++i)call(out,"push",{typed.get(JSValue(i))});return out;}
JSValue coerce(const JSValue& events,const char* name,const Args& sequence) {
    auto state=JSValue::object();state.set("index",0);auto result=JSValue::object();
    result.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {
        auto state=c[3];const auto index=state.get("index").number();state.set("index",index+1);push(c[0],{"coerce",c[1],args[0],index});
        return c[2].get(JSValue(std::min(index,double(c[2].length()-1))));
    },{events,name,array(sequence),state}));return result;
}
void metadata() {
    const auto constructor=nativeFloat16BufferAttributeConstructor(),prototype=constructor.get("prototype");auto methods=array();
    for(const auto& key:prototype.ownPropertyNames()) {const auto d=*prototype.getOwnPropertyDescriptor(key);const auto fn=prototype.get(key);
        call(methods,"push",{array({key,fn.get("name"),fn.get("length"),fn.isConstructible(),d.enumerable.value_or(false),d.configurable.value_or(false),d.writable.value_or(false)})});}
    row("metadata",array({constructor.get("name"),constructor.get("length"),constructor.getPrototype().sameValue(nativeBufferAttributeConstructor()),prototype.getPrototype().sameValue(nativeBufferAttributeConstructor().get("prototype")),methods}));
    auto native=std::make_shared<Float16BufferAttribute>(std::vector<std::uint16_t>{15360,49152},1);auto value=nativeAttribute(native);
    auto ordinary=nativeAttribute(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{15360},1));ordinary.set("isFloat16BufferAttribute",true);
    row("identity",array({instanceOfFloat16BufferAttribute(value),instanceOfFloat16BufferAttribute(ordinary),instanceOfFloat16BufferAttribute(JSValue::object(prototype)),instanceOfFloat16BufferAttribute(prototype),instanceOfFloat16BufferAttribute(nullptr)}));
    value.set("isFloat16BufferAttribute",false);row("flag independent",array({instanceOfFloat16BufferAttribute(value),call(value,"getX",{0}),call(value,"getX",{1})}));
    value.setPrototype(nativeBufferAttributeConstructor().get("prototype"));row("changed instance prototype",instanceOfFloat16BufferAttribute(value));
    auto mutablePrototype=prototype;mutablePrototype.set("constructor",nullptr);row("stable binding",nativeFloat16BufferAttributeConstructor().sameValue(constructor));mutablePrototype.set("constructor",constructor);
    for(const int mode:{0,1,2,3,4}) {
        auto events=array();auto mutableConstructor=constructor;
        if(mode==0||mode==1)getter(mutableConstructor,JSValue::wellKnownSymbol("hasInstance"),JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
            push(c[0],{"get"});return JSValue::function([](const JSValue& receiver,const Args& args,const Args& c)->JSValue {push(c[0],{"call",receiver.sameValue(c[1]),args[0].sameValue(c[2])});return c[3];},c);
        },{events,constructor,value,mode==1}));
        else {JSPropertyDescriptor d;d.value=mode==2?JSValue(0):mode==3?JSValue(nullptr):JSValue();d.configurable=true;mutableConstructor.defineProperty(JSValue::wellKnownSymbol("hasInstance"),d);}
        row("hasInstance",array({mode,attempt([&]{return JSValue(instanceOfFloat16BufferAttribute(value));}),events}));mutableConstructor.erase(JSValue::wellKnownSymbol("hasInstance"));
    }
}
void construction() {
    const auto constructor=nativeFloat16BufferAttributeConstructor();
    const Args inputs{{},nullptr,0,3,2.5,-1,std::numeric_limits<double>::quiet_NaN(),array({15360,49152,-1,65537}),JSValue::symbol("input")};
    for(const auto& input:inputs)row("construction",attempt([&]{const auto value=constructor.construct({input,"2",1});return array({values(value.get("array")),value.get("itemSize"),value.get("count"),value.get("normalized"),value.get("isFloat16BufferAttribute"),instanceOfFloat16BufferAttribute(value)});}));
    const auto input=NativeArray_detail::constructor(3).construct({array({15360,49152})});
    auto value=constructor.construct({input,1});call(value,"setX",{0,.5});
    const auto clone=call(value,"clone");row("copied backing and clone",array({values(input),values(value.get("array")),instanceOfFloat16BufferAttribute(clone),call(clone,"getX",{0}),value.get("array").sameValue(clone.get("array"))}));
    auto subclass=JSValue::constructor([](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Derived",0);subclass.get("prototype").setPrototype(constructor.get("prototype"));
    auto derived=constructor.construct({array({15360}),1},subclass);row("newTarget",array({derived.getPrototype().sameValue(subclass.get("prototype")),instanceOfFloat16BufferAttribute(derived),call(derived,"getX",{0})}));
    auto events=array();auto alternate=JSValue::constructor([](const JSValue& receiver,const Args& args,const Args& c)->JSValue {push(c[0],{"super",values(args[0]),args[1],args[2],receiver.getPrototype().sameValue(c[1])});return receiver;},{events,constructor.get("prototype")},"Alternate",3);
    auto mutableConstructor=constructor;const auto parent=constructor.getPrototype();mutableConstructor.setPrototype(alternate);
    const auto alternateValue=constructor.construct({array({65537}),"item",false});mutableConstructor.setPrototype(parent);
    row("dynamic super",array({alternateValue.get("isFloat16BufferAttribute"),events}));
    auto mutationEvents=array();auto changedSuper=JSValue::constructor([](const JSValue& receiver,const Args& args,const Args& c)->JSValue {push(c[0],{"changedSuper",values(args[0])});return receiver;},{mutationEvents},"ChangedSuper",3);
    auto mutationInput=JSValue::object();mutationInput.set(JSValue::wellKnownSymbol("iterator"),JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
        push(c[0],{"iterator"});auto constructor=c[1];constructor.setPrototype(c[2]);const auto values=array({65537});return values.get(JSValue::wellKnownSymbol("iterator")).call(values);
    },{mutationEvents,constructor,changedSuper}));
    const auto changedValue=constructor.construct({mutationInput,1});mutableConstructor.setPrototype(parent);
    row("super mutated by argument",array({changedValue.get("isFloat16BufferAttribute"),mutationEvents}));
    auto coercions=array();const auto size=coerce(coercions,"itemSize",{2,7});const auto raw=constructor.construct({array({1,2,3,4}),size,{}});
    row("constructor coercion",array({raw.get("itemSize").sameValue(size),raw.get("count"),raw.get("normalized"),coercions}));
    row("constructor class call",attempt([&]{return constructor.call({},{});}));
}
void halfMethods() {
    const auto constructor=nativeFloat16BufferAttributeConstructor(),prototype=constructor.get("prototype");
    for(const auto* method:{"getX","getY","getZ","getW"})for(double index:{0.,1.,-1.,.5}) {
        auto value=constructor.construct({array({15360,49152,14336,32768,31744,32256,0,16384}),4});
        row("half getter",array({method,index,attempt([&]{return call(value,method,{index});})}));
    }
    for(const auto* method:{"setX","setY","setZ","setW","setXY","setXYZ","setXYZW"})for(const bool normalized:{false,true}) {
        auto value=constructor.construct({array({0,0,0,0,0,0,0,0}),4,normalized});const Args input{1,.25,-.5,.125,.75};
        row("half setter",array({method,normalized,attempt([&]{return JSValue(call(value,method,input).sameValue(value));}),values(value.get("array"))}));
    }
    for(const auto* method:{"getX","getY","getZ","getW","setX","setY","setZ","setW","setXY","setXYZ","setXYZW"})for(const bool normalized:{false,true}) {
        auto events=array(),backing=array({15360,15360,15360,15360});auto self=JSValue::object();
        // The methods remain generic: own getters and a non-native receiver are observable.
        const auto size=coerce(events,"size",{4});
        for(const auto* key:{"array","itemSize","normalized"})getter(self,key,JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"get",c[1]});return c[2];},{events,key,std::string(key)=="array"?backing:std::string(key)=="itemSize"?size:JSValue(normalized)}));
        const auto result=attempt([&]{return prototype.get(method).call(self,{coerce(events,"index",{0}),coerce(events,"x",{.5,.25}),.5,.125,.75});});
        row("generic half protocol",array({method,normalized,result.isObject()?JSValue("#self"):result,backing,events}));
    }
    for(const auto* method:{"getX","setX"}) {
        auto events=array(),self=JSValue::object();self.set("itemSize",1);self.set("normalized",false);
        auto backing=JSValue::object();backing.set("0",coerce(events,"read",{15360,0}));self.set("array",backing);
        const auto result=attempt([&]{return prototype.get(method).call(self,{0,coerce(events,"write",{1,.5})});});
        row("half repeated coercion",array({method,result.isObject()?JSValue("#self"):result,normal(backing.get("0")).isObject()?JSValue("#object"):normal(backing.get("0")),events}));
    }
}
JSValue iterableInput(const JSValue& events,int mode) {
    if(mode==15) {
        auto input=NativeArray_detail::constructor(3).construct({array({1,2})});
        for(const auto& key:{JSValue("length"),JSValue::wellKnownSymbol("iterator")})getter(input,key,JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue {throw JSTypeError("typed internal branch");}));return input;
    }
    auto input=JSValue::object(),state=JSValue::object();state.set("index",0);
    getter(input,"length",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"length"});return 2;},{events}));
    for(int index=0;index<2;++index)getter(input,JSValue(index),JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"index",c[1]});return coerce(c[0],"element",{c[1].number()+1});},{events,index}));
    getter(input,JSValue::wellKnownSymbol("iterator"),JSValue::function([](const JSValue& receiver,const Args&,const Args& c)->JSValue {
        const auto mode=int(c[1].number());push(c[0],{"iterator:get",receiver.sameValue(c[3])});
        if(mode==12)throw JSTypeError("iterator getter");if(mode==1)return nullptr;if(mode==13)return {};if(mode==2)return 7;
        return JSValue::function([](const JSValue& receiver,const Args&,const Args& c)->JSValue {
            const auto mode=int(c[1].number());push(c[0],{"iterator:call",receiver.sameValue(c[3])});
            if(mode==3)throw JSTypeError("iterator call");if(mode==4)return 1;
            auto iterator=JSValue::object();getter(iterator,"next",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
                const auto mode=int(c[1].number());push(c[0],{"next:get"});if(mode==10)throw JSTypeError("next getter");if(mode==5)return 1;
                return JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
                    const auto mode=int(c[1].number());auto state=c[2];const auto index=state.get("index").number();state.set("index",index+1);push(c[0],{"next",index});
                    if(mode==6&&index==1)throw JSTypeError("next call");if(mode==11&&index==1)return 1;
                    auto step=JSValue::object();getter(step,"done",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"done",c[4]});if(c[1].strictEquals(7)&&c[4].strictEquals(1))throw JSTypeError("done getter");return c[4].number()==2;},{c[0],c[1],c[2],c[3],index}));
                    getter(step,"value",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
                        push(c[0],{"value",c[4]});if(c[1].strictEquals(8)&&c[4].strictEquals(1))throw JSTypeError("value getter");
                        auto element=JSValue::object();element.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {push(c[0],{"convert",c[4],args[0]});if(c[1].strictEquals(9)&&c[4].strictEquals(0))throw JSTypeError("element conversion");return c[4].number()+1;},c));return element;
                    },{c[0],c[1],c[2],c[3],index}));return step;
                },c);
            },c));
            getter(iterator,"return",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"return:get"});return JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"return:call"});return JSValue::object();},c);},c));return iterator;
        },c);
    },{events,mode,state,input}));return input;
}
void typedConstruction() {
    for(std::size_t kind=0;kind<10;++kind)for(int mode=0;mode<16;++mode) {
        auto events=array();const auto input=iterableInput(events,mode);
        row("iterable construction",array({kind,mode,attempt([&]{const auto value=kind==9?nativeFloat16BufferAttributeConstructor().construct({input,1}).get("array"):NativeArray_detail::constructor(kind).construct({input});return values(value);}),events}));
    }
}
void allocationTiming() {
    for(unsigned kind=0;kind<9;++kind)for(unsigned mode=0;mode<11;++mode) {
        auto events=array(),old=JSValue::object(),fresh=JSValue::object();
        auto target=JSValue::constructor([](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Alternate",0,JSValue::ConstructorKind::Callable);target.set("prototype",old);
        auto state=JSValue::object();state.set("events",events);state.set("target",target);state.set("fresh",fresh);state.set("mode",mode);state.set("index",0);
        const auto phase=JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue {
            const auto mode=c[0].get("mode").number();const auto name=args[0];push(c[0].get("events"),{name,args.size()>1?args[1]:JSValue()});
            if((mode==0&&name.strictEquals("iterator:get"))||(mode==1&&name.strictEquals("iterator:call"))||(mode==2&&name.strictEquals("next:get"))||
               (mode==3&&name.strictEquals("next")&&args[1].strictEquals(0))||(mode==4&&name.strictEquals("element")&&args[1].strictEquals(0))||
               (mode==5&&name.strictEquals("length"))||(mode==6&&name.strictEquals("index")&&args[1].strictEquals(0))||
               (mode==8&&name.strictEquals("offset"))||(mode==9&&name.strictEquals("length:convert"))) {
                auto target=c[0].get("target");target.set("prototype",c[0].get("fresh"));
            }
            if(mode==10&&name.strictEquals("iterator:get")){auto target=c[0].get("target");target.set("prototype",nullptr);}return {};
        },{state});state.set("phase",phase);
        auto input=JSValue::object();Args args;
        if(mode==7) {
            input=NativeArray_detail::constructor(3).construct({array({1,2})});getter(input,JSValue::wellKnownSymbol("iterator"),JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{throw JSTypeError("ignored typed iterator");}));args={input};
        } else if(mode==8) {
            const auto bytes=NativeArray_detail::constructor(kind).construct({2}).get("buffer");auto offset=JSValue::object();offset.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {c[0].call({}, {"offset"});return 0;},{phase}));args={bytes,offset};
        } else if(mode==5||mode==6||mode==9) {
            getter(input,"length",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
                c[0].call({}, {"length"});if(!c[1].strictEquals(9))return 2;auto length=JSValue::object();length.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {c[0].call({}, {"length:convert"});return 2;},{c[0]}));return length;
            },{phase,mode}));
            for(unsigned i=0;i<2;++i)getter(input,JSValue(i),JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {c[0].call({}, {"index",c[1]});return c[1].number()+1;},{phase,i}));args={input};
        } else {
            getter(input,JSValue::wellKnownSymbol("iterator"),JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
                c[0].get("phase").call({}, {"iterator:get"});return JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
                    c[0].get("phase").call({}, {"iterator:call"});auto iterator=JSValue::object();getter(iterator,"next",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
                        c[0].get("phase").call({}, {"next:get"});return JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
                            auto state=c[0];const auto index=state.get("index").number();state.set("index",index+1);state.get("phase").call({}, {"next",index});auto step=JSValue::object();step.set("done",index==2);
                            if(index<2){auto element=JSValue::object();element.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {c[0].get("phase").call({}, {"element",c[1]});return c[1].number()+1;},{state,index}));step.set("value",element);}return step;
                        },c);
                    },c));return iterator;
                },c);
            },{state}));args={input};
        }
        row("typed allocation timing",array({kind,mode,attempt([&]{const auto result=NativeArray_detail::constructor(kind).construct(args,target);return array({result.getPrototype().sameValue(old),result.getPrototype().sameValue(fresh),result.getPrototype().sameValue(NativeArray_detail::constructor(kind).get("prototype")),values(result)});}),events}));
    }
}
JSValue builder() {
    auto renderer=JSValue::object(),debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);
    return nativeNodeBuilderConstructor().construct({nullptr,renderer,nullptr});
}
void attributeTypes() {
    const auto b=builder();const Args sizes{1,2,3,4,9,16,"3"},normalizations{false,true,0,1,"true",nullptr,{}};
    for(unsigned kind=0;kind<9;++kind)for(const auto& size:sizes)for(const auto& normalized:normalizations) {
        const auto typed=NativeArray_detail::constructor(kind).construct({4});
        const auto attribute=nativeBufferAttributeConstructor().construct({typed,size,normalized});
        row("builder actual attribute",array({kind,size,normalized,attempt([&]{return call(b,"getTypeFromAttribute",{attribute});})}));
    }
    for(const auto& size:sizes)for(const auto& normalized:normalizations) {
        auto attribute=nativeFloat16BufferAttributeConstructor().construct({array({0,0,0,0}),size,normalized});attribute.set("isFloat16BufferAttribute",false);
        row("builder actual half attribute",array({size,normalized,attempt([&]{return call(b,"getTypeFromAttribute",{attribute});})}));
    }
    for(unsigned kind=0;kind<9;++kind)for(const auto& normalized:normalizations) {
        auto data=JSValue::object();data.set("array",NativeArray_detail::constructor(kind).construct({4}));data.set("stride",4);data.set("count",1);
        const auto attribute=InterleavedBufferAttribute(data,3,0,normalized).object();
        row("builder actual interleaved",array({kind,normalized,attempt([&]{return call(b,"getTypeFromAttribute",{attribute});})}));
    }
    for(unsigned mode=0;mode<32;++mode) {
        auto events=array();auto self=builder();auto attribute=JSValue::object(mode&2?nativeFloat16BufferAttributeConstructor().get("prototype"):JSValue::object());
        auto data=JSValue::object(),typed=NativeArray_detail::constructor(2).construct({4});
        for(const auto* key:{"isInterleavedBufferAttribute","data","array","itemSize","normalized","isFloat16BufferAttribute"}) {
            const JSValue result=std::string(key)=="isInterleavedBufferAttribute"?JSValue(bool(mode&1)):std::string(key)=="data"?data:std::string(key)=="array"?typed:std::string(key)=="itemSize"?JSValue(2):std::string(key)=="normalized"?(mode&4?JSValue(true):JSValue(1)):JSValue();
            getter(attribute,key,JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"get",c[1]});if(c[1].strictEquals("isFloat16BufferAttribute"))throw JSTypeError("flag must not be read");return c[2];},{events,key,result}));
        }
        getter(data,"array",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"get","data.array"});return c[1];},{events,typed}));
        getter(typed,"constructor",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"get","array.constructor"});return NativeArray_detail::constructor(2);},{events}));
        for(const auto* method:{"getTypeFromArray","getTypeFromLength"})getter(self,method,JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
            push(c[0],{"get",c[1]});return JSValue::function([](const JSValue& receiver,const Args& args,const Args& c)->JSValue {
                if(c[1].strictEquals("getTypeFromArray")) {push(c[0],{"call",c[1],args[0].sameValue(c[3])});if(c[2].toBoolean())throw JSTypeError("array type");}
                else push(c[0],{"call",c[1],args[0],args[1]});
                return nativeNodeBuilderConstructor().get("prototype").get(c[1]).call(receiver,args);
            },c);
        },{events,method,bool(mode&16),typed}));
        auto constructor=nativeFloat16BufferAttributeConstructor();
        if(mode&8)getter(constructor,JSValue::wellKnownSymbol("hasInstance"),JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {push(c[0],{"get","hasInstance"});return JSValue::function([](const JSValue& receiver,const Args& args,const Args& c)->JSValue {push(c[0],{"call","hasInstance",receiver.sameValue(c[1]),args[0].sameValue(c[2])});return false;},c);},{events,constructor,attribute}));
        const auto result=attempt([&]{return call(self,"getTypeFromAttribute",{attribute});});
        if(mode&8)constructor.erase(JSValue::wellKnownSymbol("hasInstance"));
        row("builder attribute protocol",array({mode,result,events}));
    }
}
}
int main() {try {auto keeper=JSValue::object();ThreeUtils::setConsoleFunction(JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return {};}));metadata();construction();halfMethods();typedConstruction();attributeTypes();allocationTiming();return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
