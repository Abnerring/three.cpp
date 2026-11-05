#include "nodes/gpgpu/WorkgroupInfoNode.h"

#include "nodes/core/Node.h"
#include "nodes/core/StackTrace.h"
#include "nodes/gpgpu/GpgpuNodeSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/utils/ArrayElementNode.h"
#include "utils.h"

#include <cctype>
#include <initializer_list>
#include <string>
#include <tuple>

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// The two class constructors are needed by element() and by the module export.
JSValue& elementNodeCtorKeeper() { static thread_local JSValue value; return value; }

std::string lowercased(const std::string& text) {
    std::string out = text;
    for (auto& ch : out) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return out;
}

// --- WorkgroupInfoElementNode ---------------------------------------------------------------------------

// generate( builder, output ) { ... }
JSValue elementGenerate(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto output = arg(args, 1, nullptr);
    const bool isAssignContext = invoke(builder, "isContextAssign")({}).toBoolean();
    // snippet = super.generate( builder )
    const auto basePrototype = nativeArrayElementNodeConstructor().get("prototype");
    auto snippet = basePrototype.get("generate").call(self, {builder});
    if (!isAssignContext) {
        const auto type = invoke(self, "getNodeType")({builder});
        snippet = invoke(builder, "format")({snippet, type, output});
    }
    return snippet;
}

JSValue makeElementClass() {
    if (elementNodeCtorKeeper().toBoolean()) return elementNodeCtorKeeper();
    const auto base = nativeArrayElementNodeConstructor();
    auto holder = JSValue::object(nullptr);
    // The class declares no static type getter, so none is registered: type resolves through the base.
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            // super( workgroupInfoNode, indexNode )
            auto self = superConstruct(target, captures,
                                       {arg(args, 0, nullptr), arg(args, 1, nullptr)});
            self.set("isWorkgroupInfoElementNode", true);
            return self;
        },
        {holder}, "WorkgroupInfoElementNode", 2);
    holder.set("constructor", ctor);
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    prototype.set("constructor", ctor);
    ctor.setFunctionSourceText(WorkgroupInfoElementNodeSource::classSource);
    JSPropertyDescriptor generateDescriptor;
    generateDescriptor.writable = generateDescriptor.configurable = true;
    generateDescriptor.value = JSValue::function(elementGenerate, {}, "generate", 2);
    generateDescriptor.value->setFunctionSourceText(WorkgroupInfoElementNodeSource::generateSource);
    prototype.defineProperty("generate", generateDescriptor);
    elementNodeCtorKeeper() = ctor;
    return elementNodeCtorKeeper();
}

// --- WorkgroupInfoNode ----------------------------------------------------------------------------------

// isCacheable( /*builder*/ ) { return false; }
JSValue isCacheable(const JSValue&, const Args&, const Args&) { return JSValue(false); }

// setName( name ) { this.name = name; return this; }
JSValue setName(const JSValue& self, const Args& args, const Args&) {
    auto node = self;
    node.set("name", arg(args, 0, nullptr));
    return self;
}

// label( name ) { warn( 'TSL: "label()" has been deprecated. Use "setName()" instead.', new StackTrace() ); return this.setName( name ); }
JSValue label(const JSValue& self, const Args& args, const Args&) {
    ThreeUtils::warn(
        {JSValue("TSL: \"label()\" has been deprecated. Use \"setName()\" instead."),
         nativeStackTraceConstructor().construct()});
    return invoke(self, "setName")({arg(args, 0, nullptr)});
}

// setScope( scope ) { this.scope = scope; return this; }
JSValue setScope(const JSValue& self, const Args& args, const Args&) {
    auto node = self;
    node.set("scope", arg(args, 0, nullptr));
    return self;
}

// setAtomic( value ) { this.isAtomic = value; return this; }
JSValue setAtomic(const JSValue& self, const Args& args, const Args&) {
    auto node = self;
    node.set("isAtomic", arg(args, 0, nullptr));
    return self;
}

// toAtomic() { return this.setAtomic( true ); }
JSValue toAtomic(const JSValue& self, const Args&, const Args&) {
    return invoke(self, "setAtomic")({JSValue(true)});
}

