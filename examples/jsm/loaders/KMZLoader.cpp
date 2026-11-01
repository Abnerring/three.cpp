#include "loaders/KMZLoader.h"

#include <cctype>

int KMZLoader::findByPath(const std::vector<std::string>& paths, const std::string& url) {
    // The file whose name ends with the one asked for, which is a suffix test rather than an
    // equality one.
    for (std::size_t i = 0; i < paths.size(); ++i) {
        const std::string& path = paths[i];
        if (path.size() < url.size()) continue;
        if (path.compare(path.size() - url.size(), url.size(), url) == 0) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::string KMZLoader::extensionOf(const std::string& path) {
    // Everything after the last dot, lower cased. A path with no dot has no extension.
    const std::size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return std::string();
    std::string extension = path.substr(dot + 1);
    for (char& c : extension) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return extension;
}

int KMZLoader::documentIndex(const std::vector<std::string>& paths) {
    // The document is looked up by its exact name, not by its tail.
    for (std::size_t i = 0; i < paths.size(); ++i) {
        if (paths[i] == kDocumentName) return static_cast<int>(i);
    }
    return -1;
}

int KMZLoader::modelIndex(const std::vector<std::string>& paths) {
    // The first file with the model's extension is taken.
    for (std::size_t i = 0; i < paths.size(); ++i) {
        if (extensionOf(paths[i]) == "dae") return static_cast<int>(i);
    }
    return -1;
}

std::string KMZLoader::missingDocumentWarning() {
    return "KMZLoader: Missing doc.kml file.";
}
