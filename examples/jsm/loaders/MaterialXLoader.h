#pragma once
#include <string>

// Port of the rules in examples/jsm/loaders/MaterialXLoader.js at the pinned revision. The loader
// reads a material document and, in some cases, an archive of it with its textures, and four of
// its rules are worth writing down.
class MaterialXLoader {
  public:
    static constexpr const char* kTypeName = "MaterialXLoader";

    // The base path for the resources a document names. The loader's own path is used when it is
    // a value, and the URL's directory otherwise; the directory keeps its trailing slash, so the
    // answer can be joined to a name without further thought. The test here is for a value rather
    // than for the empty string, which is what the two loaders beside this one use, so a path of
    // nought characters falls through while an absent one does too.
    static std::string resourcePath(const std::string& loaderPath, const std::string& url);

    // What disposing does. The disposer kept from the last archive is run and then forgotten, so
    // a second disposal has nothing to do; that makes the call safe to make twice, which a caller
    // tearing down in more than one place will.
    enum class DisposeEffect { Nothing, RanAndForgot };

    static DisposeEffect disposeEffect(bool hasDisposer);

    // Whether the manager is told the item started. It is, before the file is asked for, so the
    // manager counts the work from the moment the call is made rather than from the moment the
    // bytes arrive.
    static bool reportsStart();

    // How many manager calls a failure makes. This loader tells the manager the item failed and
    // then that it ended, so it makes two; the collada and font loaders make only the first, so a
    // caller comparing their own counts across formats would find a difference that is not in the
    // format.
    static int managerCallsOnFailure();

    // The kind of data read from the file, which is raw bytes because the document may be inside
    // an archive.
    static constexpr const char* kResponseType = "arraybuffer";

    // The order of the two calls a failure makes.
    static bool errorBeforeEnd();
};
