#include "loaders/USDLoader.h"

std::string USDLoader::extension(const std::string& filename) {
    const auto lastDot = filename.find_last_of('.');
    if (lastDot == std::string::npos) return std::string();
    const auto lastSlash = filename.find_last_of('/');
    // A slash after the dot means the dot belongs to a directory, not to the file.
    if (lastSlash != std::string::npos && lastSlash > lastDot) return std::string();
    std::string out = filename.substr(lastDot + 1);
    for (auto& c : out) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

USDLoader::BufferCase USDLoader::bufferCase(bool isWholeBuffer, bool viewCoversAll) {
    if (isWholeBuffer) return BufferCase::WholeBuffer;
    if (viewCoversAll) return BufferCase::FullView;
    return BufferCase::PartialView;
}

std::string USDLoader::basePath(const std::string& loaderPath,
                                const std::string& urlDirectory) {
    if (loaderPath.empty()) return urlDirectory;
    return loaderPath;
}

bool USDLoader::returnsBeforeTexturesLoaded() {
    return true;
}
