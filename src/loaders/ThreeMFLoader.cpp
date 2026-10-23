#include "loaders/ThreeMFLoader.h"

#include <limits>

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

std::vector<double> ThreeMFLoader::transformNumbers(const std::string& transform) {
    std::vector<double> numbers;
    // Split on a single space, not on any whitespace: a second space produces an empty piece.
    std::size_t start = 0;
    while (true) {
        const std::size_t space = transform.find(' ', start);
        const std::string piece =
            space == std::string::npos ? transform.substr(start) : transform.substr(start, space - start);
        // An empty piece is not a number, and the source lets that through rather than skipping
        // it, so the count can exceed what the caller expects and the values can be invalid.
        if (piece.empty()) {
            numbers.push_back(std::numeric_limits<double>::quiet_NaN());
        } else {
            try {
                numbers.push_back(std::stod(piece));
            } catch (...) {
                numbers.push_back(std::numeric_limits<double>::quiet_NaN());
            }
        }
        if (space == std::string::npos) break;
        start = space + 1;
    }
    return numbers;
}

std::vector<double> ThreeMFLoader::transformMatrix(const std::string& transform) {
    const auto t = transformNumbers(transform);
    // A document that writes fewer than twelve leaves the rest not a number, which is what
    // reading past the end of an array does.
    const auto at = [&t](std::size_t index) {
        return index < t.size() ? t[index] : std::numeric_limits<double>::quiet_NaN();
    };
    // The matrix takes its numbers row by row, while the document writes them column by column.
    return {at(0), at(3), at(6), at(9),
            at(1), at(4), at(7), at(10),
            at(2), at(5), at(8), at(11),
            0.0,   0.0,   0.0,   1.0};
}