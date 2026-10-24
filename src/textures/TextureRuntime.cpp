#include "textures/Texture.h"
#include "core/NativeObject.h"
#include "core/JSModuleObject.h"
#include "core/JSOperators.h"
#include "core/JSRegExp.h"
#include "math/MathUtils.h"
#include "math/Vector3.h"
#include "utils.h"
#include <set>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue fallback(const Args& args, std::size_t i, const JSValue& value) { const auto next = arg(args, i); return next.isUndefined() ? value : next; }
JSValue invoke(const JSValue& self, const char* key, const Args& args = {}) { return self.get(key).call(self, args); }
void method(JSValue prototype, const char* name, unsigned arity, JSValue::NativeFunction fn) {
    JSPropertyDescriptor d; d.value = JSValue::function(fn, {name}, name, arity); d.writable = d.configurable = true; prototype.defineProperty(name, d);
}
void accessor(JSValue prototype, const char* name, JSValue::NativeFunction fn, bool read, bool write) {
    JSPropertyDescriptor d; d.configurable = true;
    if (read) d.getter = JSValue::function(fn, {std::string("get ") + name}, std::string("get ") + name, 0);
    if (write) d.setter = JSValue::function(fn, {std::string("set ") + name}, std::string("set ") + name, 1);
    prototype.defineProperty(name, d);
}
JSValue literal(const std::initializer_list<std::pair<const char*, JSValue>>& values) {
    auto result = JSValue::object(); for (const auto& [key, value] : values) { JSPropertyDescriptor d; d.value = value; d.writable = d.enumerable = d.configurable = true; result.defineProperty(key, d); } return result;
}
JSValue literalArray(const Args& values) {
    auto result = JSValue::array(); for (std::size_t i = 0; i < values.size(); ++i) { JSPropertyDescriptor d; d.value = values[i]; d.writable = d.enumerable = d.configurable = true; result.defineProperty(JSValue(i), d); } return result;
}
std::vector<std::string> keysForIn(JSValue value) {
    std::vector<std::string> keys; std::set<std::string> seen;
    while (!value.isNull() && !value.isUndefined()) {
        if (!value.isObject()) value = JSValue::intrinsicObjectConstructor().call({}, {value});
        for (const auto& key : value.ownPropertyNames()) { const auto d = value.getOwnPropertyDescriptor(key); if (d && seen.insert(key).second && d->enumerable.value_or(false)) keys.push_back(key); }
        value = value.getPrototype();
    }
    return keys;
}
JSValue sourceSerializeImage(const JSValue& image) {
    if (image.get("data").toBoolean()) {
        const auto arrayCtor = JSValue::intrinsicArrayConstructor();
        const auto data = arrayCtor.get("from").call(arrayCtor, {image.get("data")});
        const auto width = image.get("width"), height = image.get("height"), type = image.get("data").get("constructor").get("name");
        return literal({{"data", data}, {"width", width}, {"height", height}, {"type", type}});
    }
    ThreeUtils::warn({"Texture: Unable to serialize Texture."}); return JSValue::object();
}
JSValue sourceMethod(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string();
    if (name == "set needsUpdate") { if (arg(args, 0).strictEquals(true)) self.set("version", self.get("version").toNumber() + 1); return {}; }
    if (name == "getSize") {
        const auto target = arg(args, 0), data = self.get("data"); const auto fn = target.get("set");
        Args size; if (data.isNull()) size = {0, 0, 0};
        else for (const auto* key : {"width", "height", "depth"}) { const auto value = data.get(key); size.push_back(value.toBoolean() ? value : JSValue(0)); }
        fn.call(target, size); return target;
    }
    const auto meta = arg(args, 0); const bool root = meta.isUndefined() || meta.type() == JSValue::Type::String;
    if (!root && !meta.get("images").get(self.get("uuid")).isUndefined()) return meta.get("images").get(self.get("uuid"));
    auto result = literal({{"uuid", self.get("uuid")}, {"url", ""}}); const auto data = self.get("data");
    if (!data.isNull()) {
        JSValue url;
        const auto arrayCtor = JSValue::intrinsicArrayConstructor();
        if (arrayCtor.get("isArray").call(arrayCtor, {data}).toBoolean()) {
            url = JSValue::array(); const auto length = data.get("length");
            for (std::size_t i = 0; JSOperators::less(i, length); ++i) {
                const auto entry = data.get(JSValue(i));
                const auto isDataTexture = entry.get("isDataTexture").toBoolean();
                const auto push = url.get("push");
                const auto image = isDataTexture ? data.get(JSValue(i)).get("image") : data.get(JSValue(i));
                push.call(url, {sourceSerializeImage(image)});
            }
        } else url = sourceSerializeImage(data);
        result.set("url", url);
    }
    if (!root) { auto images = meta.get("images"); images.set(self.get("uuid"), result); }
    return result;
}
JSValue textureMethod(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string();
    if (name == "get width" || name == "get height" || name == "get depth") {
        const auto source = self.get("source"), getSize = source.get("getSize");
        return getSize.call(source, {captures[1]}).get(name == "get width" ? "x" : name == "get height" ? "y" : "z");
    }
    if (name == "get image") return self.get("source").get("data");
    if (name == "set image") { auto source = self.get("source"); source.set("data", arg(args, 0)); return {}; }
    if (name == "set needsUpdate") {
        if (arg(args, 0).strictEquals(true)) { self.set("version", self.get("version").toNumber() + 1); auto source = self.get("source"); source.set("needsUpdate", true); } return {};
    }
    if (name == "set needsPMREMUpdate") { if (arg(args, 0).strictEquals(true)) self.set("pmremVersion", self.get("pmremVersion").toNumber() + 1); return {}; }
    if (name == "clone") { auto next = self.get("constructor").construct(); return invoke(next, "copy", {self}); }
    if (name == "dispose") { invoke(self, "dispatchEvent", {literal({{"type", "dispose"}})}); return {}; }
    if (name == "addUpdateRange") { const auto ranges = self.get("updateRanges"), push = ranges.get("push"); push.call(ranges, {literal({{"start", arg(args, 0)}, {"count", arg(args, 1)}})}); return {}; }
    if (name == "clearUpdateRanges") { auto ranges = self.get("updateRanges"); ranges.set("length", 0); return {}; }
    if (name == "updateMatrix") {
        const auto matrix = self.get("matrix"), fn = matrix.get("setUvTransform"); Args values;
        for (const auto& [object, key] : std::initializer_list<std::pair<const char*, const char*>>{{"offset", "x"}, {"offset", "y"}, {"repeat", "x"}, {"repeat", "y"}}) values.push_back(self.get(object).get(key));
        values.push_back(self.get("rotation")); values.push_back(self.get("center").get("x")); values.push_back(self.get("center").get("y")); fn.call(matrix, values); return {};
    }
    if (name == "copy") {
        const auto source = arg(args, 0); self.set("name", source.get("name")); self.set("source", source.get("source"));
        const auto mipmaps = source.get("mipmaps"), slice = mipmaps.get("slice"); self.set("mipmaps", slice.call(mipmaps, {0}));
        for (const auto* key : {"mapping", "channel", "wrapS", "wrapT", "magFilter", "minFilter", "anisotropy", "format", "internalFormat", "type", "normalized"}) self.set(key, source.get(key));
        for (const auto* key : {"offset", "repeat", "center"}) { const auto target = self.get(key), copy = target.get("copy"); copy.call(target, {source.get(key)}); }
        self.set("rotation", source.get("rotation")); self.set("matrixAutoUpdate", source.get("matrixAutoUpdate"));
        const auto matrix = self.get("matrix"), copy = matrix.get("copy"); copy.call(matrix, {source.get("matrix")});
        for (const auto* key : {"generateMipmaps", "mipmapsAutoUpdate", "premultiplyAlpha", "flipY", "unpackAlignment", "colorSpace", "renderTarget", "isRenderTargetTexture", "isPMREMTexture", "isArrayTexture"}) self.set(key, source.get(key));
        const auto userData = source.get("userData").toJSON(); if (!userData) throw JSSyntaxError("Unexpected token 'u', \"undefined\" is not valid JSON");
        self.set("userData", JSValue::fromJSON(*userData)); self.set("needsUpdate", true); return self;
    }
    if (name == "setValues") {
        const auto values = arg(args, 0);
        for (const auto& key : keysForIn(values)) {
            if (!values.has(key)) continue; const auto next = values.get(key);
            if (next.isUndefined()) { ThreeUtils::warn({"Texture.setValues(): parameter '" + key + "' has value of undefined."}); continue; }
            const auto current = self.get(key);
            if (current.isUndefined()) { ThreeUtils::warn({"Texture.setValues(): property '" + key + "' does not exist."}); continue; }
            bool copied = false;
            for (const auto* flag : {"isVector2", "isVector3", "isMatrix3"}) if (current.toBoolean() && next.toBoolean() && current.get(flag).toBoolean() && next.get(flag).toBoolean()) { invoke(current, "copy", {next}); copied = true; break; }
            if (!copied) self.set(key, next);
        }
        return {};
    }
    if (name == "toJSON") {
        const auto meta = arg(args, 0); const bool root = meta.isUndefined() || meta.type() == JSValue::Type::String;
        if (!root && !meta.get("textures").get(self.get("uuid")).isUndefined()) return meta.get("textures").get(self.get("uuid"));
        auto output = literal({{"metadata", literal({{"version", 4.7}, {"type", "Texture"}, {"generator", "Texture.toJSON"}})},
            {"uuid", self.get("uuid")}, {"name", self.get("name")}, {"image", invoke(self.get("source"), "toJSON", {meta}).get("uuid")},
            {"mapping", self.get("mapping")}, {"channel", self.get("channel")},
            {"repeat", literalArray({self.get("repeat").get("x"), self.get("repeat").get("y")})},
            {"offset", literalArray({self.get("offset").get("x"), self.get("offset").get("y")})},
            {"center", literalArray({self.get("center").get("x"), self.get("center").get("y")})},
            {"rotation", self.get("rotation")}, {"wrap", literalArray({self.get("wrapS"), self.get("wrapT")})},
            {"format", self.get("format")}, {"internalFormat", self.get("internalFormat")}, {"type", self.get("type")},
            {"normalized", self.get("normalized")}, {"colorSpace", self.get("colorSpace")}, {"minFilter", self.get("minFilter")},
            {"magFilter", self.get("magFilter")}, {"anisotropy", self.get("anisotropy")}, {"flipY", self.get("flipY")},
            {"generateMipmaps", self.get("generateMipmaps")}, {"mipmapsAutoUpdate", self.get("mipmapsAutoUpdate")},
            {"premultiplyAlpha", self.get("premultiplyAlpha")}, {"unpackAlignment", self.get("unpackAlignment")}});
        const auto object = JSValue::intrinsicObjectConstructor();
        if (JSOperators::greater(object.get("keys").call(object, {self.get("userData")}).get("length"), 0)) output.set("userData", self.get("userData"));
        if (!root) { auto textures = meta.get("textures"); textures.set(self.get("uuid"), output); }
        return output;
    }
    if (name == "transformUv") {
        auto uv = arg(args, 0); if (!self.get("mapping").strictEquals(UVMapping)) return uv;
        const auto applyMatrix = uv.get("applyMatrix3");
        applyMatrix.call(uv, {self.get("matrix")});
        for (const auto& [key, wrap] : std::initializer_list<std::pair<const char*, const char*>>{{"x", "wrapS"}, {"y", "wrapT"}}) {
            if (JSOperators::less(uv.get(key), 0) || JSOperators::greater(uv.get(key), 1)) {
                const auto mode = self.get(wrap);
                if (mode.strictEquals(RepeatWrapping)) { const auto left = uv.get(key); const auto right = std::floor(uv.get(key).toNumber()); uv.set(key, left.toNumber() - right); }
                else if (mode.strictEquals(ClampToEdgeWrapping)) uv.set(key, JSOperators::less(uv.get(key), 0) ? JSValue(0) : JSValue(1));
                else if (mode.strictEquals(MirroredRepeatWrapping)) {
                    if (std::abs(std::fmod(std::floor(uv.get(key).toNumber()), 2)) == 1) { const auto left = std::ceil(uv.get(key).toNumber()); const auto right = uv.get(key); uv.set(key, left - right.toNumber()); }
                    else { const auto left = uv.get(key); const auto right = std::floor(uv.get(key).toNumber()); uv.set(key, left.toNumber() - right); }
                }
            }
        }
        if (self.get("flipY").toBoolean()) uv.set("y", 1 - uv.get("y").toNumber());
        return uv;
    }
    throw JSTypeError("Texture managed runtime method not implemented: " + name);
}
}

