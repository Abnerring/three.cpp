#pragma once
#include <string>

// Port of the rules in examples/jsm/loaders/USDZLoader.js at the pinned revision. The file is a
// deprecation shim and nothing else: it has no parsing of its own and forwards everything to the
// loader it was renamed from.
class USDZLoader {
  public:
    static constexpr const char* kTypeName = "USDZLoader";

    // The sentence printed when one is made. It has no capital in three's own wording beyond the
    // class name, and it names both the old class and the one to use instead.
    static std::string deprecationWarning();

    // Whether the warning is printed before the parent is initialised. It is: the call to the
    // parent comes after the message, so a caller who has asked for the deprecated name hears
    // about it even if the parent's own initialisation then fails. That order is unusual in a
    // constructor and is what makes the shim worth a test of its own.
    static bool warnsBeforeParentInitialises();

    // Whether the shim does any loading of its own, which it does not: every method it has comes
    // from the loader it extends.
    static bool parsesAnythingItself();
};
