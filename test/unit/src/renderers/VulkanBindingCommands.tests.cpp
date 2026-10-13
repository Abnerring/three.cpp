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
    call("vertexAttribPointer",{1,3,0x1406,false,0,0});
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
    auto sequence=JSValue::object();sequence.set("length",3.9);sequence.set("0",.25);sequence.set("1",.5);sequence.set("2",.75);
    sequence.defineProperty("3",shadow);call("vertexAttrib3fv",{4,sequence});
    require(bindings.snapshot().attributes[4].value==std::array<float,4>{.25f,.5f,.75f,1},"Sequence ToLength truncates fraction before indexed reads");
    sequence.set("length",NAN);call("vertexAttrib3fv",{4,sequence});
    require(call("getError").strictEquals(0x0501),"NaN sequence length becomes zero and rejects a short default value");
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
    WebGLBindingStates protocol(gl,attributes);protocol.setup(object,material,program,geometry,nullptr);
    require(changes.get("calls").strictEquals(2),"Default value length is read by source switch then native list conversion");
    require(call("getError").strictEquals(0x0501),"Second changed length rejects native default command");
    require(bindings.snapshot().attributes[5].value==std::array<float,4>{0,0,0,1},"Failed second length read leaves generic constant initialized");
    protocol.dispose();otherBindings.shutdown();otherBuffer.shutdown();
    const auto escaped=gl.get("createVertexArray");bindings.shutdown();buffers.shutdown();
    bool rejected=false;try{escaped.call(gl,{});}catch(const JSTypeError&){rejected=true;}
    require(rejected,"Escaped command rejects retired renderer state");
    prototypeIsolationTests();
    std::cout<<checks<<" binding command checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

