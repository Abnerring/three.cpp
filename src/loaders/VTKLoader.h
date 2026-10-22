#pragma once
#include <string>
#include <vector>

// Port of the text scanning in examples/jsm/loaders/VTKLoader.js at the pinned revision. The
// text form of the format is a sequence of keywords and numbers, so the reader starts by
// splitting a line into the values it holds. The binary form is compressed and needs a
// decompressor the port does not carry, so only the text form is portable.
class VTKLoader {
  public:
    // The values on one line, split on whitespace. A token that does not begin with a number
    // reads as NaN, which is what the upstream conversion gives when it meets one.
    static std::vector<double> parseFloats(const std::string& line);

    // How many characters at the front of a line form a word rather than a number. A word is
    // anything that is not a digit, a dot, whitespace or a minus sign.
    static std::size_t wordLength(const std::string& line);
};
