#pragma once
#include <string>
#include <vector>

// Port of examples/jsm/loaders/GCodeLoader.js at the pinned revision. The command state
// machine, the layer detection and the two vertex streams are ported; building the Group of
// LineSegments and applying the -90 degree X rotation are left to the caller.
class GCodeLoader {
  public:
    // Upstream keeps a plain object and clones it per move.
    struct State {
        double x = 0, y = 0, z = 0, e = 0, f = 0;
        bool extruding = false;
        bool relative = false;
        bool extrusionOverride = false;
        bool extrusionRelative = false;
    };

    struct Layer {
        // Positions of extruding moves, three numbers per vertex.
        std::vector<double> vertex;
        // Positions of travel moves, three numbers per vertex.
        std::vector<double> pathVertex;
        double z = 0;
    };

    struct Result {
        std::vector<Layer> layers;
        // The two streams concatenated across layers, as upstream does when not splitting.
        std::vector<double> vertex;
        std::vector<double> pathVertex;
        // The final state after the whole program, useful for tests and for callers.
        State finalState;
    };

    bool splitLayer = false;

    Result parse(const std::string& data) const;
};
