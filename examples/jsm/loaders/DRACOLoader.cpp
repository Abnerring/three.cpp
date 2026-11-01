#include "loaders/DRACOLoader.h"

std::string DRACOLoader::decoderAttributeID(const std::string& attributeName) {
    if (attributeName == "position") return "POSITION";
    if (attributeName == "normal") return "NORMAL";
    if (attributeName == "color") return "COLOR";
    // The odd one: not the key in capitals.
    if (attributeName == "uv") return "TEX_COORD";
    return std::string();
}

std::string DRACOLoader::decoderAttributeType(const std::string& attributeName) {
    // All four are floating point arrays, the colour included.
    if (attributeName == "position" || attributeName == "normal" || attributeName == "color" ||
        attributeName == "uv") {
        return "Float32Array";
    }
    return std::string();
}

DRACOLoader::DecoderPaths DRACOLoader::pathsForObjectForm() {
    DecoderPaths paths;
    paths.hasJs = true;
    paths.hasWasm = true;
    // The plain JavaScript decoder is set to nothing by the object form.
    paths.hasDependencyJs = false;
    return paths;
}

DRACOLoader::DecoderPaths DRACOLoader::pathsForStringForm() {
    DecoderPaths paths;
    paths.hasJs = true;
    paths.hasWasm = true;
    paths.hasDependencyJs = true;
    return paths;
}

bool DRACOLoader::objectFormKeepsFallback() {
    return false;
}
