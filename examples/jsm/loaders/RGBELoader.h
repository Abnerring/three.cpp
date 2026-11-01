#pragma once
#include "loaders/HDRLoader.h"

#include <string>

// Port of examples/jsm/loaders/RGBELoader.js at the pinned revision. Upstream deprecated it
// in r180 in favour of HDRLoader and only warns from the constructor, which is what this
// does, so the parsing behaviour is exactly HDRLoader's.
class RGBELoader : public HDRLoader {
  public:
    RGBELoader();
    // The warning upstream prints once per construction.
    static const char* deprecatedMessage();
};
