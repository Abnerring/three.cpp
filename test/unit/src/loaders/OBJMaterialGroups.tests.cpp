#include "loaders/OBJMaterialGroups.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("OBJMaterialGroups assigns contiguous vertex ranges") {
    OBJMaterialGroups groups;
    const auto& first = groups.startMaterial("A", {});
    REQUIRE(first.name == "A");
    REQUIRE(first.index == 0);
    REQUIRE(first.groupStart == 0);
    REQUIRE(first.groupEnd == -1);

    // Three vertices belong to A.
    groups.finalize(false, 9);
    REQUIRE(groups.materials()[0].groupEnd == 3);
    REQUIRE(groups.materials()[0].groupCount == 3);

    // B starts where A ended.
    const auto& second = groups.startMaterial("B", {});
    REQUIRE(second.index == 1);
    REQUIRE(second.groupStart == 3);
    groups.finalize(false, 15);
    REQUIRE(groups.materials()[1].groupEnd == 5);
    REQUIRE(groups.materials()[1].groupCount == 2);
    REQUIRE(groups.materials().size() == 2);
}

TEST_CASE("OBJMaterialGroups drops a trailing material that covered nothing") {
    OBJMaterialGroups groups;
    groups.startMaterial("A", {});
    groups.finalize(false, 9);
    groups.startMaterial("B", {});
    // B never sees a face, so closing the object removes it again.
    groups.finalize(true, 9);
    REQUIRE(groups.materials().size() == 1);
    REQUIRE(groups.materials()[0].name == "A");
}

TEST_CASE("OBJMaterialGroups guarantees one placeholder material") {
    OBJMaterialGroups groups;
    groups.finalize(true, 0);
    // With no material at all, closing still leaves one so the caller has a group to fill.
    REQUIRE(groups.materials().size() == 1);
    REQUIRE(groups.materials()[0].name.empty());
}

TEST_CASE("OBJMaterialGroups keeps the last material library") {
    OBJMaterialGroups groups;
    const auto& material = groups.startMaterial("A", {"one.mtl", "two.mtl"});
    REQUIRE(material.mtllib == "two.mtl");
    const auto& again = groups.startMaterial("B", {});
    REQUIRE(again.mtllib.empty());
}

TEST_CASE("OBJMaterialGroups reports the current material") {
    OBJMaterialGroups groups;
    REQUIRE(groups.currentMaterial() == nullptr);
    groups.startMaterial("A", {});
    REQUIRE(groups.currentMaterial() != nullptr);
    REQUIRE(groups.currentMaterial()->name == "A");
    groups.startMaterial("B", {});
    REQUIRE(groups.currentMaterial()->name == "B");
}
