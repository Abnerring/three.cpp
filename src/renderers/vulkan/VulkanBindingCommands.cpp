#include "renderers/vulkan/VulkanBindingCommands.h"
#include "core/NativeArray.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace {
using Args = std::vector<JSValue>;
constexpr const char* vaoTag = "VulkanBindingCommands.VertexArray";
constexpr unsigned arrayTarget = 0x8892, elementTarget = 0x8893;
constexpr unsigned invalidEnum = 0x0500, invalidValue = 0x0501, invalidOperation = 0x0502;
struct VertexArray { JSWeakValue context; std::uint64_t id; };
void literal(JSValue target, const char* name, JSValue value) {
    JSPropertyDescriptor d; d.value = std::move(value);
    d.writable = d.enumerable = d.configurable = true; target.defineProperty(name,d);
}
JSValue internal() { auto value=JSValue::object();value.setPrototype(nullptr);return value; }
const JSValue& arg(const Args& args,std::size_t index) {
    static const JSValue missing;return index<args.size()?args[index]:missing;
}
void require(const Args& args,std::size_t count) {
    if(args.size()<count)throw JSTypeError("Insufficient vertex input command arguments");
}
std::uint32_t uint32(const JSValue& value) {
    const auto number=value.toNumber();
    if(!std::isfinite(number) || number==0)return 0;
    auto result=std::fmod(std::trunc(number),4294967296.0);
    if(result<0)result+=4294967296.0;return static_cast<std::uint32_t>(result);
}
std::int32_t int32(const JSValue& value) { return std::bit_cast<std::int32_t>(uint32(value)); }
std::int64_t intptr(const JSValue& value) {
    const auto number=value.toNumber();
    if(!std::isfinite(number) || number==0)return 0;
    const auto residue=std::fmod(std::trunc(std::abs(number)),std::ldexp(1.0,64));
    const auto magnitude=static_cast<std::uint64_t>(residue);
    const auto bits=number<0?std::uint64_t(0)-magnitude:magnitude;
    return std::bit_cast<std::int64_t>(bits);
}
JSValue pointer() {
    auto result=internal();literal(result,"enabled",false);literal(result,"normalized",false);
    literal(result,"integer",false);literal(result,"size",4);literal(result,"type",0x1406);
    literal(result,"divisor",0);literal(result,"stride",0);literal(result,"offset",0);
    literal(result,"buffer",nullptr);literal(result,"attachment",nullptr);return result;
}
JSValue vao(std::uint32_t maximum) {
    auto result=internal(),attributes=internal();
    for(std::uint32_t i=0;i<maximum;++i)attributes.set(std::to_string(i),pointer());
    literal(result,"attributes",attributes);literal(result,"index",nullptr);literal(result,"indexAttachment",nullptr);return result;
}
void open(const JSValue& state) {
    static_cast<void>(JSWeakValue(state));
    if(state.get("closed").toBoolean())throw JSTypeError("Vulkan vertex input commands are shut down");
}
void error(JSValue state,unsigned code) {
    const auto bridge=state.get("bridge");bridge.get("recordError").call(bridge,{code});
}
JSValue current(const JSValue& state) {
    const auto identity=state.get("current");
    return identity.isNull()?state.get("default"):state.get("vaos").get(identity.toString());
}
std::shared_ptr<VertexArray> handle(const JSValue& state,const JSValue& value) {
    if(!value.isHostObject(vaoTag))throw JSTypeError("Expected a native vertex array handle");
    const auto result=std::static_pointer_cast<VertexArray>(value.hostPayload(vaoTag));
    const auto context=result->context.lock();
    if(!context || !context->sameValue(state))return {};
    return result;
}
bool validLocation(const JSValue& state,std::uint32_t location) {
    if(location>=state.get("maximum").number()){error(state,invalidValue);return false;}return true;
}
std::uint32_t bytes(unsigned type) {
    switch(type) {
        case 0x1400:case 0x1401:return 1;
        case 0x1402:case 0x1403:case 0x140b:return 2;
        case 0x1404:case 0x1405:case 0x1406:return 4;
        default:return 0;
    }
}
void releaseVAO(const JSValue& state, JSValue vao) {
    const auto bridge=state.get("bridge"), attributes=vao.get("attributes");
    bridge.get("release").call(bridge,{vao.get("indexAttachment")});
    vao.set("indexAttachment",nullptr);vao.set("index",nullptr);
    for(std::uint32_t i=0;i<state.get("maximum").number();++i) {
        auto pointer=attributes.get(std::to_string(i));
        bridge.get("release").call(bridge,{pointer.get("attachment")});
        pointer.set("attachment",nullptr);pointer.set("buffer",nullptr);
    }
}
JSValue command(const JSValue&,const Args& args,const Args& captures) {
    auto state=captures[0];open(state);const auto name=captures[1].string();
    const auto buffers=state.get("buffers"),bridge=state.get("bridge");
    if(name=="getParameter") {
        require(args,1);const auto parameter=uint32(args[0]);
        if(parameter==0x8869)return state.get("maximum");
        error(state,invalidEnum);return nullptr;
    }
    if(name=="getError") {
        return buffers.get("getError").call(buffers,{});
    }
    if(name=="createVertexArray") {
        const auto id=state.get("next").number();state.set("next",id+1);
        state.get("vaos").set(JSValue(id).toString(),vao(static_cast<std::uint32_t>(state.get("maximum").number())));
        auto payload=std::make_shared<VertexArray>(VertexArray{JSWeakValue(state),static_cast<std::uint64_t>(id)});
        return JSValue::hostObject(payload,payload.get(),vaoTag);
    }
    if(name=="bindVertexArray") {
        require(args,1);
        if(args[0].isNull() || args[0].isUndefined())state.set("current",nullptr);
        else {
            const auto value=handle(state,args[0]);
            if(!value){error(state,invalidOperation);return {};}
            if(!state.get("vaos").hasOwn(std::to_string(value->id))){error(state,invalidOperation);return {};}
            state.set("current",double(value->id));
        }
        bridge.get("restoreElement").call(bridge,{current(state).get("indexAttachment")});return {};
    }
    if(name=="deleteVertexArray") {
        require(args,1);if(args[0].isNull() || args[0].isUndefined())return {};
        const auto value=handle(state,args[0]);
        if(!value){error(state,invalidOperation);return {};}
        if(state.get("current").strictEquals(double(value->id))) {
            state.set("current",nullptr);
            bridge.get("restoreElement").call(bridge,{state.get("default").get("indexAttachment")});
        }
        const auto stored=state.get("vaos").get(std::to_string(value->id));
        if(!stored.isUndefined())releaseVAO(state,stored);
        state.get("vaos").erase(std::to_string(value->id));return {};
    }
    if(name=="bindBuffer") {
        require(args,2);const auto target=uint32(args[0]);
        if(!bridge.get("checkedBind").call(bridge,{target,args[1]}).toBoolean())return {};
        const auto value=args[1].isUndefined()?JSValue(nullptr):args[1];
        if(target==arrayTarget)state.set("array",value);
        else if(target==elementTarget) {
            auto vao=current(state);const auto attached=bridge.get("attach").call(bridge,{value});
            bridge.get("release").call(bridge,{vao.get("indexAttachment")});
            vao.set("indexAttachment",attached);vao.set("index",value);
        }
        return {};
    }
    if(name=="deleteBuffer") {
        require(args,1);
        if(!bridge.get("checkedDelete").call(bridge,{args[0]}).toBoolean())return {};
        if(state.get("array").sameValue(args[0]))state.set("array",nullptr);
        auto vao=current(state);
        if(vao.get("index").sameValue(args[0])) {
            bridge.get("release").call(bridge,{vao.get("indexAttachment")});
            vao.set("index",nullptr);vao.set("indexAttachment",nullptr);
        }
        for(std::uint32_t i=0;i<state.get("maximum").number();++i) {
            auto pointer=vao.get("attributes").get(std::to_string(i));
            if(!pointer.get("buffer").sameValue(args[0]))continue;
            bridge.get("release").call(bridge,{pointer.get("attachment")});
            pointer.set("buffer",nullptr);pointer.set("attachment",nullptr);
        }
        return {};
    }
    if(name=="enableVertexAttribArray" || name=="disableVertexAttribArray") {
        require(args,1);const auto location=uint32(args[0]);if(!validLocation(state,location))return {};
        current(state).get("attributes").get(std::to_string(location)).set("enabled",name=="enableVertexAttribArray");return {};
    }
    if(name=="vertexAttribDivisor") {
        require(args,2);const auto location=uint32(args[0]);const auto divisor=uint32(args[1]);
        if(!validLocation(state,location))return {};
        current(state).get("attributes").get(std::to_string(location)).set("divisor",divisor);return {};
    }
    if(name=="vertexAttribPointer" || name=="vertexAttribIPointer") {
        const bool integer=name=="vertexAttribIPointer";require(args,integer?5:6);
        const auto location=uint32(args[0]);const auto size=int32(args[1]);const auto type=uint32(args[2]);
        const bool normalized=integer?false:args[3].toBoolean();
        const auto stride=int32(args[integer?3:4]);const auto offset=intptr(args[integer?4:5]);
        if(!validLocation(state,location))return {};
        const auto width=bytes(type);
        if(!width || (integer && (type==0x1406 || type==0x140b))){error(state,invalidEnum);return {};}
        if(size<1 || size>4 || stride<0 || stride>255 || offset<0){error(state,invalidValue);return {};}
        if(stride%width!=0 || offset%width!=0 || state.get("array").isNull()) {
            error(state,invalidOperation);return {};
        }
        auto attribute=current(state).get("attributes").get(std::to_string(location));
        attribute.set("size",size);attribute.set("type",type);attribute.set("normalized",normalized);
        attribute.set("integer",integer);attribute.set("stride",stride);attribute.set("offset",double(offset));
        const auto attached=bridge.get("attach").call(bridge,{state.get("array")});
        bridge.get("release").call(bridge,{attribute.get("attachment")});
        attribute.set("attachment",attached);attribute.set("buffer",state.get("array"));return {};
    }
    if(name.starts_with("vertexAttrib") && name.ends_with("fv")) {
        require(args,2);const auto location=uint32(args[0]);const unsigned count=name[12]-'0';
        const auto value=args[1];
        auto converted=internal();
        std::uint64_t length=0;
        const auto native=NativeArray_detail::unwrap(value);
        if(const auto floats=std::get_if<Float32Array>(&native.storage())) {
            length=floats->size();
            for(std::uint64_t i=0;i<length;++i)converted.set(std::to_string(i),double((*floats)[std::size_t(i)]));
        } else {
            const auto number=value.get("length").toNumber();
            if(number>0)length=static_cast<std::uint64_t>(std::min(std::floor(number),9007199254740991.0));
            for(std::uint64_t i=0;i<length;++i)converted.set(std::to_string(i),double(float(value.get(std::to_string(i)).toNumber())));
        }
        if(!validLocation(state,location))return {};
        if(length<count){error(state,invalidValue);return {};}
        auto components=internal();
        for(unsigned i=0;i<4;++i)components.set(std::to_string(i),i<count?converted.get(std::to_string(i)):JSValue(i==3?1:0));
        state.get("constants").set(std::to_string(location),components);return {};
    }
    throw JSTypeError("Unknown vertex input command");
}
}
VulkanBindingCommands::VulkanBindingCommands(JSValue buffers,JSValue bridge,std::uint32_t maximumAttributes) {
    if(!maximumAttributes)throw std::invalid_argument("Vulkan requires vertex input locations");
    state_=internal();literal(state_,"buffers",buffers);literal(state_,"bridge",bridge);literal(state_,"maximum",maximumAttributes);
    literal(state_,"closed",false);literal(state_,"current",nullptr);literal(state_,"array",nullptr);
    literal(state_,"vaos",internal());literal(state_,"default",vao(maximumAttributes));
    literal(state_,"next",1);
    auto constants=internal();
    for(std::uint32_t i=0;i<maximumAttributes;++i){auto value=internal();for(unsigned c=0;c<4;++c)value.set(std::to_string(c),c==3?1:0);constants.set(std::to_string(i),value);}
    literal(state_,"constants",constants);
    object_=JSValue::object();object_.setPrototype(buffers);literal(object_,"MAX_VERTEX_ATTRIBS",0x8869);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{
        {"getParameter",1},{"getError",0},{"createVertexArray",0},{"bindVertexArray",1},{"deleteVertexArray",1},
        {"bindBuffer",2},{"deleteBuffer",1},{"enableVertexAttribArray",1},{"disableVertexAttribArray",1},{"vertexAttribDivisor",2},
        {"vertexAttribPointer",6},{"vertexAttribIPointer",5},{"vertexAttrib1fv",2},{"vertexAttrib2fv",2},
        {"vertexAttrib3fv",2},{"vertexAttrib4fv",2}})
        literal(object_,name,JSValue::function(command,{state_,name},name,arity));
}
JSValue VulkanBindingCommands::object() const {open(state_);return object_;}
VulkanBindingCommands::Snapshot VulkanBindingCommands::snapshot() const {
    open(state_);Snapshot result;const auto state=current(state_);result.index=state.get("index");
    for(std::uint32_t i=0;i<state_.get("maximum").number();++i) {
        const auto name=std::to_string(i);
        const auto source=state.get("attributes").get(name),constant=state_.get("constants").get(name);
        Attribute a;a.enabled=source.get("enabled").toBoolean();a.normalized=source.get("normalized").toBoolean();
        a.integer=source.get("integer").toBoolean();a.size=uint32(source.get("size"));a.type=uint32(source.get("type"));
        a.divisor=uint32(source.get("divisor"));a.stride=static_cast<std::uint64_t>(source.get("stride").number());
        a.offset=static_cast<std::uint64_t>(source.get("offset").number());a.buffer=source.get("buffer");
        for(unsigned c=0;c<4;++c)a.value[c]=float(constant.get(std::to_string(c)).number());result.attributes.push_back(std::move(a));
    }
    return result;
}
void VulkanBindingCommands::shutdown() {
    if(state_.get("closed").toBoolean())return;
    const auto vaos=state_.get("vaos");
    for(const auto& name:vaos.ownPropertyNames())releaseVAO(state_,vaos.get(name));
    releaseVAO(state_,state_.get("default"));
    state_.get("bridge").get("restoreElement").call(state_.get("bridge"),{nullptr});
    state_.set("closed",true);state_.set("array",nullptr);state_.set("vaos",internal());
    state_.set("default",vao(static_cast<std::uint32_t>(state_.get("maximum").number())));
}
void VulkanBindingCommands::invalidDraw() { open(state_);error(state_,invalidOperation); }
