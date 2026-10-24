#pragma once
#include <string>

// Port of the tables in examples/jsm/loaders/DRACOLoader.js at the pinned revision. The loader runs
// a decoder in workers and maps geometry attribute names onto the decoder's own names, and three
// of its rules are worth writing down.
class DRACOLoader {
  public:
    static constexpr const char* kTypeName = "DRACOLoader";

    // The decoder name of a geometry attribute. Three of the four follow the rule that the name is
    // the key in capitals, and the fourth does not: the coordinate attribute is called TEX_COORD
    // rather than UV, so a reader who assumed the rule would ask the decoder for a name it does
    // not have.
    static std::string decoderAttributeID(const std::string& attributeName);

    // The type every one of the four is read as, which is a floating point array in each case
    // including the colour.
    static std::string decoderAttributeType(const std::string& attributeName);

    // How many workers may run at once, and what the first task is numbered. The numbering starts
    // at one rather than nought, so a task with number nought does not exist and a caller that
    // counted from nought would be off by one.
    static constexpr int kWorkerLimit = 4;
    static constexpr int kFirstTaskId = 1;

    // What setting the decoder path as an object does to the three paths it configures. The two
    // the object names are taken from it, and the third, the plain JavaScript decoder used when
    // the web assembly one cannot run, is set to nothing: so the object form disables that
    // fallback, which the string form does not.
    struct DecoderPaths {
        bool hasJs = false;
        bool hasWasm = false;
        bool hasDependencyJs = false;
    };

    static DecoderPaths pathsForObjectForm();
    static DecoderPaths pathsForStringForm();

    // Whether the plain JavaScript fallback stays available after the object form, which it does
    // not.
    static bool objectFormKeepsFallback();
};
