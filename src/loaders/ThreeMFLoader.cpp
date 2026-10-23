#include "loaders/ThreeMFLoader.h"

std::string ThreeMFLoader::unitOf(const std::string& declared) {
    // An absent attribute reads as the empty string, which the format fills in with millimeters.
    if (declared.empty()) return kDefaultUnit;
    return declared;
}

bool ThreeMFLoader::unitIsApplied() {
    // The attribute is recorded and never read, so nothing in the loader scales by it.
    return false;
}

const std::vector<std::string>& ThreeMFLoader::metadataNames() {
    // The eight the format defines, in the order the source lists them.
    static const std::vector<std::string> names{"Title",        "Designer",  "Description",
                                                "Copyright",    "LicenseTerms", "Rating",
                                                "CreationDate", "ModificationDate"};
    return names;
}

bool ThreeMFLoader::isMetadataName(const std::string& name) {
    // A name outside the list is skipped.
    for (const auto& valid : metadataNames()) {
        if (valid == name) return true;
    }
    return false;
}

std::string ThreeMFLoader::objectIdOf(const std::string& written) {
    // Kept exactly as written; the loader only looks the object up by it.
    return written;
}

bool ThreeMFLoader::carriesTransform(const std::string& transform) {
    // An attribute that is absent and one that is empty both fail this test, which is what the
    // source does by testing the string itself.
    return !transform.empty();
}