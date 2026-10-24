#pragma once
#include <string>

// Port of the constant generation and the format step in src/nodes/core/NodeBuilder.js at the
// pinned revision. Five of their rules are worth writing down, including one that does nothing.
class NodeBuilderFormat {
  public:
    // The value a constant of a type starts from when the caller gives none. The scalar types
    // start at nought, a boolean at false, and each vector length has four spellings that all start
    // from the same vector; a type none of those covers keeps nothing at all, so a matrix asked for
    // without a value is handed nothing rather than a zero matrix, and the code that follows would
    // read through it.
    static bool startsWithAValue(const std::string& type);

    // Whether every spelling of a vector length starts from the same value. Each of the four kinds
    // has its own spelling at each length, and all four are listed, so the list is longer than a
    // reader counting only the plain vectors would write.
    static int spellingsPerVectorLength();

    // What an unsigned constant is written as. A value that is not negative is written with a u
    // after it; a negative one is written as nought with a u after it, so a negative number becomes
    // zero rather than its own size. That is the same choice the constant node makes, and it is a
    // clamp rather than an absolute value.
    static std::string unsignedText(long long value);

    // Whether a negative unsigned constant becomes its size, which it does not.
    static bool negativeUnsignedKeepsItsSize();

    // The three conditions that let a snippet through unchanged. The two types are first turned into
    // their vector forms, and then the snippet is returned as it stands when they agree, when
    // nothing was asked for, or when what was asked for is a reference rather than a value. So a
    // caller naming no type gets no conversion and is not told that none was possible.
    static bool passesThrough(const std::string& fromType, bool toTypeIsNull,
                              const std::string& toType, bool vectorTypesAgree,
                              bool toTypeIsReference);

    // What happens to a value longer than four parts. The source leaves a note saying to ignore it
    // for now and does nothing further, so a matrix being formatted into something else is not
    // converted at all and the snippet is returned as it came. That is a gap in the source rather
    // than a rule of it, and copying it means the port has the same gap.
    static bool longValuesAreIgnored();

    // Whether a sixteen part value is written from its first three rows, which is how a four by four
    // matrix becomes a three by three one: three rows of three parts each, taken from the first
    // three entries of each row.
    static bool sixteenToNineUsesThreeRows();
};
