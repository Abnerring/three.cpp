#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the selection logic in examples/jsm/loaders/KMZLoader.js at the pinned revision. A KMZ
// file is a zip archive holding a placemark document and the model it points at; the parsing of
// the model itself belongs to another loader, so what lives here is deciding which file is which.
class KMZLoader {
  public:
    // The name of the document the format expects at the root of the archive.
    static constexpr const char* kDocumentName = "doc.kml";

    // The path a file's name ends with, which is how the source looks a file up. The comparison
    // is a suffix one rather than an exact one: a request for a name that is the tail of a
    // longer path finds that file, so a model referenced as its own name is found under the
    // directory the archive happens to keep it in.
    static int findByPath(const std::vector<std::string>& paths, const std::string& url);

    // The extension a path ends with, lower cased. A path with no dot has none.
    static std::string extensionOf(const std::string& path);

    // The index of the document, or minus one. The name must match exactly, unlike the lookups
    // above.
    static int documentIndex(const std::vector<std::string>& paths);

    // The index of the first file whose extension is the one a model uses, which is what the
    // archive is searched for when it holds no document.
    static int modelIndex(const std::vector<std::string>& paths);

    // The message printed when the document is missing, which the archive then works around.
    static std::string missingDocumentWarning();
};