// getElementType() { return this.elementType; }
JSValue getElementType(const JSValue& self, const Args&, const Args&) {
    return self.get("elementType");
}

// getInputType( /*builder*/ ) { return `${this.scope}Array`; }
JSValue getInputType(const JSValue& self, const Args&, const Args&) {
    return JSValue(self.get("scope").toString() + "Array");
}

// element( indexNode ) { return new WorkgroupInfoElementNode( this, indexNode ); }
JSValue element(const JSValue& self, const Args& args, const Args&) {
    return makeElementClass().construct({self, arg(args, 0, nullptr)});
}

// generate( builder ) { ... }
JSValue generate(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    if (!builder.get("shaderStage").strictEquals("compute")) {
        ThreeUtils::error(
            {JSValue("TSL: \"workgroupArray()\" can only be executed within the compute shader stage")});
    }
    const auto name = self.get("name").toString().empty()
        ? self.get("scope").toString() + "Array_" + self.get("id").toString()
        : self.get("name").toString();
    return invoke(builder, "getScopedArray")({
        JSValue(name), JSValue(lowercased(self.get("scope").toString())), self.get("bufferType"),
        self.get("bufferCount"), self.get("isAtomic")});
}
}  // namespace

JSValue nativeWorkgroupInfoNodeModule() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "WorkgroupInfoNode module");

    auto bindings = JSValue::object(nullptr);
    const auto base = nativeNodeConstructor();
    // Like the element class, this one declares no static type getter.
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            // super( bufferType )
            auto self = superConstruct(target, captures, {arg(args, 1, nullptr)});
            self.set("bufferType", arg(args, 1, nullptr));
            self.set("bufferCount", arg(args, 2, JSValue(0.0)));
            self.set("isAtomic", false);
            self.set("isWorkgroupInfoNode", true);
            self.set("elementType", arg(args, 1, nullptr));
            self.set("scope", arg(args, 0, nullptr));
            self.set("name", JSValue(""));
            return self;
        },
        {holder}, "WorkgroupInfoNode", 2);
    holder.set("constructor", ctor);
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    prototype.set("constructor", ctor);
    ctor.setFunctionSourceText(WorkgroupInfoNodeSource::classSource);
    method(prototype, "isCacheable", 0, isCacheable, {}, WorkgroupInfoNodeSource::isCacheableSource);
    method(prototype, "setName", 1, setName, {}, WorkgroupInfoNodeSource::setNameSource);
    method(prototype, "label", 1, label, {}, WorkgroupInfoNodeSource::labelSource);
    method(prototype, "setScope", 1, setScope, {}, WorkgroupInfoNodeSource::setScopeSource);
    method(prototype, "setAtomic", 1, setAtomic, {}, WorkgroupInfoNodeSource::setAtomicSource);
    method(prototype, "toAtomic", 0, toAtomic, {}, WorkgroupInfoNodeSource::toAtomicSource);
    method(prototype, "getElementType", 0, getElementType, {},
           WorkgroupInfoNodeSource::getElementTypeSource);
    method(prototype, "getInputType", 0, getInputType, {},
           WorkgroupInfoNodeSource::getInputTypeSource);
    method(prototype, "element", 1, element, {}, WorkgroupInfoNodeSource::elementSource);
    method(prototype, "generate", 1, generate, {}, WorkgroupInfoNodeSource::generateSource);

    // export const workgroupArray = ( type, count ) => new WorkgroupInfoNode( 'Workgroup', type, count );
    const auto workgroupArray = JSValue::function(
        +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            return captures[0].construct(
                {JSValue("Workgroup"), arg(args, 0, nullptr), arg(args, 1, JSValue(0.0))});
        },
        {ctor}, "workgroupArray", 2);

    own(bindings, "default", ctor);
    own(bindings, "workgroupArray", workgroupArray);
    return publish(bindings, lexical(), weak);
}

JSValue nativeWorkgroupInfoNodeBinding(const std::string& name) {
    const auto keeper = nativeWorkgroupInfoNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeWorkgroupInfoNodeConstructor() {
    return nativeWorkgroupInfoNodeBinding("default");
}

JSValue nativeWorkgroupInfoElementNodeConstructor() { return makeElementClass(); }
