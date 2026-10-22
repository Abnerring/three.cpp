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
};
