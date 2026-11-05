#include "loaders/materialx/MaterialXLog.h"

#include "catch.hpp"

TEST_CASE("MaterialXLog tracks entries and filters them by severity") {
    MaterialXLog log;
    REQUIRE(log.entries.empty());
    REQUIRE(log.errors().empty());
    REQUIRE(log.warnings().empty());

    log.add(MaterialXLogCodes::UNSUPPORTED_NODE, "node is unsupported");
    log.add(MaterialXLogCodes::IGNORED_SURFACE_INPUT, "surface input ignored", "surfaceShader");
    log.add(MaterialXLogCodes::TYPE_MISMATCH, "type mismatch");

    REQUIRE(log.entries.size() == 3);
    REQUIRE(log.errors().size() == 2);
    REQUIRE(log.warnings().size() == 1);
}

TEST_CASE("MaterialXLog records the code label and severity, and omits a missing node name") {
    MaterialXLog log;
    log.add(MaterialXLogCodes::MISSING_REFERENCE, "reference missing");
    log.add(MaterialXLogCodes::TEXTURE_LOAD_FAILED, "texture failed", "imageNode");

    REQUIRE(log.entries.size() == 2);
    const auto& plain = log.entries[0];
    REQUIRE(plain.code == "missing-reference");
    REQUIRE(plain.severity == "error");
    REQUIRE(plain.message == "reference missing");
    REQUIRE_FALSE(plain.nodeName.has_value());

    const auto& named = log.entries[1];
    REQUIRE(named.code == "texture-load-failed");
    REQUIRE(named.severity == "warning");
    REQUIRE(named.nodeName.has_value());
    REQUIRE(*named.nodeName == "imageNode");

    // Every code in the table carries the pinned label and severity.
    REQUIRE(std::string(MaterialXLogCodes::UNSUPPORTED_NODE.label) == "unsupported-node");
    REQUIRE(std::string(MaterialXLogCodes::UNKNOWN_INPUT.severity) == "error");
    REQUIRE(std::string(MaterialXLogCodes::INVALID_OUTPUT_CONNECTION.label) ==
            "invalid-output-connection");
}
