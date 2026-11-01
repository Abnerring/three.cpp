#pragma once
#include <string>

// Port of the rules in examples/jsm/loaders/ColladaLoader.js at the pinned revision. The class
// itself is a short orchestration over a parser and a composer, and three of its rules are worth
// writing down.
class ColladaLoader {
  public:
    static constexpr const char* kTypeName = "ColladaLoader";

    // What parsing an empty text gives. It is an object holding an empty scene rather than
    // nothing, so a caller always receives something to add to its own scene; a text that fails
    // to parse, by contrast, gives nothing at all. The two failures therefore differ, and a
    // caller testing only for an empty answer would treat them alike.
    enum class Outcome { EmptyScene, Nothing, Composed };

    static Outcome outcomeFor(bool textIsEmpty, bool parserFailed, bool composerRan);

    // Whether an empty text reaches the parser at all, which it does not.
    static bool emptyTextReachesTheParser();

    // The base path a load uses. It is the loader's own when that is a value, and the URL's
    // directory otherwise; the test is against the empty string exactly, so a path of nought
    // characters is the only one replaced.
    static std::string basePath(const std::string& loaderPath, const std::string& urlDirectory);

    // Whether the guard around the TGA loader can ever be false. The name it tests is an imported
    // module, which is always a value, so the branch is taken every time and the test is a no-op;
    // it is copied as it stands rather than dropped, since a reader comparing the two sources
    // should find the same shape.
    static bool tgaGuardIsAlwaysTrue();

    // The kind of data read from the file, which is text rather than bytes.
    static constexpr const char* kResponseType = "text";
};
