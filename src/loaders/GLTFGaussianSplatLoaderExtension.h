#pragma once
#include <string>
#include <vector>

// Port of the naming and detection helpers in GLTFGaussianSplatLoaderExtension.js at the pinned
// revision. A glTF primitive marked with the Gaussian splatting extension carries its fields as
// ordinary attributes whose semantics are glTF names; the loader has to map those to the
// attribute names its geometry uses. The geometry itself needs the glTF loader and the splat
// object, neither of which is portable here, so only the naming rules live in this class.
class GLTFGaussianSplatLoaderExtension {
  public:
    static constexpr const char* kExtensionName = "KHR_gaussian_splatting";
    // The primitive mode points stand for.
    static constexpr int kPointsMode = 0;

    // The attribute name a semantic maps to. Only the position has an explicit entry; every
    // other semantic is its own name lower cased, so SCALE becomes scale and _ROTATION
    // becomes _rotation.
    static std::string attributeName(const std::string& semantic);

    // Whether a primitive carries the extension, given the names its extensions object holds.
    static bool isGaussianSplatPrimitive(const std::vector<std::string>& extensionNames);

    // The messages the two accessors raise. The first is for a semantic the primitive does not
    // declare, the second for one it declares but whose data is missing.
    static std::string requiredMessage(const std::string& semantic);
    static std::string missingMessage(const std::string& semantic);
};
