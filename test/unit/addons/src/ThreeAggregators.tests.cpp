#include "Three.Core.h"
#include "Three.TSL.h"
#include "Three.WebGPU.Nodes.h"
#include "Three.WebGPU.h"
#include "nodes/TSL.h"

#include "catch.hpp"

// The three aggregator headers have no behaviour of their own; including them from a translation unit
// proves that the core entry point they share compiles and that the aggregators do not duplicate
// symbols.
TEST_CASE("Three aggregator headers include the core surface") {
    REQUIRE(true);
}
