#include "renderers/vulkan/VulkanBufferCommands.h"
#include "core/NativeArray.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>

namespace {
using Args = std::vector<JSValue>;
using Storage = VulkanBufferCommands::Storage;
constexpr const char* stateTag = "VulkanBufferCommands.State";
constexpr const char* bufferTag = "VulkanBufferCommands.Buffer";
constexpr const char* attachmentTag = "VulkanBufferCommands.Attachment";
constexpr std::uint32_t arrayTarget = 0x8892, elementTarget = 0x8893;
constexpr std::uint32_t invalidEnum = 0x0500, invalidValue = 0x0501;
constexpr std::uint32_t invalidOperation = 0x0502, outOfMemory = 0x0505;
struct State;
struct Handle {
    std::weak_ptr<State> owner;
    std::shared_ptr<Storage> storage;
    std::optional<std::uint32_t> target;
    std::uint64_t revision = 0;
    bool deleted = false;
    std::size_t attachments = 0;
};
struct Attachment {
    std::shared_ptr<Handle> handle;
    explicit Attachment(std::shared_ptr<Handle> value) : handle(std::move(value)) { ++handle->attachments; }
    void release() {
        if (!handle) return;
        if (--handle->attachments == 0 && handle->deleted) handle->storage.reset();
        handle.reset();
    }
    ~Attachment() { release(); }
};
struct State {
    VulkanBufferCommands::Factory factory;
    std::vector<std::weak_ptr<Handle>> handles;
    std::size_t nextPrune = 64;
    std::array<std::shared_ptr<Handle>, 2> bindings;
    std::array<bool, 6> errors{};
    bool closed = false;
    explicit State(VulkanBufferCommands::Factory value) : factory(std::move(value)) {}
    void close() {
        if (closed) return;
        closed = true;
        for (const auto& weak : handles) if (const auto handle = weak.lock()) {
            handle->deleted = true;
            handle->storage.reset();
        }
        bindings = {};
        handles.clear();
        factory = {};
    }
    ~State() { close(); }
    void requireOpen() const { if (closed) throw JSTypeError("Vulkan buffer commands are shut down"); }
    void error(std::uint32_t value) { errors[value - invalidEnum] = true; }
    std::uint32_t getError() {
        for (std::size_t i = 0; i < errors.size(); ++i) if (errors[i]) {
            errors[i] = false; return invalidEnum + std::uint32_t(i);
        }
        return 0;
    }
};
const JSValue& argument(const Args& args, std::size_t i) {
    static const JSValue undefined;
    return i < args.size() ? args[i] : undefined;
}
void requireArgs(const Args& args, std::size_t count) {
    if (args.size() < count) throw JSTypeError("Insufficient buffer command arguments");
}
std::shared_ptr<State> state(const JSValue& value) {
    static_cast<void>(JSWeakValue(value));
    return std::static_pointer_cast<State>(value.hostPayload(stateTag));
}
std::shared_ptr<Handle> handle(const JSValue& value, bool nullable = false) {
    static_cast<void>(JSWeakValue(value));
    if (nullable && (value.isNull() || value.isUndefined())) return {};
    if (!value.isHostObject(bufferTag)) throw JSTypeError("Expected a native Vulkan buffer handle");
    return std::static_pointer_cast<Handle>(value.hostPayload(bufferTag));
}
bool owned(const std::shared_ptr<State>& context, const std::shared_ptr<Handle>& value) {
    return value && value->owner.lock() == context;
}
std::size_t bindingIndex(std::uint32_t target) { return target == arrayTarget ? 0 : 1; }
bool validTarget(State& context, std::uint32_t target) {
    if (target == arrayTarget || target == elementTarget) return true;
    context.error(invalidEnum); return false;
}
bool validUsage(State& context, std::uint32_t usage) {
    switch (usage) {
        case 0x88e0: case 0x88e1: case 0x88e2: // STREAM_DRAW/READ/COPY
        case 0x88e4: case 0x88e5: case 0x88e6: // STATIC_DRAW/READ/COPY
        case 0x88e8: case 0x88e9: case 0x88ea: return true; // DYNAMIC_DRAW/READ/COPY
        default: context.error(invalidEnum); return false;
    }
}
// WebIDL ConvertToInt without EnforceRange/Clamp. Handle negative residues with
// unsigned arithmetic: adding 2^64 in double would round -1's residue to 2^64.
std::uint64_t unsignedInteger(const JSValue& value, unsigned bits) {
    const double x = value.toNumber();
    if (!std::isfinite(x) || x == 0) return 0;
    const double modulus = std::ldexp(1.0, int(bits));
    const double residue = std::fmod(std::trunc(std::abs(x)), modulus);
    const auto magnitude = static_cast<std::uint64_t>(residue);
    const auto result = x < 0 ? std::uint64_t(0) - magnitude : magnitude;
    return bits == 32 ? result & UINT64_C(0xffffffff) : result;
}
InterpolantArray view(const JSValue& value) {
    static_cast<void>(JSWeakValue(value));
    auto result = NativeArray_detail::unwrap(value);
    if (std::holds_alternative<JSValue>(result.storage()))
        throw JSTypeError("Attribute buffer commands require a native ArrayBufferView");
    return result;
}
struct ViewInfo { std::size_t length, elementSize; };
ViewInfo viewInfo(const InterpolantArray& source) {
    return std::visit([](const auto& values) -> ViewInfo {
        using T = std::decay_t<decltype(values)>;
        if constexpr (std::is_same_v<T, JSValue>) throw JSTypeError("Expected ArrayBufferView");
        else return {values.size(), T::BYTES_PER_ELEMENT};
    }, source.storage());
}
std::vector<std::byte> copyBytes(const InterpolantArray& source, std::size_t first, std::size_t count) {
    return std::visit([&](const auto& values) -> std::vector<std::byte> {
        using T = std::decay_t<decltype(values)>;
        if constexpr (std::is_same_v<T, JSValue>) throw JSTypeError("Expected ArrayBufferView");
        else {
            const Uint8Array bytes(values.buffer, double(std::size_t(values.byteOffset)), double(std::size_t(values.byteLength)));
            std::vector<std::byte> result(count);
            for (std::size_t i = 0; i < count; ++i) result[i] = std::byte(bytes[first + i]);
            return result;
        }
    }, source.storage());
}
std::shared_ptr<Handle> bound(State& context, std::uint32_t target) {
    if (!validTarget(context, target)) return {};
    const auto value = context.bindings[bindingIndex(target)];
    if (!value || !value->storage) { context.error(invalidOperation); return {}; }
    return value;
}
void requireRevision(const Handle& value) {
    if (value.revision == std::numeric_limits<std::uint64_t>::max())
        throw JSRangeError("Vulkan buffer revision exhausted");
}
JSNativeOwnerNode handleNode(const std::shared_ptr<Handle>& value) {
    JSNativeOwnerNode result{value, {}, {}};
    if (value->storage) result.edges.push_back({&value->storage, value->storage});
    return result;
}
std::shared_ptr<Attachment> attachment(const JSValue& value) {
    static_cast<void>(JSWeakValue(value));
    if (!value.isHostObject(attachmentTag)) throw JSTypeError("Expected a native buffer attachment");
    return std::static_pointer_cast<Attachment>(value.hostPayload(attachmentTag));
}
JSValue attach(const std::shared_ptr<State>& context, const JSValue& buffer) {
    if (buffer.isNull()) return nullptr;
    const auto target = handle(buffer);
    if (!owned(context, target) || target->deleted || !target->storage)
        throw JSTypeError("Cannot attach an unavailable buffer");
    auto value = std::make_shared<Attachment>(target);
    auto result = JSValue::hostObject(value, value.get(), attachmentTag);
    static const auto hooks = [] {
        auto h = std::make_shared<JSHostObjectHooks>();
        h->nativeOwnership = +[](const JSValue& self) {
            const auto value = attachment(self);
            JSNativeOwnerNode node{value, {}, {}};
            std::vector<JSNativeOwnerNode> nodes;
            if (value->handle) {
                node.edges.push_back({&value->handle, value->handle});
                nodes.push_back(handleNode(value->handle));
                if (value->handle->storage) nodes.push_back({value->handle->storage, {}, {}});
            }
            nodes.push_back(std::move(node)); return nodes;
        };
        return h;
    }();
    result.setHostHooks(hooks); return result;
}
JSValue create(const std::shared_ptr<State>& context) {
    auto value = std::make_shared<Handle>();
    value->owner = context;
    const auto factory = context->factory;
    try { value->storage = factory(); }
    catch (const std::bad_alloc&) { context->requireOpen(); context->error(outOfMemory); return nullptr; }
    context->requireOpen();
    if (!value->storage) { context->error(outOfMemory); return nullptr; }
    auto result = JSValue::hostObject(value, value.get(), bufferTag);
    static const auto hooks = [] {
        auto h = std::make_shared<JSHostObjectHooks>();
        h->nativeOwnership = +[](const JSValue& self) {
            const auto value = handle(self);
            std::vector<JSNativeOwnerNode> nodes{handleNode(value)};
            if (value->storage) nodes.push_back({value->storage, {}, {}});
            return nodes;
        };
        return h;
    }();
    result.setHostHooks(hooks);
    if (context->handles.size() >= context->nextPrune) {
        std::erase_if(context->handles, [](const auto& weak) { return weak.expired(); });
        context->nextPrune = context->handles.size() > std::numeric_limits<std::size_t>::max() / 2
            ? std::numeric_limits<std::size_t>::max() : std::max<std::size_t>(64, context->handles.size() * 2);
    }
    context->handles.emplace_back(value);
    return result;
}
JSValue command(const JSValue&, const Args& args, const Args& captures) {
    const auto context = state(captures[0]);
    context->requireOpen();
    const auto name = captures[1].string();
    if(name=="recordError") {
        requireArgs(args,1);const auto code=std::uint32_t(unsignedInteger(args[0],32));
        if(code<invalidEnum || code>outOfMemory)throw JSRangeError("Invalid internal GL error flag");
        context->error(code);return {};
    }
    if (name == "attach") { requireArgs(args,1); return attach(context,args[0]); }
    if (name == "release") {
        requireArgs(args,1);if (!args[0].isNull()) attachment(args[0])->release(); return {};
    }
    if (name == "restoreElement") {
        requireArgs(args,1);
        const auto value = args[0].isNull() ? std::shared_ptr<Handle>{} : attachment(args[0])->handle;
        if (value && !owned(context,value)) throw JSTypeError("Foreign buffer attachment");
        context->bindings[1] = value; return {};
    }
    if (name == "createBuffer") return create(context);
    if (name == "getError") return context->getError();
    if (name == "bindBuffer" || name == "checkedBind") {
        requireArgs(args, 2);
        const auto target = std::uint32_t(unsignedInteger(args[0], 32));
        const auto value = handle(args[1], true);
        context->requireOpen();
        bool success = validTarget(*context,target);
        if (success && value && (!owned(context,value) || value->deleted || (value->target && *value->target != target))) {
            context->error(invalidOperation);success=false;
        }
        if (success) {
            if (value) value->target=target;
            context->bindings[bindingIndex(target)]=value;
        }
        return name=="checkedBind" ? JSValue(success) : JSValue{};
    }
    if (name == "deleteBuffer" || name == "checkedDelete") {
        requireArgs(args, 1);const auto value = handle(args[0], true);
        bool success = false;
        if (value) {
            if (!owned(context,value)) context->error(invalidOperation);
            else if (!value->deleted) {
                success=true;value->deleted=true;
                for(auto& binding:context->bindings)if(binding==value)binding.reset();
                if(!value->attachments)value->storage.reset();
            }
        }
        return name=="checkedDelete" ? JSValue(success) : JSValue{};
    }
    requireArgs(args, 3);
    const auto target = std::uint32_t(unsignedInteger(args[0], 32));
    if (name == "bufferData") {
        // This internal command set deliberately exposes only the view overload.
        if (args.size() >= 4) throw JSTypeError("Attribute bufferData does not expose source-range overloads");
        const auto source = view(args[1]);
        const auto usage = std::uint32_t(unsignedInteger(args[2], 32));
        context->requireOpen();
        if (!validTarget(*context, target) || !validUsage(*context, usage)) return {};
        const auto destination = bound(*context, target);if (!destination) return {};
        const auto info = viewInfo(source);
        const auto bytes = copyBytes(source, 0, info.length * info.elementSize);
        requireRevision(*destination);
        const auto storage = destination->storage;
        storage->setData(bytes);
        ++destination->revision;
        return {};
    }
    // Conversion order is observable. Do not get the binding or copy source
    // bytes before srcOffset / length converters have had a chance to reenter.
    const auto destinationOffsetBits = unsignedInteger(args[1], 64);
    const auto source = view(args[2]);
    const auto sourceOffset = args.size() >= 4 ? unsignedInteger(args[3], 64) : 0;
    const auto length = args.size() >= 4 ? unsignedInteger(argument(args, 4), 32) : 0;
    context->requireOpen();
    const auto destination = bound(*context, target);if (!destination) return {};
    const auto info = viewInfo(source);
    if (destinationOffsetBits >= (UINT64_C(1) << 63) || sourceOffset > info.length) {
        context->error(invalidValue);return {};
    }
    const auto copyLength = length == 0 ? info.length - std::size_t(sourceOffset) : length;
    if (copyLength > info.length - std::size_t(sourceOffset)) { context->error(invalidValue);return {}; }
    const auto byteCount = std::size_t(copyLength) * info.elementSize;
    const auto size = destination->storage->size();
    if (destinationOffsetBits > size || byteCount > size - std::size_t(destinationOffsetBits)) {
        context->error(invalidValue);return {};
    }
    if (!byteCount) return {};
    const auto bytes = copyBytes(source, std::size_t(sourceOffset) * info.elementSize, byteCount);
    requireRevision(*destination);
    const auto storage = destination->storage;
    storage->setSubData(std::size_t(destinationOffsetBits), bytes);
    ++destination->revision;
    return {};
}
void property(JSValue object, const char* key, JSValue value) {
    JSPropertyDescriptor d;d.value = std::move(value);
    d.writable = d.enumerable = d.configurable = true;object.defineProperty(key, d);
}
std::shared_ptr<Handle> readable(const JSValue& stateValue, const JSValue& buffer) {
    const auto context = state(stateValue);context->requireOpen();
    const bool isAttachment=buffer.isHostObject(attachmentTag);
    const auto value = isAttachment ? attachment(buffer)->handle : handle(buffer);
    if (!owned(context, value) || (!isAttachment && value->deleted && !value->attachments) || !value->storage)
        throw JSTypeError("Vulkan buffer is foreign, deleted, or unavailable");
    return value;
}
}
VulkanBufferCommands::VulkanBufferCommands(Factory factory) : object_(JSValue::object()) {
    if (!factory) throw std::invalid_argument("Vulkan buffer storage factory is required");
    auto context = std::make_shared<State>(std::move(factory));
    state_ = JSValue::hostObject(context, context.get(), stateTag);
    static const auto hooks = [] {
        auto h = std::make_shared<JSHostObjectHooks>();
        h->nativeOwnership = +[](const JSValue& self) {
            const auto context = state(self);JSNativeOwnerNode node{context, {}, {}};
            std::vector<JSNativeOwnerNode> nodes;
            for (const auto& binding : context->bindings) if (binding) {
                node.edges.push_back({&binding, binding});nodes.push_back(handleNode(binding));
                if (binding->storage) nodes.push_back({binding->storage, {}, {}});
            }
            nodes.push_back(std::move(node));return nodes;
        };
        return h;
    }();
    state_.setHostHooks(hooks);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
        {"createBuffer",0},{"bindBuffer",2},{"bufferData",3},{"bufferSubData",3},{"deleteBuffer",1},{"getError",0}})
        property(object_, name, JSValue::function(command, {state_, name}, name, arity));
    for (const auto& [name, value] : std::initializer_list<std::pair<const char*, std::uint32_t>>{
        {"ARRAY_BUFFER",arrayTarget},{"ELEMENT_ARRAY_BUFFER",elementTarget},
        {"BYTE",0x1400},{"UNSIGNED_BYTE",0x1401},{"SHORT",0x1402},{"UNSIGNED_SHORT",0x1403},
        {"INT",0x1404},{"UNSIGNED_INT",0x1405},{"FLOAT",0x1406},{"HALF_FLOAT",0x140b},
        {"STREAM_DRAW",0x88e0},{"STREAM_READ",0x88e1},{"STREAM_COPY",0x88e2},
        {"STATIC_DRAW",0x88e4},{"STATIC_READ",0x88e5},{"STATIC_COPY",0x88e6},
        {"DYNAMIC_DRAW",0x88e8},{"DYNAMIC_READ",0x88e9},{"DYNAMIC_COPY",0x88ea},
        {"NO_ERROR",0},{"INVALID_ENUM",invalidEnum},{"INVALID_VALUE",invalidValue},
        {"INVALID_OPERATION",invalidOperation},{"OUT_OF_MEMORY",outOfMemory}})
        property(object_, name, value);
}
JSValue VulkanBufferCommands::object() const {
    static_cast<void>(JSWeakValue(object_));return object_;
}
JSValue VulkanBufferCommands::bindingInterface() const {
    state(state_)->requireOpen();auto result=JSValue::object();result.setPrototype(nullptr);
    for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{
        {"attach",1},{"release",1},{"restoreElement",1},{"checkedBind",2},{"checkedDelete",1},{"recordError",1}})
        property(result,name,JSValue::function(command,{state_,name},name,arity));
    return result;
}
std::uint64_t VulkanBufferCommands::revision(const JSValue& buffer) const { return readable(state_, buffer)->revision; }
std::vector<std::byte> VulkanBufferCommands::read(const JSValue& buffer) const { return readable(state_, buffer)->storage->read(); }
bool VulkanBufferCommands::hasStorage(const JSValue& buffer) const {
    const auto context=state(state_);context->requireOpen();
    const bool isAttachment=buffer.isHostObject(attachmentTag);
    if(!isAttachment && !buffer.isHostObject(bufferTag))return false;
    const auto value=isAttachment ? attachment(buffer)->handle : handle(buffer);
    return owned(context,value) && (isAttachment || !value->deleted || value->attachments) && bool(value->storage);
}
void VulkanBufferCommands::shutdown() { state(state_)->close(); }
