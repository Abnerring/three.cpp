// Native port of three.js Texture3DNode.
#include "nodes/accessors/Texture3DNode.h"

#include "nodes/accessors/TextureNode.h"
#include "nodes/core/VarNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "textures/Texture.h"

#include <algorithm>
#include <initializer_list>
#include <stdexcept>

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue branchAssign(const JSValue&, const Args&, const Args& captures) {
    invoke(captures[0], "assign")({captures[1]});
    return {};
}

JSValue normalBody(const JSValue&, const Args& args, const Args& captures) {
    const auto inputs = arg(args, 0);
    const auto texture = inputs.get("texture");
    const auto uv = inputs.get("uv");
    const auto vec3 = captures[0];
    const auto ifFunction = captures[1];
    const auto ret = invoke(vec3.call({}, {}), "toVar")();
    const auto epsilon = 0.0001;
    const auto edge = 1.0 - epsilon;

    const auto makeVector = [&](const Args& values) { return vec3.call({}, values); };
    const auto makeBranch = [&](const JSValue& value) {
        return JSValue::function(branchAssign, {ret, value}, "", 0);
    };
    auto chain = ifFunction.call({}, {invoke(uv.get("x"), "lessThan")({epsilon}),
                                      makeBranch(makeVector({1, 0, 0}))});
    invoke(chain, "ElseIf")({invoke(uv.get("y"), "lessThan")({epsilon}),
                               makeBranch(makeVector({0, 1, 0}))});
    invoke(chain, "ElseIf")({invoke(uv.get("z"), "lessThan")({epsilon}),
                               makeBranch(makeVector({0, 0, 1}))});
    invoke(chain, "ElseIf")({invoke(uv.get("x"), "greaterThan")({edge}),
                               makeBranch(makeVector({-1, 0, 0}))});
    invoke(chain, "ElseIf")({invoke(uv.get("y"), "greaterThan")({edge}),
                               makeBranch(makeVector({0, -1, 0}))});
    invoke(chain, "ElseIf")({invoke(uv.get("z"), "greaterThan")({edge}),
                               makeBranch(makeVector({0, 0, -1}))});

    const auto step = 0.01;
    const auto sample = [&](const Args& delta) {
        const auto offset = invoke(uv, "add")({makeVector(delta)});
        return invoke(invoke(texture, "sample")({offset}), "r")();
    };
    const auto x = invoke(sample({-step, 0, 0}), "sub")({sample({step, 0, 0})});
    const auto y = invoke(sample({0, -step, 0}), "sub")({sample({0, step, 0})});
    const auto z = invoke(sample({0, 0, -step}), "sub")({sample({0, 0, step})});
    invoke(chain, "Else")({makeBranch(makeVector({x, y, z}))});
    return invoke(ret, "normalize")();
}

JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    const auto self = receiver;
    const auto name = captures[0].string();
    if (name == "getInputType") return "texture3D";
    if (name == "getDefaultUV") return captures[2].call({}, {.5, .5, .5});
    if (name == "setUpdateMatrix") return {};
    if (name == "generateUV") {
        const auto builder = arg(args, 0);
        const auto uv = arg(args, 1);
        return invoke(uv, "build")({builder, self.get("sampler").strictEquals(true) ? JSValue("vec3") : JSValue("ivec3")});
    }
    if (name == "generateOffset") return invoke(arg(args, 1), "build")({arg(args, 0), "ivec3"});
    if (name == "normal") {
        auto inputs = JSValue::object(nullptr);
        inputs.set("texture", self);
        inputs.set("uv", arg(args, 0));
        return captures[3].call({}, {inputs});
    }
    throw std::logic_error("Unknown Texture3DNode method: " + name);
}

JSValue texture3DLoadFactory(const JSValue&, const Args& args, const Args& captures) {
    const auto node = captures[0].call({}, args);
    return invoke(node, "setSampler")({false});
}

JSValue texture3DFactory(const JSValue&, const Args& args, const Args& captures) {
    const auto intent = nativeVarNodeBinding("VarIntent");
    const auto toIntent = [&](const JSValue& value) {
        return value.isNull() || value.isUndefined() ? value : intent.call({}, {value});
    };
    return captures[0].construct({arg(args, 0), toIntent(arg(args, 1, nullptr)), toIntent(arg(args, 2, nullptr))});
}

JSValue texture3DLevelFactory(const JSValue&, const Args& args, const Args& captures) {
    const auto node = captures[0].call({}, {arg(args, 0), arg(args, 1, nullptr)});
    return invoke(node, "level")({arg(args, 2)});
}
} // namespace

JSValue nativeTexture3DNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "Texture3DNode module");

    const auto base = nativeTextureNodeConstructor();
    nativeVarNodeModule();
    const auto vec3 = nativeTSLBaseBinding("vec3");
    const auto ifFunction = nativeTSLBaseBinding("If");
    auto normalCallback = JSValue::function(normalBody, {vec3, ifFunction}, "", 1);
    auto normal = nativeTSLBaseBinding("Fn").call({}, {normalCallback});

    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {arg(args, 0), arg(args, 1, nullptr), arg(args, 2, nullptr)});
            self.set("isTexture3DNode", true);
            return self;
        },
        {holder, normal}, "Texture3DNode", 1);
    holder.set("constructor", ctor);
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));

    JSPropertyDescriptor type;
    type.configurable = true;
    type.getter = JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue { return "Texture3DNode"; }, {}, "get type", 0);
    ctor.defineProperty("type", type);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
             {"getInputType", 0}, {"getDefaultUV", 0}, {"setUpdateMatrix", 1},
             {"generateUV", 2}, {"generateOffset", 2}, {"normal", 1}}) {
        JSPropertyDescriptor descriptor;
        descriptor.value = JSValue::function(method, {name, prototype, vec3, normal}, name, arity);
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }

    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", ctor);
    const auto texture3D = JSValue::function(texture3DFactory, {ctor}, "texture3D", 1);
    own(bindings, "texture3D", texture3D);
    own(bindings, "texture3DLoad", JSValue::function(texture3DLoadFactory, {texture3D}, "texture3DLoad", 0));
    own(bindings, "texture3DLevel", JSValue::function(texture3DLevelFactory, {texture3D}, "texture3DLevel", 3));

    auto module = JSValue::object(nullptr);
    auto names = bindings.ownPropertyNames();
    std::sort(names.begin(), names.end());
    for (const auto& name : names) own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeTexture3DNodeBinding(const std::string& name) {
    const auto keeper = nativeTexture3DNodeModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeTexture3DNodeConstructor() { return nativeTexture3DNodeBinding("default"); }

Texture3DNode::Texture3DNode(const JSValue& value, const JSValue& uvNode, const JSValue& levelNode)
    : object_(constructor().construct({value, uvNode, levelNode})) {}
