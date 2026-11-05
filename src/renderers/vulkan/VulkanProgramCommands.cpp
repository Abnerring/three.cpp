#include "renderers/vulkan/VulkanProgramCommands.h"
#include "renderers/vulkan/VulkanShaderCompiler.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <map>
#include <string>

namespace {
using Args = std::vector<JSValue>;
using Linked = VulkanProgramCommands::LinkedProgram;
constexpr const char* stateTag = "VulkanProgramCommands.State";
constexpr const char* shaderTag = "VulkanProgramCommands.Shader";
constexpr const char* programTag = "VulkanProgramCommands.Program";
constexpr const char* locationTag = "VulkanProgramCommands.Location";
constexpr std::uint32_t vertexShader = 0x8b31, fragmentShader = 0x8b30;
constexpr std::uint32_t invalidEnum = 0x0500, invalidValue = 0x0501, invalidOperation = 0x0502;
struct Program;
struct State {
    bool alive = true;
    std::uint32_t error = 0;
    std::uint64_t generation = 0;
    std::vector<std::weak_ptr<Program>> programs;
};
struct Shader {
    std::weak_ptr<State> owner;
    std::uint32_t type = 0;
    bool compiled = false, deleted = false;
    std::string source, diagnostics;
};
struct Program {
    std::weak_ptr<State> owner;
    bool deleted = false, retired = false, linked = false, validated = false;
    std::map<std::string, std::uint32_t> attributes;
    std::shared_ptr<Linked> data;
    std::string diagnostics;
};
struct Location {
    std::weak_ptr<State> owner;
    std::weak_ptr<Program> program;
    std::uint64_t generation = 0;
    std::string name;
    int type = 0, size = 0;
    bool array = false;
};
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
void own(JSValue object, const char* name, JSValue value) {
    JSPropertyDescriptor d; d.value = value; d.writable = d.configurable = d.enumerable = true;
    object.defineProperty(name, d);
}
std::uint32_t uint32(double value) {
    if (!std::isfinite(value) || value == 0) return 0;
    double result = std::fmod(std::trunc(value), 4294967296.0);
    if (result < 0) result += 4294967296.0;
    return static_cast<std::uint32_t>(result);
}
std::int32_t int32(double value) {
    const auto n = uint32(value);
    return n < 0x80000000u ? static_cast<std::int32_t>(n) : static_cast<std::int32_t>(std::int64_t(n) - 4294967296ll);
}
void error(const std::shared_ptr<State>& state, std::uint32_t code) { if (!state->error) state->error = code; }
template<class T> std::shared_ptr<T> handle(const JSValue& value, const char* tag, const std::shared_ptr<State>& state) {
    if (!value.isHostObject(tag)) { error(state, invalidOperation); return {}; }
    const auto result = std::static_pointer_cast<T>(value.hostPayload(tag));
    if (!result || result->owner.lock() != state) { error(state, invalidOperation); return {}; }
    return result;
}
void retireProgram(JSValue value, const std::shared_ptr<Program>& program) {
    program->retired = true;
    program->linked = program->validated = false;
    // Already recorded GPU draws may own this generation independently.
    program->data.reset(); program->attributes.clear(); program->diagnostics.clear();
    if (!value.isUndefined()) value.setInternalSlot("shaders", nullptr);
}
void selectProgram(JSValue stateValue, const JSValue& value) {
    const auto previous = stateValue.getInternalSlot("currentProgram");
    if (previous.sameValue(value)) return;
    stateValue.setInternalSlot("currentProgram", value);
    if (previous.isHostObject(programTag)) {
        const auto program = std::static_pointer_cast<Program>(previous.hostPayload(programTag));
        if (program->deleted) retireProgram(previous, program);
    }
}
JSValue info(const std::string& name, int size, int type) {
    auto result = JSValue::object(); own(result, "name", name); own(result, "size", size); own(result, "type", type); return result;
}
bool samplerType(int type) {
    switch (type) {
    case 0x8b5e: case 0x8b5f: case 0x8b60: case 0x8b62: case 0x8dc1: case 0x8dc4: case 0x8dc5:
    case 0x8dca: case 0x8dcb: case 0x8dcc: case 0x8dcf: case 0x8dd2: case 0x8dd3: case 0x8dd4: case 0x8dd7: return true;
    default: return false;
    }
}
enum class UniformScalar { Float, Int, Uint, Bool, Sampler, Unknown };
struct UniformType {
    UniformScalar scalar = UniformScalar::Unknown;
    unsigned columns = 1, rows = 1;
    bool matrix = false;
    unsigned components() const { return columns * rows; }
};
UniformType uniformType(int type) {
    if (samplerType(type)) return {UniformScalar::Sampler};
    switch (type) {
    case 0x1406: return {UniformScalar::Float};
    case 0x1404: return {UniformScalar::Int};
    case 0x1405: return {UniformScalar::Uint};
    case 0x8b56: return {UniformScalar::Bool};
    case 0x8b50: case 0x8b51: case 0x8b52: return {UniformScalar::Float, 1, unsigned(type - 0x8b50 + 2)};
    case 0x8b53: case 0x8b54: case 0x8b55: return {UniformScalar::Int, 1, unsigned(type - 0x8b53 + 2)};
    case 0x8dc6: case 0x8dc7: case 0x8dc8: return {UniformScalar::Uint, 1, unsigned(type - 0x8dc6 + 2)};
    case 0x8b57: case 0x8b58: case 0x8b59: return {UniformScalar::Bool, 1, unsigned(type - 0x8b57 + 2)};
    case 0x8b5a: return {UniformScalar::Float, 2, 2, true};
    case 0x8b5b: return {UniformScalar::Float, 3, 3, true};
    case 0x8b5c: return {UniformScalar::Float, 4, 4, true};
    case 0x8b65: return {UniformScalar::Float, 2, 3, true};
    case 0x8b66: return {UniformScalar::Float, 2, 4, true};
    case 0x8b67: return {UniformScalar::Float, 3, 2, true};
    case 0x8b68: return {UniformScalar::Float, 3, 4, true};
    case 0x8b69: return {UniformScalar::Float, 4, 2, true};
    case 0x8b6a: return {UniformScalar::Float, 4, 3, true};
    default: return {};
    }
}
UniformType uniformSetter(const std::string& name) {
    if (name.starts_with("uniformMatrix")) {
        const auto columns = unsigned(name[13] - '0');
        const auto rows = name[14] == 'x' ? unsigned(name[15] - '0') : columns;
        return {UniformScalar::Float, columns, rows, true};
    }
    return {name.find("ui", 8) != std::string::npos ? UniformScalar::Uint :
            name[8] == 'i' ? UniformScalar::Int : UniformScalar::Float, 1, unsigned(name[7] - '0')};
}
bool accepts(const UniformType& type, const UniformType& setter) {
    if (type.matrix != setter.matrix || type.columns != setter.columns || type.rows != setter.rows) return false;
    if (type.scalar == UniformScalar::Sampler) return setter.scalar == UniformScalar::Int;
    return type.scalar == UniformScalar::Bool || type.scalar == setter.scalar;
}
struct UniformArray {
    std::string base;
    bool array = false;
};
UniformArray uniformArray(const VulkanEsslResult& program, const VulkanEsslProgram::ActiveUniform& uniform) {
    const auto mapping = std::find_if(program.uniformMappings.begin(), program.uniformMappings.end(),
        [&](const auto& value) { return value.sourceName == uniform.name; });
    std::size_t dimensions = 0, indices = 0;
    if (mapping != program.uniformMappings.end()) {
        if (mapping->kind == VulkanEsslResult::UniformMapping::Kind::Sampler) {
            for (const auto& stage : program.interfaces) for (const auto& descriptor : stage.descriptors)
                if (descriptor.set == mapping->set && descriptor.binding == mapping->binding)
                    dimensions = descriptor.arrayDimensions.size();
            indices = std::count(mapping->memberPath.begin(), mapping->memberPath.end(), '[');
        } else {
            const auto* layout = &mapping->rootLayout;
            for (std::size_t i = 0; i < mapping->memberPath.size();) {
                if (mapping->memberPath[i] == '[') {
                    const auto end = mapping->memberPath.find(']', i);
                    if (end == std::string::npos) return {uniform.name};
                    ++indices; i = end + 1;
                } else if (mapping->memberPath[i] == '.') {
                    const auto begin = ++i, end = mapping->memberPath.find_first_of(".[", begin);
                    const auto name = mapping->memberPath.substr(begin, end - begin);
                    const auto member = std::find_if(layout->members.begin(), layout->members.end(),
                        [&](const auto& value) { return value.name == name; });
                    if (member == layout->members.end()) return {uniform.name};
                    layout = &*member; indices = 0;
                    i = end == std::string::npos ? mapping->memberPath.size() : end;
                } else return {uniform.name};
            }
            dimensions = layout->array.dimensions.size();
        }
    }
    const bool array = dimensions != 0 || uniform.size > 1;
    // glslang's default reflection omits the basic array's [0]. Outer struct
    // and multidimensional indices remain fixed parts of the reflected name.
    const bool suffix = array && dimensions == indices && uniform.name.ends_with("[0]");
    return {suffix ? uniform.name.substr(0, uniform.name.size() - 3) : uniform.name, array};
}
double unsignedOffset(double value) {
    if (!std::isfinite(value) || value == 0) return 0;
    constexpr double modulus = 18446744073709551616.0;
    auto result = std::fmod(std::trunc(value), modulus);
    return result < 0 ? result + modulus : result;
}
JSValue dispatch(const JSValue& stateValue, const std::string& name, const Args& args) {
    const auto state = std::static_pointer_cast<State>(stateValue.hostPayload(stateTag));
    if (!state || !state->alive) throw JSTypeError("Vulkan program commands have been shut down");
    const auto a = arg(args, 0), b = arg(args, 1), c = arg(args, 2);
    if (name == "getError") { const auto code = state->error; state->error = 0; return code; }
    if (name == "createShader") {
        const auto type = uint32(a.toNumber());
        if (type != vertexShader && type != fragmentShader) { error(state, invalidEnum); return nullptr; }
        auto shader = std::make_shared<Shader>(); shader->owner = state; shader->type = type;
        return JSValue::hostObject(shader, shader.get(), shaderTag);
    }
    if (name == "createProgram") {
        auto program = std::make_shared<Program>(); program->owner = state;
        std::erase_if(state->programs, [](const auto& weak) {
            const auto value = weak.lock(); return !value || value->retired;
        });
        state->programs.push_back(program);
        auto result = JSValue::hostObject(program, program.get(), programTag);
        result.setInternalSlot("shaders", JSValue::array()); return result;
    }
    if (name == "shaderSource" || name == "compileShader" || name == "getShaderParameter" ||
        name == "getShaderInfoLog" || name == "getShaderSource" || name == "deleteShader") {
        const auto shader = handle<Shader>(a, shaderTag, state); if (!shader) return nullptr;
        if (name == "shaderSource") { shader->source = b.toString(); return {}; }
        if (name == "compileShader") {
#ifdef THREECPP_HAS_NATIVE_ESSL
            const auto result = VulkanShaderCompiler::validateESSLStage(shader->type == vertexShader ? VulkanEsslStage::Vertex : VulkanEsslStage::Fragment, shader->source);
            shader->compiled = result.compiled; shader->diagnostics = result.diagnostics;
            return {};
#else
            throw std::logic_error("Native ESSL program compilation requires THREECPP_NATIVE_ESSL");
#endif
        }
        if (name == "deleteShader") { shader->deleted = true; return {}; }
        if (name == "getShaderInfoLog") return shader->diagnostics;
        if (name == "getShaderSource") return shader->source;
        const auto parameter = uint32(b.toNumber());
        if (parameter == 0x8b81) return shader->compiled;
        if (parameter == 0x8b80) return shader->deleted;
        if (parameter == 0x8b4f) return shader->type;
        error(state, invalidEnum); return nullptr;
    }
    if (name.starts_with("uniform")) {
        const auto setter = uniformSetter(name);
        const bool matrix = setter.matrix, vector = name.ends_with('v');
        std::vector<double> values;
        if (vector) {
            const auto array = arg(args, matrix ? 2 : 1);
            const auto length = array.get("length").toNumber();
            if (!std::isfinite(length) || length < 0 || length > UINT32_MAX) throw JSTypeError("Invalid uniform list length");
            const auto offsetArg = arg(args, matrix ? 3 : 2), countArg = arg(args, matrix ? 4 : 3);
            const auto offsetNumber = offsetArg.isUndefined() ? 0. : unsignedOffset(offsetArg.toNumber());
            const auto requested = countArg.isUndefined() ? 0u : uint32(countArg.toNumber());
            if (offsetNumber > length || (requested && requested > length - offsetNumber)) { error(state, invalidValue); return {}; }
            const auto offset = static_cast<std::uint32_t>(offsetNumber);
            const auto count = requested ? requested : static_cast<std::uint32_t>(length) - offset;
            values.reserve(count);
            for (std::uint32_t i = 0; i < count; ++i) values.push_back(array.get(JSValue(double(offset) + i)).toNumber());
        } else for (std::size_t i = 1; i <= setter.components(); ++i) values.push_back(arg(args, i).toNumber());
        if (a.isNull()) return {};
        const auto location = handle<Location>(a, locationTag, state); if (!location) return {};
        const auto program = location->program.lock();
        const auto current = stateValue.getInternalSlot("currentProgram");
        if (!program || !program->linked || !program->data || program->data->generation != location->generation ||
            !current.isHostObject(programTag) || current.hostPayload(programTag).get() != program.get()) {
            error(state, invalidOperation); return {};
        }
        if (matrix && b.toBoolean()) { error(state, invalidValue); return {}; }
        if (vector && (values.empty() || values.size() % setter.components())) { error(state, invalidValue); return {}; }
        const auto type = uniformType(location->type);
        if (!accepts(type, setter) || (!location->array && values.size() > setter.components())) {
            error(state, invalidOperation); return {};
        }
        // ES ignores complete elements past an array's active end. Storage
        // performs strict bounds checks, so trim only after protocol validation.
        if (location->array) values.resize(std::min(values.size(), std::size_t(location->size) * setter.components()));
        auto& uniforms = *program->data->uniforms;
        try {
            if (type.scalar == UniformScalar::Bool) {
                std::vector<std::int32_t> converted;
                for (const auto value : values) converted.push_back(setter.scalar == UniformScalar::Float ? float(value) != 0.f :
                    setter.scalar == UniformScalar::Uint ? uint32(value) != 0 : int32(value) != 0);
                uniforms.writeBooleans(location->name, converted);
            } else if (setter.scalar == UniformScalar::Uint) {
                std::vector<std::uint32_t> converted; for (const auto value : values) converted.push_back(uint32(value));
                uniforms.write(location->name, converted);
            } else if (setter.scalar == UniformScalar::Int) {
                std::vector<std::int32_t> converted; for (const auto value : values) converted.push_back(int32(value));
                if (samplerType(location->type)) uniforms.writeSamplerUnits(location->name, converted);
                else uniforms.write(location->name, converted);
            } else {
                std::vector<float> converted; for (const auto value : values) converted.push_back(float(value));
                uniforms.write(location->name, converted);
            }
            ++program->data->uniformRevision;
        } catch (const std::invalid_argument&) { error(state, invalidOperation); }
        return {};
    }
    if (name == "useProgram" && a.isNull()) { selectProgram(stateValue, nullptr); return {}; }
    if (name == "deleteProgram" && a.isNull()) return {};
    const auto program = handle<Program>(a, programTag, state);
    if (!program) return nullptr;
    if (name == "deleteProgram") {
        if (program->deleted) return {};
        program->deleted = true;
        if (!stateValue.getInternalSlot("currentProgram").sameValue(a)) retireProgram(a, program);
        return {};
    }
    if (program->retired) { error(state, invalidValue); return name == "getAttribLocation" ? JSValue(-1) : JSValue(nullptr); }
    if (name == "attachShader" || name == "detachShader") {
        const auto shader = handle<Shader>(b, shaderTag, state); if (!shader) return {};
        auto list = a.getInternalSlot("shaders");
        for (double i = 0; i < list.length(); ++i) if (list.get(JSValue(i)).sameValue(b)) {
            if (name == "detachShader") list.get("splice").call(list, {i, 1});
            else error(state, invalidOperation);
            return {};
        }
        if (name == "attachShader") list.get("push").call(list, {b}); else error(state, invalidOperation);
        return {};
    }
    if (name == "bindAttribLocation") { program->attributes[c.toString()] = uint32(b.toNumber()); return {}; }
    if (name == "linkProgram") {
        program->linked = program->validated = false; program->diagnostics.clear();
        VulkanEsslSourcePair pair; bool vertex = false, fragment = false;
        const auto list = a.getInternalSlot("shaders");
        for (double i = 0; i < list.length(); ++i) {
            const auto shader = handle<Shader>(list.get(JSValue(i)), shaderTag, state); if (!shader) continue;
            if (!shader->compiled) { program->diagnostics = shader->diagnostics; return {}; }
            if (shader->type == vertexShader) { if (vertex) { program->diagnostics = "Multiple vertex shaders"; return {}; } vertex = true; pair.vertex = shader->source; }
            else { if (fragment) { program->diagnostics = "Multiple fragment shaders"; return {}; } fragment = true; pair.fragment = shader->source; }
        }
        if (!vertex || !fragment) { program->diagnostics = "A vertex and fragment shader are required"; return {}; }
#ifdef THREECPP_HAS_NATIVE_ESSL
        std::vector<VulkanEsslAttributeBinding> bindings;
        for (const auto& [attribute, index] : program->attributes) bindings.push_back({attribute, index});
        auto linked = std::make_shared<Linked>(); linked->result = VulkanShaderCompiler::compileESSL(pair, bindings);
        program->diagnostics = linked->result.diagnostics;
        if (!linked->result.linked) return {};
        linked->uniforms = std::make_unique<VulkanProgramUniforms>(linked->result);
        for (const auto& attribute : linked->result.activeAttributes) {
            const auto mapping = std::find_if(linked->result.attributeMappings.begin(), linked->result.attributeMappings.end(),
                [&](const auto& value) { return value.sourceName == attribute.name; });
            if (mapping != linked->result.attributeMappings.end() && mapping->builtIn < 0) linked->activeAttributes.push_back(attribute);
        }
        linked->generation = ++state->generation; program->data = linked;
        program->linked = program->validated = true; return {};
#else
        throw std::logic_error("Native ESSL program compilation requires THREECPP_NATIVE_ESSL");
#endif
    }
    if (name == "useProgram") {
        if (!program->linked) { error(state, invalidOperation); return {}; }
        selectProgram(stateValue, a); return {};
    }
    if (name == "validateProgram") { program->validated = program->linked; return {}; }
    if (name == "getProgramInfoLog") return program->diagnostics;
    if (name == "getProgramParameter") {
        const auto parameter = uint32(b.toNumber());
        if (parameter == 0x8b82) return program->linked;
        if (parameter == 0x8b83) return program->validated;
        if (parameter == 0x8b80) return program->deleted;
        if (parameter == 0x8b89) return program->data && program->linked ? double(program->data->activeAttributes.size()) : 0.;
        if (parameter == 0x8b86) return program->data && program->linked ? double(program->data->result.activeUniforms.size()) : 0.;
        if (parameter == 0x8b85) return a.getInternalSlot("shaders").length();
        error(state, invalidEnum); return nullptr;
    }
    if (!program->linked || !program->data) { error(state, invalidOperation); return nullptr; }
    const auto& reflection = program->data->result;
    if (name == "getActiveUniform") {
        const auto index = uint32(b.toNumber()); if (index >= reflection.activeUniforms.size()) { error(state, invalidValue); return nullptr; }
        const auto& value = reflection.activeUniforms[index];
        const auto array = uniformArray(reflection, value);
        return info(array.array ? array.base + "[0]" : value.name, value.size, value.glType);
    }
    if (name == "getActiveAttrib") {
        const auto index = uint32(b.toNumber()); if (index >= program->data->activeAttributes.size()) { error(state, invalidValue); return nullptr; }
        const auto& value = program->data->activeAttributes[index]; return info(value.name, value.size, value.glType);
    }
    if (name == "getAttribLocation") {
        const auto key = b.toString();
        for (const auto& value : reflection.attributeMappings) if (value.sourceName == key && value.builtIn < 0) return value.location;
        return -1;
    }
    if (name == "getUniformLocation") {
        auto key = b.toString();
        for (const auto& value : reflection.activeUniforms) {
            if (value.block >= 0) continue;
            const auto array = uniformArray(reflection, value);
            unsigned index = 0;
            if (array.array) {
                if (key != array.base) {
                    if (!key.starts_with(array.base + "[") || !key.ends_with(']')) continue;
                    const auto* begin = key.data() + array.base.size() + 1;
                    const auto* end = key.data() + key.size() - 1;
                    const auto parsed = std::from_chars(begin, end, index);
                    if (parsed.ec != std::errc{} || parsed.ptr != end || index >= unsigned(value.size)) continue;
                }
            } else if (value.name != key) continue;
            auto location = std::make_shared<Location>(); location->owner = state; location->program = program;
            location->generation = program->data->generation;
            location->name = array.array ? array.base + "[" + std::to_string(index) + "]" : value.name;
            location->type = value.glType; location->size = value.size - int(index); location->array = array.array;
            return JSValue::hostObject(location, location.get(), locationTag);
        }
        return nullptr;
    }
    throw std::logic_error("Unsupported native program command: " + name);
}
}

