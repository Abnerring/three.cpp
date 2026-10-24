#include "nodes/core/NodeTypeCache.h"

#include "catch.hpp"

#include <optional>
#include <string>

using CacheSlot = NodeTypeCache::CacheSlot;
using Source = NodeTypeCache::Source;

TEST_CASE("Node remembers a type in one of two places") {
    // No output named: a single slot for the node.
    REQUIRE(NodeTypeCache::slotFor(false) == CacheSlot::Single);
    // An output named: a map keyed by it, so two outputs are remembered apart.
    REQUIRE(NodeTypeCache::slotFor(true) == CacheSlot::PerOutput);
    REQUIRE(NodeTypeCache::slotFor(true) != NodeTypeCache::slotFor(false));
}

TEST_CASE("Node uses a remembered type and works out a missing one") {
    // A remembered entry is used as it stands.
    const auto hit = NodeTypeCache::lookup(std::string("vec3"), "float");
    REQUIRE(hit.wasRemembered);
    REQUIRE(hit.type == "vec3");
    // A miss is worked out and the computed value is the answer for this call.
    const auto miss = NodeTypeCache::lookup(std::nullopt, "float");
    REQUIRE_FALSE(miss.wasRemembered);
    REQUIRE(miss.type == "float");
    // So the two are distinguishable by the flag even when the answers agree, which they do here
    // by construction.
    REQUIRE(hit.wasRemembered != miss.wasRemembered);
}

TEST_CASE("Node remembers an empty type rather than redoing the work") {
    // The entry exists, so the test is against absence rather than against a falsy value; the
    // empty type is remembered and found next time.
    const auto empty = NodeTypeCache::lookup(std::string(""), "vec3");
    REQUIRE(empty.wasRemembered);
    REQUIRE(empty.type.empty());
    REQUIRE(empty.type != "vec3");
    REQUIRE(NodeTypeCache::emptyTypeIsRemembered());
}

TEST_CASE("Node's default type computation passes the question on") {
    // With an output node of its own the node answers with what it wraps rather than calculating.
    REQUIRE(NodeTypeCache::sourceFor(true) == Source::DelegatedToOutputNode);
    // Without one it answers its own declared type.
    REQUIRE(NodeTypeCache::sourceFor(false) == Source::OwnDeclaredType);
    REQUIRE(NodeTypeCache::sourceFor(true) != NodeTypeCache::sourceFor(false));
    // And the output is passed along, so the wrapped node is asked the same question.
    REQUIRE(NodeTypeCache::outputIsPassedThrough());
}