#include "loaders/KMZLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("KMZLoader finds a file by the tail of its path") {
    const std::vector<std::string> paths{"models/duck.dae", "doc.kml", "images/tex.png"};
    // A request for just the name finds the file under its directory, which is what makes a
    // model's own references work whatever folder the archive keeps them in.
    REQUIRE(KMZLoader::findByPath(paths, "duck.dae") == 0);
    REQUIRE(KMZLoader::findByPath(paths, "tex.png") == 2);
    // The whole path works too, since it ends with itself.
    REQUIRE(KMZLoader::findByPath(paths, "models/duck.dae") == 0);
    // A name that is not a tail of anything is not found.
    REQUIRE(KMZLoader::findByPath(paths, "missing.dae") == -1);
    // A request longer than the path cannot be a tail of it.
    REQUIRE(KMZLoader::findByPath({"a.dae"}, "longer/a.dae") == -1);
    // An empty list finds nothing.
    REQUIRE(KMZLoader::findByPath({}, "a.dae") == -1);
}

TEST_CASE("KMZLoader reads the extension of a path") {
    // Everything after the last dot, lower cased.
    REQUIRE(KMZLoader::extensionOf("models/duck.DAE") == "dae");
    REQUIRE(KMZLoader::extensionOf("doc.kml") == "kml");
    // A path with several dots uses the last.
    REQUIRE(KMZLoader::extensionOf("a.b.c.dae") == "dae");
    // A path with none has no extension.
    REQUIRE(KMZLoader::extensionOf("noext").empty());
    REQUIRE(KMZLoader::extensionOf("").empty());
    // A dot at the end gives an empty extension rather than a dot.
    REQUIRE(KMZLoader::extensionOf("trailing.").empty());
}

TEST_CASE("KMZLoader wants the document by its exact name") {
    // The document is looked up exactly, unlike the lookups above.
    const std::vector<std::string> paths{"models/duck.dae", "doc.kml"};
    REQUIRE(KMZLoader::documentIndex(paths) == 1);
    // The same name under a directory is not the document.
    REQUIRE(KMZLoader::documentIndex({"sub/doc.kml"}) == -1);
    // Nor is a longer name that ends with it.
    REQUIRE(KMZLoader::documentIndex({"mydoc.kml"}) == -1);
    // An archive without one says so.
    REQUIRE(KMZLoader::documentIndex({"a.dae"}) == -1);
}

TEST_CASE("KMZLoader falls back to the first model") {
    // The first file with the model's extension is taken, whatever else is there.
    REQUIRE(KMZLoader::modelIndex({"readme.txt", "one.dae", "two.dae"}) == 1);
    // The case of the extension does not matter, since it is lowered first.
    REQUIRE(KMZLoader::modelIndex({"model.DAE"}) == 0);
    // An archive with no model gives nothing.
    REQUIRE(KMZLoader::modelIndex({"a.kml", "b.png"}) == -1);
    REQUIRE(KMZLoader::modelIndex({}) == -1);
}

TEST_CASE("KMZLoader names the document it expects") {
    REQUIRE(std::string(KMZLoader::kDocumentName) == "doc.kml");
    REQUIRE(KMZLoader::missingDocumentWarning() == "KMZLoader: Missing doc.kml file.");
}