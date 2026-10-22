#pragma once
#include <array>
#include <string_view>

// Port of src/nodes/core/constants.js at the pinned revision. Upstream exports plain
// string enums and arrays; these are constexpr string views and arrays with the same
// values and the same order.
namespace NodeShaderStage {
inline constexpr std::string_view VERTEX = "vertex";
inline constexpr std::string_view FRAGMENT = "fragment";
}  // namespace NodeShaderStage

namespace NodeUpdateType {
inline constexpr std::string_view NONE = "none";
inline constexpr std::string_view FRAME = "frame";
inline constexpr std::string_view RENDER = "render";
inline constexpr std::string_view OBJECT = "object";
}  // namespace NodeUpdateType

namespace NodeType {
inline constexpr std::string_view BOOLEAN = "bool";
inline constexpr std::string_view INTEGER = "int";
inline constexpr std::string_view FLOAT = "float";
inline constexpr std::string_view VECTOR2 = "vec2";
inline constexpr std::string_view VECTOR3 = "vec3";
inline constexpr std::string_view VECTOR4 = "vec4";
inline constexpr std::string_view MATRIX2 = "mat2";
inline constexpr std::string_view MATRIX3 = "mat3";
inline constexpr std::string_view MATRIX4 = "mat4";
}  // namespace NodeType

namespace NodeAccess {
inline constexpr std::string_view READ_ONLY = "readOnly";
inline constexpr std::string_view WRITE_ONLY = "writeOnly";
inline constexpr std::string_view READ_WRITE = "readWrite";
}  // namespace NodeAccess

inline constexpr std::array<std::string_view, 2> defaultShaderStages{"fragment", "vertex"};
inline constexpr std::array<std::string_view, 3> defaultBuildStages{"setup", "analyze", "generate"};
inline constexpr std::array<std::string_view, 3> shaderStages{"fragment", "vertex", "compute"};
inline constexpr std::array<std::string_view, 4> vectorComponents{"x", "y", "z", "w"};
