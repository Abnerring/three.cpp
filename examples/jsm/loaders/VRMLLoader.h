#pragma once
#include <string>

// Port of the rules in examples/jsm/loaders/VRMLLoader.js at the pinned revision. The loader reads
// a scene description with a parser library, and three of its rules are worth writing down.
class VRMLLoader {
  public:
    static constexpr const char* kTypeName = "VRMLLoader";

    // Where the parser library comes from. It is named by a relative path inside the addons set
    // rather than by an address, so it ships with the code and a build that used this module
    // would carry it. Two other loaders in this set fetch their libraries from a content
    // delivery address instead, so the three together show that there is no single convention.
    static constexpr const char* kParserLibraryPath = "../libs/chevrotain.module.min.js";

    static bool parserLibraryIsBundled();

    // What a failed load tells the manager. It reports the failure and nothing else, so it is in
    // the group the collada and font loaders belong to rather than the one the materialx loader
    // does, which also reports the end.
    static int managerCallsOnFailure();

    // The base path a load uses, which is the loader's own unless that is the empty string; this
    // is the same test the collada and usd loaders use.
    static std::string basePath(const std::string& loaderPath, const std::string& urlDirectory);

    // What parsing answers. A scene is produced rather than a group, so a caller adding the answer
    // to something is adding a scene to it rather than a container of one.
    static constexpr const char* kParseResultType = "Scene";

    // The kind of data read from the file, which is text: the format is a description rather than
    // a binary container.
    static constexpr const char* kResponseType = "text";
};
