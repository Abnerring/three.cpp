#include "renderers/vulkan/VulkanBindingCommands.h"
#include "renderers/vulkan/VulkanBufferCommands.h"
#include "core/NativeArray.h"
#include "renderers/webgl/WebGLBindingStates.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cmath>
namespace {
int checks=0,live=0;
void require(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
JSValue list(const std::vector<JSValue>& values) {
    auto result=JSValue::array();
    for(std::size_t i=0;i<values.size();++i)result.set(std::to_string(i),values[i]);
    return result;
}
struct Iterable {
    JSValue value=JSValue::object(),iterator=JSValue::object(),state=JSValue::object();
    explicit Iterable(const std::vector<JSValue>& values) {
        state.set("values",list(values));state.set("index",0);state.set("nextGets",0);state.set("nextCalls",0);
        state.set("methodCalls",0);state.set("returnCalls",0);state.set("receivers",true);state.set("throwAt",-1);
        state.set("receiver",value);state.set("iterator",iterator);
        state.set("next",JSValue::function(+[](const JSValue& self,const auto&,const auto& c)->JSValue {
            auto s=c[0];s.set("receivers",s.get("receivers").toBoolean()&&self.sameValue(s.get("iterator")));
            s.set("nextCalls",s.get("nextCalls").number()+1);
            const auto index=s.get("index").number();
            if(s.get("throwAt").strictEquals(index))throw JSTypeError("iterator next failure");
            auto result=JSValue::object();const auto values=s.get("values");
            result.set("done",index>=values.length());
            if(index<values.length())result.set("value",values.get(JSValue(index)));
            s.set("index",index+1);return result;
        },{state}));
        JSPropertyDescriptor next;next.configurable=true;
        next.getter=JSValue::function(+[](const JSValue&,const auto&,const auto& c)->JSValue {
            auto s=c[0];s.set("nextGets",s.get("nextGets").number()+1);return s.get("next");
        },{state});iterator.defineProperty("next",next);
        iterator.set("return",JSValue::function(+[](const JSValue&,const auto&,const auto& c)->JSValue {
            auto s=c[0];s.set("returnCalls",s.get("returnCalls").number()+1);return JSValue::object();
        },{state}));
        value.set(JSValue::wellKnownSymbol("iterator"),JSValue::function(+[](const JSValue& self,const auto&,const auto& c)->JSValue {
            auto s=c[0];s.set("receivers",s.get("receivers").toBoolean()&&self.sameValue(s.get("receiver")));
            s.set("methodCalls",s.get("methodCalls").number()+1);s.set("index",0);return s.get("iterator");
        },{state}));
    }
};
struct Memory final:VulkanBufferCommands::Storage {
    std::vector<std::byte> bytes;
    Memory(){++live;}~Memory(){--live;}
    void setData(std::span<const std::byte> data)override{bytes.assign(data.begin(),data.end());}
    void setSubData(std::size_t offset,std::span<const std::byte> data)override{std::memcpy(bytes.data()+offset,data.data(),data.size());}
    std::vector<std::byte> read()const override{return bytes;}
    std::size_t size()const override{return bytes.size();}
};
struct RestoreProperty {
    JSValue object;
    std::string name;
    std::optional<JSPropertyDescriptor> original;
    RestoreProperty(JSValue value,std::string key):object(std::move(value)),name(std::move(key)),original(object.getOwnPropertyDescriptor(name)){}
    ~RestoreProperty(){if(original)object.defineProperty(name,*original);else object.erase(name);}
};
void prototypeIsolationTests() {
    auto input=JSValue::array();
    for(unsigned i=0;i<4;++i)input.set(std::to_string(i),double(i+1)/4);
    const auto typed=NativeArray_detail::wrap(Float32Array{.125f,.25f,.5f});
    for(auto prototype:{JSValue::array().getPrototype(),JSValue::object().getPrototype()}) {
        auto counters=JSValue::object();counters.setPrototype(nullptr);counters.set("calls",0);
        RestoreProperty restoreZero(prototype,"0"),restoreOne(prototype,"1");
        JSPropertyDescriptor setter;setter.configurable=true;
        setter.setter=JSValue::function(+[](const JSValue&,const auto&,const auto& captures)->JSValue {
            auto state=captures[0];state.set("calls",state.get("calls").number()+1);return {};
        },{counters});
        prototype.defineProperty("0",setter);
        JSPropertyDescriptor readonly;readonly.value=999;readonly.writable=false;readonly.configurable=true;
        prototype.defineProperty("1",readonly);
        VulkanBufferCommands storage([]{return std::make_shared<Memory>();});
        VulkanBindingCommands commands(storage.object(),storage.bindingInterface(),4);const auto gl=commands.object();
        const auto call=[&](const char* name,std::vector<JSValue> args={}){return gl.get(name).call(gl,args);};
        require(commands.snapshot().attributes[0].value==std::array<float,4>{0,0,0,1},"Internal constant initialization ignores inherited numeric setters");
        require(commands.snapshot().attributes[1].size==4 && !commands.snapshot().attributes[1].enabled,"Internal attribute initialization ignores inherited readonly indices");
        const auto vao=call("createVertexArray");call("bindVertexArray",{vao});
        call("vertexAttrib4fv",{0,input});
        require(commands.snapshot().attributes[0].value==std::array<float,4>{.25f,.5f,.75f,1},"Sequence conversion and stored constants bypass inherited numeric properties");
        call("vertexAttrib3fv",{1,typed});
        require(commands.snapshot().attributes[1].value==std::array<float,4>{.125f,.25f,.5f,1},"Branded conversion storage bypasses inherited readonly indices");
        const auto buffer=call("createBuffer");call("bindBuffer",{0x8892,buffer});call("bufferData",{0x8892,typed,0x88e4});
        call("vertexAttribPointer",{0,3,0x1406,false,0,0});call("enableVertexAttribArray",{0});
        require(commands.snapshot().attributes[0].enabled && commands.snapshot().attributes[0].buffer.sameValue(buffer),"VAO records preserve pointers under prototype pollution");
        call("deleteVertexArray",{vao});call("deleteBuffer",{buffer});
        require(live==0,"VAO teardown releases native storage under prototype pollution");
        commands.shutdown();storage.shutdown();
        require(counters.get("calls").strictEquals(0),"No internal constructor, conversion, VAO or teardown invokes inherited index setters");
    }
}
}
void integerConstantTests() {
    VulkanBufferCommands buffers([]{return std::make_shared<Memory>();});
    VulkanBindingCommands bindings(buffers.object(),buffers.bindingInterface(),8);auto gl=bindings.object();
    const auto call=[&](const char* name,const std::vector<JSValue>& args=std::vector<JSValue>{}){return gl.get(name).call(gl,args);};
    using Kind=VulkanBindingCommands::Attribute::ConstantType;
    require(bindings.snapshot().attributes[2].constantType==Kind::FloatingPoint,"Generic defaults begin as floating-point values");
    call("vertexAttribI4i",{2,2147483648.0,-1.75,NAN,4294967297.0});
    auto value=bindings.snapshot().attributes[2];
    require(value.constantType==Kind::SignedInteger&&value.signedValue==std::array<std::int32_t,4>{INT32_MIN,-1,0,1},"Native signed integer constants apply exact WebIDL truncation/wrap");
    call("vertexAttribI4ui",{2,-1,4294967297.0,16777217,.9});value=bindings.snapshot().attributes[2];
    require(value.constantType==Kind::UnsignedInteger&&value.unsignedValue==std::array<std::uint32_t,4>{UINT32_MAX,1,16777217,0},"Native unsigned constants preserve all 32 bits");
    const auto vao=call("createVertexArray");call("bindVertexArray",{vao});
    require(bindings.snapshot().attributes[2].unsignedValue==value.unsignedValue,"Integer current values belong to the context across vertex arrays");
    auto shortValues=JSValue::array();for(int i=0;i<3;++i)shortValues.set(JSValue(i),i);
    call("vertexAttribI4uiv",{2,shortValues});require(call("getError").strictEquals(0x0501),"Short integer value list reports INVALID_VALUE");
    require(bindings.snapshot().attributes[2].unsignedValue==value.unsignedValue,"Rejected integer list preserves the previous native value");
    call("vertexAttribI4iv",{2,NativeArray_detail::wrap(Int32Array(std::vector<std::int32_t>{-2,-3,-4,-5}))});value=bindings.snapshot().attributes[2];
    require(value.constantType==Kind::SignedInteger&&value.signedValue==std::array<std::int32_t,4>{-2,-3,-4,-5},"Native integer list accepts the actual typed array protocol");
    call("vertexAttrib2fv",{2,NativeArray_detail::wrap(Float32Array(std::vector<float>{.25f,.5f}))});value=bindings.snapshot().attributes[2];
    require(value.constantType==Kind::FloatingPoint&&value.value==std::array<float,4>{.25f,.5f,0,1},"Floating generic commands restore category and default missing components");
    call("deleteVertexArray",{vao});bindings.shutdown();buffers.shutdown();
}
void constantListProtocolTests() {
    VulkanBufferCommands buffers([]{return std::make_shared<Memory>();});
    VulkanBindingCommands bindings(buffers.object(),buffers.bindingInterface(),8);const auto gl=bindings.object();
    const auto call=[&](const char* name,const std::vector<JSValue>& args=std::vector<JSValue>{}){return gl.get(name).call(gl,args);};
    using Kind=VulkanBindingCommands::Attribute::ConstantType;
    JSPropertyDescriptor shadow;shadow.configurable=true;
    shadow.getter=JSValue::function(+[](const JSValue&,const auto&,const auto&)->JSValue{throw JSTypeError("shadowed typed-array property");});
    auto floats=NativeArray_detail::wrap(Float32Array{.125f,.25f,.5f,1});
    auto signedValues=NativeArray_detail::wrap(Int32Array(std::vector<std::int32_t>{INT32_MIN,-1,16777217,1}));
    auto unsignedValues=NativeArray_detail::wrap(Uint32Array(std::vector<std::uint32_t>{UINT32_MAX,0,16777217,1}));
    for(auto value:{floats,signedValues,unsignedValues}) {
        value.defineProperty("length",shadow);value.defineProperty(JSValue::wellKnownSymbol("iterator"),shadow);
    }
    call("vertexAttrib4fv",{0,floats});call("vertexAttribI4iv",{1,signedValues});call("vertexAttribI4uiv",{2,unsignedValues});
    require(bindings.snapshot().attributes[0].value==std::array<float,4>{.125f,.25f,.5f,1},"Float32List ignores shadowed length and iterator");
    require(bindings.snapshot().attributes[1].signedValue==std::array<std::int32_t,4>{INT32_MIN,-1,16777217,1},"Int32List reads exact native values without shadowed properties");
    require(bindings.snapshot().attributes[2].unsignedValue==std::array<std::uint32_t,4>{UINT32_MAX,0,16777217,1},"Uint32List reads exact native values without shadowed properties");
    Iterable custom({16777217,-1,4294967297.0,1});custom.value.defineProperty("length",shadow);
    call("vertexAttribI4uiv",{2,custom.value});
    require(bindings.snapshot().attributes[2].unsignedValue==std::array<std::uint32_t,4>{16777217,UINT32_MAX,1,1},"Custom iterable uses WebIDL integer conversion without length");
    require(custom.state.get("methodCalls").strictEquals(1)&&custom.state.get("nextGets").strictEquals(1)&&custom.state.get("nextCalls").strictEquals(5),"Sequence obtains next once and consumes the iterator through done");
    require(custom.state.get("receivers").strictEquals(true),"Iterator and next receive their original receivers");
    Iterable overrideValues({.25,.5,.75,1});auto array=list({999});
    overrideValues.state.set("receiver",array);array.set(JSValue::wellKnownSymbol("iterator"),overrideValues.value.get(JSValue::wellKnownSymbol("iterator")));
    call("vertexAttrib4fv",{0,array});
    require(bindings.snapshot().attributes[0].value==std::array<float,4>{.25f,.5f,.75f,1},"Array iterator override determines the complete float sequence");
    require(overrideValues.state.get("receivers").strictEquals(true),"Overridden array iterator receives the array");
    // A different typed-array kind takes the sequence member of the union.
    auto wrongKind=NativeArray_detail::wrap(Int16Array{9,9});Iterable wrongValues({-3,-2,-1,0});
    wrongValues.state.set("receiver",wrongKind);wrongKind.set(JSValue::wellKnownSymbol("iterator"),wrongValues.value.get(JSValue::wellKnownSymbol("iterator")));
    call("vertexAttribI4iv",{1,wrongKind});
    require(bindings.snapshot().attributes[1].signedValue==std::array<std::int32_t,4>{-3,-2,-1,0},"Nonmatching typed-array kind uses its observable iterator");
    auto arrayLike=JSValue::object();arrayLike.set("length",4);for(unsigned i=0;i<4;++i)arrayLike.set(std::to_string(i),1);
    for(const auto* name:{"vertexAttrib4fv","vertexAttribI4iv","vertexAttribI4uiv"}) {
        const auto previous=bindings.snapshot().attributes[3];bool rejected=false;
        try{call(name,{3,arrayLike});}catch(const JSTypeError&){rejected=true;}
        require(rejected,"Noniterable array-like list throws TypeError");
        require(call("getError").strictEquals(0),"WebIDL rejection does not set a GL error");
        require(bindings.snapshot().attributes[3].value==previous.value&&bindings.snapshot().attributes[3].constantType==previous.constantType,"Rejected WebIDL list preserves the generic value");
    }
    for(const auto* name:{"vertexAttrib4fv","vertexAttribI4iv","vertexAttribI4uiv"})for(const bool throws:{false,true}) {
        auto reenter=JSValue::object();
        reenter.set("valueOf",JSValue::function(+[](const JSValue&,const auto&,const auto& c)->JSValue {
            const auto gl=c[0];gl.get("vertexAttribI4ui").call(gl,{4,9,8,7,6});
            if(c[1].toBoolean())throw JSTypeError("element conversion failure");return 2;
        },{gl,throws}));
        Iterable source({1,reenter,3,4});bool rejected=false;
        try{call(name,{4,source.value});}catch(const JSTypeError&){rejected=true;}
        const auto result=bindings.snapshot().attributes[4];
        require(rejected==throws,"Element conversion preserves thrown completion");
        if(throws) {
            require(result.constantType==Kind::UnsignedInteger&&result.unsignedValue==std::array<std::uint32_t,4>{9,8,7,6},"Failed outer conversion preserves the committed reentrant command");
            require(source.state.get("nextCalls").strictEquals(2),"Throwing conversion stops before the next iterator step");
        } else {
            const auto expected=std::string(name)=="vertexAttrib4fv"?Kind::FloatingPoint:std::string(name)=="vertexAttribI4iv"?Kind::SignedInteger:Kind::UnsignedInteger;
            require(result.constantType==expected&&result.unsignedValue==std::array<std::uint32_t,4>{1,2,3,4},"Successful outer conversion commits after the reentrant command");
        }
        require(source.state.get("returnCalls").strictEquals(0),"WebIDL sequence conversion does not add IteratorClose");
        require(call("getError").strictEquals(0),"Conversion and reentry leave the GL error flag clear");
    }
    auto tail=JSValue::object();tail.set("valueOf",JSValue::function(+[](const JSValue&,const auto&,const auto& c)->JSValue {
        const auto gl=c[0];gl.get("vertexAttribI4ui").call(gl,{5,19,18,17,16});throw JSTypeError("tail conversion failure");
    },{gl}));
    for(const auto* name:{"vertexAttrib4fv","vertexAttribI4iv","vertexAttribI4uiv"}) {
        Iterable source({1,2,3,4,tail});bool rejected=false;
        try{call(name,{5,source.value});}catch(const JSTypeError&){rejected=true;}
        require(rejected&&source.state.get("nextCalls").strictEquals(5),"Unused tail elements are still converted before the GL command");
        require(bindings.snapshot().attributes[5].unsignedValue==std::array<std::uint32_t,4>{19,18,17,16},"Tail conversion failure does not commit the outer first four values");
        require(source.state.get("returnCalls").strictEquals(0),"Tail conversion failure does not call iterator return");
    }
    Iterable broken({1,2,3,4});broken.state.set("throwAt",1);
    const auto previous=bindings.snapshot().attributes[5];bool rejected=false;
    try{call("vertexAttribI4uiv",{5,broken.value});}catch(const JSTypeError&){rejected=true;}
    require(rejected&&broken.state.get("nextCalls").strictEquals(2),"Iterator next exception propagates immediately");
    require(bindings.snapshot().attributes[5].unsignedValue==previous.unsignedValue&&broken.state.get("returnCalls").strictEquals(0),"Iterator exception preserves prior state without closing the iterator");
    for(const auto* name:{"vertexAttrib4fv","vertexAttribI4iv","vertexAttribI4uiv"})for(const bool throws:{false,true}) {
        Iterable source({1,2,3,4});const auto next=source.state.get("next");
        source.state.set("next",JSValue::function(+[](const JSValue& self,const auto& args,const auto& c)->JSValue {
            const auto gl=c[0];gl.get("vertexAttribI4ui").call(gl,{6,29,28,27,26});
            if(c[2].toBoolean())throw JSTypeError("reentrant iterator failure");return c[1].call(self,args);
        },{gl,next,throws}));
        bool failed=false;try{call(name,{6,source.value});}catch(const JSTypeError&){failed=true;}
        const auto result=bindings.snapshot().attributes[6];
        require(failed==throws,"Reentrant next preserves its normal or abrupt completion");
        require(result.unsignedValue==(throws?std::array<std::uint32_t,4>{29,28,27,26}:std::array<std::uint32_t,4>{1,2,3,4}),"The outer generic command commits only after all reentrant iterator steps succeed");
        require(source.state.get("returnCalls").strictEquals(0),"Reentrant iterator failure does not add a return call");
    }
    auto getterFailure=JSValue::object();JSPropertyDescriptor iteratorGetter;iteratorGetter.configurable=true;
    iteratorGetter.getter=JSValue::function(+[](const JSValue&,const auto&,const auto& c)->JSValue {
        const auto gl=c[0];gl.get("vertexAttribI4ui").call(gl,{7,39,38,37,36});throw JSTypeError("iterator getter failure");
    },{gl});getterFailure.defineProperty(JSValue::wellKnownSymbol("iterator"),iteratorGetter);
    rejected=false;try{call("vertexAttrib4fv",{7,getterFailure});}catch(const JSTypeError&){rejected=true;}
    require(rejected&&bindings.snapshot().attributes[7].unsignedValue==std::array<std::uint32_t,4>{39,38,37,36},"Iterator getter exception preserves its reentrant state mutation");
    require(call("getError").strictEquals(0),"Iterator exceptions leave the GL error flag clear");
    bindings.shutdown();buffers.shutdown();
}
void nullPointerDetachTests() {
    for(const bool integer:{false,true}) {
        VulkanBufferCommands buffers([]{return std::make_shared<Memory>();});
        VulkanBindingCommands bindings(buffers.object(),buffers.bindingInterface(),4);const auto gl=bindings.object();
        const auto call=[&](const char* name,const std::vector<JSValue>& args=std::vector<JSValue>{}){return gl.get(name).call(gl,args);};
        const auto buffer=call("createBuffer"),attached=call("createVertexArray"),other=call("createVertexArray");
        call("bindBuffer",{0x8892,buffer});call("bufferData",{0x8892,NativeArray_detail::wrap(Uint32Array{1,2,3,4}),0x88e4});
        call("bindVertexArray",{attached});
        const auto pointer=[&](unsigned offset){if(integer)call("vertexAttribIPointer",{0,1,0x1405,0,offset});else call("vertexAttribPointer",{0,1,0x1406,false,0,offset});};
        pointer(0);call("enableVertexAttribArray",{0});call("bindVertexArray",{other});call("deleteBuffer",{buffer});
        require(live==1,"Unbound VAO retains deleted pointer storage before clearing");
        call("bindVertexArray",{attached});call("bindBuffer",{0x8892,nullptr});pointer(4);
        require(call("getError").strictEquals(0x0502),"Null ARRAY_BUFFER with nonzero offset reports INVALID_OPERATION");
        require(bindings.snapshot().attributes[0].buffer.sameValue(buffer)&&live==1,"Rejected null pointer leaves the previous attachment alive");
        pointer(0);
        require(call("getError").strictEquals(0),"Null ARRAY_BUFFER and zero offset legally clear a pointer");
        require(bindings.snapshot().attributes[0].buffer.isNull()&&bindings.snapshot().attributes[0].enabled,"Clearing a pointer preserves enabled state for draw-time validation");
        require(live==0&&!buffers.hasStorage(buffer),"Null zero-offset pointer releases the final deleted storage attachment");
        bindings.shutdown();buffers.shutdown();
    }
}
void bindingQueryTests() {
    VulkanBufferCommands buffers([]{return std::make_shared<Memory>();});
    VulkanBindingCommands bindings(buffers.object(),buffers.bindingInterface(),4);const auto gl=bindings.object();
    const auto call=[&](const char* name,const std::vector<JSValue>& args=std::vector<JSValue>{}){return gl.get(name).call(gl,args);};
    const auto query=[&](unsigned name){return call("getParameter",{name});};
    require(gl.get("ARRAY_BUFFER_BINDING").strictEquals(0x8894)&&gl.get("ELEMENT_ARRAY_BUFFER_BINDING").strictEquals(0x8895)&&gl.get("VERTEX_ARRAY_BINDING").strictEquals(0x85b5),"Public binding enums identify the actual context queries");
    require(query(0x8894).isNull()&&query(0x8895).isNull()&&query(0x85b5).isNull(),"All public bindings begin as null");
    const auto a=call("createVertexArray"),b=call("createVertexArray");
    require(query(0x85b5).isNull(),"Creating a VAO does not bind it");
    const auto array=call("createBuffer"),defaultIndex=call("createBuffer"),aIndex=call("createBuffer"),bIndex=call("createBuffer");
    call("bindBuffer",{0x8892,array});call("bindBuffer",{0x8893,defaultIndex});
    require(query(0x8894).sameValue(array)&&query(0x8895).sameValue(defaultIndex),"Default VAO queries return the actual bound buffer handles");
    call("bindVertexArray",{a});
    require(query(0x85b5).sameValue(a)&&query(0x85b5).sameValue(a),"Repeated VAO query returns the original bound object identity");
    require(query(0x8894).sameValue(array)&&query(0x8895).isNull(),"ARRAY_BUFFER is context state while a new VAO has no element binding");
    call("bindBuffer",{0x8893,aIndex});call("bindVertexArray",{b});
    require(query(0x8895).isNull(),"A second VAO owns an independent element binding");
    call("bindBuffer",{0x8893,bIndex});call("bindVertexArray",{a});
    require(query(0x8895).sameValue(aIndex)&&query(0x8894).sameValue(array),"VAO switching restores the element handle without changing ARRAY_BUFFER");
    call("deleteBuffer",{aIndex});
    require(query(0x8895).isNull()&&query(0x85b5).sameValue(a),"Deleting the current element buffer clears its query without unbinding the VAO");
    call("deleteBuffer",{bIndex});
    require(buffers.hasStorage(bIndex)&&query(0x8895).isNull(),"Unbound VAO retains its deleted element storage without affecting current queries");
    call("bindVertexArray",{b});
    require(query(0x8895).sameValue(bIndex)&&query(0x85b5).sameValue(b),"Restored retained element binding queries the original deleted buffer object");
    call("bindBuffer",{0x8893,bIndex});
    require(call("getError").strictEquals(0x0502)&&query(0x8895).sameValue(bIndex),"Rejected deleted buffer rebinding preserves the retained element query");
    call("deleteVertexArray",{b});
    require(query(0x85b5).isNull()&&query(0x8895).sameValue(defaultIndex),"Deleting the bound VAO restores the default VAO and its real element binding");
    require(!buffers.hasStorage(bIndex)&&query(0x8894).sameValue(array),"Deleted VAO releases retained element storage without changing ARRAY_BUFFER");
    auto parameter=JSValue::object();parameter.set("valueOf",JSValue::function(+[](const JSValue&,const auto&,const auto& c)->JSValue {
        const auto gl=c[0];gl.get("bindVertexArray").call(gl,{c[1]});return 0x85b5;
    },{gl,a}));
    require(call("getParameter",{parameter}).sameValue(a),"Query observes VAO changes made during parameter conversion");
    call("bindVertexArray",{nullptr});
    require(query(0x85b5).isNull()&&query(0x8895).sameValue(defaultIndex),"Explicit null VAO binding restores default query state");
    call("deleteVertexArray",{a});call("bindVertexArray",{a});
    require(call("getError").strictEquals(0x0502)&&query(0x85b5).isNull(),"Rejected deleted VAO binding preserves the queried null identity");
    call("deleteBuffer",{array});call("deleteBuffer",{defaultIndex});
    require(query(0x8894).isNull()&&query(0x8895).isNull(),"Deleting current context and default element buffers clears their queries");
    require(call("getError").strictEquals(0),"Valid binding queries do not enqueue GL errors");
    const auto escaped=gl.get("getParameter");bindings.shutdown();buffers.shutdown();bool rejected=false;
    try{escaped.call(gl,{0x85b5});}catch(const JSTypeError&){rejected=true;}
    require(rejected&&live==0,"Escaped binding query rejects shutdown and all attachments are released");
}
int main(){try {
    VulkanBufferCommands buffers([]{return std::make_shared<Memory>();});
    VulkanBindingCommands bindings(buffers.object(),buffers.bindingInterface(),16);auto gl=bindings.object();
    const auto call=[&](const char* name,std::vector<JSValue> args={}){return gl.get(name).call(gl,args);};
    const auto buffer=call("createBuffer"),a=call("createVertexArray"),b=call("createVertexArray");
    call("bindBuffer",{0x8892,buffer});call("bufferData",{0x8892,NativeArray_detail::wrap(Float32Array{1,2,3,4,5,6}),0x88e4});
    for(const auto& vao:{a,b}){call("bindVertexArray",{vao});call("vertexAttribPointer",{0,3,0x1406,false,0,0});call("enableVertexAttribArray",{0});}
    require(live==1,"Two VAOs share one storage");
    call("deleteBuffer",{buffer});
    require(live==1,"Unbound VAO retains deleted storage");
    require(bindings.snapshot().attributes[0].buffer.isNull(),"Deleting buffer detaches current VAO");
    call("bindVertexArray",{a});
    require(bindings.snapshot().attributes[0].buffer.sameValue(buffer),"Unbound VAO preserves deleted buffer identity");
    require(buffers.read(buffer).size()==24,"Retained deleted storage remains readable for drawing");
    call("bindBuffer",{0x8892,buffer});
    require(call("getError").strictEquals(0x0502),"Rebinding deleted name queues INVALID_OPERATION");
    call("vertexAttribPointer",{1,3,0x1406,false,0,4});
    require(call("getError").strictEquals(0x0502),"Failed bind leaves ARRAY_BUFFER null");
    require(bindings.snapshot().attributes[1].buffer.isNull(),"Rejected pointer does not capture deleted name");
    call("deleteBuffer",{buffer});
    require(bindings.snapshot().attributes[0].buffer.sameValue(buffer),"Repeated delete is a no-op even when retained VAO is current");
    call("deleteVertexArray",{a});
    require(live==0,"Deleting final attached VAO releases deleted storage immediately");
    require(!buffers.hasStorage(buffer),"Deleted storage is unavailable after final attachment");
    const auto element=call("createBuffer"),e1=call("createVertexArray"),e2=call("createVertexArray");
    for(const auto& vao:{e1,e2}){call("bindVertexArray",{vao});call("bindBuffer",{0x8893,element});}
    call("bufferData",{0x8893,NativeArray_detail::wrap(Uint16Array{0,1,2}),0x88e4});
    call("deleteBuffer",{element});
    require(bindings.snapshot().index.isNull(),"Delete detaches current element binding");
    call("bindVertexArray",{e1});
    require(bindings.snapshot().index.sameValue(element),"VAO bind restores deleted element storage");
    call("bufferSubData",{0x8893,0,NativeArray_detail::wrap(Uint16Array{2})});
    require(call("getError").strictEquals(0),"Restored deleted element storage accepts writes");
    require(buffers.read(element)[0]==std::byte{2},"Restored binding writes actual retained bytes");
    call("deleteVertexArray",{e1});require(live==0,"Final element attachment releases immediately");
    const auto good=call("createBuffer");call("bindBuffer",{0x8892,good});
    call("bufferData",{0x8892,NativeArray_detail::wrap(Float32Array{1,2,3}),0x88e4});
    call("vertexAttribPointer",{2,3,0x1406,false,0,18446744073709551616.0});
    require(call("getError").strictEquals(0),"GLintptr modulo conversion maps 2^64 to zero");
    require(bindings.snapshot().attributes[2].offset==0,"Converted pointer offset is zero");
    call("vertexAttribPointer",{2,3,0x1406,false,0,-1});
    require(call("getError").strictEquals(0x0501),"Negative pointer offset queues INVALID_VALUE");
    call("deleteBuffer",{good});require(live==0,"Current-only vertex attachment retires immediately");
    auto native=NativeArray_detail::wrap(Float32Array{.25f,.5f,.75f});
    JSPropertyDescriptor shadow;shadow.getter=JSValue::function(+[](const JSValue&,const auto&,const auto&)->JSValue{throw JSTypeError("shadowed length observed");});shadow.configurable=true;
    native.defineProperty("length",shadow);call("vertexAttrib3fv",{3,native});
    require(bindings.snapshot().attributes[3].value==std::array<float,4>{.25f,.5f,.75f,1},"Branded Float32List reads native storage without shadowed length");
    auto sequence=list({.25,.5,.75});call("vertexAttrib3fv",{4,sequence});
    require(bindings.snapshot().attributes[4].value==std::array<float,4>{.25f,.5f,.75f,1},"Array iterator supplies the default float components");
    call("vertexAttrib3fv",{4,list({.25,.5})});
    require(call("getError").strictEquals(0x0501),"A short sequence rejects the default value");
    require(bindings.snapshot().attributes[4].value==std::array<float,4>{.25f,.5f,.75f,1},"Failed default conversion keeps previous generic value");
    auto otherBuffer=VulkanBufferCommands([]{return std::make_shared<Memory>();});
    auto otherBindings=VulkanBindingCommands(otherBuffer.object(),otherBuffer.bindingInterface(),16);auto other=otherBindings.object();
    auto foreign=other.get("createVertexArray").call(other,{});call("bindVertexArray",{foreign});
    require(call("getError").strictEquals(0x0502),"Foreign correctly branded VAO queues INVALID_OPERATION");
    call("deleteVertexArray",{foreign});require(call("getError").strictEquals(0x0502),"Foreign VAO deletion queues INVALID_OPERATION");
    call("bindBuffer",{0x8892,good});bindings.invalidDraw();
    require(call("getError").strictEquals(0x0502),"Buffer and draw commands share one INVALID_OPERATION flag");
    require(call("getError").strictEquals(0),"A shared GL error flag is consumed only once");
    auto changes=JSValue::object();changes.set("calls",0);auto changing=JSValue::object();
    JSPropertyDescriptor changingLength;changingLength.getter=JSValue::function(+[](const JSValue&,const auto&,const auto& captures)->JSValue {
        auto state=captures[0];const auto n=state.get("calls").number()+1;state.set("calls",n);return n==1?JSValue(3):JSValue(NAN);
    },{changes});changingLength.configurable=true;changing.defineProperty("length",changingLength);
    auto material=JSValue::object(),defaults=JSValue::object(),geometry=JSValue::object(),object=JSValue::object(),program=JSValue::object(),attributes=JSValue::object();
    defaults.set("color",changing);material.set("defaultAttributeValues",defaults);material.set("wireframe",false);
    geometry.set("id",1);geometry.set("attributes",JSValue::object());object.set("id",1);program.set("id",1);
    auto inputs=JSValue::object(),input=JSValue::object();input.set("location",5);input.set("locationSize",1);inputs.set("color",input);
    program.set("getAttributes",JSValue::function(+[](const JSValue&,const auto&,const auto& c){return c[0];},{inputs}));
    WebGLBindingStates protocol(gl,attributes);bool defaultRejected=false;
    try{protocol.setup(object,material,program,geometry,nullptr);}catch(const JSTypeError&){defaultRejected=true;}
    require(defaultRejected,"Source-selected default command rejects a noniterable plain object at the WebIDL boundary");
    require(changes.get("calls").strictEquals(1),"Only the source switch reads length before sequence conversion rejects the noniterable");
    require(call("getError").strictEquals(0),"WebIDL TypeError does not enqueue a GL error");
    require(bindings.snapshot().attributes[5].value==std::array<float,4>{0,0,0,1},"Failed sequence conversion leaves the generic constant initialized");
    protocol.dispose();otherBindings.shutdown();otherBuffer.shutdown();
    const auto escaped=gl.get("createVertexArray");bindings.shutdown();buffers.shutdown();
    bool rejected=false;try{escaped.call(gl,{});}catch(const JSTypeError&){rejected=true;}
    require(rejected,"Escaped command rejects retired renderer state");
    prototypeIsolationTests();
    integerConstantTests();
    constantListProtocolTests();
    nullPointerDetachTests();
    bindingQueryTests();
    std::cout<<checks<<" binding command checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

