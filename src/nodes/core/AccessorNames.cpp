#include "nodes/core/AccessorNames.h"

std::string AccessorNames::uvAttribute(int index) {
    // The number is appended only when it is greater than nought.
    if (index > 0) return "uv" + std::to_string(index);
    return "uv";
}

std::string AccessorNames::modelViewProjectionName() {
    // The accessor is turned into a varying, and a varying carries the v_ prefix the shader
    // stages share it under. The name without the prefix would be a different varying.
    return "v_modelViewProjection";
}
