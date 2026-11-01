#pragma once
#include <string>
#include <vector>

// Port of the small confirmed parts of examples/jsm/loaders/3MFLoader.js at the pinned revision.
// The loader is one long parse over an archive of documents, so what is covered here is what can
// be stated on its own.
class ThreeMFLoader {
  public:
    // A 3MF package is a zip archive whose model document declares the unit its numbers are in.
    // The attribute is optional and the format names the fallback.
    static constexpr const char* kDefaultUnit = "millimeter";

    // The space the colours in a 3MF document are written in. The loader names it once at the top
    // of the file rather than writing it out at each use.
    static constexpr const char* kColourSpace = "srgb";

    // The unit a document declares, or the fallback when the attribute says nothing.
    static std::string unitOf(const std::string& declared);

    // Whether the loader scales the geometry by that unit. It does not: the source reads the
    // attribute exactly once, stores it, and never consults it again, so a model authored in
    // inches arrives with the numbers its document wrote. The format expects the unit to be
    // honoured, so this is a difference from the specification rather than a subtlety of it.
    static bool unitIsApplied();
    // The eight metadata names the format defines. A metadata element with any other name is
    // skipped rather than kept, so an extension's own metadata never reaches the model.
    static const std::vector<std::string>& metadataNames();

    // Whether a name is one of them. The comparison is exact and case sensitive.
    static bool isMetadataName(const std::string& name);

    // A build item's object id is kept as the string the document wrote, not turned into a
    // number, because the format lets it be any identifier and the loader only uses it to look
    // the object up again.
    static std::string objectIdOf(const std::string& written);

    // Whether a build item carries a transform. An attribute that is present but empty reads as
    // the empty string, which does not count, so a document writing transform="" gets no
    // transform rather than an identity one.
    static bool carriesTransform(const std::string& transform);
    // The twelve numbers of a build item's transform, in the order the document writes them.
    // The split is on a single space rather than on any whitespace, so two spaces in a row give
    // an empty piece, and an empty piece reads as not a number. That a run of spaces can poison
    // a matrix is a property of the source rather than of the format.
    static std::vector<double> transformNumbers(const std::string& transform);

    // The sixteen numbers of the matrix, row by row as the matrix takes them. The document
    // writes its twelve column by column, so the first four of the result are the first, fourth,
    // seventh and tenth the document wrote. The last row is the constant one.
    static std::vector<double> transformMatrix(const std::string& transform);

    // How many numbers a document must write for a transform to be complete.
    static constexpr int kTransformNumberCount = 12;
    // The resources a document may hold. The source reads them one kind at a time in this order
    // and collects each into its own map, keyed by the id the document gave.
    static const std::vector<std::string>& resourceKinds();

    // An object's type is taken as written and never checked against a list, which is the
    // opposite of how a metadata name is treated: a type the loader does not know still reaches
    // the model, while a metadata name it does not know is dropped.
    static std::string objectTypeOf(const std::string& written);

    // Whether an object's type is one the loader knows how to build. The three it builds are the
    // model, the solid support and the support; anything else keeps its type but is not built.
    static bool buildsObjectType(const std::string& type);

    // The attribute a relationship's type is read from is written with a capital T, while an
    // object's is written in lower case. The two are not interchangeable.
    static constexpr const char* kRelationshipTypeAttribute = "Type";
    static constexpr const char* kObjectTypeAttribute = "type";

    // Whether a relationship points at the model document. The extension is lowered before it is
    // compared, so a document named in capitals is found.
    static bool isModelRelationship(const std::string& type);
};