VulkanProgramCommands::VulkanProgramCommands() {
    const auto state = std::make_shared<State>(); state_ = JSValue::hostObject(state, state.get(), stateTag);
    state_.setInternalSlot("currentProgram", nullptr); object_ = JSValue::object();
    for (const auto& [key, value] : std::initializer_list<std::pair<const char*, std::uint32_t>>{
        {"VERTEX_SHADER",vertexShader},{"FRAGMENT_SHADER",fragmentShader},{"COMPILE_STATUS",0x8b81},
        {"LINK_STATUS",0x8b82},{"VALIDATE_STATUS",0x8b83},{"DELETE_STATUS",0x8b80},
        {"ACTIVE_UNIFORMS",0x8b86},{"ACTIVE_ATTRIBUTES",0x8b89},{"ATTACHED_SHADERS",0x8b85},
        {"FLOAT",0x1406},{"INT",0x1404},{"UNSIGNED_INT",0x1405},
        {"FLOAT_MAT2",0x8b5a},{"FLOAT_MAT3",0x8b5b},{"FLOAT_MAT4",0x8b5c},
        {"NO_ERROR",0},{"INVALID_ENUM",invalidEnum},{"INVALID_VALUE",invalidValue},{"INVALID_OPERATION",invalidOperation}})
        own(object_, key, value);
    std::vector<std::string> methods{"createShader","shaderSource","compileShader","getShaderParameter","getShaderInfoLog","getShaderSource","deleteShader",
        "createProgram","attachShader","detachShader","bindAttribLocation","linkProgram","useProgram","deleteProgram","validateProgram",
        "getProgramParameter","getProgramInfoLog","getActiveUniform","getActiveAttrib","getUniformLocation","getAttribLocation","getError"};
    for (unsigned size = 1; size <= 4; ++size) for (const auto* suffix : {"f","fv","i","iv","ui","uiv"}) methods.push_back("uniform" + std::to_string(size) + suffix);
    for (const auto* dimensions : {"2","3","4","2x3","2x4","3x2","3x4","4x2","4x3"}) methods.push_back(std::string("uniformMatrix") + dimensions + "fv");
    for (const auto& name : methods) own(object_, name.c_str(), JSValue::function(+[](const JSValue&, const Args& args, const Args& captures) {
        return dispatch(captures[0], captures[1].string(), args);
    }, {state_, name}, name));
}
JSValue VulkanProgramCommands::object() const { return object_; }
JSValue VulkanProgramCommands::currentProgram() const { return state_.getInternalSlot("currentProgram"); }
std::shared_ptr<VulkanProgramCommands::LinkedProgram> VulkanProgramCommands::linked(const JSValue& program) const {
    const auto state = std::static_pointer_cast<State>(state_.hostPayload(stateTag));
    if (!state || !state->alive || !program.isHostObject(programTag)) return {};
    const auto value = std::static_pointer_cast<Program>(program.hostPayload(programTag));
    return value->owner.lock() == state && value->linked ? value->data : nullptr;
}
std::shared_ptr<VulkanProgramCommands::LinkedProgram> VulkanProgramCommands::current() const { return linked(currentProgram()); }
void VulkanProgramCommands::shutdown() {
    const auto state = std::static_pointer_cast<State>(state_.hostPayload(stateTag));
    if (!state->alive) return;
    state->alive = false; state_.setInternalSlot("currentProgram", nullptr);
    for (const auto& weak : state->programs) if (const auto program = weak.lock())
        retireProgram(state_.findHostObject(program.get(), programTag), program);
    state->programs.clear();
}
