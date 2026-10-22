#include "loaders/GLTFGaussianSplatLoaderExtension.h"

#include <algorithm>
#include <cctype>

std::string GLTFGaussianSplatLoaderExtension::attributeName(const std::string& semantic) {
    // Position is the one semantic whose glTF name differs from its attribute name in case
    // only, and it is listed explicitly.
    if (semantic == "POSITION") return "position";
    // Every other semantic is its own name lower cased.
    std::string name = semantic;
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return name;
}

bool GLTFGaussianSplatLoaderExtension::isGaussianSplatPrimitive(
    const std::vector<std::string>& extensionNames) {
    // A primitive has the extension when its extensions object names it.
    return std::find(extensionNames.begin(), extensionNames.end(), kExtensionName) !=
           extensionNames.end();
}

std::string GLTFGaussianSplatLoaderExtension::requiredMessage(const std::string& semantic) {
    return std::string("THREE.GLTFGaussianSplatLoaderExtension: KHR_gaussian_splatting requires ") +
           semantic + ".";
}

std::string GLTFGaussianSplatLoaderExtension::missingMessage(const std::string& semantic) {
    return std::string("THREE.GLTFGaussianSplatLoaderExtension: KHR_gaussian_splatting attribute ") +
           semantic + " was not loaded.";
}
