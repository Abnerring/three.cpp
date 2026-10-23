#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Port of examples/jsm/loaders/PDBLoader.js at the pinned revision. The line parsing, the
// CPK colour table and the bond deduplication are ported; building the two BufferGeometry
// objects is left to the caller, so parse returns the raw arrays.
class PDBLoader {
  public:
    struct Atom {
        double x = 0, y = 0, z = 0;
        // The CPK colour is an empty array for elements the table does not know.
        std::vector<int> cpk;
        std::string element;  // capitalised, as upstream reports it
        std::string symbol;   // lower case lookup key
        int index = 0;        // one based serial from columns 6 to 11
    };
    struct Result {
        std::vector<Atom> atoms;
        // Each bond is three integers: start atom minus one, end atom minus one, order.
        std::vector<std::array<int, 3>> bonds;
    };

    Result parse(const std::string& text) const;

    // The upstream table, keyed by lower case element symbol.
    static const std::unordered_map<std::string, std::array<int, 3>>& cpkTable();
    // The element a line names. The format puts it in two columns near the end, and a file that
    // leaves those blank has it in the two columns after the atom name instead, so the second
    // place is only consulted when the first is empty. The answer is lower cased.
    static std::string elementOf(const std::string& line);

    // The columns a bond line reads. A CONECT line lists at most four partners, at fixed offsets
    // and five columns each, so a line with more than four simply has the rest ignored rather
    // than read on.
    static const std::array<std::size_t, 4>& bondColumns();

    // The atom number a bond refers to, with the format's one based numbering taken off. A number
    // that does not parse gives minus one rather than zero, so a caller can tell it from the
    // first atom.
    static int atomIndex(int oneBased);
    // A string with the spaces taken off both ends.
    static std::string trimmed(const std::string& text);

    // A name with its first letter raised and everything after it lowered, so N and n both become
    // N and NO becomes No. Raising only the first would leave the rest as written.
    static std::string capitalised(const std::string& text);

    // The key a pair of atoms is filed under. The smaller number comes first whichever order the
    // pair is given in, so a bond named from either end lands on the same key and is only
    // recorded once.
    static std::string bondHash(int first, int second);

    // Whether a bond read from a line should be recorded. A partner number of zero means the
    // column was blank and is skipped, and a pair already recorded is dropped rather than added
    // again: the source leaves the line that would have counted repeats commented out, with a
    // note saying most files repeat ordinary bonds rather than meaning higher orders.
    static bool acceptsBond(int partner, bool alreadyRecorded);
};
