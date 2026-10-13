// Direct CPU port of three.js 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "renderers/webgl/WebGLUniforms.h"
#include "constants.h"
#include "core/NativeArray.h"
#include "core/NativeObject.h"
#include "textures/CubeDepthTexture.h"
#include "textures/CubeTexture.h"
#include "textures/Data3DTexture.h"
#include "textures/DataArrayTexture.h"
#include "textures/DepthTexture.h"
#include "textures/Texture.h"
#include <cmath>
#include <regex>

namespace {
using Args = std::vector<JSValue>;
void validate(std::initializer_list<const JSValue*> values) {
    for (const auto* value : values)
        static_cast<void>(JSWeakValue(*value));
}
JSValue arg(const Args& values, std::size_t i) {
    return i < values.size() ? values[i] : JSValue();
}
void literal(JSValue self, const std::string& name, JSValue value) {
    JSPropertyDescriptor d;
    d.value = std::move(value);
    d.writable = d.enumerable = d.configurable = true;
    self.defineProperty(name, d);
}
void method(JSValue self, const char* name, JSValue::NativeFunction fn, const Args& captures,
            unsigned arity) {
    JSPropertyDescriptor d;
    d.value = JSValue::function(fn, captures, name, arity);
    d.writable = d.configurable = true;
    d.enumerable = false;
    self.defineProperty(name, d);
}
std::string key(const JSValue& value) {
    return value.toPrimitive(JSValue::PreferredType::String).toString();
}
JSValue call(const JSValue& self, const char* name, const Args& args = {}) {
    const auto fn = self.get(name);
    return fn.call(self, args);
}
void push(const JSValue& self, const JSValue& value) {
    call(self, "push", {value});
}
bool less(double i, const JSValue& length) {
    return i < length.toNumber();
}

bool arraysEqual(const JSValue& a, const JSValue& b) {
    const auto al = a.get("length"), bl = b.get("length");
    if (!al.strictEquals(bl))
        return false;
    const auto length = a.get("length");
    for (double i = 0; less(i, length); ++i) {
        const auto av = a.get(key(i)), bv = b.get(key(i));
        if (!av.strictEquals(bv))
            return false;
    }
    return true;
}
void copyArray(JSValue a, const JSValue& b) {
    const auto length = b.get("length");
    for (double i = 0; less(i, length); ++i) {
        const auto value = b.get(key(i));
        a.set(key(i), value);
    }
}
JSValue typed(unsigned kind, const JSValue& size) {
    return NativeArray_detail::constructor(kind).construct({size});
}
JSValue module();

JSValue flatten(const JSValue& array, const JSValue& blocks, int size, const JSValue& state) {
    const auto first = array.get("0");
    // Relational operators each perform their own ToPrimitive/ToNumber.
    if (first.toNumber() <= 0 || first.toNumber() > 0)
        return array;
    const auto n = blocks.toNumber() * size;
    auto cache = state.get("arrayCacheF32");
    auto result = cache.get(key(n));
    if (result.isUndefined()) {
        result = typed(6, n);
        cache.set(key(n), result);
    }
    if (!blocks.strictEquals(0)) {
        call(first, "toArray", {result, 0});
        for (double i = 1, offset = 0; !JSValue(i).strictEquals(blocks); ++i) {
            offset += size;
            const auto element = array.get(key(i));
            call(element, "toArray", {result, offset});
        }
    }
    return result;
}
JSValue allocTexUnits(const JSValue& textures, const JSValue& count, const JSValue& state) {
    auto cache = state.get("arrayCacheI32");
    auto result = cache.get(key(count));
    if (result.isUndefined()) {
        result = typed(4, count);
        cache.set(key(count), result);
    }
    for (double i = 0; !JSValue(i).strictEquals(count); ++i) {
        const auto value = call(textures, "allocateTextureUnit");
        result.set(key(i), value);
    }
    return result;
}
JSValue shadowTexture(JSValue texture) {
    if (texture.get("version").strictEquals(0)) {
        texture.set("compareFunction", AlwaysCompare);
        texture.set("needsUpdate", true);
    }
    return texture;
}

struct Setter {
    unsigned type;
    const char* singular;
    const char* array;
    const char* gl;
    int width;
    int kind;
};
// kind: scalar=0, vector=1, matrix=2, texture2D=3, texture3D=4, cube=5, array=6.
constexpr Setter setters[] = {
    {0x1406, "setValueV1f", "setValueV1fArray", "uniform1f", 1, 0},
    {0x8b50, "setValueV2f", "setValueV2fArray", "uniform2f", 2, 1},
    {0x8b51, "setValueV3f", "setValueV3fArray", "uniform3f", 3, 1},
    {0x8b52, "setValueV4f", "setValueV4fArray", "uniform4f", 4, 1},
    {0x8b5a, "setValueM2", "setValueM2Array", "uniformMatrix2fv", 2, 2},
    {0x8b5b, "setValueM3", "setValueM3Array", "uniformMatrix3fv", 3, 2},
    {0x8b5c, "setValueM4", "setValueM4Array", "uniformMatrix4fv", 4, 2},
    {0x1404, "setValueV1i", "setValueV1iArray", "uniform1i", 1, 0},
    {0x8b56, "setValueV1i", "setValueV1iArray", "uniform1i", 1, 0},
    {0x8b53, "setValueV2i", "setValueV2iArray", "uniform2i", 2, 1},
    {0x8b57, "setValueV2i", "setValueV2iArray", "uniform2i", 2, 1},
    {0x8b54, "setValueV3i", "setValueV3iArray", "uniform3i", 3, 1},
    {0x8b58, "setValueV3i", "setValueV3iArray", "uniform3i", 3, 1},
    {0x8b55, "setValueV4i", "setValueV4iArray", "uniform4i", 4, 1},
    {0x8b59, "setValueV4i", "setValueV4iArray", "uniform4i", 4, 1},
    {0x1405, "setValueV1ui", "setValueV1uiArray", "uniform1ui", 1, 0},
    {0x8dc6, "setValueV2ui", "setValueV2uiArray", "uniform2ui", 2, 1},
    {0x8dc7, "setValueV3ui", "setValueV3uiArray", "uniform3ui", 3, 1},
    {0x8dc8, "setValueV4ui", "setValueV4uiArray", "uniform4ui", 4, 1},
    {0x8b5e, "setValueT1", "setValueT1Array", "setTexture2D", 1, 3},
    {0x8d66, "setValueT1", "setValueT1Array", "setTexture2D", 1, 3},
    {0x8dca, "setValueT1", "setValueT1Array", "setTexture2D", 1, 3},
    {0x8dd2, "setValueT1", "setValueT1Array", "setTexture2D", 1, 3},
    {0x8b62, "setValueT1", "setValueT1Array", "setTexture2D", 1, 3},
    {0x8b5f, "setValueT3D1", "setValueT3DArray", "setTexture3D", 1, 4},
    {0x8dcb, "setValueT3D1", "setValueT3DArray", "setTexture3D", 1, 4},
    {0x8dd3, "setValueT3D1", "setValueT3DArray", "setTexture3D", 1, 4},
    {0x8b60, "setValueT6", "setValueT6Array", "setTextureCube", 1, 5},
    {0x8dcc, "setValueT6", "setValueT6Array", "setTextureCube", 1, 5},
    {0x8dd4, "setValueT6", "setValueT6Array", "setTextureCube", 1, 5},
    {0x8dc5, "setValueT6", "setValueT6Array", "setTextureCube", 1, 5},
    {0x8dc1, "setValueT2DArray1", "setValueT2DArrayArray", "setTexture2DArray", 1, 6},
    {0x8dcf, "setValueT2DArray1", "setValueT2DArrayArray", "setTexture2DArray", 1, 6},
    {0x8dd7, "setValueT2DArray1", "setValueT2DArrayArray", "setTexture2DArray", 1, 6},
    {0x8dc4, "setValueT2DArray1", "setValueT2DArrayArray", "setTexture2DArray", 1, 6}};
JSValue setter(const JSValue& self, const Args& args, const Args& captures) {
    const auto& spec = setters[std::size_t(captures[1].number())];
    const bool pure = captures[2].toBoolean();
    const auto state = captures[0], gl = arg(args, 0), value = arg(args, 1),
               textures = arg(args, 2);
    if (spec.kind >= 3) {
        auto cache = self.get("cache");
        JSValue units, count, unit;
        if (pure) {
            count = value.get("length");
            units = allocTexUnits(textures, count, state);
            if (!arraysEqual(cache, units)) {
                const auto fn = gl.get("uniform1iv"), addr = self.get("addr");
                fn.call(gl, {addr, units});
                copyArray(cache, units);
            }
        } else {
            unit = call(textures, "allocateTextureUnit");
            if (!cache.get("0").strictEquals(unit)) {
                const auto fn = gl.get("uniform1i"), addr = self.get("addr");
                fn.call(gl, {addr, unit});
                cache.set("0", unit);
            }
        }
        JSValue fallback;
        if (spec.kind == 3 || spec.kind == 5) {
            const auto type = self.get("type"),
                       shadow =
                           gl.get(spec.kind == 3 ? "SAMPLER_2D_SHADOW" : "SAMPLER_CUBE_SHADOW");
            fallback = type.strictEquals(shadow)
                           ? shadowTexture(state.get(spec.kind == 3 ? "emptyShadowTexture"
                                                                    : "emptyCubeShadowTexture"))
                           : state.get(spec.kind == 3 ? "emptyTexture" : "emptyCubeTexture");
        } else
            fallback = state.get(spec.kind == 4 ? "empty3dTexture" : "emptyArrayTexture");
        if (pure) {
            for (double i = 0; !JSValue(i).strictEquals(count); ++i) {
                const auto fn = textures.get(spec.gl), entry = value.get(key(i));
                const auto actual = entry.toBoolean() ? entry : fallback;
                const auto texUnit = units.get(key(i));
                fn.call(textures, {actual, texUnit});
            }
        } else {
            const auto fn = textures.get(spec.gl);
            fn.call(textures, {value.toBoolean() ? value : fallback, unit});
        }
        return {};
    }
    if (pure) {
        JSValue data = value;
        if ((spec.kind == 1 && spec.type >= 0x8b50 && spec.type <= 0x8b52) || spec.kind == 2) {
            const auto size = self.get("size");
            data =
                flatten(value, size, spec.kind == 2 ? spec.width * spec.width : spec.width, state);
        }
        const std::string name = spec.kind == 2 ? spec.gl : std::string(spec.gl) + "v";
        const auto fn = gl.get(name), addr = self.get("addr");
        fn.call(gl, spec.kind == 2 ? Args{addr, false, data} : Args{addr, data});
        return {};
    }
    auto cache = self.get("cache");
    if (spec.kind == 0) {
        if (cache.get("0").strictEquals(value))
            return {};
        const auto fn = gl.get(spec.gl), addr = self.get("addr");
        fn.call(gl, {addr, value});
        cache.set("0", value);
        return {};
    }
    if (spec.kind == 2) {
        const auto elements = value.get("elements");
        if (elements.isUndefined()) {
            if (arraysEqual(cache, value))
                return {};
            const auto fn = gl.get(spec.gl), addr = self.get("addr");
            fn.call(gl, {addr, false, value});
            copyArray(cache, value);
        } else {
            if (arraysEqual(cache, elements))
                return {};
            const auto scratch = state.get("mat" + std::to_string(spec.width) + "array");
            call(scratch, "set", {elements});
            const auto fn = gl.get(spec.gl), addr = self.get("addr");
            fn.call(gl, {addr, false, scratch});
            copyArray(cache, elements);
        }
        return {};
    }
    const char* components = nullptr;
    if (!value.get("x").isUndefined())
        components = "xyzw";
    else if (spec.type == 0x8b51 && !value.get("r").isUndefined())
        components = "rgb";
    if (components) {
        bool changed = false;
        for (int i = 0; i < spec.width; ++i) {
            const auto saved = cache.get(std::to_string(i)),
                       input = value.get(std::string(1, components[i]));
            if (!saved.strictEquals(input)) {
                changed = true;
                break;
            }
        }
        if (changed) {
            const auto fn = gl.get(spec.gl), addr = self.get("addr");
            Args upload{addr};
            for (int i = 0; i < spec.width; ++i)
                upload.push_back(value.get(std::string(1, components[i])));
            fn.call(gl, upload);
            for (int i = 0; i < spec.width; ++i) {
                const auto input = value.get(std::string(1, components[i]));
                cache.set(std::to_string(i), input);
            }
        }
    } else {
        if (arraysEqual(cache, value))
            return {};
        const auto fn = gl.get(std::string(spec.gl) + "v"), addr = self.get("addr");
        fn.call(gl, {addr, value});
        copyArray(cache, value);
    }
    return {};
}
JSValue selectedSetter(const JSValue& type, bool pure, const JSValue& state) {
    for (const auto& spec : setters)
        if (type.strictEquals(spec.type))
            return state.get(pure ? spec.array : spec.singular);
    return {};
}
JSValue leafConstructor(const JSValue& self, const Args& args, const Args& captures) {
    auto result = self;
    const auto info = arg(args, 1);
    result.set("id", arg(args, 0));
    result.set("addr", arg(args, 2));
    result.set("cache", JSValue::array());
    result.set("type", info.get("type"));
    const bool pure = captures[1].toBoolean();
    if (pure)
        result.set("size", info.get("size"));
    result.set("setValue", selectedSetter(info.get("type"), pure, captures[0]));
    return {};
}
JSValue structuredConstructor(const JSValue& self, const Args& args, const Args&) {
    auto result = self;
    result.set("id", arg(args, 0));
    result.set("seq", JSValue::array());
    result.set("map", JSValue::object());
    return {};
}
JSValue structuredSet(const JSValue& self, const Args& args, const Args&) {
    const auto seq = self.get("seq"), count = seq.get("length");
    for (double i = 0; !JSValue(i).strictEquals(count); ++i) {
        const auto u = seq.get(key(i)), fn = u.get("setValue");
        const auto id = u.get("id"), value = arg(args, 1).get(key(id));
        fn.call(u, {arg(args, 0), value, arg(args, 2)});
    }
    return {};
}
void addUniform(JSValue container, const JSValue& uniform) {
    const auto seq = container.get("seq"), fn = seq.get("push");
    fn.call(seq, {uniform});
    auto map = container.get("map");
    const auto id = uniform.get("id");
    map.set(key(id), uniform);
}
std::size_t utf16Length(const std::string& value, std::size_t bytes) {
    std::size_t result = 0;
    for (std::size_t i = 0; i < bytes;) {
        const auto c = static_cast<unsigned char>(value[i]);
        const unsigned size = c < 0x80 ? 1 : c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
        i += size;
        result += size == 4 ? 2 : 1;
    }
    return result;
}
void parseUniform(const JSValue& info, const JSValue& addr, JSValue container, JSValue state) {
    const auto path = info.get("name"), pathLength = path.get("length");
    state.set("regexIndex", 0);
    static const std::regex expression(R"((\w+)(\])?(\[|\.)?)", std::regex_constants::ECMAScript);
    for (;;) {
        const auto text = path.toString();
        const auto start = state.get("regexIndex").toNumber();
        std::size_t byteStart = 0;
        while (byteStart < text.size() && utf16Length(text, byteStart) < start)
            ++byteStart;
        std::smatch match;
        if (byteStart > text.size() ||
            !std::regex_search(text.begin() + byteStart, text.end(), match, expression)) {
            state.set("regexIndex", 0);
            throw JSTypeError("Uniform path has no identifier");
        }
        const auto end = utf16Length(text, byteStart + match.position() + match.length());
        state.set("regexIndex", end);
        JSValue id = match[1].str();
        const bool index = match[2].matched;
        const auto subscript = match[3].str();
        if (index) {
            double number = id.toNumber();
            if (!std::isfinite(number) || number == 0)
                id = 0;
            else {
                number = std::fmod(std::trunc(number), 4294967296.);
                if (number < 0)
                    number += 4294967296.;
                id = number >= 2147483648. ? number - 4294967296. : number;
            }
        }
        if (subscript.empty() || (subscript == "[" && JSValue(end + 2).strictEquals(pathLength))) {
            const auto constructor =
                state.get(subscript.empty() ? "SingleUniform" : "PureArrayUniform");
            const auto uniform = constructor.construct({id, info, addr});
            addUniform(container, uniform);
            break;
        }
        const auto map = container.get("map");
        auto next = map.get(key(id));
        if (next.isUndefined()) {
            next = state.get("StructuredUniform").construct({id});
            addUniform(container, next);
        }
        container = next;
    }
}
JSValue rootConstructor(const JSValue& self, const Args& args, const Args& captures) {
    auto result = self;
    const auto gl = arg(args, 0), program = arg(args, 1), state = captures[0];
    result.set("seq", JSValue::array());
    result.set("map", JSValue::object());
    const auto parameter = gl.get("getProgramParameter"), active = gl.get("ACTIVE_UNIFORMS");
    const auto count = parameter.call(gl, {program, active});
    for (double i = 0; less(i, count); ++i) {
        const auto info = call(gl, "getActiveUniform", {program, i});
        const auto location = gl.get("getUniformLocation"), name = info.get("name");
        const auto addr = location.call(gl, {program, name});
        parseUniform(info, addr, self, state);
    }
    auto shadows = JSValue::array(), other = JSValue::array();
    const auto seq = self.get("seq");
    for (double i = 0; less(i, seq.get("length")); ++i) {
        const auto u = seq.get(key(i));
        bool shadow = false;
        for (const auto* name :
             {"SAMPLER_2D_SHADOW", "SAMPLER_CUBE_SHADOW", "SAMPLER_2D_ARRAY_SHADOW"}) {
            const auto type = u.get("type"), code = gl.get(name);
            if (type.strictEquals(code)) {
                shadow = true;
                break;
            }
        }
        push(shadow ? shadows : other, u);
    }
    if (shadows.get("length").toNumber() > 0)
        result.set("seq", call(shadows, "concat", {other}));
    return {};
}
JSValue rootSet(const JSValue& self, const Args& args, const Args&) {
    const auto map = self.get("map"), u = map.get(key(arg(args, 1)));
    if (!u.isUndefined()) {
        const auto fn = u.get("setValue");
        fn.call(u, {arg(args, 0), arg(args, 2), arg(args, 3)});
    }
    return {};
}
JSValue rootOptional(const JSValue& self, const Args& args, const Args&) {
    const auto value = arg(args, 1).get(key(arg(args, 2)));
    if (!value.isUndefined()) {
        const auto fn = self.get("setValue");
        fn.call(self, {arg(args, 0), arg(args, 2), value});
    }
    return {};
}
JSValue upload(const JSValue&, const Args& args, const Args&) {
    const auto seq = arg(args, 1), values = arg(args, 2), count = seq.get("length");
    for (double i = 0; !JSValue(i).strictEquals(count); ++i) {
        const auto u = seq.get(key(i)), id = u.get("id"), v = values.get(key(id));
        if (!v.get("needsUpdate").strictEquals(false)) {
            const auto fn = u.get("setValue"), value = v.get("value");
            fn.call(u, {arg(args, 0), value, arg(args, 3)});
        }
    }
    return {};
}
JSValue seqWithValue(const JSValue&, const Args& args, const Args&) {
    const auto seq = arg(args, 0), values = arg(args, 1);
    auto result = JSValue::array();
    const auto count = seq.get("length");
    for (double i = 0; !JSValue(i).strictEquals(count); ++i) {
        const auto u = seq.get(key(i)), id = u.get("id");
        if (!values.isObject())
            throw JSTypeError("Right-hand side of in is not an object");
        if (values.has(key(id)))
            push(result, u);
    }
    return result;
}
JSValue module() {
    static thread_local JSWeakValue cached;
    if (const auto value = cached.lock(); value && value->isObject())
        return *value;
    auto state = JSValue::object(nullptr);
    literal(state, "emptyTexture", nativeTexture(std::make_shared<Texture>()));
    literal(state, "emptyShadowTexture", nativeTexture(std::make_shared<DepthTexture>(1, 1)));
    literal(state, "emptyCubeShadowTexture", nativeTexture(std::make_shared<CubeDepthTexture>(1)));
    literal(state, "emptyArrayTexture", nativeTexture(std::make_shared<DataArrayTexture>()));
    literal(state, "empty3dTexture", nativeTexture(std::make_shared<Data3DTexture>()));
    literal(state, "emptyCubeTexture", nativeTexture(std::make_shared<CubeTexture>()));
    literal(state, "arrayCacheF32", JSValue::array());
    literal(state, "arrayCacheI32", JSValue::array());
    literal(state, "mat4array", typed(6, 16));
    literal(state, "mat3array", typed(6, 9));
    literal(state, "mat2array", typed(6, 4));
    literal(state, "regexIndex", 0);
    for (std::size_t i = 0; i < std::size(setters); ++i)
        for (bool pure : {false, true}) {
            const auto name = pure ? setters[i].array : setters[i].singular;
            if (!state.hasOwn(name))
                literal(state, name,
                        JSValue::constructor(setter, {state, i, pure}, name,
                                             setters[i].kind >= 3 ? 3 : 2,
                                             JSValue::ConstructorKind::Callable));
        }
    literal(state, "SingleUniform",
            JSValue::constructor(leafConstructor, {state, false}, "SingleUniform", 3));
    literal(state, "PureArrayUniform",
            JSValue::constructor(leafConstructor, {state, true}, "PureArrayUniform", 3));
    auto structured = JSValue::constructor(structuredConstructor, {}, "StructuredUniform", 1);
    method(structured.get("prototype"), "setValue", structuredSet, {}, 3);
    literal(state, "StructuredUniform", structured);
    auto root = JSValue::constructor(rootConstructor, {state}, "WebGLUniforms", 2);
    method(root.get("prototype"), "setValue", rootSet, {}, 4);
    method(root.get("prototype"), "setOptional", rootOptional, {}, 3);
    method(root, "upload", ::upload, {}, 4);
    method(root, "seqWithValue", ::seqWithValue, {}, 2);
    literal(state, "WebGLUniforms", root);
    state.retainForHeapLifetime();
    cached = JSWeakValue(state);
    return state;
}
} // namespace
JSValue nativeWebGLUniformsConstructor() {
    return module().get("WebGLUniforms");
}
WebGLUniforms::WebGLUniforms(const JSValue& gl, const JSValue& program) {
    validate({&gl, &program});
    object_ = nativeWebGLUniformsConstructor().construct({gl, program});
}
JSValue WebGLUniforms::setValue(const JSValue& gl, const JSValue& name, const JSValue& value,
                                const JSValue& textures) {
    validate({&object_, &gl, &name, &value, &textures});
    return call(object_, "setValue", {gl, name, value, textures});
}
JSValue WebGLUniforms::setOptional(const JSValue& gl, const JSValue& object, const JSValue& name) {
    validate({&object_, &gl, &object, &name});
    return call(object_, "setOptional", {gl, object, name});
}
JSValue WebGLUniforms::upload(const JSValue& gl, const JSValue& seq, const JSValue& values,
                              const JSValue& textures) {
    validate({&gl, &seq, &values, &textures});
    return call(nativeWebGLUniformsConstructor(), "upload", {gl, seq, values, textures});
}
JSValue WebGLUniforms::seqWithValue(const JSValue& seq, const JSValue& values) {
    validate({&seq, &values});
    return call(nativeWebGLUniformsConstructor(), "seqWithValue", {seq, values});
}