JSValue nativeTextureSourceConstructor() {
    static thread_local JSWeakValue weak; static thread_local bool initializing = false;
    if (const auto ctor = weak.lock(); ctor && ctor->isConstructible()) return *ctor;
    JSModuleInitializationScope scope(initializing, "TextureSource"); auto state = JSValue::object(nullptr); state.set("id", 0);
    auto ctor = JSValue::constructor([](const JSValue& receiver, const Args& args, const Args& captures) -> JSValue {
        auto self = receiver, state = captures[0]; self.set("isTextureSource", true);
        const auto id = state.get("id").toNumber(); state.set("id", id + 1); JSPropertyDescriptor d; d.value = id; self.defineProperty("id", d);
        self.set("uuid", MathUtils::generateUUID()); self.set("data", fallback(args, 0, nullptr)); self.set("dataReady", true); self.set("version", 0); return {};
    }, {state}, "TextureSource", 0);
    const auto prototype = ctor.get("prototype"); method(prototype, "getSize", 1, sourceMethod); accessor(prototype, "needsUpdate", sourceMethod, false, true); method(prototype, "toJSON", 1, sourceMethod);
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}

JSValue nativeTextureConstructor() {
    static thread_local JSWeakValue weak; static thread_local bool initializing = false;
    if (const auto ctor = weak.lock(); ctor && ctor->isConstructible()) return *ctor;
    JSModuleInitializationScope scope(initializing, "Texture"); auto state = JSValue::object(nullptr); state.set("id", 0);
    const auto base = EventDispatcher::runtimePrototype().get("constructor");
    auto ctor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        auto state = captures[0]; const auto constructor = state.get("constructor");
        // Default parameter expressions execute before super().
        auto image = arg(args, 0); if (image.isUndefined()) image = constructor.get("DEFAULT_IMAGE");
        auto mapping = arg(args, 1); if (mapping.isUndefined()) mapping = constructor.get("DEFAULT_MAPPING");
        const auto wrapS = fallback(args, 2, ClampToEdgeWrapping), wrapT = fallback(args, 3, ClampToEdgeWrapping);
        const auto mag = fallback(args, 4, LinearFilter), min = fallback(args, 5, LinearMipmapLinearFilter);
        const auto format = fallback(args, 6, RGBAFormat), type = fallback(args, 7, UnsignedByteType);
        auto anisotropy = arg(args, 8); if (anisotropy.isUndefined()) anisotropy = constructor.get("DEFAULT_ANISOTROPY");
        const auto space = fallback(args, 9, NoColorSpace);
        auto self = constructor.getPrototype().construct({}, newTarget); self.set("isTexture", true);
        const auto id = state.get("id").toNumber(); state.set("id", id + 1); JSPropertyDescriptor d; d.value = id; self.defineProperty("id", d);
        self.set("uuid", MathUtils::generateUUID()); self.set("name", ""); self.set("source", nativeTextureSourceConstructor().construct({image})); self.set("mipmaps", JSValue::array());
        for (const auto& [key, value] : std::initializer_list<std::pair<const char*, JSValue>>{{"mapping", mapping}, {"channel", 0}, {"wrapS", wrapS}, {"wrapT", wrapT}, {"magFilter", mag}, {"minFilter", min}, {"anisotropy", anisotropy}, {"format", format}, {"internalFormat", nullptr}, {"type", type}}) self.set(key, value);
        self.set("offset", nativeVector2Constructor().construct({0, 0})); self.set("repeat", nativeVector2Constructor().construct({1, 1})); self.set("center", nativeVector2Constructor().construct({0, 0}));
        self.set("rotation", 0); self.set("matrixAutoUpdate", true); self.set("matrix", nativeMatrix(std::make_shared<Matrix3>()));
        for (const auto& [key, value] : std::initializer_list<std::pair<const char*, JSValue>>{{"generateMipmaps", true}, {"mipmapsAutoUpdate", true}, {"premultiplyAlpha", false}, {"flipY", true}, {"unpackAlignment", 4}, {"colorSpace", space}}) self.set(key, value);
        self.set("userData", JSValue::object()); self.set("updateRanges", JSValue::array()); self.set("version", 0); self.set("onUpdate", nullptr); self.set("renderTarget", nullptr); self.set("isRenderTargetTexture", false);
        self.set("isArrayTexture", image.toBoolean() && image.get("depth").toBoolean() && JSOperators::greater(image.get("depth"), 1));
        self.set("pmremVersion", 0); self.set("isPMREMTexture", false); self.set("normalized", false); return self;
    }, {state}, "Texture", 0);
    state.set("constructor", ctor); ctor.setPrototype(base); auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    const auto temp = nativeVector3Constructor().construct();
    for (const auto* key : {"width", "height", "depth"}) { JSPropertyDescriptor d; d.getter = JSValue::function(textureMethod, {std::string("get ") + key, temp}, std::string("get ") + key, 0); d.configurable = true; prototype.defineProperty(key, d); }
    accessor(prototype, "image", textureMethod, true, true);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"updateMatrix", 0}, {"addUpdateRange", 2}, {"clearUpdateRanges", 0}, {"clone", 0}, {"copy", 1}, {"setValues", 1}, {"toJSON", 1}, {"dispose", 0}, {"transformUv", 1}}) method(prototype, name, arity, textureMethod);
    accessor(prototype, "needsUpdate", textureMethod, false, true); accessor(prototype, "needsPMREMUpdate", textureMethod, false, true);
    ctor.set("DEFAULT_IMAGE", nullptr); ctor.set("DEFAULT_MAPPING", UVMapping); ctor.set("DEFAULT_ANISOTROPY", 1);
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
