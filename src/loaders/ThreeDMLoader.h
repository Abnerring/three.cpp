#pragma once
#include <string>

// Port of the rules in examples/jsm/loaders/3DMLoader.js at the pinned revision. The loader reads a
// modeler's own format through a library it fetches, and four of its rules are worth writing down.
class ThreeDMLoader {
  public:
    static constexpr const char* kTypeName = "Rhino3dmLoader";

    // The library the loader needs, which it fetches from a content delivery address rather than
    // carrying. This is the second loader in the addons set to do that, the font one being the
    // first, so the pattern is not a single accident.
    static constexpr const char* kLibraryVersion = "8.32.1";

    static bool libraryIsBundled();

    // The subdivision level applied to smooth surfaces before they are turned into a mesh. It is
    // three by default, which is a choice rather than a neutral value: a higher number makes the
    // surface smoother at the cost of a vertex count that grows quickly, so a caller who never
    // touches the setting still gets a mesh that was subdivided three times.
    static constexpr int kDefaultSubdivisionLevel = 3;

    // Whether a setting of nought means no subdivision at all, which it does: the level counts
    // how many times the surface is split, so nought leaves it as it is.
    static bool subdivisionIsOff(int level);

    // The worker settings, which are the same pair the draco loader uses: four at a time and the
    // first task numbered one. Two loaders agreeing on both numbers makes them a convention
    // rather than a coincidence, so a test records both rather than only the one in use here.
    static constexpr int kWorkerLimit = 4;
    static constexpr int kFirstTaskId = 1;

    // Whether the geometry loader used to read the meshes is shared between every instance. It
    // is: the loader is made once outside the class, so one is used for the whole module rather
    // than one per loader.
    static bool geometryLoaderIsShared();

    // Whether a library path has to be given before the loader can work, which it does: the path
    // starts as the empty string rather than as a usable default.
    static bool libraryPathStartsEmpty();
};
