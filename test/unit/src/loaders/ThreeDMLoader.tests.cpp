#include "loaders/ThreeDMLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ThreeDMLoader fetches its library rather than carrying it") {
    // The example names a content delivery address, so the library arrives when the page runs.
    REQUIRE_FALSE(ThreeDMLoader::libraryIsBundled());
    // And a path has to be set before the loader works, which is why it starts empty.
    REQUIRE(ThreeDMLoader::libraryPathStartsEmpty());
    REQUIRE(std::string(ThreeDMLoader::kLibraryVersion) == "8.32.1");
    REQUIRE(std::string(ThreeDMLoader::kTypeName) == "Rhino3dmLoader");
}

TEST_CASE("ThreeDMLoader subdivides a smooth surface three times by default") {
    // Three is a choice rather than a neutral value: more subdivisions give a smoother surface at
    // a vertex count that grows quickly, so a caller who never touches the setting still gets a
    // surface split three times.
    REQUIRE(ThreeDMLoader::kDefaultSubdivisionLevel == 3);
    REQUIRE(ThreeDMLoader::kDefaultSubdivisionLevel != 0);
    REQUIRE(ThreeDMLoader::kDefaultSubdivisionLevel != 1);
    // Nought really does mean none.
    REQUIRE(ThreeDMLoader::subdivisionIsOff(0));
    REQUIRE(ThreeDMLoader::subdivisionIsOff(-1));
    REQUIRE_FALSE(ThreeDMLoader::subdivisionIsOff(1));
    REQUIRE_FALSE(ThreeDMLoader::subdivisionIsOff(ThreeDMLoader::kDefaultSubdivisionLevel));
}

TEST_CASE("ThreeDMLoader shares the draco loader's worker settings") {
    // The same pair the draco loader uses: four at a time, first task numbered one. Two loaders
    // agreeing on both makes them a convention rather than a coincidence.
    REQUIRE(ThreeDMLoader::kWorkerLimit == 4);
    REQUIRE(ThreeDMLoader::kFirstTaskId == 1);
    REQUIRE(ThreeDMLoader::kFirstTaskId != 0);
    // The geometry loader used for the meshes is made once outside the class, so every loader in
    // the page reuses one.
    REQUIRE(ThreeDMLoader::geometryLoaderIsShared());
}